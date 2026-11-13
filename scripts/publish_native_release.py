#!/usr/bin/env python3
"""Publish only successfully tested native artifacts matching the release source."""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
from pathlib import Path
from build_native import source_fingerprint
from verify_release import verify


def command(*args):
    return subprocess.check_output(list(map(str, args)), text=True).strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag', required=True)
    parser.add_argument('--artifacts', type=Path, required=True)
    parser.add_argument('--result', type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'v\d+\.\d+\.\d+', args.tag):
        raise ValueError('Invalid release tag')
    verify(args.tag)
    version = args.tag[1:]
    repository = os.environ['GITHUB_REPOSITORY']
    commit = command('git', 'rev-parse', f'{args.tag}^{{commit}}')
    fingerprint = source_fingerprint(Path('.'), commit)
    expected = ['linux-x86_64', 'windows-x86_64', 'macos-universal']
    metadata, uploads = [], []
    for platform in expected:
        matches = list(args.artifacts.rglob(f'build-info-{platform}.json'))
        if len(matches) != 1:
            raise ValueError(f'Missing or ambiguous native artifact: {platform}')
        record = json.loads(matches[0].read_text())
        if (record['version'] != version or record['platform'] != platform
                or record['source_fingerprint'] != fingerprint
                or record['tests']['passed'] < (8 if platform.startswith('linux') else 6)
                or record['tests']['failed'] != 0 or record['tests']['skipped'] != 0):
            raise ValueError(f'Native build does not match validated release source: {platform}')
        name = record['archive']
        if Path(name).name != name or not name.startswith(f'logisth-{version}-{platform}.'):
            raise ValueError('Invalid archive name')
        archive = matches[0].parent / name
        if hashlib.sha256(archive.read_bytes()).hexdigest() != record['archive_sha256']:
            raise ValueError(f'Corrupt artifact: {name}')
        uploads += [archive, matches[0]]
        metadata.append(record)
    release = subprocess.run(['gh', 'api', f'repos/{repository}/releases/tags/{args.tag}'],
                             capture_output=True, text=True)
    if release.returncode:
        if 'HTTP 404' not in release.stderr:
            raise RuntimeError(release.stderr)
        subprocess.run(['gh', 'release', 'create', args.tag, '--verify-tag', '--title',
                        f'logisth {version}', '--notes-file', f'docs/RELEASE-{args.tag}.md',
                        '--repo', repository], check=True)
        release_info = json.loads(command('gh', 'api', f'repos/{repository}/releases/tags/{args.tag}'))
    else:
        release_info = json.loads(release.stdout)
    existing = Path('release-asset-checksums')
    existing.mkdir(exist_ok=True)
    if release_info.get('assets'):
        for item in release_info['assets']:
            if not re.fullmatch(r'[A-Za-z0-9_.-]+', item['name']):
                raise ValueError('Unsafe existing release asset name')
        subprocess.run(['gh', 'release', 'download', args.tag, '--repo', repository,
                        '--dir', str(existing), '--clobber'], check=True)
    # Keep the demo, screenshots and source archive available even on a first release.
    original = Path('releases') / f'logisth-{version}'
    replacement_names = {p.name for p in uploads}
    for path in original.iterdir():
        if path.is_file() and path.name not in {'README.md', 'SHA256SUMS'} and path.name not in replacement_names and not (existing / path.name).exists():
            shutil.copy2(path, existing / path.name)
            uploads.append(path)
    for path in uploads:
        shutil.copy2(path, existing / path.name)
    manifest = existing / 'SHA256SUMS'
    manifest.write_text(''.join(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + path.name + '\n'
                                for path in sorted(existing.iterdir())
                                if path.is_file() and path.name != 'SHA256SUMS'))
    uploads.append(manifest)
    # One upload operation after all three builds and checksums have succeeded.
    subprocess.run(['gh', 'release', 'upload', args.tag, *map(str, uploads), '--clobber',
                    '--repo', repository], check=True)
    rows = []
    for record in metadata:
        platform = record['platform']
        requirement = ('glibc >= ' + record['minimum_glibc'] if platform.startswith('linux')
                       else 'macOS 11+, ad-hoc signed, not notarized' if platform.startswith('macos')
                       else 'Windows x64, static MSVC runtime')
        rows.append(f'| {platform} | {record["tests"]["passed"]} / 0 skipped | {requirement} |')
    notes = Path('native-release-notes.md')
    authored_notes = (Path('docs') / f'RELEASE-{args.tag}.md').read_text()
    notes.write_text(authored_notes + f"\n## Native build validation\n\n"
                     + "| Platform | Tests passed | Requirements |\n| --- | --- | --- |\n"
                     + '\n'.join(rows)
                     + f"\n\nSource: `{args.tag}` (`{commit}`). "
                     + f"[Build and validation]({os.environ.get('LOGISTH_RUN_URL', '')}).\n")
    subprocess.run(['gh', 'release', 'edit', args.tag, '--notes-file', str(notes),
                    '--repo', repository], check=True)
    verified = json.loads(command('gh', 'api', f'repos/{repository}/releases/tags/{args.tag}'))
    assets = {item['name']: item for item in verified['assets']}
    for path in uploads:
        if path.name not in assets or assets[path.name]['size'] != path.stat().st_size:
            raise ValueError(f'Release upload verification failed: {path.name}')
    args.result.write_text(json.dumps({'phase': 'published', 'tag': args.tag,
                           'release_url': verified['html_url'], 'release_commit': commit,
                           'platforms': metadata,
                           'assets': [{'name': name, 'size': item['size'],
                                       'url': item['browser_download_url']}
                                      for name, item in sorted(assets.items())]}, indent=2) + '\n')
    print('Published and verified:', verified['html_url'])


if __name__ == '__main__':
    main()

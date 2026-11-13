#!/usr/bin/env python3
"""Build, test and package a native logisth release without signing credentials."""
import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import struct
import subprocess
import sys
import tarfile
import zipfile
from pathlib import Path


def run(command, log):
    print('Running:', ' '.join(map(str, command)), flush=True)
    with log.open('w', encoding='utf-8') as output:
        process = subprocess.Popen(list(map(str, command)), stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True, errors='replace')
        for line in process.stdout:
            print(line, end='', flush=True)
            output.write(line)
        code = process.wait()
    if code:
        raise RuntimeError(f'{command[0]} failed ({code}); see {log.name}')


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def source_fingerprint(root, revision='HEAD'):
    names = ['Source', 'CMakeLists.txt', 'tests']
    if git(root, 'ls-tree', '--name-only', revision, 'assets'):
        names.append('assets')
    return {name: git(root, 'rev-parse', f'{revision}:{name}') for name in names}


def dependencies(folder, logs):
    specs = [('JUCE', 'https://github.com/juce-framework/JUCE.git',
              '51a8a6d7aeae7326956d747737ccf1575e61e209', '8.0.6'),
             ('clap-juce-extensions', 'https://github.com/free-audio/clap-juce-extensions.git',
              '6bde108329ff9fc24404b79b8b397261a75cf6a8', None)]
    folder.mkdir(parents=True, exist_ok=True)
    for name, url, commit, tag in specs:
        target = folder / name
        if not target.exists():
            if tag:
                run(['git', '-c', 'core.longpaths=true', 'clone', '--depth', '1', '--branch', tag,
                     url, target], logs / f'dependency-{name}.log')
            else:
                run(['git', 'init', target], logs / f'dependency-{name}-init.log')
                run(['git', '-C', target, 'remote', 'add', 'origin', url], logs / f'dependency-{name}-remote.log')
                run(['git', '-C', target, 'fetch', '--depth', '1', 'origin', commit], logs / f'dependency-{name}.log')
                run(['git', '-C', target, 'checkout', '--detach', commit], logs / f'dependency-{name}-checkout.log')
        if git(target, 'rev-parse', 'HEAD') != commit:
            raise ValueError(f'Unexpected dependency commit: {name}')
        if name != 'JUCE':
            run(['git', '-C', target, 'submodule', 'update', '--init', '--recursive', '--depth', '1'],
                logs / 'dependency-clap-submodules.log')


def binary_architectures(path, platform):
    with path.open('rb') as stream:
        header = stream.read(64)
        if platform == 'windows-x86_64':
            if header[:2] != b'MZ':
                return None
            stream.seek(struct.unpack_from('<I', header, 60)[0])
            pe = stream.read(6)
            if pe[:4] != b'PE\0\0' or struct.unpack_from('<H', pe, 4)[0] != 0x8664:
                raise ValueError(f'Expected Windows x64 PE: {path}')
            return ['x86_64']
        if platform == 'linux-x86_64':
            if header[:4] != b'\x7fELF':
                return None
            if header[4:6] != b'\x02\x01' or struct.unpack_from('<H', header, 18)[0] != 62:
                raise ValueError(f'Expected Linux x86_64 ELF: {path}')
            return ['x86_64']
        if header[:4] not in (b'\xca\xfe\xba\xbe', b'\xca\xfe\xba\xbf',
                              b'\xcf\xfa\xed\xfe', b'\xfe\xed\xfa\xcf'):
            return None
        architectures = subprocess.check_output(['lipo', '-archs', str(path)], text=True).split()
        if set(architectures) != {'arm64', 'x86_64'}:
            raise ValueError(f'Expected a Universal macOS binary: {path}: {architectures}')
        return sorted(architectures)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, default=Path('.'))
    parser.add_argument('--build', type=Path, default=Path('build'))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--platform', required=True,
                        choices=['linux-x86_64', 'windows-x86_64', 'macos-universal'])
    parser.add_argument('--dependencies', type=Path)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--parallel', type=int, default=2)
    parser.add_argument('--reuse', action='store_true')
    args = parser.parse_args()
    source, build, output = args.source.resolve(), args.build.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    logs = output / 'logs'
    logs.mkdir(exist_ok=True)
    if git(source, 'status', '--porcelain', '--', 'Source', 'CMakeLists.txt', 'tests', 'assets'):
        raise ValueError('Compiled source or tests differ from the recorded Git revision')
    version = re.search(r'project\(Logisth VERSION ([\d.]+)',
                        (source / 'CMakeLists.txt').read_text()).group(1)
    existing = output / f'build-info-{args.platform}.json'
    if args.reuse and existing.is_file():
        record = json.loads(existing.read_text())
        fingerprint = source_fingerprint(source)
        name = record.get('archive', '')
        expected_tests = 8 if args.platform.startswith('linux') else 6
        valid = (record.get('version') == version and record.get('platform') == args.platform
                 and record.get('source_fingerprint') == fingerprint
                 and record.get('tests', {}).get('passed', 0) >= expected_tests
                 and record.get('tests', {}).get('failed') == 0
                 and record.get('tests', {}).get('skipped') == 0
                 and Path(name).name == name and (output / name).is_file())
        if valid and hashlib.sha256((output / name).read_bytes()).hexdigest() == record['archive_sha256']:
            record['reuse_validated_in_run'] = os.environ.get('LOGISTH_RUN_URL', '')
            existing.write_text(json.dumps(record, indent=2) + '\n')
            print(f'Reused {name}: source identity, {expected_tests} passed tests and checksum verified', flush=True)
            return
        print('Existing artifact differs from the source or failed validation; rebuilding', flush=True)
    deps = args.dependencies.resolve() if args.dependencies else build / 'dependencies'
    dependencies(deps, logs)
    configure = [args.cmake, '-S', source, '-B', build, '-DCMAKE_BUILD_TYPE=Release',
                 '-DBUILD_TESTING=ON', '-DLOGISTH_PLUGINS=ON', '-DLOGISTH_VST2=OFF',
                 f'-DFETCHCONTENT_SOURCE_DIR_JUCE={deps / "JUCE"}',
                 f'-DFETCHCONTENT_SOURCE_DIR_CLAP_JUCE_EXTENSIONS={deps / "clap-juce-extensions"}']
    if args.platform == 'macos-universal':
        configure += ['-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64', '-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0']
    if args.platform == 'windows-x86_64':
        configure += ['-A', 'x64', '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>']
    run(configure, logs / 'configure.log')
    run([args.cmake, '--build', build, '--config', 'Release', '--parallel', str(args.parallel)], logs / 'build.log')
    ctest = Path(args.cmake).with_name('ctest' + ('.exe' if os.name == 'nt' else ''))
    ctest_command = str(ctest) if ctest.is_file() else 'ctest'
    result_file = logs / 'test-results.xml'
    run([ctest_command, '--test-dir', build, '-C', 'Release', '--output-on-failure',
         '--no-tests=error', '--output-junit', result_file], logs / 'tests.log')
    import xml.etree.ElementTree as ET
    tests = ET.parse(result_file).getroot()
    count = int(tests.attrib.get('tests', '0'))
    if count < (8 if args.platform == 'linux-x86_64' else 6):
        raise ValueError('Required tests did not execute')
    if any(int(tests.attrib.get(key, '0')) for key in ('failures', 'errors', 'skipped', 'disabled')):
        raise ValueError('Failed, skipped or disabled tests prevent packaging')
    artifacts = build / 'Logisth_artefacts' / 'Release'
    stage = build / 'native-package' / args.platform / 'logisth'
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    for format_name in ['VST3', 'CLAP', 'Standalone']:
        original = artifacts / format_name
        if not original.is_dir() or not any(original.iterdir()):
            raise ValueError(f'Missing plug-in format: {format_name}')
        shutil.copytree(original, stage / format_name, symlinks=True)
    if args.platform == 'macos-universal':
        for pattern in ['VST3/*.vst3', 'CLAP/*.clap', 'Standalone/*.app']:
            bundles = list(stage.glob(pattern))
            if len(bundles) != 1:
                raise ValueError(f'Expected one macOS bundle: {pattern}')
            bundle = bundles[0]
            run(['codesign', '--force', '--deep', '--sign', '-', bundle], logs / f'sign-{bundle.suffix}.log')
            run(['codesign', '--verify', '--deep', '--strict', bundle], logs / f'verify-{bundle.suffix}.log')
    binaries = []
    for format_name in ['VST3', 'CLAP', 'Standalone']:
        found = []
        for path in (stage / format_name).rglob('*'):
            if path.is_file() and not path.is_symlink():
                arch = binary_architectures(path, args.platform)
                if arch:
                    found.append({'path': path.relative_to(stage).as_posix(),
                                  'architectures': arch,
                                  'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
        if not found:
            raise ValueError(f'No native executable found in {format_name}')
        binaries += found
    info = {'version': version, 'platform': args.platform,
            'source_commit': git(source, 'rev-parse', 'HEAD'),
            'source_fingerprint': source_fingerprint(source),
            'tests': {'passed': count, 'failed': 0, 'skipped': 0}, 'binaries': binaries,
            'formats': ['VST3', 'CLAP', 'Standalone'],
            'run_url': os.environ.get('LOGISTH_RUN_URL', ''),
            'signing': 'ad-hoc, not notarized' if args.platform == 'macos-universal' else 'unsigned'}
    if args.platform == 'linux-x86_64':
        requirements = set()
        for binary in binaries:
            symbols = subprocess.check_output(['objdump', '-T', str(stage / binary['path'])], text=True)
            requirements.update(re.findall(r'GLIBC_([\d.]+)', symbols))
        info['minimum_glibc'] = max(requirements, key=lambda s: tuple(map(int, s.split('.'))))
    metadata = json.dumps(info, indent=2) + '\n'
    (stage / 'BUILD_INFO.json').write_text(metadata)
    (output / f'build-info-{args.platform}.json').write_text(metadata)
    for name in ['README.md', 'docs/IMPLEMENTATION.md', 'docs/STUDIO-0.5.md', 'docs/INTERFACE.md', 'docs/MANUAL.md', 'docs/logisth-Manual-English.pdf',
                 'assets/fonts/LICENSE.txt', 'assets/fonts/NOTICE.txt']:
        if not (source / name).is_file():
            continue
        destination = stage / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / name, destination)
    name = f'logisth-{version}-{args.platform}'
    if args.platform == 'linux-x86_64':
        archive_path = output / f'{name}.tar.gz'
        with tarfile.open(archive_path, 'w:gz') as archive:
            archive.add(stage, arcname='logisth')
    else:
        archive_path = output / f'{name}.zip'
        with zipfile.ZipFile(archive_path, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(stage.rglob('*')):
                relative = path.relative_to(stage.parent).as_posix()
                if path.is_symlink():
                    item = zipfile.ZipInfo(relative)
                    item.create_system = 3
                    item.external_attr = (stat.S_IFLNK | 0o777) << 16
                    archive.writestr(item, os.readlink(path))
                elif path.is_file():
                    archive.write(path, relative)
    info['archive'] = archive_path.name
    info['archive_sha256'] = hashlib.sha256(archive_path.read_bytes()).hexdigest()
    (output / f'build-info-{args.platform}.json').write_text(json.dumps(info, indent=2) + '\n')
    print(f'Packaged {archive_path.name}: {count} tests passed; architectures verified', flush=True)


if __name__ == '__main__':
    main()

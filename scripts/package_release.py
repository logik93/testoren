#!/usr/bin/env python3
"""Prepare a new release's source, manual, demo, themes and calibration assets."""
import argparse
import gzip
import hashlib
import io
import re
import shutil
import subprocess
import tarfile
import zipfile
from pathlib import Path


def package(preview: Path, calibration: Path) -> Path:
    root = Path(__file__).resolve().parents[1]
    version = re.search(r'project\(Logisth VERSION ([\d.]+)',
                        (root / 'CMakeLists.txt').read_text()).group(1)
    folder = root / 'releases' / f'logisth-{version}'
    if folder.exists():
        raise ValueError('Refusing to replace an existing release asset folder')
    paths = ['.github', '.gitignore', 'CMakeLists.txt', 'README.md', 'LICENSE',
             'THIRD_PARTY_NOTICES.md', 'Source', 'tests', 'scripts', 'assets', 'docs', 'licenses']
    subprocess.run(['git', 'diff', '--exit-code', 'HEAD', '--', *paths], cwd=root,
                   check=True, stdout=subprocess.DEVNULL)
    if subprocess.check_output(['git', 'ls-files', '--others', '--exclude-standard', '--', *paths], cwd=root):
        raise ValueError('Commit all new source/documentation files before packaging')
    source = subprocess.check_output(['git', 'archive', '--format=tar',
                                     '--prefix=logisth/', 'HEAD', *paths], cwd=root)
    # Prove the committed archive contains the exact source/docs being packaged.
    with tarfile.open(fileobj=io.BytesIO(source)) as archive:
        for member in archive.getmembers():
            if member.isfile():
                local = root / member.name.removeprefix('logisth/')
                if archive.extractfile(member).read() != local.read_bytes():
                    raise ValueError(f'Source archive differs from working copy: {local}')
    copies = [(root / 'docs/MANUAL.md', 'logisth-Manual-English.md'),
              (root / 'docs/logisth-Manual-English.pdf', 'logisth-Manual-English.pdf'),
              (root / 'docs/CALIBRATION.md', 'logisth-GainCalibration.md'),
              (root / 'LICENSE', 'logisth-LICENSE-MIT.txt'),
              (root / 'THIRD_PARTY_NOTICES.md', 'logisth-ThirdPartyNotices.md'),
              (preview / 'logisth-demo.wav', 'logisth-demo.wav'),
              (root / 'docs/images/logisth-dark.png', 'logisth.png')]
    copies += [(p, p.name) for p in sorted((root / 'docs/images').glob('*.png'))]
    calibration_files = [(p, p.name) for p in sorted(calibration.iterdir()) if p.is_file()]
    calibration_files += [(root / p, Path(p).name) for p in (
        'docs/CALIBRATION.md', 'docs/ANTICLICK.md',
        'docs/calibration/before-0.5.1.csv', f'docs/calibration/after-{version}.csv')]
    theme_files = [(p, p.name) for p in sorted((root / 'assets/themes').glob('*.json'))]
    theme_files += [(root / 'docs/THEMES.md', 'README.md'), (root / 'LICENSE', 'LICENSE')]
    for file, _ in copies + calibration_files + theme_files:
        if not file.is_file():
            raise ValueError(f'Missing release input: {file}')
    for name in ['measurements.csv', 'logisth-sine.wav', 'reference-sine-0dBFS.wav',
                 'Calibration - sine 0 dBFS.logisth']:
        if not (calibration / name).is_file():
            raise ValueError(f'Missing calibration input: {name}')
    if len(list((root / 'assets/themes').glob('*.json'))) != 5:
        raise ValueError('All five standard theme templates are required')
    if not (root / 'docs' / f'RELEASE-v{version}.md').is_file():
        raise ValueError('Missing versioned release notes')
    folder.mkdir(parents=True)
    for file, name in copies:
        shutil.copyfile(file, folder / name)
    for label, files in [('calibration', calibration_files), ('themes', theme_files)]:
        with zipfile.ZipFile(folder / f'logisth-{version}-{label}.zip', 'w',
                             compression=zipfile.ZIP_DEFLATED) as archive:
            for file, name in files:
                archive.write(file, name)
    with (folder / f'logisth-{version}-source.tar.gz').open('wb') as stream:
        with gzip.GzipFile(filename='', mode='wb', fileobj=stream, mtime=0) as archive:
            archive.write(source)
    (folder / 'SHA256SUMS').write_text(''.join(
        hashlib.sha256(file.read_bytes()).hexdigest() + '  ' + file.name + '\n'
        for file in sorted(folder.iterdir()) if file.is_file()))
    print(f'Packaged {len(list(folder.iterdir()))} original release files: {folder}')
    return folder


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preview', type=Path, required=True)
    parser.add_argument('--calibration', type=Path, required=True)
    args = parser.parse_args()
    try:
        package(args.preview.resolve(), args.calibration.resolve())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error)) from error

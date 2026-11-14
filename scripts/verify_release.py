#!/usr/bin/env python3
"""Verify the checked-in release assets before GitHub Actions publishes them."""
import hashlib
import re
import sys
import tarfile
import zipfile
from pathlib import Path


def verify(tag: str) -> None:
    if not re.fullmatch(r"v\d+\.\d+\.\d+", tag):
        raise ValueError("Expected a version tag such as v0.2.0")
    root = Path(__file__).resolve().parents[1]
    version = tag[1:]
    project = re.search(r"project\(Logisth VERSION ([\d.]+)",
                        (root / "CMakeLists.txt").read_text())
    if project is None or project.group(1) != version:
        raise ValueError("Tag and CMake project version do not match")
    folder = root / "releases" / f"logisth-{version}"
    manifest = folder / "SHA256SUMS"
    names = set()
    for line in manifest.read_text().splitlines():
        checksum, name = line.split("  ", 1)
        if (not re.fullmatch(r"[a-f0-9]{64}", checksum)
                or Path(name).name != name or name in names):
            raise ValueError("Invalid or duplicate checksum entry")
        asset = folder / name
        if asset.is_symlink() or not asset.is_file():
            raise ValueError(f"Missing regular asset: {name}")
        if hashlib.sha256(asset.read_bytes()).hexdigest() != checksum:
            raise ValueError(f"Checksum mismatch: {name}")
        names.add(name)
    actual = {p.name for p in folder.iterdir()
              if p.is_file() and p.name not in {"SHA256SUMS", "README.md"}}
    required = {f"logisth-{version}-source.tar.gz", "logisth-demo.wav", "logisth.png"}
    required |= {"logisth-Manual-English.pdf", "logisth-Manual-English.md",
                 "logisth-LICENSE-MIT.txt", "logisth-ThirdPartyNotices.md",
                 f"logisth-{version}-themes.zip", f"logisth-{version}-calibration.zip"}
    if names != actual or not required.issubset(names):
        raise ValueError("Release manifest does not cover all expected assets")
    if not (root / "docs" / f"RELEASE-{tag}.md").is_file():
        raise ValueError("Missing release notes")
    with tarfile.open(folder / f"logisth-{version}-source.tar.gz") as archive:
        for name in ["LICENSE", "THIRD_PARTY_NOTICES.md", "docs/logisth-Manual-English.pdf"]:
            if archive.extractfile("logisth/" + name).read() != (root / name).read_bytes():
                raise ValueError(f"Source archive is missing the current file: {name}")
    if (folder / "logisth-LICENSE-MIT.txt").read_bytes() != (root / "LICENSE").read_bytes():
        raise ValueError("Release MIT license differs from source")
    with zipfile.ZipFile(folder / f"logisth-{version}-themes.zip") as archive:
        for file in (root / "assets/themes").glob("*.json"):
            if archive.read(file.name) != file.read_bytes():
                raise ValueError(f"Theme archive mismatch: {file.name}")
    print(f"Verified {len(names)} release assets for {tag}")


if __name__ == "__main__":
    try:
        verify(sys.argv[1] if len(sys.argv) == 2 else "")
    except (OSError, ValueError) as error:
        raise SystemExit(str(error)) from error

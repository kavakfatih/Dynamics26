#!/usr/bin/env python3
"""Create a deterministic FEMCAE source ZIP and SHA256 sidecar.

The archive does not depend on source file mtimes/ownership. Every entry uses a
fixed timestamp, deterministic order and normalized Unix permissions. An
internal SHA256SUMS.txt is generated from the exact archived source files.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from hashlib import sha256
import os
from pathlib import Path
import re
import stat
import sys
import zipfile

EXCLUDED_DIRS = {
    ".git", "build", "stage", "dist", "__pycache__", ".idea", ".vscode",
}
EXCLUDED_PREFIXES = ("build-", "stage-", "cmake-build-")
EXCLUDED_FILES = {"SHA256SUMS.txt", ".DS_Store", "CMakeCache.txt"}
BINARY_SUFFIXES = (".o", ".obj", ".mod", ".smod", ".so", ".dylib", ".dll")
FIXED_ZIP_TIME = (2026, 1, 1, 0, 0, 0)


@dataclass(frozen=True)
class SourceFile:
    rel: str
    data: bytes
    executable: bool


def infer_version(root: Path) -> str:
    text = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    m = re.search(r"project\s*\(\s*FEMCAE\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", text, re.S)
    if not m:
        raise RuntimeError("CMakeLists.txt PROJECT_VERSION could not be inferred")
    return m.group(1)


def excluded_dir(name: str) -> bool:
    return name in EXCLUDED_DIRS or name.startswith(EXCLUDED_PREFIXES)


def collect(root: Path, output: Path) -> list[SourceFile]:
    result: list[SourceFile] = []
    output = output.resolve()
    output_sha = Path(str(output) + ".sha256").resolve()
    for base, dirs, files in os.walk(root):
        base_path = Path(base)
        dirs[:] = sorted(d for d in dirs if not excluded_dir(d))
        for name in sorted(files):
            path = base_path / name
            resolved = path.resolve()
            if resolved in {output, output_sha}:
                continue
            rel = path.relative_to(root).as_posix()
            if name in EXCLUDED_FILES or name.endswith(BINARY_SUFFIXES):
                continue
            if path.is_symlink():
                raise RuntimeError(f"Symlink is not allowed in release source archive: {rel}")
            data = path.read_bytes()
            executable = bool(path.stat().st_mode & stat.S_IXUSR)
            result.append(SourceFile(rel=rel, data=data, executable=executable))
    result.sort(key=lambda item: item.rel)
    return result


def zip_info(name: str, executable: bool) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, FIXED_ZIP_TIME)
    info.create_system = 3
    mode = 0o755 if executable else 0o644
    info.external_attr = (stat.S_IFREG | mode) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    return info


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default=".")
    parser.add_argument("--output", required=True)
    parser.add_argument("--version")
    parser.add_argument("--write-manifest", action="store_true")
    args = parser.parse_args()

    root = Path(args.source).resolve()
    output = Path(args.output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    version = args.version or infer_version(root)
    prefix = f"FEMCAE-v{version}/"
    files = collect(root, output)

    manifest_lines = [f"{sha256(item.data).hexdigest()}  {item.rel}" for item in files]
    manifest = ("\n".join(manifest_lines) + "\n").encode("utf-8")
    if args.write_manifest:
        (root / "SHA256SUMS.txt").write_bytes(manifest)

    tmp = output.with_suffix(output.suffix + ".tmp")
    if tmp.exists():
        tmp.unlink()
    with zipfile.ZipFile(tmp, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for item in files:
            zf.writestr(zip_info(prefix + item.rel, item.executable), item.data)
        zf.writestr(zip_info(prefix + "SHA256SUMS.txt", False), manifest)
    os.replace(tmp, output)

    digest = sha256(output.read_bytes()).hexdigest()
    sidecar = Path(str(output) + ".sha256")
    sidecar.write_text(f"{digest}  {output.name}\n", encoding="utf-8")
    print(f"archive={output}")
    print(f"sha256={digest}")
    print(f"files={len(files) + 1}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""FEMCAE Git repository/source-tree hygiene gate.

Bu kontrol release/source deposunda derleme ve gizli credential artefaktlarinin
yanlislikla tutulmasini engeller. Git metadata varsa yalniz tracked dosyalar,
yoksa kaynak agaci taranir.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys

FORBIDDEN_PARTS = {
    "build", "stage", "dist", ".idea", ".vscode", "__pycache__",
}
FORBIDDEN_PREFIXES = ("build-", "stage-", "cmake-build-")
FORBIDDEN_SUFFIXES = (
    ".o", ".obj", ".mod", ".smod", ".a", ".so", ".dylib", ".dll",
    ".exe", ".pdb", ".dSYM", ".zip", ".dmg", ".p12", ".p8", ".key",
)
FORBIDDEN_NAMES = {
    "CMakeCache.txt", ".DS_Store", "compile_commands.json",
}
SECRET_NAME_RE = re.compile(r"(^|/)(\.env($|\.)|.*secret.*|.*credential.*)$", re.I)
ALLOW_SECRET_DOCS = {
    ".github/workflows/macos-release.yml",
    "docs/development/V1.0.1_NATIVE_RELEASE_GATES.md",
    "scripts/macos/sign_and_notarize.sh",
}


def git_tracked(root: Path) -> list[str] | None:
    if not (root / ".git").exists():
        return None
    proc = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z"],
        check=True,
        stdout=subprocess.PIPE,
    )
    return [p.decode("utf-8") for p in proc.stdout.split(b"\0") if p]


def tree_files(root: Path) -> list[str]:
    out: list[str] = []
    for base, dirs, files in os.walk(root):
        base_path = Path(base)
        rel_base = base_path.relative_to(root)
        dirs[:] = [d for d in dirs if d != ".git"]
        for name in files:
            rel = (rel_base / name).as_posix()
            if rel.startswith("./"):
                rel = rel[2:]
            out.append(rel)
    return out


def violation(path: str) -> str | None:
    parts = Path(path).parts
    for part in parts[:-1]:
        if part in FORBIDDEN_PARTS or part.startswith(FORBIDDEN_PREFIXES):
            return f"forbidden directory: {part}"
    name = parts[-1]
    if name in FORBIDDEN_NAMES:
        return f"forbidden generated file: {name}"
    if name.endswith(FORBIDDEN_SUFFIXES):
        return f"forbidden binary/credential suffix: {name}"
    if SECRET_NAME_RE.search(path) and path not in ALLOW_SECRET_DOCS:
        return "secret/credential-like filename"
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    root = Path(args.root).resolve()

    files = git_tracked(root)
    mode = "git-tracked" if files is not None else "source-tree"
    if files is None:
        files = tree_files(root)

    failures = [(p, violation(p)) for p in sorted(files) if violation(p)]
    if failures:
        print(f"FAIL repository hygiene ({mode})", file=sys.stderr)
        for path, why in failures:
            print(f"  {path}: {why}", file=sys.stderr)
        return 2

    print(f"PASS repository hygiene ({mode}): {len(files)} files checked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

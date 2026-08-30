#!/usr/bin/env python3
from __future__ import annotations

from hashlib import sha256
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import zipfile


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def main() -> int:
    if len(sys.argv) != 2:
        print("source root argument required", file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    archive_tool = root / "scripts/release/make_source_archive.py"
    hygiene_tool = root / "scripts/github/verify_repository_hygiene.py"
    managed_vscode = {"extensions.json", "launch.json", "settings.json", "tasks.json"}

    for name in managed_vscode:
        path = root / ".vscode" / name
        if not path.is_file():
            raise RuntimeError(f"managed VS Code file missing from source tree: {path}")
        json.loads(path.read_text(encoding="utf-8"))

    tasks = json.loads((root / ".vscode" / "tasks.json").read_text(encoding="utf-8"))["tasks"]
    task_labels = {task.get("label") for task in tasks}
    required_tasks = {
        "Dynamics26: Configure Debug GUI",
        "Dynamics26: Build Debug GUI",
        "Dynamics26: Test Debug GUI",
        "Dynamics26: Configure + Build Debug GUI",
    }
    if not required_tasks.issubset(task_labels):
        raise RuntimeError("required Debug GUI VS Code tasks are incomplete")

    launches = json.loads((root / ".vscode" / "launch.json").read_text(encoding="utf-8"))["configurations"]
    gui_launch = next((item for item in launches if item.get("name") == "Dynamics26 GUI — Debug"), None)
    expected_program = "${workspaceFolder}/build/macos-debug-gui/gui/FEMCAE.app/Contents/MacOS/FEMCAE"
    if (
        gui_launch is None
        or gui_launch.get("type") != "lldb"
        or gui_launch.get("request") != "launch"
        or gui_launch.get("program") != expected_program
        or gui_launch.get("preLaunchTask") != "Dynamics26: Build Debug GUI"
    ):
        raise RuntimeError("CodeLLDB Debug GUI launch profile does not match CMakePresets output")

    extensions = set(
        json.loads((root / ".vscode" / "extensions.json").read_text(encoding="utf-8"))["recommendations"]
    )
    if not {"ms-vscode.cmake-tools", "vadimcn.vscode-lldb"}.issubset(extensions):
        raise RuntimeError("required CMake Tools / CodeLLDB recommendations are missing")

    gui_cmake = (root / "gui" / "CMakeLists.txt").read_text(encoding="utf-8")
    if (
        "check_cxx_source_compiles" not in gui_cmake
        or "FEMCAE_VTK_HAS_CAMERA_ORIENTATION_WIDGET" not in gui_cmake
        or 'VTK_VERSION VERSION_GREATER_EQUAL "9.0"' in gui_cmake
    ):
        raise RuntimeError("orientation widget must use compile capability detection, not a VTK version gate")

    if (root / ".git").exists():
        run([sys.executable, str(hygiene_tool), "--root", str(root)])
    else:
        print("repository hygiene tracked-file gate skipped: source archive has no .git metadata")

    # Proje tarafindan yonetilen VS Code dosyalari izinli; rastgele kullanici
    # dosyalari ise repository hygiene gate tarafindan reddedilmelidir.
    with tempfile.TemporaryDirectory(prefix="femcae-vscode-hygiene-") as vscode_tmp:
        vscode_root = Path(vscode_tmp)
        managed = sorted(managed_vscode)
        (vscode_root / ".vscode").mkdir()
        for name in managed:
            (vscode_root / ".vscode" / name).write_text("{}\n", encoding="utf-8")
        run([sys.executable, str(hygiene_tool), "--root", str(vscode_root)])
        (vscode_root / ".vscode" / "local-state.json").write_text("{}\n", encoding="utf-8")
        rejected = subprocess.run(
            [sys.executable, str(hygiene_tool), "--root", str(vscode_root)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if rejected.returncode == 0:
            raise RuntimeError("unmanaged .vscode file was not rejected")

    with tempfile.TemporaryDirectory(prefix="femcae-v102-") as tmp:
        a = Path(tmp) / "a.zip"
        b = Path(tmp) / "b.zip"
        base = [sys.executable, str(archive_tool), "--source", str(root), "--version", "1.0.2"]
        run(base + ["--output", str(a)])
        run(base + ["--output", str(b)])
        if a.read_bytes() != b.read_bytes():
            raise RuntimeError("deterministic source archives differ byte-for-byte")
        if sha256(a.read_bytes()).digest() != sha256(b.read_bytes()).digest():
            raise RuntimeError("deterministic source archive SHA256 differs")
        with zipfile.ZipFile(a) as zf:
            names = zf.namelist()
            expected = "FEMCAE-v1.0.2/SHA256SUMS.txt"
            if expected not in names:
                raise RuntimeError("internal SHA256SUMS.txt missing")
            expected_vscode = {
                f"FEMCAE-v1.0.2/.vscode/{name}" for name in managed_vscode
            }
            missing_vscode = expected_vscode.difference(names)
            if missing_vscode:
                raise RuntimeError(
                    "managed VS Code files missing from source archive: "
                    + ", ".join(sorted(missing_vscode))
                )
            bad = zf.testzip()
            if bad is not None:
                raise RuntimeError(f"zip CRC failure: {bad}")
            extracted = Path(tmp) / "extracted"
            zf.extractall(extracted)

        repo_root = extracted / "FEMCAE-v1.0.2"
        home = Path(tmp) / "home"
        home.mkdir()
        env = os.environ.copy()
        env["HOME"] = str(home)
        subprocess.run(["git", "config", "--global", "user.name", "FEMCAE CI"], check=True, env=env)
        subprocess.run(["git", "config", "--global", "user.email", "ci@example.invalid"], check=True, env=env)
        subprocess.run(
            ["bash", "scripts/github/bootstrap_repo.sh", "example/FEMCAE", "--no-push"],
            cwd=repo_root,
            check=True,
            env=env,
        )
        head = subprocess.check_output(["git", "rev-parse", "--verify", "HEAD"], cwd=repo_root, env=env).decode().strip()
        if len(head) < 12:
            raise RuntimeError("bootstrap did not create an initial Git commit")
        origin = subprocess.check_output(["git", "remote", "get-url", "origin"], cwd=repo_root, env=env).decode().strip()
        if origin != "https://github.com/example/FEMCAE.git":
            raise RuntimeError(f"unexpected bootstrap remote: {origin}")

    print("PASS V1.0.2 repository/reproducible-source hardening")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

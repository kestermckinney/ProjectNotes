#!/usr/bin/env python3
"""
Copies a filtered subset of a Python site-packages directory to the deploy
destination, then generates NSIS install_files.nsh and remove_files.nsh for
the result.

Usage (called by CMake deploy target):
    python collect_packages.py
        --source  <site-packages dir>
        --dest    <deploy/site-packages dir>
        --nsis-install   <path/to/install_files.nsh>
        --nsis-uninstall <path/to/remove_files.nsh>
"""

import argparse
import os
import shutil
import sys

# Directory names skipped at any level of the tree
SKIP_DIRS = {
    "__pycache__",
    "test",
    "tests",
    # PyInstaller and its graph library — build tools, not runtime
    "PyInstaller",
    "_pyinstaller_hooks_contrib",
    "altgraph",
    # Documentation tools
    "mkdocs",
    "mkdocs_get_deps",
    "mkdocs_material",
    "mkdocs_material_extensions",
    # YAML (only needed by mkdocs)
    "yaml",
    "_yaml",
    # Package management tools — not needed at runtime
    "pip",
    "setuptools",
    "wheel",
    "pkg_resources",
    # adodbapi ships tests/examples that aren't needed
    "adodbapi",
}

# Directory name suffixes that are always skipped (.dist-info contains pip
# metadata only; .data directories are installer scratch space)
SKIP_SUFFIXES = (".dist-info", ".data")


def should_skip_dir(name: str) -> bool:
    if name in SKIP_DIRS:
        return True
    for suffix in SKIP_SUFFIXES:
        if name.endswith(suffix):
            return True
    return False


def copy_filtered(src: str, dst: str) -> tuple[int, int]:
    """
    Recursively copy src to dst, skipping blacklisted directories.
    Returns (files_copied, dirs_skipped).
    """
    os.makedirs(dst, exist_ok=True)
    files_copied = 0
    dirs_skipped = 0

    for entry in sorted(os.scandir(src), key=lambda e: e.name):
        if entry.is_dir(follow_symlinks=False):
            if should_skip_dir(entry.name):
                print(f"  skip  {entry.name}/")
                dirs_skipped += 1
            else:
                fc, ds = copy_filtered(entry.path, os.path.join(dst, entry.name))
                files_copied += fc
                dirs_skipped += ds
        else:
            shutil.copy2(entry.path, os.path.join(dst, entry.name))
            files_copied += 1

    return files_copied, dirs_skipped


def generate_nsis(staged_dir: str, install_path: str, uninstall_path: str) -> None:
    """
    Walk staged_dir and write NSIS File/Delete/RMDir directives.
    Install paths use the staged file's absolute path as the source so NSIS
    can compile the installer from anywhere.
    Uninstall paths use $INSTDIR\\site-packages\\... as the target.
    """
    NSIS_BASE = r"$INSTDIR\site-packages"

    install_lines: list[str] = []
    remove_lines: list[str] = []
    visited_nsis_dirs: list[str] = []

    for root, dirs, files in os.walk(staged_dir, topdown=True):
        # Prune __pycache__ from the walk (shouldn't be there, but just in case)
        dirs[:] = sorted(d for d in dirs if d != "__pycache__")

        rel = os.path.relpath(root, staged_dir)
        if rel == ".":
            nsis_out = NSIS_BASE
        else:
            nsis_out = NSIS_BASE + "\\" + rel  # rel already uses backslashes on Windows

        visited_nsis_dirs.append(nsis_out)

        if files:
            install_lines.append(f'   SetOutPath "{nsis_out}"')
            for fname in sorted(files):
                abs_src = os.path.join(root, fname)
                install_lines.append(f'   File "{abs_src}"')
                remove_lines.append(f'   Delete "{nsis_out}\\{fname}"')

    # RMDir deepest-first so parent directories are empty before removal
    for nsis_dir in reversed(visited_nsis_dirs):
        remove_lines.append(f'   Delete "{nsis_dir}\\__pycache__\\*.*"')
        remove_lines.append(f'   RMDir  "{nsis_dir}\\__pycache__"')
        remove_lines.append(f'   RMDir  "{nsis_dir}"')

    with open(install_path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\r\n".join(install_lines) + "\r\n")

    with open(uninstall_path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\r\n".join(remove_lines) + "\r\n")

    print(f"  install_files.nsh  : {len(install_lines)} lines")
    print(f"  remove_files.nsh   : {len(remove_lines)} lines")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Filter site-packages and generate NSIS include files."
    )
    parser.add_argument("--source",          required=True, action="append",
                        help="Python site-packages source directory (may be repeated)")
    parser.add_argument("--dest",            required=True,
                        help="Destination inside the deploy staging directory")
    parser.add_argument("--nsis-install",    required=True,
                        help="Output path for install_files.nsh")
    parser.add_argument("--nsis-uninstall",  required=True,
                        help="Output path for remove_files.nsh")
    args = parser.parse_args()

    sources = [os.path.abspath(s) for s in args.source]
    dst     = os.path.abspath(args.dest)

    for src in sources:
        if not os.path.isdir(src):
            print(f"ERROR: source directory not found: {src}", file=sys.stderr)
            return 1

    if os.path.exists(dst):
        shutil.rmtree(dst)

    total_files, total_skipped = 0, 0
    for src in sources:
        print(f"Copying filtered packages")
        print(f"  from : {src}")
        print(f"  to   : {dst}")
        fc, ds = copy_filtered(src, dst)
        total_files   += fc
        total_skipped += ds
        print(f"  done : {fc} files copied, {ds} directories skipped")

    files_copied, dirs_skipped = total_files, total_skipped

    print("Generating NSIS include files")
    generate_nsis(dst, args.nsis_install, args.nsis_uninstall)

    return 0


if __name__ == "__main__":
    sys.exit(main())

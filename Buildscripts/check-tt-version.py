#!/usr/bin/env python3
"""Fails when the files of tt changed since a base revision, but TT_VERSION in tt.py wasn't increased.
Usage: check-tt-version.py <base revision>, e.g. origin/main"""

import importlib.util
import os
import re
import subprocess
import sys

ROOT_PATH = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
VERSION_PATTERN = re.compile(r'^TT_VERSION = "([^"]+)"', re.MULTILINE)


def load_file_names():
    """The files that make up tt, as release-tt.py packages them"""
    spec = importlib.util.spec_from_file_location("release_tt", os.path.join(ROOT_PATH, "Buildscripts", "release-tt.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.FILE_NAMES


def git(*arguments):
    return subprocess.run(["git", *arguments], cwd=ROOT_PATH, capture_output=True, text=True)


def read_version(source):
    match = VERSION_PATTERN.search(source)
    return match.group(1) if match else None


def parse_version(version):
    return tuple(int(part) for part in re.findall(r"\d+", version)[:3])


def main():
    if len(sys.argv) != 2:
        print("Usage: check-tt-version.py <base revision>", file=sys.stderr)
        sys.exit(2)
    base = sys.argv[1]
    file_names = load_file_names()
    diff = git("diff", "--name-only", f"{base}...HEAD", "--", *file_names)
    if diff.returncode != 0:
        print(f"git diff failed: {diff.stderr.strip()}", file=sys.stderr)
        sys.exit(2)
    changed = diff.stdout.split()
    if not changed:
        print("tt is unchanged")
        return
    base_source = git("show", f"{base}:tt.py")
    if base_source.returncode != 0:
        print("tt is new")
        return
    with open(os.path.join(ROOT_PATH, "tt.py")) as file:
        current_version = read_version(file.read())
    base_version = read_version(base_source.stdout)
    if current_version is None or base_version is None:
        print("TT_VERSION not found in tt.py", file=sys.stderr)
        sys.exit(1)
    if parse_version(current_version) <= parse_version(base_version):
        print(f"{', '.join(changed)} changed, but TT_VERSION is still {current_version}: increase TT_VERSION in tt.py", file=sys.stderr)
        sys.exit(1)
    print(f"tt changed and its version increased from {base_version} to {current_version}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Exports the CDN files of tt to release/tt/: tt.zip with tt.py and its wrappers, and tt.json with the version. They are uploaded to sdk/ on the CDN."""

import json
import os
import re
import shutil
import sys
import zipfile

ROOT_PATH = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
LAUNCHER_PATH = os.path.join(ROOT_PATH, "tt.py")
# tt.py and the wrappers that run it: tt (Linux, macOS) and tt.cmd (Windows)
FILE_NAMES = ["tt.py", "tt", "tt.cmd"]
CDN_OUTPUT_PATH = os.path.join(ROOT_PATH, "release", "tt")


def export_launcher():
    with open(LAUNCHER_PATH) as file:
        match = re.search(r'^TT_VERSION = "([^"]+)"', file.read(), re.MULTILINE)
    if match is None:
        print(f"TT_VERSION not found in {LAUNCHER_PATH}", file=sys.stderr)
        sys.exit(1)
    version = match.group(1)
    shutil.rmtree(CDN_OUTPUT_PATH, ignore_errors=True)
    os.makedirs(CDN_OUTPUT_PATH)
    with zipfile.ZipFile(os.path.join(CDN_OUTPUT_PATH, "tt.zip"), "w", zipfile.ZIP_DEFLATED) as archive:
        for file_name in FILE_NAMES:
            info = zipfile.ZipInfo(file_name)
            info.compress_type = zipfile.ZIP_DEFLATED
            # Executable for tools that extract the archive themselves
            info.external_attr = 0o100755 << 16
            with open(os.path.join(ROOT_PATH, file_name), "rb") as file:
                archive.writestr(info, file.read())
    with open(os.path.join(CDN_OUTPUT_PATH, "tt.json"), "w") as file:
        json.dump({"version": version}, file, indent=2)
    print(f"Exported tt {version} to {CDN_OUTPUT_PATH}")


if __name__ == "__main__":
    export_launcher()

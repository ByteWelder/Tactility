#!/usr/bin/env python3
# License: Apache License v2
"""Updates tt: replaces tt.py and its wrappers with the ones in sdk/tt.zip on the CDN. Run it as "tt update-self"."""

import io
import os
import re
import sys
import urllib.request
import zipfile

CDN_URL = os.environ.get("TT_CDN", "https://cdn.tactilityproject.org")
FILE_NAME_PATTERN = re.compile(r"^[A-Za-z0-9_-][A-Za-z0-9_.-]*$")
# Files of older versions of tt: tt.cmd made cmd.exe parse the arguments a second time, tt.ps1 replaces it
OBSOLETE_FILE_NAMES = ["tt.cmd"]


def fail(message):
    print(f"tt update-self: {message}", file=sys.stderr)
    sys.exit(1)


def download_archive():
    request = urllib.request.Request(f"{CDN_URL}/sdk/tt.zip", headers={"User-Agent": "tt update-self"})
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            data = response.read()
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            return {member.filename: archive.read(member) for member in archive.infolist()}
    except OSError as error:
        fail(f"failed to download tt.zip: {error}")
    except zipfile.BadZipFile:
        fail("the downloaded tt.zip is invalid")


def main():
    directory = os.environ.get("TT_DIRECTORY")
    if directory is None:
        fail("run this tool as 'tt update-self'")
    contents = download_archive()
    if "tt.py" not in contents or not all(FILE_NAME_PATTERN.match(file_name) for file_name in contents):
        fail("the downloaded tt.zip is invalid")
    try:
        compile(contents["tt.py"], "tt.py", "exec")
    except SyntaxError:
        fail("the downloaded tt.py is invalid")
    for file_name, content in contents.items():
        path = os.path.join(directory, file_name)
        temporary_path = f"{path}.new"
        with open(temporary_path, "wb") as file:
            file.write(content)
        os.chmod(temporary_path, 0o755)
        os.replace(temporary_path, path)
    for file_name in OBSOLETE_FILE_NAMES:
        path = os.path.join(directory, file_name)
        if file_name not in contents and os.path.isfile(path):
            os.remove(path)
    print("tt was updated")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# License: Apache License v2
"""Replaces older versions of tactility.py: they update themselves to this script, which installs tt."""

import io
import os
import re
import sys
import urllib.request
import zipfile

# Older versions of tactility.py compare this version with tactility.py.json to suggest an update
ttbuild_version = "8.0.0"
CDN_URL = "https://cdn.tactilityproject.org"
FILE_NAME_PATTERN = re.compile(r"^[A-Za-z0-9_-][A-Za-z0-9_.-]*$")

shell_color_orange = "\033[93m"
shell_color_red = "\033[91m"
shell_color_reset = "\033[m"


def print_error(message):
    print(f"{shell_color_red}ERROR: {message}{shell_color_reset}")


def print_warning(message):
    print(f"{shell_color_orange}WARNING: {message}{shell_color_reset}")


def install_tt(directory):
    """Extracts sdk/tt.zip: tt.py and its wrappers"""
    request = urllib.request.Request(f"{CDN_URL}/sdk/tt.zip", headers={"User-Agent": f"tactility.py {ttbuild_version}"})
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            archive_data = response.read()
        with zipfile.ZipFile(io.BytesIO(archive_data)) as archive:
            contents = {member.filename: archive.read(member) for member in archive.infolist()}
    except (OSError, zipfile.BadZipFile) as error:
        print_error(f"Failed to install tt: {error}")
        return False
    if not all(FILE_NAME_PATTERN.match(file_name) for file_name in contents):
        print_error("Failed to install tt: invalid tt.zip")
        return False
    for file_name, content in contents.items():
        path = os.path.join(directory, file_name)
        with open(path, "wb") as file:
            file.write(content)
        os.chmod(path, 0o755)
    return True


if __name__ == "__main__":
    directory = os.path.dirname(os.path.abspath(__file__))
    if not os.path.exists(os.path.join(directory, "tt.py")):
        if not install_tt(directory):
            sys.exit(1)
        print(f"Installed tt in {directory}")
    command = "tt" if os.name == "nt" else "./tt"
    print_warning(f"tactility.py was replaced by tt: use tt from now on, e.g. '{command} app build'")
    print_warning(f"Run '{command} help' for more info")
    sys.exit(1)

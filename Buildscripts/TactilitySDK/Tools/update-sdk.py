#!/usr/bin/env python3
# License: Apache License v2
"""Downloads the tools and installed platforms of the selected SDK again when the CDN has a newer build of it. Run it as "tt update-sdk"."""

import hashlib
import io
import json
import os
import shutil
import sys
import urllib.request
import zipfile

CDN_URL = os.environ.get("TT_CDN", "https://cdn.tactilityproject.org")


def fail(message):
    print(f"tt update-sdk: {message}", file=sys.stderr)
    sys.exit(1)


def fetch(url):
    request = urllib.request.Request(url, headers={"User-Agent": "tt update-sdk"})
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.read()
    except OSError as error:
        fail(f"failed to download {url}: {error}")


def read_commit(platform_directory):
    try:
        with open(os.path.join(platform_directory, "commit")) as file:
            return file.read().strip()
    except OSError:
        return None


def installed_platforms(version_directory):
    prefix = "platform-"
    return sorted(
        name[len(prefix):] for name in os.listdir(version_directory)
        if name.startswith(prefix) and os.path.isdir(os.path.join(version_directory, name, "TactilitySDK"))
    )


def read_installed_tools_hash(version_directory):
    try:
        with open(os.path.join(version_directory, "tools.sha256")) as file:
            return file.read().strip()
    except OSError:
        return None


def extract(archive_data, target_directory):
    """Extracts next to target_directory and then replaces it, refusing entries outside of it"""
    staging_directory = f"{target_directory}.new"
    shutil.rmtree(staging_directory, ignore_errors=True)
    os.makedirs(staging_directory)
    root = os.path.realpath(staging_directory)
    try:
        with zipfile.ZipFile(io.BytesIO(archive_data)) as archive:
            for member in archive.infolist():
                destination = os.path.realpath(os.path.join(root, member.filename))
                if not destination.startswith(root + os.sep):
                    fail(f"invalid entry in the archive: {member.filename}")
            archive.extractall(root)
    except zipfile.BadZipFile:
        shutil.rmtree(staging_directory, ignore_errors=True)
        fail("the downloaded archive is invalid")
    shutil.rmtree(target_directory, ignore_errors=True)
    os.replace(staging_directory, target_directory)


def install_platform(platform_directory, archive_data, commit):
    extract(archive_data, os.path.join(platform_directory, "TactilitySDK"))
    with open(os.path.join(platform_directory, "commit"), "w") as file:
        file.write(commit)


def install_tools(version_directory, archive_data, sha256):
    extract(archive_data, os.path.join(version_directory, "tools"))
    with open(os.path.join(version_directory, "tools.sha256"), "w") as file:
        file.write(sha256)


def main():
    version = os.environ.get("TACTILITY_SDK_VERSION")
    sdk_path = os.environ.get("TACTILITY_SDK_PATH")
    if version is None or sdk_path is None:
        fail("run this tool as 'tt update-sdk'")
    version_directory = os.path.join(sdk_path, "sdk", version)
    if os.environ.get("TT_LOCAL") == "1":
        fail("a local SDK can't be updated")
    index_data = fetch(f"{CDN_URL}/sdk/{version}/index.json")
    try:
        index = json.loads(index_data)
    except ValueError:
        fail("the downloaded index.json is invalid")
    commit = index.get("gitCommit", "")
    available = index.get("platforms", {})
    updated = False
    tools = index.get("tools")
    if tools is not None and read_installed_tools_hash(version_directory) != tools.get("sha256"):
        print(f"Updating the tools of SDK {version}")
        archive_data = fetch(f"{CDN_URL}/sdk/{version}/{tools['file']}")
        if hashlib.sha256(archive_data).hexdigest() != tools["sha256"]:
            fail("the downloaded tools don't match their checksum")
        install_tools(version_directory, archive_data, tools["sha256"])
        updated = True
    for platform in installed_platforms(version_directory):
        platform_directory = os.path.join(version_directory, f"platform-{platform}")
        if read_commit(platform_directory) == commit or platform not in available:
            continue
        print(f"Updating SDK {version} for {platform}")
        install_platform(platform_directory, fetch(f"{CDN_URL}/sdk/{version}/{available[platform]}"), commit)
        updated = True
    with open(os.path.join(version_directory, "index.json"), "wb") as file:
        file.write(index_data)
    print(f"SDK {version} was updated" if updated else f"SDK {version} is up to date")


if __name__ == "__main__":
    main()

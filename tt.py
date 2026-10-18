#!/usr/bin/env python3
# License: Apache License v2
"""Runs the Tactility developer tools of the SDK version that an app targets.

"tt <command> [arguments]" runs <command>.py from the tools of that SDK version.
When TACTILITY_SDK_PATH is set, its SDK and tools are used instead of downloading them.
Both ~/.tactility and TACTILITY_SDK_PATH hold sdk/<version>/tools/ and sdk/<version>/platform-<platform>/TactilitySDK/.
The tt (Linux, macOS) and tt.ps1 (Windows PowerShell) wrappers next to this script run it.
The SDK version is read from ./manifest.properties, or else from the first Apps/*/manifest.properties or apps/*/manifest.properties.
"""

import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.request
import zipfile

TT_VERSION = "1.1.1"
CDN_URL = os.environ.get("TT_CDN", "https://cdn.tactilityproject.org")
HOME_DIRECTORY = os.path.join(os.path.expanduser("~"), ".tactility")
INDEX_VALIDITY_SECONDS = 3600
COMMAND_PATTERN = re.compile(r"^[a-z0-9-]+$")
VERSION_PATTERN = re.compile(r"^[0-9A-Za-z.+-]+$")
ID_PATTERN = re.compile(r"^[0-9A-Za-z.]{5,32}$")
NAME_PATTERN = re.compile(r"^[0-9A-Za-z -]{2,32}$")
NEW_PROJECT_ARCHIVE = "new-project.zip"


def fail(message):
    print(f"tt: {message}", file=sys.stderr)
    sys.exit(1)


def warn(message):
    print(f"tt: {message}", file=sys.stderr)


def download(url, path):
    """Downloads to a temporary file next to path, so a failed download keeps the existing file"""
    request = urllib.request.Request(url, headers={"User-Agent": f"tt.py {TT_VERSION}"})
    partial_path = f"{path}.part"
    try:
        with urllib.request.urlopen(request, timeout=30) as response, open(partial_path, "wb") as file:
            shutil.copyfileobj(response, file)
    except OSError as error:
        if os.path.exists(partial_path):
            os.remove(partial_path)
        warn(f"failed to download {url}: {error}")
        return False
    os.replace(partial_path, path)
    return True


def read_properties(path):
    properties = {}
    with open(path) as file:
        for line in file:
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            key, separator, value = line.partition("=")
            if separator:
                properties[key.strip()] = value.strip()
    return properties


def find_manifest():
    if os.path.isfile("manifest.properties"):
        return "manifest.properties"
    for apps_directory in ("Apps", "apps"):
        if not os.path.isdir(apps_directory):
            continue
        for name in sorted(os.listdir(apps_directory)):
            path = os.path.join(apps_directory, name, "manifest.properties")
            if os.path.isfile(path):
                return path
    return None


def is_valid_version(version):
    return VERSION_PATTERN.match(version) is not None and ".." not in version


def ask(question, default=None, is_valid=None, description="invalid value"):
    """Asks until the answer is valid. An empty answer or the end of the input gives the default. Returns None when there is none."""
    suffix = f" [{default}]" if default is not None else ""
    while True:
        try:
            # On stderr, so the prompts don't end up in the output of the command
            print(f"{question}{suffix}: ", end="", file=sys.stderr, flush=True)
            answer = input().strip()
        except EOFError:
            answer = ""
        if not answer:
            return default
        if is_valid is None or is_valid(answer):
            return answer
        print(f"tt: {description}", file=sys.stderr)


def ask_yes_no(question, default):
    answer = ask(f"{question} [{'Y/n' if default else 'y/N'}]")
    if answer is None:
        return default
    return answer.lower() in ("y", "yes")


def read_new_project_archive(version):
    """Returns the content of the new project archive of the SDK version, from the local SDK or else the CDN"""
    local_path = os.environ.get("TACTILITY_SDK_PATH") or None
    if local_path is not None:
        archive_path = os.path.join(version_path(os.path.abspath(local_path), version), NEW_PROJECT_ARCHIVE)
        if not os.path.isfile(archive_path):
            fail(f"new project not found: {archive_path}")
        with open(archive_path, "rb") as file:
            return file.read()
    version_directory = version_path(HOME_DIRECTORY, version)
    os.makedirs(version_directory, exist_ok=True)
    entry = load_index(version, version_directory).get("newProject")
    if entry is None:
        fail(f"SDK {version} has no new project")
    archive_path = os.path.join(version_directory, NEW_PROJECT_ARCHIVE)
    if not download(sdk_url(version, entry["file"]), archive_path):
        fail(f"failed to download the new project of SDK {version}")
    try:
        with open(archive_path, "rb") as file:
            content = file.read()
    finally:
        os.remove(archive_path)
    if hashlib.sha256(content).hexdigest() != entry["sha256"]:
        fail(f"the new project of SDK {version} doesn't match its checksum")
    return content


def generate_project(version, package_id, app_id, app_name, is_terminal):
    """Extracts the new project of the SDK version into . and fills in the answers"""
    variant = "terminal" if is_terminal else "gui"
    root = os.path.realpath(".")
    created = []
    with zipfile.ZipFile(io.BytesIO(read_new_project_archive(version))) as archive:
        for member in archive.infolist():
            if member.is_dir() or not member.filename.startswith(f"{variant}/"):
                continue
            relative_path = member.filename[len(variant) + 1:]
            destination = os.path.realpath(os.path.join(root, relative_path))
            if not destination.startswith(root + os.sep):
                fail(f"invalid entry in the new project: {member.filename}")
            content = archive.read(member)
            if relative_path == "manifest.properties":
                text = content.decode("utf-8")
                for placeholder, value in (("@SDK_VERSION@", version), ("@PACKAGE_ID@", package_id), ("@APP_ID@", app_id), ("@APP_NAME@", app_name)):
                    text = text.replace(placeholder, value)
                content = text.encode("utf-8")
            os.makedirs(os.path.dirname(destination), exist_ok=True)
            with open(destination, "wb") as file:
                file.write(content)
            created.append(relative_path)
    if "manifest.properties" not in created:
        fail(f"the new project of SDK {version} has no {variant} variant")
    return created


def offer_new_project():
    """Asks to generate a new app project. Returns the manifest path when it did, else None"""
    if not ask_yes_no("Generate a new app project in this folder?", False):
        return None
    if os.path.exists("manifest.properties") or os.path.exists("main"):
        fail("this folder already has a manifest.properties or main/")
    version = ask("SDK version to use", None, is_valid_version, "invalid SDK version")
    if version is None:
        fail("no SDK version given")
    package_id = ask("Package id", "example.helloworld", ID_PATTERN.match, "use 5 to 32 letters, digits and dots")
    app_id = ask("App id", "helloworld", ID_PATTERN.match, "use 5 to 32 letters, digits and dots")
    app_name = ask("App name", "Hello World", NAME_PATTERN.match, "use 2 to 32 letters, digits, spaces and dashes")
    is_terminal = ask_yes_no("Terminal app?", False)
    for path in generate_project(version, package_id, app_id, app_name, is_terminal):
        print(f"Created {path}", file=sys.stderr)
    return "manifest.properties"


def resolve_target():
    """Returns the SDK version and platforms of the app manifest"""
    manifest_path = find_manifest()
    if manifest_path is None:
        print("tt: no manifest.properties found in . or Apps/*/ or apps/*/", file=sys.stderr)
        manifest_path = offer_new_project()
    if manifest_path is None:
        # Commands like "tt shell" don't need an app, only the tools of an SDK version
        version = ask("SDK version to use without a project (leave empty to abort)", None, is_valid_version, "invalid SDK version")
        if version is None:
            fail("no manifest.properties found in . or Apps/*/ or apps/*/")
        return version, []
    manifest = read_properties(manifest_path)
    version = manifest.get("target.sdk")
    if not version:
        fail(f"target.sdk not found in {manifest_path}")
    if not is_valid_version(version):
        fail(f"invalid target.sdk in {manifest_path}: {version}")
    platforms = [platform.strip() for platform in manifest.get("target.platforms", "").split(",") if platform.strip()]
    return version, platforms


def sdk_url(version, file_name):
    return f"{CDN_URL}/sdk/{version}/{file_name}"


def extract_zip(zip_path, target_directory):
    """Extracts into a new directory and then replaces target_directory, refusing entries outside of it"""
    staging_directory = f"{target_directory}.new"
    shutil.rmtree(staging_directory, ignore_errors=True)
    os.makedirs(staging_directory)
    root = os.path.realpath(staging_directory)
    with zipfile.ZipFile(zip_path) as archive:
        for member in archive.infolist():
            destination = os.path.realpath(os.path.join(root, member.filename))
            if not destination.startswith(root + os.sep):
                shutil.rmtree(staging_directory, ignore_errors=True)
                fail(f"invalid entry in {zip_path}: {member.filename}")
        archive.extractall(root)
    shutil.rmtree(target_directory, ignore_errors=True)
    os.replace(staging_directory, target_directory)


def file_sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as file:
        for block in iter(lambda: file.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def load_index(version, version_directory):
    index_path = os.path.join(version_directory, "index.json")
    is_fresh = os.path.isfile(index_path) and time.time() - os.path.getmtime(index_path) < INDEX_VALIDITY_SECONDS
    if not is_fresh and not download(sdk_url(version, "index.json"), index_path):
        if not os.path.isfile(index_path):
            fail(f"SDK {version} is not available: check your internet connection and that this SDK version exists")
        warn(f"using the cached information of SDK {version}")
    with open(index_path) as file:
        return json.load(file)


def version_path(sdk_path, version):
    return os.path.join(sdk_path, "sdk", version)


def install_tools(version, version_directory, index):
    tools = index.get("tools")
    if tools is None:
        fail(f"SDK {version} has no tools")
    tools_directory = os.path.join(version_directory, "tools")
    hash_path = os.path.join(version_directory, "tools.sha256")
    print(f"Downloading the tools of SDK {version}", file=sys.stderr)
    zip_path = os.path.join(version_directory, "tools.zip")
    if not download(sdk_url(version, tools["file"]), zip_path):
        fail(f"failed to download the tools of SDK {version}")
    if file_sha256(zip_path) != tools["sha256"]:
        os.remove(zip_path)
        fail(f"the tools of SDK {version} don't match their checksum")
    extract_zip(zip_path, tools_directory)
    os.remove(zip_path)
    with open(hash_path, "w") as file:
        file.write(tools["sha256"])


def install_platforms(version, version_directory, index, platforms):
    available = index.get("platforms", {})
    for platform in platforms:
        platform_directory = os.path.join(version_directory, f"platform-{platform}")
        if platform not in available:
            fail(f"platform {platform} is not available in SDK {version}")
        print(f"Downloading SDK {version} for {platform}", file=sys.stderr)
        zip_path = os.path.join(version_directory, f"platform-{platform}.zip")
        if not download(sdk_url(version, available[platform]), zip_path):
            fail(f"failed to download SDK {version} for {platform}")
        extract_zip(zip_path, os.path.join(platform_directory, "TactilitySDK"))
        os.remove(zip_path)
        # The commit of the SDK build, to find out when a -dev version was published again
        with open(os.path.join(platform_directory, "commit"), "w") as file:
            file.write(index.get("gitCommit", ""))


def read_text(path):
    try:
        with open(path) as file:
            return file.read().strip()
    except OSError:
        return None


def is_installed_build(version_directory, index):
    """True when the installed tools and platforms are of the build that the index describes"""
    installed_tools_hash = read_text(os.path.join(version_directory, "tools.sha256"))
    if installed_tools_hash is not None and installed_tools_hash != index.get("tools", {}).get("sha256"):
        return False
    for name in os.listdir(version_directory):
        commit = read_text(os.path.join(version_directory, name, "commit"))
        if name.startswith("platform-") and commit is not None and commit != index.get("gitCommit", ""):
            return False
    return True


def prepare_sdk(version, platforms):
    """Installs what's missing of the SDK version. Installed tools and platforms are only replaced by "tt update-sdk", so they stay from the same build."""
    version_directory = version_path(HOME_DIRECTORY, version)
    index = None
    # "target.platforms=all" means every platform of the SDK
    if platforms == ["all"]:
        os.makedirs(version_directory, exist_ok=True)
        index = load_index(version, version_directory)
        platforms = sorted(index.get("platforms", {}))
    has_tools = os.path.isdir(os.path.join(version_directory, "tools"))
    missing_platforms = [platform for platform in platforms if not os.path.isdir(os.path.join(version_directory, f"platform-{platform}", "TactilitySDK"))]
    if has_tools and not missing_platforms:
        return version_directory
    os.makedirs(version_directory, exist_ok=True)
    if index is None:
        index = load_index(version, version_directory)
    # The CDN only has the latest build of a version, so a newer build can't be mixed with the installed one
    if not is_installed_build(version_directory, index):
        if has_tools:
            fail(f"SDK {version} was published again since it was installed: run 'tt update-sdk' first")
        fail(f"SDK {version} was published again since it was installed: remove {version_directory} and try again")
    if not has_tools:
        install_tools(version, version_directory, index)
    install_platforms(version, version_directory, index, missing_platforms)
    return version_directory


def is_command(name):
    """Tools named _*.py are internal to the tools and are never commands"""
    return COMMAND_PATTERN.match(name) is not None and not name.startswith("_")


def list_commands(tools_directory):
    return sorted(name[:-3] for name in os.listdir(tools_directory) if name.endswith(".py") and is_command(name[:-3]))


def print_help(version, tools_directory):
    """Without an app manifest, the version and its commands are unknown"""
    print("Usage: tt <command> [arguments]")
    print("")
    if version is None:
        print("Run this in an app folder, or in a folder with Apps/*/ or apps/*/, to list the commands of its SDK")
    else:
        print(f"SDK {version}, commands:")
        for command in list_commands(tools_directory):
            print(f"  {command}")
    print("")
    print("The SDK and tools of TACTILITY_SDK_PATH are used when it is set")


def tool_environment(version, sdk_path, is_local):
    environment = dict(os.environ)
    environment["TACTILITY_SDK_VERSION"] = version
    environment["TACTILITY_SDK_PATH"] = sdk_path
    environment["TT_DIRECTORY"] = os.path.dirname(os.path.abspath(__file__))
    environment["TT_VERSION"] = TT_VERSION
    # The SDK in TACTILITY_SDK_PATH is managed by tt unless it is local
    if is_local:
        environment["TT_LOCAL"] = "1"
    else:
        environment.pop("TT_LOCAL", None)
    return environment


def run_tool(tool_path, arguments, environment):
    command = [sys.executable, tool_path, *arguments]
    if os.name == "posix":
        sys.stdout.flush()
        sys.stderr.flush()
        os.execve(sys.executable, command, environment)
    sys.exit(subprocess.run(command, env=environment).returncode)


def main(arguments):
    command = arguments[0] if arguments else None
    is_help = command is None or command in ("help", "--help")
    if not is_help and not is_command(command):
        fail(f"command not found: {command}")
    if is_help and find_manifest() is None:
        print_help(None, None)
        return

    version, platforms = resolve_target()
    # Empty counts as unset, like after "export TACTILITY_SDK_PATH="
    local_path = os.environ.get("TACTILITY_SDK_PATH") or None
    is_local = local_path is not None
    if is_local:
        sdk_path = os.path.abspath(local_path)
        print(f"tt: using the local SDK at {sdk_path}", file=sys.stderr)
        tools_directory = os.path.join(version_path(sdk_path, version), "tools")
        if not os.path.isdir(tools_directory):
            fail(f"tools not found: {tools_directory}")
    else:
        sdk_path = HOME_DIRECTORY
        # update-sdk replaces the installed build, so it only needs the tools
        required_platforms = [] if command == "update-sdk" else platforms
        tools_directory = os.path.join(prepare_sdk(version, required_platforms), "tools")
    environment = tool_environment(version, sdk_path, is_local)
    if is_help:
        print_help(version, tools_directory)
        return
    tool_path = os.path.join(tools_directory, f"{command}.py")
    if not os.path.isfile(tool_path):
        fail(f"command not found: {command}")
    run_tool(tool_path, arguments[1:], environment)


if __name__ == "__main__":
    main(sys.argv[1:])

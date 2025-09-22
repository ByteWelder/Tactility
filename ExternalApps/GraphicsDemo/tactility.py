import configparser
import json
import os
import re
import shutil
import sys
import subprocess
import time
import urllib.request
import zipfile
import requests
import tarfile
import shutil
import configparser

ttbuild_path = ".tactility"
ttbuild_version = "2.2.0"
ttbuild_cdn = "https://cdn.tactility.one"
ttbuild_sdk_json_validity = 3600  # seconds
ttport = 6666
verbose = False
use_local_sdk = False
valid_platforms = ["esp32", "esp32s3"]

spinner_pattern = [
    "⠋",
    "⠙",
    "⠹",
    "⠸",
    "⠼",
    "⠴",
    "⠦",
    "⠧",
    "⠇",
    "⠏"
]

if sys.platform == "win32":
    shell_color_red = ""
    shell_color_orange = ""
    shell_color_green = ""
    shell_color_purple = ""
    shell_color_cyan = ""
    shell_color_reset = ""
else:
    shell_color_red = "\033[91m"
    shell_color_orange = "\033[93m"
    shell_color_green = "\033[32m"
    shell_color_purple = "\033[35m"
    shell_color_cyan = "\033[36m"
    shell_color_reset = "\033[m"

def print_help():
    print("Usage: python tactility.py [action] [options]")
    print("")
    print("Actions:")
    print("  build [esp32,esp32s3]          Build the app. Optionally specify a platform.")
    print("    esp32:         ESP32")
    print("    esp32s3:       ESP32 S3")
    print("  clean                          Clean the build folders")
    print("  clearcache                     Clear the SDK cache")
    print("  updateself                     Update this tool")
    print("  run [ip]                       Run the application")
    print("  install [ip]                   Install the application")
    print("  uninstall [ip]                 Uninstall the application")
    print("  bir [ip] [esp32,esp32s3]       Build, install then run. Optionally specify a platform.")
    print("  brrr [ip] [esp32,esp32s3]      Functionally the same as \"bir\", but \"app goes brrr\" meme variant.")
    print("")
    print("Options:")
    print("  --help                         Show this commandline info")
    print("  --local-sdk                    Use SDK specified by environment variable TACTILITY_SDK_PATH")
    print("  --skip-build                   Run everything except the idf.py/CMake commands")
    print("  --verbose                      Show extra console output")

# region Core

def download_file(url, filepath):
    global verbose
    if verbose:
        print(f"Downloading from {url} to {filepath}")
    request = urllib.request.Request(
        url,
        data=None,
        headers={
            "User-Agent": f"Tactility Build Tool {ttbuild_version}"
        }
    )
    try:
        response = urllib.request.urlopen(request)
        file = open(filepath, mode="wb")
        file.write(response.read())
        file.close()
        return True
    except OSError as error:
        if verbose:
            print_error(f"Failed to fetch URL {url}\n{error}")
        return False

def print_warning(message):
    print(f"{shell_color_orange}WARNING: {message}{shell_color_reset}")

def print_error(message):
    print(f"{shell_color_red}ERROR: {message}{shell_color_reset}")

def exit_with_error(message):
    print_error(message)
    sys.exit(1)

def get_url(ip, path):
    return f"http://{ip}:{ttport}{path}"

def read_properties_file(path):
    config = configparser.RawConfigParser()
    config.read(path)
    return config

#endregion Core

#region SDK helpers

def read_sdk_json():
    json_file_path = os.path.join(ttbuild_path, "sdk.json")
    json_file = open(json_file_path)
    return json.load(json_file)

def get_sdk_dir(version, platform):
    global use_local_sdk
    if use_local_sdk:
        return os.environ.get("TACTILITY_SDK_PATH")
    else:
        global ttbuild_cdn
        return os.path.join(ttbuild_path, f"{version}-{platform}", "TactilitySDK")

def get_sdk_root_dir(version, platform):
    global ttbuild_cdn
    return os.path.join(ttbuild_path, f"{version}-{platform}")

def get_sdk_url(version, platform):
    global ttbuild_cdn
    return f"{ttbuild_cdn}/TactilitySDK-{version}-{platform}.zip"

def sdk_exists(version, platform):
    sdk_dir = get_sdk_dir(version, platform)
    return os.path.isdir(sdk_dir)

def should_update_sdk_json():
    global ttbuild_cdn
    json_filepath = os.path.join(ttbuild_path, "sdk.json")
    if os.path.exists(json_filepath):
        json_modification_time = os.path.getmtime(json_filepath)
        now = time.time()
        global ttbuild_sdk_json_validity
        minimum_seconds_difference = ttbuild_sdk_json_validity
        return (now - json_modification_time) > minimum_seconds_difference
    else:
        return True

def update_sdk_json():
    global ttbuild_cdn, ttbuild_path
    json_url = f"{ttbuild_cdn}/sdk.json"
    json_filepath = os.path.join(ttbuild_path, "sdk.json")
    return download_file(json_url, json_filepath)

def should_fetch_sdkconfig_files(platform_targets):
    for platform in platform_targets:
        sdkconfig_filename = f"sdkconfig.app.{platform}"
        if not os.path.exists(os.path.join(ttbuild_path, sdkconfig_filename)):
            return True
    return False

def fetch_sdkconfig_files(platform_targets):
    for platform in platform_targets:
        sdkconfig_filename = f"sdkconfig.app.{platform}"
        target_path = os.path.join(ttbuild_path, sdkconfig_filename)
        if not download_file(f"{ttbuild_cdn}/{sdkconfig_filename}", target_path):
            exit_with_error(f"Failed to download sdkconfig file for {platform}")

#endregion SDK helpers

#region Validation

def validate_environment():
    if os.environ.get("IDF_PATH") is None:
        exit_with_error("Cannot find the Espressif IDF SDK. Ensure it is installed and that it is activated via $PATH_TO_IDF_SDK/export.sh")
    if not os.path.exists("manifest.properties"):
        exit_with_error("manifest.properties not found")
    if use_local_sdk == False and os.environ.get("TACTILITY_SDK_PATH") is not None:
        print_warning("TACTILITY_SDK_PATH is set, but will be ignored by this command.")
        print_warning("If you want to use it, use the 'build local' parameters.")
    elif use_local_sdk == True and os.environ.get("TACTILITY_SDK_PATH") is None:
        exit_with_error("local build was requested, but TACTILITY_SDK_PATH environment variable is not set.")

def validate_version_and_platforms(sdk_json, sdk_version, platforms_to_build):
    version_map = sdk_json["versions"]
    if not sdk_version in version_map:
        exit_with_error(f"Version not found: {sdk_version}")
    version_data = version_map[sdk_version]
    available_platforms = version_data["platforms"]
    for desired_platform in platforms_to_build:
        if not desired_platform in available_platforms:
            exit_with_error(f"Platform {desired_platform} is not available. Available ones: {available_platforms}")

def validate_self(sdk_json):
    if not "toolVersion" in sdk_json:
        exit_with_error("Server returned invalid SDK data format (toolVersion not found)")
    if not "toolCompatibility" in sdk_json:
        exit_with_error("Server returned invalid SDK data format (toolCompatibility not found)")
    if not "toolDownloadUrl" in sdk_json:
        exit_with_error("Server returned invalid SDK data format (toolDownloadUrl not found)")
    tool_version = sdk_json["toolVersion"]
    tool_compatibility = sdk_json["toolCompatibility"]
    if tool_version != ttbuild_version:
        print_warning(f"New version available: {tool_version} (currently using {ttbuild_version})")
        print_warning(f"Run 'tactility.py updateself' to update.")
    if re.search(tool_compatibility, ttbuild_version) is None:
        print_error("The tool is not compatible anymore.")
        print_error("Run 'tactility.py updateself' to update.")
        sys.exit(1)

#endregion Validation

#region Manifest

def read_manifest():
    return read_properties_file("manifest.properties")

def validate_manifest(manifest):
    # [manifest]
    if not "manifest" in manifest:
        exit_with_error("Invalid manifest format: [manifest] not found")
    if not "version" in manifest["manifest"]:
        exit_with_error("Invalid manifest format: [manifest] version not found")
    # [target]
    if not "target" in manifest:
        exit_with_error("Invalid manifest format: [target] not found")
    if not "sdk" in manifest["target"]:
        exit_with_error("Invalid manifest format: [target] sdk not found")
    if not "platforms" in manifest["target"]:
        exit_with_error("Invalid manifest format: [target] platforms not found")
    # [app]
    if not "app" in manifest:
        exit_with_error("Invalid manifest format: [app] not found")
    if not "id" in manifest["app"]:
        exit_with_error("Invalid manifest format: [app] id not found")
    if not "versionName" in manifest["app"]:
        exit_with_error("Invalid manifest format: [app] versionName not found")
    if not "versionCode" in manifest["app"]:
        exit_with_error("Invalid manifest format: [app] versionCode not found")
    if not "name" in manifest["app"]:
        exit_with_error("Invalid manifest format: [app] name not found")

def is_valid_manifest_platform(manifest, platform):
    manifest_platforms = manifest["target"]["platforms"].split(",")
    return platform in manifest_platforms

def validate_manifest_platform(manifest, platform):
    if not is_valid_manifest_platform(manifest, platform):
        exit_with_error(f"Platform {platform} is not available in the manifest.")

def get_manifest_target_platforms(manifest, requested_platform):
    if requested_platform == "" or requested_platform is None:
        return manifest["target"]["platforms"].split(",")
    else:
        validate_manifest_platform(manifest, requested_platform)
        return [requested_platform]

#endregion Manifest

#region SDK download

def sdk_download(version, platform):
    sdk_root_dir = get_sdk_root_dir(version, platform)
    os.makedirs(sdk_root_dir, exist_ok=True)
    sdk_url = get_sdk_url(version, platform)
    filepath = os.path.join(sdk_root_dir, f"{version}-{platform}.zip")
    print(f"Downloading SDK version {version} for {platform}")
    if download_file(sdk_url, filepath):
        with zipfile.ZipFile(filepath, "r") as zip_ref:
            zip_ref.extractall(os.path.join(sdk_root_dir, "TactilitySDK"))
        return True
    else:
        return False

def sdk_download_all(version, platforms):
    for platform in platforms:
        if not sdk_exists(version, platform):
            if not sdk_download(version, platform):
                return False
        else:
            if verbose:
                print(f"Using cached download for SDK version {version} and platform {platform}")
    return True

#endregion SDK download

#region Building

def get_cmake_path(platform):
    return os.path.join("build", f"cmake-build-{platform}")

def find_elf_file(platform):
    cmake_dir = get_cmake_path(platform)
    if os.path.exists(cmake_dir):
        for file in os.listdir(cmake_dir):
            if file.endswith(".app.elf"):
                return os.path.join(cmake_dir, file)
    return None

def build_all(version, platforms, skip_build):
    for platform in platforms:
        # First build command must be "idf.py build", otherwise it fails to execute "idf.py elf"
        # We check if the ELF file exists and run the correct command
        # This can lead to code caching issues, so sometimes a clean build is required
        if find_elf_file(platform) is None:
            if not build_first(version, platform, skip_build):
                break
        else:
            if not build_consecutively(version, platform, skip_build):
                break

def wait_for_build(process, platform):
    buffer = []
    os.set_blocking(process.stdout.fileno(), False)
    while process.poll() is None:
        for i in spinner_pattern:
            time.sleep(0.1)
            progress_text = f"Building for {platform} {shell_color_cyan}" + str(i) + shell_color_reset
            sys.stdout.write(progress_text + "\r")
            while True:
                line = process.stdout.readline()
                decoded_line = line.decode("UTF-8")
                if decoded_line != "":
                    buffer.append(decoded_line)
                else:
                    break
    return buffer

# The first build must call "idf.py build" and consecutive builds must call "idf.py elf" as it finishes faster.
# The problem is that the "idf.py build" always results in an error, even though the elf file is created.
# The solution is to suppress the error if we find that the elf file was created.
def build_first(version, platform, skip_build):
    sdk_dir = get_sdk_dir(version, platform)
    if verbose:
        print(f"Using SDK at {sdk_dir}")
    os.environ["TACTILITY_SDK_PATH"] = sdk_dir
    sdkconfig_path = os.path.join(ttbuild_path, f"sdkconfig.app.{platform}")
    os.system(f"cp {sdkconfig_path} sdkconfig")
    elf_path = find_elf_file(platform)
    # Remove previous elf file: re-creation of the file is used to measure if the build succeeded,
    # as the actual build job will always fail due to technical issues with the elf cmake script
    if elf_path is not None:
        os.remove(elf_path)
    if skip_build:
        return True
    print("Building first build")
    cmake_path = get_cmake_path(platform)
    with subprocess.Popen(["idf.py", "-B", cmake_path, "build"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as process:
        build_output = wait_for_build(process, platform)
        # The return code is never expected to be 0 due to a bug in the elf cmake script, but we keep it just in case
        if process.returncode == 0:
            print(f"{shell_color_green}Building for {platform} ✅{shell_color_reset}")
            return True
        else:
            if find_elf_file(platform) is None:
                for line in build_output:
                    print(line, end="")
                print(f"{shell_color_red}Building for {platform} failed ❌{shell_color_reset}")
                return False
            else:
                print(f"{shell_color_green}Building for {platform} ✅{shell_color_reset}")
                return True

def build_consecutively(version, platform, skip_build):
    sdk_dir = get_sdk_dir(version, platform)
    if verbose:
        print(f"Using SDK at {sdk_dir}")
    os.environ["TACTILITY_SDK_PATH"] = sdk_dir
    sdkconfig_path = os.path.join(ttbuild_path, f"sdkconfig.app.{platform}")
    os.system(f"cp {sdkconfig_path} sdkconfig")
    if skip_build:
        return True
    cmake_path = get_cmake_path(platform)
    with subprocess.Popen(["idf.py", "-B", cmake_path, "elf"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as process:
        build_output = wait_for_build(process, platform)
        if process.returncode == 0:
            print(f"{shell_color_green}Building for {platform} ✅{shell_color_reset}")
            return True
        else:
            for line in build_output:
                print(line, end="")
            print(f"{shell_color_red}Building for {platform} failed ❌{shell_color_reset}")
            return False

#endregion Building

#region Packaging

def package_intermediate_manifest(target_path):
    if not os.path.isfile("manifest.properties"):
        print_error("manifest.properties not found")
        return
    shutil.copy("manifest.properties", os.path.join(target_path, "manifest.properties"))

def package_intermediate_binaries(target_path, platforms):
    elf_dir = os.path.join(target_path, "elf")
    os.makedirs(elf_dir, exist_ok=True)
    for platform in platforms:
        elf_path = find_elf_file(platform)
        if elf_path is None:
            print_error(f"ELF file not found at {elf_path}")
            return
        shutil.copy(elf_path, os.path.join(elf_dir, f"{platform}.elf"))

def package_intermediate_assets(target_path):
    if os.path.isdir("assets"):
        shutil.copytree("assets", os.path.join(target_path, "assets"), dirs_exist_ok=True)

def package_intermediate(platforms):
    target_path = os.path.join("build", "package-intermediate")
    if os.path.isdir(target_path):
        shutil.rmtree(target_path)
    os.makedirs(target_path, exist_ok=True)
    package_intermediate_manifest(target_path)
    package_intermediate_binaries(target_path, platforms)
    package_intermediate_assets(target_path)

def package_name(platforms):
    elf_path = find_elf_file(platforms[0])
    elf_base_name = os.path.basename(elf_path).removesuffix(".app.elf")
    return os.path.join("build", f"{elf_base_name}.app")

def package_all(platforms):
    print("Packaging app")
    package_intermediate(platforms)
    # Create build/something.app
    tar_path = package_name(platforms)
    tar = tarfile.open(tar_path, mode="w", format=tarfile.USTAR_FORMAT)
    tar.add(os.path.join("build", "package-intermediate"), arcname="")
    tar.close()

#endregion Packaging

def setup_environment():
    global ttbuild_path
    os.makedirs(ttbuild_path, exist_ok=True)

def build_action(manifest, platform_arg):
    # Environment validation
    validate_environment()
    platforms_to_build = get_manifest_target_platforms(manifest, platform_arg)
    if not use_local_sdk:
        if should_fetch_sdkconfig_files(platforms_to_build):
            fetch_sdkconfig_files(platforms_to_build)
        sdk_json = read_sdk_json()
        validate_self(sdk_json)
        if not "versions" in sdk_json:
            exit_with_error("Version data not found in sdk.json")
    # Build
    sdk_version = manifest["target"]["sdk"]
    if not use_local_sdk:
        validate_version_and_platforms(sdk_json, sdk_version, platforms_to_build)
        if not sdk_download_all(sdk_version, platforms_to_build):
            exit_with_error("Failed to download one or more SDKs")
    build_all(sdk_version, platforms_to_build, skip_build)  # Environment validation
    if not skip_build:
        package_all(platforms_to_build)

def clean_action():
    if os.path.exists("build"):
        print(f"Removing build/")
        shutil.rmtree("build")
    else:
        print("Nothing to clean")

def clear_cache_action():
    if os.path.exists(ttbuild_path):
        print(f"Removing {ttbuild_path}/")
        shutil.rmtree(ttbuild_path)
    else:
        print("Nothing to clear")

def update_self_action():
    sdk_json = read_sdk_json()
    tool_download_url = sdk_json["toolDownloadUrl"]
    if download_file(tool_download_url, "tactility.py"):
        print("Updated")
    else:
        exit_with_error("Update failed")

def get_device_info(ip):
    print(f"Getting device info from {ip}")
    url = get_url(ip, "/info")
    try:
        response = requests.get(url)
        if response.status_code != 200:
            print_error("Run failed")
        else:
            print(response.json())
            print(f"{shell_color_green}Run successful ✅{shell_color_reset}")
    except requests.RequestException as e:
        print(f"Request failed: {e}")

def run_action(manifest, ip):
    app_id = manifest["app"]["id"]
    print(f"Running {app_id} on {ip}")
    url = get_url(ip, "/app/run")
    params = {'id': app_id}
    try:
        response = requests.post(url, params=params)
        if response.status_code != 200:
            print_error("Run failed")
        else:
            print(f"{shell_color_green}Run successful ✅{shell_color_reset}")
    except requests.RequestException as e:
        print(f"Request failed: {e}")

def install_action(ip, platforms):
    for platform in platforms:
        elf_path = find_elf_file(platform)
        if elf_path is None:
            exit_with_error(f"ELF file not built for {platform}")
    package_path = package_name(platforms)
    print(f"Installing {package_path} to {ip}")
    url = get_url(ip, "/app/install")
    try:
        # Prepare multipart form data
        with open(package_path, 'rb') as file:
            files = {
                'elf': file
            }
            response = requests.put(url, files=files)
            if response.status_code != 200:
                print_error("Install failed")
            else:
                print(f"{shell_color_green}Installation successful ✅{shell_color_reset}")
    except requests.RequestException as e:
        print_error(f"Installation failed: {e}")
    except IOError as e:
        print_error(f"File error: {e}")

def uninstall_action(manifest, ip):
    app_id = manifest["app"]["id"]
    print(f"Uninstalling {app_id} on {ip}")
    url = get_url(ip, "/app/uninstall")
    params = {'id': app_id}
    try:
        response = requests.put(url, params=params)
        if response.status_code != 200:
            print_error("Uninstall failed")
        else:
            print(f"{shell_color_green}Uninstall successful ✅{shell_color_reset}")
    except requests.RequestException as e:
        print(f"Request failed: {e}")
#region Main

if __name__ == "__main__":
    print(f"Tactility Build System v{ttbuild_version}")
    if "--help" in sys.argv:
        print_help()
        sys.exit()
    # Argument validation
    if len(sys.argv) == 1:
        print_help()
        sys.exit()
    action_arg = sys.argv[1]
    verbose = "--verbose" in sys.argv
    skip_build = "--skip-build" in sys.argv
    use_local_sdk = "--local-sdk" in sys.argv
    # Environment setup
    setup_environment()
    if not os.path.isfile("manifest.properties"):
        exit_with_error("manifest.properties not found")
    manifest = read_manifest()
    validate_manifest(manifest)
    all_platform_targets = manifest["target"]["platforms"].split(",")
    # Update SDK cache (sdk.json)
    if should_update_sdk_json() and not update_sdk_json():
        exit_with_error("Failed to retrieve SDK info")
    # Actions
    if action_arg == "build":
        if len(sys.argv) < 2:
            print_help()
            exit_with_error("Commandline parameter missing")
        platform = None
        if len(sys.argv) > 2:
            platform = sys.argv[2]
        build_action(manifest, platform)
    elif action_arg == "clean":
        clean_action()
    elif action_arg == "clearcache":
        clear_cache_action()
    elif action_arg == "updateself":
        update_self_action()
    elif action_arg == "run":
        if len(sys.argv) < 3:
            print_help()
            exit_with_error("Commandline parameter missing")
        run_action(manifest, sys.argv[2])
    elif action_arg == "install":
        if len(sys.argv) < 3:
            print_help()
            exit_with_error("Commandline parameter missing")
        platform = None
        platforms_to_install = all_platform_targets
        if len(sys.argv) >= 4:
            platform = sys.argv[3]
            platforms_to_install = [platform]
        install_action(sys.argv[2], platforms_to_install)
    elif action_arg == "uninstall":
        if len(sys.argv) < 3:
            print_help()
            exit_with_error("Commandline parameter missing")
        uninstall_action(manifest, sys.argv[2])
    elif action_arg == "bir" or action_arg == "brrr":
        if len(sys.argv) < 3:
            print_help()
            exit_with_error("Commandline parameter missing")
        platform = None
        platforms_to_install = all_platform_targets
        if len(sys.argv) >= 4:
            platform = sys.argv[3]
            platforms_to_install = [platform]
        build_action(manifest, platform)
        install_action(sys.argv[2], platforms_to_install)
        run_action(manifest, sys.argv[2])
    else:
        print_help()
        exit_with_error("Unknown commandline parameter")

#endregion Main

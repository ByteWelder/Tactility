import configparser
import json
import os
import re
import sys
import time
import urllib.request
import zipfile

esp_platforms = ["esp32", "esp32s3"]
ttbuild_path = '.ttbuild'
ttbuild_version = '0.1.0'
ttbuild_properties_file = 'tactility.properties'
ttbuild_cdn = "https://cdn.tactility.one"
ttbuild_sdk_json_validity = 3600  # seconds
verbose = False

def print_help():
    print("Usage: python ttbuild.py [all|esp32|esp32s3] [options]")
    print("Options:")
    print("  --help           Show this commandline info")
    print("  --skip-build     Run everything except the idf.py/CMake commands")
    print("  --verbose        Show extra console output")

def download_file(url, filepath):
    global verbose
    if verbose:
        print(f"Downloading from {url} to {filepath}")
    request = urllib.request.Request(
        url,
        data=None,
        headers={
            'User-Agent': f"Tactility Build Tool {ttbuild_version}"
        }
    )
    try:
        response = urllib.request.urlopen(request)
        file = open(filepath, mode='wb')
        file.write(response.read())
        file.close()
        return True
    except OSError as error:
        if verbose:
            print_error(f"Failed to fetch URL {url}\n{error}")
        return False

def print_warning(message):
    if sys.platform == 'win32':
        print(f"ERROR: {message}")
    else:
        print(f"\033[93mWARNING: {message}\033[m")

def print_error(message):
    if sys.platform == 'win32':
        print(f"ERROR: {message}")
    else:
        print(f"\033[91mERROR: {message}\033[m")

def exit_with_error(message):
    print_error(message)
    sys.exit(1)

def is_valid_platform_name(name):
    return name == "all" or name == "esp32" or name == "esp32s3"

def validate_environment():
    global ttbuild_properties_file
    if os.environ.get('IDF_PATH') is None:
        exit_with_error("IDF is not installed or activated. Ensure you installed the toolset and ran the export command.")
    if os.environ.get('TACTILITY_SDK_PATH') is not None:
        print_warning("TACTILITY_SDK_PATH is set, but will be ignored by this command")
    if not os.path.exists(ttbuild_properties_file):
        exit_with_error(f"{ttbuild_properties_file} file not found")

def setup_environment():
    global ttbuild_path
    os.makedirs(ttbuild_path, exist_ok=True)

def get_sdk_dir(version, platform):
    global ttbuild_cdn
    return os.path.join(ttbuild_path, f"{version}-{platform}", 'TactilitySDK')

def get_sdk_version():
    global ttbuild_properties_file
    parser = configparser.RawConfigParser()
    parser.read(ttbuild_properties_file)
    sdk_dict = dict(parser.items('sdk'))
    if not 'version' in sdk_dict:
        exit_with_error(f"Could not find 'version' in [sdk] section in {ttbuild_properties_file}")
    return sdk_dict['version']

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

def validate_version_and_platforms(sdk_json, sdk_version, platforms_to_build):
    version_map = sdk_json['versions']
    if not sdk_version in version_map:
        exit_with_error(f"Version not found: {sdk_version}")
    version_data = version_map[sdk_version]
    available_platforms = version_data['platforms']
    for desired_platform in platforms_to_build:
        if not desired_platform in available_platforms:
            exit_with_error(f"Platform {desired_platform} is not available. Available ones: {available_platforms}")

def validate_self(sdk_json):
    if not 'toolVersion' in sdk_json:
        exit_with_error("Server returned invalid SDK data format (toolVersion not found)")
    if not 'toolCompatibility' in sdk_json:
        exit_with_error("Server returned invalid SDK data format (toolCompatibility not found)")
    if not 'toolDownloadUrl' in sdk_json:
        exit_with_error("Server returned invalid SDK data format (toolDownloadUrl not found)")
    tool_version = sdk_json['toolVersion']
    tool_compatibility = sdk_json['toolCompatibility']
    tool_download_url = sdk_json['toolDownloadUrl']
    if tool_version != ttbuild_version:
        print_warning(f"New version available: {tool_version} (currently using {ttbuild_version})")
        print_warning(f"Download it at {tool_download_url}")
    if re.search(tool_compatibility, ttbuild_version) is None:
        exit_with_error('The tool is not compatible anymore. Please upgrade. See https://docs.tactility.one for more information.')

def sdk_download(version, platform):
    sdk_root_dir = get_sdk_root_dir(version, platform)
    os.makedirs(sdk_root_dir, exist_ok=True)
    sdk_url = get_sdk_url(version, platform)
    filepath = os.path.join(sdk_root_dir, f"{version}-{platform}.zip")
    print(f"Downloading SDK version {version} for {platform}")
    if download_file(sdk_url, filepath):
        with zipfile.ZipFile(filepath, 'r') as zip_ref:
            zip_ref.extractall(sdk_root_dir)
        return True
    else:
        return False

def sdk_download_all(version, platforms):
    for platform in platforms:
        if not sdk_exists(version, platform):
            if not sdk_download(version, platform):
                return False
        else:
            print(f"Using cached download for SDK version {version} and platform {platform}")
    return True

def build(version, platforms, skip_build):
    for platform in platforms:
        if verbose:
            print(f"Platform: {platform}")
        sdk_dir = get_sdk_dir(version, platform)
        if verbose:
            print(f"Using SDK at {sdk_dir}")
        os.environ['TACTILITY_SDK_PATH'] = sdk_dir
        os.system(f"cp sdkconfig.{platform} sdkconfig")
        if not skip_build:
            os.system(f"idf.py -B build-{platform} build")

if __name__ == "__main__":
    print(f"Tactility Build System v{ttbuild_version}")
    if "--help" in sys.argv:
        print_help()
        sys.exit()
    # Argument validation
    if len(sys.argv) == 1:
        print_help()
        sys.exit()
    platform_arg = sys.argv[1]
    if not is_valid_platform_name(platform_arg):
        print_help()
        exit_with_error("Invalid platform name")
    verbose = "--verbose" in sys.argv
    skip_build = '--skip-build' in sys.argv
    # Environment validation
    validate_environment()
    # Environment setup
    setup_environment()
    # Update SDK cache
    if should_update_sdk_json() and not update_sdk_json():
        exit_with_error("Failed to retrieve SDK info")
    json_file_path = os.path.join(ttbuild_path, 'sdk.json')
    json_file = open(json_file_path)
    sdk_json = json.load(json_file)
    validate_self(sdk_json)
    if not 'versions' in sdk_json:
        exit_with_error("version data not found in sdk.json")
    # Build
    platforms_to_build = esp_platforms if platform_arg == "all" else [platform_arg]
    sdk_version = get_sdk_version()
    validate_version_and_platforms(sdk_json, sdk_version, platforms_to_build)
    if not sdk_download_all(sdk_version, platforms_to_build):
        exit_with_error("Failed to download one or more SDKs")
    build(sdk_version, platforms_to_build, skip_build)

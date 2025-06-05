import sys
import os
import urllib.request
import zipfile

esp_platforms = ["esp32", "esp32s3"]
ttbuild_path = '.ttbuild'
ttbuild_version = '0.1.0'

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

def build(version, platforms):
    for platform in platforms:
        print(f"Platform: {platform}")
        sdk_dir = get_sdk_dir(version, platform)
        print(f"Using SDK at {sdk_dir}")
        os.environ['TACTILITY_SDK_PATH'] = sdk_dir
        os.system(f"cp sdkconfig.{platform} sdkconfig")
        # os.system(f"idf.py -B build-{platform} build")

def print_help():
    print("Usage:")
    print("\tpython ttbuild.py [platformName]")
    print("\tplatformName:   all|esp32|esp32s3")

def validate_environment():
    if os.environ.get('IDF_PATH') is None:
        exit_with_error("IDF is not installed or activated. Ensure you installed the toolset and ran the export command.")
    if os.environ.get('TACTILITY_SDK_PATH') is not None:
        print_warning("TACTILITY_SDK_PATH is set, but will be ignored by this command")

def setup_environment():
    os.makedirs(ttbuild_path, exist_ok=True)

def get_sdk_dir(version, platform):
    return os.path.join(ttbuild_path, f"{version}-{platform}", 'TactilitySDK')

def get_sdk_root_dir(version, platform):
    return os.path.join(ttbuild_path, f"{version}-{platform}")

def get_sdk_url(version, platform):
    return f"https://cdn.tactility.one/TactilitySDK-{version}-{platform}.zip"

def sdk_exists(version, platform):
    sdk_dir = get_sdk_dir(version, platform)
    return os.path.isdir(sdk_dir)

def sdk_download(version, platform):
    sdk_root_dir = get_sdk_root_dir(version, platform)
    os.makedirs(sdk_root_dir, exist_ok=True)
    sdk_url = get_sdk_url(version, platform)
    print(f"Downloading SDK version {version} for {platform} from {sdk_url}")
    request = urllib.request.Request(
        sdk_url,
        data=None,
        headers={
            'User-Agent': f"Tactility Build Tool {ttbuild_version}"
        }
    )
    response = urllib.request.urlopen(request)
    filepath = os.path.join(sdk_root_dir, f"{version}-{platform}.zip")
    file = open(filepath, mode='wb')
    file.write(response.read())
    file.close()
    with zipfile.ZipFile(filepath, 'r') as zip_ref:
        zip_ref.extractall(sdk_root_dir)
    return True

def sdk_download_all(version, platforms):
    for platform in platforms:
        if not sdk_exists(version, platform):
            sdk_download(version, platform)
        else:
            print(f"Using cached download for SDK version {version} and platform {platform}")

if __name__ == "__main__":
    print(f"Tactility Build System v{ttbuild_version}")
    # Environment
    validate_environment()
    setup_environment()
    # Argument validation
    if len(sys.argv) == 1:
        print_help()
        sys.exit()
    platform_arg = sys.argv[1]
    if not is_valid_platform_name(platform_arg):
        print_help()
        exit_with_error("Invalid platform name")
    # Build
    platforms_to_build = esp_platforms if platform_arg == "all" else [platform_arg]
    sdk_version = '0.4.0-dev'
    sdk_download_all(sdk_version, platforms_to_build)
    build(sdk_version, platforms_to_build)


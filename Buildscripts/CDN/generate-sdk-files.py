import subprocess
from datetime import datetime, UTC
import os
import sys
from dataclasses import dataclass, asdict
import json
import shutil

VERBOSE = False

@dataclass
class SdkIndex:
    version: str
    created: str
    gitCommit: str
    platforms: dict

shell_color_red = "\033[91m"
shell_color_orange = "\033[93m"
shell_color_reset = "\033[m"

def print_warning(message):
    print(f"{shell_color_orange}WARNING: {message}{shell_color_reset}")

def print_error(message):
    print(f"{shell_color_red}ERROR: {message}{shell_color_reset}")

def exit_with_error(message):
    print_error(message)
    sys.exit(1)

def print_help():
    print("Usage: python generate-sdk-files.py [inPath] [outPath] [version]")
    print("     inPath   path with the extracted release files")
    print("     outPath  path where the CDN files will become available")
    print("     version  technical version name (e.g. 1.2.0)")
    print("Options:")
    print("  --verbose                      Show extra console output")

def get_git_commit_hash():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('ascii').strip()

def main(in_path: str, out_path: str, version: str):
    if not os.path.exists(in_path):
        exit_with_error(f"Input path not found: {in_path}")
    if os.path.exists(out_path):
        shutil.rmtree(out_path)
    os.mkdir(out_path)
    artifact_directories = os.listdir(in_path)
    sdk_index = SdkIndex(
        version=version,
        created=datetime.now(UTC).strftime('%Y-%m-%dT%H:%M:%S'),
        gitCommit=get_git_commit_hash(),
        platforms={}
    )
    for artifact_directory in artifact_directories:
        if VERBOSE:
            print(f"Processing {in_path}/{artifact_directory}")
        if not artifact_directory.startswith("TactilitySDK-"):
            continue
        sdk_platform = artifact_directory.removeprefix("TactilitySDK-")
        if not sdk_platform:
            exit_with_error(f"Cannot derive platform from directory name: {artifact_directory}")
        sdk_index.platforms[sdk_platform] = f"{artifact_directory}.zip"
        if VERBOSE:
            print(f"Archiving {in_path}/{artifact_directory} to {out_path}/{artifact_directory}.zip")
        shutil.make_archive(os.path.join(out_path, artifact_directory), 'zip', os.path.join(in_path, artifact_directory))
    index_file_path = os.path.join(out_path, "index.json")
    if VERBOSE:
        print(f"Generating {index_file_path}")
    with open(index_file_path, "w") as index_file:
        json.dump(asdict(sdk_index), index_file, indent=2)

if __name__ == "__main__":
    print("Tactility CDN SDK File Generator")
    if "--help" in sys.argv:
        print_help()
        sys.exit()
    # Argument validation
    if len(sys.argv) < 4:
        print_help()
        sys.exit(1)
    if "--verbose" in sys.argv:
        VERBOSE = True
        sys.argv.remove("--verbose")
    main(in_path=sys.argv[1], out_path=sys.argv[2], version=sys.argv[3])

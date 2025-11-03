import os
import sys
import configparser
from dataclasses import dataclass, asdict
import json
import shutil

verbose = False

@dataclass
class IndexEntry:
    id: str
    name: str
    vendor: str
    incubating: bool
    warningMessage: str
    infoMessage: str

@dataclass
class Manifest:
    name: str
    version: str
    new_install_prompt_erase: str
    funding_url: str
    builds: list

@dataclass
class ManifestBuild:
    chipFamily: str
    parts: list

@dataclass
class ManifestBuildPart:
    path: str
    offset: int

@dataclass
class DeviceIndex:
    version: str
    devices: list

if sys.platform == "win32":
    shell_color_red = ""
    shell_color_orange = ""
    shell_color_reset = ""
else:
    shell_color_red = "\033[91m"
    shell_color_orange = "\033[93m"
    shell_color_reset = "\033[m"

def print_warning(message):
    print(f"{shell_color_orange}WARNING: {message}{shell_color_reset}")

def print_error(message):
    print(f"{shell_color_red}ERROR: {message}{shell_color_reset}")

def print_help():
    print("Usage: python generate-files.py [inPath] [outPath] [version]")
    print("     inPath   path with the extracted release files")
    print("     outPath  path where the CDN files will become available")
    print("     version  technical version name (e.g. 1.2.0)")
    print("Options:")
    print("  --verbose                      Show extra console output")

def exit_with_error(message):
    print_error(message)
    sys.exit(1)

def read_properties_file(path):
    config = configparser.RawConfigParser()
    config.read(path)
    return config

def read_mapping_file():
    mapping_file_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "devices.properties")
    if not os.path.isfile(mapping_file_path):
        exit_with_error(f"Mapping file not found: {mapping_file_path}")
    return read_properties_file(mapping_file_path)

def to_manifest_chip_name(name):
    if name == "esp32":
        return "ESP32"
    elif name == "esp32s2":
        return "ESP32-S2"
    elif name == "esp32s3":
        return "ESP32-S3"
    elif name == "esp32c3":
        return "ESP32-C3"
    elif name == "esp32c5":
        return "ESP32-C5"
    elif name == "esp32c6":
        return "ESP32-C6"
    elif name == "esp32p4":
        return "ESP32-P4"
    else:
        exit_with_error(f"to_manifest_chip_name() doesn't support {name} yet")
        return ""


def process_board(in_path: str, out_path: str, device_directory: str, device_id: str, device_mapping: configparser, version: str):
    in_device_path = os.path.join(in_path, device_directory)
    in_device_binaries_path = os.path.join(in_device_path, "Binaries")
    assert os.path.isdir(in_device_binaries_path)
    flasher_args_path = os.path.join(in_device_binaries_path, "flasher_args.json")
    assert os.path.isfile(flasher_args_path)
    with open(flasher_args_path) as json_data:
        flasher_args = json.load(json_data)
        json_data.close()
        flash_files = flasher_args["flash_files"]
        manifest = Manifest(
            name=f"Tactility for {device_mapping["vendor"]} {device_mapping["boardName"]}",
            version=version,
            new_install_prompt_erase="true",
            funding_url="https://github.com/sponsors/ByteWelder",
            builds=[
                ManifestBuild(
                    chipFamily=to_manifest_chip_name(flasher_args["extra_esptool_args"]["chip"]),
                    parts=[]
                )
            ]
        )
        for offset in flash_files:
            flash_file_entry = flash_files[offset]
            flash_file_entry_name = os.path.basename(flash_file_entry)
            in_flash_file_path = os.path.join(in_device_binaries_path, flash_file_entry)
            out_flash_file_name = f"{device_id}-{flash_file_entry_name}"
            out_flash_file_path = os.path.join(out_path, out_flash_file_name)
            if verbose:
                print(f"Copying {in_flash_file_path} -> {out_flash_file_path}")
            shutil.copy(in_flash_file_path, out_flash_file_path)
            manifest.builds[0].parts.append(
                ManifestBuildPart(
                    path=out_flash_file_name,
                    offset=int(offset, 16)
                )
            )

        json_manifest_path = os.path.join(out_path, f"{device_id}.json")
        with open(json_manifest_path, 'w') as json_manifest_file:
            json.dump(asdict(manifest), json_manifest_file, indent=2)
            json_manifest_file.close()

def main(in_path: str, out_path: str, version: str):
    if not os.path.exists(in_path):
        exit_with_error(f"Input path not found: {in_path}")
    if os.path.exists(out_path):
        shutil.rmtree(out_path)
    os.mkdir(out_path)
    mapping = read_mapping_file()
    device_directories = os.listdir(in_path)
    device_index = DeviceIndex(version, [])
    for device_directory in device_directories:
        if not device_directory.endswith("-symbols"):
            device_id = device_directory[10:]
            if device_id not in mapping.sections():
                exit_with_error(f"Mapping for {device_id} not found in mapping file")
            device_properties = mapping[device_id]
            process_board(in_path, out_path, device_directory, device_id, device_properties, version)
            if "warningMessage" in device_properties.keys():
                warning_message = device_properties["warningMessage"]
            else:
                warning_message = None
            if "infoMessage" in device_properties.keys():
                info_message = device_properties["infoMessage"]
            else:
                info_message = None
            if "incubating" in device_properties.keys():
                incubating = device_properties["incubating"].lower() == 'true'
            else:
                incubating = False
            board_names = device_properties["boardName"].split(',')
            for board_name in board_names:
                device_index.devices.append(asdict(IndexEntry(
                    id=device_id,
                    name=board_name,
                    vendor=device_properties["vendor"],
                    incubating=incubating,
                    warningMessage=warning_message,
                    infoMessage=info_message
                )))
    index_file_path = os.path.join(out_path, "index.json")
    with open(index_file_path, "w") as index_file:
        json.dump(asdict(device_index), index_file, indent=2)
        index_file.close()

if __name__ == "__main__":
    print("Tactility CDN File Generator")
    if "--help" in sys.argv:
        print_help()
        sys.exit()
    # Argument validation
    if len(sys.argv) < 4:
        print_help()
        sys.exit()
    if "--verbose" in sys.argv:
        verbose = True
        sys.argv.remove("--verbose")
    main(in_path=sys.argv[1], out_path=sys.argv[2], version=sys.argv[3])
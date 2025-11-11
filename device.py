import configparser
import os
import sys
from configparser import ConfigParser
from os import write

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

def exit_with_error(message):
    print_error(message)
    sys.exit(1)

def print_help():
    print("Usage: python device.py [device_id]\n\n")
    print("\t[device_id]            the device identifier (folder name in Boards/)")

def list_devices():
    boards = os.listdir("Boards")
    boards.remove("simulator")
    return boards

def read_file(path: str):
    with open(path, "r") as file:
        result = file.read()
        file.close()
        return result

def read_properties_file(path):
    config = configparser.RawConfigParser()
    config.read(path)
    return config

def read_device_properties(device_id):
    device_file_path = os.path.join("Boards", device_id, "device.properties")
    if not os.path.isfile(device_file_path):
        exit_with_error(f"Device file not found: {device_file_path}")
    return read_properties_file(device_file_path)

def get_property_or_exit(properties: ConfigParser, group: str, key: str):
    if not group in properties.sections():
        exit_with_error(f"Device properties does not contain group: {group}")
    if not key in properties[group].keys():
        exit_with_error(f"Device properties does not contain key: {key}")
    return properties[group][key]

def get_property_or_none(properties: ConfigParser, group: str, key: str):
    if not group in properties.sections():
        return None
    if not key in properties[group].keys():
        return None
    return properties[group][key]

def get_boolean_property_or_false(properties: ConfigParser, group: str, key: str):
    if not group in properties.sections():
        return False
    if not key in properties[group].keys():
        return False
    return properties[group][key] == "true"

def write_defaults(output_file):
    default_properties = read_file("Buildscripts/sdkconfig/default.properties")
    output_file.write(default_properties)

def write_partition_table(output_file, device_properties: ConfigParser):
    flash_size = get_property_or_exit(device_properties, "hardware", "flashSize")
    if not flash_size.endswith("MB"):
        exit_with_error("Flash size should be written as xMB or xxMB (e.g. 4MB, 16MB)")
    flash_size_number = flash_size[:-2]
    output_file.write("CONFIG_PARTITION_TABLE_CUSTOM=y\n")
    output_file.write(f"CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions-{flash_size_number}mb.csv\"\n")
    output_file.write(f"CONFIG_PARTITION_TABLE_FILENAME=\"partitions-{flash_size_number}mb.csv\"\n")

def write_tactility_variables(output_file, device_properties: ConfigParser, device_id: str):
    device_selector_name = device_id.upper().replace("-", "_")
    device_selector = f"CONFIG_TT_BOARD_{device_selector_name}"
    output_file.write(f"{device_selector}=y\n")
    board_vendor = get_property_or_exit(device_properties, "general", "vendor").replace("\"", "\\\"")
    board_name = get_property_or_exit(device_properties, "general", "name").replace("\"", "\\\"")
    if board_name == board_vendor or board_vendor == "":
        output_file.write(f"CONFIG_TT_BOARD_NAME=\"{board_name}\"\n")
    else:
        output_file.write(f"CONFIG_TT_BOARD_NAME=\"{board_vendor} {board_name}\"\n")
    output_file.write(f"CONFIG_TT_BOARD_ID=\"{device_id}\"\n")

def write_target_variables(output_file, device_properties: ConfigParser):
    idf_target = get_property_or_exit(device_properties, "hardware", "target")
    output_file.write(f"CONFIG_IDF_TARGET=\"{idf_target.lower()}\"\n")
    output_file.write("CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y\n")
    output_file.write("CONFIG_ESP32_DEFAULT_CPU_FREQ_240=y\n")

def write_flash_variables(output_file, device_properties: ConfigParser):
    flash_size = get_property_or_exit(device_properties, "hardware", "flashSize")
    if not flash_size.endswith("MB"):
        exit_with_error("Flash size should be written as xMB or xxMB (e.g. 4MB, 16MB)")
    flash_size_number = flash_size[:-2]
    output_file.write(f"CONFIG_ESPTOOLPY_FLASHSIZE_{flash_size_number}MB=y\n")
    flash_mode = get_property_or_none(device_properties, "hardware", "flashMode")
    if flash_mode is not None:
        output_file.write(f"CONFIG_FLASHMODE_{flash_mode}=y\n")
    else:
        output_file.write("CONFIG_FLASHMODE_QIO=y\n")
    esptool_flash_freq = get_property_or_none(device_properties, "hardware", "esptoolFlashFreq")
    if esptool_flash_freq is not None:
        output_file.write(f"CONFIG_ESPTOOLPY_FLASHFREQ_{esptool_flash_freq}=y\n")

def write_spiram_variables(output_file, device_properties: ConfigParser):
    idf_target = get_property_or_exit(device_properties, "hardware", "target")
    has_spiram = get_property_or_exit(device_properties, "hardware", "spiRam")
    if has_spiram != "true":
        return
    output_file.write("# Hardware: SPIRAM\n")
    # Enable
    if idf_target == "ESP32S3":
        output_file.write("CONFIG_ESP32S3_SPIRAM_SUPPORT=y\n")
    else:
        output_file.write("CONFIG_ESP32_SPIRAM_SUPPORT=y\n")
        output_file.write("CONFIG_SPIRAM=y\n")
    mode = get_property_or_exit(device_properties, "hardware", "spiRamMode")
    # Mode
    if mode != "AUTO":
        output_file.write(f"CONFIG_SPIRAM_MODE_{mode}=y\n")
    else:
        output_file.write("CONFIG_SPIRAM_TYPE_AUTO=y\n")
    speed = get_property_or_exit(device_properties, "hardware", "spiRamSpeed")
    # Speed
    output_file.write(f"CONFIG_SPIRAM_SPEED_{speed}=y\n")
    # IRAM memory optimization
    output_file.write("CONFIG_SPIRAM_USE_MALLOC=y\n")
    output_file.write("CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y\n")
    # Boot speed optimization
    output_file.write("CONFIG_SPIRAM_MEMTEST=n\n")

def write_lvgl_variables(output_file, device_properties: ConfigParser):
    dpi = get_property_or_exit(device_properties, "display", "dpi")
    output_file.write("# LVGL\n")
    output_file.write("CONFIG_LV_DISP_DEF_REFR_PERIOD=10\n")
    output_file.write(f"CONFIG_LV_DPI_DEF={dpi}\n")
    color_depth = get_property_or_exit(device_properties, "lvgl", "colorDepth")
    output_file.write(f"CONFIG_LV_COLOR_DEPTH={color_depth}\n")
    output_file.write(f"CONFIG_LV_COLOR_DEPTH_{color_depth}=y\n")
    theme = get_property_or_exit(device_properties, "lvgl", "theme")
    if theme == "DefaultDark":
        output_file.write("CONFIG_LV_THEME_DEFAULT_DARK=y\n")
    elif theme == "DefaultLight":
        output_file.write("CONFIG_LV_THEME_DEFAULT_LIGHT=y\n")
    elif theme == "Mono":
        output_file.write("CONFIG_LV_THEME_DEFAULT_DARK=y\n")
        output_file.write("CONFIG_LV_THEME_MONO=y\n")
    else:
        exit_with_error(f"Unknown theme: {theme}")

def write_iram_fix(output_file, device_properties: ConfigParser):
    idf_target = get_property_or_exit(device_properties, "hardware", "target")
    if idf_target == "ESP32":
        output_file.write("# Fix for IRAM\n")
        output_file.write("CONFIG_FREERTOS_PLACE_FUNCTIONS_INTO_FLASH=y\n")
        output_file.write("CONFIG_FREERTOS_PLACE_SNAPSHOT_FUNS_INTO_FLASH=y\n")
        output_file.write("CONFIG_HEAP_PLACE_FUNCTION_INTO_FLASH=y\n")
        output_file.write("CONFIG_RINGBUF_PLACE_FUNCTIONS_INTO_FLASH=y\n")

def write_usb_variables(output_file, device_properties: ConfigParser):
    has_tiny_usb = get_property_or_exit(device_properties, "hardware", "tinyUsb")
    if has_tiny_usb == "true":
        output_file.write("# USB\n")
        output_file.write("CONFIG_TINYUSB_MSC_ENABLED=y\n")
        output_file.write("CONFIG_TINYUSB_MSC_MOUNT_PATH=\"/sdcard\"\n")

def write_custom_sdkconfig(output_file, device_properties: ConfigParser):
    if "sdkconfig" in device_properties.sections():
        section = device_properties["sdkconfig"]
        for key in section.keys():
            value = section[key].replace("\"", "\\\"")
            output_file.write(f"{key.upper()}={value}\n")

def write_rgb_display_glitch_fix(output_file, device_properties: ConfigParser):
    enabled = get_boolean_property_or_false(device_properties, "hardware", "fixRgbDisplayGlitch")
    if enabled:
        output_file.write("# Fixes glitches in the display driver when rendering new screens/apps\n")
        output_file.write("CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y\n")

def write_properties(output_file, device_properties: ConfigParser, device_id: str):
    write_defaults(output_file)
    output_file.write("\n\n# Hardware: Main\n")
    write_partition_table(output_file, device_properties)
    write_tactility_variables(output_file, device_properties, device_id)
    write_target_variables(output_file, device_properties)
    write_flash_variables(output_file, device_properties)
    write_spiram_variables(output_file, device_properties)
    write_lvgl_variables(output_file, device_properties)
    write_iram_fix(output_file, device_properties)
    write_usb_variables(output_file, device_properties)
    write_custom_sdkconfig(output_file, device_properties)
    write_rgb_display_glitch_fix(output_file, device_properties)

def main(device_id: str):
    devices = list_devices()
    # if device_id not in devices:
    #     exit_with_error(f"{device_id} is not a valid device identifier")
    for device_id in devices:
        print(device_id)
        output_file_path = f"sdkconfig.board.{device_id}"
        if os.path.isfile(output_file_path):
            os.remove(output_file_path)
        device_properties = read_device_properties(device_id)
        with open(output_file_path, "w") as output_file:
            write_properties(output_file, device_properties, device_id)
            output_file.close()

if __name__ == "__main__":
    if "--help" in sys.argv:
        print_help()
        sys.exit()
    if len(sys.argv) != 2:
        print_help()
        sys.exit()
    main(sys.argv[1])

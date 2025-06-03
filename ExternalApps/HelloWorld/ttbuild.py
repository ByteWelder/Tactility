import sys
import os

def printWarning(message):
    if sys.platform == 'win32':
        print(f"ERROR: {message}")
    else:
        print(f"\033[93mWARNING: {message}\033[m")

def printError(message):
    if sys.platform == 'win32':
        print(f"ERROR: {message}")
    else:
        print(f"\033[91mERROR: {message}\033[m")

def exitWithError(message):
    printError(message)
    sys.exit(1)

def isValidPlatformName(name):
    return name == "all" or name == "esp32" or name == "esp32s3"

def build(platformName):
    print(f"Platform: {platformName}")
    os.system(f"cp sdkconfig.{platformName} sdkconfig")
    os.system(f"idf.py -B build-{platformName} build")

def buildAll():
    build("esp32")
    build("esp32s3")

def printHelp():
    print("Usage:")
    print("\tpython ttbuild.py [platformName]")
    print("\tplatformName:   all|esp32|esp32s3")

if __name__ == "__main__":
    print("Tactility Build System v0.1.0")
    # Environment
    if os.environ.get('IDF_PATH') == None:
        exitWithError("IDF is not installed or activated. Ensure you installed the toolset and ran the export command.")
    if os.environ.get('TACTILITY_SDK_PATH') != None:
        printWarning("TACTILITY_SDK_PATH is set, but will be ignored by this command")
    os.environ['TACTILITY_SDK_PATH'] = '../../release/TactilitySDK'
    # Argument validation
    if len(sys.argv) == 1:
        printHelp()
        sys.exit()
    platformName = sys.argv[1]
    if not isValidPlatformName(platformName):
        printHelp()
        exitWithError("Invalid platform name")
    # Build
    if platformName == "all":
        buildAll()
    else:
        build(sys.argv[1])


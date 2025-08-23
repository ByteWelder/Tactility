import os

def generate(csvFile, headerFile, headerNamespace, dataPath):
    if os.path.isfile(csvFile):
        print(f"Generating {headerFile}")
        os.system(f"python generate.py {csvFile} {headerFile} {headerNamespace} {dataPath}")
    else:
        print(f"Skipping {headerFile} (not found)")

if __name__ == "__main__":
    generate("system.csv", "../Tactility/Include/Tactility/i18n/Core.h", "tt::i18n::core", "../Data/data/i18n/core")
    generate("launcher.csv", "../Tactility/Include/Tactility/i18n/Launcher.h", "tt::i18n::launcher", "../Data/data/i18n/launcher")

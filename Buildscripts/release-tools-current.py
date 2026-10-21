#!/usr/bin/env python3
"""Exports the tools that tt runs to release/sdk/<version>/tools/ and the new project to release/sdk/<version>/new-project.zip, for tt with TACTILITY_SDK_PATH=release."""

import os
import shutil
import sys

ROOT_PATH = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(ROOT_PATH, "Buildscripts", "CDN"))

from new_project import create_new_project_zip
TOOLS_PATH = os.path.join(ROOT_PATH, "Buildscripts", "TactilitySDK", "Tools")


if __name__ == "__main__":
    with open(os.path.join(ROOT_PATH, "version.txt")) as file:
        version = file.read().strip()
    output_path = os.path.join(ROOT_PATH, "release", "sdk", version, "tools")
    shutil.rmtree(output_path, ignore_errors=True)
    os.makedirs(output_path)
    for file_name in sorted(os.listdir(TOOLS_PATH)):
        if file_name.endswith(".py"):
            shutil.copy2(os.path.join(TOOLS_PATH, file_name), os.path.join(output_path, file_name))
    # Next to the tools, where tt looks for the new project of a local SDK
    create_new_project_zip(os.path.dirname(output_path))
    print(f"Exported the tools to {output_path}")

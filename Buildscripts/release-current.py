#!/usr/bin/env python3
"""Releases the SDK of the current build and the tools to release/sdk/<version>/, for "tt local" with TACTILITY_SDK_PATH=release."""

import os
import subprocess
import sys

SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))


def run(script_name):
    result = subprocess.run([sys.executable, os.path.join(SCRIPT_DIRECTORY, script_name)], cwd=os.path.dirname(SCRIPT_DIRECTORY))
    if result.returncode != 0:
        print(f"Error: {script_name} failed with return code {result.returncode}")
        sys.exit(result.returncode)


if __name__ == "__main__":
    run("release-sdk-current.py")
    run("release-tools-current.py")

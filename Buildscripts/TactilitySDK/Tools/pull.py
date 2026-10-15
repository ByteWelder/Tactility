#!/usr/bin/env python3
# License: Apache License v2
"""Copies a file from the device through its development server. Run it as "tt pull"."""

import argparse
import os
import sys

import shell


def main():
    shell.PROGRAM_NAME = "tt pull" if "TACTILITY_SDK_VERSION" in os.environ else "pull.py"
    parser = argparse.ArgumentParser(prog=shell.PROGRAM_NAME, description="copy a file from the device", add_help=False)
    shell.add_connection_arguments(parser)
    shell.add_pull_arguments(parser)
    sys.exit(shell.run_pull(shell.parse_arguments(parser)))


if __name__ == "__main__":
    import _bootstrap
    _bootstrap.run()
    main()

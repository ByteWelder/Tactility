#!/usr/bin/env python3
# License: Apache License v2
"""Copies a file to the device through its development server. Run it as "tt push"."""

import argparse
import os
import sys

import shell


def main():
    shell.PROGRAM_NAME = "tt push" if "TACTILITY_SDK_VERSION" in os.environ else "push.py"
    parser = argparse.ArgumentParser(prog=shell.PROGRAM_NAME, description="copy a file to the device", add_help=False)
    shell.add_connection_arguments(parser)
    shell.add_push_arguments(parser)
    sys.exit(shell.run_push(shell.parse_arguments(parser)))


if __name__ == "__main__":
    import _bootstrap
    _bootstrap.run()
    main()

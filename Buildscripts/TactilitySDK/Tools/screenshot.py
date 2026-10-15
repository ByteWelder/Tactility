#!/usr/bin/env python3
# License: Apache License v2
"""Takes a screenshot of the device through its development server. Run it as "tt screenshot"."""

import argparse
import datetime
import os
import sys

import shell

# Encoding the PNG can take a while on a large display
SCREENSHOT_TIMEOUT_SECONDS = 30


def main():
    shell.PROGRAM_NAME = "tt screenshot" if "TACTILITY_SDK_VERSION" in os.environ else "screenshot.py"
    parser = argparse.ArgumentParser(prog=shell.PROGRAM_NAME, description="take a screenshot of the device", add_help=False)
    shell.add_connection_arguments(parser)
    parser.add_argument("local_path", nargs="?", help="the .png file on this computer (default: screenshot-<yyyy-mm-dd>.png)")
    args = shell.parse_arguments(parser)
    if args.local_path is None:
        args.local_path = f"screenshot-{datetime.date.today().isoformat()}.png"

    # The device saves the screenshot in its temp folder and prints the path
    exit_code, output = shell.run_command(args.host, args.port, "screenshot", SCREENSHOT_TIMEOUT_SECONDS)
    if exit_code != 0:
        shell.fail(output.strip() or "the device failed to take a screenshot")
    remote_path = output.strip().splitlines()[-1]

    try:
        shell.run_pull(argparse.Namespace(host=args.host, port=args.port, remote_path=remote_path, local_path=args.local_path))
    finally:
        shell.run_command(args.host, args.port, f"rm {remote_path}")
    sys.exit(0)


if __name__ == "__main__":
    import _bootstrap
    _bootstrap.run()
    main()

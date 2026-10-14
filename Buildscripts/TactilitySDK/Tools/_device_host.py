# License: Apache License v2
"""The device host that "tt host" stores for the directory that tt runs in."""

import os

# Absolute, so the path stays the same when a tool changes its working directory
DEVICE_HOST_PATH = os.path.abspath(os.path.join(".tactility", "device_host"))
MISSING_HOST_MESSAGE = "no device host: pass -h/--host or run 'tt host <hostname>'"


def read_device_host():
    """Returns None when no host is stored"""
    try:
        with open(DEVICE_HOST_PATH) as file:
            return file.read().strip() or None
    except OSError:
        return None


def write_device_host(host):
    os.makedirs(os.path.dirname(DEVICE_HOST_PATH), exist_ok=True)
    with open(DEVICE_HOST_PATH, "w") as file:
        file.write(f"{host}\n")

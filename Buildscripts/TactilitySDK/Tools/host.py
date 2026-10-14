#!/usr/bin/env python3
# License: Apache License v2
"""Shows or stores the host name of the device in .tactility/device_host.
"tt host" prints the host or "none", "tt host <hostname>" stores it."""

import sys

import _device_host


def main():
    if len(sys.argv) == 1:
        print(_device_host.read_device_host() or "none")
    elif len(sys.argv) == 2 and sys.argv[1].strip():
        host = sys.argv[1].strip()
        _device_host.write_device_host(host)
        print(f"Device host: {host}")
    else:
        print("usage: tt host [hostname]", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    import _bootstrap
    _bootstrap.run()
    main()

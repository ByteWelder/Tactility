#!/usr/bin/env python3

"""
Buildscripts/build_single_bin.py

A tool to merge ESP-IDF build artifacts into a single flat binary.

Options:
  --build-cmd    Shell command to run to build the project (default: "idf.py build")
  --build-dir    Build directory to read artifacts from (default: "build")
  --no-build     Skip running the build command
  --out          Output file path (default: build/{board}-single_file.bin)
  --fill         Fill byte for gaps (hex like 0xFF or decimal). Default: 0xFF
  --force        Overwrite existing output without asking
  --verbose      Print debug info
"""

from __future__ import annotations
import argparse
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple, Optional

DEFAULT_BUILD_CMD = "idf.py build"
DEFAULT_BUILD_DIR = "build"
DEFAULT_FILL = 0xFF
FLASH_ARGS_FILENAME = "flash_args"  # text file with lines like "0x10000 app.bin"

OFFSET_LINE_RE = re.compile(r"^\s*(0x[0-9A-Fa-f]+|\d+)\s+(.+?)\s*$")


def run_cmd(cmd: str) -> int:
    print("Running build command:", cmd)
    return subprocess.call(cmd, shell=True)


def read_flash_args(build_dir: Path, verbose: bool = False) -> List[Tuple[int, str]]:
    """
    Read build/flash_args (text) and parse offset/filename lines only.
    Return list of (offset:int, filename:str) sorted by offset ascending.
    """
    path = build_dir / FLASH_ARGS_FILENAME
    if not path.is_file():
        raise FileNotFoundError(f"Expected flash args file at: {path}")
    if verbose:
        print(f"Reading flash args from: {path}")
    pairs: List[Tuple[int, str]] = []
    with path.open("r", encoding="utf-8") as fh:
        for lineno, raw in enumerate(fh, start=1):
            line = raw.strip()
            if not line:
                continue
            # ignore option lines beginning with --
            if line.startswith("--"):
                if verbose:
                    print(f"Skipping option line {lineno}: {line}")
                continue
            m = OFFSET_LINE_RE.match(line)
            if not m:
                if verbose:
                    print(f"Ignoring unrecognized line {lineno}: {line}")
                continue
            offset_str, filename = m.group(1), m.group(2)
            try:
                offset = int(offset_str, 16) if offset_str.lower().startswith("0x") else int(offset_str)
            except Exception as e:
                raise ValueError(f"Could not parse offset on line {lineno}: {offset_str}") from e
            pairs.append((offset, filename))
    # sort by offset ascending so merging is deterministic (not required but nice :D)
    pairs.sort(key=lambda x: x[0])
    if verbose:
        print("Parsed flash entries:")
        for off, fn in pairs:
            print(f"  0x{off:X} -> {fn}")
    return pairs


def resolve_in_build(build_dir: Path, filename: str) -> Path:
    """
    Resolve a filename by joining with build_dir.
    No recursive searching. If the resulting path doesn't exist, raise.
    """
    candidate = build_dir / filename
    # Prevent path traversal out of build_dir
    try:
        candidate_resolved = candidate.resolve(strict=False)
        build_resolved = build_dir.resolve(strict=True)
        # If candidate not inside build_dir, it's not allowed.
        if not str(candidate_resolved).startswith(str(build_resolved)):
            raise FileNotFoundError(f"Resolved path would be outside build dir: {candidate}")
    except FileNotFoundError:
        # candidate.resolve(strict=False) can still be fine; we only ensure candidate path is under build_dir syntactically.
        pass
    if not candidate.exists():
        raise FileNotFoundError(f"Expected file in build dir but not found: {candidate}")
    return candidate


def read_board_id_from_sdkconfig(sdkconfig_path: Path) -> Optional[str]:
    """
    Read sdkconfig and return CONFIG_TT_BOARD_ID value if present (unquoted).
    """
    if not sdkconfig_path.is_file():
        return None
    with sdkconfig_path.open("r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("CONFIG_TT_BOARD_ID="):
                # Could be CONFIG_TT_BOARD_ID="board-name"
                val = line.partition("=")[2].strip()
                # strip quotes if present
                if val.startswith('"') and val.endswith('"') and len(val) >= 2:
                    val = val[1:-1]
                return val
    return None


def merge_and_write(blobs: List[Tuple[int, bytes]], out_path: Path, fill: int, verbose: bool = False) -> None:
    if not blobs:
        raise ValueError("No blobs to merge.")
    max_end = max(offset + len(data) for offset, data in blobs)
    if max_end == 0:
        raise ValueError("Merged size would be zero.")
    if verbose:
        print(f"Creating merged image of {max_end} bytes (fill=0x{fill:02X})")
    image = bytearray([fill] * max_end)
    for offset, data in blobs:
        if offset < 0:
            raise ValueError(f"Negative offset {offset}")
        image[offset:offset + len(data)] = data
        if verbose:
            print(f"Placed {len(data)} bytes at 0x{offset:X}")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(image)
    print(f"Wrote merged image: {out_path} ({len(image)} bytes)")


def main():
    parser = argparse.ArgumentParser(description="Merge build/flash_args entries into a single flat binary.")
    parser.add_argument("--build-cmd", default=DEFAULT_BUILD_CMD, help="Build command to run (shell). Default: 'idf.py build'")
    parser.add_argument("--build-dir", default=DEFAULT_BUILD_DIR, help="Build directory where artifacts exist (default: build)")
    parser.add_argument("--no-build", action="store_true", help="Skip running the build command")
    parser.add_argument("--out", default=None, help="Output file path (default: build/{board}-single_file.bin)")
    parser.add_argument("--fill", default=hex(DEFAULT_FILL), help="Fill byte for gaps (hex like 0xFF). Default: 0xFF")
    parser.add_argument("--force", action="store_true", help="Overwrite existing output")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    build_dir = Path(args.build_dir)
    if not build_dir.is_dir():
        print(f"Error: build directory not found: {build_dir}", file=sys.stderr)
        sys.exit(2)

    if not args.no_build:
        rc = run_cmd(args.build_cmd)
        if rc != 0:
            print(f"Build command failed with exit code {rc}", file=sys.stderr)
            sys.exit(rc)
    elif args.verbose:
        print("Skipping build step (--no-build)")

    # Parse flash_args only (text file) under build/*
    try:
        entries = read_flash_args(build_dir, verbose=args.verbose)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(3)
    except Exception as e:
        print(f"Error parsing flash args: {e}", file=sys.stderr)
        sys.exit(4)

    # Build blobs by reading resolved files from build_dir only
    blobs: List[Tuple[int, bytes]] = []
    for offset, filename in entries:
        try:
            fp = resolve_in_build(build_dir, filename)
        except FileNotFoundError as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(5)
        if args.verbose:
            print(f"Loading {fp} at 0x{offset:X}")
        data = fp.read_bytes()
        blobs.append((offset, data))

    # Determine board id from sdkconfig in repo root
    repo_root = Path.cwd()
    sdkconfig_path = repo_root / "sdkconfig"
    board_id = read_board_id_from_sdkconfig(sdkconfig_path) or "unknown-board"

    # Default output path
    default_out = build_dir / f"{board_id}-single_file.bin"
    out_path = Path(args.out) if args.out else default_out

    if out_path.exists() and not args.force:
        print(f"Output '{out_path}' already exists. Use --force to overwrite.", file=sys.stderr)
        sys.exit(6)

    # parse fill
    try:
        fill_raw = str(args.fill)
        fill = int(fill_raw, 16) if fill_raw.lower().startswith("0x") else int(fill_raw)
        if not (0 <= fill <= 0xFF):
            raise ValueError("fill must be 0-255")
    except Exception as e:
        print(f"Invalid --fill value: {e}", file=sys.stderr)
        sys.exit(7)

    try:
        merge_and_write(blobs, out_path, fill=fill, verbose=args.verbose)
    except Exception as e:
        print(f"Failed to merge/write: {e}", file=sys.stderr)
        sys.exit(8)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

# Functions shared between release-sdk-esp32.py and release-sdk-posix.py. Not runnable on its
# own; loaded by those scripts via importlib (its hyphenated filename isn't a valid Python
# module name for a plain "import").

import os
import shutil
import glob
import sys

def map_copy(mappings, target_base):
    """
    Helper function to map input files/directories to output files/directories.
    mappings: list of dicts with 'src' (glob pattern) and 'dst' (relative to target_base or absolute)
    'src' can be a single file or a directory (if it ends with /).
    """
    for mapping in mappings:
        src_pattern = mapping['src']
        dst_rel = mapping['dst']
        dst_path = os.path.join(target_base, dst_rel)

        # To preserve directory structure, we need to know where the wildcard starts
        # or have a way to determine the "base" of the search.
        # We'll split the pattern into a fixed base and a pattern part.

        # Simple heuristic: find the first occurrence of '*' or '?'
        wildcard_idx = -1
        for i, char in enumerate(src_pattern):
            if char in '*?':
                wildcard_idx = i
                break

        if wildcard_idx != -1:
            # Found a wildcard. The base is the directory containing it.
            pattern_base = os.path.dirname(src_pattern[:wildcard_idx])
        else:
            # No wildcard. If it's a directory, we might want to preserve its name?
            # For now, let's treat no-wildcard as no relative structure needed.
            pattern_base = None

        src_files = glob.glob(src_pattern, recursive=True)
        if not src_files:
            continue

        for src in src_files:
            if os.path.isdir(src):
                continue

            if pattern_base and src.startswith(pattern_base):
                # Calculate relative path from the base of the glob pattern
                rel_src = os.path.relpath(src, pattern_base)
                # If dst_rel ends with /, it's a target directory
                if dst_rel.endswith('/') or os.path.isdir(dst_path):
                    final_dst = os.path.join(dst_path, rel_src)
                else:
                    # If dst_rel is a file, we can't really preserve structure
                    # unless we join it. But usually it's a dir if structure is preserved.
                    final_dst = dst_path
            else:
                final_dst = dst_path if not (dst_rel.endswith('/') or os.path.isdir(dst_path)) else os.path.join(dst_path, os.path.basename(src))

            os.makedirs(os.path.dirname(final_dst), exist_ok=True)
            shutil.copy2(src, final_dst)

def write_module_cmakelists(path, content):
    with open(path, 'w') as f:
        f.write(content)

def read_module_list(path):
    """Reads a newline-separated module name list, skipping empty lines, and checks that each
    named module actually exists under Modules/ - exits the process with an error if not."""
    with open(path, 'r') as f:
        module_names = [line.strip() for line in f if line.strip()]

    for module_name in module_names:
        if not os.path.isdir(os.path.join('Modules', module_name)):
            print(f"Error: Modules/{module_name} does not exist (listed in {path})")
            sys.exit(1)

    return module_names

def generate_tactility_sdk_cmake(target_path, variant):
    """variant selects Buildscripts/TactilitySDK/TactilitySDK.{variant}.cmake (e.g. "esp32" or
    "posix") - always copied into the SDK as the platform-neutral name TactilitySDK.cmake."""
    src = os.path.join('Buildscripts', 'TactilitySDK', f'TactilitySDK.{variant}.cmake')
    shutil.copy2(src, os.path.join(target_path, 'TactilitySDK.cmake'))

def generate_tactility_sdk_top_cmakelists(target_path):
    src = os.path.join('Buildscripts', 'TactilitySDK', 'CMakeLists.txt')
    shutil.copy2(src, os.path.join(target_path, 'CMakeLists.txt'))

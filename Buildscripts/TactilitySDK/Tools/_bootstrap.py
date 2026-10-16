#!/usr/bin/env python3
# License: Apache License v2
"""Run by the tools at startup, when tt runs them. Checks the CDN for a newer tt and a newer build of the selected SDK at most every 30 minutes, and suggests how to update."""

import json
import os
import re
import sys
import time
import urllib.request

CDN_URL = os.environ.get("TT_CDN", "https://cdn.tactilityproject.org")
STATE_PATH = os.path.join(os.path.expanduser("~"), ".tactility", "tt.json")
CHECK_INTERVAL_SECONDS = 30 * 60


def parse_version(version):
    return tuple(int(part) for part in re.findall(r"\d+", version)[:3])


def read_state():
    try:
        with open(STATE_PATH) as file:
            state = json.load(file)
            return state if isinstance(state, dict) else {}
    except (OSError, ValueError):
        return {}


def write_state(state):
    try:
        os.makedirs(os.path.dirname(STATE_PATH), exist_ok=True)
        with open(STATE_PATH, "w") as file:
            json.dump(state, file)
    except OSError:
        pass


def fetch_json(url):
    """Returns None when the CDN can't be reached"""
    try:
        request = urllib.request.Request(url, headers={"User-Agent": "tt bootstrap"})
        with urllib.request.urlopen(request, timeout=5) as response:
            return json.load(response)
    except (OSError, ValueError):
        return None


def check_cdn(state, sdk_version):
    """Stores the latest tt version and SDK build in the state, keeping the previous ones when the CDN can't be reached"""
    state["checked"] = time.time()
    tt_json = fetch_json(f"{CDN_URL}/sdk/tt.json")
    if isinstance(tt_json, dict):
        state["version"] = tt_json.get("version")
    if sdk_version is not None:
        index = fetch_json(f"{CDN_URL}/sdk/{sdk_version}/index.json")
        if isinstance(index, dict):
            state.setdefault("sdk", {})[sdk_version] = index.get("gitCommit")
            tools = index.get("tools")
            state.setdefault("sdk_tools", {})[sdk_version] = tools.get("sha256") if isinstance(tools, dict) else None
    write_state(state)


def installed_commits(version_directory):
    prefix = "platform-"
    commits = []
    if not os.path.isdir(version_directory):
        return commits
    for name in os.listdir(version_directory):
        platform_directory = os.path.join(version_directory, name)
        if name.startswith(prefix) and os.path.isdir(os.path.join(platform_directory, "TactilitySDK")):
            try:
                with open(os.path.join(platform_directory, "commit")) as file:
                    commits.append(file.read().strip())
            except OSError:
                commits.append(None)
    return commits


def installed_tools_hash(version_directory):
    try:
        with open(os.path.join(version_directory, "tools.sha256")) as file:
            return file.read().strip()
    except OSError:
        return None


def run():
    tt_version = os.environ.get("TT_VERSION")
    if tt_version is None:
        return
    sdk_version = os.environ.get("TACTILITY_SDK_VERSION")
    sdk_path = os.environ.get("TACTILITY_SDK_PATH")
    # A local SDK isn't managed by tt
    if os.environ.get("TT_LOCAL") == "1":
        sdk_version = None
    state = read_state()
    is_unchecked_sdk = sdk_version is not None and sdk_version not in state.get("sdk", {})
    if time.time() - state.get("checked", 0) >= CHECK_INTERVAL_SECONDS or is_unchecked_sdk:
        check_cdn(state, sdk_version)
    latest = state.get("version")
    if isinstance(latest, str) and parse_version(latest) > parse_version(tt_version):
        print(f"tt: version {latest} is available (currently {tt_version}): run 'tt update-self'", file=sys.stderr)
    if sdk_version is None or sdk_path is None:
        return
    version_directory = os.path.join(sdk_path, "sdk", sdk_version)
    latest_commit = state.get("sdk", {}).get(sdk_version)
    latest_tools = state.get("sdk_tools", {}).get(sdk_version)
    has_newer_platforms = latest_commit and any(commit != latest_commit for commit in installed_commits(version_directory))
    has_newer_tools = latest_tools and installed_tools_hash(version_directory) != latest_tools
    if has_newer_platforms or has_newer_tools:
        print(f"tt: a newer build of SDK {sdk_version} is available: run 'tt update-sdk'", file=sys.stderr)


if __name__ == "__main__":
    run()

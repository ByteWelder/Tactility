"""Tests for tt.py against a local fake CDN. Run with: python3 -m unittest discover -s Tests/Python -p '*_test.py'"""

import functools
import hashlib
import http.server
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest
import zipfile

TT_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "tt.py"))
TOOLS_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "Buildscripts", "TactilitySDK", "Tools"))
VERSION = "0.8.0-dev"

ECHO_TOOL = """import json, os, sys
import _bootstrap
_bootstrap.run()
print(json.dumps({"arguments": sys.argv[1:], "version": os.environ.get("TACTILITY_SDK_VERSION"), "path": os.environ.get("TACTILITY_SDK_PATH"), "cwd": os.getcwd()}))
sys.exit(3)
"""


def read_tool(file_name):
    with open(os.path.join(TOOLS_PATH, file_name)) as file:
        return file.read()


def make_zip(files):
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w") as archive:
        for name, content in files.items():
            archive.writestr(name, content)
    return buffer.getvalue()


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass


class TtTest(unittest.TestCase):

    def setUp(self):
        self.root = tempfile.mkdtemp()
        self.cdn = os.path.join(self.root, "cdn")
        self.home = os.path.join(self.root, "home")
        self.work = os.path.join(self.root, "work")
        os.makedirs(self.home)
        os.makedirs(self.work)
        self.publish_sdk(self.default_tools())
        handler = functools.partial(QuietHandler, directory=self.cdn)
        self.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
        threading.Thread(target=self.server.serve_forever, daemon=True).start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        shutil.rmtree(self.root)

    def default_tools(self):
        return {name: read_tool(name) for name in ("update-self.py", "update-sdk.py", "_bootstrap.py")} | {"echo.py": ECHO_TOOL}

    def publish_sdk(self, tools, commit="commit1", sdk_content="# sdk"):
        version_directory = os.path.join(self.cdn, "sdk", VERSION)
        os.makedirs(version_directory, exist_ok=True)
        tools_zip = make_zip(tools)
        with open(os.path.join(version_directory, "tools.zip"), "wb") as file:
            file.write(tools_zip)
        with open(os.path.join(version_directory, "TactilitySDK-posix.zip"), "wb") as file:
            file.write(make_zip({"TactilitySDK.cmake": sdk_content}))
        index = {
            "version": VERSION,
            "gitCommit": commit,
            "platforms": {"posix": "TactilitySDK-posix.zip"},
            "tools": {"file": "tools.zip", "sha256": hashlib.sha256(tools_zip).hexdigest()},
        }
        with open(os.path.join(version_directory, "index.json"), "w") as file:
            json.dump(index, file)

    def write_manifest(self, directory, version=VERSION, platforms="posix"):
        os.makedirs(directory, exist_ok=True)
        with open(os.path.join(directory, "manifest.properties"), "w") as file:
            file.write(f"[target]\ntarget.sdk={version}\ntarget.platforms={platforms}\n")

    def run_tt(self, *arguments, tt_path=TT_PATH, environment=None):
        extra_environment = environment or {}
        environment = dict(os.environ)
        environment.pop("TACTILITY_SDK_PATH", None)
        environment.update(extra_environment)
        environment["HOME"] = self.home
        environment["TT_CDN"] = f"http://127.0.0.1:{self.server.server_address[1]}"
        environment.pop("TACTILITY_SDK_VERSION", None)
        return subprocess.run([sys.executable, tt_path, *arguments], cwd=self.work, env=environment, capture_output=True, text=True, timeout=60)

    def sdk_path(self, *parts):
        return os.path.join(self.home, ".tactility", "sdk", VERSION, *parts)

    def expire_index(self):
        os.utime(self.sdk_path("index.json"), (0, 0))

    def test_forwards_arguments_exit_code_and_environment(self):
        self.write_manifest(self.work)
        result = self.run_tt("echo", "a", "b c")
        self.assertEqual(result.returncode, 3, result.stderr)
        output = json.loads(result.stdout)
        self.assertEqual(output["arguments"], ["a", "b c"])
        self.assertEqual(output["version"], VERSION)
        self.assertEqual(output["path"], os.path.join(self.home, ".tactility"))
        self.assertEqual(os.path.realpath(output["cwd"]), os.path.realpath(self.work))
        self.assertTrue(os.path.isfile(self.sdk_path("platform-posix", "TactilitySDK", "TactilitySDK.cmake")))
        self.assertTrue(os.path.isfile(self.sdk_path("tools", "echo.py")))

    def test_current_directory_beats_apps_directories(self):
        self.write_manifest(self.work)
        self.write_manifest(os.path.join(self.work, "Apps", "a"), version="9.9.9")
        self.assertEqual(self.run_tt("echo").returncode, 3)

    def test_apps_directory_beats_lowercase_apps_and_sorts(self):
        self.write_manifest(os.path.join(self.work, "Apps", "b"), version="9.9.9")
        self.write_manifest(os.path.join(self.work, "Apps", "a"))
        self.write_manifest(os.path.join(self.work, "apps", "a"), version="9.9.9")
        self.assertEqual(self.run_tt("echo").returncode, 3)

    def test_lowercase_apps_directory(self):
        self.write_manifest(os.path.join(self.work, "apps", "a"))
        self.assertEqual(self.run_tt("echo").returncode, 3)

    def test_arguments_are_checked_before_the_manifest(self):
        shutil.rmtree(self.cdn)
        result = self.run_tt("help")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Usage: tt", result.stdout)
        result = self.run_tt("../x")
        self.assertEqual(result.returncode, 1)
        self.assertIn("command not found: ../x", result.stderr)
        self.assertNotIn("manifest", result.stderr)

    def test_missing_manifest_fails(self):
        result = self.run_tt("echo")
        self.assertEqual(result.returncode, 1)
        self.assertIn("no manifest.properties found", result.stderr)

    def test_unknown_and_invalid_commands_are_not_found(self):
        self.write_manifest(self.work)
        for command in ("nope", "../echo", "Echo"):
            result = self.run_tt(command)
            self.assertEqual(result.returncode, 1)
            self.assertIn(f"command not found: {command}", result.stderr)

    def test_help_lists_commands(self):
        self.write_manifest(self.work)
        result = self.run_tt()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("echo", result.stdout)
        self.assertIn(VERSION, result.stdout)

    def test_installed_tools_and_platforms_are_only_replaced_by_update_sdk(self):
        self.write_manifest(self.work)
        self.assertEqual(self.run_tt("echo").returncode, 3)
        tools = self.default_tools() | {"echo.py": "import sys\nimport _bootstrap\n_bootstrap.run()\nsys.exit(4)\n"}
        self.publish_sdk(tools, commit="commit2", sdk_content="# sdk 2")
        self.expire_index()
        self.expire_update_check()
        result = self.run_tt("echo")
        self.assertEqual(result.returncode, 3)
        self.assertEqual(self.read_sdk_file(), "# sdk")
        self.assertIn("run 'tt update-sdk'", result.stderr)
        result = self.run_tt("update-sdk")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Updating the tools", result.stdout)
        self.assertEqual(self.run_tt("echo").returncode, 4)
        self.assertEqual(self.read_sdk_file(), "# sdk 2")

    def test_installed_sdk_works_offline(self):
        self.write_manifest(self.work)
        self.assertEqual(self.run_tt("echo").returncode, 3)
        shutil.rmtree(self.cdn)
        self.expire_index()
        result = self.run_tt("echo")
        self.assertEqual(result.returncode, 3, result.stderr)

    def test_tools_with_a_wrong_checksum_are_refused(self):
        self.write_manifest(self.work)
        with open(os.path.join(self.cdn, "sdk", VERSION, "tools.zip"), "wb") as file:
            file.write(make_zip({"echo.py": "tampered"}))
        result = self.run_tt("echo")
        self.assertEqual(result.returncode, 1)
        self.assertIn("checksum", result.stderr)

    def test_local_uses_tactility_sdk_path(self):
        self.write_manifest(self.work)
        local_path = os.path.join(self.root, "local")
        os.makedirs(os.path.join(local_path, "sdk", VERSION, "tools"))
        for name, content in self.default_tools().items():
            with open(os.path.join(local_path, "sdk", VERSION, "tools", name), "w") as file:
                file.write(content)
        shutil.rmtree(self.cdn)
        result = self.run_tt("echo", "a", environment={"TACTILITY_SDK_PATH": local_path})
        self.assertEqual(result.returncode, 3, result.stderr)
        self.assertIn(f"using the local SDK at {local_path}", result.stderr)
        output = json.loads(result.stdout)
        self.assertEqual(output["arguments"], ["a"])
        self.assertEqual(output["path"], local_path)
        self.assertEqual(output["version"], VERSION)
        self.assertFalse(os.path.exists(self.sdk_path()))

    def test_empty_tactility_sdk_path_counts_as_unset(self):
        self.write_manifest(self.work)
        result = self.run_tt("echo", environment={"TACTILITY_SDK_PATH": ""})
        self.assertEqual(result.returncode, 3, result.stderr)
        self.assertNotIn("local SDK", result.stderr)
        self.assertEqual(json.loads(result.stdout)["path"], os.path.join(self.home, ".tactility"))

    def test_local_tools_without_tactility_sdk_path_are_not_used(self):
        self.write_manifest(self.work)
        result = self.run_tt("echo")
        self.assertEqual(result.returncode, 3, result.stderr)
        self.assertNotIn("local SDK", result.stderr)
        self.assertEqual(json.loads(result.stdout)["path"], os.path.join(self.home, ".tactility"))

    def publish_tt(self, files):
        with open(os.path.join(self.cdn, "sdk", "tt.zip"), "wb") as file:
            file.write(make_zip(files))
        with open(os.path.join(self.cdn, "sdk", "tt.json"), "w") as file:
            json.dump({"version": "1.0.1"}, file)

    def test_update_replaces_itself_and_installs_wrappers(self):
        tt_copy = os.path.join(self.root, "tt.py")
        shutil.copy(TT_PATH, tt_copy)
        with open(os.path.join(self.root, "tt.cmd"), "w") as file:
            file.write("@echo off\r\n")
        self.publish_tt({"tt.py": "# updated\n", "tt": "#!/bin/sh\n", "tt.ps1": "exit 0\r\n"})
        self.write_manifest(self.work)
        result = self.run_tt("update-self", tt_path=tt_copy)
        self.assertEqual(result.returncode, 0, result.stderr)
        with open(tt_copy) as file:
            self.assertEqual(file.read(), "# updated\n")
        self.assertTrue(os.access(os.path.join(self.root, "tt"), os.X_OK))
        self.assertTrue(os.access(os.path.join(self.root, "tt.ps1"), os.X_OK))
        self.assertTrue(os.access(tt_copy, os.X_OK))
        # The obsolete tt.cmd is removed
        self.assertEqual(sorted(os.listdir(self.root)), ["cdn", "home", "tt", "tt.ps1", "tt.py", "work"])

    def test_update_rejects_invalid_python(self):
        tt_copy = os.path.join(self.root, "tt.py")
        shutil.copy(TT_PATH, tt_copy)
        self.publish_tt({"tt.py": "def broken(:\n", "tt": "#!/bin/sh\n"})
        self.write_manifest(self.work)
        result = self.run_tt("update-self", tt_path=tt_copy)
        self.assertEqual(result.returncode, 1)
        with open(tt_copy) as file, open(TT_PATH) as original:
            self.assertEqual(file.read(), original.read())
        self.assertEqual(sorted(os.listdir(self.root)), ["cdn", "home", "tt.py", "work"])

    def test_update_rejects_unsafe_file_names(self):
        tt_copy = os.path.join(self.root, "tt.py")
        shutil.copy(TT_PATH, tt_copy)
        self.publish_tt({"tt.py": "# updated\n", "../evil": "x"})
        self.write_manifest(self.work)
        result = self.run_tt("update-self", tt_path=tt_copy)
        self.assertEqual(result.returncode, 1)
        self.assertIn("invalid", result.stderr)

    def test_wrapper_runs_tt(self):
        self.write_manifest(self.work)
        wrapper = os.path.join(os.path.dirname(TT_PATH), "tt")
        environment = dict(os.environ, HOME=self.home, TT_CDN=f"http://127.0.0.1:{self.server.server_address[1]}")
        result = subprocess.run([wrapper, "echo", "x"], cwd=self.work, env=environment, capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 3, result.stderr)
        self.assertEqual(json.loads(result.stdout)["arguments"], ["x"])

    def publish_tt_version(self, version):
        with open(os.path.join(self.cdn, "sdk", "tt.json"), "w") as file:
            json.dump({"version": version}, file)

    def test_bootstrap_suggests_update(self):
        self.write_manifest(self.work)
        self.publish_tt_version("99.0.0")
        result = self.run_tt("echo")
        self.assertEqual(result.returncode, 3)
        self.assertIn("version 99.0.0 is available", result.stderr)
        self.assertIn("tt update-self", result.stderr)

    def test_bootstrap_checks_at_most_every_30_minutes(self):
        self.write_manifest(self.work)
        self.publish_tt_version("1.0.0")
        self.assertNotIn("is available", self.run_tt("echo").stderr)
        self.publish_tt_version("99.0.0")
        self.assertNotIn("is available", self.run_tt("echo").stderr)
        state_path = os.path.join(self.home, ".tactility", "tt.json")
        with open(state_path) as file:
            state = json.load(file)
        state["checked"] -= 31 * 60
        with open(state_path, "w") as file:
            json.dump(state, file)
        self.assertIn("version 99.0.0 is available", self.run_tt("echo").stderr)

    def expire_update_check(self):
        state_path = os.path.join(self.home, ".tactility", "tt.json")
        with open(state_path) as file:
            state = json.load(file)
        state["checked"] -= 31 * 60
        with open(state_path, "w") as file:
            json.dump(state, file)

    def read_sdk_file(self):
        with open(self.sdk_path("platform-posix", "TactilitySDK", "TactilitySDK.cmake")) as file:
            return file.read()

    def test_newer_sdk_build_is_suggested_and_installed_by_update_sdk(self):
        self.write_manifest(self.work)
        self.assertNotIn("update-sdk", self.run_tt("echo").stderr)
        self.publish_sdk(self.default_tools(), commit="commit2", sdk_content="# sdk 2")
        self.assertNotIn("update-sdk", self.run_tt("echo").stderr)
        self.expire_update_check()
        self.assertIn("run 'tt update-sdk'", self.run_tt("echo").stderr)
        self.assertEqual(self.read_sdk_file(), "# sdk")
        result = self.run_tt("update-sdk")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"SDK {VERSION} was updated", result.stdout)
        self.assertEqual(self.read_sdk_file(), "# sdk 2")
        self.assertNotIn("update-sdk", self.run_tt("echo").stderr)
        self.assertIn("is up to date", self.run_tt("update-sdk").stdout)

    def test_local_sdk_is_not_updated(self):
        self.write_manifest(self.work)
        local_path = os.path.join(self.root, "local")
        os.makedirs(os.path.join(local_path, "sdk", VERSION, "platform-posix", "TactilitySDK"))
        os.makedirs(os.path.join(local_path, "sdk", VERSION, "tools"))
        for name, content in self.default_tools().items():
            with open(os.path.join(local_path, "sdk", VERSION, "tools", name), "w") as file:
                file.write(content)
        result = self.run_tt("echo", environment={"TACTILITY_SDK_PATH": local_path})
        self.assertEqual(result.returncode, 3, result.stderr)
        self.assertNotIn("update-sdk", result.stderr)
        result = self.run_tt("update-sdk", environment={"TACTILITY_SDK_PATH": local_path})
        self.assertEqual(result.returncode, 1)
        self.assertIn("local SDK", result.stderr)

    def test_internal_tools_are_not_commands(self):
        self.write_manifest(self.work)
        result = self.run_tt("_bootstrap")
        self.assertEqual(result.returncode, 1)
        self.assertIn("command not found", result.stderr)
        self.assertNotIn("_bootstrap", self.run_tt("help").stdout)


if __name__ == "__main__":
    unittest.main()

# Save this in your directory root as: tactility_tui.py
import json
import os
import re
import sys
import subprocess
import time
import urllib.request
import zipfile
import tarfile
from pathlib import Path
from urllib.parse import urlparse

from textual.app import App, ComposeResult
from textual.containers import Container, Horizontal, Vertical
from textual.widgets import Footer, Button, Log, ListView, ListItem, Label, Select, TabbedContent, TabPane, Input, Checkbox
from textual.binding import Binding

import _device_host

ttbuild_path = os.path.abspath(".tactility")
ttbuild_version = "6.0.0"
ttbuild_cdn = "https://cdn.tactilityproject.org"
ttbuild_sdk_json_validity = 3600
ttport = 6666
http_timeout_seconds = 10
install_timeout_seconds = 120

# Active target logging pointers to feed real-time outputs straight to UI containers
active_ui_logger = None

# app.py does the actual work: it is next to this script
APP_TOOL_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "app.py")
ANSI_ESCAPE_PATTERN = re.compile(r"\x1b\[[0-9;]*m")

def resolve_espressif_environment():
    """Captures Espressif's custom paths and mirrors them globally."""
    activation_profile = r"C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"
    if not os.path.isfile(activation_profile):
        return
    try:
        cmd = ["powershell.exe", "-ExecutionPolicy", "Bypass", "-File", activation_profile, "-e"]
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        for line in result.stdout.splitlines():
            line = line.strip()
            if not line or "=" not in line:
                continue
            key, sep, value = line.partition("=")
            if key.strip() == "PATH":
                os.environ["PATH"] = value.strip() + os.pathsep + os.environ.get("PATH", "")
            else:
                os.environ[key.strip()] = value.strip()
    except Exception:
        pass

resolve_espressif_environment()

# region Core Action Logic Engines (Imported from Original Utility Script)

def tui_print(message):
    """Intercepts and streams prints directly into active log panel views."""
    global active_ui_logger
    if active_ui_logger:
        active_ui_logger.write_line(str(message))
    else:
        print(message)

def download_file(url, filepath):
    tui_print(f"⌛ Downloading from {url}...")
    try:
        request = urllib.request.Request(url, data=None, headers={"User-Agent": f"Tactility Build Tool {ttbuild_version}"})
        with urllib.request.urlopen(request, timeout=30) as response, open(filepath, mode="wb") as file:
            file.write(response.read())
        return True
    except Exception as e:
        tui_print(f"❌ Failed to fetch URL {url}: {e}")
        return False

def read_properties_file(path):
    properties = {}
    if not os.path.isfile(path):
        return properties
    with open(path, "r") as file:
        for line in file:
            stripped = line.strip()
            if not stripped or stripped.startswith("#") or "=" not in line:
                continue
            key, sep, value = line.partition("=")
            properties[key.strip()] = value.strip()
    return properties

class SemanticVersion:
    def __init__(self, major, minor, patch, tag=None):
        self.major, self.minor, self.patch, self.tag = major, minor, patch, tag
    @staticmethod
    def parse(v_str):
        match = re.match(r"^(\d+)\.(\d+)\.(\d+)(?:-(.+))?$", v_str)
        if not match: return SemanticVersion(0,0,0)
        return SemanticVersion(int(match.group(1)), int(match.group(2)), int(match.group(3)), match.group(4))
    def _numeric(self): return (self.major, self.minor, self.patch)
    def is_older_release_of(self, other): return self._numeric() < other._numeric()

# The last output line of the last subprocess, which tells why it failed
last_output_line = ""

def run_native_subprocess(command_args, target_cwd):
    """Executes active compiler builders while flushing out standard line streams instantly."""
    global last_output_line
    last_output_line = ""
    try:
        shell_needed = sys.platform == "win32"
        process = subprocess.Popen(
            command_args,
            # The dashboard owns the terminal, so tools can't ask questions
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=target_cwd,
            shell=shell_needed,
            text=True,
            bufsize=1 # Forces immediate line un-buffering rules down to stdout
        )
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break
            if line:
                # Progress lines are rewritten with a carriage return in a terminal, and colored
                text = ANSI_ESCAPE_PATTERN.sub("", line.rstrip("\r\n")).split("\r")[-1]
                tui_print(text)
                if text.strip():
                    last_output_line = text.strip()
        process.wait()
        return process.returncode == 0
    except Exception as e:
        tui_print(f"❌ Subprocess crash encountered: {e}")
        return False

#endregion Core Action Logic Engines

class TactilityTUI(App):
    """The master native asynchronous dashboard controlling the compilation ecosystem."""
    
    CSS = """
    Screen { background: #121212; }
    #sidebar { width: 32; background: #1a1a1a; border-right: solid #333333; padding: 1; }
    #main_content { padding: 1; }
    #control_panel { height: 7; margin-bottom: 1; background: #161616; border: solid #333333; padding: 1 1 0 1; }
    #network_settings { margin-left: 4; width: 35; }
    .section_title { text-style: bold; color: #00ffcc; margin-bottom: 0; }
    Horizontal Button { margin-right: 1; min-width: 14; height: 3; }
    Log { background: #1e1e1e; color: #ffffff; overflow-x: hidden; text-wrap: wrap; }
    TabbedContent, TabPane, ContentSwitcher { padding: 0; margin: 0; }
    TabbedContent { margin-top: 1; height: 1fr; }
    Input { height: 3; border: tall #333333; }
    ListItem { padding: 1; }
    ListItem:hover { background: #262626; }
    ListItem.--on-selection { background: #00ffcc; color: #121212; text-style: bold; }
    Checkbox { background: transparent; border: none; padding: 0; margin-bottom: 0; }
    """

    BINDINGS = [
        Binding("f5", "action_pipeline('build')", "Build (F5)"),
        Binding("f6", "action_pipeline('install')", "Install (F6)"),
        Binding("f7", "action_pipeline('run')", "Run (F7)"),
        Binding("f8", "action_pipeline('bir')", "Full Cycle BIR (F8)"),
        Binding("f9", "action_pipeline('clean')", "Clean Folders"),
        Binding("q", "quit", "Quit Dashboard")
    ]

    def __init__(self):
        super().__init__()
        self.selected_app_path = None
        self.apps_list = []

    def scan_apps_directory(self):
        apps_root = Path("Apps")
        if not apps_root.exists(): return []
        return sorted([p for p in apps_root.iterdir() if p.is_dir() and (p / "manifest.properties").exists()], key=lambda x: x.name)

    def compose(self) -> ComposeResult:
        with Horizontal():
            with Vertical(id="sidebar"):
                yield Label("📁 APP WORKSPACES", classes="section_title")
                self.list_view = ListView(id="app_selector")
                yield self.list_view
                
                yield Label("\n⚙️ TARGET ARCH", classes="section_title")
                yield Select([("Auto-detect (Manifest)", "auto"), ("ESP32", "esp32"), ("ESP32-C6", "esp32c6"), ("ESP32-P4", "esp32p4"), ("ESP32-S3", "esp32s3"), ("POSIX", "posix-x86_64")], value="auto", id="arch_selector")

                yield Label("\n🛠️ BUILD MODIFIERS", classes="section_title")
                yield Checkbox("Skip compiler steps", id="chk_skip_build")
                yield Checkbox("Verbose log output", id="chk_verbose")

            with Vertical(id="main_content"):
                with Horizontal(id="control_panel"):
                    with Vertical():
                        yield Label("⚡ ACTIVE TASK CONTROLS", classes="section_title")
                        with Horizontal():
                            yield Button("Build", id="btn_build", variant="success")
                            yield Button("Install", id="btn_install", variant="warning")
                            yield Button("Run", id="btn_run", variant="primary")
                            yield Button("Clean Folders", id="btn_clean", variant="error")
                    with Vertical(id="network_settings"):
                        yield Label("🌐 REMOTE TARGET HOST", classes="section_title")
                        yield Input(value=_device_host.read_device_host() or "", placeholder="e.g. 192.168.1.50", id="host_input")
                
                with TabbedContent(id="tabs"):
                    with TabPane("📋 System Output Feed", id="tab_system"): yield Log(id="system_log")
                    with TabPane("🛠️ Active Build Pipeline Monitor", id="tab_pipeline"): yield Log(id="pipeline_log")
        yield Footer()

    def on_mount(self) -> None:
        self.apps_list = self.scan_apps_directory()
        if not self.apps_list: return
        for app_path in self.apps_list:
            self.list_view.append(ListItem(Label(f"🔹 {app_path.name}"), id=f"app_{app_path.name}".replace(" ", "_")))
        self.list_view.index = 0
        self.selected_app_path = self.apps_list[0]

    def on_list_view_selected(self, event: ListView.Selected) -> None:
        idx = self.list_view.index
        if idx is not None and idx < len(self.apps_list):
            self.selected_app_path = self.apps_list[idx]
            self.query_one("#system_log", Log).write_line(f"🎯 Switched app context to: {str(self.selected_app_path)}")

    def on_button_pressed(self, event: Button.Pressed) -> None:
        action_map = {"btn_build": "build", "btn_install": "install", "btn_run": "run", "btn_clean": "clean"}
        a = action_map.get(event.button.id)
        if a: self.execute_task_pipeline(a)

    def action_pipeline(self, action: str) -> None: self.execute_task_pipeline(action)

    def execute_task_pipeline(self, action: str) -> None:
        """Handles safe, non-blocking task execution by separating UI management from compilation workers."""
        pipeline_log = self.query_one("#pipeline_log", Log)
        tabs = self.query_one("#tabs", TabbedContent)

        # app.py falls back to the host of "tt host" when this is empty
        host_ip = self.query_one("#host_input", Input).value.strip()

        # --- SAFE MAIN THREAD UI MANAGEMENT ---
        # Mutating UI properties here on the main event loop avoids the RuntimeError entirely
        tabs.active = "tab_pipeline"
        pipeline_log.clear()
        
        # Capture all layout checkbox states safely before spawning the thread
        skip_build = self.query_one("#chk_skip_build", Checkbox).value
        verbose_mode = self.query_one("#chk_verbose", Checkbox).value
        arch_select = self.query_one("#arch_selector", Select).value

        # Global logging variable reassignment
        global active_ui_logger
        active_ui_logger = pipeline_log

        # --- SPAWN THE THREAD WORKER ---
        # Pass captured primitive values explicitly to isolate the thread worker from UI widgets
        self.run_worker(
            lambda: self._native_thread_executor(action, host_ip, skip_build, verbose_mode, arch_select),
            thread=True
        )

    def _native_thread_executor(self, action: str, host_ip: str, skip_build: bool, verbose_mode: bool, arch_select: str) -> None:
        """Isolated background thread handler running purely synchronous script and compiler operations."""
        # Use safe tui_print wrappers to safely proxy text calls to the log widget across threads
        tui_print(f"⚙️ Running native compiler pipeline operation: '{action.upper()}' on workspace path '{str(self.selected_app_path)}'...")
        
        app_dir = os.path.abspath(str(self.selected_app_path))
        manifest = read_properties_file(os.path.join(app_dir, "manifest.properties"))
        
        if not manifest:
            tui_print("❌ Failed to parse manifest.properties!")
            return

        # Ensure platform un-nesting evaluates strictly to a flat string token
        if arch_select == "auto":
            raw_platforms = manifest.get("target.platforms", "esp32s3").split(",")
            platform = raw_platforms[0].strip() # Default cleanly to the first listed target architecture
        else:
            platform = arch_select

        def run_app_tool(arguments):
            # Run from here with --path, so app.py finds the host of "tt host" in this directory
            command = [sys.executable, APP_TOOL_PATH, action, "--path", app_dir, *arguments]
            if verbose_mode:
                command.append("--verbose")
            return run_native_subprocess(command, os.getcwd())

        host_arguments = ["--host", host_ip] if host_ip else []
        platform_arguments = ["--architecture", platform]
        start_time = time.time()

        if action == "clean":
            success = run_app_tool([])
        elif action == "build":
            success = run_app_tool(platform_arguments + (["--skip-build"] if skip_build else []))
        elif action in ("install", "run"):
            success = run_app_tool(host_arguments)
        elif action in ("bir", "brrr"):
            success = run_app_tool(host_arguments + platform_arguments)
        else:
            success = False

        duration = time.time() - start_time
        
        # Use call_from_thread to safely update the system_log widget from this background thread
        if success:
            tui_print(f"\n✅ Task pipeline finished successfully in {duration:.1f}s!")
            self.call_from_thread(self.query_one("#system_log", Log).write_line, f"✅ Active job completed successfully: '{action}' ({duration:.1f}s)")
        else:
            tui_print("\n❌ Pipeline operation failed or sub-process exited with an error state.")
            reason = f": {last_output_line}" if last_output_line else ""
            self.call_from_thread(self.query_one("#system_log", Log).write_line, f"❌ Active job failed: '{action}'{reason}")

if __name__ == "__main__":
    import _bootstrap
    _bootstrap.run()
    TactilityTUI().run()

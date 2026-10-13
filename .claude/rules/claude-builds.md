# Claude Builds

- Always build ESP32 firmware in `build-claude/` (gitignored), never in the user's `build/`:

```bash
cp sdkconfig <scratchpad>/sdkconfig.copy
idf.py -B build-claude -D SDKCONFIG=<scratchpad>/sdkconfig.copy build
```

- Never modify the `sdkconfig` in the project root, including through `python device.py`, which overwrites it.
  Only do so when the user explicitly asks. That permission is valid for the current session only.

# Releasing Tactility

1. Update `version.txt` in the Tactility project and create a branch to start a build for it
2. Upload the new Tactility SDK to the CDN
    1. Upload it to the [CDN](https://dash.cloudflare.com)
    2. Update `sdk.json` from [TactilityTool](https://github.com/ByteWelder/TactilityTool) and upload it to [CDN](https://dash.cloudflare.com)
3. Update the [TactilityApps](https://github.com/ByteWelder/TactilityApps) with the new SDK and also release these to the CDN:
    1. Download the `cdn-files.zip` from the pipelines
    2. Upload them to the CDN at `apps/x.y.z/`
4. Download the latest firmwares [main branch](https://github.com/ByteWelder/Tactility/actions/workflows/build-firmware.yml)
5. Test the latest version of Tactility on several devices. Pay special attention to:
    1. App Hub
    2. Wi-Fi
6. Prepare a new version of [TactilityWebInstaller](https://github.com/ByteWelder/TactilityWebInstaller) locally:
    1. Copy the GitHub firmwares into `scripts/` in the `TactilityWebInstaller` project
    2. Run `python release-all.py`
    3. Copy the unpacked files to `/rom/(device)/(version)/` and copy in `manifest.json` from existing release
        1. **WARNING** If the partitions have changed, update the json!
    4. Update the version in `manifest.json`
    5. Update `version.json` for the device
7. Test the firmwares on all devices with the local web installer
8. If all went well: release the web installer
9. Test web installer in production (use popular devices)
10. Make a new version of the docs available at [TactilityDocs](https://github.com/ByteWelder/TactilityDocs)
11. Make a new [GitHub release](https://github.com/ByteWelder/Tactility/releases/new)
12. Double-check that all CDN/TactilityApps/Tactility repository changes are merged.

### Post-release

1. Mention on Discord
2. Consider notifying vendors/stakeholders
3. Update SDK updates to CDN at [TactilityTool](https://github.com/ByteWelder/TactilityTool) and upload it to [CDN](https://dash.cloudflare.com)
# Releasing Tactility

1. Test the latest SDK build from GitHub with the CDN as a SNAPSHOT version:
    1. Download it from the [main branch](https://github.com/ByteWelder/Tactility/actions/workflows/build-sdk.yml)
    2. Upload it to the [CDN](https://dash.cloudflare.com)
    3. Update `sdk.json` from [TactilityTool](https://github.com/ByteWelder/TactilityTool) and upload it to [CDN](https://dash.cloudflare.com)
    4. Test it with `ExternalApps/HelloWorld` (clear all its cache and update the SDK version)
2. Build the SDK locally and test it with `ExternalApps/HelloWorld`
3. Download the latest firmwares [main branch](https://github.com/ByteWelder/Tactility/actions/workflows/build-firmware.yml)
4. Test the latest version of Tactility on several devices
5. Prepare a new version of [TactilityWebInstaller](https://github.com/ByteWelder/TactilityWebInstaller) locally:
    1. Copy the GitHub firmwares into `scripts/` in the `TactilityWebInstaller` project
    2. Run `python release-all.py`
    3. Copy the unpacked files to `/rom/(device)/(version)/` and copy in `manifest.json` from existing release
        1. **WARNING** If the partitions have changed, update the json!
    4. Update the version in `manifest.json`
    5. Update `version.json` for the device
6. Test the firmwares on all devices with the local web installer
7. If all went well: release the web installer
8. Test web installer in production (use popular devices)
9. Make a new version of the docs available at [TactilityDocs](https://github.com/ByteWelder/TactilityDocs)
10. Make a new [GitHub release](https://github.com/ByteWelder/Tactility/releases/new)

### Post-release

1. Mention on Discord
2. Consider notifying vendors/stakeholders
3. Remove dev versions in `sdk.json`from [TactilityTool](https://github.com/ByteWelder/TactilityTool) and upload it to [CDN](https://dash.cloudflare.com)
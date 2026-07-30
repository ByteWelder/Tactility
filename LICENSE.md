# License Grant

## Definitions

"external apps" or "external applications" refers to applications that are built with the TactilitySDK.
These applications are not part of the Tactility operating system's main firmware.

"end-users" refers to people who install and/or use Tactility software on their devices.

"subproject" refers to any project or dependency within the Tactility project.

## Intentions

The intentions behind picking the licenses for the subprojects:

1. Forks of the entire Tactility project are forced to be open source (GPL v3.0 applies).
2. It should be possible to make closed source external applications using TactilitySDK and the libraries it includes (it must exclude software with a GPL license).
3. It should be possible to make closed source forks that only contain TactilityKernel, the platform implementations (`Platforms/*`) and most of the device and driver implementations. As few as possible driver and device combinations should prevent this.

## Summary

**IMPORTANT:** Make sure you double-check the license(s) of each subproject if you intend to make a derived project that is not offered with a GPL license.

**IMPORTANT:** This document may have in accuracies. It mainly exists to create awareness about license differences.

The top-level projects `Firmware/` and `Tactility/` are licensed under [GPL v3.0](Documentation/LICENSE-GPL-3.0.md).

Most drivers have an [Apache License v2.0](Documentation/LICENSE-Apache-2.0.md), but exceptions such as `Drivers/gps-meshtastic-module/` exist.
Licensing may also differ for subprojects intended for use in external applications.

All projects under `Modules/` have an Apache license.

## Overview

| Project            | License                 |
|--------------------|-------------------------|
| Tactility          | GNU Public License v3.0 |
| TactilityC         | Apache License v2.0     |
| TactilityFreeRTOS  | Apache License v2.0     |
| TactilityKernel    | Apache License v2.0     |
| Tests              | (varies)                |
| Devices/*          | GNU Public License v3.0 |
| Drivers/*          | (varies)                |
| Modules/*          | Apache License v2.0     |
| DevicetreeCompiler | Apache License v2.0     |
| Platforms/*        | Apache License v2.0     |

Subprojects and directories in this project can contain license files.

The presence of such a license file indicates that this license applies to all the files and folders that are contained by the folder at the level where the license file resides.

## Logo

The Tactility logo copyrights are owned by Ken Van Hoeylandt.

Logo usage is permitted in these scenarios:
- News, blog posts, articles and documentation that write about the official Tactility project.
- Firmwares built with unmodified source code from [the official repository](https://github.com/TactilityProject/Tactility) can be redistributed with the Tactility logo.
- Personal use for local builds that contain Tactility source code (original or modified), and aren't re-distributed online.

Logo usage is forbidden in all other scenarios unless an exception was granted by the author.
For other usages or exceptions, [contact me](https://kenvanhoeylandt.net).

Practical examples:
- A blog post about Tactility can use screenshots and the logo itself when writing about Tactility
- A developer who forked Tactility to merge new features or fixes to the official Tactility repository is allowed to make builds with the logo, as long as these builds are not re-distributed to end-users.

## Third Party Notices

Third-party licenses and copyrights are listed in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

## FAQ

- Q: Can I build closed source applications?
- A: If the applications were built with the Tactility SDK, then they can have a proprietary license. Applications inside the Tactility firmware can't be released with a proprietary license.

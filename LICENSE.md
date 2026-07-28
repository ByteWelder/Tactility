# License Grant

## Definitions

"external apps" or "external applications" refers to applications that are built with the TactilitySDK.
These applications are not part of the Tactility operating system's main firmware.

"end-users" refers to people who install and/or use Tactility software on their devices.

"subproject" refers to a child project of the parent Tactility project.

## License texts

The license texts that are relevant to this document:

- [Apache License v2.0](Documentation/LICENSE-Apache-2.0.md).
- [GPL v3.0](Documentation/LICENSE-GPL-3.0.md).

## Summary

The main firmware projects (`Firmware/`, `Tactility/`) are licensed under `GPL v3.0`.

Most devices are licensed with `Apache License v2.0` while some are implemented as `GPL v3.0` until author consent is given to change the license.
New device implementations should be licensed under `Apache License v2.0`.

Most drivers have an `Apache License v2.0`, with exceptions such as `Drivers/gps-generic-module/`.
Licensing may also differ for subprojects intended for use in external applications.

Driver subprojects aren't generally used directly in external app projects, but if they are, make sure to check their licenses.

All projects under `Modules/` have an `Apache License v2.0`.

## GPL v3.0 to Apache License v2.0

Some code has changed license from GPL to Apache due to one or more of:

### 1. Consent of Authors

Consent of all authors involved in a specific subproject.
This consent is confirmed in writing.

### 2. Rewriting the code entirely

Some projects were rewritten entirely. Some examples:

- Drivers that were written for the `Tactility/` subproject (C++ interface, GPL) and were rewritten from scratch based on `TactilityKernel/` (C interface, Apache)
- Device subprojects were rewritten from a purely code-focused configuration project to an empty module declaration (can't really copyright this) and a DTS file.

## Device project considerations

Some device projects are provided with an Apache license, but might refer to subprojects containing GPL code.
These projects are required to be licensed with GPL as soon as they are compiled.

Their non-binary form is their non-combined form (they don't include GPL code yet), so the project retains its Apache license until it is compiled.
This allows for derivates that cut out GPL dependencies.

For example: `LilyGO T-Deck Plus` and `LilyGO T-Lora Pager`:

The projects themselves have a `module.cpp` and a `.dts` file. As long as the code is not compiled with the `gps-generic-module` drivers,
the project is not forced into a GPS license. This allows someone to copy the subproject's files into a closed-source project, remove the GPS driver from the DTS file and the `devicetree.yaml`,
and then use that in a project that is compatible with the Apache license.

In other words: If you intend to use parts Tactility in a closed-source application, make sure you check the license of the device project
and all the driver projects that it depends on.

## Overview

Below is an overview of the licenses of some of the subprojects.

| Project            | License                 |
|--------------------|-------------------------|
| Tactility          | GNU Public License v3.0 |
| TactilityC         | Apache License v2.0     |
| TactilityFreeRTOS  | Apache License v2.0     |
| TactilityKernel    | Apache License v2.0     |
| Tests              | (varies)                |
| Devices/*          | (varies)                |
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

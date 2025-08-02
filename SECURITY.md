# Security Policy

## Supported Versions

| Version     | Supported          |
|-------------|--------------------|
| main branch | :white_check_mark: |

## Important project context

Tactility is foremost a tinkering platform as opposed to a user platform.  It does not have desktop OS security features
such as user/access management, file system protections, memory protections, app permissions and more.

[ESP Privilege Separation](https://github.com/espressif/esp-privilege-separation) is not implemented nor planned to be implemented.
It is limited to C3 and S3 hardware, so it wouldn't even work on the original ESP32.

Keep this in mind when reporting vulnerabilities.

## Reporting a Vulnerability

We appreciate your efforts to responsibly disclose your findings, and we will make every effort to acknowledge your contributions.

To report a security issue, please use the GitHub Security Advisory ["Report a Vulnerability"](https://github.com/bytewelder/tactility/security/advisories/new) tab.

You can expect a response indicating the next steps in handling your report. After the initial reply to your report,
you'll be kept informed of the progress towards a fix and full announcement, and may ask for additional information or guidance.

Report security bugs in third-party dependencies to the person or team maintaining the module.

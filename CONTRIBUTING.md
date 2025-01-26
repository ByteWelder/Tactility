# Contributing

## Accepted changes

Before releasing version 1.0.0, the APIs are changing rapidly.
I want to minimize the amount of changes that I have to maintain, because it limits the amount of breaking changes that I have to deal with when the APIs change.

### New features

These are currently not accepted.

### Visual / interaction design changes

These are currently not accepted. I'm aware that there is a lot of room for improvement, but I want to mainly focusing on strong technical foundations before
I start building further on top of that.
Feel free to open an [issue](https://github.com/ByteWelder/Tactility/issues/new) to discuss ideas you have, if you feel like they will have a considerable impact.

### Fixing things

The general take here is that minor changes are not accepted at this stage. I don't want to spend my time reviewing and discussing these during the current stage of the project.

Only fixes for serious issues are accepted, like:
- Bugs
- Crashes
- Security issues

Some examples of non-serious issues include:
- Documentation changes
- Renaming code
- Fixes for compiler warnings that have low impact on the actual application logic (e.g. only happens on simulator when calling a logging function)

### New board implementations

I only support boards that I also own. If I don't own a board, I can't properly test the reliability of the firmware.
If you wish to send me a board, please [contact me](https://tactility.one/#/support).

Please open an [issue](https://github.com/ByteWelder/Tactility/issues/new) on GitHub to discuss new boards.

If you implemented a board yourself, I'm willing to refer to your implementation on the main website. Please [contact me](https://tactility.one/#/support).

### Anything that doesn't fall in the above categories?

Please [contact me](https://tactility.one/#/support) me first!

## Pull Requests

Pull requests should only contain a single set of changes that are related to eachother.
That way, an approved set of changes will not be blocked by an unapproved set of changes.

## Code Style

See [this document](CODING_STYLE.md) and [.clang-format](.clang-format).

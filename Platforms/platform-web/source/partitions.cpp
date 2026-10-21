// SPDX-License-Identifier: Apache-2.0
// Emscripten provides a POSIX-like in-memory filesystem (plus IDBFS when mounted by
// the frontend), so the platform-posix directory-backed partition registration works
// unchanged: "data" resolves against the process cwd, which serve.py sets up.
#include "../../platform-posix/source/partitions.cpp"

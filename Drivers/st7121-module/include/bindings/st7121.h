// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/bindings/bindings.h>
#include <drivers/st7121.h>

// The devicetree compiler derives the expected config typedef name from the compatible
// string's suffix (e.g. "sitronix,st7121" -> st7121_config_dt), not from the node name or
// driver name, so the tag here must match that exactly.
DEFINE_DEVICETREE(st7121, struct St7121Config)

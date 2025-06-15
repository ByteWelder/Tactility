#pragma once

#include <Tactility/Bundle.h>

namespace tt::app::fileselection {

enum class Mode {
    Existing = 0,
    ExistingOrNew = 1
};

Mode getMode(const Bundle& bundle);

}

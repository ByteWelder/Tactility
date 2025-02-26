#pragma once

#include "./View.h"

namespace tt::app::serialconsole {

class ConsoleView final : public View {
public:

    void onStart(lv_obj_t* parent) final {
    }

    void onStop() final {
    }
};

} // namespace tt::app::serialconsole

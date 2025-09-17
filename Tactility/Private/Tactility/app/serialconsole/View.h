#pragma once

namespace tt::app::serialconsole {

class View {
public:
    virtual void onStop() = 0;
};

}
#include "Tactility/service/wifi/Wifi.h"

namespace tt::service::wifi {

const char* radioStateToString(RadioState state) {
    switch (state) {
        using enum RadioState;
        case OnPending:
            return TT_STRINGIFY(OnPending);
        case On:
            return TT_STRINGIFY(On);
        case ConnectionPending:
            return TT_STRINGIFY(ConnectionPending);
        case ConnectionActive:
            return TT_STRINGIFY(ConnectionActive);
        case OffPending:
            return TT_STRINGIFY(OnPending);
        case Off:
            return TT_STRINGIFY(Off);
    }
    tt_crash("not implemented");
}

}

#include "./Wifi.h"

namespace tt::service::wifi {

const char* radioStateToString(RadioState state) {
    switch (state) {
        case RadioState::OnPending:
            return TT_STRINGIFY(RadioState::OnPending);
        case RadioState::On:
            return TT_STRINGIFY(RadioState::On);
        case RadioState::ConnectionPending:
            return TT_STRINGIFY(RadioState::ConnectionPending);
        case RadioState::ConnectionActive:
            return TT_STRINGIFY(RadioState::ConnectionActive);
        case RadioState::OffPending:
            return TT_STRINGIFY(RadioState::OnPending);
        case RadioState::Off:
            return TT_STRINGIFY(RadioState::Off);
    }
    tt_crash("not implemented");
}

}

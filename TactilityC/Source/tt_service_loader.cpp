#include "tt_service_loader.h"
#include <Bundle.h>
#include <service/loader/Loader.h>

extern "C" {

void tt_service_loader_start_app(const char* id, bool blocking, BundleHandle _Nullable bundle) {
    auto shared_bundle = std::shared_ptr<tt::Bundle>((tt::Bundle*)bundle);
    tt::service::loader::startApp(id, blocking, std::move(shared_bundle));
}

void tt_service_loader_stop_app() {
    tt::service::loader::stopApp();
}

AppContextHandle tt_service_loader_get_current_app() {
    return tt::service::loader::getCurrentApp();
}

}

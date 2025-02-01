#include "tt_service_loader.h"
#include <Tactility/Bundle.h>
#include <Tactility/service/loader/Loader.h>

extern "C" {

void tt_service_loader_start_app(const char* id, BundleHandle _Nullable bundle) {
    auto shared_bundle = std::shared_ptr<tt::Bundle>((tt::Bundle*)bundle);
    tt::service::loader::startApp(id, std::move(shared_bundle));
}

void tt_service_loader_stop_app() {
    tt::service::loader::stopApp();
}

AppHandle tt_service_loader_get_current_app() {
    return tt::service::loader::getCurrentAppContext().get();
}

}

#include "App.h"
#include "service/loader/Loader.h"

namespace tt::app {

void start(const std::string& id, std::shared_ptr<const Bundle> _Nullable parameters) {
    service::loader::startApp(id, std::move(parameters));
}

void stop() {
    service::loader::stopApp();
}

std::shared_ptr<app::AppContext> _Nullable getCurrentAppContext() {
    return service::loader::getCurrentAppContext();
}

std::shared_ptr<app::App> _Nullable getCurrentApp() {
    return service::loader::getCurrentApp();
}

}

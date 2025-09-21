#include <Tactility/app/App.h>
#include <Tactility/service/loader/Loader.h>

namespace tt::app {

LaunchId start(const std::string& id, std::shared_ptr<const Bundle> _Nullable parameters) {
    return service::loader::startApp(id, std::move(parameters));
}

void stop() {
    service::loader::stopApp();
}

std::shared_ptr<AppContext> _Nullable getCurrentAppContext() {
    return service::loader::getCurrentAppContext();
}

std::shared_ptr<App> _Nullable getCurrentApp() {
    return service::loader::getCurrentApp();
}

}

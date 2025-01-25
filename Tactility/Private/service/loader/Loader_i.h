#pragma once

#include "EventFlag.h"
#include "MessageQueue.h"
#include "PubSub.h"
#include "RtosCompatSemaphore.h"
#include "Thread.h"
#include "app/AppInstance.h"
#include "app/AppManifest.h"
#include "service/loader/Loader.h"
#include <DispatcherThread.h>
#include <stack>
#include <utility>

namespace tt::service::loader {

// region LoaderMessage

class LoaderMessageAppStart {
public:
    std::string id;
    std::shared_ptr<const Bundle> _Nullable parameters;

    LoaderMessageAppStart() = default;

    LoaderMessageAppStart(LoaderMessageAppStart& other) :
        id(other.id),
        parameters(other.parameters) {}

    LoaderMessageAppStart(const std::string& id, std::shared_ptr<const Bundle> parameters) :
        id(id),
        parameters(std::move(parameters))
    {}

    ~LoaderMessageAppStart() = default;
};

// endregion LoaderMessage

struct Loader {
    std::shared_ptr<PubSub> pubsubExternal = std::make_shared<PubSub>();
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::stack<std::shared_ptr<app::AppInstance>> appStack;
    /** The dispatcher thread needs a callstack large enough to accommodate all the dispatched methods.
     * This includes full LVGL redraw via Gui::redraw()
     */
    std::unique_ptr<DispatcherThread> dispatcherThread = std::make_unique<DispatcherThread>("loader_dispatcher", 6144); // Files app requires ~5k
};

} // namespace

#pragma once

#include "app/AppManifest.h"
#include "app/AppInstance.h"
#include "EventFlag.h"
#include "MessageQueue.h"
#include "Pubsub.h"
#include "Thread.h"
#include "service/loader/Loader.h"
#include "RtosCompatSemaphore.h"
#include <stack>
#include <utility>
#include <DispatcherThread.h>

namespace tt::service::loader {


// region LoaderEvent

typedef enum {
    LoaderEventTypeApplicationStarted,
    LoaderEventTypeApplicationShowing,
    LoaderEventTypeApplicationHiding,
    LoaderEventTypeApplicationStopped
} LoaderEventType;

struct LoaderEvent {
    LoaderEventType type;
};

// endregion LoaderEvent

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

#pragma once

#include "Tactility/app/AppInstance.h"
#include "Tactility/app/AppManifest.h"
#include "Tactility/service/loader/Loader.h"

#include <Tactility/DispatcherThread.h>
#include <Tactility/EventFlag.h>
#include <Tactility/MessageQueue.h>
#include <Tactility/PubSub.h>
#include <Tactility/RtosCompatSemaphore.h>
#include <Tactility/Thread.h>

#include <stack>
#include <utility>

namespace tt::service::loader {

// region LoaderMessage

class LoaderMessageAppStart {
public:
    std::string id;
    std::shared_ptr<const Bundle> _Nullable parameters;

    LoaderMessageAppStart() = default;
    LoaderMessageAppStart(LoaderMessageAppStart& other) = default;

    LoaderMessageAppStart(const std::string& id, std::shared_ptr<const Bundle> parameters) :
        id(id),
        parameters(std::move(parameters))
    {}

    ~LoaderMessageAppStart() = default;
};

class LoaderMessageAppStop {
public:
    std::string id;

    LoaderMessageAppStop() = default;
    LoaderMessageAppStop(LoaderMessageAppStop& other) = default;

    LoaderMessageAppStop(const std::string& id) : id(id) {}

    ~LoaderMessageAppStop() = default;
};

// endregion LoaderMessage

struct Loader {

};

} // namespace

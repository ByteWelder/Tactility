#include <Tactility/CpuAffinity.h>
#include <Tactility/MainDispatcher.h>
#include <Tactility/TactilityPrivate.h>

#include <tactility/Memory.hpp>

#include <tactility/check.h>
#include <tactility/log.h>

// Implements the deprecated MainDispatcher API
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace tt {

constexpr auto* TAG = "MainDispatcher";

// Internal RAM, so that the functions can access flash
constexpr size_t TASK_STACK_SIZE = 8192;

struct MainDispatcher::Node {
    Function function;
    Node* next = nullptr;
};

MainDispatcher::MainDispatcher() {
    mutex_construct(&mutex);
}

MainDispatcher::Node* MainDispatcher::popOrStop() {
    mutex_lock(&mutex);
    Node* node = head;
    if (node != nullptr) {
        head = node->next;
        if (head == nullptr) {
            tail = nullptr;
        }
    } else {
        running = false;
    }
    mutex_unlock(&mutex);
    return node;
}

int32_t MainDispatcher::taskMain(void* context) {
    auto* dispatcher = static_cast<MainDispatcher*>(context);
    while (Node* node = dispatcher->popOrStop()) {
        node->function();
        deleteObject(node);
    }
    return 0;
}

bool MainDispatcher::startTask() {
    if (stale != nullptr) {
        thread_join(stale, portMAX_DELAY, 1);
        thread_free(stale);
        stale = nullptr;
    }

    ::Thread* thread = thread_alloc_full("main_dispatcher", TASK_STACK_SIZE, taskMain, this, getCpuAffinityConfiguration().mainDispatcher);
    if (thread == nullptr) {
        return false;
    }
    running = true;
    if (thread_start(thread) != ERROR_NONE) {
        running = false;
        thread_free(thread);
        return false;
    }
    stale = thread;
    return true;
}

bool MainDispatcher::dispatch(Function function) {
    auto* node = newObject<Node>();
    if (node == nullptr) {
        return false;
    }
    node->function = std::move(function);

    mutex_lock(&mutex);
    if (tail != nullptr) {
        tail->next = node;
    } else {
        head = node;
    }
    tail = node;

    if (!running && !startTask()) {
        // The queue was empty before this node, because no task was running
        head = nullptr;
        tail = nullptr;
        mutex_unlock(&mutex);
        LOG_E(TAG, "Failed to start the task");
        deleteObject(node);
        return false;
    }
    mutex_unlock(&mutex);
    return true;
}

MainDispatcher& getMainDispatcher() {
    // Allocated once and never freed
    static MainDispatcher* instance = newObject<MainDispatcher>();
    check(instance != nullptr);
    return *instance;
}

bool dispatchOnMainDispatcher(void (*function)(void* context), void* context) {
    return getMainDispatcher().dispatch([function, context] { function(context); });
}

} // namespace

#pragma GCC diagnostic pop

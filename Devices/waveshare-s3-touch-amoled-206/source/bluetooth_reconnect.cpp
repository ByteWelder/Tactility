// SPDX-License-Identifier: Apache-2.0

// Board-local Bluetooth auto-reconnect for the Waveshare S3 Touch AMOLED 2.06.
//
// Tactility's own auto-connect runs on the "bt-events" thread
// (Tactility/Source/bluetooth/Bluetooth.cpp). That thread can be blocked while the radio is on,
// so saved peers (e.g. a BLE keyboard) never get reconnected. This task subscribes to the kernel
// BT driver directly - its own subscription and its own thread, so it doesn't depend on bt-events
// - and, whenever the radio is on, scans for and reconnects every saved peer it sees.

#include <sdkconfig.h>

#if defined(CONFIG_BT_NIMBLE_ENABLED)

#include <Tactility/Thread.h>
#include <Tactility/bluetooth/Bluetooth.h>
#include <Tactility/bluetooth/BluetoothPairedDevice.h>

#include <tactility/concurrent/task_event_group.h>
#include <tactility/device.h>
#include <tactility/drivers/bluetooth.h>
#include <tactility/error.h>
#include <tactility/log.h>

#include <array>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

namespace tt::bluetooth {

// The core HID host resolves a peer's address type from a scan cache that is filled in by the
// (blocked) bt-events thread. The helper that fills it is internal to Tactility (declared in
// Private/Tactility/bluetooth/BluetoothPrivate.h, which isn't exported to components), so it is
// re-declared here to prime that cache with the address type this task learns from its own scan.
void cacheScanAddr(const uint8_t addr[6], uint8_t addr_type);

} // namespace tt::bluetooth

namespace {

constexpr auto* TAG = "WsBtReconnect";

// How long the task blocks per wait, and how many of those slices make up one periodic retry.
// The slice bounds how quickly stop() is noticed; the total paces the retry scan.
constexpr TickType_t WAIT_SLICE = pdMS_TO_TICKS(500);
constexpr int RETRY_SLICES = 40; // ~20 s
// This task runs the same heavy work the core dispatches to the main task (settings::loadAll()
// plus tt::bluetooth::connect() -> hidHostConnect()), so it needs at least the main task's stack
// (CONFIG_ESP_MAIN_TASK_STACK_SIZE, 6144). Sized to match NimBLE's host task.
constexpr size_t TASK_STACK_SIZE = 8192;

// The full CONFIG_TT_DEVICE_NAME ("Waveshare S3 Touch AMOLED 2.06\"", 30 chars) doesn't fit the
// 31-byte legacy advertising payload, so the board advertises this shorter name instead. The full
// name is still shown in the UI. Must stay <= 26 chars to fit alongside the advertising flags.
constexpr const char* ADVERTISED_NAME = "S3 Touch AMOLED";

std::atomic<bool> reconnect_running { false };
std::unique_ptr<tt::Thread> reconnect_thread;

struct SavedPeer {
    std::array<uint8_t, 6> addr;
    int profile_id;
    bool attempted;
};

// Only ever touched by the task.
std::vector<SavedPeer> saved_peers;
Device* bt_device = nullptr;

void refresh_saved_peers() {
    saved_peers.clear();
    for (const auto& peer : tt::bluetooth::settings::loadAll()) {
        saved_peers.push_back(SavedPeer { .addr = peer.addr, .profile_id = peer.profileId, .attempted = false });
    }
    static int last_count = -1;
    if (static_cast<int>(saved_peers.size()) != last_count) {
        last_count = static_cast<int>(saved_peers.size());
        LOG_I(TAG, "%d saved peer(s)", last_count);
        for (const auto& peer : saved_peers) {
            LOG_I(TAG, "  saved %s profile=%d", tt::bluetooth::settings::addrToHex(peer.addr).c_str(), peer.profile_id);
        }
    }
}

SavedPeer* find_saved_peer(const BtAddr addr) {
    for (auto& peer : saved_peers) {
        if (std::memcmp(peer.addr.data(), addr, 6) == 0) {
            return &peer;
        }
    }
    return nullptr;
}

void connect_peer(const BtPeerRecord& record) {
    SavedPeer* peer = find_saved_peer(record.addr);
    if (peer == nullptr || peer->attempted) {
        return;
    }
    peer->attempted = true;

    // Prime the core HID host cache with the address type from our own scan.
    tt::bluetooth::cacheScanAddr(record.addr, record.addr_type);

    const auto hex = tt::bluetooth::settings::addrToHex(peer->addr);
    LOG_I(TAG, "Reconnecting saved peer %s (profile=%d)", hex.c_str(), peer->profile_id);
    tt::bluetooth::connect(peer->addr, peer->profile_id);
}

void scan_if_idle() {
    if (bt_device == nullptr || saved_peers.empty()) {
        return;
    }
    if (!tt::bluetooth::isRadioOnOrPending(bt_device)) {
        return;
    }
    if (bluetooth_is_scanning(bt_device)) {
        return;
    }
    for (auto& peer : saved_peers) {
        peer.attempted = false;
    }
    LOG_I(TAG, "Scanning for %d saved peer(s)", static_cast<int>(saved_peers.size()));
    bluetooth_scan_start(bt_device);
}

int32_t task_main() {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    LOG_I(TAG, "Task started");

    // Wait for the Bluetooth device to be constructed and started (the module starts before the
    // devicetree's devices do).
    while (reconnect_running.load()) {
        if (device_get_first_active_by_type(&BLUETOOTH_TYPE, &bt_device) == ERROR_NONE) {
            break;
        }
        bt_device = nullptr;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    BtEventSubscription subscription {};
    if (!reconnect_running.load() || bt_device == nullptr ||
        bluetooth_event_subscribe(bt_device, &subscription, &event_group) != ERROR_NONE) {
        LOG_W(TAG, "Failed to acquire/subscribe the Bluetooth device");
        if (bt_device != nullptr) {
            device_put(bt_device);
            bt_device = nullptr;
        }
        task_event_group_destruct(&event_group);
        return 0;
    }

    LOG_I(TAG, "Subscribed to Bluetooth events");

    // Advertise a name that fits the legacy advertising payload (see ADVERTISED_NAME). The driver
    // restarts advertising with it if the radio is already on.
    if (bluetooth_set_device_name(bt_device, ADVERTISED_NAME) != ERROR_NONE) {
        LOG_W(TAG, "Failed to set the advertised Bluetooth name");
    }

    refresh_saved_peers();
    scan_if_idle();

    int slices = 0;
    while (reconnect_running.load()) {
        task_event_group_wait_any(&event_group, nullptr, WAIT_SLICE);

        BtEvent event {};
        while (bluetooth_event_poll(&subscription, &event) == ERROR_NONE) {
            switch (event.type) {
                case BT_EVENT_RADIO_STATE_CHANGED:
                    LOG_I(TAG, "Radio state -> %d", static_cast<int>(event.radio_state));
                    if (event.radio_state == BT_RADIO_STATE_ON) {
                        refresh_saved_peers();
                        scan_if_idle();
                    }
                    break;
                case BT_EVENT_SCAN_FINISHED:
                    LOG_I(TAG, "Scan finished");
                    break;
                case BT_EVENT_PROFILE_STATE_CHANGED:
                    LOG_I(TAG, "Profile %d state -> %d",
                              static_cast<int>(event.profile_state.profile),
                              static_cast<int>(event.profile_state.state));
                    // A saved peer dropped (e.g. the keyboard was powered off): rescan now instead
                    // of waiting for the next periodic tick.
                    if (event.profile_state.state == BT_PROFILE_STATE_IDLE) {
                        refresh_saved_peers();
                        scan_if_idle();
                    }
                    break;
                case BT_EVENT_PEER_FOUND: {
                    SavedPeer* match = find_saved_peer(event.peer.addr);
                    if (event.peer.name[0] != '\0' || match != nullptr) {
                        std::array<uint8_t, 6> peer_addr {};
                        std::memcpy(peer_addr.data(), event.peer.addr, 6);
                        LOG_I(TAG, "Peer %s '%s' type=%d saved=%d",
                                  tt::bluetooth::settings::addrToHex(peer_addr).c_str(),
                                  event.peer.name,
                                  static_cast<int>(event.peer.addr_type),
                                  match != nullptr ? 1 : 0);
                    }
                    connect_peer(event.peer);
                    break;
                }
                default:
                    break;
            }
        }

        // Periodic retry: pick up newly paired peers and rescan when nothing connected yet.
        if (++slices >= RETRY_SLICES) {
            slices = 0;
            refresh_saved_peers();
            LOG_I(TAG, "Tick: radioOn=%d scanning=%d saved=%d",
                      static_cast<int>(tt::bluetooth::isRadioOnOrPending(bt_device)),
                      static_cast<int>(bluetooth_is_scanning(bt_device)),
                      static_cast<int>(saved_peers.size()));
            scan_if_idle();
        }
    }

    bluetooth_event_unsubscribe(bt_device, &subscription);
    device_put(bt_device);
    bt_device = nullptr;
    task_event_group_destruct(&event_group);
    return 0;
}

void task_start() {
    reconnect_thread = std::make_unique<tt::Thread>(
        "ws-bt-reconnect",
        TASK_STACK_SIZE,
        []() -> int32_t { return task_main(); }
    );
    reconnect_thread->start();
}

} // namespace

void waveshare_bluetooth_reconnect_start() {
    if (reconnect_running.exchange(true)) {
        return;
    }
    task_start();
}

void waveshare_bluetooth_reconnect_stop() {
    if (!reconnect_running.exchange(false)) {
        return;
    }
    if (reconnect_thread != nullptr) {
        reconnect_thread->join();
        reconnect_thread.reset();
    }
}

#else

void waveshare_bluetooth_reconnect_start() {}
void waveshare_bluetooth_reconnect_stop() {}

#endif // CONFIG_BT_NIMBLE_ENABLED

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if defined(CONFIG_BT_NIMBLE_ENABLED)

#include <Tactility/bluetooth/Bluetooth.h>
#include <Tactility/bluetooth/BluetoothPairedDevice.h>
#include <Tactility/bluetooth/BluetoothSettings.h>
#include <Tactility/bluetooth/BluetoothPrivate.h>

#include <Tactility/Mutex.h>
#include <Tactility/Tactility.h>
#include <Tactility/Thread.h>
#include <tactility/check.h>
#include <tactility/device.h>
#include <tactility/drivers/bluetooth.h>
#include <tactility/drivers/bluetooth_hid_device.h>
#include <tactility/drivers/bluetooth_midi.h>
#include <tactility/drivers/bluetooth_serial.h>
#include <tactility/log.h>

#include <array>
#include <atomic>
#include <cstring>
#include <vector>

namespace tt::bluetooth {

constexpr auto* TAG = "Bluetooth";

// ---- Scan result cache (C++ PeerRecord list, updated from BT_EVENT_PEER_FOUND) ----

static Mutex scan_cache_mutex;
static std::vector<PeerRecord> scan_results_cache;

struct CachedAddr {
    uint8_t addr[6];
    uint8_t addr_type;
};
static std::vector<CachedAddr> scan_addr_cache; // parallel to scan_results_cache

// ---- Device accessor ----

Device* findFirstRegisteredDevice() {
    Device* found = nullptr;
    device_for_each_of_type(&BLUETOOTH_TYPE, &found, [](Device* dev, void* ctx) -> bool {
        *static_cast<Device**>(ctx) = dev;
        return true;
    });
    return found;
}


// ---- Scan cache helpers ----

void cacheScanAddr(const uint8_t addr[6], uint8_t addr_type) {
    auto lock = scan_cache_mutex.asScopedLock();
    lock.lock();
    for (auto& entry : scan_addr_cache) {
        if (memcmp(entry.addr, addr, 6) == 0) {
            entry.addr_type = addr_type;
            return;
        }
    }
    CachedAddr e = {};
    memcpy(e.addr, addr, 6);
    e.addr_type = addr_type;
    scan_addr_cache.push_back(e);
}

bool getCachedScanAddrType(const uint8_t addr[6], uint8_t* addr_type_out) {
    auto lock = scan_cache_mutex.asScopedLock();
    lock.lock();
    for (const auto& entry : scan_addr_cache) {
        if (memcmp(entry.addr, addr, 6) == 0) {
            if (addr_type_out) *addr_type_out = entry.addr_type;
            return true;
        }
    }
    if (addr_type_out) *addr_type_out = 0;
    return false;
}

static void cachePeerRecord(const BtPeerRecord& krecord) {
    PeerRecord rec;
    memcpy(rec.addr.data(), krecord.addr, 6);
    rec.name      = krecord.name[0] != '\0' ? krecord.name : "";
    rec.rssi      = krecord.rssi;
    rec.paired    = krecord.paired;
    rec.connected = krecord.connected;
    rec.profileId = 0;

    cacheScanAddr(krecord.addr, krecord.addr_type);

    auto lock = scan_cache_mutex.asScopedLock();
    lock.lock();
    for (auto& existing : scan_results_cache) {
        if (existing.addr == rec.addr) {
            if (!rec.name.empty()) existing.name = rec.name;
            existing.rssi = rec.rssi;
            return;
        }
    }
    scan_results_cache.push_back(std::move(rec));
}

// ---- Bridge thread (subscribed to the kernel driver) ----
// This thread listens to platform driver events to perform auto-start logic and settings
// management. Consumers should subscribe directly via bluetooth_event_subscribe() to receive
// events themselves.

TaskEventGroup btEventGroup {};
BtEventSubscription btEventSub {};
Thread* btEventThread = nullptr;
std::atomic<bool> btEventThreadRunning {false};

static void bt_event_bridge(BtEvent event) {
    switch (event.type) {
        case BT_EVENT_RADIO_STATE_CHANGED:
            switch (event.radio_state) {
                case BT_RADIO_STATE_ON:
                    getMainDispatcher().dispatch([] {
                        auto peers = settings::loadAll();
                        bool has_hid_host_auto   = false;
                        bool has_hid_device_auto = false;
                        for (const auto& p : peers) {
                            if (!p.autoConnect) continue;
                            if (p.profileId == BT_PROFILE_HID_HOST)   has_hid_host_auto   = true;
                            if (p.profileId == BT_PROFILE_HID_DEVICE) has_hid_device_auto = true;
                        }
                        if (has_hid_host_auto) {
                            LOG_I(TAG, "HID host auto-connect peer found — starting scan");
                            Device* dev;
                            if (device_get_first_active_by_type(&BLUETOOTH_TYPE, &dev) == ERROR_NONE) {
                                bluetooth_scan_start(dev);
                                device_put(dev);
                            }
                        } else if (has_hid_device_auto) {
                            LOG_I(TAG, "HID device auto-start (bonded peer found)");
                            if (Device* dev = bluetooth_hid_device_get()) {
                                bluetooth_hid_device_start(dev, BT_HID_DEVICE_MODE_KEYBOARD);
                                device_put(dev);
                            }
                        } else {
                            if (settings::shouldSppAutoStart()) {
                                LOG_I(TAG, "Auto-starting SPP server");
                                if (Device* dev = bluetooth_serial_get()) {
                                    bluetooth_serial_start(dev);
                                    device_put(dev);
                                }
                            }
                            if (settings::shouldMidiAutoStart()) {
                                LOG_I(TAG, "Auto-starting MIDI server");
                                if (Device* dev = bluetooth_midi_get()) {
                                    bluetooth_midi_start(dev);
                                    device_put(dev);
                                }
                            }
                        }
                    });
                    break;
                default:
                    break;
            }
            break;

        case BT_EVENT_SCAN_STARTED:
            {
                auto lock = scan_cache_mutex.asScopedLock();
                lock.lock();
                scan_results_cache.clear();
                scan_addr_cache.clear();
            }
            break;

        case BT_EVENT_SCAN_FINISHED:
            getMainDispatcher().dispatch([] { autoConnectHidHost(); });
            break;

        case BT_EVENT_PEER_FOUND:
            cachePeerRecord(event.peer);
            break;

        case BT_EVENT_PAIR_RESULT:
            if (event.pair_result.result == BT_PAIR_RESULT_SUCCESS) {
                uint8_t addr_buf[6];
                int profile_copy = event.pair_result.profile;
                memcpy(addr_buf, event.pair_result.addr, 6);
                getMainDispatcher().dispatch([addr_buf, profile_copy]() mutable {
                    std::array<uint8_t, 6> peer_addr;
                    memcpy(peer_addr.data(), addr_buf, 6);
                    const auto hex = settings::addrToHex(peer_addr);
                    if (!settings::hasFileForDevice(hex)) {
                        settings::PairedDevice dev;
                        dev.addr        = peer_addr;
                        dev.name        = "";
                        dev.autoConnect = true;
                        dev.profileId   = profile_copy;
                        if (settings::save(dev)) {
                            LOG_I(TAG, "Saved paired peer %s (profile=%d)", hex.c_str(), profile_copy);
                        }
                    }
                });
            } else if (event.pair_result.result == BT_PAIR_RESULT_BOND_LOST) {
                uint8_t addr_buf[6];
                memcpy(addr_buf, event.pair_result.addr, 6);
                getMainDispatcher().dispatch([addr_buf]() mutable {
                    std::array<uint8_t, 6> peer_addr;
                    memcpy(peer_addr.data(), addr_buf, 6);
                    settings::remove(settings::addrToHex(peer_addr));
                });
            }
            break;

        case BT_EVENT_PROFILE_STATE_CHANGED:
            if (event.profile_state.state == BT_PROFILE_STATE_CONNECTED) {
                uint8_t addr_buf[6];
                int profile_copy = (int)event.profile_state.profile;
                memcpy(addr_buf, event.profile_state.addr, 6);
                getMainDispatcher().dispatch([addr_buf, profile_copy]() mutable {
                    std::array<uint8_t, 6> peer_addr;
                    memcpy(peer_addr.data(), addr_buf, 6);
                    const auto hex = settings::addrToHex(peer_addr);
                    settings::PairedDevice stored;
                    if (settings::load(hex, stored) && stored.profileId != profile_copy) {
                        stored.profileId = profile_copy;
                        settings::save(stored);
                    }
                });
            // TODO: Fix auto reconnect if user manually disconnects
            } else if (event.profile_state.state == BT_PROFILE_STATE_IDLE &&
                       event.profile_state.profile == BT_PROFILE_HID_HOST) {
                // HID host disconnected — check if any peer has autoConnect and re-scan
                // so that autoConnectHidHost() fires when the scan finishes.
                getMainDispatcher().dispatch([] {
                    auto peers = settings::loadAll();
                    bool has_auto = false;
                    for (const auto& p : peers) {
                        if (p.autoConnect && p.profileId == BT_PROFILE_HID_HOST) {
                            has_auto = true;
                            break;
                        }
                    }
                    if (has_auto) {
                        Device* dev;
                        if (device_get_first_active_by_type(&BLUETOOTH_TYPE, &dev) == ERROR_NONE) {
                            if (!bluetooth_is_scanning(dev)) {
                                bluetooth_scan_start(dev);
                            }
                            device_put(dev);
                        }
                    }
                });
            }
            break;

        default:
            break;
    }
}

// ---- Bridge thread lifecycle ----
// Runs bt_event_bridge() on its own stack instead of whichever thread published the event, by
// blocking in task_event_group_wait_any() rather than being called back directly.

constexpr configSTACK_DEPTH_TYPE BT_EVENT_THREAD_STACK_SIZE = 4096;

Device* btEventDevice = nullptr;

int32_t btEventThreadMain() {
    while (btEventThreadRunning.load()) {
        task_event_group_wait_any(&btEventGroup, nullptr, pdMS_TO_TICKS(250));

        BtEvent event {};
        while (bluetooth_event_poll(&btEventSub, &event) == ERROR_NONE) {
            bt_event_bridge(event);
        }
    }
    return 0;
}

bool startBtEventThread(Device* dev) {
    if (btEventThread != nullptr) {
        return true; // already running
    }

    task_event_group_construct(&btEventGroup);
    if (bluetooth_event_subscribe(dev, &btEventSub, &btEventGroup) != ERROR_NONE) {
        task_event_group_destruct(&btEventGroup);
        return false;
    }

    btEventDevice = dev;
    btEventThreadRunning = true;
    btEventThread = new Thread("bt-events", BT_EVENT_THREAD_STACK_SIZE, [] { return btEventThreadMain(); });
    btEventThread->start();
    return true;
}

void stopBtEventThread() {
    if (btEventThread == nullptr) return;

    btEventThreadRunning = false;
    btEventThread->join();
    delete btEventThread;
    btEventThread = nullptr;

    bluetooth_event_unsubscribe(btEventDevice, &btEventSub);
    task_event_group_destruct(&btEventGroup);
    btEventDevice = nullptr;
}

// ---- systemStart ----

void systemStart() {
    Device* dev = findFirstRegisteredDevice();
    if (dev == nullptr) {
        LOG_W(TAG, "systemStart: no BLE device found");
        return;
    }

    if (settings::shouldEnableOnBoot()) {
        start(dev);
    }
}

bool isRadioOnOrPending(Device* dev) {
    if (!device_is_ready(dev)) return false;
    BtRadioState state;
    if (bluetooth_get_radio_state(dev, &state) != ERROR_NONE) return false;
    return state == BT_RADIO_STATE_ON || state == BT_RADIO_STATE_ON_PENDING;
}

// dev is started (device_start()) once, at kernel_init (see ble0's devicetree status) and never
// stopped for the process lifetime - this only toggles the radio itself, so callers subscribed
// directly to the driver (e.g. BtManage) stay subscribed across on/off toggles instead of
// having to resubscribe.
bool start(Device* dev) {
    // TODO: Fix bug where repeatedly calling start would try to subscribe the bridge thread twice
    if (!startBtEventThread(dev)) {
        LOG_E(TAG, "Failed to subscribe to BLE events");
    }

    LOG_I(TAG, "Enabling BT radio");
    if (bluetooth_set_radio_enabled(dev, true) != ERROR_NONE) {
        LOG_E(TAG, "Failed to enable BLE radio");
        stopBtEventThread();
        return false;
    }

    LOG_I(TAG, "BT enabled");
    return true;
}

bool stop(Device* dev) {
    BtRadioState state;
    if (bluetooth_get_radio_state(dev, &state) != ERROR_NONE) {
        return false;
    }

    if (state == BT_RADIO_STATE_OFF || state == BT_RADIO_STATE_OFF_PENDING) {
        return true;
    }

    stopBtEventThread();

    if (bluetooth_set_radio_enabled(dev, false) != ERROR_NONE) {
        LOG_E(TAG, "Failed to disable BT radio");
        // Re-subscribe bridge
        startBtEventThread(dev);
        return false;
    }

    return true;
}

// ---- Public API ----

const char* radioStateToString(RadioState state) {
    switch (state) {
        using enum RadioState;
        case Off:        return "Off";
        case OnPending:  return "OnPending";
        case On:         return "On";
        case OffPending: return "OffPending";
    }
    check(false, "not implemented");
}

RadioState getRadioState() {
    BtRadioState state = BT_RADIO_STATE_OFF;

    // Scoped to safeguard dev usage
    {
        Device* dev = nullptr;
        device_get_first_active_by_type(&BLUETOOTH_TYPE, &dev);
        if (dev == nullptr) {
            return RadioState::Off;
        }
        bluetooth_get_radio_state(dev, &state);
        device_put(dev);
    }

    switch (state) {
        case BT_RADIO_STATE_OFF:         return RadioState::Off;
        case BT_RADIO_STATE_ON_PENDING:  return RadioState::OnPending;
        case BT_RADIO_STATE_ON:          return RadioState::On;
        case BT_RADIO_STATE_OFF_PENDING: return RadioState::OffPending;
    }
    return RadioState::Off;
}

std::vector<PeerRecord> getScanResults() {
    auto lock = scan_cache_mutex.asScopedLock();
    lock.lock();
    return scan_results_cache;
}

std::vector<PeerRecord> getPairedPeers() {
    auto stored = settings::loadAll();
    std::vector<PeerRecord> result;
    result.reserve(stored.size());
    std::array<uint8_t, 6> connected_addr = {};
    bool hid_host_connected = hidHostGetConnectedPeer(connected_addr);
    for (const auto& device : stored) {
        PeerRecord record;
        record.addr      = device.addr;
        record.name      = device.name;
        record.rssi      = 0;
        record.paired    = true;
        record.profileId = device.profileId;
        record.connected = hid_host_connected && device.addr == connected_addr;
        result.push_back(std::move(record));
    }
    // Synthesize fallback: LittleFS readdir can lag behind fwrite by one tick, so the
    // connected peer may not appear in loadAll() yet. Always ensure it is in the list.
    if (hid_host_connected) {
        bool found = false;
        for (const auto& r : result) {
            if (r.addr == connected_addr) { found = true; break; }
        }
        if (!found) {
            PeerRecord record;
            record.addr      = connected_addr;
            record.rssi      = 0;
            record.paired    = true;
            record.connected = true;
            record.profileId = BT_PROFILE_HID_HOST;
            // Try to get the name from the scan cache.
            {
                auto lock = scan_cache_mutex.asScopedLock();
                lock.lock();
                for (const auto& sr : scan_results_cache) {
                    if (sr.addr == connected_addr) { record.name = sr.name; break; }
                }
            }
            result.push_back(std::move(record));
        }
    }
    return result;
}

void pair(const std::array<uint8_t, 6>& /*addr*/) {
    // Pairing is handled automatically during connection by NimBLE SM.
}

void unpair(const std::array<uint8_t, 6>& addr) {
    Device* dev;
    if (device_get_first_active_by_type(&BLUETOOTH_TYPE, &dev) == ERROR_NONE) {
        bluetooth_unpair(dev, addr.data());
        device_put(dev);
    }
    settings::remove(settings::addrToHex(addr));
}

void connect(const std::array<uint8_t, 6>& addr, int profileId) {
    LOG_I(TAG, "connect(profile=%d)", profileId);
    if (profileId == BT_PROFILE_HID_HOST) {
        hidHostConnect(addr);
    } else if (profileId == BT_PROFILE_HID_DEVICE) {
        if (Device* dev = bluetooth_hid_device_get()) {
            bluetooth_hid_device_start(dev, BT_HID_DEVICE_MODE_KEYBOARD);
            device_put(dev);
        }
    } else if (profileId == BT_PROFILE_SPP) {
        if (Device* dev = bluetooth_serial_get()) {
            bluetooth_serial_start(dev);
            settings::setSppAutoStart(true);
            device_put(dev);
        }
    } else if (profileId == BT_PROFILE_MIDI) {
        if (Device* dev = bluetooth_midi_get()) {
            bluetooth_midi_start(dev);
            settings::setMidiAutoStart(true);
            device_put(dev);
        }
    }
}

void disconnect(const std::array<uint8_t, 6>& addr, int profileId) {
    LOG_I(TAG, "disconnect(profile=%d)", profileId);
    if (profileId == BT_PROFILE_HID_HOST) {
        hidHostDisconnect();
    } else if (profileId == BT_PROFILE_HID_DEVICE) {
        if (Device* dev = bluetooth_hid_device_get()) {
            bluetooth_hid_device_stop(dev);
            device_put(dev);
        }
    } else {
        Device* dev;
        if (device_get_first_active_by_type(&BLUETOOTH_TYPE, &dev) == ERROR_NONE) {
            bluetooth_disconnect(dev, addr.data(), (BtProfileId)profileId);
            device_put(dev);
        }
    }
}

bool isProfileSupported(int profileId) {
    return profileId == BT_PROFILE_HID_HOST ||
           profileId == BT_PROFILE_HID_DEVICE ||
           profileId == BT_PROFILE_SPP ||
           profileId == BT_PROFILE_MIDI;
}

} // namespace tt::bluetooth

#endif // CONFIG_BT_NIMBLE_ENABLED

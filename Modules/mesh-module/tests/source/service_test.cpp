#include "doctest.h"

#include <mesh/backend.h>
#include <mesh/mesh.h>
#include <mesh/mesh_service.h>
#include <mesh/mesh_settings.h>

#include <service/manager.h>

#include <tactility/concurrent/thread.h>
#include <tactility/delay.h>
#include <tactility/device.h>
#include <tactility/drivers/radio.h>
#include <tactility/filesystem/file_system.h>
#include <tactility/paths.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

namespace {

// region Fake data filesystem, so the service can store messages in a temporary directory

error_t fakeMount(void*) { return ERROR_NONE; }
error_t fakeUnmount(void*) { return ERROR_NONE; }
bool fakeIsMounted(void*) { return false; }

error_t fakeGetPath(void* data, char* out_path, size_t out_path_size) {
    const char* path = static_cast<const char*>(data);
    if (strlen(path) + 1 > out_path_size) {
        return ERROR_BUFFER_OVERFLOW;
    }
    strcpy(out_path, path);
    return ERROR_NONE;
}

const FileSystemApi fakeDataApi = {
    .mount = fakeMount,
    .unmount = fakeUnmount,
    .is_mounted = fakeIsMounted,
    .get_path = fakeGetPath,
};

// endregion

// region Loopback backend: a frame is a 1-byte channel followed by the text

int startCount = 0;

void* loopbackCreate(MeshHost* host) {
    // Backends load their state here
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (mesh_host_get_data_path(host, "state.bin", path, sizeof(path)) != ERROR_NONE) {
        return nullptr;
    }
    return host;
}
void loopbackDestroy(void*) {}

const char* const loopbackPresets[] = { "Default", "Other", nullptr };

error_t loopbackGetRadioProfile(void*, RadioRegion, size_t preset, MeshRadioProfile* profile) {
    *profile = {
        .modulation = RADIO_MODULATION_LORA,
        .frequency = 869525000,
        .bandwidth = 250000,
        .spreading_factor = (preset == 1) ? 7 : 11,
        .coding_rate = 5,
        .sync_word = 0x2B,
        .preamble_length = 16,
        .power = 14,
    };
    return ERROR_NONE;
}

void loopbackOnStart(void*) { startCount++; }

// Channel 0xFF stands for frames the backend can't read, 0xFE for adverts
constexpr uint8_t UNREADABLE_CHANNEL = 0xFF;
constexpr uint8_t ADVERT_CHANNEL = 0xFE;

bool loopbackOnReceive(void* backend, const uint8_t* data, size_t length, float rssi, float snr) {
    auto* host = static_cast<MeshHost*>(backend);
    if (length < 1 || data[0] == UNREADABLE_CHANNEL) {
        return false;
    }
    if (data[0] == ADVERT_CHANNEL) {
        MeshAddress from = { .length = 1, .bytes = { 0x43 } };
        mesh_host_advert_heard(host, &from, "advertiser", rssi, snr);
        return true;
    }
    std::string text(reinterpret_cast<const char*>(data + 1), length - 1);
    MeshAddress from = { .length = 1, .bytes = { 0x42 } };
    mesh_host_node_heard(host, &from, "peer", rssi, snr);
    mesh_host_receive_text(host, &from, data[0], "peer", text.c_str(), rssi, snr, 0);
    return true;
}

void loopbackOnTick(void*) {}

error_t loopbackSendText(void* backend, uint32_t seq, const MeshAddress*, uint8_t channel, const char* text) {
    uint8_t frame[1 + MESH_TEXT_MAX_LENGTH];
    frame[0] = channel;
    const size_t length = strlen(text);
    memcpy(frame + 1, text, length);
    return mesh_host_transmit(static_cast<MeshHost*>(backend), frame, length + 1, seq);
}

const MeshBackendApi loopbackApi = {
    .id = "loopback",
    .create = loopbackCreate,
    .destroy = loopbackDestroy,
    .presets = loopbackPresets,
    .get_radio_profile = loopbackGetRadioProfile,
    .on_start = loopbackOnStart,
    .on_receive = loopbackOnReceive,
    .on_tick = loopbackOnTick,
    .send_text = loopbackSendText,
    .get_channels = nullptr,
    .set_channel = nullptr,
};

// endregion

bool waitFor(const std::function<bool()>& condition) {
    for (int i = 0; i < 300; i++) {
        if (condition()) {
            return true;
        }
        delay_millis(10);
    }
    return false;
}

MeshDeliveryState getDelivery(uint32_t seq) {
    MeshMessage message {};
    mesh_message_get(seq, &message);
    return message.delivery;
}

} // namespace

TEST_CASE("mesh service sends and receives through the active backend") {
    char dataPath[] = "/tmp/mesh_service_test_XXXXXX";
    REQUIRE_NE(mkdtemp(dataPath), nullptr);
    FileSystem* dataFs = file_system_add("data", &fakeDataApi, dataPath);

    // The service uses the first radio, the second one plays the remote node
    static Device local { .name = "radio_local", .config = nullptr, .parent = nullptr };
    static Device remote { .name = "radio_remote", .config = nullptr, .parent = nullptr };
    CHECK_FALSE(mesh_is_available());
    REQUIRE_EQ(device_construct_add(&local, "mock,radio"), ERROR_NONE);
    REQUIRE_EQ(device_construct_add(&remote, "mock,radio"), ERROR_NONE);
    REQUIRE_EQ(device_start(&local), ERROR_NONE);
    REQUIRE_EQ(device_start(&remote), ERROR_NONE);
    CHECK(mesh_is_available());

    REQUIRE_EQ(mesh_backend_register(&loopbackApi), ERROR_NONE);
    CHECK_EQ(mesh_backend_register(&loopbackApi), ERROR_ALREADY_EXISTS);
    REQUIRE_EQ(service_manager_add(&mesh_service_manifest, false), ERROR_NONE);

    SUBCASE("without a region the radio stays off") {
        REQUIRE_EQ(service_manager_start(MESH_SERVICE_ID), ERROR_NONE);
        CHECK(waitFor([] { return mesh_get_state() == MESH_STATE_NO_REGION; }));
        CHECK_EQ(mesh_send_text(nullptr, 0, "hi", nullptr), ERROR_INVALID_STATE);

        // Setting a region later brings the service up
        CHECK_EQ(radio_set_region(&local, RADIO_REGION_EU_868), ERROR_NONE);
        CHECK(waitFor([] { return mesh_get_state() == MESH_STATE_ON; }));
    }

    SUBCASE("a radio that starts after the service is picked up") {
        REQUIRE_EQ(device_stop(&local), ERROR_NONE);
        REQUIRE_EQ(device_stop(&remote), ERROR_NONE);
        REQUIRE_EQ(service_manager_start(MESH_SERVICE_ID), ERROR_NONE);
        CHECK(waitFor([] { return mesh_get_state() == MESH_STATE_NO_RADIO; }));

        REQUIRE_EQ(device_start(&local), ERROR_NONE);
        REQUIRE_EQ(device_start(&remote), ERROR_NONE);
        REQUIRE_EQ(radio_set_region(&local, RADIO_REGION_EU_868), ERROR_NONE);
        CHECK(waitFor([] { return mesh_get_state() == MESH_STATE_ON; }));
    }

    SUBCASE("selecting a preset reconfigures the radio") {
        REQUIRE_EQ(radio_set_region(&local, RADIO_REGION_EU_868), ERROR_NONE);
        REQUIRE_EQ(service_manager_start(MESH_SERVICE_ID), ERROR_NONE);
        REQUIRE(waitFor([] { return mesh_get_state() == MESH_STATE_ON; }));

        char name[MESH_NAME_MAX_LENGTH + 1];
        CHECK_EQ(mesh_get_preset_count(), 2);
        CHECK_EQ(mesh_get_preset_name(1, name, sizeof(name)), ERROR_NONE);
        CHECK_EQ(std::string(name), "Other");
        CHECK_EQ(mesh_get_preset_name(2, name, sizeof(name)), ERROR_NOT_FOUND);
        CHECK_EQ(mesh_get_preset(name, sizeof(name)), ERROR_NONE);
        CHECK_EQ(std::string(name), "Default");

        CHECK_EQ(mesh_set_preset("Missing"), ERROR_NOT_FOUND);
        REQUIRE_EQ(mesh_set_preset("Other"), ERROR_NONE);
        CHECK_EQ(mesh_get_preset(name, sizeof(name)), ERROR_NONE);
        CHECK_EQ(std::string(name), "Other");
        CHECK(waitFor([] {
            int32_t spreadingFactor = 0;
            return radio_get_parameter(&local, RADIO_PARAMETER_SPREADING_FACTOR, &spreadingFactor) == ERROR_NONE && spreadingFactor == 7;
        }));
        CHECK(waitFor([] { return mesh_get_state() == MESH_STATE_ON; }));
        REQUIRE_EQ(mesh_set_preset("Default"), ERROR_NONE);
    }

    SUBCASE("concurrent synchronous calls finish") {
        REQUIRE_EQ(radio_set_region(&local, RADIO_REGION_EU_868), ERROR_NONE);
        REQUIRE_EQ(service_manager_start(MESH_SERVICE_ID), ERROR_NONE);
        REQUIRE(waitFor([] { return mesh_get_state() == MESH_STATE_ON; }));

        const auto caller = [](void*) -> int32_t {
            for (int i = 0; i < 200; i++) {
                MeshChannel channels[1];
                size_t count = 0;
                mesh_get_channels(channels, 1, &count);
                mesh_get_state();
            }
            return 0;
        };
        Thread* threads[4];
        for (auto& thread : threads) {
            thread = thread_alloc_full("mesh_caller", 4096, caller, nullptr, -1);
            REQUIRE_EQ(thread_start(thread), ERROR_NONE);
        }
        for (auto* thread : threads) {
            CHECK_EQ(thread_join(thread, pdMS_TO_TICKS(5000), 1), ERROR_NONE);
            thread_free(thread);
        }
    }

    SUBCASE("messages travel both ways") {
        REQUIRE_EQ(radio_set_region(&local, RADIO_REGION_EU_868), ERROR_NONE);
        REQUIRE_EQ(service_manager_start(MESH_SERVICE_ID), ERROR_NONE);
        REQUIRE(waitFor([] { return mesh_get_state() == MESH_STATE_ON; }));

        char backend[MESH_BACKEND_ID_MAX_LENGTH + 1];
        CHECK_EQ(mesh_get_backend(backend, sizeof(backend)), ERROR_NONE);
        CHECK_EQ(std::string(backend), "loopback");

        // The service owns the radio
        CHECK_EQ(radio_acquire(&local, "someone-else"), ERROR_RESOURCE_BUSY);

        // Tune the remote radio like the backend profile
        MeshRadioProfile profile;
        loopbackGetRadioProfile(nullptr, RADIO_REGION_EU_868, 0, &profile);
        REQUIRE_EQ(radio_set_modulation(&remote, profile.modulation), ERROR_NONE);
        radio_set_parameter(&remote, RADIO_PARAMETER_FREQUENCY, profile.frequency);
        radio_set_parameter(&remote, RADIO_PARAMETER_BANDWIDTH, profile.bandwidth);
        radio_set_parameter(&remote, RADIO_PARAMETER_SPREADING_FACTOR, profile.spreading_factor);
        radio_set_parameter(&remote, RADIO_PARAMETER_SYNC_WORD, profile.sync_word);
        REQUIRE_EQ(radio_set_enabled(&remote, true), ERROR_NONE);

        TaskEventGroup group {};
        task_event_group_construct(&group);
        static RadioRxEventSubscription remoteRx {};
        REQUIRE_EQ(radio_rx_event_subscribe(&remoteRx, &group, &remote), ERROR_NONE);
        MeshEventSubscription meshEvents {};
        REQUIRE_EQ(mesh_event_subscribe(&meshEvents, &group), ERROR_NONE);

        // Outgoing
        uint32_t seq = 0;
        REQUIRE_EQ(mesh_send_text(nullptr, 3, "hello mesh", &seq), ERROR_NONE);
        CHECK(waitFor([seq] { return getDelivery(seq) == MESH_DELIVERY_SENT; }));

        static RadioRxEvent rxEvent;
        REQUIRE_EQ(radio_rx_event_poll(&remoteRx, &rxEvent), ERROR_NONE);
        CHECK_EQ(rxEvent.data[0], 3);
        CHECK_EQ(std::string(reinterpret_cast<const char*>(rxEvent.data + 1), rxEvent.length - 1), "hello mesh");

        // Incoming
        const uint8_t reply[] = { 3, 'h', 'i', '!' };
        REQUIRE_EQ(radio_transmit(&remote, reply, sizeof(reply), nullptr), ERROR_NONE);
        MeshMessage history[4];
        size_t count = 0;
        CHECK(waitFor([&] {
            return mesh_history_get(0, history, 4, &count) == ERROR_NONE && count == 2;
        }));
        CHECK_FALSE(history[0].outgoing);
        CHECK_EQ(std::string(history[0].text), "hi!");
        CHECK_EQ(std::string(history[0].sender_name), "peer");
        CHECK_EQ(history[0].channel, 3);
        CHECK_EQ(history[0].delivery, MESH_DELIVERY_RECEIVED);
        CHECK(history[1].outgoing);

        MeshNode nodes[4];
        CHECK_EQ(mesh_get_nodes(nodes, 4, &count), ERROR_NONE);
        CHECK_EQ(count, 1);
        CHECK_EQ(std::string(nodes[0].name), "peer");

        // Events: state, message, delivery and node events were queued
        MeshEvent event {};
        bool sawMessage = false;
        bool sawDelivery = false;
        while (mesh_event_poll(&meshEvents, &event) == ERROR_NONE) {
            sawMessage |= event.type == MESH_EVENT_MESSAGE;
            sawDelivery |= event.type == MESH_EVENT_DELIVERY;
        }
        CHECK(sawMessage);
        CHECK(sawDelivery);

        // Frames that aren't messages are only reported as events
        const uint8_t unreadable[] = { UNREADABLE_CHANNEL, 1, 2, 3, 4 };
        REQUIRE_EQ(radio_transmit(&remote, unreadable, sizeof(unreadable), nullptr), ERROR_NONE);
        const uint8_t advert[] = { ADVERT_CHANNEL };
        REQUIRE_EQ(radio_transmit(&remote, advert, sizeof(advert), nullptr), ERROR_NONE);
        bool sawUnreadable = false;
        bool sawAdvert = false;
        CHECK(waitFor([&] {
            while (mesh_event_poll(&meshEvents, &event) == ERROR_NONE) {
                if (event.type == MESH_EVENT_FRAME && event.frame.kind == MESH_FRAME_UNREADABLE) {
                    sawUnreadable = event.frame.length == sizeof(unreadable);
                }
                if (event.type == MESH_EVENT_FRAME && event.frame.kind == MESH_FRAME_ADVERT) {
                    sawAdvert = std::string(event.frame.name) == "advertiser" && event.frame.from.bytes[0] == 0x43;
                }
            }
            return sawUnreadable && sawAdvert;
        }));
        CHECK(mesh_history_get(0, history, 4, &count) == ERROR_NONE);
        CHECK_EQ(count, 2);

        CHECK_EQ(mesh_cancel(seq), ERROR_INVALID_STATE);
        CHECK_EQ(mesh_raw_send("other", reply, sizeof(reply)), ERROR_INVALID_STATE);
        CHECK_EQ(mesh_raw_send("loopback", reply, sizeof(reply)), ERROR_NONE);

        CHECK_EQ(mesh_event_unsubscribe(&meshEvents), ERROR_NONE);
        CHECK_EQ(radio_rx_event_unsubscribe(&remoteRx), ERROR_NONE);
        task_event_group_destruct(&group);
    }

    CHECK_EQ(service_manager_stop(MESH_SERVICE_ID), ERROR_NONE);
    CHECK_EQ(mesh_get_state(), MESH_STATE_OFF);
    // Released on stop
    CHECK_EQ(radio_acquire(&local, "someone-else"), ERROR_NONE);
    CHECK_EQ(radio_release(&local), ERROR_NONE);

    CHECK_EQ(service_manager_remove(MESH_SERVICE_ID), ERROR_NONE);
    CHECK_EQ(mesh_backend_unregister(&loopbackApi), ERROR_NONE);
    radio_set_region(&local, RADIO_REGION_UNSET);
    radio_set_enabled(&remote, false);
    device_stop(&remote);
    device_stop(&local);
    device_remove(&remote);
    device_remove(&local);
    device_destruct(&remote);
    device_destruct(&local);
    file_system_remove(dataFs);
}

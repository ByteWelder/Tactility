// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mesh/mesh.h>

#include <tactility/drivers/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interface between the mesh service and a protocol backend. Not part of the app SDK.
 *
 * The service calls every backend function on its own thread, and backends call the mesh_host_*() functions
 * only from those calls, so backends need no locking.
 */

/** The service, as seen by a backend */
struct MeshHost;

/** Radio configuration a backend needs for a region */
struct MeshRadioProfile {
    enum RadioModulation modulation;
    int32_t frequency;
    int32_t bandwidth;
    int32_t spreading_factor;
    int32_t coding_rate;
    int32_t sync_word;
    int32_t preamble_length;
    int32_t power;
};

struct MeshBackendApi {
    /** Unique id, at most MESH_BACKEND_ID_MAX_LENGTH characters */
    const char* id;

    /** Create the backend state. Called when the backend becomes active. */
    void* (*create)(struct MeshHost* host);

    /** Destroy the backend state. Called when another backend becomes active or the service stops. */
    void (*destroy)(void* backend);

    /** Optional NULL-terminated list of selectable radio preset names. Index 0 is the default preset. */
    const char* const* presets;

    /**
     * @param[in] preset index in presets, 0 when the backend has none
     * @retval ERROR_NOT_SUPPORTED when the backend can't operate in @a region with @a preset
     */
    error_t (*get_radio_profile)(void* backend, enum RadioRegion region, size_t preset, struct MeshRadioProfile* profile);

    /** Called once the radio is on with the backend's profile */
    void (*on_start)(void* backend);

    /** A frame was received */
    /** @return false when the frame couldn't be read, e.g. a malformed frame or a channel without a known key */
    bool (*on_receive)(void* backend, const uint8_t* data, size_t length, float rssi, float snr);

    /** Called about once per second */
    void (*on_tick)(void* backend);

    /**
     * Encode and transmit an outgoing text message with mesh_host_transmit(), passing @a seq along.
     * @param[in] to the recipient, NULL for a channel message
     */
    error_t (*send_text)(void* backend, uint32_t seq, const struct MeshAddress* to, uint8_t channel, const char* text);

    /** Optional */
    error_t (*get_channels)(void* backend, struct MeshChannel* channels, size_t max_count, size_t* count);

    /** Optional */
    error_t (*set_channel)(void* backend, const struct MeshChannel* channel);
};

/**
 * Register a backend. Usually called from the backend module's start().
 * @retval ERROR_ALREADY_EXISTS when a backend with the same id is registered
 * @retval ERROR_RESOURCE when the registry is full
 */
error_t mesh_backend_register(const struct MeshBackendApi* api);

error_t mesh_backend_unregister(const struct MeshBackendApi* api);

// region Host functions, only to be called from backend functions

/**
 * Queue a frame for transmission. The service enforces the airtime budget of the region.
 * @param[in] seq the message this frame belongs to: its delivery state becomes MESH_DELIVERY_SENT once the
 * frame is out. 0 for frames that don't belong to a message.
 */
error_t mesh_host_transmit(struct MeshHost* host, const uint8_t* data, size_t length, uint32_t seq);

/**
 * Store an incoming text message.
 * @param[in] from the sender, NULL for a channel message from an unknown sender
 * @param[in] sender_time the sender's timestamp in seconds since the Unix epoch, 0 when unknown
 * @return the sequence number of the stored message, 0 on failure
 */
uint32_t mesh_host_receive_text(
    struct MeshHost* host,
    const struct MeshAddress* from,
    uint8_t channel,
    const char* sender_name,
    const char* text,
    float rssi,
    float snr,
    int64_t sender_time
);

/** Update the delivery state of an outgoing message, e.g. MESH_DELIVERY_DELIVERED when acknowledged */
void mesh_host_set_delivery(struct MeshHost* host, uint32_t seq, enum MeshDeliveryState delivery);

/** Record that a node was heard. @a name may be NULL to keep the known name. */
void mesh_host_node_heard(struct MeshHost* host, const struct MeshAddress* address, const char* name, float rssi, float snr);

/** Record that a node announced itself, e.g. a MeshCore advert or a Meshtastic NodeInfo. Also records the node as heard. */
void mesh_host_advert_heard(struct MeshHost* host, const struct MeshAddress* address, const char* name, float rssi, float snr);

/** Get a path in the backend's own data directory, which exists after this call */
error_t mesh_host_get_data_path(struct MeshHost* host, const char* child_path, char* out_path, size_t out_path_size);

/** Get the name this node announces */
void mesh_host_get_node_name(struct MeshHost* host, char* name, size_t name_size);

// endregion

#ifdef __cplusplus
}
#endif

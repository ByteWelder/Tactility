// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tactility/concurrent/task_event_group.h>
#include <tactility/drivers/radio.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mesh networking service. One protocol backend (e.g. MeshCore or Meshtastic) is active at a time and owns
 * the radio. Messages are stored durably, independent of the backend that produced them.
 *
 * Received raw frames are available to anyone through radio_rx_event_subscribe() on the radio device.
 */

/** Incremented when the API changes in a way that matters to apps */
#define MESH_API_VERSION 2

#define MESH_BACKEND_ID_MAX_LENGTH 15
#define MESH_ADDRESS_MAX_LENGTH 32
#define MESH_NAME_MAX_LENGTH 31
#define MESH_TEXT_MAX_LENGTH 200
#define MESH_CHANNEL_KEY_MAX_LENGTH 32

enum MeshState {
    /** The service is not running */
    MESH_STATE_OFF,
    /** Running and connected to the mesh */
    MESH_STATE_ON,
    /** There is no radio device */
    MESH_STATE_NO_RADIO,
    /** The radio is owned by someone else */
    MESH_STATE_RADIO_BUSY,
    /** No regulatory region is set on the radio (see radio_set_region()), so it stays off */
    MESH_STATE_NO_REGION,
    /** No backend is registered or the selected backend doesn't exist */
    MESH_STATE_NO_BACKEND,
    /** The radio or backend failed to start */
    MESH_STATE_ERROR,
};

enum MeshTimeSource {
    /** Wall time unknown */
    MESH_TIME_SOURCE_NONE,
    /** System clock (set by RTC, NTP or GPS) */
    MESH_TIME_SOURCE_SYSTEM,
    /** Time reported by the sender over the mesh */
    MESH_TIME_SOURCE_MESH,
};

enum MeshDeliveryState {
    /** Incoming message */
    MESH_DELIVERY_RECEIVED,
    /** Outgoing message waiting for the radio */
    MESH_DELIVERY_QUEUED,
    /** Outgoing message of a backend that is not active. It resumes when that backend becomes active again. */
    MESH_DELIVERY_BLOCKED,
    /** Transmitted */
    MESH_DELIVERY_SENT,
    /** Acknowledged by the recipient */
    MESH_DELIVERY_DELIVERED,
    MESH_DELIVERY_FAILED,
    MESH_DELIVERY_CANCELLED,
};

/** Backend-specific node address, e.g. a public key or a node number */
struct MeshAddress {
    uint8_t length;
    uint8_t bytes[MESH_ADDRESS_MAX_LENGTH];
};

struct MeshMessage {
    /** Unique, increasing sequence number */
    uint32_t seq;
    char backend[MESH_BACKEND_ID_MAX_LENGTH + 1];
    bool outgoing;
    enum MeshDeliveryState delivery;
    /** Sender of an incoming message, recipient of an outgoing one. Empty for channel messages. */
    struct MeshAddress peer;
    uint8_t channel;
    char sender_name[MESH_NAME_MAX_LENGTH + 1];
    /** Seconds since the Unix epoch, see time_source */
    int64_t wall_time;
    enum MeshTimeSource time_source;
    /** Milliseconds since boot when the message was stored */
    uint64_t uptime_ms;
    /** Signal strength of an incoming message in dBm */
    float rssi;
    /** Signal-to-noise ratio of an incoming message in dB, NAN when unknown */
    float snr;
    char text[MESH_TEXT_MAX_LENGTH + 1];
};

struct MeshNode {
    char backend[MESH_BACKEND_ID_MAX_LENGTH + 1];
    struct MeshAddress address;
    char name[MESH_NAME_MAX_LENGTH + 1];
    /** Milliseconds since boot */
    uint64_t last_heard_ms;
    float rssi;
    float snr;
};

/** A node that the user saved for direct messages */
struct MeshContact {
    char backend[MESH_BACKEND_ID_MAX_LENGTH + 1];
    struct MeshAddress address;
    char name[MESH_NAME_MAX_LENGTH + 1];
};

/**
 * A channel of the active backend. How the name and key combine is backend-specific: Meshtastic derives
 * the channel identity from both, while for MeshCore the name is only a local label.
 */
struct MeshChannel {
    uint8_t index;
    char name[MESH_NAME_MAX_LENGTH + 1];
    uint8_t key[MESH_CHANNEL_KEY_MAX_LENGTH];
    uint8_t key_length;
};

// region Service control

/** Turn the service on or off. The choice is persisted. */
error_t mesh_set_enabled(bool enabled);

bool mesh_is_enabled(void);

enum MeshState mesh_get_state(void);

const char* mesh_state_to_string(enum MeshState state);

/** Set the name this node announces. The choice is persisted. */
error_t mesh_set_node_name(const char* name);

error_t mesh_get_node_name(char* name, size_t name_size);

/** @return true when a started radio can be used for mesh networking */
bool mesh_is_available(void);

/** @return the number of started radios that can be used for mesh networking: LoRa radios, and ESP-NOW radios */
size_t mesh_get_radio_count(void);

/**
 * @param[in] index the radio index, below mesh_get_radio_count()
 * @param[out] name the name of the radio device
 * @retval ERROR_NOT_FOUND for an invalid index
 */
error_t mesh_get_radio_name(size_t index, char* name, size_t name_size);

enum MeshRadioKind {
    MESH_RADIO_LORA,
    MESH_RADIO_ESPNOW
};

/**
 * @param[in] index the radio index, below mesh_get_radio_count()
 * @param[out] kind what the radio does
 * @retval ERROR_NOT_FOUND for an invalid index
 */
error_t mesh_get_radio_kind(size_t index, enum MeshRadioKind* kind);

/**
 * Select the radio to use. The choice is persisted by name.
 * @param[in] name the name of a radio, or NULL or empty for the default radio: the first radio that isn't ESP-NOW,
 * or the first ESP-NOW radio when there is no other
 * @retval ERROR_NOT_FOUND when there is no such radio
 */
error_t mesh_set_radio(const char* name);

/**
 * Get the name of the radio that is used: the selected radio, or the default radio
 * @retval ERROR_NOT_FOUND when there is no radio
 */
error_t mesh_get_radio(char* name, size_t name_size);

// endregion

// region Backends

size_t mesh_get_backend_count(void);

/**
 * @param[in] index the backend index, below mesh_get_backend_count()
 * @param[out] id the backend id
 * @param[in] id_size size of @a id
 * @retval ERROR_NOT_FOUND for an invalid index
 */
error_t mesh_get_backend_id(size_t index, char* id, size_t id_size);

/**
 * Select the active backend. Outgoing messages of the previous backend that weren't sent yet become
 * MESH_DELIVERY_BLOCKED. The choice is persisted.
 * @retval ERROR_NOT_FOUND when no such backend is registered
 */
error_t mesh_set_backend(const char* id);

/** @retval ERROR_NOT_FOUND when no backend is selected */
error_t mesh_get_backend(char* id, size_t id_size);

/** @return the number of radio presets of the selected backend, 0 when it has none */
size_t mesh_get_preset_count(void);

/**
 * @param[in] index the preset index, below mesh_get_preset_count()
 * @retval ERROR_NOT_FOUND for an invalid index
 */
error_t mesh_get_preset_name(size_t index, char* name, size_t name_size);

/**
 * Select the radio preset of the selected backend. The choice is persisted and resets when the backend changes.
 * @retval ERROR_NOT_FOUND when the selected backend has no such preset
 */
error_t mesh_set_preset(const char* name);

/** Get the selected preset name, which is the backend's default preset when none was selected */
error_t mesh_get_preset(char* name, size_t name_size);

// endregion

// region Messaging

/**
 * Queue a text message on the active backend.
 * @param[in] to the recipient, or NULL to send to @a channel
 * @param[in] channel the channel index, ignored when @a to is set
 * @param[in] text the text, at most MESH_TEXT_MAX_LENGTH bytes
 * @param[out] seq the sequence number of the stored message (optional)
 * @retval ERROR_INVALID_STATE when the service is not on
 * @retval ERROR_OUT_OF_RANGE when the text is too long
 */
error_t mesh_send_text(const struct MeshAddress* to, uint8_t channel, const char* text, uint32_t* seq);

/**
 * Cancel an outgoing message that is queued or blocked.
 * @retval ERROR_NOT_FOUND when there is no such message
 * @retval ERROR_INVALID_STATE when the message can't be cancelled anymore
 */
error_t mesh_cancel(uint32_t seq);

/**
 * Send a backend-specific frame as-is.
 * @param[in] backend the backend that must be active
 * @retval ERROR_INVALID_STATE when @a backend is not the active backend
 */
error_t mesh_raw_send(const char* backend, const uint8_t* data, size_t length);

/** @retval ERROR_NOT_FOUND when there is no such message */
error_t mesh_message_get(uint32_t seq, struct MeshMessage* message);

/**
 * Read the messages of one conversation of the active backend, newest first.
 * @param[in] peer the peer of a direct conversation, or NULL for the conversation of @a channel
 * @param[in] channel the channel index, ignored when @a peer is set
 * @param[in] before_seq only return messages older than this, or 0 for the newest
 * @param[out] messages buffer for the messages
 * @param[in] max_count capacity of @a messages
 * @param[out] count the number of messages returned
 * @retval ERROR_INVALID_STATE when the service is not running
 */
error_t mesh_conversation_get(const struct MeshAddress* peer, uint8_t channel, uint32_t before_seq, struct MeshMessage* messages, size_t max_count, size_t* count);

/**
 * Read stored messages, newest first.
 * @param[in] before_seq only return messages older than this, or 0 for the newest
 * @param[out] messages buffer for the messages
 * @param[in] max_count capacity of @a messages
 * @param[out] count the number of messages returned
 */
error_t mesh_history_get(uint32_t before_seq, struct MeshMessage* messages, size_t max_count, size_t* count);

// endregion

// region Contacts

/**
 * Get the saved contacts of the active backend. A node that sends a direct message is saved automatically.
 * @retval ERROR_INVALID_STATE when the service is not running
 */
error_t mesh_get_contacts(struct MeshContact* contacts, size_t max_count, size_t* count);

/**
 * Save a contact for the active backend, or rename it when the address is saved already. The choice is persisted.
 * @param[in] name the name to show, or NULL to take it from the nodes that were heard
 * @retval ERROR_INVALID_STATE when the service is not running
 * @retval ERROR_INVALID_ARGUMENT when the active backend doesn't accept the address, e.g. a public key of the wrong length
 */
error_t mesh_add_contact(const struct MeshAddress* address, const char* name);

/**
 * Remove a contact of the active backend. The stored messages remain.
 * @retval ERROR_NOT_FOUND when there is no such contact
 */
error_t mesh_remove_contact(const struct MeshAddress* address);

/**
 * Get a URI of a contact of the active backend that other mesh apps can scan as a QR code to add it.
 * @param[in] contact the contact's address, or NULL for this node
 * @param[out] uri the URI
 * @retval ERROR_NOT_SUPPORTED when the backend has no such URI
 * @retval ERROR_NOT_FOUND when @a contact isn't a saved contact
 * @retval ERROR_BUFFER_OVERFLOW when the URI doesn't fit
 */
error_t mesh_get_contact_uri(const struct MeshAddress* contact, char* uri, size_t uri_size);

// endregion

// region Nodes and channels

/** Get the nodes heard by the active backend since it started */
error_t mesh_get_nodes(struct MeshNode* nodes, size_t max_count, size_t* count);

/** @retval ERROR_NOT_SUPPORTED when the backend has no channels */
error_t mesh_get_channels(struct MeshChannel* channels, size_t max_count, size_t* count);

/**
 * Add or replace the channel with the same index.
 * A channel without a key (key_length 0) is only accepted when the backend derives the key from the name,
 * like MeshCore does for hashtag channels such as "#test".
 */
error_t mesh_set_channel(const struct MeshChannel* channel);

/**
 * Leave a channel. The stored messages remain. Channel 0 is the primary channel and can't be removed.
 * @retval ERROR_NOT_FOUND when there is no such channel
 * @retval ERROR_INVALID_ARGUMENT for channel 0
 * @retval ERROR_NOT_SUPPORTED when the backend has no channels
 */
error_t mesh_remove_channel(uint8_t index);

// endregion

// region Events

enum MeshEventType {
    /** The service state changed, see MeshEvent.state */
    MESH_EVENT_STATE,
    /** A message was stored, see MeshEvent.seq */
    MESH_EVENT_MESSAGE,
    /** The delivery state of a message changed, see MeshEvent.seq and MeshEvent.delivery */
    MESH_EVENT_DELIVERY,
    /** The node list changed */
    MESH_EVENT_NODE,
    /** The active backend received a frame that isn't a message, see MeshEvent.frame. It isn't stored. */
    MESH_EVENT_FRAME,
    /** A contact was added or removed */
    MESH_EVENT_CONTACT,
    /** A channel was added, replaced or removed */
    MESH_EVENT_CHANNEL,
};

enum MeshFrameKind {
    /** A frame the backend couldn't read, e.g. a malformed frame or a channel without a known key */
    MESH_FRAME_UNREADABLE,
    /** A node announced itself, see MeshEvent.frame.from and MeshEvent.frame.name */
    MESH_FRAME_ADVERT,
};

struct MeshEvent {
    uint64_t timestamp;
    enum MeshEventType type;
    enum MeshState state;
    uint32_t seq;
    enum MeshDeliveryState delivery;
    struct {
        enum MeshFrameKind kind;
        /** Frame length in bytes, 0 when unknown */
        uint16_t length;
        float rssi;
        float snr;
        struct MeshAddress from;
        char name[MESH_NAME_MAX_LENGTH + 1];
    } frame;
};

#define MESH_EVENT_QUEUE_CAPACITY 8

/**
 * Caller-owned subscription, used like the radio event subscriptions: wait on the event group, then drain
 * with mesh_event_poll().
 * @warning Fields other than `bit` are for internal use only.
 */
struct MeshEventSubscription {
    uint32_t bit;

    struct {
        struct TaskEventGroup* event_group;
        struct MeshEvent queue[MESH_EVENT_QUEUE_CAPACITY];
        uint8_t head;
        uint8_t count;
        struct MeshEventSubscription* next;
    } internal;
};

/**
 * @param[in,out] sub subscription to register, must stay alive until unsubscribed
 * @param[in] event_group caller-owned group to wait on, must outlive @a sub
 * @retval ERROR_RESOURCE @a event_group has no free bits left
 * @retval ERROR_INVALID_STATE @a sub is already registered
 */
error_t mesh_event_subscribe(struct MeshEventSubscription* sub, struct TaskEventGroup* event_group);

error_t mesh_event_unsubscribe(struct MeshEventSubscription* sub);

/**
 * Non-blocking: pop the next event.
 * @retval ERROR_TIMEOUT nothing queued
 */
error_t mesh_event_poll(struct MeshEventSubscription* sub, struct MeshEvent* event);

// endregion

#ifdef __cplusplus
}
#endif

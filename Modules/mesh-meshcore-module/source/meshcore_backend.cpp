// SPDX-License-Identifier: Apache-2.0
#include <mesh_meshcore/meshcore_backend.h>
#include <mesh_meshcore/meshcore_protocol.h>

#include <mesh/backend.h>

#include <TactilityCpp/Memory.h>
#include <TactilityCpp/std/deque.h>
#include <TactilityCpp/std/map.h>
#include <TactilityCpp/std/string.h>
#include <TactilityCpp/std/vector.h>

#include <tactility/log.h>
#include <tactility/paths.h>
#include <tactility/time.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>

constexpr auto* TAG = "meshcore";

namespace {

constexpr size_t MAX_CONTACTS = 64;
constexpr size_t MAX_CHANNELS = 8;
constexpr size_t SEEN_PACKETS_LIMIT = 64;
constexpr uint64_t ADVERT_INTERVAL_MS = 12ULL * 3600 * 1000;
constexpr uint64_t ACK_TIMEOUT_MS = 30 * 1000;
constexpr uint8_t TEXT_TYPE_PLAIN = 0;
constexpr int64_t MIN_VALID_WALL_TIME = 1700000000;
constexpr size_t FRAME_SIZE = 255;

struct Contact {
    uint8_t publicKey[MESHCORE_PUBLIC_KEY_SIZE];
    char name[MESH_NAME_MAX_LENGTH + 1];
};

struct Channel {
    uint8_t index;
    char name[MESH_NAME_MAX_LENGTH + 1];
    uint8_t key[16];
    MeshCoreChannel channel;
};

struct PendingAck {
    uint32_t seq;
    uint64_t deadlineMs;
};

struct Backend {
    MeshHost* host = nullptr;
    MeshCoreIdentity identity {};
    tt::std::vector<Contact> contacts;
    tt::std::vector<Channel> channels;
    tt::std::deque<uint32_t> seenPackets;
    tt::std::map<uint32_t, PendingAck> pendingAcks;
    uint64_t lastAdvertMs = 0;
};

uint64_t getMillis() {
    return get_micros_since_boot() / 1000;
}

/** MeshCore uses seconds since the epoch, or uptime when the clock was never set */
uint32_t getTimestamp() {
    const int64_t now = (int64_t)time(nullptr);
    return (now >= MIN_VALID_WALL_TIME) ? (uint32_t)now : (uint32_t)(getMillis() / 1000);
}

// region Persistence

bool getPath(Backend* backend, const char* name, char* path, size_t size) {
    return mesh_host_get_data_path(backend->host, name, path, size) == ERROR_NONE;
}

bool readFile(const char* path, tt::std::vector<uint8_t>& data) {
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    uint8_t buffer[256];
    size_t read;
    while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        data.insert(data.end(), buffer, buffer + read);
    }
    fclose(file);
    return true;
}

bool writeFile(const char* path, const uint8_t* data, size_t length) {
    FILE* file = fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    const bool ok = fwrite(data, 1, length, file) == length;
    fclose(file);
    return ok;
}

void loadOrCreateIdentity(Backend* backend) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (!getPath(backend, "identity.bin", path, sizeof(path))) {
        meshcore_identity_create(&backend->identity);
        return;
    }
    tt::std::vector<uint8_t> data;
    if (readFile(path, data) && data.size() == sizeof(MeshCoreIdentity)) {
        memcpy(&backend->identity, data.data(), sizeof(MeshCoreIdentity));
        return;
    }
    meshcore_identity_create(&backend->identity);
    if (!writeFile(path, reinterpret_cast<const uint8_t*>(&backend->identity), sizeof(MeshCoreIdentity))) {
        LOG_E(TAG, "Failed to save the identity");
    }
}

void loadContacts(Backend* backend) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    tt::std::vector<uint8_t> data;
    if (!getPath(backend, "contacts.bin", path, sizeof(path)) || !readFile(path, data)) {
        return;
    }
    for (size_t offset = 0; offset + sizeof(Contact) <= data.size() && backend->contacts.size() < MAX_CONTACTS; offset += sizeof(Contact)) {
        Contact contact;
        memcpy(&contact, data.data() + offset, sizeof(Contact));
        contact.name[MESH_NAME_MAX_LENGTH] = '\0';
        backend->contacts.push_back(contact);
    }
}

void saveContacts(Backend* backend) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (getPath(backend, "contacts.bin", path, sizeof(path))) {
        writeFile(path, reinterpret_cast<const uint8_t*>(backend->contacts.data()), backend->contacts.size() * sizeof(Contact));
    }
}

void addChannel(Backend* backend, uint8_t index, const char* name, const uint8_t key[16]) {
    Channel channel {};
    channel.index = index;
    strncpy(channel.name, name, MESH_NAME_MAX_LENGTH);
    memcpy(channel.key, key, 16);
    meshcore_channel_from_key(key, &channel.channel);
    std::erase_if(backend->channels, [index](const Channel& existing) { return existing.index == index; });
    backend->channels.push_back(channel);
    std::sort(backend->channels.begin(), backend->channels.end(), [](const Channel& a, const Channel& b) { return a.index < b.index; });
}

void loadChannels(Backend* backend) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    tt::std::vector<uint8_t> data;
    // Record: index (1) + name (32) + key (16)
    constexpr size_t RECORD_SIZE = 1 + MESH_NAME_MAX_LENGTH + 1 + 16;
    if (getPath(backend, "channels.bin", path, sizeof(path)) && readFile(path, data)) {
        for (size_t offset = 0; offset + RECORD_SIZE <= data.size(); offset += RECORD_SIZE) {
            char name[MESH_NAME_MAX_LENGTH + 1];
            memcpy(name, data.data() + offset + 1, sizeof(name));
            name[MESH_NAME_MAX_LENGTH] = '\0';
            addChannel(backend, data[offset], name, data.data() + offset + 1 + sizeof(name));
        }
    }
    if (backend->channels.empty()) {
        addChannel(backend, 0, "Public", MESHCORE_PUBLIC_CHANNEL_KEY);
    }
}

void saveChannels(Backend* backend) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (!getPath(backend, "channels.bin", path, sizeof(path))) {
        return;
    }
    tt::std::vector<uint8_t> data;
    for (const auto& channel : backend->channels) {
        data.push_back(channel.index);
        data.insert(data.end(), channel.name, channel.name + MESH_NAME_MAX_LENGTH + 1);
        data.insert(data.end(), channel.key, channel.key + 16);
    }
    writeFile(path, data.data(), data.size());
}

// endregion

MeshAddress toAddress(const uint8_t publicKey[MESHCORE_PUBLIC_KEY_SIZE]) {
    MeshAddress address {};
    address.length = MESHCORE_PUBLIC_KEY_SIZE;
    memcpy(address.bytes, publicKey, MESHCORE_PUBLIC_KEY_SIZE);
    return address;
}

Contact* findContact(Backend* backend, const uint8_t publicKey[MESHCORE_PUBLIC_KEY_SIZE]) {
    for (auto& contact : backend->contacts) {
        if (memcmp(contact.publicKey, publicKey, MESHCORE_PUBLIC_KEY_SIZE) == 0) {
            return &contact;
        }
    }
    return nullptr;
}

bool transmit(Backend* backend, const MeshCorePacket& packet, uint32_t seq) {
    uint8_t frame[FRAME_SIZE];
    const size_t length = meshcore_packet_serialize(packet, frame, sizeof(frame));
    if (length == 0) {
        return false;
    }
    // Our own packets come back when repeated, so remember them as seen
    backend->seenPackets.push_back(meshcore_packet_hash(packet));
    while (backend->seenPackets.size() > SEEN_PACKETS_LIMIT) {
        backend->seenPackets.pop_front();
    }
    return mesh_host_transmit(backend->host, frame, length, seq) == ERROR_NONE;
}

void sendAdvert(Backend* backend) {
    char name[MESH_NAME_MAX_LENGTH + 1];
    mesh_host_get_node_name(backend->host, name, sizeof(name));
    MeshCorePacket packet;
    if (meshcore_build_advert(backend->identity, getTimestamp(), name, &packet) && transmit(backend, packet, 0)) {
        LOG_I(TAG, "Advert sent as \"%s\"", name);
    }
    backend->lastAdvertMs = getMillis();
}

// region Receiving

bool onAdvert(Backend* backend, const MeshCorePacket& packet, float rssi, float snr) {
    MeshCoreAdvert advert;
    if (!meshcore_parse_advert(packet, &advert)) {
        LOG_D(TAG, "Dropped advert with an invalid signature");
        return false;
    }
    if (memcmp(advert.publicKey, backend->identity.publicKey, MESHCORE_PUBLIC_KEY_SIZE) == 0) {
        return true;
    }

    auto* contact = findContact(backend, advert.publicKey);
    if (contact == nullptr) {
        if (backend->contacts.size() >= MAX_CONTACTS) {
            backend->contacts.erase(backend->contacts.begin());
        }
        backend->contacts.push_back({});
        contact = &backend->contacts.back();
        memcpy(contact->publicKey, advert.publicKey, MESHCORE_PUBLIC_KEY_SIZE);
    }
    strncpy(contact->name, advert.name.c_str(), MESH_NAME_MAX_LENGTH);
    contact->name[MESH_NAME_MAX_LENGTH] = '\0';
    saveContacts(backend);

    const MeshAddress address = toAddress(advert.publicKey);
    mesh_host_advert_heard(backend->host, &address, contact->name, rssi, snr);
    return true;
}

/** @return false when no known channel decrypts the message */
bool onGroupText(Backend* backend, const MeshCorePacket& packet, float rssi, float snr) {
    for (const auto& channel : backend->channels) {
        MeshCoreText text;
        if (!meshcore_parse_group_text(channel.channel, packet, &text)) {
            continue;
        }
        if ((text.flags >> 2) != TEXT_TYPE_PLAIN) {
            return true;
        }
        // Channel messages carry the sender's name as a "name: " prefix
        tt::std::string sender;
        tt::std::string body = text.text;
        const size_t separator = text.text.find(": ");
        if (separator != tt::std::string::npos) {
            sender = text.text.substr(0, separator);
            body = text.text.substr(separator + 2);
        }
        mesh_host_receive_text(backend->host, nullptr, channel.index, sender.c_str(), body.c_str(), rssi, snr, text.timestamp);
        return true;
    }
    return false;
}

/** @return false when the message isn't for this node or its sender is unknown */
bool onTextMessage(Backend* backend, const MeshCorePacket& packet, float rssi, float snr) {
    uint8_t destinationHash;
    uint8_t sourceHash;
    if (!meshcore_peek_text_message(packet, &destinationHash, &sourceHash) || destinationHash != backend->identity.publicKey[0]) {
        return false;
    }

    // Hashes are 1 byte, so several contacts may match: the MAC tells which one sent it
    for (const auto& contact : backend->contacts) {
        if (contact.publicKey[0] != sourceHash) {
            continue;
        }
        MeshCoreText text;
        uint32_t ackHash;
        if (!meshcore_parse_text_message(backend->identity, contact.publicKey, packet, &text, &ackHash)) {
            continue;
        }

        MeshCorePacket ack;
        meshcore_build_ack(ackHash, &ack);
        transmit(backend, ack, 0);

        if ((text.flags >> 2) == TEXT_TYPE_PLAIN) {
            const MeshAddress address = toAddress(contact.publicKey);
            mesh_host_node_heard(backend->host, &address, nullptr, rssi, snr);
            mesh_host_receive_text(backend->host, &address, 0, contact.name, text.text.c_str(), rssi, snr, text.timestamp);
        }
        return true;
    }
    LOG_D(TAG, "Text message from an unknown sender (hash %02X)", sourceHash);
    return false;
}

bool onAck(Backend* backend, const MeshCorePacket& packet) {
    uint32_t ackHash;
    if (!meshcore_parse_ack(packet, &ackHash)) {
        return false;
    }
    auto iterator = backend->pendingAcks.find(ackHash);
    if (iterator != backend->pendingAcks.end()) {
        mesh_host_set_delivery(backend->host, iterator->second.seq, MESH_DELIVERY_DELIVERED);
        backend->pendingAcks.erase(iterator);
    }
    return true;
}

// endregion

// region MeshBackendApi

void* create(MeshHost* host) {
    auto* backend = tt::newObject<Backend>();
    if (backend == nullptr) {
        return nullptr;
    }
    backend->host = host;
    loadOrCreateIdentity(backend);
    loadContacts(backend);
    loadChannels(backend);
    return backend;
}

void destroy(void* context) {
    tt::deleteObject(static_cast<Backend*>(context));
}

constexpr size_t PRESET_DEFAULT = 0;
constexpr size_t PRESET_EU_NL = 1;

const char* const presets[] = { "Default", "EU NL", nullptr };

error_t getRadioProfile(void*, RadioRegion region, size_t preset, MeshRadioProfile* profile) {
    // MeshCore's recommended presets. Sync word 0x12 is LoRa's private network sync word.
    switch (region) {
        case RADIO_REGION_EU_868:
            *profile = {
                .modulation = RADIO_MODULATION_LORA,
                .frequency = 869618000,
                .bandwidth = 62500,
                .spreading_factor = (preset == PRESET_EU_NL) ? 7 : 8,
                .coding_rate = (preset == PRESET_EU_NL) ? 5 : 8,
                .sync_word = 0x12,
                .preamble_length = 16,
                .power = 22,
            };
            return ERROR_NONE;
        case RADIO_REGION_US_915:
            if (preset != PRESET_DEFAULT) {
                return ERROR_NOT_SUPPORTED;
            }
            *profile = {
                .modulation = RADIO_MODULATION_LORA,
                .frequency = 910525000,
                .bandwidth = 62500,
                .spreading_factor = 7,
                .coding_rate = 5,
                .sync_word = 0x12,
                .preamble_length = 16,
                .power = 22,
            };
            return ERROR_NONE;
        case RADIO_REGION_AU_915:
            if (preset != PRESET_DEFAULT) {
                return ERROR_NOT_SUPPORTED;
            }
            *profile = {
                .modulation = RADIO_MODULATION_LORA,
                .frequency = 915800000,
                .bandwidth = 250000,
                .spreading_factor = 10,
                .coding_rate = 5,
                .sync_word = 0x12,
                .preamble_length = 16,
                .power = 22,
            };
            return ERROR_NONE;
        default:
            return ERROR_NOT_SUPPORTED;
    }
}

void onStart(void* context) {
    sendAdvert(static_cast<Backend*>(context));
}

bool onReceive(void* context, const uint8_t* data, size_t length, float rssi, float snr) {
    auto* backend = static_cast<Backend*>(context);
    MeshCorePacket packet;
    if (!meshcore_packet_parse(data, length, &packet)) {
        return false;
    }

    const uint32_t hash = meshcore_packet_hash(packet);
    if (std::find(backend->seenPackets.begin(), backend->seenPackets.end(), hash) != backend->seenPackets.end()) {
        return true;
    }
    backend->seenPackets.push_back(hash);
    while (backend->seenPackets.size() > SEEN_PACKETS_LIMIT) {
        backend->seenPackets.pop_front();
    }

    switch (packet.payloadType) {
        case MeshCorePayload::Advert:
            return onAdvert(backend, packet, rssi, snr);
        case MeshCorePayload::GroupText:
            return onGroupText(backend, packet, rssi, snr);
        case MeshCorePayload::TextMessage:
            return onTextMessage(backend, packet, rssi, snr);
        case MeshCorePayload::Ack:
            return onAck(backend, packet);
        default:
            // Payload types this backend doesn't support, e.g. path and trace packets
            return false;
    }
}

void onTick(void* context) {
    auto* backend = static_cast<Backend*>(context);
    const uint64_t nowMs = getMillis();
    if (nowMs - backend->lastAdvertMs >= ADVERT_INTERVAL_MS) {
        sendAdvert(backend);
    }
    for (auto iterator = backend->pendingAcks.begin(); iterator != backend->pendingAcks.end();) {
        if (nowMs >= iterator->second.deadlineMs) {
            // Sent but never acknowledged: it stays "sent"
            iterator = backend->pendingAcks.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

error_t sendText(void* context, uint32_t seq, const MeshAddress* to, uint8_t channelIndex, const char* text) {
    auto* backend = static_cast<Backend*>(context);
    MeshCorePacket packet;

    if (to == nullptr) {
        for (const auto& channel : backend->channels) {
            if (channel.index != channelIndex) {
                continue;
            }
            char name[MESH_NAME_MAX_LENGTH + 1];
            mesh_host_get_node_name(backend->host, name, sizeof(name));
            if (!meshcore_build_group_text(channel.channel, getTimestamp(), name, text, &packet)) {
                return ERROR_OUT_OF_RANGE;
            }
            return transmit(backend, packet, seq) ? ERROR_NONE : ERROR_RESOURCE;
        }
        return ERROR_NOT_FOUND;
    }

    if (to->length != MESHCORE_PUBLIC_KEY_SIZE) {
        return ERROR_INVALID_ARGUMENT;
    }
    uint32_t ackHash;
    if (!meshcore_build_text_message(backend->identity, to->bytes, getTimestamp(), 0, text, &packet, &ackHash)) {
        return ERROR_OUT_OF_RANGE;
    }
    if (!transmit(backend, packet, seq)) {
        return ERROR_RESOURCE;
    }
    backend->pendingAcks[ackHash] = { .seq = seq, .deadlineMs = getMillis() + ACK_TIMEOUT_MS };
    return ERROR_NONE;
}

error_t getChannels(void* context, MeshChannel* channels, size_t maxCount, size_t* count) {
    auto* backend = static_cast<Backend*>(context);
    *count = 0;
    for (const auto& channel : backend->channels) {
        if (*count >= maxCount) {
            break;
        }
        MeshChannel& out = channels[(*count)++];
        out = {};
        out.index = channel.index;
        strncpy(out.name, channel.name, MESH_NAME_MAX_LENGTH);
        memcpy(out.key, channel.key, 16);
        out.key_length = 16;
    }
    return ERROR_NONE;
}

error_t setChannel(void* context, const MeshChannel* channel) {
    auto* backend = static_cast<Backend*>(context);
    uint8_t key[16];
    if (channel->key_length == 16) {
        memcpy(key, channel->key, sizeof(key));
    } else if (channel->key_length == 0 && channel->name[0] == '#') {
        meshcore_hashtag_channel_key(channel->name, key);
    } else {
        return ERROR_INVALID_ARGUMENT;
    }
    const bool exists = std::any_of(backend->channels.begin(), backend->channels.end(), [channel](const Channel& existing) {
        return existing.index == channel->index;
    });
    if (!exists && backend->channels.size() >= MAX_CHANNELS) {
        return ERROR_RESOURCE;
    }
    addChannel(backend, channel->index, channel->name, key);
    saveChannels(backend);
    return ERROR_NONE;
}

// endregion

} // namespace

const MeshBackendApi meshcore_backend_api = {
    .id = "meshcore",
    .create = create,
    .destroy = destroy,
    .presets = presets,
    .get_radio_profile = getRadioProfile,
    .on_start = onStart,
    .on_receive = onReceive,
    .on_tick = onTick,
    .send_text = sendText,
    .get_channels = getChannels,
    .set_channel = setChannel,
};

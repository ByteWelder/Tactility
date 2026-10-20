// SPDX-License-Identifier: Apache-2.0
//
// Filesystem-backed NimBLE security store.
//
// NimBLE's stock store (store/config) keeps its records in RAM and persists them to
// NVS. A bond that goes missing in NVS makes the stack treat a bonded peer as unbonded,
// which fires BT_PAIR_RESULT_BOND_LOST and tears down the Tactility-side paired-device
// record - so BLE auto-reconnect permanently loses its target. This implementation keeps
// the same in-RAM database and the same key-matching/iteration semantics, but persists
// the records to one file on the data partition (/data, FAT) instead of NVS.
//
// Install with ble_store_fs_init() in place of ble_store_config_init(). Only the three
// ble_hs_cfg.store_*_cb callbacks are replaced; ble_store_read()/write()/delete() and
// ble_store_iterate() all dispatch through those, so they keep working unchanged.

#include <sdkconfig.h>

#if defined(CONFIG_BT_NIMBLE_ENABLED)

// tactility/log.h must precede the NimBLE headers: NimBLE's log_common.h defines
// LOG_LEVEL_* as macros with the same names as this header's LogLevel enum.
#include <tactility/log.h>
#include <tactility/paths.h>

#include <bluetooth/ble_store_fs.h>

#include <syscfg/syscfg.h>
#include <host/ble_hs.h>
#include <host/ble_store.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

namespace {

constexpr auto* TAG = "BleStoreFs";

constexpr uint32_t STORE_MAGIC   = 0x424c5346; // "BLSF"
constexpr uint32_t STORE_VERSION = 1;

// Capacities mirror IDF's store/config (see esp_nimble_cfg.h). The "> 0 ? ... : 1" keeps
// the arrays well-formed on a target where a section is compiled out.
constexpr size_t MAX_BONDS = MYNEWT_VAL(BLE_STORE_MAX_BONDS) > 0 ? MYNEWT_VAL(BLE_STORE_MAX_BONDS) : 1;
constexpr size_t MAX_CCCDS = MYNEWT_VAL(BLE_STORE_MAX_CCCDS) > 0 ? MYNEWT_VAL(BLE_STORE_MAX_CCCDS) : 1;
constexpr size_t MAX_CSFCS = MYNEWT_VAL(BLE_STORE_MAX_CSFCS) > 0 ? MYNEWT_VAL(BLE_STORE_MAX_CSFCS) : 1;

// Sections in file order.
constexpr size_t SECTION_COUNT = 6;

struct Database {
    size_t num_our_secs = 0;
    ble_store_value_sec our_secs[MAX_BONDS] = {};
    size_t num_peer_secs = 0;
    ble_store_value_sec peer_secs[MAX_BONDS] = {};
    size_t num_cccds = 0;
    ble_store_value_cccd cccds[MAX_CCCDS] = {};
    size_t num_csfcs = 0;
    ble_store_value_csfc csfcs[MAX_CSFCS] = {};
    size_t num_local_irks = 0;
    ble_store_value_local_irk local_irks[MAX_BONDS] = {};
    size_t num_rpa_recs = 0;
    ble_store_value_rpa_rec rpa_recs[MAX_BONDS] = {};
};

// Only ever touched under the NimBLE host lock: ble_store_read()/write()/delete() take it
// around each callback, and ble_store_fs_init() runs before the host is up.
Database db;

struct FileHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t sec_size;
    uint32_t cccd_size;
    uint32_t csfc_size;
    uint32_t irk_size;
    uint32_t rpa_size;
    uint32_t counts[SECTION_COUNT]; // our_secs, peer_secs, cccds, csfcs, local_irks, rpa_recs
};

std::string storeDir() {
    char data_path[FILE_MAX_PATH_STRING_LENGTH];
    if (paths_get_data_path(data_path, sizeof(data_path)) != ERROR_NONE) {
        return {};
    }
    return std::string(data_path) + "/bluetooth";
}

std::string storePath() {
    const std::string dir = storeDir();
    return dir.empty() ? std::string() : dir + "/ble_store.bin";
}

// Creates each level; mkdir() failing with EEXIST means it is already there.
bool ensureStoreDir(const std::string& dir) {
    const size_t separator = dir.rfind('/');
    if (separator == std::string::npos) {
        return false;
    }
    // The data path itself ("<root>/tactility") may not exist yet on a fresh device.
    const std::string parent = dir.substr(0, separator);
    ::mkdir(parent.c_str(), 0755);
    if (::mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
        LOG_E(TAG, "Failed to create directory %s", dir.c_str());
        return false;
    }
    return true;
}

// ---- Generic array helpers ----

template <typename T>
bool writeArray(FILE* file, const T* items, size_t count) {
    return count == 0 || fwrite(items, sizeof(T), count, file) == count;
}

template <typename T>
bool readArray(FILE* file, T* items, size_t count) {
    return count == 0 || fread(items, sizeof(T), count, file) == count;
}

template <typename T>
void removeAt(T* items, size_t& count, size_t index) {
    if (index + 1 < count) {
        std::memmove(&items[index], &items[index + 1], (count - index - 1) * sizeof(T));
    }
    count--;
}

size_t clampCount(uint32_t value, size_t max) {
    return value > max ? max : value;
}

// ---- Persistence ----

bool persist() {
    const std::string dir = storeDir();
    if (!ensureStoreDir(dir)) {
        return false;
    }

    const std::string path = storePath();
    if (path.empty()) {
        return false;
    }
    const std::string temp_path = path + ".tmp";

    FILE* file = fopen(temp_path.c_str(), "wb");
    if (file == nullptr) {
        LOG_E(TAG, "Failed to open %s for writing", temp_path.c_str());
        return false;
    }

    FileHeader header = {};
    header.magic      = STORE_MAGIC;
    header.version    = STORE_VERSION;
    header.sec_size   = sizeof(ble_store_value_sec);
    header.cccd_size  = sizeof(ble_store_value_cccd);
    header.csfc_size  = sizeof(ble_store_value_csfc);
    header.irk_size   = sizeof(ble_store_value_local_irk);
    header.rpa_size   = sizeof(ble_store_value_rpa_rec);
    header.counts[0]  = static_cast<uint32_t>(db.num_our_secs);
    header.counts[1]  = static_cast<uint32_t>(db.num_peer_secs);
    header.counts[2]  = static_cast<uint32_t>(db.num_cccds);
    header.counts[3]  = static_cast<uint32_t>(db.num_csfcs);
    header.counts[4]  = static_cast<uint32_t>(db.num_local_irks);
    header.counts[5]  = static_cast<uint32_t>(db.num_rpa_recs);

    bool ok = fwrite(&header, sizeof(header), 1, file) == 1;
    ok = ok && writeArray(file, db.our_secs, db.num_our_secs);
    ok = ok && writeArray(file, db.peer_secs, db.num_peer_secs);
    ok = ok && writeArray(file, db.cccds, db.num_cccds);
    ok = ok && writeArray(file, db.csfcs, db.num_csfcs);
    ok = ok && writeArray(file, db.local_irks, db.num_local_irks);
    ok = ok && writeArray(file, db.rpa_recs, db.num_rpa_recs);

    if (fclose(file) != 0) {
        ok = false;
    }
    if (!ok) {
        LOG_E(TAG, "Failed to write %s", temp_path.c_str());
        std::remove(temp_path.c_str());
        return false;
    }

    // Temp file + rename: a power loss mid-write can never leave a half-written store.
    if (std::rename(temp_path.c_str(), path.c_str()) != 0) {
        LOG_E(TAG, "Failed to replace %s", path.c_str());
        std::remove(temp_path.c_str());
        return false;
    }
    return true;
}

void load() {
    db = Database {};

    const std::string path = storePath();
    FILE* file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return; // first boot / nothing paired yet
    }

    FileHeader header = {};
    bool ok = fread(&header, sizeof(header), 1, file) == 1;
    ok = ok && header.magic == STORE_MAGIC && header.version == STORE_VERSION;
    ok = ok && header.sec_size  == sizeof(ble_store_value_sec);
    ok = ok && header.cccd_size == sizeof(ble_store_value_cccd);
    ok = ok && header.csfc_size == sizeof(ble_store_value_csfc);
    ok = ok && header.irk_size  == sizeof(ble_store_value_local_irk);
    ok = ok && header.rpa_size  == sizeof(ble_store_value_rpa_rec);

    if (ok) {
        db.num_our_secs    = clampCount(header.counts[0], MAX_BONDS);
        db.num_peer_secs   = clampCount(header.counts[1], MAX_BONDS);
        db.num_cccds       = clampCount(header.counts[2], MAX_CCCDS);
        db.num_csfcs       = clampCount(header.counts[3], MAX_CSFCS);
        db.num_local_irks  = clampCount(header.counts[4], MAX_BONDS);
        db.num_rpa_recs    = clampCount(header.counts[5], MAX_BONDS);

        ok = readArray(file, db.our_secs, db.num_our_secs);
        ok = ok && readArray(file, db.peer_secs, db.num_peer_secs);
        ok = ok && readArray(file, db.cccds, db.num_cccds);
        ok = ok && readArray(file, db.csfcs, db.num_csfcs);
        ok = ok && readArray(file, db.local_irks, db.num_local_irks);
        ok = ok && readArray(file, db.rpa_recs, db.num_rpa_recs);
    }

    fclose(file);

    if (!ok) {
        LOG_W(TAG, "Store file %s is unreadable or from an incompatible build - starting empty", path.c_str());
        db = Database {};
    }
}

// BLE_ADDR_ANY is a C compound literal (&(ble_addr_t){...}), which is an rvalue in C++, so
// C++ code cannot take its address. Use a real object with the same value instead.
const ble_addr_t ANY_ADDR = { BLE_ADDR_PUBLIC, { 0, 0, 0, 0, 0, 0 } };

// ---- Key matching (mirrors store/config's ble_store_config_find_*) ----
//
// read() must honour key.idx by skipping that many matches, because ble_store_iterate()
// walks a type by bumping idx from 0 until read() returns ENOENT.

int findSec(const ble_store_key_sec& key, const ble_store_value_sec* values, size_t count) {
    int skipped = 0;
    for (size_t i = 0; i < count; i++) {
        if (ble_addr_cmp(&key.peer_addr, &ANY_ADDR) != 0 &&
            ble_addr_cmp(&values[i].peer_addr, &key.peer_addr) != 0) {
            continue;
        }
        if (key.idx > skipped) {
            skipped++;
            continue;
        }
        return static_cast<int>(i);
    }
    return -1;
}

int findCccd(const ble_store_key_cccd& key, const ble_store_value_cccd* values, size_t count) {
    int skipped = 0;
    for (size_t i = 0; i < count; i++) {
        if (ble_addr_cmp(&key.peer_addr, &ANY_ADDR) != 0 &&
            ble_addr_cmp(&values[i].peer_addr, &key.peer_addr) != 0) {
            continue;
        }
        if (key.chr_val_handle != 0 && values[i].chr_val_handle != key.chr_val_handle) {
            continue;
        }
        if (key.idx > skipped) {
            skipped++;
            continue;
        }
        return static_cast<int>(i);
    }
    return -1;
}

int findCsfc(const ble_store_key_csfc& key, const ble_store_value_csfc* values, size_t count) {
    if (ble_addr_cmp(&key.peer_addr, &ANY_ADDR) != 0) {
        for (size_t i = 0; i < count; i++) {
            if (ble_addr_cmp(&values[i].peer_addr, &key.peer_addr) == 0) {
                return static_cast<int>(i);
            }
        }
    } else if (key.idx < count) {
        // Enumerate: idx is used as a direct index when the peer is unspecified.
        return static_cast<int>(key.idx);
    }
    return -1;
}

int findLocalIrk(const ble_store_key_local_irk& key, const ble_store_value_local_irk* values, size_t count) {
    int skipped = 0;
    for (size_t i = 0; i < count; i++) {
        if (ble_addr_cmp(&key.addr, &ANY_ADDR) != 0 &&
            ble_addr_cmp(&values[i].addr, &key.addr) != 0) {
            continue;
        }
        if (key.idx > skipped) {
            skipped++;
            continue;
        }
        return static_cast<int>(i);
    }
    return -1;
}

int findRpaRec(const ble_store_key_rpa_rec& key, const ble_store_value_rpa_rec* values, size_t count) {
    int skipped = 0;
    for (size_t i = 0; i < count; i++) {
        // Matches when the key names either the RPA or the resolved identity address.
        if (ble_addr_cmp(&values[i].peer_rpa_addr, &key.peer_rpa_addr) != 0 &&
            ble_addr_cmp(&values[i].peer_addr, &key.peer_rpa_addr) != 0) {
            continue;
        }
        if (key.idx > skipped) {
            skipped++;
            continue;
        }
        return static_cast<int>(i);
    }
    return -1;
}

// ---- Write / erase primitives ----

template <typename T>
int upsert(T* values, size_t& count, size_t max, int index, const T& value) {
    if (index < 0) {
        if (count >= max) {
            return BLE_HS_ESTORE_CAP;
        }
        index = static_cast<int>(count);
        count++;
    }
    values[index] = value;
    return persist() ? 0 : BLE_HS_ESTORE_FAIL;
}

template <typename T>
int erase(T* values, size_t& count, int index) {
    if (index < 0) {
        return BLE_HS_ENOENT;
    }
    removeAt(values, count, static_cast<size_t>(index));
    return persist() ? 0 : BLE_HS_ESTORE_FAIL;
}

// ---- Store backend ----

int fsRead(int obj_type, const union ble_store_key* key, union ble_store_value* value) {
    switch (obj_type) {
        case BLE_STORE_OBJ_TYPE_PEER_SEC: {
            int index = findSec(key->sec, db.peer_secs, db.num_peer_secs);
            if (index < 0) return BLE_HS_ENOENT;
            value->sec = db.peer_secs[index];
            return 0;
        }
        case BLE_STORE_OBJ_TYPE_OUR_SEC: {
            int index = findSec(key->sec, db.our_secs, db.num_our_secs);
            if (index < 0) return BLE_HS_ENOENT;
            value->sec = db.our_secs[index];
            return 0;
        }
        case BLE_STORE_OBJ_TYPE_CCCD: {
            int index = findCccd(key->cccd, db.cccds, db.num_cccds);
            if (index < 0) return BLE_HS_ENOENT;
            value->cccd = db.cccds[index];
            return 0;
        }
        case BLE_STORE_OBJ_TYPE_CSFC: {
            int index = findCsfc(key->csfc, db.csfcs, db.num_csfcs);
            if (index < 0) return BLE_HS_ENOENT;
            value->csfc = db.csfcs[index];
            return 0;
        }
        case BLE_STORE_OBJ_TYPE_PEER_ADDR: {
            int index = findRpaRec(key->rpa_rec, db.rpa_recs, db.num_rpa_recs);
            if (index < 0) return BLE_HS_ENOENT;
            value->rpa_rec = db.rpa_recs[index];
            return 0;
        }
        case BLE_STORE_OBJ_TYPE_LOCAL_IRK: {
            int index = findLocalIrk(key->local_irk, db.local_irks, db.num_local_irks);
            if (index < 0) return BLE_HS_ENOENT;
            value->local_irk = db.local_irks[index];
            return 0;
        }
        default:
            return BLE_HS_ENOTSUP;
    }
}

int fsWrite(int obj_type, const union ble_store_value* val) {
    switch (obj_type) {
        case BLE_STORE_OBJ_TYPE_PEER_SEC: {
            ble_store_key_sec key;
            ble_store_key_from_value_sec(&key, &val->sec);
            return upsert(db.peer_secs, db.num_peer_secs, MAX_BONDS,
                          findSec(key, db.peer_secs, db.num_peer_secs), val->sec);
        }
        case BLE_STORE_OBJ_TYPE_OUR_SEC: {
            ble_store_key_sec key;
            ble_store_key_from_value_sec(&key, &val->sec);
            return upsert(db.our_secs, db.num_our_secs, MAX_BONDS,
                          findSec(key, db.our_secs, db.num_our_secs), val->sec);
        }
        case BLE_STORE_OBJ_TYPE_CCCD: {
            ble_store_key_cccd key;
            ble_store_key_from_value_cccd(&key, &val->cccd);
            return upsert(db.cccds, db.num_cccds, MAX_CCCDS,
                          findCccd(key, db.cccds, db.num_cccds), val->cccd);
        }
        case BLE_STORE_OBJ_TYPE_CSFC: {
            ble_store_key_csfc key;
            ble_store_key_from_value_csfc(&key, &val->csfc);
            return upsert(db.csfcs, db.num_csfcs, MAX_CSFCS,
                          findCsfc(key, db.csfcs, db.num_csfcs), val->csfc);
        }
        case BLE_STORE_OBJ_TYPE_PEER_ADDR: {
            ble_store_key_rpa_rec key;
            ble_store_key_from_value_rpa_rec(&key, &val->rpa_rec);
            return upsert(db.rpa_recs, db.num_rpa_recs, MAX_BONDS,
                          findRpaRec(key, db.rpa_recs, db.num_rpa_recs), val->rpa_rec);
        }
        case BLE_STORE_OBJ_TYPE_LOCAL_IRK: {
            ble_store_key_local_irk key;
            ble_store_key_from_value_local_irk(&key, &val->local_irk);
            return upsert(db.local_irks, db.num_local_irks, MAX_BONDS,
                          findLocalIrk(key, db.local_irks, db.num_local_irks), val->local_irk);
        }
        default:
            return BLE_HS_ENOTSUP;
    }
}

int fsDelete(int obj_type, const union ble_store_key* key) {
    switch (obj_type) {
        case BLE_STORE_OBJ_TYPE_PEER_SEC:
            return erase(db.peer_secs, db.num_peer_secs, findSec(key->sec, db.peer_secs, db.num_peer_secs));
        case BLE_STORE_OBJ_TYPE_OUR_SEC:
            return erase(db.our_secs, db.num_our_secs, findSec(key->sec, db.our_secs, db.num_our_secs));
        case BLE_STORE_OBJ_TYPE_CCCD:
            return erase(db.cccds, db.num_cccds, findCccd(key->cccd, db.cccds, db.num_cccds));
        case BLE_STORE_OBJ_TYPE_CSFC:
            return erase(db.csfcs, db.num_csfcs, findCsfc(key->csfc, db.csfcs, db.num_csfcs));
        case BLE_STORE_OBJ_TYPE_PEER_ADDR:
            return erase(db.rpa_recs, db.num_rpa_recs, findRpaRec(key->rpa_rec, db.rpa_recs, db.num_rpa_recs));
        case BLE_STORE_OBJ_TYPE_LOCAL_IRK:
            return erase(db.local_irks, db.num_local_irks, findLocalIrk(key->local_irk, db.local_irks, db.num_local_irks));
        default:
            return BLE_HS_ENOTSUP;
    }
}

} // namespace

extern "C" {

static int ble_store_fs_read(int obj_type, const union ble_store_key* key, union ble_store_value* value) {
    return fsRead(obj_type, key, value);
}

static int ble_store_fs_write(int obj_type, const union ble_store_value* val) {
    return fsWrite(obj_type, val);
}

static int ble_store_fs_delete(int obj_type, const union ble_store_key* key) {
    return fsDelete(obj_type, key);
}

void ble_store_fs_init(void) {
    load();

    ble_hs_cfg.store_read_cb   = ble_store_fs_read;
    ble_hs_cfg.store_write_cb  = ble_store_fs_write;
    ble_hs_cfg.store_delete_cb = ble_store_fs_delete;

    LOG_I(TAG, "Installed filesystem store (%s)", storePath().c_str());
}

} // extern "C"

#endif // CONFIG_BT_NIMBLE_ENABLED

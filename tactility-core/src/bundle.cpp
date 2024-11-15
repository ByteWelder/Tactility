#include "bundle.h"

#include "m-dict.h"
#include "m_cstr_dup.h"
#include "check.h"

// region BundleEntry

typedef enum {
    BundleEntryTypeBool,
    BundleEntryTypeInt32,
    BundleEntryTypeString,
} BundleEntryType;

typedef struct {
    BundleEntryType type;
    union {
        bool bool_value;
        int32_t int32_value;
        char* string_ptr;
    };
} BundleEntry;

BundleEntry* bundle_entry_alloc_bool(bool value) {
    auto* entry = static_cast<BundleEntry*>(malloc(sizeof(BundleEntry)));
    entry->type = BundleEntryTypeBool;
    entry->bool_value = value;
    return entry;
}

BundleEntry* bundle_entry_alloc_int32(int32_t value) {
    auto* entry = static_cast<BundleEntry*>(malloc(sizeof(BundleEntry)));
    entry->type = BundleEntryTypeInt32;
    entry->int32_value = value;
    return entry;
}

BundleEntry* bundle_entry_alloc_string(const char* text) {
    auto* entry = static_cast<BundleEntry*>(malloc(sizeof(BundleEntry)));
    entry->type = BundleEntryTypeString;
    entry->string_ptr = static_cast<char*>(malloc(strlen(text) + 1));
    strcpy(entry->string_ptr, text);
    return entry;
}

BundleEntry* bundle_entry_alloc_copy(BundleEntry* source) {
    auto* entry = static_cast<BundleEntry*>(malloc(sizeof(BundleEntry)));
    entry->type = source->type;
    if (source->type == BundleEntryTypeString) {
        entry->string_ptr = static_cast<char*>(malloc(strlen(source->string_ptr) + 1));
        strcpy(entry->string_ptr, source->string_ptr);
    } else {
        entry->int32_value = source->int32_value;
    }
    return entry;
}

void bundle_entry_free(BundleEntry* entry) {
    if (entry->type == BundleEntryTypeString) {
        free(entry->string_ptr);
    }
    free(entry);
}

// endregion BundleEntry

// region Bundle

// From https://stackoverflow.com/a/40766163/3848666
// TODO: Remove
char* strdup(const char* s) {
    size_t slen = strlen(s);
    auto* result = static_cast<char*>(malloc(slen + 1));
    if (result == nullptr) {
        return nullptr;
    }
    memcpy(result, s, slen + 1);
    return result;
}

DICT_DEF2(BundleDict, const char*, M_CSTR_DUP_OPLIST, BundleEntry*, M_PTR_OPLIST)

typedef struct {
    BundleDict_t dict;
} BundleData;

Bundle tt_bundle_alloc() {
    auto* bundle = static_cast<BundleData*>(malloc(sizeof(BundleData)));
    BundleDict_init(bundle->dict);
    return bundle;
}

Bundle tt_bundle_alloc_copy(Bundle source) {
    auto* source_data = static_cast<BundleData*>(source);
    auto* target_data = static_cast<BundleData*>(tt_bundle_alloc());

    BundleDict_it_t it;
    for (BundleDict_it(it, source_data->dict); !BundleDict_end_p(it); BundleDict_next(it)) {
        const char* key = BundleDict_cref(it)->key;
        BundleEntry* entry = BundleDict_cref(it)->value;
        BundleEntry* entry_copy = bundle_entry_alloc_copy(entry);
        BundleDict_set_at(target_data->dict, key, entry_copy);
    }

    return target_data;
}

void tt_bundle_free(Bundle bundle) {
    auto* data = static_cast<BundleData*>(bundle);

    BundleDict_it_t it;
    for (BundleDict_it(it, data->dict); !BundleDict_end_p(it); BundleDict_next(it)) {
        bundle_entry_free(BundleDict_cref(it)->value);
    }

    BundleDict_clear(data->dict);
    free(data);
}

bool tt_bundle_get_bool(Bundle bundle, const char* key) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry = BundleDict_get(data->dict, key);
    tt_check(entry != nullptr);
    return (*entry)->bool_value;
}

int32_t tt_bundle_get_int32(Bundle bundle, const char* key) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry = BundleDict_get(data->dict, key);
    tt_check(entry != nullptr);
    return (*entry)->int32_value;
}

const char* tt_bundle_get_string(Bundle bundle, const char* key) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry = BundleDict_get(data->dict, key);
    tt_check(entry != nullptr);
    return (*entry)->string_ptr;
}

bool tt_bundle_has_bool(Bundle bundle, const char* key) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry = BundleDict_get(data->dict, key);
    return (entry != nullptr) && ((*entry)->type == BundleEntryTypeBool);
}

bool tt_bundle_has_int32(Bundle bundle, const char* key) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry = BundleDict_get(data->dict, key);
    return (entry != nullptr) && ((*entry)->type == BundleEntryTypeInt32);
}

bool tt_bundle_has_string(Bundle bundle, const char* key) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry = BundleDict_get(data->dict, key);
    return (entry != nullptr) && ((*entry)->type == BundleEntryTypeString);
}

bool tt_bundle_opt_bool(Bundle bundle, const char* key, bool* out) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry_ptr = BundleDict_get(data->dict, key);
    if (entry_ptr != nullptr) {
        BundleEntry* entry = *entry_ptr;
        if (entry->type == BundleEntryTypeBool) {
            *out = entry->bool_value;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool tt_bundle_opt_int32(Bundle bundle, const char* key, int32_t* out) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry_ptr = BundleDict_get(data->dict, key);
    if (entry_ptr != nullptr) {
        BundleEntry* entry = *entry_ptr;
        if (entry->type == BundleEntryTypeInt32) {
            *out = entry->int32_value;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool tt_bundle_opt_string(Bundle bundle, const char* key, char** out) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry_ptr = BundleDict_get(data->dict, key);
    if (entry_ptr != nullptr) {
        BundleEntry* entry = *entry_ptr;
        if (entry->type == BundleEntryTypeString) {
            *out = entry->string_ptr;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void tt_bundle_put_bool(Bundle bundle, const char* key, bool value) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry_handle = BundleDict_get(data->dict, key);
    if (entry_handle != nullptr) {
        BundleEntry* entry = *entry_handle;
        tt_assert(entry->type == BundleEntryTypeBool);
        entry->bool_value = value;
    } else {
        BundleEntry* entry = bundle_entry_alloc_bool(value);
        BundleDict_set_at(data->dict, key, entry);
    }
}

void tt_bundle_put_int32(Bundle bundle, const char* key, int32_t value) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry_handle = BundleDict_get(data->dict, key);
    if (entry_handle != nullptr) {
        BundleEntry* entry = *entry_handle;
        tt_assert(entry->type == BundleEntryTypeInt32);
        entry->int32_value = value;
    } else {
        BundleEntry* entry = bundle_entry_alloc_int32(value);
        BundleDict_set_at(data->dict, key, entry);
    }
}

void tt_bundle_put_string(Bundle bundle, const char* key, const char* value) {
    auto* data = static_cast<BundleData*>(bundle);
    BundleEntry** entry_handle = BundleDict_get(data->dict, key);
    if (entry_handle != nullptr) {
        BundleEntry* entry = *entry_handle;
        tt_assert(entry->type == BundleEntryTypeString);
        if (entry->string_ptr != nullptr) {
            free(entry->string_ptr);
        }
        entry->string_ptr = static_cast<char*>(malloc(strlen(value) + 1));
        strcpy(entry->string_ptr, value);
    } else {
        BundleEntry* entry = bundle_entry_alloc_string(value);
        BundleDict_set_at(data->dict, key, entry);
    }
}

// endregion Bundle
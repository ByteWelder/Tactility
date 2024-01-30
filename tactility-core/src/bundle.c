#include "bundle.h"

#include "m-dict.h"
#include "m_cstr_dup.h"
#include "check.h"

// region BundleEntry

typedef enum {
    BUNDLE_ENTRY_TYPE_BOOL,
    BUNDLE_ENTRY_TYPE_INT32,
    BUNDLE_ENTRY_TYPE_STRING,
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
    BundleEntry* entry = malloc(sizeof(BundleEntry));
    entry->type = BUNDLE_ENTRY_TYPE_BOOL;
    entry->bool_value = value;
    return entry;
}

BundleEntry* bundle_entry_alloc_int32(int32_t value) {
    BundleEntry* entry = malloc(sizeof(BundleEntry));
    entry->type = BUNDLE_ENTRY_TYPE_INT32;
    entry->int32_value = value;
    return entry;
}

BundleEntry* bundle_entry_alloc_string(const char* text) {
    BundleEntry* entry = malloc(sizeof(BundleEntry));
    entry->type = BUNDLE_ENTRY_TYPE_STRING;
    entry->string_ptr = malloc(strlen(text) + 1);
    strcpy(entry->string_ptr, text);
    return entry;
}

BundleEntry* bundle_entry_alloc_copy(BundleEntry* source) {
    BundleEntry* entry = malloc(sizeof(BundleEntry));
    entry->type = source->type;
    if (source->type == BUNDLE_ENTRY_TYPE_STRING) {
        entry->string_ptr = malloc(strlen(source->string_ptr) + 1);
        strcpy(entry->string_ptr, source->string_ptr);
    } else {
        entry->int32_value = source->int32_value;
    }
    return entry;
}

void bundle_entry_free(BundleEntry* entry) {
    if (entry->type == BUNDLE_ENTRY_TYPE_STRING) {
        free(entry->string_ptr);
    }
    free(entry);
}

// endregion BundleEntry

// region Bundle

DICT_DEF2(BundleDict, const char*, M_CSTR_DUP_OPLIST, BundleEntry*, M_PTR_OPLIST)

typedef struct {
    BundleDict_t dict;
} BundleData;

Bundle tt_bundle_alloc() {
    BundleData* bundle = malloc(sizeof(BundleData));
    BundleDict_init(bundle->dict);
    return bundle;
}

Bundle tt_bundle_alloc_copy(Bundle source) {
    BundleData* source_data = (BundleData*)source;
    BundleData* target_data = tt_bundle_alloc();

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
    BundleData* data = (BundleData*)bundle;

    BundleDict_it_t it;
    for (BundleDict_it(it, data->dict); !BundleDict_end_p(it); BundleDict_next(it)) {
        bundle_entry_free(BundleDict_cref(it)->value);
    }

    BundleDict_clear(data->dict);
    free(data);
}

bool tt_bundle_get_bool(Bundle bundle, const char* key) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry = BundleDict_get(data->dict, key);
    tt_check(entry != NULL);
    return (*entry)->bool_value;
}

int32_t tt_bundle_get_int32(Bundle bundle, const char* key) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry = BundleDict_get(data->dict, key);
    tt_check(entry != NULL);
    return (*entry)->int32_value;
}

const char* tt_bundle_get_string(Bundle bundle, const char* key) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry = BundleDict_get(data->dict, key);
    tt_check(entry != NULL);
    return (*entry)->string_ptr;
}

bool tt_bundle_has_bool(Bundle bundle, const char* key) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry = BundleDict_get(data->dict, key);
    return (entry != NULL) && ((*entry)->type == BUNDLE_ENTRY_TYPE_BOOL);
}

bool tt_bundle_has_int32(Bundle bundle, const char* key) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry = BundleDict_get(data->dict, key);
    return (entry != NULL) && ((*entry)->type == BUNDLE_ENTRY_TYPE_INT32);
}

bool tt_bundle_has_string(Bundle bundle, const char* key) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry = BundleDict_get(data->dict, key);
    return (entry != NULL) && ((*entry)->type == BUNDLE_ENTRY_TYPE_STRING);
}

bool tt_bundle_opt_bool(Bundle bundle, const char* key, bool* out) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry_ptr = BundleDict_get(data->dict, key);
    if (entry_ptr != NULL) {
        BundleEntry* entry = *entry_ptr;
        if (entry->type == BUNDLE_ENTRY_TYPE_BOOL) {
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
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry_ptr = BundleDict_get(data->dict, key);
    if (entry_ptr != NULL) {
        BundleEntry* entry = *entry_ptr;
        if (entry->type == BUNDLE_ENTRY_TYPE_INT32) {
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
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry_ptr = BundleDict_get(data->dict, key);
    if (entry_ptr != NULL) {
        BundleEntry* entry = *entry_ptr;
        if (entry->type == BUNDLE_ENTRY_TYPE_STRING) {
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
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry_handle = BundleDict_get(data->dict, key);
    if (entry_handle != NULL) {
        BundleEntry* entry = *entry_handle;
        tt_assert(entry->type == BUNDLE_ENTRY_TYPE_BOOL);
        entry->bool_value = value;
    } else {
        BundleEntry* entry = bundle_entry_alloc_bool(value);
        BundleDict_set_at(data->dict, key, entry);
    }
}

void tt_bundle_put_int32(Bundle bundle, const char* key, int32_t value) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry_handle = BundleDict_get(data->dict, key);
    if (entry_handle != NULL) {
        BundleEntry* entry = *entry_handle;
        tt_assert(entry->type == BUNDLE_ENTRY_TYPE_INT32);
        entry->int32_value = value;
    } else {
        BundleEntry* entry = bundle_entry_alloc_int32(value);
        BundleDict_set_at(data->dict, key, entry);
    }
}

void tt_bundle_put_string(Bundle bundle, const char* key, const char* value) {
    BundleData* data = (BundleData*)bundle;
    BundleEntry** entry_handle = BundleDict_get(data->dict, key);
    if (entry_handle != NULL) {
        BundleEntry* entry = *entry_handle;
        tt_assert(entry->type == BUNDLE_ENTRY_TYPE_STRING);
        if (entry->string_ptr != NULL) {
            free(entry->string_ptr);
        }
        entry->string_ptr = malloc(strlen(value) + 1);
        strcpy(entry->string_ptr, value);
    } else {
        BundleEntry* entry = bundle_entry_alloc_string(value);
        BundleDict_set_at(data->dict, key, entry);
    }
}

// endregion Bundle
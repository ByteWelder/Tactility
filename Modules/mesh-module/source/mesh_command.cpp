// SPDX-License-Identifier: Apache-2.0
#include <mesh/mesh.h>
#include <mesh/mesh_service.h>

#include <app/manifest.h>

#include <tactility/drivers/radio.h>
#include <tactility/radio_settings.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

constexpr size_t CHANNEL_LIMIT = 8;

static void print_usage() {
    printf(
        "usage: mesh <command>\n"
        "  status\n"
        "  on|off\n"
        "  backend [<id>]\n"
        "  region [<name>]\n"
        "  preset [<name>]\n"
        "  channels\n"
        "  join <#channel>\n"
    );
}

static int report_error(const char* action, error_t error) {
    printf("mesh: %s failed: %s\n", action, error_to_string(error));
    return 1;
}

static int command_status() {
    printf("enabled: %s\n", mesh_is_enabled() ? "yes" : "no");
    printf("state: %s\n", mesh_state_to_string(mesh_get_state()));

    char backend[MESH_BACKEND_ID_MAX_LENGTH + 1];
    if (mesh_get_backend(backend, sizeof(backend)) == ERROR_NONE) {
        printf("backend: %s\n", backend);
    }
    char preset[MESH_NAME_MAX_LENGTH + 1];
    if (mesh_get_preset(preset, sizeof(preset)) == ERROR_NONE) {
        printf("preset: %s\n", preset);
    }
    printf("region: %s\n", radio_region_to_string(radio_settings_load_region()));
    return 0;
}

static int command_enable(bool enabled) {
    error_t error = mesh_set_enabled(enabled);
    if (error != ERROR_NONE) {
        return report_error(enabled ? "on" : "off", error);
    }
    printf("state: %s\n", mesh_state_to_string(mesh_get_state()));
    return 0;
}

static int command_backend(int argc, char* argv[]) {
    if (argc > 3) {
        printf("usage: mesh backend [<id>]\n");
        return 1;
    }

    if (argc == 3) {
        error_t error = mesh_set_backend(argv[2]);
        if (error == ERROR_NOT_FOUND) {
            printf("mesh: unknown backend: %s\n", argv[2]);
            return 1;
        } else if (error != ERROR_NONE) {
            return report_error("backend", error);
        }
        printf("backend: %s\n", argv[2]);
        return 0;
    }

    char selected[MESH_BACKEND_ID_MAX_LENGTH + 1] = {};
    mesh_get_backend(selected, sizeof(selected));
    const size_t count = mesh_get_backend_count();
    if (count == 0) {
        printf("No backends available\n");
        return 0;
    }
    for (size_t i = 0; i < count; i++) {
        char id[MESH_BACKEND_ID_MAX_LENGTH + 1];
        if (mesh_get_backend_id(i, id, sizeof(id)) == ERROR_NONE) {
            printf("%s %s\n", strcmp(id, selected) == 0 ? "*" : " ", id);
        }
    }
    return 0;
}

static int command_region(int argc, char* argv[]) {
    if (argc > 3) {
        printf("usage: mesh region [<name>]\n");
        return 1;
    }

    if (argc == 3) {
        RadioRegion region = RADIO_REGION_UNSET;
        if (radio_region_from_string(argv[2], &region) != ERROR_NONE || region == RADIO_REGION_UNSET) {
            printf("mesh: unknown region: %s\n", argv[2]);
            return 1;
        }
        error_t error = radio_settings_save_region(region);
        if (error != ERROR_NONE) {
            return report_error("region", error);
        }
        printf("region: %s\n", argv[2]);
        return 0;
    }

    const RadioRegion selected = radio_settings_load_region();
    for (int i = RADIO_REGION_UNSET + 1; i < RADIO_REGION_COUNT; i++) {
        const auto region = static_cast<RadioRegion>(i);
        printf("%s %s\n", region == selected ? "*" : " ", radio_region_to_string(region));
    }
    return 0;
}

static int command_preset(int argc, char* argv[]) {
    if (argc > 3) {
        printf("usage: mesh preset [<name>]\n");
        return 1;
    }

    if (argc == 3) {
        error_t error = mesh_set_preset(argv[2]);
        if (error == ERROR_NOT_FOUND) {
            printf("mesh: unknown preset: %s\n", argv[2]);
            return 1;
        } else if (error != ERROR_NONE) {
            return report_error("preset", error);
        }
        printf("preset: %s\n", argv[2]);
        return 0;
    }

    char selected[MESH_NAME_MAX_LENGTH + 1] = {};
    mesh_get_preset(selected, sizeof(selected));
    const size_t count = mesh_get_preset_count();
    if (count == 0) {
        printf("The backend has no presets\n");
        return 0;
    }
    for (size_t i = 0; i < count; i++) {
        char name[MESH_NAME_MAX_LENGTH + 1];
        if (mesh_get_preset_name(i, name, sizeof(name)) == ERROR_NONE) {
            printf("%s %s\n", strcmp(name, selected) == 0 ? "*" : " ", name);
        }
    }
    return 0;
}

static int command_channels() {
    MeshChannel channels[CHANNEL_LIMIT];
    size_t count = 0;
    error_t error = mesh_get_channels(channels, CHANNEL_LIMIT, &count);
    if (error == ERROR_NOT_SUPPORTED) {
        printf("The backend has no channels\n");
        return 0;
    } else if (error != ERROR_NONE) {
        return report_error("channels", error);
    }

    if (count == 0) {
        printf("No channels\n");
        return 0;
    }
    for (size_t i = 0; i < count; i++) {
        printf("%u %s\n", channels[i].index, channels[i].name);
    }
    return 0;
}

static int command_join(int argc, char* argv[]) {
    const char* name = argc == 3 ? argv[2] : nullptr;
    if (name == nullptr || name[0] != '#' || name[1] == '\0') {
        printf("usage: mesh join <#channel>\n");
        return 1;
    }
    if (strlen(name) > MESH_NAME_MAX_LENGTH) {
        printf("mesh: channel name must be at most %d characters\n", MESH_NAME_MAX_LENGTH);
        return 1;
    }

    // A new channel gets the next free index
    MeshChannel channels[CHANNEL_LIMIT];
    size_t count = 0;
    uint8_t index = 0;
    bool found = false;
    if (mesh_get_channels(channels, CHANNEL_LIMIT, &count) == ERROR_NONE) {
        for (size_t i = 0; i < count; i++) {
            if (strcmp(channels[i].name, name) == 0) {
                index = channels[i].index;
                found = true;
                break;
            }
            index = std::max<uint8_t>(index, channels[i].index + 1);
        }
        if (!found && count >= CHANNEL_LIMIT) {
            printf("mesh: no free channel slot\n");
            return 1;
        }
    }

    MeshChannel channel {};
    channel.index = index;
    strncpy(channel.name, name, MESH_NAME_MAX_LENGTH);
    channel.key_length = 0;
    error_t error = mesh_set_channel(&channel);
    if (error != ERROR_NONE) {
        return report_error("join", error);
    }
    printf("Joined %s\n", name);
    return 0;
}

static int32_t mesh_command_main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "help") == 0) {
        print_usage();
        return 0;
    }

    const char* command = argv[1];
    if (strcmp(command, "status") == 0) {
        return command_status();
    } else if (strcmp(command, "on") == 0 || strcmp(command, "off") == 0) {
        return command_enable(strcmp(command, "on") == 0);
    } else if (strcmp(command, "backend") == 0) {
        return command_backend(argc, argv);
    } else if (strcmp(command, "region") == 0) {
        return command_region(argc, argv);
    } else if (strcmp(command, "preset") == 0) {
        return command_preset(argc, argv);
    } else if (strcmp(command, "channels") == 0) {
        return command_channels();
    } else if (strcmp(command, "join") == 0) {
        return command_join(argc, argv);
    }

    printf("mesh: unknown command: %s\n", command);
    print_usage();
    return 1;
}

extern "C" {

const AppManifest mesh_command_manifest = {
    .id = "mesh",
    .name = "mesh",
    .category = APP_CATEGORY_SYSTEM,
    .location = { .type = APP_LOCATION_MEMORY, .location = reinterpret_cast<void*>(mesh_command_main) },
    .flags = APP_MANIFEST_FLAG_HEADLESS,
    .stack = { .depth = 5120, .desired_memory_capability = 0 },
};

}

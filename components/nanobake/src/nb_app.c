#include "nb_app_i.h"
#include "check.h"

const char* prv_type_service = "service";
const char* prv_type_system = "system";
const char* prv_type_user = "user";

const char* nb_app_type_to_string(NbAppType type) {
    switch (type) {
        case SERVICE:
            return prv_type_service;
        case SYSTEM:
            return prv_type_system;
        case USER:
            return prv_type_user;
        default:
            furi_crash();
    }
}

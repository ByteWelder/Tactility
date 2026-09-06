#include <http/types.h>
#include <tactility/error.h>
#include <cstring>

extern "C" {

const char* http_method_to_string(HttpMethod method) {
    switch (method) {
        case HTTP_METHOD_CONNECT:
            return "CONNECT";
        case HTTP_METHOD_DELETE:
            return "DELETE";
        case HTTP_METHOD_GET:
            return "GET";
        case HTTP_METHOD_HEAD:
            return "HEAD";
        case HTTP_METHOD_OPTIONS:
            return "OPTIONS";
        case HTTP_METHOD_POST:
            return "POST";
        case HTTP_METHOD_PATCH:
            return "PATCH";
        case HTTP_METHOD_PUT:
            return "PUT";
        case HTTP_METHOD_TRACE:
            return "TRACE";
    }
    return "UNKNOWN";
}

error_t http_method_from_string(const char* text, HttpMethod* out_method) {
    if (strcmp("CONNECT", text) == 0) { *out_method = HTTP_METHOD_CONNECT; return ERROR_NONE; }
    if (strcmp("DELETE", text) == 0) { *out_method = HTTP_METHOD_DELETE; return ERROR_NONE; }
    if (strcmp("GET", text) == 0) { *out_method = HTTP_METHOD_GET; return ERROR_NONE; }
    if (strcmp("HEAD", text) == 0) { *out_method = HTTP_METHOD_HEAD; return ERROR_NONE; }
    if (strcmp("OPTIONS", text) == 0) { *out_method = HTTP_METHOD_OPTIONS; return ERROR_NONE; }
    if (strcmp("POST", text) == 0) { *out_method = HTTP_METHOD_POST; return ERROR_NONE; }
    if (strcmp("PATCH", text) == 0) { *out_method = HTTP_METHOD_PATCH; return ERROR_NONE; }
    if (strcmp("PUT", text) == 0) { *out_method = HTTP_METHOD_PUT; return ERROR_NONE; }
    if (strcmp("TRACE", text) == 0) { *out_method = HTTP_METHOD_TRACE; return ERROR_NONE; }
    return ERROR_NOT_FOUND;
}

error_t status_code_to_string(status_code_t code, const char** text) {
    switch (code) {
        case 200: *text = "OK"; return ERROR_NONE;
        case 302: *text = "Found"; return ERROR_NONE;
        case 400: *text = "Bad Request"; return ERROR_NONE;
        case 401: *text = "Unauthorized"; return ERROR_NONE;
        case 403: *text = "Forbidden"; return ERROR_NONE;
        case 404: *text = "Not Found"; return ERROR_NONE;
        case 405: *text = "Method Not Allowed"; return ERROR_NONE;
        case 500: *text = "Internal Server Error"; return ERROR_NONE;
        case 501: *text = "Method Not Implemented"; return ERROR_NONE;
        default: return ERROR_NOT_FOUND;
    }
}

}

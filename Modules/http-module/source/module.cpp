// SPDX-License-Identifier: Apache-2.0
#include <http/download.h>
#include <http/module.h>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#include <esp_http_client.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <esp_sntp.h>
#include <esp_netif.h>
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include <esp_crt_bundle.h>
#endif
#endif


#include <sys/select.h>
extern "C" {

static const ModuleSymbol SYMBOLS[] = {
    DEFINE_MODULE_SYMBOL(http_download_subscribe),
    DEFINE_MODULE_SYMBOL(http_download_unsubscribe),
    DEFINE_MODULE_SYMBOL(http_download_poll),
    DEFINE_MODULE_SYMBOL(http_download_start),
    DEFINE_MODULE_SYMBOL(http_download_cancel),
    // posix
    DEFINE_MODULE_SYMBOL(select),
#ifdef ESP_PLATFORM
    // esp_netif.h
    DEFINE_MODULE_SYMBOL(esp_netif_get_ip_info),
    DEFINE_MODULE_SYMBOL(esp_netif_get_handle_from_ifkey),
    // lwip/sockets.h
    DEFINE_MODULE_SYMBOL(lwip_setsockopt),
    DEFINE_MODULE_SYMBOL(lwip_socket),
    DEFINE_MODULE_SYMBOL(lwip_recv),
    DEFINE_MODULE_SYMBOL(lwip_getpeername),
    DEFINE_MODULE_SYMBOL(lwip_bind),
    DEFINE_MODULE_SYMBOL(lwip_listen),
    DEFINE_MODULE_SYMBOL(lwip_close),
    DEFINE_MODULE_SYMBOL(lwip_accept),
    DEFINE_MODULE_SYMBOL(lwip_getsockname),
    DEFINE_MODULE_SYMBOL(lwip_send),
    DEFINE_MODULE_SYMBOL(lwip_connect),
    DEFINE_MODULE_SYMBOL(lwip_select),
    DEFINE_MODULE_SYMBOL(lwip_gethostbyname),
    DEFINE_MODULE_SYMBOL(ipaddr_addr),
    // esp_sntp.h
    DEFINE_MODULE_SYMBOL(sntp_get_sync_status),
    // esp_http
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    // Needed for HTTPS: an app passes this as crt_bundle_attach to validate certificates against
    // the bundle already compiled into the firmware (CONFIG_MBEDTLS_CERTIFICATE_BUNDLE).
    DEFINE_MODULE_SYMBOL(esp_crt_bundle_attach),
#endif
    DEFINE_MODULE_SYMBOL(esp_http_client_init),
    DEFINE_MODULE_SYMBOL(esp_http_client_perform),
    DEFINE_MODULE_SYMBOL(esp_http_client_cancel_request),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_url),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_post_field),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_post_field),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_header),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_header),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_username),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_username),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_password),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_password),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_authtype),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_user_data),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_user_data),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_errno),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_and_clear_last_tls_error),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_method),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_timeout_ms),
    DEFINE_MODULE_SYMBOL(esp_http_client_delete_header),
    DEFINE_MODULE_SYMBOL(esp_http_client_delete_all_headers),
    DEFINE_MODULE_SYMBOL(esp_http_client_open),
    DEFINE_MODULE_SYMBOL(esp_http_client_write),
    DEFINE_MODULE_SYMBOL(esp_http_client_fetch_headers),
    DEFINE_MODULE_SYMBOL(esp_http_client_is_chunked_response),
    DEFINE_MODULE_SYMBOL(esp_http_client_read),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_status_code),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_content_length),
    DEFINE_MODULE_SYMBOL(esp_http_client_close),
    DEFINE_MODULE_SYMBOL(esp_http_client_cleanup),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_transport_type),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_redirection),
    DEFINE_MODULE_SYMBOL(esp_http_client_reset_redirect_counter),
    DEFINE_MODULE_SYMBOL(esp_http_client_set_auth_data),
    DEFINE_MODULE_SYMBOL(esp_http_client_add_auth),
    DEFINE_MODULE_SYMBOL(esp_http_client_is_complete_data_received),
    DEFINE_MODULE_SYMBOL(esp_http_client_read_response),
    DEFINE_MODULE_SYMBOL(esp_http_client_flush_response),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_url),
    DEFINE_MODULE_SYMBOL(esp_http_client_get_chunk_length),
#endif
    MODULE_SYMBOL_TERMINATOR
};

Module http_module = {
    .name = "http",
    .start = nullptr,
    .stop = nullptr,
    .drivers = nullptr,
    .symbols = SYMBOLS,
    .internal = nullptr,
};

}

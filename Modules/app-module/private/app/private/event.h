#pragma once

#include <app/event.h>
#include <app/instance.h>

#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Deliver @a event to every subscription registered for @a app_instance_id (normally exactly one).
 * @warning Does not work in ISR context.
 * @retval ERROR_NONE delivered to at least one subscription
 * @retval ERROR_NOT_FOUND no subscription is registered for @a app_instance_id
 * @retval ERROR_RESOURCE at least one matching subscription's queue was full; the event was
 * dropped for that subscription (still delivered to any other matching subscription)
 */
error_t app_event_emit(AppInstanceId app_instance_id, const struct AppEvent* event);

#ifdef __cplusplus
}
#endif

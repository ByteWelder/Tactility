#pragma once

#include "core_types.h"

typedef void EventFlag;

/** Allocate EventFlag
 *
 * @return     pointer to EventFlag
 */
EventFlag* tt_event_flag_alloc();

/** Deallocate EventFlag
 *
 * @param      instance  pointer to EventFlag
 */
void tt_event_flag_free(EventFlag* instance);

/** Set flags
 *
 * @param      instance  pointer to EventFlag
 * @param[in]  flags     The flags
 *
 * @return     Resulting flags or error (TtStatus)
 */
uint32_t tt_event_flag_set(EventFlag* instance, uint32_t flags);

/** Clear flags
 *
 * @param      instance  pointer to EventFlag
 * @param[in]  flags     The flags
 *
 * @return     Resulting flags or error (TtStatus)
 */
uint32_t tt_event_flag_clear(EventFlag* instance, uint32_t flags);

/** Get flags
 *
 * @param      instance  pointer to EventFlag
 *
 * @return     Resulting flags
 */
uint32_t tt_event_flag_get(EventFlag* instance);

/** Wait flags
 *
 * @param      instance  pointer to EventFlag
 * @param[in]  flags     The flags
 * @param[in]  options   The option flags
 * @param[in]  timeout   The timeout
 *
 * @return     Resulting flags or error (TtStatus)
 */
uint32_t tt_event_flag_wait(
    EventFlag* instance,
    uint32_t flags,
    uint32_t options,
    uint32_t timeout
);

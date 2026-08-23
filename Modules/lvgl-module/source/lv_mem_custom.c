// SPDX-License-Identifier: Apache-2.0
// LVGL's custom stdlib allocator backend (LV_STDLIB_CUSTOM / CONFIG_LV_USE_CUSTOM_MALLOC) - routes
// lv_malloc()/lv_realloc()/lv_free() through the kernel's memory_*_with_policy() functions instead
// of a fixed-size private pool (LV_STDLIB_BUILTIN) or plain malloc (LV_STDLIB_CLIB), so LVGL grows
// dynamically and prefers PSRAM when available instead of contending with everything else for a
// small, fixed internal-RAM arena.
#include <lvgl/lvgl.h>
#include <tactility/memory.h>

// PSRAM is preferred, not required: MEMORY_CAPABILITY_EXTERNAL is `desired`, so
// memory_alloc_with_policy()/memory_realloc_with_policy() fall back to internal RAM automatically
// on boards without PSRAM, or if PSRAM is exhausted.
static const struct MemoryPolicy LVGL_MEMORY_POLICY = {
    .required = 0,
    .desired = MEMORY_CAPABILITY_EXTERNAL,
    .alignment = 0,
};

void lv_mem_init(void) {
}

void lv_mem_deinit(void) {
}

lv_mem_pool_t lv_mem_add_pool(void* mem, size_t bytes) {
    // Not supported - memory_*_with_policy() owns allocation, LVGL doesn't need to manage its own
    // pools on top of it.
    (void)mem;
    (void)bytes;
    return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool) {
    (void)pool;
}

void* lv_malloc_core(size_t size) {
    return memory_alloc_with_policy(size, &LVGL_MEMORY_POLICY);
}

void* lv_realloc_core(void* p, size_t new_size) {
    return memory_realloc_with_policy(p, new_size, &LVGL_MEMORY_POLICY);
}

void lv_free_core(void* p) {
    memory_free(p);
}

void lv_mem_monitor_core(lv_mem_monitor_t* mon_p) {
    // Not supported - memory_*_with_policy() doesn't expose LVGL-specific usage/fragmentation
    // stats (memory_print_stats() covers overall heap state instead).
    (void)mon_p;
}

lv_result_t lv_mem_test_core(void) {
    return LV_RESULT_OK;
}

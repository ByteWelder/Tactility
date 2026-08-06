#include "doctest.h"
#include <tactility/module.h>

static void symbol_test_function() { /* NO-OP */ }

static error_t test_start_result = ERROR_NONE;
static bool start_called = false;
static struct Module* start_add_order_check_module = nullptr;
static error_t test_start() {
    start_called = true;
    if (start_add_order_check_module != nullptr) {
        // If the module was already added to the ledger before start() runs,
        // a duplicate module_add() must report that it already exists.
        CHECK_EQ(module_add(start_add_order_check_module), ERROR_INVALID_STATE);
    }
    return test_start_result;
}

static error_t test_stop_result = ERROR_NONE;
static bool stop_called = false;
static error_t test_stop() {
    stop_called = true;
    return test_stop_result;
}

TEST_CASE("Module construction and destruction") {
    struct Module module = {
        .name = "test",
        .start = test_start,
        .stop = test_stop,
        .symbols = nullptr,
        .internal = nullptr
    };

    // Test successful construction
    CHECK_EQ(module_construct(&module), ERROR_NONE);
    CHECK_EQ(module_is_started(&module), false);

    // Test successful destruction
    CHECK_EQ(module_destruct(&module), ERROR_NONE);
}

TEST_CASE("Module registration") {
    struct Module module = {
        .name = "test",
        .start = test_start,
        .stop = test_stop,
        .symbols = nullptr,
        .internal = nullptr
    };

    // module_add should succeed
    CHECK_EQ(module_add(&module), ERROR_NONE);

    // module_remove should succeed
    CHECK_EQ(module_remove(&module), ERROR_NONE);
}

TEST_CASE("Module lifecycle") {
    start_called = false;
    stop_called = false;
    test_start_result = ERROR_NONE;
    test_stop_result = ERROR_NONE;

    struct Module module = {
        .name = "test",
        .start = test_start,
        .stop = test_stop,
        .symbols = nullptr,
        .internal = nullptr
    };

    CHECK_EQ(module_construct(&module), ERROR_NONE);

    // 1. Successful start (no parent required anymore)
    CHECK_EQ(module_start(&module), ERROR_NONE);
    CHECK_EQ(module_is_started(&module), true);
    CHECK_EQ(start_called, true);

    // Start when already started (should return ERROR_NONE)
    start_called = false;
    CHECK_EQ(module_start(&module), ERROR_NONE);
    CHECK_EQ(start_called, false); // start() function should NOT be called again

    // Stop successful
    CHECK_EQ(module_stop(&module), ERROR_NONE);
    CHECK_EQ(module_is_started(&module), false);
    CHECK_EQ(stop_called, true);

    // Stop when already stopped (should return ERROR_NONE)
    stop_called = false;
    CHECK_EQ(module_stop(&module), ERROR_NONE);
    CHECK_EQ(stop_called, false); // stop() function should NOT be called again

    // Test failed start
    test_start_result = ERROR_NOT_FOUND;
    start_called = false;
    CHECK_EQ(module_start(&module), ERROR_NOT_FOUND);
    CHECK_EQ(module_is_started(&module), false);
    CHECK_EQ(start_called, true);

    // Test failed stop
    test_start_result = ERROR_NONE;
    CHECK_EQ(module_start(&module), ERROR_NONE);

    test_stop_result = ERROR_NOT_SUPPORTED;
    stop_called = false;
    CHECK_EQ(module_stop(&module), ERROR_NOT_SUPPORTED);
    CHECK_EQ(module_is_started(&module), true); // Should still be started if stop failed
    CHECK_EQ(stop_called, true);

    // Clean up: fix stop result so we can stop it
    test_stop_result = ERROR_NONE;
    CHECK_EQ(module_stop(&module), ERROR_NONE);

    CHECK_EQ(module_destruct(&module), ERROR_NONE);
}

TEST_CASE("Global symbol resolution") {
    static const struct ModuleSymbol test_symbols[] = {
        DEFINE_MODULE_SYMBOL(symbol_test_function),
        MODULE_SYMBOL_TERMINATOR
    };

    struct Module module = {
        .name = "test_sym",
        .start = test_start,
        .stop = test_stop,
        .symbols = test_symbols,
        .internal = nullptr
    };

    REQUIRE_EQ(module_construct(&module), ERROR_NONE);

    uintptr_t addr;
    // Should fail as it is not added or started
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), false);
    REQUIRE_EQ(module_add(&module), ERROR_NONE);
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), false);
    REQUIRE_EQ(module_start(&module), ERROR_NONE);
    // Still fails as symbols are null
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), true);
    // Cleanup
    CHECK_EQ(module_remove(&module), ERROR_NONE);

    CHECK_EQ(module_destruct(&module), ERROR_NONE);
}

TEST_CASE("module_ensure_started adds module to global ledger") {
    start_called = false;
    stop_called = false;
    test_start_result = ERROR_NONE;
    test_stop_result = ERROR_NONE;

    static const struct ModuleSymbol test_symbols[] = {
        DEFINE_MODULE_SYMBOL(symbol_test_function),
        MODULE_SYMBOL_TERMINATOR
    };

    struct Module module = {
        .name = "test_ensure_started",
        .start = test_start,
        .stop = test_stop,
        .symbols = test_symbols,
        .internal = nullptr
    };

    uintptr_t addr;
    // Not resolvable before module_ensure_started is called
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), false);

    // test_start() asserts module_add(&module) is already ERROR_INVALID_STATE by the
    // time start() runs, proving module_add happens before module_start (not after).
    start_add_order_check_module = &module;
    CHECK_EQ(module_ensure_started(&module), ERROR_NONE);
    start_add_order_check_module = nullptr;
    CHECK_EQ(module_is_started(&module), true);
    CHECK_EQ(start_called, true);

    // Module must be both added to the ledger and started to be resolvable
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), true);

    // Calling again should be idempotent: no duplicate start, still resolvable
    start_called = false;
    CHECK_EQ(module_ensure_started(&module), ERROR_NONE);
    CHECK_EQ(start_called, false);
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), true);

    // Cleanup
    CHECK_EQ(module_stop(&module), ERROR_NONE);
    CHECK_EQ(module_remove(&module), ERROR_NONE);
    CHECK_EQ(module_destruct(&module), ERROR_NONE);
}

TEST_CASE("module_ensure_destructed removes module from global ledger") {
    start_called = false;
    stop_called = false;
    test_start_result = ERROR_NONE;
    test_stop_result = ERROR_NONE;

    static const struct ModuleSymbol test_symbols[] = {
        DEFINE_MODULE_SYMBOL(symbol_test_function),
        MODULE_SYMBOL_TERMINATOR
    };

    struct Module module = {
        .name = "test_ensure_destructed",
        .start = test_start,
        .stop = test_stop,
        .symbols = test_symbols,
        .internal = nullptr
    };

    CHECK_EQ(module_ensure_started(&module), ERROR_NONE);

    uintptr_t addr;
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), true);

    CHECK_EQ(module_ensure_destructed(&module), ERROR_NONE);
    CHECK_EQ(module_is_started(&module), false);
    CHECK_EQ(stop_called, true);

    // Module must no longer be resolvable once destructed. Note: this alone doesn't
    // prove removal from the ledger, since module_resolve_symbol_global() also skips
    // non-started modules — a leaked-but-stopped ledger entry would look the same.
    CHECK_EQ(module_resolve_symbol_global("symbol_test_function", &addr), false);

    // Directly prove detachment from the ledger: if module_ensure_destructed had left
    // the module in place, this module_add() would return ERROR_INVALID_STATE.
    CHECK_EQ(module_add(&module), ERROR_NONE);
    CHECK_EQ(module_remove(&module), ERROR_NONE);

    // Calling again on an already-destructed module should be a no-op
    CHECK_EQ(module_ensure_destructed(&module), ERROR_NONE);
}

// SPDX-License-Identifier: Apache-2.0
#include "doctest.h"

#include <app/private/arguments.h>

#include <cstring>

TEST_CASE("app_arguments_copy returns NULL for argc <= 0") {
    const char* const argv[] = { "a" };
    CHECK_EQ(app_arguments_copy(0, argv), nullptr);
    CHECK_EQ(app_arguments_copy(-1, argv), nullptr);
    CHECK_EQ(app_arguments_copy(0, nullptr), nullptr);
}

TEST_CASE("app_arguments_copy deep-copies argv") {
    const char* const argv[] = { "one", "two", "three" };
    char** copy = app_arguments_copy(3, argv);
    REQUIRE_NE(copy, nullptr);

    CHECK_NE(static_cast<const void*>(copy[0]), static_cast<const void*>(argv[0]));
    CHECK_EQ(std::strcmp(copy[0], "one"), 0);
    CHECK_EQ(std::strcmp(copy[1], "two"), 0);
    CHECK_EQ(std::strcmp(copy[2], "three"), 0);
    CHECK_EQ(copy[3], nullptr);

    app_arguments_free(3, copy);
}

TEST_CASE("app_arguments_copy handles an empty string argument") {
    const char* const argv[] = { "" };
    char** copy = app_arguments_copy(1, argv);
    REQUIRE_NE(copy, nullptr);
    CHECK_EQ(std::strcmp(copy[0], ""), 0);
    CHECK_EQ(copy[1], nullptr);
    app_arguments_free(1, copy);
}

TEST_CASE("app_arguments_free is a no-op for count 0 / null values") {
    app_arguments_free(0, nullptr);
}

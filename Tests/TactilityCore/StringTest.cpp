#include "doctest.h"
#include "StringUtils.h"

using namespace tt;
using namespace std;

// region string_split

TEST_CASE("splitting an empty string results in an empty vector") {
    auto result = string_split("", ".");
    CHECK_EQ(result.empty(), true);
}

TEST_CASE("splitting a string with a single token results in a vector with that token") {
    auto result = string_split("token", ".");
    CHECK_EQ(result.size(), 1);
    CHECK_EQ(result.front(), "token");
}

TEST_CASE("splitting a string with multiple tokens results in a vector with those tokens") {
    auto result = string_split("token1;token2;token3;", ";");
    CHECK_EQ(result.size(), 3);
    CHECK_EQ(result[0], "token1");
    CHECK_EQ(result[1], "token2");
    CHECK_EQ(result[2], "token3");
}

// endregion string_split

// region string_join

TEST_CASE("joining an empty vector results in an empty string") {
    vector<string> tokens = {};
    auto result = string_join(tokens, ".");
    CHECK_EQ(result, "");
}

TEST_CASE("joining a single token results in a string with that value") {
    vector<string> tokens = {
        "token"
    };
    auto result = string_join(tokens, ".");
    CHECK_EQ(result, "token");
}

TEST_CASE("joining multiple tokens results in a string with all the tokens and the delimiter") {
    vector<string> tokens = {
        "token1",
        "token2",
        "token3",
    };
    auto result = string_join(tokens, ".");
    CHECK_EQ(result, "token1.token2.token3");
}

TEST_CASE("joining with empty tokens leads to an extra delimiter") {
    vector<string> tokens = {
     "token1",
     "",
     "token2",
    };
    auto result = string_join(tokens, ".");
    CHECK_EQ(result, "token1..token2");
}

// endregion string_join

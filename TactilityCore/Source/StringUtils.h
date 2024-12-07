#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace tt::string {

/**
 * Find the last occurrence of a character.
 * @param[in] text the text to search in
 * @param[in] from_index the index to search from (searching from right to left)
 * @param[in] find the character to search for
 * @return the index of the found character, or -1 if none found
 */
int findLastIndex(const char* text, size_t from_index, char find);

/**
 * Given a filesystem path as input, try and get the parent path.
 * @param[in] path input path
 * @param[out] output an output buffer that is allocated to at least the size of "current"
 * @return true when successful
 */
bool getPathParent(const char* path, char* output);

/**
 * Splits the provided input into separate pieces with delimiter as separator text.
 * When the input string is empty, the output list will be empty too.
 *
 * @param input the input to split up
 * @param delimiter a non-empty string to recognize as separator
 */
std::vector<std::string> split(const std::string& input, const std::string& delimiter);

/**
 * Join a set of tokens into a single string, given a delimiter (separator).
 * If the input is an empty list, the result will be an empty string.
 * The delimeter is only placed inbetween tokens and not appended at the end of the resulting string.
 *
 * @param input the tokens to join together
 * @param delimiter the separator to join with
 */
std::string join(const std::vector<std::string>& input, const std::string& delimiter);

} // namespace

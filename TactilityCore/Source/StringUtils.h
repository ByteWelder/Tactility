#pragma once

#include <cstdio>

namespace tt {

/**
 * Find the last occurrence of a character.
 * @param[in] text the text to search in
 * @param[in] from_index the index to search from (searching from right to left)
 * @param[in] find the character to search for
 * @return the index of the found character, or -1 if none found
 */
int string_find_last_index(const char* text, size_t from_index, char find);

/**
 * Given a filesystem path as input, try and get the parent path.
 * @param[in] path input path
 * @param[out] output an output buffer that is allocated to at least the size of "current"
 * @return true when successful
 */
bool string_get_path_parent(const char* path, char* output);

} // namespace

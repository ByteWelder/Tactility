#include "html2text.h"
#include <cstring>
#include <cctype>

enum { HTML_FIRST, HTML_MID };

static int SearchHtmlTag(const char* html, int offset, int* condition) {
    int i = offset;
    size_t max = strlen(html);

    if (offset >= max) return -2;

    switch (*condition) {
        case HTML_FIRST:
            while (i <= max) {
                if (strncmp(html + i, "<", 1) == 0) {
                    while (1) {
                        i++;
                        if (strncmp(html + i, ">", 1) == 0) {
                            i++;
                            if (strncmp(html + i, "<", 1) == 0) {
                                return SearchHtmlTag(html, i, condition);
                            } else {
                                return i;
                            }
                        }
                        if (i == max) {
                            *condition = HTML_MID;
                            return -1;
                        }
                    }
                } else {
                    i++;
                    if (i >= max) return -3;
                }
            }
            break;
        case HTML_MID:
            while (1) {
                if (strncmp(html + i, ">", 1) == 0) {
                    i++;
                    if (strncmp(html + i, "<", 1) == 0) {
                        return SearchHtmlTag(html, i, condition);
                    } else {
                        return i;
                    }
                }
                i++;
            }
            break;
    }
    return -3;  // Shouldn’t reach here
}

std::string html2text(const std::string& html) {
    std::string result;
    const char* html_cstr = html.c_str();
    int i = 0;
    int condition = HTML_FIRST;
    char tmp[100];  // Temporary buffer for words

    while (i < html.length()) {
        if (strncmp(html_cstr + i, "<", 1) != 0) {
            int first_place = i;
            while (i < html.length() && strncmp(html_cstr + i, " ", 1) != 0 && strncmp(html_cstr + i, "<", 1) != 0) {
                i++;
            }

            int word_length = i - first_place;
            if (word_length > 0 && word_length < 100) {
                int last_place = i;
                // Skip non-alphanumeric at start/end
                while (first_place < last_place && !isalnum(html_cstr[first_place])) first_place++;
                while (last_place > first_place && !isalnum(html_cstr[last_place - 1])) last_place--;

                word_length = last_place - first_place;
                if (word_length > 0) {
                    strncpy(tmp, html_cstr + first_place, word_length);
                    tmp[word_length] = '\0';
                    // Convert first letter to lowercase if uppercase
                    if (isupper(tmp[0]) && isalpha(tmp[0])) tmp[0] = tolower(tmp[0]);
                    result += tmp;
                    result += " ";  // Add space between words
                }
            }
            if (i < html.length() && strncmp(html_cstr + i, "<", 1) != 0) i++;
        } else {
            i = SearchHtmlTag(html_cstr, i, &condition);
            if (i == -1 || i < -1) break;  // End or error
        }
    }

    // Trim trailing space
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

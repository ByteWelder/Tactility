#pragma once

#include <m-core.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief String failure constant.
 */
#define TT_STRING_FAILURE ((size_t)-1)

/**
 * @brief Tactility string primitive.
 */
typedef struct TtString TtString;

//---------------------------------------------------------------------------
//                               Constructors
//---------------------------------------------------------------------------

/**
 * @brief Allocate new TtString.
 * @return TtString*
 */
TtString* tt_string_alloc();

/**
 * @brief Allocate new TtString and set it to string.
 * Allocate & Set the string a to the string.
 * @param source 
 * @return TtString*
 */
TtString* tt_string_alloc_set(const TtString* source);

/**
 * @brief Allocate new TtString and set it to C string.
 * Allocate & Set the string a to the C string.
 * @param cstr_source 
 * @return TtString*
 */
TtString* tt_string_alloc_set_str(const char cstr_source[]);

/**
 * @brief Allocate new TtString and printf to it.
 * Initialize and set a string to the given formatted value.
 * @param format 
 * @param ... 
 * @return TtString*
 */
TtString* tt_string_alloc_printf(const char format[], ...)
    _ATTRIBUTE((__format__(__printf__, 1, 2)));

/**
 * @brief Allocate new TtString and printf to it.
 * Initialize and set a string to the given formatted value.
 * @param format 
 * @param args 
 * @return TtString*
 */
TtString* tt_string_alloc_vprintf(const char format[], va_list args);

/**
 * @brief Allocate new TtString and move source string content to it.
 * Allocate the string, set it to the other one, and destroy the other one.
 * @param source 
 * @return TtString*
 */
TtString* tt_string_alloc_move(TtString* source);

//---------------------------------------------------------------------------
//                               Destructors
//---------------------------------------------------------------------------

/**
 * @brief Free TtString.
 * @param string 
 */
void tt_string_free(TtString* string);

//---------------------------------------------------------------------------
//                         String memory management
//---------------------------------------------------------------------------

/**
 * @brief Reserve memory for string.
 * Modify the string capacity to be able to handle at least 'alloc' characters (including final null char).
 * @param string 
 * @param size 
 */
void tt_string_reserve(TtString* string, size_t size);

/**
 * @brief Reset string.
 * Make the string empty.
 * @param s 
 */
void tt_string_reset(TtString* string);

/**
 * @brief Swap two strings.
 * Swap the two strings string_1 and string_2.
 * @param string_1 
 * @param string_2 
 */
void tt_string_swap(TtString* string_1, TtString* string_2);

/**
 * @brief Move string_2 content to string_1.
 * Set the string to the other one, and destroy the other one.
 * @param string_1 
 * @param string_2 
 */
void tt_string_move(TtString* string_1, TtString* string_2);

/**
 * @brief Compute a hash for the string.
 * @param string 
 * @return size_t 
 */
size_t tt_string_hash(const TtString* string);

/**
 * @brief Get string size (usually length, but not for UTF-8)
 * @param string 
 * @return size_t 
 */
size_t tt_string_size(const TtString* string);

/**
 * @brief Check that string is empty or not
 * @param string 
 * @return bool
 */
bool tt_string_empty(const TtString* string);

//---------------------------------------------------------------------------
//                               Getters
//---------------------------------------------------------------------------

/**
 * @brief Get the character at the given index.
 * Return the selected character of the string.
 * @param string 
 * @param index 
 * @return char 
 */
char tt_string_get_char(const TtString* string, size_t index);

/**
 * @brief Return the string view a classic C string.
 * @param string 
 * @return const char* 
 */
const char* tt_string_get_cstr(const TtString* string);

//---------------------------------------------------------------------------
//                               Setters
//---------------------------------------------------------------------------

/**
 * @brief Set the string to the other string.
 * Set the string to the source string.
 * @param string 
 * @param source 
 */
void tt_string_set(TtString* string, TtString* source);

/**
 * @brief Set the string to the other C string.
 * Set the string to the source C string.
 * @param string 
 * @param source 
 */
void tt_string_set_str(TtString* string, const char source[]);

/**
 * @brief Set the string to the n first characters of the C string.
 * @param string 
 * @param source 
 * @param length 
 */
void tt_string_set_strn(TtString* string, const char source[], size_t length);

/**
 * @brief Set the character at the given index.
 * @param string 
 * @param index 
 * @param c 
 */
void tt_string_set_char(TtString* string, size_t index, const char c);

/**
 * @brief Set the string to the n first characters of other one.
 * @param string 
 * @param source 
 * @param offset 
 * @param length 
 */
void tt_string_set_n(TtString* string, const TtString* source, size_t offset, size_t length);

/**
 * @brief Format in the string the given printf format
 * @param string 
 * @param format 
 * @param ... 
 * @return int 
 */
int tt_string_printf(TtString* string, const char format[], ...)
    _ATTRIBUTE((__format__(__printf__, 2, 3)));

/**
 * @brief Format in the string the given printf format
 * @param string 
 * @param format 
 * @param args 
 * @return int 
 */
int tt_string_vprintf(TtString* string, const char format[], va_list args);

//---------------------------------------------------------------------------
//                               Appending
//---------------------------------------------------------------------------

/**
 * @brief Append a character to the string.
 * @param string 
 * @param c 
 */
void tt_string_push_back(TtString* string, char c);

/**
 * @brief Append a string to the string.
 * Concatenate the string with the other string.
 * @param string_1 
 * @param string_2 
 */
void tt_string_cat(TtString* string_1, const TtString* string_2);

/**
 * @brief Append a C string to the string.
 * Concatenate the string with the C string.
 * @param string_1 
 * @param cstring_2 
 */
void tt_string_cat_str(TtString* string_1, const char cstring_2[]);

/**
 * @brief Append to the string the formatted string of the given printf format.
 * @param string 
 * @param format 
 * @param ... 
 * @return int 
 */
int tt_string_cat_printf(TtString* string, const char format[], ...)
    _ATTRIBUTE((__format__(__printf__, 2, 3)));

/**
 * @brief Append to the string the formatted string of the given printf format.
 * @param string 
 * @param format 
 * @param args 
 * @return int 
 */
int tt_string_cat_vprintf(TtString* string, const char format[], va_list args);

//---------------------------------------------------------------------------
//                               Comparators
//---------------------------------------------------------------------------

/**
 * @brief Compare two strings and return the sort order.
 * @param string_1 
 * @param string_2 
 * @return int 
 */
int tt_string_cmp(const TtString* string_1, const TtString* string_2);

/**
 * @brief Compare string with C string and return the sort order.
 * @param string_1 
 * @param cstring_2 
 * @return int 
 */
int tt_string_cmp_str(const TtString* string_1, const char cstring_2[]);

/**
 * @brief Compare two strings (case insensitive according to the current locale) and return the sort order.
 * Note: doesn't work with UTF-8 strings.
 * @param string_1 
 * @param string_2 
 * @return int 
 */
int tt_string_cmpi(const TtString* string_1, const TtString* string_2);

/**
 * @brief Compare string with C string (case insensitive according to the current locale) and return the sort order.
 * Note: doesn't work with UTF-8 strings.
 * @param string_1 
 * @param cstring_2 
 * @return int 
 */
int tt_string_cmpi_str(const TtString* string_1, const char cstring_2[]);

//---------------------------------------------------------------------------
//                                 Search
//---------------------------------------------------------------------------

/**
 * @brief Search the first occurrence of the needle in the string from the position start.
 * Return STRING_FAILURE if not found.
 * By default, start is zero.
 * @param string 
 * @param needle 
 * @param start 
 * @return size_t 
 */
size_t tt_string_search(const TtString* string, const TtString* needle, size_t start);

/**
 * @brief Search the first occurrence of the needle in the string from the position start.
 * Return STRING_FAILURE if not found.
 * @param string 
 * @param needle 
 * @param start 
 * @return size_t 
 */
size_t tt_string_search_str(const TtString* string, const char needle[], size_t start);

/**
 * @brief Search for the position of the character c from the position start (include) in the string.
 * Return STRING_FAILURE if not found.
 * By default, start is zero.
 * @param string 
 * @param c 
 * @param start 
 * @return size_t 
 */
size_t tt_string_search_char(const TtString* string, char c, size_t start);

/**
 * @brief Reverse search for the position of the character c from the position start (include) in the string.
 * Return STRING_FAILURE if not found.
 * By default, start is zero.
 * @param string 
 * @param c 
 * @param start 
 * @return size_t 
 */
size_t tt_string_search_rchar(const TtString* string, char c, size_t start);

//---------------------------------------------------------------------------
//                                Equality
//---------------------------------------------------------------------------

/**
 * @brief Test if two strings are equal.
 * @param string_1 
 * @param string_2 
 * @return bool 
 */
bool tt_string_equal(const TtString* string_1, const TtString* string_2);

/**
 * @brief Test if the string is equal to the C string.
 * @param string_1 
 * @param cstring_2 
 * @return bool 
 */
bool tt_string_equal_str(const TtString* string_1, const char cstring_2[]);

//---------------------------------------------------------------------------
//                                Replace
//---------------------------------------------------------------------------

/**
 * @brief Replace in the string the sub-string at position 'pos' for 'len' bytes into the C string 'replace'.
 * @param string 
 * @param pos 
 * @param len 
 * @param replace 
 */
void tt_string_replace_at(TtString* string, size_t pos, size_t len, const char replace[]);

/**
 * @brief Replace a string 'needle' to string 'replace' in a string from 'start' position.
 * By default, start is zero.
 * Return STRING_FAILURE if 'needle' not found or replace position.
 * @param string 
 * @param needle 
 * @param replace 
 * @param start 
 * @return size_t 
 */
size_t
tt_string_replace(TtString* string, TtString* needle, TtString* replace, size_t start);

/**
 * @brief Replace a C string 'needle' to C string 'replace' in a string from 'start' position.
 * By default, start is zero.
 * Return STRING_FAILURE if 'needle' not found or replace position.
 * @param string 
 * @param needle 
 * @param replace 
 * @param start 
 * @return size_t 
 */
size_t tt_string_replace_str(
    TtString* string,
    const char needle[],
    const char replace[],
    size_t start
);

/**
 * @brief Replace all occurrences of 'needle' string into 'replace' string.
 * @param string 
 * @param needle 
 * @param replace 
 */
void tt_string_replace_all(
    TtString* string,
    const TtString* needle,
    const TtString* replace
);

/**
 * @brief Replace all occurrences of 'needle' C string into 'replace' C string.
 * @param string 
 * @param needle 
 * @param replace 
 */
void tt_string_replace_all_str(TtString* string, const char needle[], const char replace[]);

//---------------------------------------------------------------------------
//                            Start / End tests
//---------------------------------------------------------------------------

/**
 * @brief Test if the string starts with the given string.
 * @param string 
 * @param start 
 * @return bool 
 */
bool tt_string_start_with(const TtString* string, const TtString* start);

/**
 * @brief Test if the string starts with the given C string.
 * @param string 
 * @param start 
 * @return bool 
 */
bool tt_string_start_with_str(const TtString* string, const char start[]);

/**
 * @brief Test if the string ends with the given string.
 * @param string 
 * @param end 
 * @return bool 
 */
bool tt_string_end_with(const TtString* string, const TtString* end);

/**
 * @brief Test if the string ends with the given C string.
 * @param string 
 * @param end 
 * @return bool 
 */
bool tt_string_end_with_str(const TtString* string, const char end[]);

//---------------------------------------------------------------------------
//                                Trim
//---------------------------------------------------------------------------

/**
 * @brief Trim the string left to the first 'index' bytes.
 * @param string 
 * @param index 
 */
void tt_string_left(TtString* string, size_t index);

/**
 * @brief Trim the string right from the 'index' position to the last position.
 * @param string 
 * @param index 
 */
void tt_string_right(TtString* string, size_t index);

/**
 * @brief Trim the string from position index to size bytes.
 * See also tt_string_set_n.
 * @param string 
 * @param index 
 * @param size 
 */
void tt_string_mid(TtString* string, size_t index, size_t size);

/**
 * @brief Trim a string from the given set of characters (default is " \n\r\t").
 * @param string 
 * @param chars 
 */
void tt_string_trim(TtString* string, const char chars[]);

//---------------------------------------------------------------------------
//                                UTF8
//---------------------------------------------------------------------------

/**
 * @brief An unicode value.
 */
typedef unsigned int TtStringUnicodeValue;

/**
 * @brief Compute the length in UTF8 characters in the string.
 * @param string 
 * @return size_t 
 */
size_t tt_string_utf8_length(TtString* string);

/**
 * @brief Push unicode into string, encoding it in UTF8.
 * @param string 
 * @param unicode 
 */
void tt_string_utf8_push(TtString* string, TtStringUnicodeValue unicode);

/**
 * @brief State of the UTF8 decoding machine state.
 */
typedef enum {
    TtStringUTF8StateStarting,
    TtStringUTF8StateDecoding1,
    TtStringUTF8StateDecoding2,
    TtStringUTF8StateDecoding3,
    TtStringUTF8StateError
} TtStringUTF8State;

/**
 * @brief Main generic UTF8 decoder.
 * It takes a character, and the previous state and the previous value of the unicode value.
 * It updates the state and the decoded unicode value.
 * A decoded unicode encoded value is valid only when the state is TtStringUTF8StateStarting.
 * @param c 
 * @param state 
 * @param unicode 
 */
void tt_string_utf8_decode(char c, TtStringUTF8State* state, TtStringUnicodeValue* unicode);

//---------------------------------------------------------------------------
//                Lasciate ogne speranza, voi ch’entrate
//---------------------------------------------------------------------------

/**
 *
 * Select either the string function or the str function depending on
 * the b operand to the function.
 * func1 is the string function / func2 is the str function.
 */

/**
 * @brief Select for 1 argument 
 */
#define TT_STRING_SELECT1(func1, func2, a) \
    _Generic((a), char*: func2, const char*: func2, TtString*: func1, const TtString*: func1)(a)

/**
 * @brief Select for 2 arguments
 */
#define TT_STRING_SELECT2(func1, func2, a, b) \
    _Generic((b), char*: func2, const char*: func2, TtString*: func1, const TtString*: func1)(a, b)

/**
 * @brief Select for 3 arguments
 */
#define TT_STRING_SELECT3(func1, func2, a, b, c) \
    _Generic((b), char*: func2, const char*: func2, TtString*: func1, const TtString*: func1)(a, b, c)

/**
 * @brief Select for 4 arguments
 */
#define TT_STRING_SELECT4(func1, func2, a, b, c, d) \
    _Generic((b), char*: func2, const char*: func2, TtString*: func1, const TtString*: func1)(a, b, c, d)

/**
 * @brief Allocate new TtString and set it content to string (or C string).
 * ([c]string)
 */
#define tt_string_alloc_set(a) \
    TT_STRING_SELECT1(tt_string_alloc_set, tt_string_alloc_set_str, a)

/**
 * @brief Set the string content to string (or C string).
 * (string, [c]string)
 */
#define tt_string_set(a, b) TT_STRING_SELECT2(tt_string_set, tt_string_set_str, a, b)

/**
 * @brief Compare string with string (or C string) and return the sort order.
 * Note: doesn't work with UTF-8 strings.
 * (string, [c]string)
 */
#define tt_string_cmp(a, b) TT_STRING_SELECT2(tt_string_cmp, tt_string_cmp_str, a, b)

/**
 * @brief Compare string with string (or C string) (case insensitive according to the current locale) and return the sort order.
 * Note: doesn't work with UTF-8 strings.
 * (string, [c]string)
 */
#define tt_string_cmpi(a, b) TT_STRING_SELECT2(tt_string_cmpi, tt_string_cmpi_str, a, b)

/**
 * @brief Test if the string is equal to the string (or C string).
 * (string, [c]string)
 */
#define tt_string_equal(a, b) TT_STRING_SELECT2(tt_string_equal, tt_string_equal_str, a, b)

/**
 * @brief Replace all occurrences of string into string (or C string to another C string) in a string.
 * (string, [c]string, [c]string)
 */
#define tt_string_replace_all(a, b, c) \
    TT_STRING_SELECT3(tt_string_replace_all, tt_string_replace_all_str, a, b, c)

/**
 * @brief Search for a string (or C string) in a string
 * (string, [c]string[, start=0])
 */
#define tt_string_search(...)             \
    M_APPLY(                                \
        TT_STRING_SELECT3,                \
        tt_string_search,                 \
        tt_string_search_str,             \
        M_DEFAULT_ARGS(3, (0), __VA_ARGS__) \
    )
/**
 * @brief Search for a C string in a string
 * (string, cstring[, start=0])
 */
#define tt_string_search_str(...) tt_string_search_str(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/**
 * @brief Test if the string starts with the given string (or C string).
 * (string, [c]string)
 */
#define tt_string_start_with(a, b) \
    TT_STRING_SELECT2(tt_string_start_with, tt_string_start_with_str, a, b)

/**
 * @brief Test if the string ends with the given string (or C string).
 * (string, [c]string)
 */
#define tt_string_end_with(a, b) \
    TT_STRING_SELECT2(tt_string_end_with, tt_string_end_with_str, a, b)

/**
 * @brief Append a string (or C string) to the string.
 * (string, [c]string)
 */
#define tt_string_cat(a, b) TT_STRING_SELECT2(tt_string_cat, tt_string_cat_str, a, b)

/**
 * @brief Trim a string from the given set of characters (default is " \n\r\t").
 * (string[, set=" \n\r\t"])
 */
#define tt_string_trim(...) tt_string_trim(M_DEFAULT_ARGS(2, ("  \n\r\t"), __VA_ARGS__))

/**
 * @brief Search for a character in a string.
 * (string, character[, start=0])
 */
#define tt_string_search_char(...) tt_string_search_char(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/**
 * @brief Reverse Search for a character in a string.
 * (string, character[, start=0])
 */
#define tt_string_search_rchar(...) tt_string_search_rchar(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/**
 * @brief Replace a string to another string (or C string to another C string) in a string.
 * (string, [c]string, [c]string[, start=0])
 */
#define tt_string_replace(...)            \
    M_APPLY(                                \
        TT_STRING_SELECT4,                \
        tt_string_replace,                \
        tt_string_replace_str,            \
        M_DEFAULT_ARGS(4, (0), __VA_ARGS__) \
    )

/**
 * @brief Replace a C string to another C string in a string.
 * (string, cstring, cstring[, start=0])
 */
#define tt_string_replace_str(...) tt_string_replace_str(M_DEFAULT_ARGS(4, (0), __VA_ARGS__))

/**
 * @brief INIT OPLIST for TtString.
 */
#define F_STR_INIT(a) ((a) = tt_string_alloc())

/**
 * @brief INIT SET OPLIST for TtString.
 */
#define F_STR_INIT_SET(a, b) ((a) = tt_string_alloc_set(b))

/**
 * @brief INIT MOVE OPLIST for TtString.
 */
#define F_STR_INIT_MOVE(a, b) ((a) = tt_string_alloc_move(b))

/**
 * @brief OPLIST for TtString.
 */
#define TT_STRING_OPLIST         \
    (INIT(F_STR_INIT),           \
     INIT_SET(F_STR_INIT_SET),   \
     SET(tt_string_set),       \
     INIT_MOVE(F_STR_INIT_MOVE), \
     MOVE(tt_string_move),     \
     SWAP(tt_string_swap),     \
     RESET(tt_string_reset),   \
     EMPTY_P(tt_string_empty), \
     CLEAR(tt_string_free),    \
     HASH(tt_string_hash),     \
     EQUAL(tt_string_equal),   \
     CMP(tt_string_cmp),       \
     TYPE(TtString*))

#ifdef __cplusplus
}
#endif

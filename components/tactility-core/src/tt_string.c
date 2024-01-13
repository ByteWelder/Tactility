#include "tt_string.h"
#include <m-string.h>

struct TtString {
    string_t string;
};

#undef tt_string_alloc_set
#undef tt_string_set
#undef tt_string_cmp
#undef tt_string_cmpi
#undef tt_string_search
#undef tt_string_search_str
#undef tt_string_equal
#undef tt_string_replace
#undef tt_string_replace_str
#undef tt_string_replace_all
#undef tt_string_start_with
#undef tt_string_end_with
#undef tt_string_search_char
#undef tt_string_search_rchar
#undef tt_string_trim
#undef tt_string_cat

TtString* tt_string_alloc() {
    TtString* string = malloc(sizeof(TtString));
    string_init(string->string);
    return string;
}

TtString* tt_string_alloc_set(const TtString* s) {
    TtString* string = malloc(sizeof(TtString)); //-V799
    string_init_set(string->string, s->string);
    return string;
} //-V773

TtString* tt_string_alloc_set_str(const char cstr[]) {
    TtString* string = malloc(sizeof(TtString)); //-V799
    string_init_set(string->string, cstr);
    return string;
} //-V773

TtString* tt_string_alloc_printf(const char format[], ...) {
    va_list args;
    va_start(args, format);
    TtString* string = tt_string_alloc_vprintf(format, args);
    va_end(args);
    return string;
}

TtString* tt_string_alloc_vprintf(const char format[], va_list args) {
    TtString* string = malloc(sizeof(TtString));
    string_init_vprintf(string->string, format, args);
    return string;
}

TtString* tt_string_alloc_move(TtString* s) {
    TtString* string = malloc(sizeof(TtString));
    string_init_move(string->string, s->string);
    free(s);
    return string;
}

void tt_string_free(TtString* s) {
    string_clear(s->string);
    free(s);
}

void tt_string_reserve(TtString* s, size_t alloc) {
    string_reserve(s->string, alloc);
}

void tt_string_reset(TtString* s) {
    string_reset(s->string);
}

void tt_string_swap(TtString* v1, TtString* v2) {
    string_swap(v1->string, v2->string);
}

void tt_string_move(TtString* v1, TtString* v2) {
    string_clear(v1->string);
    string_init_move(v1->string, v2->string);
    free(v2);
}

size_t tt_string_hash(const TtString* v) {
    return string_hash(v->string);
}

char tt_string_get_char(const TtString* v, size_t index) {
    return string_get_char(v->string, index);
}

const char* tt_string_get_cstr(const TtString* s) {
    return string_get_cstr(s->string);
}

void tt_string_set(TtString* s, TtString* source) {
    string_set(s->string, source->string);
}

void tt_string_set_str(TtString* s, const char cstr[]) {
    string_set(s->string, cstr);
}

void tt_string_set_strn(TtString* s, const char str[], size_t n) {
    string_set_strn(s->string, str, n);
}

void tt_string_set_char(TtString* s, size_t index, const char c) {
    string_set_char(s->string, index, c);
}

int tt_string_cmp(const TtString* s1, const TtString* s2) {
    return string_cmp(s1->string, s2->string);
}

int tt_string_cmp_str(const TtString* s1, const char str[]) {
    return string_cmp(s1->string, str);
}

int tt_string_cmpi(const TtString* v1, const TtString* v2) {
    return string_cmpi(v1->string, v2->string);
}

int tt_string_cmpi_str(const TtString* v1, const char p2[]) {
    return string_cmpi_str(v1->string, p2);
}

size_t tt_string_search(const TtString* v, const TtString* needle, size_t start) {
    return string_search(v->string, needle->string, start);
}

size_t tt_string_search_str(const TtString* v, const char needle[], size_t start) {
    return string_search(v->string, needle, start);
}

bool tt_string_equal(const TtString* v1, const TtString* v2) {
    return string_equal_p(v1->string, v2->string);
}

bool tt_string_equal_str(const TtString* v1, const char v2[]) {
    return string_equal_p(v1->string, v2);
}

void tt_string_push_back(TtString* v, char c) {
    string_push_back(v->string, c);
}

size_t tt_string_size(const TtString* s) {
    return string_size(s->string);
}

int tt_string_printf(TtString* v, const char format[], ...) {
    va_list args;
    va_start(args, format);
    int result = tt_string_vprintf(v, format, args);
    va_end(args);
    return result;
}

int tt_string_vprintf(TtString* v, const char format[], va_list args) {
    return string_vprintf(v->string, format, args);
}

int tt_string_cat_printf(TtString* v, const char format[], ...) {
    va_list args;
    va_start(args, format);
    int result = tt_string_cat_vprintf(v, format, args);
    va_end(args);
    return result;
}

int tt_string_cat_vprintf(TtString* v, const char format[], va_list args) {
    TtString* string = tt_string_alloc();
    int ret = tt_string_vprintf(string, format, args);
    tt_string_cat(v, string);
    tt_string_free(string);
    return ret;
}

bool tt_string_empty(const TtString* v) {
    return string_empty_p(v->string);
}

void tt_string_replace_at(TtString* v, size_t pos, size_t len, const char str2[]) {
    string_replace_at(v->string, pos, len, str2);
}

size_t
tt_string_replace(TtString* string, TtString* needle, TtString* replace, size_t start) {
    return string_replace(string->string, needle->string, replace->string, start);
}

size_t tt_string_replace_str(TtString* v, const char str1[], const char str2[], size_t start) {
    return string_replace_str(v->string, str1, str2, start);
}

void tt_string_replace_all_str(TtString* v, const char str1[], const char str2[]) {
    string_replace_all_str(v->string, str1, str2);
}

void tt_string_replace_all(TtString* v, const TtString* str1, const TtString* str2) {
    string_replace_all(v->string, str1->string, str2->string);
}

bool tt_string_start_with(const TtString* v, const TtString* v2) {
    return string_start_with_string_p(v->string, v2->string);
}

bool tt_string_start_with_str(const TtString* v, const char str[]) {
    return string_start_with_str_p(v->string, str);
}

bool tt_string_end_with(const TtString* v, const TtString* v2) {
    return string_end_with_string_p(v->string, v2->string);
}

bool tt_string_end_with_str(const TtString* v, const char str[]) {
    return string_end_with_str_p(v->string, str);
}

size_t tt_string_search_char(const TtString* v, char c, size_t start) {
    return string_search_char(v->string, c, start);
}

size_t tt_string_search_rchar(const TtString* v, char c, size_t start) {
    return string_search_rchar(v->string, c, start);
}

void tt_string_left(TtString* v, size_t index) {
    string_left(v->string, index);
}

void tt_string_right(TtString* v, size_t index) {
    string_right(v->string, index);
}

void tt_string_mid(TtString* v, size_t index, size_t size) {
    string_mid(v->string, index, size);
}

void tt_string_trim(TtString* v, const char charac[]) {
    string_strim(v->string, charac);
}

void tt_string_cat(TtString* v, const TtString* v2) {
    string_cat(v->string, v2->string);
}

void tt_string_cat_str(TtString* v, const char str[]) {
    string_cat(v->string, str);
}

void tt_string_set_n(TtString* v, const TtString* ref, size_t offset, size_t length) {
    string_set_n(v->string, ref->string, offset, length);
}

size_t tt_string_utf8_length(TtString* str) {
    return string_length_u(str->string);
}

void tt_string_utf8_push(TtString* str, TtStringUnicodeValue u) {
    string_push_u(str->string, u);
}

static m_str1ng_utf8_state_e furi_state_to_state(TtStringUTF8State state) {
    switch (state) {
        case TtStringUTF8StateStarting:
            return M_STR1NG_UTF8_STARTING;
        case TtStringUTF8StateDecoding1:
            return M_STR1NG_UTF8_DECODING_1;
        case TtStringUTF8StateDecoding2:
            return M_STR1NG_UTF8_DECODING_2;
        case TtStringUTF8StateDecoding3:
            return M_STR1NG_UTF8_DECODING_3;
        default:
            return M_STR1NG_UTF8_ERROR;
    }
}

static TtStringUTF8State state_to_furi_state(m_str1ng_utf8_state_e state) {
    switch (state) {
        case M_STR1NG_UTF8_STARTING:
            return TtStringUTF8StateStarting;
        case M_STR1NG_UTF8_DECODING_1:
            return TtStringUTF8StateDecoding1;
        case M_STR1NG_UTF8_DECODING_2:
            return TtStringUTF8StateDecoding2;
        case M_STR1NG_UTF8_DECODING_3:
            return TtStringUTF8StateDecoding3;
        default:
            return TtStringUTF8StateError;
    }
}

void tt_string_utf8_decode(char c, TtStringUTF8State* state, TtStringUnicodeValue* unicode) {
    string_unicode_t m_u = *unicode;
    m_str1ng_utf8_state_e m_state = furi_state_to_state(*state);
    m_str1ng_utf8_decode(c, &m_state, &m_u);
    *state = state_to_furi_state(m_state);
    *unicode = m_u;
}

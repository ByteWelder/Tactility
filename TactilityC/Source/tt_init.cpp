#ifdef ESP_PLATFORM

#include "private/elf_symbol.h"
#include "tt_app_alertdialog.h"
#include "tt_app_fileselection.h"
#include "tt_app_selectiondialog.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <ctime>
#include <ctype.h>
#include <dirent.h>
#include <driver/ledc.h>
#include <esp_heap_caps.h>
#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <miniz.h>
#include <setjmp.h>
#include <strings.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <vector>

#include <Tactility/Tactility.h>

#include <driver/i2s_common.h>
#include <driver/i2s_std.h>
#include <driver/gpio.h>

#ifdef CONFIG_IDF_TARGET_ESP32P4
#include <driver/ppa.h>
#include <esp_cache.h>
#endif

extern "C" {

extern double __floatsidf(int x);
extern void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression);

const esp_elfsym main_symbols[] {
    // stdlib.h
    ESP_ELFSYM_EXPORT(malloc),
    ESP_ELFSYM_EXPORT(calloc),
    ESP_ELFSYM_EXPORT(realloc),
    ESP_ELFSYM_EXPORT(free),
    ESP_ELFSYM_EXPORT(rand),
    ESP_ELFSYM_EXPORT(srand),
    ESP_ELFSYM_EXPORT(rand_r),
    ESP_ELFSYM_EXPORT(atof),
    ESP_ELFSYM_EXPORT(atoi),
    ESP_ELFSYM_EXPORT(atol),
    ESP_ELFSYM_EXPORT(system),
    // unistd.h
    ESP_ELFSYM_EXPORT(usleep),
    ESP_ELFSYM_EXPORT(sleep),
    ESP_ELFSYM_EXPORT(exit),
    ESP_ELFSYM_EXPORT(close),
    ESP_ELFSYM_EXPORT(rmdir),
    ESP_ELFSYM_EXPORT(unlink),
    ESP_ELFSYM_EXPORT(open),
    // strings.h
    ESP_ELFSYM_EXPORT(explicit_bzero),
    ESP_ELFSYM_EXPORT(strcasecmp),
    ESP_ELFSYM_EXPORT(strncasecmp),
    // time.h
    ESP_ELFSYM_EXPORT(clock_gettime),
    ESP_ELFSYM_EXPORT(strftime),
    ESP_ELFSYM_EXPORT(time),
    ESP_ELFSYM_EXPORT(difftime),
    ESP_ELFSYM_EXPORT(localtime_r),
    ESP_ELFSYM_EXPORT(localtime),
    ESP_ELFSYM_EXPORT(mktime),
    // math.h
    ESP_ELFSYM_EXPORT(acos),
    ESP_ELFSYM_EXPORT(acoshf),
    ESP_ELFSYM_EXPORT(acosf),
    ESP_ELFSYM_EXPORT(asin),
    ESP_ELFSYM_EXPORT(asinhf),
    ESP_ELFSYM_EXPORT(asinf),
    ESP_ELFSYM_EXPORT(atan),
    ESP_ELFSYM_EXPORT(atanhf),
    ESP_ELFSYM_EXPORT(atanf),
    ESP_ELFSYM_EXPORT(cos),
    ESP_ELFSYM_EXPORT(coshf),
    ESP_ELFSYM_EXPORT(cosf),
    ESP_ELFSYM_EXPORT(sin),
    ESP_ELFSYM_EXPORT(sinhf),
    ESP_ELFSYM_EXPORT(sinf),
    ESP_ELFSYM_EXPORT(tan),
    ESP_ELFSYM_EXPORT(tanhf),
    ESP_ELFSYM_EXPORT(tanf),
    ESP_ELFSYM_EXPORT(frexp),
    ESP_ELFSYM_EXPORT(frexpf),
    ESP_ELFSYM_EXPORT(modf),
    ESP_ELFSYM_EXPORT(modff),
    ESP_ELFSYM_EXPORT(ceil),
    ESP_ELFSYM_EXPORT(ceilf),
    ESP_ELFSYM_EXPORT(fabs),
    ESP_ELFSYM_EXPORT(fabsf),
    ESP_ELFSYM_EXPORT(floor),
    ESP_ELFSYM_EXPORT(floorf),
    ESP_ELFSYM_EXPORT(fmax),
    ESP_ELFSYM_EXPORT(fmaxf),
    ESP_ELFSYM_EXPORT(fmin),
    ESP_ELFSYM_EXPORT(fminf),
    ESP_ELFSYM_EXPORT(round),
    ESP_ELFSYM_EXPORT(roundf),
#ifndef _REENT_ONLY
    ESP_ELFSYM_EXPORT(acos),
    ESP_ELFSYM_EXPORT(acosf),
    ESP_ELFSYM_EXPORT(asin),
    ESP_ELFSYM_EXPORT(asinf),
    ESP_ELFSYM_EXPORT(atan2),
    ESP_ELFSYM_EXPORT(atan2f),
    ESP_ELFSYM_EXPORT(sinh),
    ESP_ELFSYM_EXPORT(sinhf),
    ESP_ELFSYM_EXPORT(exp),
    ESP_ELFSYM_EXPORT(expf),
    ESP_ELFSYM_EXPORT(ldexp),
    ESP_ELFSYM_EXPORT(ldexpf),
    ESP_ELFSYM_EXPORT(log),
    ESP_ELFSYM_EXPORT(logf),
    ESP_ELFSYM_EXPORT(log10),
    ESP_ELFSYM_EXPORT(log10f),
    ESP_ELFSYM_EXPORT(pow),
    ESP_ELFSYM_EXPORT(powf),
    ESP_ELFSYM_EXPORT(sqrt),
    ESP_ELFSYM_EXPORT(sqrtf),
    ESP_ELFSYM_EXPORT(fmod),
    ESP_ELFSYM_EXPORT(fmodf),
#endif
#ifdef __HAVE_LOCALE_INFO__
    // ctype.h
    ESP_ELFSYM_EXPORT(__locale_ctype_ptr),
#else
    ESP_ELFSYM_EXPORT(_ctype_),
#endif
    // getopt.h
    ESP_ELFSYM_EXPORT(getopt_long),
    ESP_ELFSYM_EXPORT(optind),
    ESP_ELFSYM_EXPORT(opterr),
    ESP_ELFSYM_EXPORT(optarg),
    ESP_ELFSYM_EXPORT(optopt),
    // setjmp.h
    ESP_ELFSYM_EXPORT(longjmp),
    ESP_ELFSYM_EXPORT(setjmp),
    // cassert
    ESP_ELFSYM_EXPORT(__assert_func),
    // cstdio
    ESP_ELFSYM_EXPORT(abort),
    ESP_ELFSYM_EXPORT(fclose),
    ESP_ELFSYM_EXPORT(feof),
    ESP_ELFSYM_EXPORT(ferror),
    ESP_ELFSYM_EXPORT(fflush),
    ESP_ELFSYM_EXPORT(fgetc),
    ESP_ELFSYM_EXPORT(fgetpos),
    ESP_ELFSYM_EXPORT(fgets),
    ESP_ELFSYM_EXPORT(fopen),
    ESP_ELFSYM_EXPORT(freopen),
    // Lets an app find the descriptor behind a stream. Needed when stdin/stdout have been pointed
    // somewhere other than descriptors 0 and 1, which is the case for an app that owns a terminal.
    ESP_ELFSYM_EXPORT(fileno),
    ESP_ELFSYM_EXPORT(setvbuf),
    ESP_ELFSYM_EXPORT(fputc),
    ESP_ELFSYM_EXPORT(fputs),
    ESP_ELFSYM_EXPORT(fprintf),
    ESP_ELFSYM_EXPORT(fread),
    ESP_ELFSYM_EXPORT(fseek),
    ESP_ELFSYM_EXPORT(fsetpos),
    ESP_ELFSYM_EXPORT(fscanf),
    ESP_ELFSYM_EXPORT(ftell),
    ESP_ELFSYM_EXPORT(fwrite),
    ESP_ELFSYM_EXPORT(getc),
    ESP_ELFSYM_EXPORT(putc),
    ESP_ELFSYM_EXPORT(putchar),
    ESP_ELFSYM_EXPORT(puts),
    ESP_ELFSYM_EXPORT(printf),
    ESP_ELFSYM_EXPORT(sscanf),
    ESP_ELFSYM_EXPORT(snprintf),
    ESP_ELFSYM_EXPORT(sprintf),
    ESP_ELFSYM_EXPORT(vsprintf),
    ESP_ELFSYM_EXPORT(vsnprintf),
    ESP_ELFSYM_EXPORT(vfprintf),
    // cstring
    ESP_ELFSYM_EXPORT(strlen),
    ESP_ELFSYM_EXPORT(strcmp),
    ESP_ELFSYM_EXPORT(strncmp),
    ESP_ELFSYM_EXPORT(strncpy),
    ESP_ELFSYM_EXPORT(strcpy),
    ESP_ELFSYM_EXPORT(strcat),
    ESP_ELFSYM_EXPORT(strchr),
    ESP_ELFSYM_EXPORT(strstr),
    ESP_ELFSYM_EXPORT(strerror),
    ESP_ELFSYM_EXPORT(strtod),
    ESP_ELFSYM_EXPORT(strrchr),
    ESP_ELFSYM_EXPORT(strtol),
    ESP_ELFSYM_EXPORT(strtoul),
    ESP_ELFSYM_EXPORT(strcspn),
    ESP_ELFSYM_EXPORT(strncat),
    ESP_ELFSYM_EXPORT(strpbrk),
    ESP_ELFSYM_EXPORT(strspn),
    ESP_ELFSYM_EXPORT(strcoll),
    ESP_ELFSYM_EXPORT(memset),
    ESP_ELFSYM_EXPORT(memcpy),
    ESP_ELFSYM_EXPORT(memcmp),
    ESP_ELFSYM_EXPORT(memchr),
    ESP_ELFSYM_EXPORT(memmove),
    ESP_ELFSYM_EXPORT(strdup),
    ESP_ELFSYM_EXPORT(stpcpy),

    // ctype
    ESP_ELFSYM_EXPORT(isalnum),
    ESP_ELFSYM_EXPORT(isalpha),
    ESP_ELFSYM_EXPORT(iscntrl),
    ESP_ELFSYM_EXPORT(isdigit),
    ESP_ELFSYM_EXPORT(isgraph),
    ESP_ELFSYM_EXPORT(islower),
    ESP_ELFSYM_EXPORT(isprint),
    ESP_ELFSYM_EXPORT(ispunct),
    ESP_ELFSYM_EXPORT(isspace),
    ESP_ELFSYM_EXPORT(isupper),
    ESP_ELFSYM_EXPORT(isxdigit),
    ESP_ELFSYM_EXPORT(tolower),
    ESP_ELFSYM_EXPORT(toupper),

    // Tactility
    ESP_ELFSYM_EXPORT(tt_app_fileselection_start_for_existing_file),
    ESP_ELFSYM_EXPORT(tt_app_fileselection_start_for_existing_or_new_file),
    ESP_ELFSYM_EXPORT(tt_app_fileselection_get_result_path),
    ESP_ELFSYM_EXPORT(tt_app_selectiondialog_start),
    ESP_ELFSYM_EXPORT(tt_app_alertdialog_start),

    // stdio.h
    ESP_ELFSYM_EXPORT(rename),
    ESP_ELFSYM_EXPORT(rewind),
    ESP_ELFSYM_EXPORT(remove),
    // dirent.h
    ESP_ELFSYM_EXPORT(opendir),
    ESP_ELFSYM_EXPORT(closedir),
    ESP_ELFSYM_EXPORT(readdir),
    // fcntl.h
    ESP_ELFSYM_EXPORT(fcntl),

    // stdlib.h - environment and sorting
    ESP_ELFSYM_EXPORT(getenv),
    ESP_ELFSYM_EXPORT(setenv),
    ESP_ELFSYM_EXPORT(unsetenv),
    ESP_ELFSYM_EXPORT(qsort),
    // unistd.h
    ESP_ELFSYM_EXPORT(access),
    ESP_ELFSYM_EXPORT(isatty),
    ESP_ELFSYM_EXPORT(read),
    ESP_ELFSYM_EXPORT(write),
    ESP_ELFSYM_EXPORT(lseek),
    // sys/stat.h
    ESP_ELFSYM_EXPORT(stat),
    ESP_ELFSYM_EXPORT(mkdir),
    // Locale
    ESP_ELFSYM_EXPORT(localeconv),
    // delimiter
    ESP_ELFSYM_END,
};

uintptr_t resolve_symbol(const esp_elfsym* source, const char* symbolName) {
    const esp_elfsym* symbol_iterator = source;
    while (symbol_iterator->name != nullptr) {
        if (strcmp(symbol_iterator->name, symbolName) == 0) {
            return reinterpret_cast<uintptr_t>(symbol_iterator->sym);
        }
        symbol_iterator++;
    }
    return 0;
}

uintptr_t tt_symbol_resolver(const char* symbolName) {
    static const std::vector all_symbols = {
        main_symbols,
    };

    for (const auto* symbols : all_symbols) {
        const uintptr_t address = resolve_symbol(symbols, symbolName);
        if (address != 0) {
            return address;
        }
    }

    uintptr_t symbol_address;
    if (module_resolve_symbol_global(symbolName, &symbol_address)) {
        return symbol_address;
    }

    return 0;
}

void tt_init_tactility_c() {
    elf_set_symbol_resolver(tt_symbol_resolver);
}

}

// extern "C"

#else // Simulator

extern "C" {

void tt_init_tactility_c() {
}

}

#endif // ESP_PLATFORM

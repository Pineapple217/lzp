#include "../lzp_core.h"
#include "../mpc.h"
#include "time.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <stdint.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

long long get_time_millis() {
    FILETIME ft;
    ULARGE_INTEGER uli;

    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // Convert to Unix epoch milliseconds
    return (long long)((uli.QuadPart / 10000) - 11644473600000ULL);
}

#elif defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>

long long get_time_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

#else
#error "Unsupported platform"
#endif


EXPORT void lzp_plugin_init(lenv* env);

lval* builtin_time_milli(lenv* e, lval* a) {
    long long millis = get_time_millis();
    lval_del(a);
    return lval_num(millis);
}

lval* builtin_time(lenv* e, lval* a) {
    long long now = time(NULL);

    lval_del(a);
    return lval_num(now);
}

void lzp_plugin_init(lenv* env) {
    init_parser();
    lenv_add_builtin(env, "time", builtin_time);
    lenv_add_builtin(env, "time-milli", builtin_time_milli);

    read_xxd(env, time_script, time_script_len);
}

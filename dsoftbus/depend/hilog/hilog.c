#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "securec.h"
#include "hilog_base/log_base.h"

#ifndef LOG_PRINT_MAX_LEN
#define LOG_PRINT_MAX_LEN 256
#endif

char *adapterStrForPrintfFormat(const char *fmt) {
    char *left, *right;
    char *buffer = (char *)malloc(LOG_PRINT_MAX_LEN * sizeof(char));
    (void)memset_s(buffer, LOG_PRINT_MAX_LEN * sizeof(char), 0, LOG_PRINT_MAX_LEN * sizeof(char));
    strcpy_s(buffer, LOG_PRINT_MAX_LEN * sizeof(char), fmt);
    while (strstr(buffer, "{")) {
        left = strstr(buffer, "{");
        right = strstr(buffer, "}");
        right++;
        while (*right != '\0') {
            *left = *right;
            left++;
            right++;
        }
        *left = '\0';
    }
    return buffer;
}

void printfAdapter(const char *fmt, ...) {
    char *buffer;
    buffer = adapterStrForPrintfFormat(fmt);
    va_list ap;
    va_start(ap, fmt);
    vprintf(buffer, ap);
    va_end(ap);
    free(buffer);
}

#ifndef _HILOG_H
#define _HILOG_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// Log type
typedef enum {
    LOG_TYPE_MIN = 0,
    LOG_APP = 0,
    // Log to kmsg, only used by init phase.
    LOG_INIT = 1,
    // Used by core service, framework.
    LOG_CORE = 3,
    LOG_TYPE_MAX
} LogType;

char *adapterStrForPrintfFormat(const char *fmt);
void printfAdapter(const char *fmt, ...);

#define HILOG_DEBUG(type, fmt, ...) printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HILOG_INFO(type, fmt, ...)  printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HILOG_WARN(type, fmt, ...)  printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HILOG_ERROR(type, fmt, ...) printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HiLogPrint(type, level, domain, tag, fmt, ...) printf(fmt"\n", ##__VA_ARGS__)
#define HiLogBasePrint(type, level, domain, tag, fmt, ...) printfAdapter(fmt"\n", ##__VA_ARGS__)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

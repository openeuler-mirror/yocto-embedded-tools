#ifndef _HILOG_H
#define _HILOG_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


char *adapterStrForPrintfFormat(const char *fmt);
void printfAdapter(const char *fmt, ...);

#define HILOG_DEBUG(type, fmt, ...) printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HILOG_INFO(type, fmt, ...)  printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HILOG_WARN(type, fmt, ...)  printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HILOG_ERROR(type, fmt, ...) printfAdapter(fmt"\n", ##__VA_ARGS__)
#define HiLogPrint(type, level, domain, tag, fmt, ...) printf(fmt"\n", ##__VA_ARGS__)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

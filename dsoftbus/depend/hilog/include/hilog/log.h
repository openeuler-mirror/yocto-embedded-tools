#ifndef _HILOG_H
#define _HILOG_H

#include <stdio.h>

#define HILOG_DEBUG(type, fmt, ...) printf(fmt, ##__VA_ARGS__)
#define HILOG_INFO(type, fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define HILOG_WARN(type, fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define HILOG_ERROR(type, fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif

#ifndef RPMSG_MODULE_H
#define RPMSG_MODULE_H

#include <openamp/rpmsg.h>

#define DEV_CLIENT_OS_AGENT        "/dev/cpu_handler"

/* callback of ns */
void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest);

#endif
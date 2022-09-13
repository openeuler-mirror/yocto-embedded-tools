#ifndef RPMSG_MODULE_H
#define RPMSG_MODULE_H

#include <openamp/rpmsg.h>

/* callback of ns */
void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest);

#endif
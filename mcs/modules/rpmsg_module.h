#ifndef RPMSG_MODULE_H
#define RPMSG_MODULE_H

#include <openamp/rpmsg.h>

/* endpoint name */
#define CONSOLE_ENDPOINT 	"console_ept"
#define LOG_ENDPOINT     	"log_ept"
#define SHELL_ENDPOINT   	"shell_ept"

/* name service callback: create matching endpoint */
void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest);

/* destroy endpoint and inform clientos to destroy */
void destroy_endpoint(const char *name);

void rpmsg_module_init(void);

#endif
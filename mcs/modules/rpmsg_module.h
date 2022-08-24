#ifndef RPMSG_MODULE_H
#define RPMSG_MODULE_H

#include <openamp/rpmsg.h>

#define DEV_CLIENT_OS_AGENT        "/dev/cpu_handler"

/* callback of ns */
void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest);

/* bringup a endpoint, receive_message and send_message are based on this */
void bringup_endpoint(struct rpmsg_endpoint *ept_inst);

/* message standard receive interface */
int receive_message(unsigned char *message, int message_len, int *real_len);

/* message standard send interface */
int send_message(unsigned char *message, int len);

#endif
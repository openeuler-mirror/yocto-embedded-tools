#ifndef OPENAMP_MODULE_H
#define OPENAMP_MODULE_H

#include <openamp/open_amp.h>
#include "remoteproc_module.h"
#include "virtio_module.h"
#include "rpmsg_module.h"

extern char *target_binfile;
extern char *target_binaddr;

/* initialize openamp module, including remoteproc, virtio, rpmsg */
int openamp_init(void);

/* release openamp resource */
int openamp_deinit(void);

/* message standard receive interface */
int receive_message(unsigned char *message, int message_len, int *real_len);

/* message standard send interface */
int send_message(unsigned char *message, int len);

#endif
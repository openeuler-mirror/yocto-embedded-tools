#ifndef OPENAMP_MODULE_H
#define OPENAMP_MODULE_H

#include <openamp/open_amp.h>
#include "remoteproc_module.h"
#include "virtio_module.h"
#include "rpmsg_module.h"

extern char *target_binfile;
extern char *target_binaddr;

int openamp_init(void);
int openamp_deinit(void);

#endif
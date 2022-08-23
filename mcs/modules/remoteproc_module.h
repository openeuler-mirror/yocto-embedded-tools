#ifndef REMOTEPROC_MODULE_H
#define REMOTEPROC_MODULE_H

#include <openamp/remoteproc.h>

#define DEV_CLIENT_OS_AGENT        "/dev/cpu_handler"

struct rproc_priv {
    struct remoteproc *rproc;  /* pass a remoteproc instance pointer */
    unsigned int idx;          /* remoteproc instance idx */
    unsigned int cpu_id;       /* related arg: cpu id */
    unsigned int boot_address; /* related arg: boot address(in hex format) */
};

/* create remoteproc */
struct remoteproc *platform_create_proc(struct rproc_priv *args);

/*
 start remoteproc: refet to <openamp/remoteproc.h>
	int remoteproc_start(struct remoteproc *rproc);
*/

/*
 stop remoteproc: refet to <openamp/remoteproc.h>
	int remoteproc_stop(struct remoteproc *rproc);
*/

/*
 remove remoteproc: refet to <openamp/remoteproc.h>
	int remoteproc_remove(struct remoteproc *rproc);
*/

#endif
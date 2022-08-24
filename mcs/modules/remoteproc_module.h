#ifndef REMOTEPROC_MODULE_H
#define REMOTEPROC_MODULE_H

#include <openamp/remoteproc.h>

#define DEV_CLIENT_OS_AGENT        "/dev/cpu_handler"

/* create remoteproc */
struct remoteproc *create_remoteproc(void);

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

extern char *cpu_id;
extern char *boot_address;
extern struct remoteproc rproc_inst;

#endif
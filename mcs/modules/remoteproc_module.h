#ifndef REMOTEPROC_MODULE_H
#define REMOTEPROC_MODULE_H

#include <openamp/remoteproc.h>

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

/* destory remoteproc */
void destory_remoteproc(void);

extern char *cpu_id;
extern char *target_binaddr;

#endif
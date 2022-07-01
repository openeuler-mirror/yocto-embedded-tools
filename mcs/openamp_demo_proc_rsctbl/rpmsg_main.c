#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openamp/rsc_table_parser.h>

#include "rpmsg-internal.h"

int pipefd1[2];
int pipefd2[2];

/* Place resource table in special ELF section */
#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)

#define NUM_TABLE_ENTRIES           2
#define DFEATURE_SUPPORT_NS         1

struct remote_resource_table __resource resources = {
    /* Version */
    RSC_TAB_SUPPORTED_VERSION,
    /* NUmber of table entries */
    NUM_TABLE_ENTRIES,
    /* reserved fields */
    { 0, 0 },
    /* Offsets of rsc entries */
    {      
     offsetof(struct remote_resource_table, rpmsg_mem),
     offsetof(struct remote_resource_table, rpmsg_vdev),
    },

    /* 1. Rpmsg buffer memory entry */
    { RSC_CARVEOUT, SHM_START_ADDR, SHM_START_ADDR, SHM_SIZE, 0 },

    /* 2. Virtio device entry */
    { RSC_VDEV, VIRTIO_ID_RPMSG, VDEV_NOTIFY_ID, DFEATURE_SUPPORT_NS, 0, 0, 0, VRING_COUNT, { 0, 0 } },
    /* Vring rsc entry - part of vdev rsc entry */
    { VRING_RX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, VRING_RX_NOTIFY_ID, 0 }, /* VRING_RX */
    { VRING_TX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, VRING_TX_NOTIFY_ID, 0 }, /* VRING_TX */
};

void *get_resource_table(int *len)
{
    *len = sizeof(resources);
    return &resources;
}

extern int rpmsg_linux_endpoint();
extern int rpmsg_rtos_endpoint();

int main(int argc, char **argv)
{
    pid_t pid;

    /* Create unamed pipe */
    if (pipe(pipefd1) < 0) {
        perror("pipe1");
        return -1;
    }
    if (pipe(pipefd2) < 0) {
        perror("pipe2");
        return -1;
    }

    if ((pid = fork()) < 0) {
        perror("fork");
        return -1;
    } else if (pid > 0) {
        /* parent: Linux Endpoint */
        rpmsg_linux_endpoint();
    } else {
        /* child: RTOS Endpoint */
        rpmsg_rtos_endpoint();
    }

    return 0;
}

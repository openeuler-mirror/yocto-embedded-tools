#ifndef RPMSG_INTERNAL_H_
#define RPMSG_INTERNAL_H_

#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include <openamp/rpmsg_virtio.h>

#define SHM_START_ADDR             0x70000000
#define SHM_SIZE                   0x30000

#define VRING_ADDR_SIZE            0x4000
#define VRING_RX_ADDRESS           (SHM_START_ADDR + SHM_SIZE)
#define VRING_TX_ADDRESS           (VRING_RX_ADDRESS + VRING_ADDR_SIZE)
#define RTOS_VRING_RX_ADDRESS      VRING_TX_ADDRESS
#define RTOS_VRING_TX_ADDRESS      VRING_RX_ADDRESS

#define VRING_COUNT                2
#define VRING_ALIGNMENT            4
#define VRING_SIZE                 16

#define RSC_TABLE_ADDR             0x71000000

#define VDEV_NOTIFY_ID             0
#define VRING_RX_NOTIFY_ID         1
#define VRING_TX_NOTIFY_ID         2

struct rproc_priv {
    struct remoteproc *rproc;
    unsigned int id;
};

#define NO_RESOURCE_ENTRIES        8

/* Resource table for the given remote */
struct remote_resource_table {
    unsigned int version;
    unsigned int num;
    unsigned int reserved[2];
    unsigned int offset[NO_RESOURCE_ENTRIES];

    /* 1. rpmsg buffer mem entry */
    struct fw_rsc_carveout rpmsg_mem;

    /* 2. rpmsg vdev entry */
    struct fw_rsc_vdev rpmsg_vdev;
    struct fw_rsc_vdev_vring rpmsg_vring0;
    struct fw_rsc_vdev_vring rpmsg_vring1;
} __attribute__((__packed__));

extern int pipefd1[2];
extern int pipefd2[2];

void *get_resource_table(int *len);

#endif

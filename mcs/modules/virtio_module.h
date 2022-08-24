#ifndef VIRTIO_MODULE_H
#define VIRTIO_MODULE_H

#include <metal/io.h>
#include <openamp/rpmsg_virtio.h>

#define VDEV_START_ADDR            0x70000000
#define VDEV_SIZE                  0x30000

#define VDEV_STATUS_ADDR           VDEV_START_ADDR
#define VDEV_STATUS_SIZE           0x4000

#define SHM_START_ADDR             (VDEV_START_ADDR + VDEV_STATUS_SIZE)
#define SHM_SIZE                   (VDEV_SIZE - VDEV_STATUS_SIZE)

#define VRING_COUNT                2
#define VRING_RX_ADDRESS           (VDEV_START_ADDR + SHM_SIZE - VDEV_STATUS_SIZE)
#define VRING_TX_ADDRESS           (VDEV_START_ADDR + SHM_SIZE)
#define VRING_ALIGNMENT            4
#define VRING_SIZE                 16

#define IRQ_SENDTO_CLIENTOS        _IOW('A', 0, int)
#define DEV_CLIENT_OS_AGENT        "/dev/cpu_handler"

void virtio_init(void);

extern char *cpu_id;
extern struct metal_io_region *io;
extern struct virtqueue *vq[2];

#endif
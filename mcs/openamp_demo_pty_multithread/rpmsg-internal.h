#ifndef RPMSG_INTERNAL_H_
#define RPMSG_INTERNAL_H_

#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
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

extern char *cpu_id;
extern struct metal_io_region *io;

struct thread_args {
    int fd;
    int *pipefd;
};

struct remoteproc *platform_create_proc(unsigned int id);
int load_bin(void);
void rpmsg_endpoint_init(void);
int rpmsg_endpoint_app(int fds, int ns_setup);

void open_pty(int *pfdm, int *pfds);
void *master(void *arg_list);
void *slave(void *arg_list);

#endif
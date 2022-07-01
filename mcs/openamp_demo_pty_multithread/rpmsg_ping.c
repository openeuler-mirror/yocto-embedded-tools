#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <openamp/open_amp.h>
#include <metal/device.h>

#include "rpmsg-internal.h"

static struct virtio_vring_info rvrings[2] = {
	[0] = {
		.info.align = VRING_ALIGNMENT,
	},
	[1] = {
		.info.align = VRING_ALIGNMENT,
	},
};

static int g_memfd;
volatile unsigned int received_data;
static struct virtio_device vdev;
static struct rpmsg_virtio_device rvdev;
struct metal_io_region *io;
struct virtqueue *vq[2];
static void *tx_addr, *rx_addr, *shm_start_addr;
static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return VIRTIO_CONFIG_STATUS_DRIVER_OK;
}

static void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	void *stat_addr = NULL;
	stat_addr = mmap((void *)VDEV_STATUS_ADDR, VDEV_STATUS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_memfd, VDEV_STATUS_ADDR);
	*(volatile unsigned int *)stat_addr = (unsigned int)status;
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return 1 << VIRTIO_RPMSG_F_NS;
}

static void virtio_notify(struct virtqueue *vq)
{
	(void)vq;
	int cpu_handler_fd;
	int ret;
	int cpu_num = strtol(cpu_id, NULL, 0);

	cpu_handler_fd = open(DEV_CLIENT_OS_AGENT, O_RDWR);
	if (cpu_handler_fd < 0) {
		printf("open %s failed\n", DEV_CLIENT_OS_AGENT);
		return;
	}

	ret = ioctl(cpu_handler_fd, IRQ_SENDTO_CLIENTOS, cpu_num);
	if (ret) {
		printf("send ipi tp second os failed\n");
	}

	close(cpu_handler_fd);
	return;
}

struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.notify = virtio_notify,
};

int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	received_data = *((unsigned int *) data);
	return RPMSG_SUCCESS;
}

struct rpmsg_endpoint my_ept;
struct rpmsg_endpoint *ep = &my_ept;

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	rpmsg_destroy_ept(ep);
}

void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	(void)rpmsg_create_ept(ep, rdev, name,
			RPMSG_ADDR_ANY, dest,
			endpoint_cb,
			rpmsg_service_unbind);
}

static struct rpmsg_virtio_shm_pool shpool;

void rpmsg_endpoint_init(void)
{
	int status = 0;

	g_memfd = open("/dev/mem", O_RDWR);
	tx_addr = mmap((void *)VRING_TX_ADDRESS, VDEV_STATUS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_memfd, VRING_TX_ADDRESS);
	rx_addr = mmap((void *)VRING_RX_ADDRESS, VDEV_STATUS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_memfd, VRING_RX_ADDRESS);
	shm_start_addr = mmap((void *)SHM_START_ADDR, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_memfd, SHM_START_ADDR);

	io = malloc(sizeof(struct metal_io_region));
	if (!io) {
		printf("malloc io failed\n");
		return;
	}
	metal_io_init(io, shm_start_addr, shm_physmap, SHM_SIZE, -1, 0, NULL);

	/* setup vdev */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (vq[0] == NULL) {
		printf("virtqueue_allocate failed to alloc vq[0]\n");
        free(io);
		return;
	}
	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (vq[1] == NULL) {
		printf("virtqueue_allocate failed to alloc vq[1]\n");
        free(io);
		return;
	}

	vdev.role = RPMSG_MASTER;
	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	rvrings[0].io = io;
	rvrings[0].info.vaddr = tx_addr;
	rvrings[0].info.num_descs = VRING_SIZE;
	rvrings[0].info.align = VRING_ALIGNMENT;
	rvrings[0].vq = vq[0];

	rvrings[1].io = io;
	rvrings[1].info.vaddr = rx_addr;
	rvrings[1].info.num_descs = VRING_SIZE;
	rvrings[1].info.align = VRING_ALIGNMENT;
	rvrings[1].vq = vq[1];

	vdev.vrings_info = &rvrings[0];

	/* setup rvdev */
	rpmsg_virtio_init_shm_pool(&shpool, shm_start_addr, SHM_SIZE);
	status = rpmsg_init_vdev(&rvdev, &vdev, ns_bind_cb, io, &shpool);
	if (status != 0) {
		printf("rpmsg_init_vdev failed %d\n", status);
		free(io);
		return;
	}
}

int rpmsg_endpoint_app(int fds, int ns_setup)
{
	int cpu_handler_fd;
	struct pollfd poll_fd[2] = {0};

	cpu_handler_fd = open(DEV_CLIENT_OS_AGENT, O_RDWR);
	if (cpu_handler_fd < 0) {
	    printf("open %s failed.\n", DEV_CLIENT_OS_AGENT);
	    return -1;
	}

	poll_fd[0].fd = fds;
	poll_fd[0].events = POLLIN;
	poll_fd[1].fd = cpu_handler_fd;
	poll_fd[1].events = POLLIN;

	/* Wait for data from slave side of PTY, and data from endpoint of openamp, blocked */
	if (poll(poll_fd, 2, -1) == -1) {
	    printf("Error %d on poll()\n", errno);
	    goto err;
	}

	close(cpu_handler_fd);

	/* If data on slave side of PTY */
	if (poll_fd[0].revents & POLLIN) {
	    char recv_buf[200] = {0};
	    unsigned int number;

	    poll_fd[0].revents = 0;
	    if (read(fds, recv_buf, sizeof(recv_buf)) > 0) {
	        number = atoi(recv_buf);
	        //printf("Master core sending messages: %d\n", number);
	        rpmsg_send(ep, &number, sizeof(number));
	    }
	}

	/* If data on endpoint of openamp */
	if (poll_fd[1].revents & POLLIN) {
	    char send_buf[200] = {0};
	    int wc;

	    poll_fd[1].revents = 0;
	    virtqueue_notification(vq[0]);
	    //printf("Master core receiving messages: %d\n", received_data);

	    /* Check whether poll event is ns setup */
	    if (ns_setup == 1)
	    	return 0;

	    received_data++;
	    wc = snprintf(send_buf, sizeof(send_buf), "Output number: %d\n", received_data);
	    if (write(fds, send_buf, wc) == -1) {
	        printf("Error %d on rpmsg_endpoint_app write()\n", errno);
	        goto err;
	    }
	}

	return 0;
err:
    close(cpu_handler_fd);
    return -1;
}

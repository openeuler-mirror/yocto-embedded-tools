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
static volatile unsigned int received_data;
static struct virtio_device vdev;
static struct rpmsg_virtio_device rvdev;
static struct metal_io_region *io;
static struct virtqueue *vq[2];

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

static unsigned int receive_message(void)
{
	int cpu_handler_fd;
	struct pollfd fds;
	int ret;
	
	cpu_handler_fd = open(DEV_CLIENT_OS_AGENT, O_RDWR);
	if (cpu_handler_fd < 0) {
		printf("open %s failed.\n", DEV_CLIENT_OS_AGENT);
		return cpu_handler_fd;
	}
	
	fds.fd = cpu_handler_fd;
	fds.events = POLLIN;
	
	printf("Master core waiting for messages.... \n");
	while (1) {
		ret = poll(&fds, 1, -1);
		if (ret < 0) {
			printf("poll error.\n");
			close(cpu_handler_fd);
			return ret;
		}
		
		if (ret == 0) {
			printf("poll time out.\n");
			close(cpu_handler_fd);
			return ret;
		}
		
		if (fds.revents & POLLIN) {
			printf("Master core receiving messages....\n");
			virtqueue_notification(vq[0]);
			break;
		}
	}
	close(cpu_handler_fd);
	return received_data;
}

static int send_message(unsigned int message)
{
	return rpmsg_send(ep, &message, sizeof(message));
}

static struct rpmsg_virtio_shm_pool shpool;

void rpmsg_app_master(void)
{
	int status = 0;
	unsigned int message = 0U;
	void *tx_addr, *rx_addr, *shm_start_addr;
	metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };

	printf("\r\nOpenAMP[master] demo started\r\n");

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

	vdev.role = RPMSG_HOST;
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

	/* Since we are using name service, we need to wait for a response
	 * from NS setup and than we need to process it
	 */
	receive_message();

	while (message < 99) {
		status = send_message(message);
		if (status < 0) {
			printf("send_message(%u) failed with status %d\n",
			       message, status);
			goto _cleanup;
		}

		sleep(1);
		message = receive_message();
		printf("Master core received a message: %u\n", message);

		message++;
		sleep(1);
	}

_cleanup:
	free(io);
	printf("OpenAMP demo ended.\n");
}

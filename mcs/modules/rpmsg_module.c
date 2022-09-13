#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include "virtio_module.h"
#include "rpmsg_module.h"

#define MCS_DEVICE_NAME "/dev/mcs"

static unsigned char received_data[2048] = {0};
static unsigned int received_len = 0;
struct rpmsg_endpoint ept_inst;

int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	memcpy(received_data + received_len, data, len);
	received_len += len;

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	rpmsg_destroy_ept(&ept_inst);
}

void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	(void)rpmsg_create_ept(&ept_inst, rdev, name,
			RPMSG_ADDR_ANY, dest,
			endpoint_cb,
			rpmsg_service_unbind);
}

int receive_message(unsigned char *message, int message_len, int *real_len)
{
	int ret;
	int cpu_handler_fd;
	struct pollfd fds;

	cpu_handler_fd = open(MCS_DEVICE_NAME, O_RDWR);
	if (cpu_handler_fd < 0) {
		printf("receive_message: open %s device failed.\n", MCS_DEVICE_NAME);
		return cpu_handler_fd;
	}

	fds.fd = cpu_handler_fd;
	fds.events = POLLIN;

	/* clear the receive buffer */
	memset(received_data, 0, sizeof(received_data));
	received_len = 0;

	while (1) {
#ifdef DEBUG
		printf("master waiting for message....\n");
#endif
		ret = poll(&fds, 1, 100); /* 100ms timeout */
		if (ret < 0) {
			printf("receive_message: poll failed.\n");
			*real_len = 0;
			goto _cleanup;
		}

		if (ret == 0) {
#ifdef DEBUG
			printf("master waiting for message timeout....\n");
#endif
			break;
		}

		if (fds.revents & POLLIN) {
#ifdef DEBUG
			printf("master receiving message....\n");
#endif
			virtqueue_notification(vq[0]);  /* will call endpoint_cb */
		}
	}

	if (received_len > message_len) {
		printf("receive_message: buffer is too small.\n");
		*real_len = 0;
		ret = -1;
		goto _cleanup;
	}

	memset(message, 0, message_len);
	memcpy(message, received_data, received_len);
	*real_len = received_len;

_cleanup:
	close(cpu_handler_fd);
	return ret;
}

int send_message(unsigned char *message, int len)
{
	return rpmsg_send(&ept_inst, message, len);
}

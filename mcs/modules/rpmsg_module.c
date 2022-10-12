#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include "virtio_module.h"
#include "rpmsg_module.h"

#define MCS_DEVICE_NAME "/dev/mcs"

/* endpoint state */
#define ENDPOINT_BOUND           0
#define ENDPOINT_NOT_FOUND      -1
#define ENDPOINT_UNKNOWN_STATE  -2

struct endpoint_priv {
	unsigned char received_data[2048];
	unsigned int received_len;
};

/* TODO: endpoint ability negotiation */
char ept_name[RPMSG_NAME_SIZE];

/* add more parameters such as dest_addr, when there are several clientos */
static struct rpmsg_endpoint* obtain_endpoint(const char *name)
{
	struct metal_list *node;
	struct rpmsg_endpoint *ept;

	metal_list_for_each(&rdev->endpoints, node) {
		ept = metal_container_of(node, struct rpmsg_endpoint, node);
		if (!strncmp(ept->name, name, sizeof(ept->name)))
			return ept;
	}

	return NULL;
}

static int check_endpoint(const char *name)
{
	struct rpmsg_endpoint *ept = obtain_endpoint(name);
	if (!ept) {
		return ENDPOINT_NOT_FOUND;
	}

	if ((ept->addr != RPMSG_ADDR_ANY) && (ept->dest_addr != RPMSG_ADDR_ANY))
		return ENDPOINT_BOUND;

	return ENDPOINT_UNKNOWN_STATE;
}

static int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
	size_t len, uint32_t src, void *priv)
{
	/* put data into endpoint's priv buffer, do not cover.
		user call receive_message of this endpoint will read and clear the buffer */
	struct endpoint_priv *p = priv;

	memcpy(p->received_data + p->received_len, data, len);
	p->received_len += len;

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	rpmsg_destroy_ept(ept);
}

void destroy_endpoint(const char *name)
{
	struct rpmsg_endpoint *ept = obtain_endpoint(name);
	rpmsg_destroy_ept(ept);
	free(ept->priv);
	free(ept);
}

void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	strncpy(ept_name, name ? name : "", RPMSG_NAME_SIZE);

	/* endpoint_cb can be defined by user */
	struct rpmsg_endpoint *ept = malloc(sizeof(struct rpmsg_endpoint));
	ept->priv = malloc(sizeof(struct endpoint_priv));
	(void)rpmsg_create_ept(ept, rdev, ept_name, RPMSG_ADDR_ANY, dest, endpoint_cb, rpmsg_service_unbind);
}

static void *rpmsg_receive_message(void *arg)
{
	int ret;
	int dev_fd;
	struct pollfd fds;

	dev_fd = open(MCS_DEVICE_NAME, O_RDWR);
	if (dev_fd < 0) {
		printf("rpmsg_receive_message: open %s device failed.\n", MCS_DEVICE_NAME);
		return (void*)-1;
	}

	fds.fd = dev_fd;
	fds.events = POLLIN;

	while (1) {
#ifdef DEBUG
		printf("master waiting for message....\n");
#endif
		ret = poll(&fds, 1, -1);
		if (ret < 0) {
			printf("rpmsg_receive_message: poll failed.\n");
			goto _cleanup;
		}

		if (fds.revents & POLLIN) {
#ifdef DEBUG
			printf("master receiving message....\n");
#endif
			virtqueue_notification(vq[0]);  /* will call endpoint_cb or ns_bind_cb */
		}
	}

_cleanup:
	close(dev_fd);
	return (void*)0;
}

int receive_message(unsigned char *message, int message_len, int *real_len)
{
	struct rpmsg_endpoint *ept = obtain_endpoint(ept_name);
	if (!ept) {
		printf("receive_message: no endpoint to receive message.\n");
		return -1;
	}

	struct endpoint_priv *data = ept->priv;
	if (data->received_len > message_len) {
		printf("receive_message: buffer is too small. received_len:%d\n", data->received_len);
		return -1;
	}
	if (!message)
		return -1;

	/* copy buffer to message */
	memset(message, 0, message_len);
	memcpy(message, data->received_data, data->received_len);
	*real_len = data->received_len;

	/* clear buffer */
	memset(data->received_data, 0, sizeof(data->received_data));
	data->received_len = 0;

	return 0;
}

int send_message(unsigned char *message, int len)
{
	/* ensure endpoint is bound, sleep to wait */
	while (check_endpoint(ept_name) != ENDPOINT_BOUND)
		usleep(1000);

	struct rpmsg_endpoint *ept = obtain_endpoint(ept_name);
	return rpmsg_send(ept, message, len);
}

void rpmsg_module_init(void)
{
	pthread_t tid;

	/* deal with messages received from clientos */
	if (pthread_create(&tid, NULL, rpmsg_receive_message, NULL) < 0) {
		perror("rpmsg_receive_message pthread_create");
		return;
	}
	pthread_detach(tid);
}

/* Linux Endpoint run in parent process, isolate with child process */

#include <stdio.h>
#define __USE_XOPEN_EXTENDED
#define __USE_XOPEN2KXSI
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>
#include <openamp/open_amp.h>
#include <metal/device.h>
#include <metal/alloc.h>
#include <metal/io.h>
#include <termios.h>

#include "rpmsg-internal.h"

static volatile unsigned int received_data;
static struct rpmsg_virtio_shm_pool shpool;  //tmp, must be global var
static struct rpmsg_endpoint linux_ept; //tmp, must be global var

static struct remoteproc *rproc_init(struct remoteproc *rproc,
                                    struct remoteproc_ops *ops, void *arg)
{
    struct rproc_priv *priv;
    unsigned int id = *((unsigned int *)arg);

    priv = metal_allocate_memory(sizeof(*priv));
    if (!priv)
            return NULL;

    memset(priv, 0, sizeof(*priv));
    priv->rproc = rproc;
    priv->id = id;
    priv->rproc->ops = ops;
    metal_list_init(&priv->rproc->mems);
    priv->rproc->priv = priv;
    rproc->state = RPROC_READY;
    return priv->rproc;
}

static void rproc_remove(struct remoteproc *rproc)
{
    struct rproc_priv *priv;

    priv = (struct rproc_priv *)rproc->priv;
    metal_free_memory(priv);
}

static void *rproc_mmap(struct remoteproc *rproc,
            metal_phys_addr_t *pa, metal_phys_addr_t *da,
            size_t size, unsigned int attribute,
            struct metal_io_region **io)
{
    struct remoteproc_mem *mem;
    struct metal_io_region *tmpio;

    if (*pa == METAL_BAD_PHYS && *da == METAL_BAD_PHYS)
        return NULL;
    if (*pa == METAL_BAD_PHYS)
        *pa = *da;
    if (*da == METAL_BAD_PHYS)
        *da = *pa;

    mem = metal_allocate_memory(sizeof(*mem));
    if (!mem)
        return NULL;
    tmpio = metal_allocate_memory(sizeof(*tmpio));
    if (!tmpio) {
        metal_free_memory(mem);
        return NULL;
    }

    /* mmap pa to va */
    int memfd;
    void *va;
    memfd = open("/dev/mem", O_RDWR);
    va = mmap((void*)pa, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, *pa);

    remoteproc_init_mem(mem, NULL, *pa, *da, size, tmpio);
    metal_io_init(tmpio, va, &mem->pa, size, -1, attribute, NULL); /* metal io manage va and pa */
    remoteproc_add_mem(rproc, mem);
    if (io)
        *io = tmpio;

    printf("master mmap pa=0x%lx, da=0x%lx, size=0x%lx, attribute=0x%x va=%p\n", *pa, *da, size, attribute, va);

    return metal_io_phys_to_virt(tmpio, mem->pa);
}

//master tmp
static int rproc_notify_tmp(struct remoteproc *rproc, uint32_t id)
{
	return 0;
}

static int rproc_notify(struct remoteproc *rproc, uint32_t id)
{
	int ret;

	printf("master notify start\n");

	/* notify RTOS Endpoint using IPC */
	ret = write(pipefd1[1], "Linux: notify ipi\n", strlen("Linux: notify ipi\n"));
	if (ret == -1)
		perror("master write pipefd1[1]");

	return 0;
}

static struct remoteproc_ops rproc_ops = {
    .init = rproc_init,
    .remove = rproc_remove,
    .mmap = rproc_mmap,
    .notify = rproc_notify_tmp, //master tmp
};

static unsigned int receive_message(struct remoteproc *rproc)
{
	char buf[50] = {0};
	int ret;

	/* 1. poll and wait for IPI(IPC) from RTOS Endpoint */
    while (read(pipefd2[0], buf, sizeof(buf)) > 0) {
    	printf("master poll:%s", buf);
        if (strcmp(buf, "RTOS: notify ipi\n") == 0)
            break;
    }

	/* 2. receive data */
	ret = remoteproc_get_notification(rproc, VRING_RX_NOTIFY_ID);
	if (ret)
		printf("remoteproc_get_notification failed: 0x%x\n", ret);
	return received_data;
}

static int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	received_data = *((unsigned int *) data);
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	rpmsg_destroy_ept(ept);
}

static void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	(void)rpmsg_create_ept(&linux_ept, rdev, name,
			RPMSG_ADDR_ANY, dest,
			endpoint_cb,
			rpmsg_service_unbind);
}

static int send_message(unsigned int message, struct rpmsg_endpoint *ept)
{
	return rpmsg_send(ept, &message, sizeof(message));
}

static struct remoteproc *platform_create_proc(unsigned int id)
{
    struct remoteproc *rproc;
    struct remoteproc rproc_inst;
    void *va;
    void *rsc;
    int ret;
    metal_phys_addr_t pa = RSC_TABLE_ADDR;
    int rsc_size;

    /* Initialize remoteproc instance */
    rproc = remoteproc_init(&rproc_inst, &rproc_ops, &id);
    if (!rproc)
        return NULL;

    /* mmap resource table */
    rsc = get_resource_table(&rsc_size);
    va = remoteproc_mmap(rproc, &pa, NULL, rsc_size, 0, NULL);
    memcpy(va, rsc, rsc_size);

    /* parse resource table to remoteproc */
    ret = remoteproc_set_rsc_table(rproc, va, rsc_size);
    if (ret) {
        printf("Failed to initialize remoteproc, ret:0x%x\n", ret);
        remoteproc_remove(rproc);
        return NULL;
    }

    //tmp
    struct remote_resource_table *va_tmp = va;
    struct remote_resource_table *rsc_tmp = rsc;
    va_tmp->rpmsg_vdev.notifyid = rsc_tmp->rpmsg_vdev.notifyid;
    va_tmp->rpmsg_vdev.vring[0].notifyid = rsc_tmp->rpmsg_vdev.vring[0].notifyid;
    va_tmp->rpmsg_vdev.vring[1].notifyid = rsc_tmp->rpmsg_vdev.vring[1].notifyid;

    printf("(1)master platform_create_proc success\n");

    return rproc;
}

static struct rpmsg_device *platform_create_rpmsg_vdev(struct remoteproc *rproc)
{
    struct rpmsg_virtio_device rpmsg_vdev;
    struct virtio_device *vdev;
    void *shbuf;
    struct metal_io_region *shbuf_io;
    int ret;

    shbuf_io = remoteproc_get_io_with_pa(rproc, SHM_START_ADDR);
    if (!shbuf_io)
        return NULL;
    shbuf = metal_io_phys_to_virt(shbuf_io, SHM_START_ADDR);

    printf("(2)master creating remoteproc virtio\n");
    vdev = remoteproc_create_virtio(rproc, 0, RPMSG_MASTER, NULL);
    if (!vdev) {
        printf("failed remoteproc_create_virtio\n");
        return NULL;
    }

    printf("(3)master initializing rpmsg shared buffer pool\n");
    /* Only RPMsg virtio master needs to initialize the shared buffers pool */
    rpmsg_virtio_init_shm_pool(&shpool, shbuf, SHM_SIZE);

    printf("(4)master initializing rpmsg vdev\n");
    /* RPMsg virtio slave can set shared buffers pool argument to NULL */
    ret = rpmsg_init_vdev(&rpmsg_vdev, vdev, ns_bind_cb, shbuf_io, &shpool);
    if (ret) {
        printf("failed rpmsg_init_vdev\n");
        remoteproc_remove_virtio(rproc, vdev);
        return NULL;
    }

    //printf("master vq[0].callback:%p, vq[1].callback:%p\n", vdev->vrings_info[0].vq->callback, vdev->vrings_info[1].vq->callback);

    printf("(5)master returning rdev\n");
    return rpmsg_virtio_get_rpmsg_device(&rpmsg_vdev);
}

static void rpmsg_app_master(struct remoteproc *rproc, int fdm)
{
	int status = 0;
	unsigned int message;

    //tmp
    /* register notify function */
    rproc->ops->notify = rproc_notify;

	/* 1.2 master receive NS Announcement, and process it */
	receive_message(rproc);

	printf("1.2 master receive_message succeed\n");
	printf("1.2 master Endpoint: name:%s, addr:0x%x, dest_addr:0x%x\n", linux_ept.name, linux_ept.addr, linux_ept.dest_addr);

    char buffer[256] = {0};
    while (read(fdm, buffer, sizeof(buffer)) > 0) {
        message = atoi(buffer);
        /* 2. master send message */
        status = send_message(message, &linux_ept);
        if (status < 0)
            printf("send_message(%u) failed with status %d\n", message, status);
        printf("2. master send_message succeed\n");

        /* 5. master receive message */
        message = receive_message(rproc);
        printf("5. Master core received a message: %u\n\n", message);
        snprintf(buffer, sizeof(buffer), "%d\n", message);
        status = write(fdm, buffer, strlen(buffer));
        if (status == -1)
            perror("master write fdm");
    }
}

void open_pty(int *pfdm, int *pfds)
{
    int ret, fdm, fds;
    struct termios tty = {0};

    /* Open the master side of the PTY */
    fdm = posix_openpt(O_RDWR | O_NOCTTY);
    if (fdm < 0)
        printf("Error %d on posix_openpt()\n", errno);

    ret = grantpt(fdm);
    if (ret != 0)
        printf("Error %d on grantpt()\n", errno);

    ret = unlockpt(fdm);
    if (ret != 0)
        printf("Error %d on unlockpt()\n", errno);

    /* Open the slave side of the PTY */
    fds = open(ptsname(fdm), O_RDWR | O_NOCTTY);

    /* Set the state of fds to TERMIOS_P */
    tty.c_cflag = CS8;
    tcsetattr(fds, TCSAFLUSH, &tty);

    *pfdm = fdm;
    *pfds = fds;
}

int rpmsg_linux_endpoint()
{
    struct remoteproc *rproc = NULL;
    unsigned int id = 1;
    struct rpmsg_device *rdev = NULL;
    int fdm, fds;

    open_pty(&fdm, &fds);

    /* create remoteproc */
    rproc = platform_create_proc(id);
    if (!rproc) {
        printf("create rproc failed\n");
        return -1;
    }

    rdev = platform_create_rpmsg_vdev(rproc);
    if (!rdev)
        printf("master create rpmsg vdev failed\n");

    rpmsg_app_master(rproc, fdm);
    //remoteproc_remove(rproc);

    return 0;
}

/* RTOS Endpoint run in child process, isolate with parent process */

#include <stdio.h>
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

#include "rpmsg-internal.h"

static volatile unsigned int rtos_received_data;

static struct remoteproc *rtos_rproc_init(struct remoteproc *rproc,
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

static void rtos_rproc_remove(struct remoteproc *rproc)
{
    struct rproc_priv *priv;

    priv = (struct rproc_priv *)rproc->priv;
    metal_free_memory(priv);
}

static void *rtos_rproc_mmap(struct remoteproc *rproc,
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

    printf("slave mmap pa=0x%lx, da=0x%lx, size=0x%lx, attribute=0x%x va=%p\n", *pa, *da, size, attribute, va);

    return metal_io_phys_to_virt(tmpio, mem->pa);
}

static int rtos_rproc_notify(struct remoteproc *rproc, uint32_t id)
{
    int ret;

    printf("slave notify start\n");

    /* notify RTOS Endpoint using IPC */
    ret = write(pipefd2[1], "RTOS: notify ipi\n", strlen("RTOS: notify ipi\n"));
    if (ret == -1)
        perror("slave write pipefd2[1]");

    return 0;
}

static struct remoteproc_ops rtos_rproc_ops = {
    .init = rtos_rproc_init,
    .remove = rtos_rproc_remove,
    .mmap = rtos_rproc_mmap,
    .notify = rtos_rproc_notify,
};

static unsigned int rtos_receive_message(struct remoteproc *rproc)
{
    char buf[50] = {0};
    int ret;

    /* 1. poll and wait for IPI(IPC) from RTOS Endpoint */
    while (read(pipefd1[0], buf, sizeof(buf)) > 0) {
        printf("slave poll:%s", buf);
        if (strcmp(buf, "Linux: notify ipi\n") == 0)
            break;
    }

    /* 2. receive data */
    ret = remoteproc_get_notification(rproc, VRING_TX_NOTIFY_ID);
    if (ret)
        printf("remoteproc_get_notification failed: 0x%x\n", ret);
    return rtos_received_data;
}

static int rtos_endpoint_cb(struct rpmsg_endpoint *ept, void *data,
        size_t len, uint32_t src, void *priv)
{
    rtos_received_data = *((unsigned int *) data);
    return RPMSG_SUCCESS;
}

static void rtos_rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
    rpmsg_destroy_ept(ept);
}

static int rtos_send_message(unsigned int message, struct rpmsg_endpoint *ept)
{
    return rpmsg_send(ept, &message, sizeof(message));
}

static struct remoteproc *rtos_platform_create_proc(unsigned int id)
{
    struct remoteproc *rproc;
    struct remoteproc rproc_inst;
    void *va;
    void *rsc;
    int ret;
    metal_phys_addr_t pa = RSC_TABLE_ADDR;
    int rsc_size;

    /* Initialize remoteproc instance */
    rproc = remoteproc_init(&rproc_inst, &rtos_rproc_ops, &id);
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

    printf("(1)slave platform_create_proc success\n");

    return rproc;
}

static struct rpmsg_device *rtos_platform_create_rpmsg_vdev(struct remoteproc *rproc)
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

    printf("(2)slave creating remoteproc virtio\n");
    vdev = remoteproc_create_virtio(rproc, 0, RPMSG_REMOTE, NULL);
    if (!vdev) {
        printf("failed remoteproc_create_virtio\n");
        return NULL;
    }

    printf("(3)slave initializing rpmsg vdev\n");
    /* RPMsg virtio slave can set shared buffers pool argument to NULL */
    ret = rpmsg_init_vdev(&rpmsg_vdev, vdev, NULL, shbuf_io, NULL);
    if (ret) {
        printf("failed rpmsg_init_vdev\n");
        remoteproc_remove_virtio(rproc, vdev);
        return NULL;
    }

    //printf("slave vq[0].callback:%p, vq[1].callback:%p\n", vdev->vrings_info[0].vq->callback, vdev->vrings_info[1].vq->callback);

    printf("(4)slave returning rdev\n");
    return rpmsg_virtio_get_rpmsg_device(&rpmsg_vdev);
}

static void rtos_rpmsg_app_master(struct remoteproc *rproc, struct rpmsg_device *rdev)
{
    int status = 0;
    unsigned int message;
    struct rpmsg_endpoint ept;

    /* 1.1 slave create endpoint, and send an NS Announcement */
    status = rpmsg_create_ept(&ept, rdev, "k", RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, rtos_endpoint_cb, rtos_rpmsg_service_unbind);
    if (status < 0)
        printf("slave rpmsg_create_ept failed:%d\n", status);

    printf("1.1 slave rpmsg_create_ept succeed\n");
    printf("1.1 slave Endpoint: name:%s, addr:0x%x, dest_addr:0x%x\n", ept.name, ept.addr, ept.dest_addr);

    while (message < 99) {
        /* 3. slave receive message */
        message = rtos_receive_message(rproc);
        printf("3. Slave core received a message: %u\n", message);
        message++;
        sleep(1);

        printf("3. slave Endpoint: name:%s, addr:0x%x, dest_addr:0x%x\n", ept.name, ept.addr, ept.dest_addr);

        /* 4. slave send message */
        status = rtos_send_message(message, &ept);
        if (status < 0)
            printf("rtos_send_message(%u) failed with status %d\n", message, status);

        printf("4. slave rtos_send_message succeed\n");

        sleep(1);
    }
}

int rpmsg_rtos_endpoint()
{
    struct remoteproc *rproc = NULL;
    unsigned int id = 1;
    struct rpmsg_device *rdev = NULL;

    /* create remoteproc */
    rproc = rtos_platform_create_proc(id);
    if (!rproc) {
        printf("create rproc failed\n");
        return -1;
    }

    rdev = rtos_platform_create_rpmsg_vdev(rproc);
    if (!rdev)
        printf("slave create rpmsg vdev failed\n");

    rtos_rpmsg_app_master(rproc, rdev);
    //remoteproc_remove(rproc);

    return 0;
}

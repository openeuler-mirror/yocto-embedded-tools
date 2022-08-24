#include <stdio.h>
#include <metal/alloc.h>
#include <metal/io.h>
#include "remoteproc_module.h"

#define BOOTCMD_MAXSIZE 100

struct rproc_priv {
    struct remoteproc *rproc;  /* pass a remoteproc instance pointer */
    unsigned int idx;          /* remoteproc instance idx */
    unsigned int cpu_id;       /* related arg: cpu id */
    unsigned int boot_address; /* related arg: boot address(in hex format) */
};

struct remoteproc rproc_inst;

static struct remoteproc *rproc_init(struct remoteproc *rproc,
                                     const struct remoteproc_ops *ops, void *args)
{
    struct rproc_priv *priv;

    (void)rproc;
    priv = metal_allocate_memory(sizeof(*priv));
    if (!priv)
        return NULL;

    memcpy(priv, (struct rproc_priv *)args, sizeof(*priv));
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

static int rproc_start(struct remoteproc *rproc)
{
    int cpu_handler_fd;
    int ret;
    char on[BOOTCMD_MAXSIZE];
    struct rproc_priv *args = (struct rproc_priv *)rproc->priv;

    (void)snprintf(on, sizeof(on), "%d%s%d", args->cpu_id, "@", args->boot_address);

    cpu_handler_fd = open(DEV_CLIENT_OS_AGENT, O_RDWR);
    if (cpu_handler_fd < 0) {
        printf("failed to open %s\n", DEV_CLIENT_OS_AGENT);
        return cpu_handler_fd;
    }

    ret = write(cpu_handler_fd, on, sizeof(on));
    return 0;
}

static int rproc_stop(struct remoteproc *rproc)
{
#if 0
    /* send message to zephyr, zephyr shut itself down by PSCI */
    int ret = send_message("shutdown\r\n", 10);
    sleep(3);
#endif
    return 0;
}

const struct remoteproc_ops rproc_ops = {
    .init = rproc_init,
    .remove = rproc_remove,
    .start = rproc_start,
    .stop = rproc_stop,
};

struct remoteproc *create_remoteproc(void)
{
    struct remoteproc *rproc;
    struct rproc_priv args;

    args.rproc = &rproc_inst;
    args.idx = 1;
    args.cpu_id = strtol(cpu_id, NULL, 10);
    args.boot_address = strtol(boot_address, NULL, 16);
    rproc = remoteproc_init(&rproc_inst, &rproc_ops, &args);
    if (!rproc)
        return NULL;

    return rproc;
}

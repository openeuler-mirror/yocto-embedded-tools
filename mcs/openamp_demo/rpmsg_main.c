#include <metal/alloc.h>
#include <metal/io.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include "rpmsg-internal.h"

#define BOOTCMD_MAXSIZE 100

static struct remoteproc rproc_inst;
static struct remoteproc_ops ops;

char *cpu_id;
static char *boot_address;

struct rproc_priv {
    struct remoteproc *rproc;
    unsigned int id;
};

static void cleanup(void)
{
    printf("\nOpenAMP demo ended.\n");
    remoteproc_stop(&rproc_inst);
    remoteproc_remove(&rproc_inst);
    if (io)
        free(io);
}

static void handler(int sig)
{
    exit(0);
}

static struct remoteproc *rproc_init(struct remoteproc *rproc,
                                     const struct remoteproc_ops *ops, void *arg)
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

static int rproc_start(struct remoteproc *rproc)
{
    int cpu_handler_fd;
    int ret;
    char on[BOOTCMD_MAXSIZE];

    (void)snprintf(on, sizeof(on), "%s%s%s", cpu_id, "@", boot_address);

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

static void rproc_remove(struct remoteproc *rproc)
{
    struct rproc_priv *priv;

    priv = (struct rproc_priv *)rproc->priv;
    metal_free_memory(priv);
}

struct remoteproc_ops rproc_ops = {
    .init = rproc_init,
    .remove = rproc_remove,
    .start = rproc_start,
    .stop = rproc_stop,
};

static struct remoteproc *platform_create_proc(unsigned int id)
{
    struct remoteproc *rproc;

    ops = rproc_ops;
    rproc = remoteproc_init(&rproc_inst, &ops, &id);
    if (!rproc)
        return NULL;

    return &rproc_inst;
}

int main(int argc, char **argv)
{
    struct remoteproc *rproc = NULL;
    unsigned int id = 1;
    int ret;
    int opt;
    pthread_t tida, tidb, tidc;
    void *reta, *retb, *retc;

    /* ctrl+c signal, exit program and do cleanup */
    atexit(cleanup);
    signal(SIGINT, handler);

    /* using qemu arg: -device loader,file=zephyr.elf,cpu-num=1 */
    while ((opt = getopt(argc, argv, "c:b:")) != -1) {
        switch (opt) {
        case 'c':
            cpu_id = optarg;
            break;
        case 'b':
            boot_address = optarg;
            break;
        default:
            break;
        }
    }

    rproc = platform_create_proc(id);
    if (!rproc) {
        printf("create rproc failed\n");
        return -1;
    }

    ret = remoteproc_start(rproc);
    if (ret) {
        printf("start processor failed\n");
        return ret;
    }

    sleep(5);
    printf("start processing OpenAMP demo...\n");
    rpmsg_endpoint_init();

    /* Multi-thread processing user requests */
    printf("Multi-thread processing user requests...\n");

    /* userA: user shell, open with screen */
    if (pthread_create(&tida, NULL, shell_user, NULL) < 0) {
        perror("userA pthread_create");
        return -1;
    }
#if 0
    /* userB: dual user shell, open with screen */
    if (pthread_create(&tidb, NULL, shell_user, NULL) < 0) {
        perror("userB pthread_create");
        return -1;
    }
#endif
    /* userC: zephyr log */
    if (pthread_create(&tidc, NULL, log_user, NULL) < 0) {
        perror("userC pthread_create");
        return -1;
    }

    pthread_join(tida, &reta);
    if ((long)reta) {
        printf("userA return failed: %ld", (long)reta);
    }
#if 0
    pthread_join(tidb, &retb);
    if ((long)retb) {
        printf("userB return failed: %ld", (long)retb);
    }
#endif
    pthread_join(tidc, &retc);
    if ((long)retc) {
        printf("userC return failed: %ld", (long)retc);
    }

    return 0;
}

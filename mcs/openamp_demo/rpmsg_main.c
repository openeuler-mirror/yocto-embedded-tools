#include <metal/alloc.h>
#include <metal/io.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include "rpmsg-internal.h"

#define MAX_BIN_BUFLEN  (10 * 1024 * 1024)
#define BOOTCMD_MAXSIZE 100

static struct remoteproc rproc_inst;
static struct remoteproc_ops ops;

char *cpu_id;
static char *boot_address;
static char *target_binfile;
static char *target_binaddr;

struct rproc_priv {
    struct remoteproc *rproc;
    unsigned int id;
};

static int load_bin(void)
{
    int memfd = open("/dev/mem", O_RDWR);
    int bin_fd = open(target_binfile, O_RDONLY);
    void *access_address = NULL, *bin_buffer = NULL;
    long bin_size;
    long long int bin_addr = strtoll(target_binaddr, NULL, 0);

    if (bin_fd < 0 || memfd < 0) {
        printf("invalid bin file fd\n");
        exit(-1);
    }

    bin_buffer = (void *)malloc(MAX_BIN_BUFLEN);
    if (!bin_buffer) {
        printf("malloc bin_buffer failed\n");
        exit(-1);
    }

    bin_size = read(bin_fd, bin_buffer, MAX_BIN_BUFLEN);
    if (bin_size == 0) {
        printf("read bin file failed\n");
        exit(-1);
    }

    access_address = mmap((void *)bin_addr, MAX_BIN_BUFLEN, PROT_READ | PROT_WRITE,
                            MAP_SHARED, memfd, bin_addr);
    memcpy(access_address, bin_buffer, bin_size);
    free(bin_buffer);
    return 0;
}

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

    while ((opt = getopt(argc, argv, "c:b:t:a:")) != -1) {
        switch (opt) {
        case 'c':
            cpu_id = optarg;
            break;
        case 'b':
            boot_address = optarg;
            break;
        case 't':
            target_binfile = optarg;
            break;
        case 'a':
            target_binaddr = optarg;
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

    ret = load_bin();
    if (ret) {
        printf("failed to load client os\n");
        return ret;
    }

    ret = remoteproc_start(rproc);
    if (ret) {
        printf("start processor failed\n");
        return ret;
    }

    sleep(5);
    printf("start processing OpenAMP demo...\n");
    rpmsg_app_master();

    remoteproc_remove(rproc);
    return ret;
}

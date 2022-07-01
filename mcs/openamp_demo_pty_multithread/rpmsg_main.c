#include <metal/alloc.h>
#include <metal/io.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

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

static void cleanup(void)
{
    printf("\nOpenAMP demo ended.\n");
    remoteproc_remove(&rproc_inst);
    if (io)
        free(io);
}

static void handler(int sig)
{
    exit(0);
}

int load_bin(void)
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

struct remoteproc *platform_create_proc(unsigned int id)
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
    int opt;
    int fdm = -1, fds = -1;
    pid_t pid;
    int pipefd[2];
    pthread_t tidm, tids;
    struct thread_args args;
    void *retval;
#if 0
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
#endif

    cpu_id = "3";
    boot_address = "0x7a000ffc";
    target_binfile = "zephyr.bin";
    target_binaddr = "0x7a000000";

    /* Open the master side and the slave side of the PTY */
    open_pty(&fdm, &fds);

    /* Create unamed pipe */
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return -1;
    }

    /* Create the child process */
    if ((pid = fork()) < 0) {
        perror("fork");
        return -1;
    } else if (pid > 0) {
        close(fds); /* Close the slave side of the PTY */

        args.fd = fdm;
        args.pipefd = pipefd;
        if (pthread_create(&tidm, NULL, master, (void*)&args) < 0) { //T3
            perror("parent pthread_create");
            exit(1);
        }
        //printf("This is thread1: %ld, pid:%d\n", pthread_self(), getpid()); //T1
        pthread_join(tidm, NULL);
    } else {
        /* ctrl+c signal, exit program and do cleanup */
        atexit(cleanup);
        signal(SIGINT, handler);

        close(fdm); /* Close the master side of the PTY */

        args.fd = fds;
        args.pipefd = pipefd;
        if (pthread_create(&tids, NULL, slave, (void*)&args) < 0) { //T4
            perror("child pthread_create");
            exit(1);
        }
        //printf("This is thread2: %ld, pid:%d\n", pthread_self(), getpid()); //T2
        pthread_join(tids, &retval);
        if (*(int*)retval)
            exit(1);
    }

    return 0;
}

#include <metal/alloc.h>
#include <metal/io.h>
#include <openamp/remoteproc.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include "remoteproc_module.h"
#include "virtio_module.h"
#include "rpmsg_pty.h"

#define MAX_BIN_BUFLEN  (10 * 1024 * 1024)

char *cpu_id;
static char *boot_address;
static char *target_binfile;
static char *target_binaddr;

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

int rpmsg_app_master(void)
{
    pthread_t tida, tidb, tidc;
    void *reta, *retb, *retc;

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

int main(int argc, char **argv)
{
    int ret;
    int opt;
    struct remoteproc rproc;
    struct rproc_priv args = { &rproc, 1 };
    struct remoteproc *rproc_inst;

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

    args.cpu_id = strtol(cpu_id, NULL, 10);
    args.boot_address = strtol(boot_address, NULL, 16);
    rproc_inst = platform_create_proc(&args);
    if (!rproc_inst) {
        printf("create rproc failed\n");
        return -1;
    }

    ret = load_bin();
    if (ret) {
        printf("failed to load client os\n");
        return ret;
    }

    ret = remoteproc_start(rproc_inst);
    if (ret) {
        printf("start processor failed\n");
        return ret;
    }

    sleep(5);  /* wait for zephyr booting */
    virtio_init();

    ret = rpmsg_app_master();
    if (ret) {
        printf("openamp demo run failed\n");
        return ret;
    }

    printf("\nOpenAMP demo ended.\n");
    if (io)
        free(io);
    remoteproc_stop(rproc_inst); 
    rproc_inst->state = RPROC_OFFLINE;
    remoteproc_remove(rproc_inst);

    return 0;
}

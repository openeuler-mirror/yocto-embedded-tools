#include <stdio.h>
#include "openamp_module.h"

#define MAX_BIN_BUFLEN  (10 * 1024 * 1024)

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

int openamp_init(void)
{
    int ret;
    struct remoteproc *rproc;
    unsigned char message[100];
    int len;

    rproc = create_remoteproc();
    if (!rproc) {
        printf("create remoteproc failed\n");
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

    sleep(5);  /* wait for zephyr booting */
    virtio_init();

    (void)receive_message(message, sizeof(message), &len);  /* name service: endpoint matching */

    return 0;
}

int openamp_deinit(void)
{
    printf("\nOpenAMP demo ended.\n");
    if (io)
        free(io);
    remoteproc_stop(&rproc_inst); 
    rproc_inst.state = RPROC_OFFLINE;
    remoteproc_remove(&rproc_inst);

    return 0;
}

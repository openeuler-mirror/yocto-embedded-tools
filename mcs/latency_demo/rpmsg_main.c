#include <stdio.h>
#include <stdarg.h>
#include "openamp_module.h"

char *cpu_id;
char *target_binfile;
char *target_binaddr;

static void cleanup(int sig)
{
    openamp_deinit();
}

int rpmsg_app_master(void)
{
    int ret;
    char buf[2048] = {0};
    int len;
    int i;

    printf("start latency measure demo...\n");

    /* linux send messsage to clientos first */
    ret = send_message("latency_demo\n", sizeof("latency_demo\n"));
    if (ret < 0) {
        printf("send_message failed with status %d\n", ret);
        return ret;
    }

    while (1) {
        ret = receive_message(buf, sizeof(buf), &len);
        if (ret < 0) {
            printf("receive_message failed with status %d\n", ret);
            return ret;
        }

        /* show message */
        for (i = 0; i < len; i++)
            printf("%c", buf[i]);
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int opt;

    /* ctrl+c signal, do cleanup before program exit */
    signal(SIGINT, cleanup);

    while ((opt = getopt(argc, argv, "c:t:a:")) != -1) {
        switch (opt) {
        case 'c':
            cpu_id = optarg;
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

    ret = openamp_init();
    if (ret) {
        printf("openamp init failed: %d\n", ret);
        openamp_deinit();
        return ret;
    }

    ret = rpmsg_app_master();
    if (ret) {
        printf("rpmsg app master failed: %d\n", ret);
        openamp_deinit();
        return ret;
    }

    openamp_deinit();

    return 0;
}

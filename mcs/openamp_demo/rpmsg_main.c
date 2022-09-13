#include <stdio.h>
#include <stdarg.h>
#include "openamp_module.h"

char *cpu_id;
char *target_binfile;
char *target_binaddr;

int rpmsg_app_master(void)
{
    int ret;
    int message = 0;
    int len;

    printf("start processing OpenAMP demo...\n");

    while (message < 99) {
        ret = send_message((unsigned char*)&message, sizeof(message));
        if (ret < 0) {
            printf("send_message(%u) failed with status %d\n", message, ret);
            return ret;
        }
        sleep(1);

        ret = receive_message((unsigned char*)&message, sizeof(message), &len);
        if (ret < 0) {
            printf("receive_message failed with status %d\n", ret);
            return ret;
        }
        printf("Master core received a message: %u\n", message);
        message++;
        sleep(1);
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int opt;

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

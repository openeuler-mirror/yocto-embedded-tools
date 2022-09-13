#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include "rpmsg_pty.h"
#include "openamp_module.h"

char *cpu_id;
char *target_binfile;
char *target_binaddr;

int rpmsg_app_master(void)
{
    pthread_t tida, tidb, tidc;

    printf("Multi-thread processing user requests...\n");

    pthread_mutex_init(&mutex, NULL);

    /* userA, zephyr shell, open with screen */
    if (pthread_create(&tida, NULL, shell_user, NULL) < 0) {
        perror("userA pthread_create");
        return -1;
    }
    pthread_detach(tida);

    /* userB, zephyr shell, open with screen */
    if (pthread_create(&tidb, NULL, shell_user, NULL) < 0) {
        perror("userB pthread_create");
        return -1;
    }
    pthread_detach(tidb);

    while (1);

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

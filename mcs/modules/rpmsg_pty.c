#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "virtio_module.h"

/* define the keys according to your terminfo */
#define KEY_CTRL_C      3

void open_pty(int *pfdm, int *pfds)
{
    int ret;
    int fdm, fds;

    /* Open the master side of the PTY */
    fdm = posix_openpt(O_RDWR | O_NOCTTY);
    if (fdm < 0)
        printf("Error %d on posix_openpt()\n", errno);

    ret = grantpt(fdm);
    if (ret != 0)
        printf("Error %d on grantpt()\n", errno);

    ret = unlockpt(fdm);
    if (ret != 0)
        printf("Error %d on unlockpt()\n", errno);

    /* Open the slave side of the PTY */
    fds = open(ptsname(fdm), O_RDWR | O_NOCTTY);
    printf("open a new terminal, exec zephyr shell: screen %s\n", ptsname(fdm));

    *pfdm = fdm;
    *pfds = fds;
}

void *shell_user(void *arg)
{
    int ret;
    int fdm, fds;
    unsigned char cmd[1];
    unsigned char reply[2048];
    int reply_len;

    /* open PTY, pts binds to terminal, using screen to open pts */
    open_pty(&fdm, &fds);

    while (1) {
        memset(cmd, 0, 1);
        ret = read(fdm, cmd, 1);   /* get command from ptmx */
        if (ret < 0) {
            printf("shell_user: get from ptmx failed: %d\n", ret);
            return (void*)-1;
        }

        if (cmd[0] == KEY_CTRL_C)  /* special key: ctrl+c */
            //pthread_exit(NULL);
            return (void*)0;

        ret = send_message(cmd, 1);  /* send command to rtos */
        if (ret < 0) {
            printf("shell_user: send_message(%s) failed: %d\n", cmd, ret);
            return (void*)-1;
        }

        ret = receive_message(reply, sizeof(reply), &reply_len);   /* receive reply from rtos */
        if (ret < 0) {
            printf("shell_user: receive_message failed: %d\n", ret);
            return (void*)-1;
        }

        ret = write(fdm, reply, reply_len);  /* send reply to ptmx */
        if (ret < 0) {
            printf("shell_user: write to ptmx(%s) failed: %d\n", reply, ret);
            return (void*)-1;
        }
    }

    return (void*)0;
}

void *log_user(void *arg)
{
    int ret;
    int log_fd;
    unsigned char log[2048] = {0};
    int log_len;
    const char *log_file = "/tmp/zephyr_log.txt";

    ret = receive_message(log, sizeof(log), &log_len);   /* receive log from rtos */
    if (ret < 0) {
        printf("log_user: receive_message(%s) failed: %d\n", log, ret);
        return (void*)-1;
    }

    /* write log into file */
    log_fd = open(log_file, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        printf("log_user: open (%s) failed: %d\n", log_file, log_fd);
        return (void*)-1;
    }
    ret = write(log_fd, log, log_len);
    if (ret < 0) {
        printf("log_user: write to file(%s) failed: %d\n", log_file, ret);
        return (void*)-1;
    }
    close(log_fd);

    return (void*)0;
}

#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "openamp_module.h"

/* define the keys according to your terminfo */
#define KEY_CTRL_D      4
#define FILE_MODE       0644

pthread_mutex_t mutex;

void open_pty(int *pfdm, int *pfds, const char *pty_name)
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
    printf("open a new terminal, zephyr %s: screen %s\n", pty_name, ptsname(fdm));

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

    open_pty(&fdm, &fds, "shell");

    while (1) {
        ret = read(fdm, cmd, 1);   /* get command from ptmx */
        if (ret < 0) {
            printf("shell_user: get from ptmx failed: %d\n", ret);
            return (void*)-1;
        }

        if (cmd[0] == KEY_CTRL_D) {  /* special key: ctrl+d */
            close(fds);
            close(fdm);
            return (void*)0;  /* exit this thread, the same as pthread_exit */
        }

        pthread_mutex_lock(&mutex);

        ret = send_message(cmd, 1);  /* send command to rtos */
        ret |= receive_message(reply, sizeof(reply), &reply_len);   /* receive reply from rtos */

        pthread_mutex_unlock(&mutex);

        if (ret < 0) {
            printf("shell_user: send(%s)/receive(%s) message failed: %d\n", cmd, reply, ret);
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

void *console_user(void *arg)
{
    int ret;
    int fdm, fds;
    unsigned char reply[2048];
    int reply_len;

    /* open PTY, pts binds to terminal, using screen to open pts */
    open_pty(&fdm, &fds, "console");

    while (1) {
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
    log_fd = open(log_file, O_RDWR | O_CREAT | O_APPEND, FILE_MODE);
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

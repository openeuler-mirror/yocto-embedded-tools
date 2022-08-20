#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>

#include "rpmsg-internal.h"

/* define the keys according to your terminfo */
#define KEY_CTRL_C      3
#define KEY_BACKSPACE   127
#define KEY_ENTER       '\r'

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

int simple_shell(int fdm, unsigned char *cmd, int cmd_len, int *real_len)
{
    int i;
    int ret;
    char display[64] = "\ruart:~$ ";
    const int title_len = strlen(display);
    int dislen = strlen(display);
    int dislen_echo;

    memset(cmd, 0, cmd_len);
    for (i = 0; i < cmd_len; i++) {
        ret = read(fdm, &cmd[i], 1);

        display[dislen] = cmd[i];
        dislen++;
        dislen_echo = dislen;

        if (cmd[i] == KEY_CTRL_C) {    /* special key: ctrl+c */
            pthread_exit(NULL);
        }

        if ((cmd[i] == KEY_BACKSPACE) && (dislen > title_len)) {  /* special key: backspace */
            dislen -= 2;
            display[dislen] = ' ';
            dislen_echo = dislen + 1;
        }

        if (cmd[i] == KEY_ENTER) {    /* special key: enter */
            cmd[i] = '\n';
            break;
        }

        ret = write(fdm, display, dislen_echo);   /* the command echo */
    }

    ret = write(fdm, "\ruart:~$ ", title_len);  /* override the command return from rtos */
    *real_len = i + 1;
    return ret;
}

void *shell_user(void *arg)
{
    int ret;
    int fdm, fds;
    unsigned char cmd[64] = {0};
    int cmd_len;
    unsigned char reply[2048] = {0};
    int reply_len;

    /* open PTY, pts binds to terminal, using screen to open pts */
    open_pty(&fdm, &fds);

    while (1) {
        ret = simple_shell(fdm, cmd, sizeof(cmd), &cmd_len);  /* get command from ptmx */
        if (ret < 0) {
            printf("shell_user: simple_shell(%s) failed: %d\n", cmd, ret);
            return (void*)-1;
        }

        ret = send_message(cmd, cmd_len);  /* send command to rtos */
        if (ret < 0) {
            printf("shell_user: send_message(%s) failed: %d\n", cmd, ret);
            return (void*)-1;
        }

        ret = receive_message(reply, sizeof(reply), &reply_len);   /* receive reply from rtos */
        if (ret < 0) {
            printf("shell_user: receive_message(%s) failed: %d\n", reply, ret);
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
    int cmd_fd, log_fd;
    unsigned char cmd[64] = {0};
    int cmd_len;
    unsigned char log[2048] = {0};
    int log_len;
    const char *cmd_file = "/tmp/zephyr_cmd.txt";
    const char *log_file = "/tmp/zephyr_log.txt";

    /* read command from file */
    cmd_fd = open(cmd_file, O_RDONLY);
    if (cmd_fd < 0) {
        printf("log_user: open (%s) failed: %d\n", cmd_file, cmd_fd);
        return (void*)-1;
    }
    cmd_len = read(cmd_fd, cmd, sizeof(cmd));
    cmd[cmd_len] = '\n';
    close(cmd_fd);

    ret = send_message(cmd, cmd_len + 1);  /* send command to rtos */
    if (ret < 0) {
        printf("shell_user: send_message(%s) failed: %d\n", cmd, ret);
        return (void*)-1;
    }

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

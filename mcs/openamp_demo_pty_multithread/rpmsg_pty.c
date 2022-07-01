#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>

#include <openamp/open_amp.h>

#include "rpmsg-internal.h"

void open_pty(int *pfdm, int *pfds)
{
    int rc, fdm, fds;

    /* Open the master side of the PTY */
    fdm = posix_openpt(O_RDWR | O_NOCTTY);
    if (fdm < 0)
        printf("Error %d on posix_openpt()\n", errno);

    rc = grantpt(fdm);
    if (rc != 0)
        printf("Error %d on grantpt()\n", errno);

    rc = unlockpt(fdm);
    if (rc != 0)
        printf("Error %d on unlockpt()\n", errno);

    /* Open the slave side of the PTY */
    fds = open(ptsname(fdm), O_RDWR | O_NOCTTY);

    *pfdm = fdm;
    *pfds = fds;
}

static void ptmx_read_write(const char *name, int in, int out)
{
    char input[200];
    int rc, ret;

    rc = read(in, input, sizeof(input));
    if (rc == -1)
        printf("Error %d on ptmx_read %s\n", errno, name);

    ret = write(out, input, rc);
    if (ret == -1)
        printf("Error %d on ptmx_write %s\n", errno, name);
}

void *master(void *arg_list)
{
    fd_set fd_in;
    char buf[50] = {0};
    int nullfd;
    struct thread_args *args = (struct thread_args*)arg_list;

    /* Wait for child */
    close(args->pipefd[1]);
    while (read(args->pipefd[0], buf, sizeof(buf)) > 0) {
        if (strcmp(buf, "init done\n") == 0)
            break;
    }

    //printf("This is thread3: %ld, pid:%d\n", pthread_self(), getpid()); //T3
    //printf("thread3 fd:%d, pipefd[0]:%d, pipefd[1]:%d\n", args->fd, args->pipefd[0], args->pipefd[1]);

    printf("----------PTY Terminal----------\n");
    printf("Input number: ");
    fflush(stdout);

    for (;;) {
        /* Wait for data from standard input and master side of PTY */
        FD_ZERO(&fd_in);
        FD_SET(0, &fd_in);
        FD_SET(args->fd, &fd_in);

        if (select(args->fd + 1, &fd_in, NULL, NULL, NULL) == -1)
            printf("Error %d on select()\n", errno);

        /* If data on standard input */
        if (FD_ISSET(0, &fd_in)) {
            ptmx_read_write("standard input", 0, args->fd);

            /* flush data to /dev/null on ptmx, avoid printing repeatedly */
            nullfd = open("/dev/null", O_RDWR);
            ptmx_read_write("standard input", args->fd, nullfd);
        }

        /* If data on master side of PTY */
        if (FD_ISSET(args->fd, &fd_in)) {
            ptmx_read_write("master pty", args->fd, 1);
            printf("Input number: ");
            fflush(stdout);
        }
    }
}

void *slave(void *arg_list)
{
    struct remoteproc *rproc = NULL;
    unsigned int id = 1;
    int ret;
    struct thread_args *args = (struct thread_args*)arg_list;

    //printf("This is thread4: %ld, pid:%d\n", pthread_self(), getpid()); //T4
    //printf("thread4 fd:%d, pipefd[0]:%d, pipefd[1]:%d\n", args->fd, args->pipefd[0], args->pipefd[1]);

    rproc = platform_create_proc(id);
    if (!rproc) {
        printf("create rproc failed\n");
        return (void*)1;
    }

    ret = load_bin();
    if (ret) {
        printf("failed to load client os\n");
        return (void*)1;
    }

    ret = remoteproc_start(rproc);
    if (ret) {
        printf("start processor failed\n");
        return (void*)1;
    }

    sleep(5); /* wait for clientos booting */
    rpmsg_endpoint_init();

    /* Since we are using name service, we need to wait for a response
     * from NS setup and then we need to process it
     */
    rpmsg_endpoint_app(args->fd, 1);

    /* now wake up parent using unamed pipe, init console */
    close(args->pipefd[0]);
    ret = write(args->pipefd[1], "init done\n", strlen("init done\n"));
    if (ret == -1) {
        printf("failed to wake up parent\n");
        return (void*)1;
    }

    for (;;) {
        ret = rpmsg_endpoint_app(args->fd, 0);
        if (ret) {
            printf("rpmsg_endpoint_app failed\n");
            return (void*)1;
        }
    }

    return (void*)0;
}

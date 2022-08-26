#ifndef RPMSG_PTY_H
#define RPMSG_PTY_H

void *shell_user(void *arg);
void *console_user(void *arg);
void *log_user(void *arg);

extern pthread_mutex_t mutex;

#endif
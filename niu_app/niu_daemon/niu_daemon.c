#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ql_oe.h>
#include <qlsyslog/ql_sys_log.h>

#define APP_PATH                "/usrdata/xn_app"
#define DAEMON_STARTUP_FILE     APP_PATH"/niu_daemon_startup.conf"
#define SLEEP_LOOP (60)
#define SYSCMD_MAX  (15)

#define LOG_V(fmt, ...) QLOGV(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_D(fmt, ...) QLOGD(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_I(fmt, ...) QLOGI(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_W(fmt, ...) QLOGW(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_E(fmt, ...) QLOGE(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_F(fmt, ...) QLOGF(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);

static void *child_thread(void *arg) 
{
    int ret, i, j, k, status, fd;
    char *cmd, *p, *q, *x, *buf;
    char **cmd_arg;
    pid_t pid, pid_ret;
    size_t bufsize;

    pthread_detach(pthread_self());
    cmd = (char *)arg;
    for(i = 0, j = 0; ; i++) {
        if (cmd[i] == ' ') {
            j++;
        } else if (cmd[i] == 0 || cmd[i] == '\n' || cmd[i] == '\r') {
            break;
        }
    }
    k = j + 2;
    cmd_arg = malloc(sizeof(char *) * k);
    if (cmd_arg == NULL) {
        LOG_E("malloc() error.");
        /* sleep SLEEP_LOOP seconds to avoid busy loop */
        sleep(SLEEP_LOOP);
        exit(1);
    }
    for(i = 0, j = 0, p = cmd; ; i++) {
        if (cmd[i] == ' ') {
            cmd[i] = 0;
            cmd_arg[j] = p;
            p = cmd + i + 1;
            j++;
        } else if (cmd[i] == 0 || cmd[i] == '\n' || cmd[i] == '\r') {
            cmd[i] = 0;
            cmd_arg[j] = p;
            j++;
            break;
        }
    }
    if (j == 0) {
        LOG_E("command is wrong.");
        goto out;
    }
    cmd_arg[j] = NULL;

    x = cmd_arg[0];
    
    k = strlen(x);
    for(i = k; i >= 0; i--) {
        if (x[i] == '/') {
            q = x + i + 1;
            break;
        }
    }
    if (i == 0) {
        q = x;
    }
    k -= i;
    k += 32;
    p = malloc(k);
    if (p == NULL) {
        LOG_E("malloc() error.");
        /* sleep SLEEP_LOOP seconds to avoid busy loop */
        sleep(SLEEP_LOOP);
        exit(1);
    }
    /* kill old legacy programs */
    snprintf(p, k, "killall -9 '%s' >/dev/null 2>&1", q);
    ret = system(p);
    free(p);

loop:
    pid = fork();
    if (pid > 0) {
        /* parent */
        pid_ret = waitpid(pid, &status, 0);
        LOG_E("child [%s] pid[%u] return[%u], status[%d],[%d].",
            x, pid, pid_ret, status, WEXITSTATUS(status));
        ql_app_ready(37, PINLEVEL_HIGH);
        LOG_D("call ql_app_exit.");
        /* at least sleep 1 second to avoid busy loop */
        sleep(1);
        goto loop;
    } else if (pid == 0) {
        /* child */
#if 1
        /* redirect the child process stdout, stderr. */
        fd = open("/dev/null", O_WRONLY);
        if (fd != -1) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
#endif
        /* close all parent fd */
        for (i = 3; i < 256; i++) {
            close(i);
        }
        
        ret = execv(cmd_arg[0], cmd_arg);
        if (ret == -1) {
            LOG_E("exec() error. commnd[%s].", cmd_arg[0]);
            /* sleep SLEEP_LOOP seconds to avoid busy loop */
            sleep(SLEEP_LOOP);
            exit(1);
        }
        
    } else {
        /* failed */
        LOG_E("fork() error.");
        /* sleep SLEEP_LOOP seconds to avoid busy loop */
        sleep(SLEEP_LOOP);
        exit(1);
    }

out:
    free(cmd_arg);
    free(arg);
    return NULL;
}

int main(int argc, char **argv)
{
    int ret, count = 0;
    char buf[256];
    FILE *fp = NULL;
    char *p = NULL;
    pthread_t pt;

    for(;;)
    {
        fp = fopen(DAEMON_STARTUP_FILE, "rb");
        if(fp == NULL)
        {
            sleep(1);
            continue;
        }
        else
        {
            break;
        }
    }

    while (fgets(buf, sizeof(buf), fp))
    {
        if (buf[0] == '#')
        {
            continue;
        }
        if (strlen(buf) <= 4)
        {
            continue;
        }
        p = strdup(buf);
        if (p == NULL)
        {
            LOG_E("malloc() error.");
            /* sleep SLEEP_LOOP seconds to avoid busy loop */
            sleep(SLEEP_LOOP);
            exit(1);
        }
        LOG_D("run %s", p);
        ret = pthread_create(&pt, NULL, child_thread, (void *)p);
        if (ret)
        {
            LOG_E("pthread_create() error.");
            /* sleep SLEEP_LOOP seconds to avoid busy loop */
            sleep(SLEEP_LOOP);
            exit(1);
        }
        count++;
        if (count >= SYSCMD_MAX)
        {
            LOG_E("run max commands, skip others.");
            break;
        }
        usleep(1000);
    }

    LOG_D("run [%d] sytstem command(s).", count);
    while(1) {
        /* main thread is a loop wait. */
        sleep(86400);
    }
    return 0;
}
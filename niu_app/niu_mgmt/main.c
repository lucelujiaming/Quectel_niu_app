#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ql_oe.h>
#include <qlsyslog/ql_sys_log.h>
#include "niu_version.h"
#include "utils.h"
#include "niu_smt.h"
int g_start_time = 0;
extern int main_thread(void);

#define LOCK_FILE       "/var/run/niu_mgmtd.pid"
#define LOCK_MODE       (S_IRUSR|S_IWUSR)

static void ql_handle_signal (int sig_num, siginfo_t *info, void *ptr)
{
    ql_sys_log_signal(QL_SYS_LOG_ID_MAIN, QL_SYS_LOG_FATAL, QL_LOG_TAG, sig_num, info, ptr);
    LOG_D("call ql_app_exit.");
    //ql_app_exit();
    ql_app_ready(37, PINLEVEL_HIGH);
    exit(-1);
}

/**
 * @brief : Check wheather the process is running
 * @param : 
 *          lock_file: lock file which when process running to write pid
 *          lock_mode: open file mode
 * @return :
 *          if process not running return 0, else return 1
 * 
 */
static int is_already_running(const char *lock_file, mode_t lock_mode)
{
    int ret, fd;
    char buf[32];
    struct flock fl;

    fd = open(lock_file, O_RDWR|O_CREAT, lock_mode);
    if (fd < 0) {
        LOG_E("open lock file[%s] error.\n", lock_file);
        exit(1);
    }
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    ret = fcntl(fd, F_SETLK, &fl);
    if (ret) {
        /* already running or some error. */
        close(fd);
        return 1;
    }
    /* O.K. write the pid */
    ret = ftruncate(fd,0);
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)getpid());
    ret = write(fd, buf, strlen(buf) + 1);

    return 0;
}

int get_process_time()
{
    struct timespec res;
    clock_gettime(CLOCK_BOOTTIME, &res);
    return res.tv_sec - g_start_time;
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    struct timespec res;
    clock_gettime(CLOCK_BOOTTIME, &res);
    g_start_time = res.tv_sec;
    sa.sa_sigaction = ql_handle_signal;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGTERM, &sa, NULL);
    sigaction (SIGSEGV, &sa, NULL);
    sigaction (SIGABRT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGBUS, &sa, NULL);
    sigaction (SIGKILL, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    //add by pony.ma for write version file at 20190605
    FILE *version = NULL;
    version = fopen("/usrdata/xn_app/version.txt","w+");
    if (version == NULL)
    {
        LOG_E("open version file error!");
    }
    
    fwrite(NIU_VERSION,8,1,version);
    fclose(version);
    //end added pony.ma
    

    if(argc > 1)
    {

        if(memcmp(argv[1], "-v", 2) == 0)
        {
            LOG_D("version: %s, build at %s.", NIU_VERSION, NIU_BUILD_TIMESTAMP);
            return 0;
        }
    }
    if(is_already_running(LOCK_FILE, LOCK_MODE))
    {
        LOG_E("`%s` is already running.", NIU_APP);
        return -1;
    }

    /* juson.zhang-2019/03/30:add for niu smt test */
    if (0 == niu_smt_check_file_exist())
    {
        niu_smt_test_after_reset();
        return 0;
    }

    return main_thread();
}

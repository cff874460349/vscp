#include "vscp_client.h"
#include "vscp_debug.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include<sys/time.h>

#include <rg_thread.h>

#define VSCP_DEBUG_FILE         "/tmp/vscp_dbg"
#define VSCP_DEBUG_CMD_LEN      10

#define VSCP_DUMP_BUFFER        768
#define VSCP_TIME_BUF           27
#define VSCP_DUMP_FILE          "output.txt"
#define VSCP_DUMP_FILE_SIZE     (100 * 1024)

char *test_name;
char *test_id;

int vscp_debug_info = 0;

int vscp_debug_timer(struct rg_thread * thread)
{
    int rv, fd;
    char buf[VSCP_DEBUG_CMD_LEN];
    vscp_client_t *data = (vscp_client_t *)thread->arg;

    data->debug_thread = NULL;
    fd = open(VSCP_DEBUG_FILE, O_RDONLY);
    if (fd < 0) {
        goto out;
    }
    rv = read(fd, buf, VSCP_DEBUG_CMD_LEN);
    if (rv < 0) {
        close(fd);
        goto out;
    }

    vscp_debug_info = strtol(buf, NULL, 16);
    close(fd);
out:
    RG_THREAD_TIMER_ON(data->vscp_master, data->debug_thread, vscp_debug_timer, (void *)data, 2);
    if (data->debug_thread == NULL) {
        VSCP_DBG_ERR("create dbg_timer error\n");
        rv = -1;
    } else {
        rv = 0;
    }

    return rv;
}

static inline void vscp_debug_get_current_time(char *buf)
{
    time_t now;
    struct tm tm;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    now = tv.tv_sec;
    localtime_r(&now, &tm);
    (void)strftime(buf, VSCP_TIME_BUF, "%Y/%m/%d %H:%M:%S", &tm);
}

static inline char *vscp_make_path(char *path_suffix)
{
    char *path_prefix;
    static char path[100]={0};
    int ret;

    //path_prefix = getenv("TMPDIR");
    path_prefix = getenv("HOME");
    if (path_prefix == NULL) {
        /* 若环境变量"TMPDIR"获取失败，则直接赋为"/tmp" */
        path_prefix = "/tmp";
    }

    strcpy(path, path_prefix);
    strcat(path, "/.gosuv/log/");
	strcat(path, test_name);
	strcat(path, "/");
    ret = mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);
    if (ret == -1) {
        if (errno != EEXIST) {
            return NULL;
        }
    }
    //strcat(path, path_suffix);
    //sprintf(path+strlen(path), "%d", getpid());
    strcat(path, test_id);

    return path;
}

void vscp_debug_dump_file(const char *fun_name, int line, const char *fmt, ...)
{

    int fd;
    va_list args;
    char tmp[VSCP_DUMP_BUFFER];
    int len;
    char buf[VSCP_TIME_BUF];
    struct stat info;

    va_start(args, fmt);
    memset(tmp, 0, sizeof(tmp));
    memset(buf, 0, sizeof(buf));
    vscp_debug_get_current_time(buf);
    len = snprintf(tmp, sizeof(tmp), "%s [%s/%d] - ", buf, fun_name, line);
    vsnprintf(tmp + len, sizeof(tmp) - len, fmt, args);
    va_end(args);

    /* 判断文件大小，如果文件大小超过100K，删除文件 */
    (void)stat(vscp_make_path(VSCP_DUMP_FILE), &info);
    if (info.st_size > VSCP_DUMP_FILE_SIZE) {
        unlink(vscp_make_path(VSCP_DUMP_FILE));
    }

    if ((fd = open(vscp_make_path(VSCP_DUMP_FILE), (O_RDWR | O_APPEND | O_CREAT),
        S_IRWXU | S_IRWXG | S_IRWXO)) >= 0) {
        write(fd, tmp, strlen(tmp));
        close(fd);
    }
}






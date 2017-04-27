#include <sys/time.h>
#include <time.h>    /* clock_gettime */
#include <stddef.h>  /* NULL宏 */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if 0
#include <sys/sysinfo.h>
#endif

#include <rg_thread.h>
#include "lib.h"


#if 0
/* Static function to get current sec and usec.  */
static int
system_uptime (struct timeval *tv, struct timezone *tz)
{
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;
  unsigned long uptime;
  static long base = 0;
  static long offset = 0;
  long leap;
  long diff;
#if 1
  struct sysinfo info;

  /* Get sysinfo.  */
  if (sysinfo (&info) < 0)
    return -1;

  /* Check for wraparound. */
  if (prev > info.uptime)
    wraparound_count++;

  /* System uptime.  */
  uptime = wraparound_count * WRAPAROUND_VALUE + info.uptime;
  prev = info.uptime;
#else
  /* Check for wraparound. */
  uptime = clock();
  if (prev > uptime)
    wraparound_count++;

  /* System uptime.  */
  prev = uptime;
  uptime = wraparound_count * WRAPAROUND_VALUE + uptime;

#endif
  /* Get tv_sec and tv_usec.  */
  gettimeofday (tv, tz);

  /* Deffernce between gettimeofday sec and uptime.  */
  leap = tv->tv_sec - uptime;

  /* Remember base diff for adjustment.  */
  if (! base)
    base = leap;

  /* Basically we use gettimeofday's return value because it is the
     only way to get required granularity.  But when diff is very
     different we adjust the value using base value.  */
  diff = (leap - base) + offset;

  /* When system time go forward than 2 sec.  */
  if (diff > 2 || diff < -2)
    offset -= diff;

  /* Adjust second.  */
  tv->tv_sec += offset;

  return 0;
}

#else
static int
system_uptime (struct timeval *tv, struct timezone *tz)
{
  struct timespec ts;

  if (tz != NULL)
    {
      /* 取当前的绝对时间 */
      gettimeofday (tv, tz);
    }
  else if (tv != NULL)
    {
      /* 获取系统起机时间 */
      clock_gettime (CLOCK_MONOTONIC, &ts);

      tv->tv_sec = ts.tv_sec;
      tv->tv_usec = ts.tv_nsec / 1000;
    }

  return 0;
}
#endif

void
rg_time_tzcurrent (struct timeval *tv, struct timezone *tz)
{
  system_uptime (tv, tz);
  return;
}

/* 根据pid，生成一个保存于内存的文件路径 */
static inline char *rg_thread_make_path(int pid)
{
  static char has_created = 0;
  static char path[50];  /* 长度需要足够存放/tmp/rg_thread/xxxx */
  char pid_str[20]; /* PID字符串，不会超过20个字符 */
  int ret;

  if (!has_created)
    {
      /* 创建rg_thread的目录等 */
      ret = mkdir("/tmp/rg_thread", S_IRWXU);
      if (ret < 0)
        {
          if (errno != EEXIST)
            {
              RG_THREAD_PRINT("Failed to create directory\"tmp/rg_thread\", %s(%d)\n",
                      strerror(errno), errno);
              return NULL;
            }
        }
      has_created = 1;
    }

  strcpy(path, "/tmp/rg_thread/");
  sprintf(pid_str, "%d", pid);
  strcat(path, pid_str);

  return path;
}

/* 将debug信息dump到调试文件 */
void
rg_thread_debug_msg(const char *fmt, ...)
{
  int fd;
  va_list args;
  char tmp[1024]; /* 调试信息使用，暂时只支持打印1k */
  struct stat st_buf;
  char *path;

  va_start(args, fmt);
  memset(tmp, 0, sizeof(tmp));
  vsnprintf(tmp, sizeof(tmp), fmt, args);
  va_end(args);
#if 1
  path = rg_thread_make_path(getpid());
  if (path == NULL)
    return;
  fd = open(path, (O_WRONLY|O_APPEND|O_CREAT));
  if (fd < 0)
    {
      /* RG_THREAD_PRINT("Failed to open %s, %s.\n", path, strerror(errno)); */
      return;
    }

  if (stat(path, &st_buf) == 0)
    {
      /* 超过1M则截断 */
      if (st_buf.st_size > (1024*1024))
        {
          ftruncate(fd, 0);
        }
    }
  write(fd, tmp, strlen(tmp));
  close(fd);
#else
  /* 强制打印到控制台 */
  if ((fd = open("/dev/ttyS1", 1)) >= 0) {
      write(fd, tmp, strlen(tmp));
      close(fd);
  }
#endif

}


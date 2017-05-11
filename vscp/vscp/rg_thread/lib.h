#ifndef _LIB_H
#define _LIB_H

#include <sys/socket.h>
#include <sys/time.h>

typedef unsigned int u_int32_t;

#define XCALLOC(type, size)      calloc (1, size)
#define XMALLOC(type, size)      malloc (size)
#define XFREE(type,ptr)          free (ptr)

#define RG_THREAD_INVALID_INDEX (-1)

#define RG_THREAD_SELECT_FD_MAX (__FD_SETSIZE)  /* select机制所支持的最大fd值 */

/* 函数声明 */
void rg_time_tzcurrent (struct timeval *tv, struct timezone *tz);
void rg_thread_debug_msg(const char *fmt, ...);

#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffUL /* max value in unsigned long	*/
#endif
#define HZ 100
#define WRAPAROUND_VALUE (ULONG_MAX / HZ + 1) /* HZ = frequency of ticks per second. */

enum memory_type
{
  MTYPE_TMP = 0,        /* Must always be first and should be zero. */

  /* Thread */
  MTYPE_THREAD_MASTER,
  MTYPE_THREAD,

  MTYPE_POLLFD,

  MTYPE_MAX		/* Must be last & should be largest. */
};
#endif /*_LIB_H*/



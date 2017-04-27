/* Copyright (C) 2003 IP Infusion, Inc. All Rights Reserved. */

#ifndef _RG_TIMEUTIL_H
#define _RG_TIMEUTIL_H

#define TV_USEC_PER_SEC 1000000

#define PAL_TIME_MAX_TV_SEC 0x7fffffff
#define PAL_TIME_MAX_TV_USEC 0x7fffffff

#define ONE_WEEK_SECOND     (60*60*24*7)
#define ONE_DAY_SECOND      (60*60*24)
#define ONE_HOUR_SECOND     (60*60)
#define ONE_MIN_SECOND      (60)
#define ONE_SEC_DECISECOND  (10)
#define ONE_SEC_CENTISECOND (100)
#define ONE_SEC_MILLISECOND (1000)
#define ONE_SEC_MICROSECOND (1000000)

#define TV_ADJUST(A)        timeval_adjust (A)
#define TV_CEIL(A)          timeval_ceil ((A))
#define TV_FLOOR(A)         timeval_floor ((A))
#define TV_ADD(A,B)         timeval_add ((A), (B))
#define TV_SUB(A,B)         timeval_subtract ((A), (B))
#define TV_CMP(A,B)         timeval_cmp ((A), (B))
#define INT2TV(A)           timeutil_int2tv ((A))
#define MSEC_INT2TV(A)      timeutil_msec_int2tv ((A))

/* Time related utility function prototypes  */

int
timeval_cmp (struct timeval, struct timeval);

struct timeval
timeval_subtract (struct timeval a, struct timeval b);

#endif /* _RG_TIMEUTIL_H */

/* Copyright (C) 2003 IP Infusion, Inc. All Rights Reserved. */

#include "lib.h"
#include "timeutil.h"

struct timeval
timeval_adjust (struct timeval a)
{
  while (a.tv_usec >= TV_USEC_PER_SEC)
    {
      a.tv_usec -= TV_USEC_PER_SEC;
      a.tv_sec++;
    }

  while (a.tv_usec < 0)
    {
      a.tv_usec += TV_USEC_PER_SEC;
      a.tv_sec--;
    }

  if (a.tv_sec < 0)
    {
      a.tv_sec = 0;
      a.tv_usec = 10;
    }

  if (a.tv_sec > TV_USEC_PER_SEC)
    a.tv_sec = TV_USEC_PER_SEC;

  return a;
}

struct timeval
timeval_subtract (struct timeval a, struct timeval b)
{
  struct timeval ret;

  ret.tv_usec = a.tv_usec - b.tv_usec;
  ret.tv_sec = a.tv_sec - b.tv_sec;

  return timeval_adjust (ret);
}


int
timeval_cmp (struct timeval a, struct timeval b)
{
  return (a.tv_sec == b.tv_sec ?
          a.tv_usec - b.tv_usec : a.tv_sec - b.tv_sec);
}


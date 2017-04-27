/* Copyright (C) 2001-2003 IP Infusion, Inc. All Rights Reserved. */

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/poll.h>
#include <rg_thread.h>
#include "lib.h"
#include "timeutil.h"

#define LIB_PARAM_CHECK 1

#define rg_thread_gettid() syscall(__NR_gettid)
typedef char bool;
/*
   Thread.c maintains a list of all the "callbacks" waiting to run.

   It is RTOS configurable with "HAVE_RTOS_TIC", "HAVE_RTOS_TIMER",
   "RTOS_EXECUTE_ONE_THREAD".

   For Linux/Unix, all the above are undefined; the main task will
   just loop continuously on "thread_fetch" and "thread_call".
   "thread_fetch" will stay in a tight loop until all active THREADS
   are run.

   When "HAVE_RTOS_TIC" is defined, the RTOS is expected to emulate
   the "select" function as non-blocking, and call "lib_tic" for any
   I/O ready (by setting PAL_SOCK_HANDLESET_ISSET true), and must call
   "lib_tic" at least once every 1 second.

   When "HAVE_RTOS_TIMER" is defined, the RTOS must support the
   "rtos_set_timer( )", and set a timer; when it expires, it must call
   "lib_tic ( )".

   When "RTOS_EXECUTE_ONE_THREAD" is defined, the RTOS must treat
   ZebOS as strict "BACKGROUND".  The BACKGROUND MANAGER Keeps track
   of all ZebOS Routers, and calls them in strict sequence.  When
   "lib_tic" is called, ONLY ONE THREAD will be run, and a TRUE state
   will be SET to indicate that a THREAD has RUN, so the BACKGROUND
   should give control to the FOREGROUND once again.  If not SET, the
   BACKGROUND is allowed to go onto another Router to try its THREAD
   and so-forth, through all routers.
*/

/* Allocate new thread master.  */
struct rg_thread_master *
rg_thread_master_create (void)
{
  struct rg_thread_master *master;

  master = XCALLOC (MTYPE_THREAD_MASTER, sizeof (struct rg_thread_master));
  if (master != NULL)
    {
      RG_THREAD_SET_FLAG(master->flag, RG_THREAD_IS_FIRST_FETCH);
    }

  return master;
}

int
rg_thread_use_poll (struct rg_thread_master *m, short max_fds)
{
  int i;

  if (m == NULL)
    {
      return -1;
    }

  RG_THREAD_INTERFACE_DBG (m, "User set use poll, max fd num %d.\n", max_fds);

  m->pfds = XMALLOC (MTYPE_POLLFD, max_fds * sizeof (struct pollfd));
  if (m->pfds == NULL)
    {
      RG_THREAD_INTERFACE_DBG (m, "Failed to create pollfd array(%d).\n", max_fds);
      return -1;
    }

  m->nfds = 0;  /* 初始化时，认为有效的pollfd数组为0 */
  m->pfds_size = max_fds;
  RG_THREAD_SET_FLAG (m->flag, RG_THREAD_USE_POLL);
  /* 初始化pollfd数组的内容 */
  for (i = 0; i < max_fds; i++)
    {
      m->pfds[i].fd = -1;
      m->pfds[i].events = 0;
    }

  return 0;
}

/*
  fill up pollfd with file descriptable
  and event
*/
static void
rg_thread_set_pollfd (struct rg_thread *thread, uint32_t events)
{
  int i, index;

  index = RG_THREAD_INVALID_INDEX; /* 初始化为-1表示不可用 */
  /*
   * thread->pfd_index保存的是上一次分配给它的索引，
   * 一般情况下进程模块都会马上再次添加该伪线程，此时取到这个index就能再次分配到这个索引
   */
  for (i = thread->pfd_index; i < thread->master->pfds_size; i++)
    {
      if (thread->master->pfds[i].fd < 0)
        {
          /* 找到一个可用的数组元素 */
          index = i;
          break;
        }
    }

  if (index == RG_THREAD_INVALID_INDEX)
    {
      /* 从前半段继续尝试查找 */
      for (i = 0; i < thread->pfd_index; i++)
        if (thread->master->pfds[i].fd < 0)
          {
            /* 找到一个可用的数组元素 */
            index = i;
            break;
          }

      if (index == RG_THREAD_INVALID_INDEX)
        {
          /*
           * 找不到可用的数组元素，设置失败
           * 因为已经由用户设置最大fd数目了，如果不够，是使用模块创建的fd已经超出了总量
           */
          RG_THREAD_PROCEDURE_DBG (thread->master, "Failed to find array index.\n");
          assert(0);
          return;
        }
    }

  thread->pfd_index = index;  /* 将所分配到的索引保存起来 */
  thread->master->pfds[index].fd = RG_THREAD_FD (thread);
  thread->master->pfds[index].events = events;
  thread->master->pfds[index].revents = 0;
  if (index > thread->master->nfds)
    {
      /* 重新设置pollfd数组的个数 */
      thread->master->nfds = index;
      RG_THREAD_PROCEDURE_DBG (thread->master, "Max nfds %d.\n", thread->master->nfds);
    }
  return;
}

static inline void
rg_thread_clear_pollfd (struct rg_thread *thread)
{
  thread->master->pfds[thread->pfd_index].fd = -1;
  thread->master->pfds[thread->pfd_index].events = 0;

  return;
}

/* Add a new thread to the list.  */
void
rg_thread_list_add (struct rg_thread_list *list, struct rg_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
  assert(thread != NULL);
#endif

  thread->next = NULL;
  thread->prev = list->tail;
  if (list->tail)
    list->tail->next = thread;
  else
    list->head = thread;
  list->tail = thread;
  list->count++;
}

/* Add a new thread just after the point. If point is NULL, add to top. */
static void
rg_thread_list_add_after (struct rg_thread_list *list,
                       struct rg_thread *point,
                       struct rg_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
  assert(thread != NULL);
#endif

  thread->prev = point;
  if (point)
    {
      if (point->next)
        point->next->prev = thread;
      else
        list->tail = thread;
      thread->next = point->next;
      point->next = thread;
    }
  else
    {
      if (list->head)
        list->head->prev = thread;
      else
        list->tail = thread;
      thread->next = list->head;
      list->head = thread;
    }
  list->count++;
}

/* Delete a thread from the list. */
static struct rg_thread *
rg_thread_list_delete (struct rg_thread_list *list, struct rg_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
  assert(thread != NULL);
  if (list != &thread->master->unuse)
    assert(thread->type != RG_THREAD_UNUSED);
#endif

  if (thread->next)
    thread->next->prev = thread->prev;
  else
    list->tail = thread->prev;
  if (thread->prev)
    thread->prev->next = thread->next;
  else
    list->head = thread->next;
  thread->next = thread->prev = NULL;
  list->count--;
  return thread;
}

/* Delete top of the list and return it. */
static struct rg_thread *
rg_thread_trim_head (struct rg_thread_list *list)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  if (list->head)
    return rg_thread_list_delete (list, list->head);
  return NULL;
}

/* Free half unused thread. */
static void
rg_thread_unuse_list_free_half (struct rg_thread_master *m)
{
  int i;
  struct rg_thread *t;
  struct rg_thread *next;
  struct rg_thread_list *list;

  assert (m != NULL);
  list = &(m->unuse);
  assert (list != NULL);

  for (t = list->head, i = RG_THREAD_FREE_UNUSE_MAX; t && i; t = next, i--)
    {
      next = t->next;
      rg_thread_list_delete(list, t);
      XFREE (MTYPE_THREAD, t);
    }

  assert (t != NULL && i == 0);

  m->alloc -= RG_THREAD_FREE_UNUSE_MAX;

  return;
}

/* Move thread to unuse list. */
static void
rg_thread_add_unuse (struct rg_thread_master *m, struct rg_thread *thread)
{
  assert (m != NULL);
  assert (thread->next == NULL);
  assert (thread->prev == NULL);
  assert (thread->type == RG_THREAD_UNUSED);
  rg_thread_list_add (&m->unuse, thread);

  if (m->unuse.count >= RG_THREAD_UNUSE_MAX)
    rg_thread_unuse_list_free_half (m);

  return;
}

/* Unuse thread */
static void
rg_thread_unuse (struct rg_thread_master *m, struct rg_thread *thread)
{
  thread->type = RG_THREAD_UNUSED;
  RG_THREAD_UNSET_FLAG(thread->flag, RG_THREAD_FLAG_IN_LIST);
  if (!RG_THREAD_CHECK_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR) && thread != m->t_recover)
    rg_thread_add_unuse (m, thread);
}

/* Free all unused thread. */
static void
rg_thread_list_free (struct rg_thread_master *m, struct rg_thread_list *list)
{
  struct rg_thread *t;
  struct rg_thread *next;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
#endif

  for (t = list->head; t; t = next)
    {
      next = t->next;
      /*
       * 如果是用户自己创建的伪线程，那么不用删除，还放在链表中。
       * 不用担心链表还保留着这些伪线程，因为master很快就被删除掉，也就没有链表了。
       * 需要把该伪线程设置为不在链表中，用户无需再cancel，只需删除它即可。
       */
      if (RG_THREAD_CHECK_FLAG(t->flag, RG_THREAD_FLAG_CRT_BY_USR))
        {
          RG_THREAD_UNSET_FLAG(t->flag, RG_THREAD_FLAG_IN_LIST);
          continue;
        }
      XFREE (MTYPE_THREAD, t);
      list->count--;
      m->alloc--;
    }
}

void
rg_thread_list_execute (struct rg_thread_master *m, struct rg_thread_list *list)
{
  struct rg_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
#endif

  thread = rg_thread_trim_head (list);
  if (thread != NULL)
    {
      rg_thread_execute (thread->func, thread->arg, thread->u.val);
      rg_thread_unuse (m, thread);
    }
}

void
rg_thread_list_clear (struct rg_thread_master *m, struct rg_thread_list *list)
{
  struct rg_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
#endif

  while ((thread = rg_thread_trim_head (list)))
    {
      rg_thread_unuse (m, thread);
    }
}

/* Stop thread scheduler. */
void
rg_thread_master_finish (struct rg_thread_master *m)
{
  int i;

  if (m == NULL)
    {
     return;
    }

  rg_thread_list_free (m, &m->queue_high);
  rg_thread_list_free (m, &m->queue_middle);
  rg_thread_list_free (m, &m->queue_low);
  rg_thread_list_free (m, &m->read);
  rg_thread_list_free (m, &m->read_high);
  rg_thread_list_free (m, &m->write);
  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    rg_thread_list_free (m, &m->timer[i]);
  rg_thread_list_free (m, &m->event);
  rg_thread_list_free (m, &m->event_low);
  rg_thread_list_free (m, &m->unuse);

  if (m->pfds)
    XFREE (MTYPE_POLLFD, m->pfds);

  XFREE (MTYPE_THREAD_MASTER, m);
}

/* Thread list is empty or not.  */
int
rg_thread_empty (struct rg_thread_list *list)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  return  list->head ? 0 : 1;
}

/* Return remain time in second. */
u_int32_t
rg_thread_timer_remain_second (struct rg_thread *thread)
{
  struct timeval timer_now;

  if (thread == NULL)
    return 0;

  rg_time_tzcurrent (&timer_now, NULL);

  if (thread->u.sands.tv_sec - timer_now.tv_sec > 0)
    return thread->u.sands.tv_sec - timer_now.tv_sec;
  else
    return 0;
}

/* Get new thread.  */
struct rg_thread *
rg_thread_get (struct rg_thread_master *m, char type,
            int (*func) (struct rg_thread *), void *arg)
{
  struct rg_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  if (m->unuse.head)
    thread = rg_thread_trim_head (&m->unuse);
  else
    {
      thread = XMALLOC (MTYPE_THREAD, sizeof (struct rg_thread));
      if (thread == NULL)
        {
          RG_THREAD_INTERFACE_DBG (m, "Failed to malloc memory for rg-thread.\n");
          return NULL;
        }

      m->alloc++;
    }
  thread->type = type;
  thread->master = m;
  thread->func = func;
  thread->arg = arg;
  thread->pfd_index = 0;
  memset (thread->name, 0, sizeof (thread->name));
  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_IN_LIST);

  return thread;
}

static int
rg_thread_name_check (struct rg_thread_master *m, char *name)
{
  assert (name);

  /* 如果名字过长，只是调试信息提醒，后续保存时只截取10个字符 */
  if (strlen (name) > RG_THREAD_NAME_LEN)
    {
      RG_THREAD_INTERFACE_DBG (m, "Rg-thread name length exceeded %d.\n",
                                       RG_THREAD_NAME_LEN);
    }

  return 0;
}

static int
rg_thread_fd_check (struct rg_thread_master *m, int fd)
{
  if (fd < 0)
    {
      RG_THREAD_INTERFACE_DBG (m, "Invalid fd %d.\n", fd);
      return -1;
    }

  if (fd > RG_THREAD_SELECT_FD_MAX)
    {
      /* 如果包含超过1024的fd，那么必须使用poll机制 */
      assert (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL));
      if (!RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL))
        {
          RG_THREAD_PRINT ("RG THREAD ERROR fd > %d, %d\n", RG_THREAD_SELECT_FD_MAX, fd);
          return -1;
        }
    }

  return 0;
}

static void
rg_thread_name_add (struct rg_thread *thread, char *name)
{
  memset (thread->name, 0, sizeof(thread->name));
  if (name != NULL)
    strncpy (thread->name, name, RG_THREAD_NAME_LEN);

  return;
}

static inline void
rg_thread_add_thread_check(struct rg_thread_master *m, struct rg_thread *thread)
{
  int tid;

  if (!RG_THREAD_DEBUG_INTERFACE_ON(m))
      return;

  RG_THREAD_PRINT("Check thread %p.\n", thread);

  tid = rg_thread_gettid();
  if (tid != m->tid && m->tid != 0)
    {
      RG_THREAD_PRINT ("Error: rg-thread %p is added from another p-thread, [%d->%d].\n",
              thread, tid, m->tid);
    }

  return;
}

static inline int
rg_thread_insert_thread_arg_check(struct rg_thread_master *m, struct rg_thread *thread, char *name)
{
  if (m == NULL)
    {
      return -1;
    }

  if (thread == NULL)
    {
      RG_THREAD_INTERFACE_DBG (m, "Insert new thread failed(thread arg null).\n");
      return -1;
    }

  rg_thread_add_thread_check(m, thread);

  if (name != NULL && rg_thread_name_check (m, name))
    return -1;

  return 0;
}

/* Insert new thread.  */
int
rg_thread_insert (struct rg_thread_master *m, struct rg_thread *thread, char type,
            int (*func) (struct rg_thread *), void *arg, char *name)
{
  thread->type = type;
  thread->master = m;
  thread->func = func;
  thread->arg = arg;

  rg_thread_name_add (thread, name);
  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_IN_LIST);

  return 0;
}

/* Keep track of the maximum file descriptor for read/write. */
static void
rg_thread_update_max_fd (struct rg_thread_master *m, int fd)
{
  if (m && m->max_fd < fd)
    m->max_fd = fd;
}

static inline void
rg_thread_update_read (struct rg_thread_master *m, struct rg_thread *thread, int fd)
{
  thread->u.fd = fd;  /* 必须先设置，在下面的函数中会用到该值 */

  if (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL))
    {
      rg_thread_set_pollfd (thread, POLLHUP | POLLIN);
    }
  else
    {
      rg_thread_update_max_fd (m, fd);
      FD_SET (fd, &m->readfd);
    }
}

/* Add new read thread. */
struct rg_thread *
rg_thread_add_read (struct rg_thread_master *m,
                    int (*func) (struct rg_thread *),
                    void *arg, int fd)
{
  struct rg_thread *thread;

  assert (m != NULL);

  if (rg_thread_fd_check (m, fd) < 0)
    {
      return NULL;
    }

  RG_THREAD_INTERFACE_DBG (m, "Add new read thread fd %d, func %p.\n", fd, func);

  thread = rg_thread_get (m, RG_THREAD_READ, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  rg_thread_update_read(m, thread, fd);
  rg_thread_list_add (&m->read, thread);

  return thread;
}

/* Add new read thread with name. */
struct rg_thread *
rg_thread_add_read_withname (struct rg_thread_master *m,
                             int (*func) (struct rg_thread *),
                             void *arg, int fd, char *name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_read (m, func, arg, fd);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_read (m, func, arg, fd);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Add new high priority read thread. */
struct rg_thread *
rg_thread_add_read_high (struct rg_thread_master *m,
                         int (*func) (struct rg_thread *),
                         void *arg, int fd)
{
  struct rg_thread *thread;

  assert (m != NULL);

  if (rg_thread_fd_check (m, fd) < 0)
    {
      return NULL;
    }

  RG_THREAD_INTERFACE_DBG (m, "Add new read high thread fd %d, func %p.\n", fd, func);

  thread = rg_thread_get (m, RG_THREAD_READ_HIGH, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  rg_thread_update_read(m, thread, fd);
  rg_thread_list_add (&m->read_high, thread);

  return thread;
}

/* Add new high priority read thread with name. */
struct rg_thread *
rg_thread_add_read_high_withname (struct rg_thread_master *m,
                                  int (*func) (struct rg_thread *),
                                  void *arg, int fd, char *name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_read_high (m, func, arg, fd);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_read_high (m, func, arg, fd);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Insert new read thread. */
int
rg_thread_insert_read (struct rg_thread_master *m, struct rg_thread *thread,
                             int (*func) (struct rg_thread *),
                             void *arg, int fd, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  if (rg_thread_fd_check (m, fd) < 0)
    {
      return -1;
    }

  RG_THREAD_INTERFACE_DBG (m, "Insert new read thread fd %d, func %p.\n", fd, func);

  rg_thread_insert(m, thread, RG_THREAD_READ, func, arg, name);
  rg_thread_update_read(m, thread, fd);
  rg_thread_list_add (&m->read, thread);
  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Insert new high priority read thread. */
int
rg_thread_insert_read_high (struct rg_thread_master *m, struct rg_thread *thread,
                         int (*func) (struct rg_thread *),
                         void *arg, int fd, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  if (rg_thread_fd_check (m, fd) < 0)
    {
      return -1;
    }

  RG_THREAD_INTERFACE_DBG (m, "Insert new read high thread fd %d, func %p, %s.\n", fd, func, name);

  rg_thread_insert(m, thread, RG_THREAD_READ_HIGH, func, arg, name);
  rg_thread_update_read(m, thread, fd);
  rg_thread_list_add (&m->read_high, thread);
  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

static inline void
rg_thread_update_write (struct rg_thread_master *m, struct rg_thread *thread, int fd)
{
  thread->u.fd = fd;  /* 必须先设置，在下面的函数中会用到该值 */

  if (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL))
    {
      rg_thread_set_pollfd (thread, POLLHUP | POLLOUT);
    }
  else
    {
      rg_thread_update_max_fd (m, fd);
      FD_SET (fd, &m->writefd);
    }

  rg_thread_list_add (&m->write, thread);
}

/* Add new write thread. */
struct rg_thread *
rg_thread_add_write (struct rg_thread_master *m,
                     int (*func) (struct rg_thread *),
                     void *arg, int fd)
{
  struct rg_thread *thread;

  assert (m != NULL);

  if (rg_thread_fd_check (m, fd) < 0 || FD_ISSET (fd, &m->writefd))
    {
      return NULL;
    }

  RG_THREAD_INTERFACE_DBG (m, "Add new write thread fd %d, func %p.\n", fd, func);

  thread = rg_thread_get (m, RG_THREAD_WRITE, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  rg_thread_update_write(m, thread, fd);

  return thread;
}

/* Add new write read thread with name. */
struct rg_thread *
rg_thread_add_write_withname (struct rg_thread_master *m,
                              int (*func) (struct rg_thread *),
                              void *arg, int fd, char *name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_write (m, func, arg, fd);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_write (m, func, arg, fd);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Insert new write thread. */
int
rg_thread_insert_write (struct rg_thread_master *m, struct rg_thread *thread,
                     int (*func) (struct rg_thread *),
                     void *arg, int fd, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  if (rg_thread_fd_check (m, fd) < 0 || FD_ISSET (fd, &m->writefd))
    {
      return -1;
    }

  RG_THREAD_INTERFACE_DBG (m, "Insert new write thread fd %d, func %p.\n", fd, func);

  rg_thread_insert(m, thread, RG_THREAD_WRITE, func, arg, name);
  rg_thread_update_write(m, thread, fd);
  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

static void
rg_thread_add_timer_common (struct rg_thread_master *m, struct rg_thread *thread)
{
#ifndef TIMER_NO_SORT
  struct rg_thread *tt;
#endif /* TIMER_NO_SORT */

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  /* Set index.  */
  thread->index = m->index;

  /* Sort by timeval. */
#ifdef TIMER_NO_SORT
  rg_thread_list_add (&m->timer[m->index], thread);
#else
  for (tt = m->timer[m->index].tail; tt; tt = tt->prev)
    if (timeval_cmp (thread->u.sands, tt->u.sands) >= 0)
      break;

  rg_thread_list_add_after (&m->timer[m->index], tt, thread);
#endif /* TIMER_NO_SORT */

  /* Increment timer slot index.  */
  m->index++;
  m->index %= RG_THREAD_TIMER_SLOT;
}

static void
rg_thread_update_time(struct rg_thread * thread, long timer)
{
  struct timeval timer_now;

  rg_time_tzcurrent (&timer_now, NULL);
#ifdef HAVE_NGSA
  if (timer < 0)
    {
      timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
    }
  else
    {
      timer_now.tv_sec += timer;
      if (timer_now.tv_sec < 0)
        timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
    }
#else
  timer_now.tv_sec += timer;
#endif

  thread->u.sands = timer_now;
}

static void
rg_thread_update_timeval(struct rg_thread *thread, struct timeval *timer)
{
  struct timeval timer_now;

  /* Do we need jitter here? */
  rg_time_tzcurrent (&timer_now, NULL);
  timer_now.tv_sec += timer->tv_sec;
  timer_now.tv_usec += timer->tv_usec;
  while (timer_now.tv_usec >= TV_USEC_PER_SEC)
    {
      timer_now.tv_sec++;
      timer_now.tv_usec -= TV_USEC_PER_SEC;
    }

  /* Correct negative value.  */
  if (timer_now.tv_sec < 0)
    timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
  if (timer_now.tv_usec < 0)
    timer_now.tv_usec = PAL_TIME_MAX_TV_USEC;

  thread->u.sands = timer_now;
}

/* Add timer thread. */
struct rg_thread *
rg_thread_add_timer (struct rg_thread_master *m,
                     int (*func) (struct rg_thread *),
                     void *arg, long timer)
{
  struct rg_thread *thread;

  assert (m != NULL);
  thread = rg_thread_get (m, RG_THREAD_TIMER, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  rg_thread_update_time(thread, timer);

  /* Common process.  */
  rg_thread_add_timer_common (m, thread);

  return thread;
}

/* Add new timer thread with name. */
struct rg_thread *
rg_thread_add_timer_withname (struct rg_thread_master * m,
                              int(* func)(struct rg_thread *),
                              void * arg, long timer, char * name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_timer (m, func, arg, timer);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_timer (m, func, arg, timer);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Add timer thread. */
struct rg_thread *
rg_thread_add_timer_timeval (struct rg_thread_master *m,
                             int (*func) (struct rg_thread *),
                             void *arg, struct timeval timer)
{
  struct rg_thread *thread;

  assert (m != NULL);

  thread = rg_thread_get (m, RG_THREAD_TIMER, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  rg_thread_update_timeval(thread, &timer);

  /* Common process.  */
  rg_thread_add_timer_common (m, thread);

  return thread;
}

/* Add new timerval thread with name. */
struct rg_thread *
rg_thread_add_timer_timeval_withname (struct rg_thread_master * m,
                                      int(* func)(struct rg_thread *),
                                      void * arg, struct timeval timer,
                                      char * name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_timer_timeval (m, func, arg, timer);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_timer_timeval (m, func, arg, timer);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Insert timer thread. */
int
rg_thread_insert_timer (struct rg_thread_master *m, struct rg_thread *thread,
                     int (*func) (struct rg_thread *),
                     void *arg, long timer, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  rg_thread_insert(m, thread, RG_THREAD_TIMER, func, arg, name);
  rg_thread_update_time(thread, timer);

  /* Common process.  */
  rg_thread_add_timer_common (m, thread);

  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Insert timer thread. */
int
rg_thread_insert_timer_timeval (struct rg_thread_master *m, struct rg_thread *thread,
                             int (*func) (struct rg_thread *),
                             void *arg, struct timeval timer, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  rg_thread_insert(m, thread, RG_THREAD_TIMER, func, arg, name);

  rg_thread_update_timeval(thread, &timer);

  /* Common process.  */
  rg_thread_add_timer_common (m, thread);

  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Add simple event thread. */
struct rg_thread *
rg_thread_add_event (struct rg_thread_master *m,
                     int (*func) (struct rg_thread *),
                     void *arg, int val)
{
  struct rg_thread *thread;

  assert (m != NULL);

  thread = rg_thread_get (m, RG_THREAD_EVENT, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  thread->u.val = val;
  rg_thread_list_add (&m->event, thread);

  return thread;
}

/* Add new event thread with name. */
struct rg_thread *
rg_thread_add_event_withname (struct rg_thread_master * m,
                              int(* func)(struct rg_thread *),
                              void * arg, int val, char * name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_event (m, func, arg, val);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_event (m, func, arg, val);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Add low priority event thread. */
struct rg_thread *
rg_thread_add_event_low (struct rg_thread_master *m,
                         int (*func) (struct rg_thread *),
                         void *arg, int val)
{
  struct rg_thread *thread;

  assert (m != NULL);

  thread = rg_thread_get (m, RG_THREAD_EVENT_LOW, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  thread->u.val = val;
  rg_thread_list_add (&m->event_low, thread);

  return thread;
}

/* Add new event low thread with name. */
struct rg_thread *
rg_thread_add_event_low_withname (struct rg_thread_master * m,
                                  int(* func)(struct rg_thread *),
                                  void * arg, int val, char * name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_event_low (m, func, arg, val);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_event_low (m, func, arg, val);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Insert event thread. */
int
rg_thread_insert_event (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func) (struct rg_thread *),
        void *arg, int val, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  rg_thread_insert(m, thread, RG_THREAD_EVENT, func, arg, name);
  thread->u.val = val;
  rg_thread_list_add (&m->event, thread);

  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Insert event thread. */
int
rg_thread_insert_event_low (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func) (struct rg_thread *),
        void *arg, int val, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  rg_thread_insert(m, thread, RG_THREAD_EVENT_LOW, func, arg, name);
  thread->u.val = val;
  rg_thread_list_add (&m->event_low, thread);

  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Add pending read thread. */
struct rg_thread *
rg_thread_add_read_pend (struct rg_thread_master *m,
                         int (*func) (struct rg_thread *),
                         void *arg, int val)
{
  struct rg_thread *thread;

  assert (m != NULL);

  thread = rg_thread_get (m, RG_THREAD_READ_PEND, func, arg);
  if (thread == NULL)
    return NULL;

  rg_thread_add_thread_check(m, thread);

  thread->u.val = val;
  rg_thread_list_add (&m->read_pend, thread);

  return thread;
}

/* Add new read pending thread with name. */
struct rg_thread *
rg_thread_add_read_pend_withname (struct rg_thread_master *m,
                                  int(* func)(struct rg_thread *),
                                  void * arg, int val, char * name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_read_pend (m, func, arg, val);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_read_pend (m, func, arg, val);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

/* Insert read pend thread. */
int
rg_thread_insert_read_pend (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func) (struct rg_thread *),
        void *arg, int val, char *name)
{
  if (rg_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  rg_thread_insert(m, thread, RG_THREAD_READ_PEND, func, arg, name);
  thread->u.val = val;
  rg_thread_list_add (&m->read_pend, thread);

  RG_THREAD_SET_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Cancel thread from scheduler. */
void
rg_thread_cancel (struct rg_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(thread != NULL);
#endif

  rg_thread_add_thread_check(thread->master, thread);

  switch (thread->type)
    {
    case RG_THREAD_READ:
      if (RG_THREAD_CHECK_FLAG(thread->master->flag, RG_THREAD_USE_POLL))
        rg_thread_clear_pollfd (thread);
      else
        FD_CLR (thread->u.fd, &thread->master->readfd);
      rg_thread_list_delete (&thread->master->read, thread);
      break;
    case RG_THREAD_READ_HIGH:
      if (RG_THREAD_CHECK_FLAG(thread->master->flag, RG_THREAD_USE_POLL))
        rg_thread_clear_pollfd (thread);
      else
        FD_CLR (thread->u.fd, &thread->master->readfd);
      rg_thread_list_delete (&thread->master->read_high, thread);
      break;
    case RG_THREAD_WRITE:
      assert (FD_ISSET (thread->u.fd, &thread->master->writefd));
      if (RG_THREAD_CHECK_FLAG(thread->master->flag, RG_THREAD_USE_POLL))
        rg_thread_clear_pollfd (thread);
      else
        FD_CLR (thread->u.fd, &thread->master->writefd);
      rg_thread_list_delete (&thread->master->write, thread);
      break;
    case RG_THREAD_TIMER:
      rg_thread_list_delete (&thread->master->timer[(int)thread->index], thread);
      break;
    case RG_THREAD_EVENT:
      rg_thread_list_delete (&thread->master->event, thread);
      break;
    case RG_THREAD_READ_PEND:
      rg_thread_list_delete (&thread->master->read_pend, thread);
      break;
    case RG_THREAD_EVENT_LOW:
      rg_thread_list_delete (&thread->master->event_low, thread);
      break;
    case RG_THREAD_QUEUE:
      switch (thread->priority)
        {
        case RG_THREAD_PRIORITY_HIGH:
          rg_thread_list_delete (&thread->master->queue_high, thread);
          break;
        case RG_THREAD_PRIORITY_MIDDLE:
          rg_thread_list_delete (&thread->master->queue_middle, thread);
          break;
        case RG_THREAD_PRIORITY_LOW:
          rg_thread_list_delete (&thread->master->queue_low, thread);
          break;
        }
      break;
    default:
      RG_THREAD_PROCEDURE_DBG (thread->master, "Invalid thread type %d.\n", thread->type);
      break;
    }
  rg_thread_unuse(thread->master, thread);
}

/* Delete all events which has argument value arg. */
void
rg_thread_cancel_event (struct rg_thread_master *m, void *arg)
{
  struct rg_thread *thread;
  struct rg_thread *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->event.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (&m->event, t);
          rg_thread_unuse(m, t);
        }
    }

  /* Since Event could have been Queued search queue_high */
  thread = m->queue_high.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (&m->queue_high, t);
          rg_thread_unuse(m, t);
        }
    }

  return;
}

/* Delete all low-events which has argument value arg */
void
rg_thread_cancel_event_low (struct rg_thread_master *m, void *arg)
{
  struct rg_thread *thread;
  struct rg_thread *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->event_low.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (&m->event_low, t);
          rg_thread_unuse(m, t);
        }
    }

  /* Since Event could have been Queued search queue_low */
  thread = m->queue_low.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (&m->queue_low, t);
          rg_thread_unuse(m, t);
        }
    }

  return;
}

/* Delete all read events which has argument value arg. */
void
rg_thread_cancel_read (struct rg_thread_master *m, void *arg)
{
  struct rg_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->read.head;
  while (thread)
    {
      struct rg_thread *t;

      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          rg_thread_list_delete (&m->read, t);
          t->type = RG_THREAD_UNUSED;
          rg_thread_add_unuse (m, t);
        }
    }
}

/* Delete all write events which has argument value arg. */
void
rg_thread_cancel_write (struct rg_thread_master *m, void *arg)
{
  struct rg_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->write.head;
  while (thread)
    {
      struct rg_thread *t;

      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          rg_thread_list_delete (&m->write, t);
          t->type = RG_THREAD_UNUSED;
          rg_thread_add_unuse (m, t);
        }
    }
}

/* Delete all timer events which has argument value arg. */
void
rg_thread_cancel_timer (struct rg_thread_master *m, void *arg)
{
  struct rg_thread *thread;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    {
      thread = m->timer[i].head;
      while (thread)
        {
          struct rg_thread *t;

          t = thread;
          thread = t->next;

          if (t->arg == arg)
            {
              rg_thread_list_delete (&m->timer[i], t);
              t->type = RG_THREAD_UNUSED;
              rg_thread_add_unuse (m, t);
            }
        }
    }
}

#ifdef RTOS_DEFAULT_WAIT_TIME
struct timeval *
rg_thread_timer_wait (struct rg_thread_master *m, struct timeval *timer_val)
{
#ifdef LIB_PARAM_CHECK
  assert(timer_val != NULL);
#endif

  timer_val->tv_sec = 1;
  timer_val->tv_usec = 0;
  return timer_val;
}
#else /* ! RTOS_DEFAULT_WAIT_TIME */
#ifdef HAVE_RTOS_TIMER
struct timeval *
rg_thread_timer_wait (struct rg_thread_master *m, struct timeval *timer_val)
{
  rtos_set_time (timer_val);
  return timer_val;
}
#else /* ! HAVE_RTOS_TIMER */
#ifdef HAVE_RTOS_TIC
struct timeval *
rg_thread_timer_wait (struct rg_thread_master *m, struct timeval *timer_val)
{
#ifdef LIB_PARAM_CHECK
  assert(timer_val != NULL);
#endif

  timer_val->tv_sec = 0;
  timer_val->tv_usec = 10;
  return timer_val;
}
#else /* ! HAVE_RTOS_TIC */
#ifdef TIMER_NO_SORT
struct timeval *
rg_thread_timer_wait (struct rg_thread_master *m, struct timeval *timer_val)
{
  struct timeval timer_now;
  struct timeval timer_min;
  struct timeval *timer_wait;
  struct rg_thread *thread;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  timer_wait = NULL;

  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    for (thread = m->timer[i].head; thread; thread = thread->next)
      {
        if (! timer_wait)
          timer_wait = &thread->u.sands;
        else if (timeval_cmp (thread->u.sands, *timer_wait) < 0)
          timer_wait = &thread->u.sands;
      }

  if (timer_wait)
    {
      timer_min = *timer_wait;

      rg_time_tzcurrent (&timer_now, NULL);
      timer_min = timeval_subtract (timer_min, timer_now);

      if (timer_min.tv_sec < 0)
        {
          timer_min.tv_sec = 0;
          timer_min.tv_usec = 10;
        }

      *timer_val = timer_min;
      return timer_val;
    }
  return NULL;
}
#else /* ! TIMER_NO_SORT */
/* Pick up smallest timer.  */
struct timeval *
rg_thread_timer_wait (struct rg_thread_master *m, struct timeval *timer_val)
{
  struct timeval timer_now;
  struct timeval timer_min;
  struct timeval *timer_wait;
  struct rg_thread *thread;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  timer_wait = NULL;

  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    if ((thread = m->timer[i].head) != NULL)
      {
        if (! timer_wait)
          timer_wait = &thread->u.sands;
        else if (timeval_cmp (thread->u.sands, *timer_wait) < 0)
          timer_wait = &thread->u.sands;
      }

  if (timer_wait)
    {
      timer_min = *timer_wait;

      rg_time_tzcurrent (&timer_now, NULL);
      timer_min = timeval_subtract (timer_min, timer_now);

      if (timer_min.tv_sec < 0)
        {
          timer_min.tv_sec = 0;
          timer_min.tv_usec = 10;
        }

      *timer_val = timer_min;
      return timer_val;
    }
  return NULL;
}
#endif /* TIMER_NO_SORT */
#endif /* HAVE_RTOS_TIC */
#endif /* HAVE_RTOS_TIMER */
#endif /* RTOS_DEFAULT_WAIT_TIME */

#ifdef HAVE_NGSA
/* lookup thread which has argument value type, arg and func ptr, from the thread list. */
struct rg_thread *
rg_thread_lookup_list_by_arg_func (struct rg_thread_list *list, char type,
                                   int (*func)(struct rg_thread *), void *arg, int val)
{
  struct rg_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
  assert(func != NULL);
  assert(arg != NULL);
#endif

  for (thread = list->head; thread != NULL; thread = thread->next)
    {
      if (thread->type == type && thread->arg == arg
          && thread->func == func && thread->u.val == val)
        {
          return thread;
        }
    }

  return NULL;
}

/* Delete all events which has argument value arg and func ptr. */
void
rg_thread_cancel_event_by_arg_func (struct rg_thread_master *m,
                                 int (*func)(struct rg_thread *), void *arg)
{
  struct rg_thread *thread;
  struct rg_thread *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(func != NULL);
#endif

  thread = m->event.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg && t->func == func)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (&m->event, t);
          rg_thread_unuse(m, t);
        }
    }

  /* Since Event could have been Queued search queue_high */
  thread = m->queue_high.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg && t->func == func)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (&m->queue_high, t);
          rg_thread_unuse(m, t);
        }
    }

  return;
}

/* Delete thread which has argument value arg and func ptr, from the thread list. */
void
rg_thread_cancel_list_by_arg_func (struct rg_thread_master *m, struct rg_thread_list *list,
                                int (*func)(struct rg_thread *), void *arg)
{
  struct rg_thread *thread, *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
  assert(func != NULL);
  assert(arg != NULL);
#endif

  thread = list->head;
  while (thread)
    {
      t = thread;
      thread = t->next;
      if (t->arg == arg && t->func == func)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (list, t);
          rg_thread_unuse(m, t);
        }
    }
}

/* Delete all thread which has argument value arg and func ptr */
void
rg_thread_cancel_by_arg_func (struct rg_thread_master *m,
                           int (*func)(struct rg_thread *), void *arg)
{
  struct rg_thread_list *list;
  int i = 0;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(func != NULL);
  assert(arg != NULL);
#endif

  /* struct thread_list queue_high; */
  list = &m->queue_high;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list queue_middle; */
  list = &m->queue_middle;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list queue_low; */
  list = &m->queue_low;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list timer[THREAD_TIMER_SLOT]; */
  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    {
      list = &m->timer[i];
      rg_thread_cancel_list_by_arg_func (m, list, func, arg);
    }

  /* struct thread_list read_pend; */
  list = &m->read_pend;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list read_high; */
  list =&m->read_high;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list read; */
  list = &m->read;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list write; */
  list = &m->write;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list event; */
  list = &m->event;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list event_low; */
  list = &m->event_low;
  rg_thread_cancel_list_by_arg_func (m, list, func, arg);
}

void
rg_thread_cancel_arg_check_list (struct rg_thread_master *m, struct rg_thread_list *list, void *arg)
{
  struct rg_thread *thread, *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
  assert(arg != NULL);
#endif

  thread = list->head;
  while (thread)
    {
      t = thread;
      thread = t->next;
      if (t->arg == arg)
        {
#ifdef LIB_PARAM_CHECK
          RG_THREAD_ASSERT (0);
          RG_THREAD_PRINT ("Function 0x%p in invalid thread list\n", t->func);
#endif
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (list, t);
          rg_thread_unuse(m, t);
        }
    }
}

void
rg_thread_cancel_arg_check (struct rg_thread_master *m, void *arg)
{
  struct rg_thread_list *list;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(arg != NULL);
#endif

  /* struct thread_list queue_high; */
  list = &m->queue_high;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list queue_middle; */
  list = &m->queue_middle;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list queue_low; */
  list = &m->queue_low;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list timer[THREAD_TIMER_SLOT]; */
  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    {
      list = &m->timer[i];
      rg_thread_cancel_arg_check_list(m, list, arg);
    }

  /* struct thread_list read_pend; */
  list = &m->read_pend;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list read_high; */
  list =&m->read_high;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list read; */
  list = &m->read;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list write; */
  list = &m->write;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list event; */
  list = &m->event;
  rg_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list event_low; */
  list = &m->event_low;
  rg_thread_cancel_arg_check_list(m, list, arg);
}

void
rg_thread_cancel_from_list_by_arg (struct rg_thread_master *m, struct rg_thread_list *list, void *arg)
{
  struct rg_thread *thread, *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
  assert(arg != NULL);
#endif

  thread = list->head;
  while (thread)
    {
      t = thread;
      thread = t->next;
      if (t->arg == arg)
        {
          rg_thread_add_thread_check(t->master, thread);

          rg_thread_list_delete (list, t);
          rg_thread_unuse(m, t);
        }
    }
}

void
rg_thread_cancel_by_arg (struct rg_thread_master *m, void *arg)
{
  struct rg_thread_list *list;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(arg != NULL);
#endif

  /* struct thread_list queue_high; */
  list = &m->queue_high;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list queue_middle; */
  list = &m->queue_middle;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list queue_low; */
  list = &m->queue_low;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list timer[THREAD_TIMER_SLOT]; */
  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    {
      list = &m->timer[i];
      rg_thread_cancel_from_list_by_arg(m, list, arg);
    }

  /* struct thread_list read_pend; */
  list = &m->read_pend;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list read_high; */
  list =&m->read_high;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list read; */
  list = &m->read;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list write; */
  list = &m->write;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list event; */
  list = &m->event;
  rg_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list event_low; */
  list = &m->event_low;
  rg_thread_cancel_from_list_by_arg(m, list, arg);
}

void
rg_thread_cancel_invalid_fd(struct rg_thread_master *m, struct rg_thread_list *list)
{
  struct rg_thread *thread;
  struct rg_thread *next;
  struct sockaddr address;
  unsigned int address_len;
  int ret;

#ifdef LIB_PARAM_CHECK
   assert(list != NULL);
#endif

  for (thread = list->head; thread; thread = next)
    {
      next = thread->next;
      ret = getsockname(RG_THREAD_FD (thread), &address, &address_len);
      if (ret < 0)
          rg_thread_cancel(thread);
    }
}
#endif

struct rg_thread *
rg_thread_run (struct rg_thread_master *m, struct rg_thread *thread,
               struct rg_thread *fetch)
{
#ifdef LIB_PARAM_CHECK
  assert(thread != NULL);
#endif

  RG_THREAD_PROCEDURE_DBG (m, "Rg-thread run thread %p[%s], func %p.\n",
          thread, thread->name, thread->func);

  *fetch = *thread;

  rg_thread_unuse(m, thread);

  return fetch;
}

void
rg_thread_enqueue_high (struct rg_thread_master *m, struct rg_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  thread->type = RG_THREAD_QUEUE;
  thread->priority = RG_THREAD_PRIORITY_HIGH;
  rg_thread_list_add (&m->queue_high, thread);
}

void
rg_thread_enqueue_middle (struct rg_thread_master *m, struct rg_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  thread->type = RG_THREAD_QUEUE;
  thread->priority = RG_THREAD_PRIORITY_MIDDLE;
  rg_thread_list_add (&m->queue_middle, thread);
}

void
rg_thread_enqueue_low (struct rg_thread_master *m, struct rg_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  thread->type = RG_THREAD_QUEUE;
  thread->priority = RG_THREAD_PRIORITY_LOW;
  rg_thread_list_add (&m->queue_low, thread);
}

/* When the file is ready move to queueu.  */
int
rg_thread_process_fd (struct rg_thread_master *m, struct rg_thread_list *list,
                      fd_set *fdset, fd_set *mfdset)
{
  struct rg_thread *thread;
  struct rg_thread *next;
  int ready = 0;

#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  for (thread = list->head; thread; thread = next)
    {
      next = thread->next;

      if (FD_ISSET (RG_THREAD_FD (thread), fdset))
        {
          FD_CLR(RG_THREAD_FD (thread), mfdset);
          rg_thread_list_delete (list, thread);
          rg_thread_enqueue_middle (m, thread);
          ready++;
        }
    }
  return ready;
}

/* When the file is ready move to queueu.  */
int
rg_thread_process_poll_fd (struct rg_thread_master *m, struct rg_thread_list *list,
                      int found, bool is_write)
{
  struct rg_thread *thread;
  struct rg_thread *next;
  short revents;
  int ready = 0;

#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  for (thread = list->head; thread && found > 0; thread = next)
    {
      next = thread->next;

      if (m->pfds[thread->pfd_index].fd < 0)
        {
          /* 该伪线程对应的pollfd没有设置，说明还没有被侦听 */
          RG_THREAD_PROCEDURE_DBG(m,
              "Thread not listened, %s, %d\n", thread->name, thread->pfd_index);
          continue;
        }

      revents = m->pfds[thread->pfd_index].revents;

      if (revents == 0)
        {
          /* 没有事件发生 */
          RG_THREAD_PROCEDURE_DBG(m,
              "Nothing happend, %s, %d\n", thread->name, thread->pfd_index);
          continue;
        }

      if (is_write)
        {
          if (!(revents & (POLLOUT | POLLWRNORM | POLLWRBAND)))
            {
              /* 如果是遍历写伪线程，却没有返回可写事件，就不用继续了 */
              RG_THREAD_PROCEDURE_DBG(m,
                  "Write thread recved pollin, %s, %d, %d\n",
                  thread->name, thread->pfd_index, revents);
              continue;
            }
        }
      else
        {
          /*
           * 增加POLLHUP是因为poll时，如果直接创建socket没有connect此时去侦听会返回这个值；
           * 此外，连接断开时会返回POLLHUP | POLLIN；
           */
          if (!(revents & (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLHUP)))
            {
              /* 如果是遍历读伪线程，却没有返回可读事件，就不用继续了 */
              RG_THREAD_PROCEDURE_DBG(m,
                  "Read thread recved pollout, %s, %d, %d\n",
                  thread->name, thread->pfd_index, revents);
              continue;
            }
        }

      found--;
      /* 将伪线程对应的pollfd数组清空，避免再次被侦听 */
      rg_thread_clear_pollfd (thread);

      /* 将伪线程放入可执行队列中 */
      rg_thread_list_delete (list, thread);
      rg_thread_enqueue_middle (m, thread);
      if (RG_THREAD_DEBUG_PROCEDURE_ON (m))
        rg_thread_disp_thread(thread);
      ready++;
    }

  return ready;
}

/* rg-thread type strings. */
static char *rg_thread_get_type_str (char type, char pri)
{
  switch (type)
    {
    case RG_THREAD_READ:
      return "read";
    case RG_THREAD_WRITE:
      return "write";
    case RG_THREAD_TIMER:
      return "timer";
    case RG_THREAD_EVENT:
      return "event";
    case RG_THREAD_QUEUE:
      switch (pri)
        {
        case RG_THREAD_PRIORITY_HIGH:
          return "high";
        case RG_THREAD_PRIORITY_MIDDLE:
          return "middle";
        case RG_THREAD_PRIORITY_LOW:
          return "low";
        default:
          assert (0);
          return NULL;
        }
      break;
    case RG_THREAD_UNUSED:
      return "unuse";
    case RG_THREAD_READ_HIGH:
      return "read high";
    case RG_THREAD_READ_PEND:
      return "read pend";
    case RG_THREAD_EVENT_LOW:
      return "event low";
    default:
      assert (0);
      return NULL;
    }

  return NULL;
}

static void
rg_thread_disp_invalid_fd_thread (struct rg_thread *thread)
{
  if (thread == NULL)
    return;

  rg_thread_debug_msg ("Rg-thread display thread:\n");
  rg_thread_debug_msg (" prev:%p\n", thread->prev);
  rg_thread_debug_msg (" next:%p\n", thread->next);
  rg_thread_debug_msg (" master:%p\n", thread->master);
  rg_thread_debug_msg (" func:%p\n", thread->func);
  rg_thread_debug_msg (" zg:%p\n", thread->zg);
  rg_thread_debug_msg (" arg:%p\n", thread->arg);
  rg_thread_debug_msg (" type:%s(%d,%d)\n", rg_thread_get_type_str (thread->type, thread->priority),
                   thread->type, thread->priority);
  rg_thread_debug_msg (" index:%d\n", thread->index);
  rg_thread_debug_msg (" fd:%d\n", thread->u.fd);
  rg_thread_debug_msg (" name:%s\n", thread->name);

  return;
}

static void
rg_thread_disp_invalid_fd_list (struct rg_thread_master *m, struct rg_thread_list *list)
{
  struct rg_thread *thread;
  struct rg_thread *next;
  struct sockaddr address;
  unsigned int address_len;
  int ret;

#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  for (thread = list->head; thread; thread = next)
    {
      next = thread->next;
      ret = getsockname(RG_THREAD_FD (thread), &address, &address_len);
      if (ret < 0)
        rg_thread_disp_invalid_fd_thread (thread);

      if (RG_THREAD_FD (thread) > RG_THREAD_SELECT_FD_MAX)
        {
          rg_thread_debug_msg("unsupport fd %d\n", RG_THREAD_FD (thread));
          rg_thread_disp_invalid_fd_thread (thread);
        }
    }

  return;
}

static void
rg_thread_disp_invalid_fd (struct rg_thread_master *m)
{
  struct rg_thread_list *list;

#ifdef LIB_PARAM_CHECK
  assert (m != NULL);
#endif

  /* struct thread_list read_high; */
  list =&m->read_high;
  rg_thread_disp_invalid_fd_list(m, list);

  /* struct thread_list read; */
  list = &m->read;
  rg_thread_disp_invalid_fd_list(m, list);

  /* struct thread_list write; */
  list = &m->write;
  rg_thread_disp_invalid_fd_list (m, list);

  return;
}

/* Fetch next ready thread. */
struct rg_thread *
rg_thread_fetch (struct rg_thread_master *m, struct rg_thread *fetch)
{
  int num;
  struct rg_thread *thread;
  struct rg_thread *next;
  fd_set readfd;
  fd_set writefd;
  fd_set exceptfd;
  struct timeval timer_now;
  struct timeval timer_val;
  struct timeval *timer_wait;
  struct timeval timer_nowait;
  long timeout_poll;  /* poll机制使用的超时时间 */
  int i;

  if (m == NULL)
    {
      assert(0);
      return NULL;
    }

  if (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_IS_FIRST_FETCH))
    {
      m->tid = rg_thread_gettid();
      RG_THREAD_UNSET_FLAG(m->flag, RG_THREAD_IS_FIRST_FETCH);
    }

#ifdef RTOS_DEFAULT_WAIT_TIME
  /* 1 sec might not be optimized */
  timer_nowait.tv_sec = 1;
  timer_nowait.tv_usec = 0;
#else
  timer_nowait.tv_sec = 0;
  timer_nowait.tv_usec = 0;
#endif /* RTOS_DEFAULT_WAIT_TIME */

  while (1)
    {
      /* Pending read is exception. */
      if ((thread = rg_thread_trim_head (&m->read_pend)) != NULL)
        return rg_thread_run (m, thread, fetch);

      /* Check ready queue.  */
      if ((thread = rg_thread_trim_head (&m->queue_high)) != NULL)
        return rg_thread_run (m, thread, fetch);

      if ((thread = rg_thread_trim_head (&m->queue_middle)) != NULL)
        return rg_thread_run (m, thread, fetch);

      if ((thread = rg_thread_trim_head (&m->queue_low)) != NULL)
        return rg_thread_run (m, thread, fetch);

      /* Check all of available events.  */

      /* Check events.  */
      while ((thread = rg_thread_trim_head (&m->event)) != NULL)
        rg_thread_enqueue_high (m, thread);

      /* Check timer.  */
      rg_time_tzcurrent (&timer_now, NULL);

      for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
        for (thread = m->timer[i].head; thread; thread = next)
          {
            next = thread->next;
            if (timeval_cmp (timer_now, thread->u.sands) >= 0)
              {
                rg_thread_list_delete (&m->timer[i], thread);
                rg_thread_enqueue_middle (m, thread);
              }
#ifndef TIMER_NO_SORT
            else
              break;
#endif /* TIMER_NO_SORT */
          }

      /* Low priority events. */
      if ((thread = rg_thread_trim_head (&m->event_low)) != NULL)
        rg_thread_enqueue_low (m, thread);

      /* Structure copy.  */
      readfd = m->readfd;
      writefd = m->writefd;
      exceptfd = m->exceptfd;

      /* Check any thing to be execute.  */
      if (m->queue_high.head || m->queue_middle.head || m->queue_low.head)
        timer_wait = &timer_nowait;
      else
        timer_wait = rg_thread_timer_wait (m, &timer_val);

      /* First check for sockets.  Return immediately.  */
      if (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL))
        {
          /* poll机制使用的超时时间为毫秒 */
          if (timer_wait != NULL)
            timeout_poll = timer_wait->tv_sec * 1000 + timer_wait->tv_usec / 1000;
          else
            timeout_poll = -1;

          num = poll (m->pfds, m->nfds + 1, timeout_poll);
        }
      else
        {
          num = select (m->max_fd + 1, &readfd, &writefd, &exceptfd,
                                 timer_wait);
        }

      /* Error handling.  */
      if (num < 0)
        {
          /* 中断信号打断，只需要继续就行 */
          if (errno == EINTR)
            {
              if (RG_THREAD_DEBUG_PROCEDURE_ON (m))
                rg_thread_disp_invalid_fd (m);
              continue;
            }

          /* 没内存，继续尝试即可 */
          if (errno == ENOMEM)
            {
              RG_THREAD_PROCEDURE_DBG (m, "No memory to select, just try again.\n");
              sleep (2 * HZ);
              continue;
            }

          /* 出现了某种错误，将调试信息打印到内存文件中/tmp/rg_thread/pid */
          rg_thread_debug_msg ("Select error: %d pid:%lu\n", errno, getpid());
          rg_thread_disp_invalid_fd (m);
          return NULL;
        }

      /* File descriptor is readable/writable.  */
      if (num > 0)
        {
          /* High priority read thead. */
          if (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL))
            rg_thread_process_poll_fd (m, &m->read_high, num, false);
          else
            rg_thread_process_fd (m, &m->read_high, &readfd, &m->readfd);

          /* Normal priority read thead. */
          if (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL))
            rg_thread_process_poll_fd (m, &m->read, num, false);
          else
            rg_thread_process_fd (m, &m->read, &readfd, &m->readfd);

          /* Write thead. */
          if (RG_THREAD_CHECK_FLAG(m->flag, RG_THREAD_USE_POLL))
            rg_thread_process_poll_fd (m, &m->write, num, true);
          else
            rg_thread_process_fd (m, &m->write, &writefd, &m->writefd);
        }
    }

  return NULL;  /* Never reach here */
}

#define HAVE_THREAD_DEBUG 1
/* Call the thread.  */
void
rg_thread_call (struct rg_thread *thread)
{
#ifdef HAVE_THREAD_DEBUG
  struct timeval st, et, rt;

  rg_time_tzcurrent (&st, NULL);
#endif

  (*thread->func) (thread);

#ifdef HAVE_THREAD_DEBUG
    rg_time_tzcurrent (&et, NULL);
    rt = TV_SUB (et, st);
    if (rt.tv_sec >= 9)
      {
        rg_thread_debug_msg ("%%This thread %s[%p] exec time[%d.%06d sec].\n", thread->name, thread->func, rt.tv_sec, rt.tv_usec);
      }
#endif
}

/* Fake execution of the thread with given arguemment.  */
struct rg_thread *
rg_thread_execute (int (*func)(struct rg_thread *),
                void *arg,
                int val)
{
  struct rg_thread dummy;

  memset (&dummy, 0, sizeof (struct rg_thread));

  dummy.type = RG_THREAD_EVENT;
  dummy.master = NULL;
  dummy.func = func;
  dummy.arg = arg;
  dummy.u.val = val;
  rg_thread_call (&dummy);

  return NULL;
}

/* Real time OS support routine.  */
#ifdef HAVE_RTOS_TIC
#ifdef RTOS_EXECUTE_ONE_THREAD
int
rg_lib_tic (struct rg_thread_master *master, struct rg_thread *thread)
{
  if (rg_thread_fetch (master, thread))
    {
      rg_thread_call(thread);
      /* To indicate that a thread has run.  */
      return(1);
    }
  /* To indicate that no thread has run yet.  */
  return (0);
}
#else /* ! RTOS_EXECUTE_ONE_THREAD */
int
rg_rg_lib_tic (struct rg_thread_master *master, struct rg_thread *thread)
{
  while (rg_thread_fetch (master, thread))
    rg_thread_call(thread);
  return(0);
}
#endif /* ! RTOS_EXECUTE_ONE_THREAD */
#endif /* HAVE_RTOS_TIC */

/* Display thread master */
void
rg_thread_disp_thread_master (struct rg_thread_master *m)
{
  int i;

  RG_THREAD_PRINT ("Rg-thread Master Display:\n");
  RG_THREAD_PRINT (" Queue-high: head %p tail %p count %u\n",
                   m->queue_high.head, m->queue_high.tail, m->queue_high.count);
  RG_THREAD_PRINT (" Queue-middle: head %p tail %p count %u\n",
                   m->queue_middle.head, m->queue_middle.tail, m->queue_middle.count);
  RG_THREAD_PRINT (" Queue-low: head %p tail %p count %u\n",
                   m->queue_low.head, m->queue_low.tail, m->queue_low.count);
  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    RG_THREAD_PRINT (" Queue-timer[%d]: head %p tail %p count %u\n",
                     i, m->timer[i].head, m->timer[i].tail, m->timer[i].count);
  RG_THREAD_PRINT (" Queue-read-pend: head %p tail %p count %u\n",
                   m->read_pend.head, m->read_pend.tail, m->read_pend.count);
  RG_THREAD_PRINT (" Queue-read-high: head %p tail %p count %u\n",
                   m->read_high.head, m->read_high.tail, m->read_high.count);
  RG_THREAD_PRINT (" Queue-read: head %p tail %p count %u\n",
                   m->read.head, m->read.tail, m->read.count);
  RG_THREAD_PRINT (" Queue-write: head %p tail %p count %u\n",
                   m->write.head, m->write.tail, m->write.count);
  RG_THREAD_PRINT (" Queue-event: head %p tail %p count %u\n",
                   m->event.head, m->event.tail, m->event.count);
  RG_THREAD_PRINT (" Queue-event-low: head %p tail %p count %u\n",
                   m->event_low.head, m->event_low.tail, m->event_low.count);

  RG_THREAD_PRINT (" Max_fd: %d\n", m->max_fd);

  RG_THREAD_PRINT (" Poll nfds:%d\n", m->nfds);
  RG_THREAD_PRINT (" Poll pfds_size:%d\n", m->pfds_size);

  RG_THREAD_PRINT (" Unuse rg-thread count:%u\n", m->unuse.count);
  RG_THREAD_PRINT (" Total rg-thread count:%u\n", m->alloc);

  return;
}

/* Display thread queue */
static void
rg_thread_disp_thread_queue_detail (struct rg_thread_list *list)
{
  struct rg_thread *thread;
  int i;

  RG_THREAD_PRINT (" count:%u\n", list->count);
  if (list->count)
    {
      RG_THREAD_PRINT (" ");
    }
  else
    {
      return;
    }

  thread = list->head;
  i = 0;
  while (thread)
    {
      RG_THREAD_PRINT ("Thread:%p", thread);
      if (strlen (thread->name))
        {
          RG_THREAD_PRINT ("name: [%s]", thread->name);
        }
      else
        {
          RG_THREAD_PRINT ("name: [NULL]");
        }

      thread = thread->next;
      if (thread)
        {
          RG_THREAD_PRINT ("-->");

          i++;
          if (i % 5 == 0)
            {
              RG_THREAD_PRINT ("\n");
              i = 0;
            }
        }
      else
        {
          RG_THREAD_PRINT ("\n");
          return;
        }
    }

  return;
}

void
rg_thread_disp_thread_queue (struct rg_thread_master *m, int queue_type)
{
  assert (m);

  if (queue_type < RG_THREAD_READ || queue_type > RG_THREAD_EVENT_LOW)
    return;

  switch (queue_type)
    {
    case RG_THREAD_READ:
      RG_THREAD_PRINT ("Rg-thread display read-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->read);
      break;
    case RG_THREAD_WRITE:
      RG_THREAD_PRINT ("Rg-thread display write-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->write);
      break;
    case RG_THREAD_TIMER:
      {
        int i;
        for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
          {
            RG_THREAD_PRINT ("Rg-thread display time-%d-queue:\n", i);
            rg_thread_disp_thread_queue_detail (&m->timer[i]);
          }
        break;
      }
    case RG_THREAD_EVENT:
      RG_THREAD_PRINT ("Rg-thread display event-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->event);
      break;
    case RG_THREAD_QUEUE:
      RG_THREAD_PRINT ("Rg-thread display high-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->queue_high);
      RG_THREAD_PRINT ("Rg-thread display middle-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->queue_middle);
      RG_THREAD_PRINT ("Rg-thread display low-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->queue_low);
      break;
    case RG_THREAD_UNUSED:
      RG_THREAD_PRINT ("Rg-thread display unuse-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->unuse);
      break;
    case RG_THREAD_READ_HIGH:
      RG_THREAD_PRINT ("Rg-thread display read-high-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->read_high);
      break;
    case RG_THREAD_READ_PEND:
      RG_THREAD_PRINT ("Rg-thread display read-pend-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->read_pend);
      break;
    case RG_THREAD_EVENT_LOW:
      RG_THREAD_PRINT ("Rg-thread display event-low-queue:\n");
      rg_thread_disp_thread_queue_detail (&m->event_low);
      break;
    default:
      break;
    }

  return;
}

void
rg_thread_disp_thread (struct rg_thread *thread)
{
  if (thread == NULL)
    return;

  RG_THREAD_PRINT ("Rg-thread display thread:\n");
  RG_THREAD_PRINT (" prev:%p\n", thread->prev);
  RG_THREAD_PRINT (" next:%p\n", thread->next);
  RG_THREAD_PRINT (" master:%p\n", thread->master);
  RG_THREAD_PRINT (" func:%p\n", thread->func);
  RG_THREAD_PRINT (" zg:%p\n", thread->zg);
  RG_THREAD_PRINT (" arg:%p\n", thread->arg);
  RG_THREAD_PRINT (" type:%s(%d,%d)\n", rg_thread_get_type_str (thread->type, thread->priority),
                   thread->type, thread->priority);
  RG_THREAD_PRINT (" index:%d\n", thread->index);
  RG_THREAD_PRINT (" val/fd/timer:%d\n", thread->u.val);
  RG_THREAD_PRINT (" name:%s\n", thread->name);
  RG_THREAD_PRINT (" creater:%s\n",
          RG_THREAD_CHECK_FLAG(thread->flag, RG_THREAD_FLAG_CRT_BY_USR) ? "USER" : "RG-THREAD");

  return;
}

static void
rg_thread_disp_thread_byname_check_list (struct rg_thread_list *list, char *name)
{
  struct rg_thread *thread;

  thread = list->head;
  while (thread)
    {
      if (strcmp (thread->name, name) == 0)
        rg_thread_disp_thread (thread);
      thread = thread->next;
    }

  return;
}

void
rg_thread_disp_thread_byname (struct rg_thread_master *m, char *name)
{
  int i;

  if (name == NULL)
    return;

  rg_thread_disp_thread_byname_check_list (&m->queue_high, name);
  rg_thread_disp_thread_byname_check_list (&m->queue_middle, name);
  rg_thread_disp_thread_byname_check_list (&m->queue_low, name);

  for (i = 0; i < RG_THREAD_TIMER_SLOT; i++)
    {
      rg_thread_disp_thread_byname_check_list (&m->timer[i], name);
    }

  rg_thread_disp_thread_byname_check_list (&m->read_pend, name);
  rg_thread_disp_thread_byname_check_list (&m->read_high, name);
  rg_thread_disp_thread_byname_check_list (&m->read, name);
  rg_thread_disp_thread_byname_check_list (&m->write, name);
  rg_thread_disp_thread_byname_check_list (&m->event, name);
  rg_thread_disp_thread_byname_check_list (&m->event_low, name);
  rg_thread_disp_thread_byname_check_list (&m->unuse, name);

  return;
}

void
rg_thread_debug_interface (struct rg_thread_master *m, int debug_switch)
{
  if (m == NULL)
    return;

  if (debug_switch == RG_THREAD_DEBUG_OFF)
    {
      RG_THREAD_UNSET_FLAG (m->debug, RG_THREAD_DEBUG_INTERFACE_FLAG);
    }
  else if (debug_switch == RG_THREAD_DEBUG_ON)
    {
      RG_THREAD_SET_FLAG (m->debug, RG_THREAD_DEBUG_INTERFACE_FLAG);
    }
  else
    {
      RG_THREAD_PRINT ("Init rg_thread debug interface param[%d] failed:\n", debug_switch);
    }

  return;
}

void
rg_thread_debug_procedure (struct rg_thread_master *m, int debug_switch)
{
  if (m == NULL)
    return;

  if (debug_switch == RG_THREAD_DEBUG_OFF)
    {
      RG_THREAD_UNSET_FLAG (m->debug, RG_THREAD_DEBUG_PROCEDURE_FLAG);
    }
  else if (debug_switch == RG_THREAD_DEBUG_ON)
    {
      RG_THREAD_SET_FLAG (m->debug, RG_THREAD_DEBUG_PROCEDURE_FLAG);
    }
  else
    {
      RG_THREAD_PRINT ("Init rg_thread debug procedure param[%d] failed:\n", debug_switch);
    }

  return;
}

struct rg_thread *
rg_thread_add_recover_timer (struct rg_thread_master *m,
                             int (*func) (struct rg_thread *),
                             void *arg, long timer)
{
  struct rg_thread *thread;

  if (m == NULL)
    return NULL;

  if (m->t_recover)
    {
      /* 恢复线程处于未使用状态，重新入等待队列 */
      if (m->t_recover->type == RG_THREAD_UNUSED)
        {
          struct timeval timer_now;

          thread = m->t_recover;

          rg_time_tzcurrent (&timer_now, NULL);
#ifdef HAVE_NGSA
          if (timer < 0)
            {
              timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
            }
          else
            {
              timer_now.tv_sec += timer;
              if (timer_now.tv_sec < 0)
                timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
            }
#else
          timer_now.tv_sec += timer;
#endif
          /* 更新定时器参数 */
          thread->u.sands = timer_now;

          /* 添加定时器线程 */
          rg_thread_add_thread_check(m, thread);
          rg_thread_add_timer_common (m, thread);

          thread->type = RG_THREAD_TIMER;
          thread->func = func;
          thread->arg = arg;

          return thread;
        }
      else
        {
          /* 伪线程处于已使用状态，添加失败(只允许添加一个恢复伪线程) */
          RG_THREAD_INTERFACE_DBG (m, "Could not modify recover timer while it's running.\n");
          return NULL;
        }
    }
  else
    {
      thread = rg_thread_add_timer (m, func, arg, timer);
      if (thread == NULL)
        {
          RG_THREAD_INTERFACE_DBG (m, "Failed to add recover timer.\n");
        }
      else
        {
          rg_thread_add_thread_check(m, thread);
          m->t_recover = thread;
        }

      return thread;
    }
}

struct rg_thread *
rg_thread_add_recover_timer_withname (struct rg_thread_master *m,
                                      int (*func) (struct rg_thread *),
                                      void *arg, long timer, char *name)
{
  struct rg_thread *thread;

  if (name == NULL)
    return rg_thread_add_recover_timer (m, func, arg, timer);

  if (rg_thread_name_check (m, name))
    return NULL;

  thread = rg_thread_add_recover_timer (m, func, arg, timer);
  if (thread)
    rg_thread_name_add (thread, name);

  return thread;
}

void
rg_thread_cancel_recover_timer (struct rg_thread_master *m)
{
#ifdef LIB_PARAM_CHECK
  assert (m != NULL);
#endif

  if (m->t_recover == NULL)
    return;

  rg_thread_cancel (m->t_recover);
  m->t_recover = NULL;

  return;
}

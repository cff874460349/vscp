/* Copyright (C) 2001-2003 IP Infusion, Inc. All Rights Reserved. */

#ifndef _RG_THREAD_H
#define _RG_THREAD_H

//#include <mng/syslog/rg_syslog.h>

/**
 * @defgroup    SERVICE_RG_THREAD     Native API
 *
 * α�߳�API�ӿڡ�ͷ�ļ�<libpub/rg_thread/rg_thread.h>
 *
 * <p>
 * α�̲߳���ʵ�������ϵĲ���ϵͳ�̣߳����������ڲ���ϵͳ�ĵ��ȶ����С�
 * ����ʹ��α�̻߳��Ƶ�ģ����˵��һ������¸�ģ��ᴴ��һ����ʵ���̣߳�
 * Ȼ��������߳��н���α�̵߳ĵ��ȡ�<br>
 * ����α�̻߳����е��̲߳������������ϵĲ���ϵͳ�̣߳����ĳ��α�̹߳�ס��ʹ��α�̵߳�����
 * �޷��ټ������У�Ҳ�ͻ������Ӧ�Ĳ���ϵͳ���̻��̹߳�ס����ˣ����Ҫ�����ϵͳ���̻��̲߳���ס��
 * ��ôα�̵߳Ĵ����в����й������������ƿ��ǵ�ʱ��ͨ��α�̵߳��Ȼ���һ������¿�������
 * α�̵߳Ĵ����������磬����socket��д����������ʹ��ԭ�ȵ�ֱ�ӳ���ȥ������д�ķ�ʽ��
 * �������һ������дα�̣߳�α�̵߳��Ȼ������ж�socket�ɶ����߿�д��ʱ��Ž��ж���д������
 * <p>
 * ����ʵ�ʲ���ϵͳ�߳�һ����α�߳�Ҳͬ���������ȼ���һ���ԣ��߳����ȼ���Ҫ�ǿ�����ʹ�����и���
 * ��ѡ�������Ÿ����¼��ĵ���˳��α�̻߳��ƾ����������ȼ�:
 * ������ȼ��������ȼ�����ͨ���ȼ��������ȼ���
 * <p>
 * α�̻߳�����һ������ѭ���Ļ��ƣ�Ҳ����˵ʹ��α�̵߳�ģ�齫һֱѭ�����ҿ������е�α�̣߳�
 * �ҵ������е��̺߳������ִ�С���ˣ���ϵͳ��ʼ����ʱ��������Ҫ���һ��α�̣߳��������
 * ��������ѭ��������û�п����е��̣߳���һֱ�޷�����µ�α�߳̽�ȥ��<br>
 * <b>ע��:α�̻߳���ֻ����һ������ϵͳ�̶߳�����в��������ܹ����������ϵĲ���ϵͳ�߳�ͬʱ��һ��
 * α�̹߳������е�α�߳̽�����ӡ�ɾ���ȡ�</b>
 * <p>
 * �����Ǹ���򵥵�ʹ��α�̵߳����ӣ�
 *
 * @code
 * #include <libpub/rg_thread/rg_thread.h>
 *
 * #define TEST_TIME 5
 *
 * struct rg_thread_master *g_master;
 * struct rg_thread g_timer;
 *
 * int test_timer(struct rg_thread *t)
 * {
 *     printf("test timer exipre\n");
 *
 *     (void)rg_thread_insert_timer(g_master, &g_timer, test_timer, NULL, TEST_TIME, "g-timer");
 *
 *     return 0;
 * }
 *
 * int main(void)
 * {
 *     struct rg_thread thread;
 *
 *     g_master = rg_thread_master_create();
 *     if (g_master == NULL) {
 *         return -1;
 *     }
 *
 *     (void)rg_thread_insert_timer(g_master, &g_timer, test_timer, NULL, TEST_TIME, "g-timer");
 *
 *     while (rg_thread_fetch (g_master, &thread)) {
 *         rg_thread_call (&thread);
 *     }
 *
 *     return 0;
 * }
 * @endcode
 *
 * @ingroup     SERVICE_RG_THREAD
 *
 * @{
 */


/* �����ȡע�͵Ķ��� */
#define HAVE_NGSA

/* Flag manipulation macros. */
#define RG_THREAD_CHECK_FLAG(V,F)       ((V) & (F))
#define RG_THREAD_SET_FLAG(V,F)         (V) = (V) | (F)
#define RG_THREAD_UNSET_FLAG(V,F)       (V) = (V) & ~(F)

#define RG_THREAD_DEBUG_INTERFACE_ON(m) RG_THREAD_CHECK_FLAG(m->debug, RG_THREAD_DEBUG_INTERFACE_FLAG)
#define RG_THREAD_DEBUG_PROCEDURE_ON(m) RG_THREAD_CHECK_FLAG(m->debug, RG_THREAD_DEBUG_PROCEDURE_FLAG)

#if 0
#define RG_THREAD_PRINT printk
#else
#define RG_THREAD_PRINT (void)0;(void)
#endif

/* Debug interface */
#define RG_THREAD_INTERFACE_DBG(m, fmt, ...)                                  \
  do {                                                                        \
    if (RG_THREAD_DEBUG_INTERFACE_ON (m))                                     \
      {                                                                       \
        RG_THREAD_PRINT ("[RG-THREAD]" fmt, ## __VA_ARGS__);                  \
      }                                                                       \
  } while (0)

/* Debug procedure */
#define RG_THREAD_PROCEDURE_DBG(m, fmt, ...)                                  \
  do {                                                                        \
    if (RG_THREAD_DEBUG_PROCEDURE_ON (m))                                     \
      {                                                                       \
        RG_THREAD_PRINT ("[RG-THREAD]" fmt, ## __VA_ARGS__);                  \
      }                                                                       \
  } while (0)

#define RG_THREAD_ASSERT(cond)                                                \
  do {                                                                        \
    if (!(cond))                                                              \
      {                                                                       \
        RG_THREAD_PRINT ("%s:%d, ASSERT fail!\n", __FILE__, __LINE__);        \
      }                                                                       \
  } while(0)

/* Thread types.  */
#define RG_THREAD_READ              0
#define RG_THREAD_WRITE             1
#define RG_THREAD_TIMER             2
#define RG_THREAD_EVENT             3
#define RG_THREAD_QUEUE             4
#define RG_THREAD_UNUSED            5
#define RG_THREAD_READ_HIGH         6
#define RG_THREAD_READ_PEND         7
#define RG_THREAD_EVENT_LOW         8

#define RG_THREAD_UNUSE_MAX         1024
#define RG_THREAD_FREE_UNUSE_MAX    (RG_THREAD_UNUSE_MAX >> 1)


typedef unsigned int u_int32_t;
typedef u_int32_t uint32_t;

#define true 1
#define false 0

/* Linked list of thread. */
struct rg_thread_list
{
  struct rg_thread *head;
  struct rg_thread *tail;
  u_int32_t count;
};

#define RG_THREAD_DEBUG_OFF         0
#define RG_THREAD_DEBUG_ON          1
 /* ����Ϊ�����ȡע�͵Ķ��� */

/**
 * α�̹߳�����.
 * ��ʹ��α�̻߳���ʱ�����봴��һ��α�̹߳����������ڹ���α�̡߳�
 */
struct rg_thread_master
{
  /* Priority based queue.  */
  struct rg_thread_list queue_high;
  struct rg_thread_list queue_middle;
  struct rg_thread_list queue_low;

  /* Timer */
#define RG_THREAD_TIMER_SLOT           4
  int index;
  struct rg_thread_list timer[RG_THREAD_TIMER_SLOT];

  /* Thread to be executed.  */
  struct rg_thread_list read_pend;
  struct rg_thread_list read_high;
  struct rg_thread_list read;
  struct rg_thread_list write;
  struct rg_thread_list event;
  struct rg_thread_list event_low;
  struct rg_thread_list unuse;
  fd_set readfd;
  fd_set writefd;
  fd_set exceptfd;
  int max_fd;

  /* ʹ��poll���� */
  struct pollfd *pfds;      /**< pollfd���� */
  short nfds;               /**< pollfd��Ч�����С��α�߳̿��ڲ�ʹ�ã��û���Ҫ�Լ����� */
  short pfds_size;          /**< pollfd�����С��α�߳̿��ڲ�ʹ�ã��û���Ҫ�Լ����� */

  u_int32_t alloc;
  int debug;
#define RG_THREAD_DEBUG_INTERFACE_FLAG     (1 << 0)
#define RG_THREAD_DEBUG_PROCEDURE_FLAG     (1 << 1)

  u_int32_t flag;
#define RG_THREAD_IS_FIRST_FETCH (1 << 0)
#define RG_THREAD_USE_POLL (1 << 1)

  int tid;

  struct rg_thread *t_recover;
};

#define RG_THREAD_NAME_LEN          10  /* α�߳�����󳤶� */

/**
 * α�̡߳�
 */
struct rg_thread
{
  struct rg_thread *next;
  struct rg_thread *prev;

  struct rg_thread_master *master;

  int (*func) (struct rg_thread *);   /**< α�߳�ִ�к��� */

  void *zg;                           /**< zebosģ��ʹ�õ�lib_globals */

  void *arg;                          /**< �¼�������һ�㴫�ݽṹ��ָ�� */

  char type;                          /**< α�߳����ͣ�α�߳̿�ʹ�ã��û���Ҫ�Լ����� */

  char priority;                      /**< α�߳����ȼ������Լ����ã�ͨ�����벻ͬ����α�߳�ʱ��α�߳̿��Զ����� */
#define RG_THREAD_PRIORITY_HIGH         0
#define RG_THREAD_PRIORITY_MIDDLE       1
#define RG_THREAD_PRIORITY_LOW          2

  char index;                         /**< α�̶߳�ʱ������������α�߳̿�ʹ�ã��û���Ҫ�Լ����� */

  char flag;                          /**< α�̱߳�־��α�߳̿�ʹ�ã��û���Ҫ�Լ����� */
#define RG_THREAD_FLAG_CRT_BY_USR       (1 << 0)
#define RG_THREAD_FLAG_IN_LIST          (1 << 1)

  union
  {
    int val;                          /**< �¼����������Դ�������ֵ */

    int fd;                           /**< �¼����������Դ��ݾ�� */

    struct timeval sands;             /**< �¼����������Դ��ݶ�ʱ����ʱʱ�� */
  } u;

  short pfd_index;                    /**< poll����������α�߳̿��ڲ�ʹ�ã��û���Ҫ�Լ����� */

  char name[RG_THREAD_NAME_LEN + 1];  /**< α�߳��� */
};

/* Macros.  */
#define RG_THREAD_ARG(X)           ((X)->arg)       /**< ȡα�̵߳�arg���� */
#define RG_THREAD_FD(X)            ((X)->u.fd)      /**< ȡα�̵߳ľ������ */
#define RG_THREAD_VAL(X)           ((X)->u.val)     /**< ȡα�̵߳����β��� */
#define RG_THREAD_TIME_VAL(X)      ((X)->u.sands)   /**< ȡα�̵߳ĳ�ʱʱ����� */
#define RG_THREAD_GLOB(X)          ((X)->zg)        /**< ȡα�̵߳�lib_global�ֶΣ�zebosģ��ʹ�� */

/*
 * *< �����жϸ�α�߳��Ƿ�����ӵ�α�̹߳������еȴ�����
 * ע�⣺������û��Լ�������thread����Ҫ��֤��ʼ����ʱ���thread�ṹ����0�������������Լ��޸ĸñ�־��
 * ����ж�ֻ������ʹ��insert��ʽ��ӵ�α�̣߳���������ʹ��add��ʽ��ӵ�α�߳�
 */
#define RG_THREAD_IS_IN_LIST(THREAD) RG_THREAD_CHECK_FLAG((THREAD)->flag, RG_THREAD_FLAG_IN_LIST)

#define RG_THREAD_READ_ON(master,thread,func,arg,sock) \
  do { \
    if (! thread) \
      thread = rg_thread_add_read (master, func, arg, sock); \
  } while (0)

#define RG_THREAD_WRITE_ON(master,thread,func,arg,sock) \
  do { \
    if (! thread) \
      thread = rg_thread_add_write (master, func, arg, sock); \
  } while (0)

#define RG_THREAD_TIMER_ON(master,thread,func,arg,time) \
  do { \
    if (! thread) \
      thread = rg_thread_add_timer (master, func, arg, time); \
  } while (0)

#define RG_THREAD_OFF(thread) \
  do { \
    if (thread) \
      { \
        rg_thread_cancel (thread); \
        thread = NULL; \
      } \
  } while (0)

#define RG_THREAD_READ_OFF(thread)   RG_THREAD_OFF(thread)  /**< ɾ��һ��socket�Ķ��߳� */
#define RG_THREAD_WRITE_OFF(thread)  RG_THREAD_OFF(thread)  /**< ɾ��һ��socket��д�߳� */
#define RG_THREAD_TIMER_OFF(thread)  RG_THREAD_OFF(thread)  /**< ɾ��һ����ʱ�� */

/* Prototypes.  */

/**
 * @rg_thread_master_create ����α�̹߳�����
 *
 * @return �ɹ����ش�����α�̹߳����������򷵻ؿ�
 */
struct rg_thread_master *rg_thread_master_create (void);

/**
 * @rg_thread_master_finish ����α�̹߳�����
 *
 * @param m α�̹߳�����
 * @return ��
 */
void rg_thread_master_finish (struct rg_thread_master *m);

/**
 * @rg_thread_use_poll ���ø�α�̹߳�����ʹ��poll����
 *
 * ������̵�fd������ᳬ��1024����ôĬ��α�̻߳�����ʹ�õ�select���޷�����������
 * ��������£����̱�������α�߳�ʹ��poll���ơ�<br>
 * ע: ֻ������һ���Ƿ�ʹ��poll���ƣ����������й��������ر任�Ƿ�ʹ��poll���ơ�
 *
 * @param m     α�̹߳�����
 * @param max_fds  ��α�̹߳��������漰����fd������Ŀ������ָ��pollfd����Ĵ�С
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_use_poll (struct rg_thread_master *m, short max_fds);

/**
 * @rg_thread_add_read ���һ��socket���߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���<br>
 * socket�ɶ�ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к������������������������socket��
 * ��ô��Ҫ���µ��øú������һ��socket���̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к������ú����е�rg_thread�������Ǳ�����ӵ�α�߳�
 * @param arg   α�߳�arg��������ֵ������ִ�к���funcʱ�Ӳ���rg_thread����ȡ��
 * @param fd    α�߳�socket�����������ֵ������ִ�к���funcʱ�Ӳ���rg_thread����ȡ��
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_read (struct rg_thread_master *m,
                                int (*func)(struct rg_thread *), void *arg,
                                int fd);

/**
 * @rg_thread_add_read_withname ���һ�������ֵ�socket���߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 * socket�ɶ�ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к������������������������socket��
 * ��ô��Ҫ���µ��øú������һ��socket���̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_read_withname (struct rg_thread_master *m,
                                int (*func)(struct rg_thread *), void *arg,
                                int fd,
                                char *name);

/**
 * @rg_thread_add_read_high ���һ��socket�����ȼ����߳�
 *
 * ��α�߳����ȼ�������ͨsocket���̣߳�������Ȼ������ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_read_high (struct rg_thread_master *m,
                                     int (*func)(struct rg_thread *), void *arg,
                                     int fd);

/**
 * @rg_thread_add_read_high_withname ���һ�������ֵ�socket�����ȼ����߳�
 *
 * ��α�߳����ȼ�������ͨsocket���̣߳�������Ȼ������ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_read_high_withname (struct rg_thread_master *m,
                                     int (*func)(struct rg_thread *), void *arg,
                                     int fd,
                                     char *name);

/**
 * @rg_thread_insert_read ����һ��socket���߳�
 *
 * �ýӿ���rg_thread_add_read���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ����ú����е�rg_thread�������Ǳ�����ӵ�α�߳�
 * @param arg    α�߳�arg��������ֵ������ִ�к���funcʱ�Ӳ���rg_thread����ȡ��
 * @param fd     α�߳�socket��������������ǺϷ����������ֵ������ִ�к���funcʱ�Ӳ���rg_thread����ȡ��
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_read (struct rg_thread_master *m, struct rg_thread *thread,
                             int (*func) (struct rg_thread *),
                             void *arg, int fd, char *name);

/**
 * @rg_thread_insert_read_high ����һ��socket�����ȼ����߳�
 *
 * �ýӿ���rg_thread_add_read_high���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param fd     α�߳�socket��������������ǺϷ������
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_read_high (struct rg_thread_master *m, struct rg_thread *thread,
                             int (*func) (struct rg_thread *),
                             void *arg, int fd, char *name);

/**
 * @rg_thread_add_write ���һ��socketд�߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_write (struct rg_thread_master *m,
                                 int (*func)(struct rg_thread *), void *arg,
                                 int fd);

/**
 * @rg_thread_add_write_withname ���һ�������ֵ�socketд�߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_write_withname (struct rg_thread_master *m,
                                 int (*func)(struct rg_thread *), void *arg,
                                 int fd,
                                 char *name);

/**
 * @rg_thread_insert_write ����һ��socketд�߳�
 *
 * �ýӿ���rg_thread_add_write���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param fd     α�߳�socket��������������ǺϷ������
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_write (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func)(struct rg_thread *), void *arg,
        int fd, char *name);

/**
 * @rg_thread_add_timer ���һ����ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���<br>
 * ��ʱ����ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к��������������������������ʱ����
 * ��ô��Ҫ���µ��øú������һ����ʱ��α�̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ�䣬��λΪ��
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_timer (struct rg_thread_master *m,
                                 int (*func)(struct rg_thread *), void *arg, long timer);

/**
 * @rg_thread_add_timer_withname ���һ�������ֵĶ�ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 * ��ʱ����ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к��������������������������ʱ����
 * ��ô��Ҫ���µ��øú������һ����ʱ��α�̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ�䣬��λΪ��
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_timer_withname (struct rg_thread_master *m,
                                 int (*func)(struct rg_thread *), void *arg, long timer, char *name);

/**
 * @rg_thread_add_timer_timeval ���һ����ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ��
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_timer_timeval (struct rg_thread_master *m,
                                         int (*func)(struct rg_thread *),
                                         void *arg, struct timeval timer);

/**
 * @rg_thread_add_timer_timeval_withname ���һ�������ֵĶ�ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ��
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_timer_timeval_withname (struct rg_thread_master *m,
                                         int (*func)(struct rg_thread *),
                                         void *arg, struct timeval timer,
                                         char *name);

/**
 * @rg_thread_insert_timer ����һ����ʱ��
 *
 * �ýӿ���rg_thread_add_timer���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param timer  ��ʱʱ��
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_timer (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func)(struct rg_thread *), void *arg, long timer, char *name);

/**
 * @rg_thread_insert_timer_timeval ����һ����ʱ��
 *
 * �ýӿ���rg_thread_add_timer_timeval���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param timer  ��ʱʱ��
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_timer_timeval (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func)(struct rg_thread *), void *arg, struct timeval timer, char *name);

/**
 * @rg_thread_add_event ���һ�������ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_event (struct rg_thread_master *m,
                                 int (*func)(struct rg_thread *), void *arg,
                                 int val);

/**
 * @rg_thread_add_event_withname ���һ�������ֵĸ����ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_event_withname (struct rg_thread_master *m,
                                 int (*func)(struct rg_thread *), void *arg,
                                 int val,
                                 char *name);

/**
 * @rg_thread_add_event_low ���һ�������ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_event_low (struct rg_thread_master *m,
                                     int (*func)(struct rg_thread *), void *arg,
                                     int val);

/**
 * @rg_thread_add_event_low_withname ���һ�������ֵĵ����ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_event_low_withname (struct rg_thread_master *m,
                                     int (*func)(struct rg_thread *), void *arg,
                                     int val,
                                     char *name);

/**
 * @rg_thread_insert_event ����һ���¼�
 *
 * �ýӿ���rg_thread_add_event���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param val    α�߳����β���
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_event (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func)(struct rg_thread *), void *arg, int val, char *name);

/**
 * @rg_thread_insert_event_low ����һ���¼�
 *
 * �ýӿ���rg_thread_add_event_low���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param val    α�߳����β���
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_event_low (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func)(struct rg_thread *), void *arg, int val, char *name);

/**
 * @rg_thread_add_read_pend ���һ����������Ϣα�߳�
 *
 * ��α�߳�Ϊ������ȼ���<br>
 * ֻ�����������:��ʱ����һ���¼���������Ҫѭ���ȴ�ĳ����Ϣ���أ�
 * ��ʱ���ȡ��һЩ�������ȴ�����Ϣ��
 * ��Щ��Ϣ����ֱ�ӱ���������ҪΪ���Ǵ���һ����������Ϣα�̣߳�
 * ��Щ��������Ϣα�̻߳�����һ���̵߳���������ִ�С�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_read_pend (struct rg_thread_master *,
                                     int (*func) (struct rg_thread *), void *arg,
                                     int val);

/**
 * @rg_thread_add_read_pend_withname ���һ�������ֵĴ�������Ϣα�߳�
 *
 * ��α�߳�Ϊ������ȼ���
 * ֻ�����������:��ʱ����һ���¼���������Ҫѭ���ȴ�ĳ����Ϣ���أ�
 * ��ʱ���ȡ��һЩ�������ȴ�����Ϣ��
 * ��Щ��Ϣ����ֱ�ӱ���������ҪΪ���Ǵ���һ����������Ϣα�̣߳�
 * ��Щ��������Ϣα�̻߳�����һ���̵߳���������ִ�С�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_add_read_pend_withname (struct rg_thread_master *,
                                     int (*func) (struct rg_thread *), void *arg,
                                     int val,
                                     char *name);

/**
 * @rg_thread_insert_read_pend ����һ����������Ϣα�߳�
 *
 * �ýӿ���rg_thread_add_read_pend���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param val    α�߳����β���
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int rg_thread_insert_read_pend (struct rg_thread_master *m, struct rg_thread *thread,
        int (*func)(struct rg_thread *), void *arg, int val, char *name);

/**
 * @rg_thread_cancel ȡ��һ��α�߳�
 *
 * ����ȡ���������͵�α�̡߳���Ҫȡ����α�̱߳�����֮ǰ�Ѿ���ӳɹ���α�̡߳�</br>
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param thread     Ҫȡ����α�߳�
 * @return     ��
 */
void rg_thread_cancel (struct rg_thread *thread);

/**
 * @rg_thread_cancel_event ȡ�����а���arg������α�߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void rg_thread_cancel_event (struct rg_thread_master *m, void *arg);

/**
 * @rg_thread_cancel_event_low ȡ�����а���arg�����ĵ����ȼ�α�߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void rg_thread_cancel_event_low (struct rg_thread_master *m, void *arg);

/**
 * @rg_thread_cancel_timer ȡ�����а���arg�����Ķ�ʱ��
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void rg_thread_cancel_timer (struct rg_thread_master *m, void *arg);

/**
 * @rg_thread_cancel_write ȡ�����а���arg������д�߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void rg_thread_cancel_write (struct rg_thread_master *m, void *arg);

/**
 * @rg_thread_cancel_read ȡ�����а���arg�����Ķ��߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void rg_thread_cancel_read (struct rg_thread_master *m, void *arg);

/**
 * @rg_thread_fetch ��ȡһ��������α�߳�
 *
 * @param m          α�̹߳�����
 * @param fetch      ������α�̱߳����ָ��
 * @return     �ɹ����ؿ�����α�̣߳����򷵻ؿ�
 */
struct rg_thread *rg_thread_fetch (struct rg_thread_master *m, struct rg_thread *fetch);

/**
 * @rg_thread_execute ����һ��α�߳�ִ�к���
 *
 * @param func       α�߳�ִ�к���
 * @param arg        α�̵߳�arg����
 * @param val        α�̵߳����β���
 * @return     ���ؿ�
 */
struct rg_thread *rg_thread_execute (int (*func)(struct rg_thread *), void *arg,
                                     int val);

/**
 * @rg_thread_call ����һ��α�߳�
 *
 * @param thread     α�߳�
 * @return     ��
 */
void rg_thread_call (struct rg_thread *thread);

/**
 * @rg_thread_timer_remain_second ��ȡ��ʱ��ʣ��ʱ��
 *
 * @param thread     ��ʱ��α�߳�
 * @return     �ɹ����ض�ʱ��ʣ��ʱ�䣬���򷵻�0
 */
u_int32_t rg_thread_timer_remain_second (struct rg_thread *thread);

#ifdef HAVE_NGSA
void rg_thread_list_add (struct rg_thread_list *, struct rg_thread *);
void rg_thread_list_execute (struct rg_thread_master *, struct rg_thread_list *);
void rg_thread_list_clear (struct rg_thread_master *, struct rg_thread_list *);

struct rg_thread *rg_thread_get (struct rg_thread_master *, char,
                                 int (*) (struct rg_thread *), void *);

struct rg_thread *rg_thread_lookup_list_by_arg_func (struct rg_thread_list *, char,
                                                     int (*)(struct rg_thread *), void *, int);
void rg_thread_cancel_event_by_arg_func (struct rg_thread_master *,
                                         int (*)(struct rg_thread *), void *);
void rg_thread_cancel_by_arg_func (struct rg_thread_master *,
                                int (*)(struct rg_thread *), void *);
void rg_thread_cancel_arg_check_list (struct rg_thread_master *, struct rg_thread_list *list, void *arg);
void rg_thread_cancel_arg_check (struct rg_thread_master *, void *arg);
void rg_thread_cancel_by_arg (struct rg_thread_master *, void *arg);
void rg_thread_cancel_invalid_fd(struct rg_thread_master *, struct rg_thread_list *list);
#endif

/**
 * @rg_thread_disp_thread_master ��ӡα�̹߳�������Ϣ
 *
 * @param m          α�̹߳�����
 * @return     ���ؿ�
 */
void
rg_thread_disp_thread_master (struct rg_thread_master *m);

/**
 * @rg_thread_disp_thread_queue ��ӡα�̶߳�����Ϣ
 *
 * @param m          α�̹߳�����
 * @param queue_type α�̶߳�������
 * @return     ���ؿ�
 */
void
rg_thread_disp_thread_queue (struct rg_thread_master *m, int queue_type);

/**
 * @rg_thread_disp_thread ��ӡα�߳���Ϣ
 *
 * @param thread          α�߳�
 * @return     ���ؿ�
 */
void
rg_thread_disp_thread (struct rg_thread *thread);

/**
 * @rg_thread_disp_thread_byname ��ӡָ�����ֵ�α�߳���Ϣ
 *
 * @param m          α�̹߳�����
 * @param name       α�߳���
 * @return     ���ؿ�
 */
void
rg_thread_disp_thread_byname (struct rg_thread_master *m, char *name);

/**
 * @rg_thread_debug_interface ��α�߳̽ӿڵ��Կ���
 *
 * @param m             α�̹߳�����
 * @param debug_switch  ����״̬(RG_THREAD_DEBUG_OFF/RG_THREAD_DEBUG_ON)
 * @return     ���ؿ�
 */
void
rg_thread_debug_interface (struct rg_thread_master *m, int debug_switch);

/**
 * @rg_thread_debug_procedure ��α�̹߳��̵��Կ���
 *
 * @param m             α�̹߳�����
 * @param debug_switch  ����״̬(RG_THREAD_DEBUG_OFF/RG_THREAD_DEBUG_ON)
 * @return     ���ؿ�
 */
void
rg_thread_debug_procedure (struct rg_thread_master *m, int debug_switch);

#endif /* _RG_THREAD_H */


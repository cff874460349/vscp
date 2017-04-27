/* http_include.h */

#ifndef _HTTP_UTIL_H_
#define _HTTP_UTIL_H_

#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>        /* timespec{} for pselect() */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>       /* for nonblocking */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>    /* for S_xxx file mode constants */
#include <sys/uio.h>     /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>      /* for Unix domain sockets */
#include <assert.h>

#include "http_util.h"
#include "http_option.h"

#define HTTP_DPRINT(fmt, ...)  do { \
    if (http_opt.debug) { \
        printf("[%s, Line: %d]: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
    } \
} while (0);


/* The number of elements in an array.  For example:
   static char a[] = "foo";     -- countof(a) == 4 (note terminating \0)
   int a[5] = {1, 2};           -- countof(a) == 5
   char *a[] = {                -- countof(a) == 3
     "foo", "bar", "baz"
   }; */
#define countof(array) (sizeof (array) / sizeof ((array)[0]))

#define HTTP_REALLOC(p, ptr, size) do { \
    (p) = realloc((ptr), (size)); \
    if ((p) != NULL) { \
        memset((p), 0, (size)); \
        ptr = p;\
    } \
} while (0)

#define HTTP_MALLOC(ptr, size) do { \
    (ptr) = malloc((size)); \
    if ((ptr) != NULL) { \
        memset((ptr), 0, (size)); \
    } \
} while (0)

/* 释放由malloc分配的缓冲区，并将缓冲区的指针置为NULL */
#define HTTP_FREE(ptr)  do { \
    if ((ptr) != NULL) { \
        free((ptr)); \
    } \
} while (0)


#define CLOSE_INVALIDATE(fd) do {                   \
    close (fd);                                     \
    fd = INVALID_SOCKET;                            \
} while (0)

typedef int socket_t;
#define INVALID_SOCKET ((socket_t)-1)

#define FD_READ_LINE_MAX 2048

/*
 * http响应报文头最大支持大小, 这个限制可以防止非法服务器端发送过大的数据，如果为0表示取消限制
 */
#define HTTP_RESPONSE_MAX_SIZE 1024


/* Some status code validation macros: */
#define H_20X(x)        (((x) >= 200) && ((x) < 300))
#define H_PARTIAL(x)    ((x) == HTTP_STATUS_PARTIAL_CONTENTS)
#define H_REDIRECTED(x) ((x) == HTTP_STATUS_MOVED_PERMANENTLY          \
                         || (x) == HTTP_STATUS_MOVED_TEMPORARILY       \
                         || (x) == HTTP_STATUS_SEE_OTHER               \
                         || (x) == HTTP_STATUS_TEMPORARY_REDIRECT)

/* Successful 2xx.  */
#define HTTP_STATUS_OK                    200
#define HTTP_STATUS_CREATED               201
#define HTTP_STATUS_ACCEPTED              202
#define HTTP_STATUS_NO_CONTENT            204
#define HTTP_STATUS_PARTIAL_CONTENTS      206

/* Redirection 3xx.  */
#define HTTP_STATUS_MULTIPLE_CHOICES      300
#define HTTP_STATUS_MOVED_PERMANENTLY     301
#define HTTP_STATUS_MOVED_TEMPORARILY     302
#define HTTP_STATUS_SEE_OTHER             303 /* from HTTP/1.1 */
#define HTTP_STATUS_NOT_MODIFIED          304
#define HTTP_STATUS_TEMPORARY_REDIRECT    307 /* from HTTP/1.1 */

/* Client error 4xx.  */
#define HTTP_STATUS_BAD_REQUEST           400
#define HTTP_STATUS_UNAUTHORIZED          401
#define HTTP_STATUS_FORBIDDEN             403
#define HTTP_STATUS_NOT_FOUND             404
#define HTTP_STATUS_RANGE_NOT_SATISFIABLE 416

/* Server errors 5xx.  */
#define HTTP_STATUS_INTERNAL              500
#define HTTP_STATUS_NOT_IMPLEMENTED       501
#define HTTP_STATUS_BAD_GATEWAY           502
#define HTTP_STATUS_UNAVAILABLE           503

#define HTTP_UTIL_BUFLEN    1024
#define HTTP_UTIL_IO        0x01

#define HTTP_PORT_STR_LEN 6
#define HTTP_PATH_SIZE 256
#define HTTP_HOST_SIZE 256
#define HTTP_DEFAULT_PORT 80
#define HTTP_FILE_NAME_SIZE 256

typedef enum {
    HTTP_STAT_BEGIN,               /*  */
    HTTP_STAT_LENGTH,
    HTTP_STAT_READ,
    HTTP_STAT_OTHER
} http_stat_action_t;

typedef struct http_response_s {
    int  status;                        /* GET:1; POST:2;  见http_method_e */
    int  version;                       /* 1:http1.1    0:http1.0 目前只支持该两种 */
    size_t content_len;                 /* 内容长度 */
    bool keep_alive;                     /* 1表示保存连接，0需要关闭连接 */
    bool accept_ranges;                 /* bytes: TRUE; none: FALSE */

    /* 断点续传处理字段 */
    size_t range_begin;                 /* 开始字节位置 */
    size_t range_end;                   /* 结束字节位置 */
    size_t file_len;                    /* 资源的总字节数 */

    char remote_time[32];               /* Last-Modified */
    time_t modify_tstamp;
} http_response_t;

/* 下载的文件信息结构定义 */
typedef struct http_stat_s {
  http_progress_t *progress;    /* 下载进度信息, 指向用户传来的数据 */
  /****************/
  int       restval;            /* the restart value */
  char      *local_file;        /* local file name. */
  time_t    orig_file_tstamp;   /* time-stamp of file to compare for time-stamping */
} http_stat_t;

typedef struct http_util_s {
  int               socket;
  char              buf[HTTP_UTIL_BUFLEN];
  int               bufidx;    /* index in buf[] */
  int               mode;
  double            timeout;
  int               error;
  char              tmpbuf[1024];
  http_response_t   http_rsp;
} http_util_t;

/* Structure containing info on a URL.  */
typedef struct http_url_s {
  char                  *url;                           /* Original URL */
  char                  host[HTTP_HOST_SIZE];           /* Extracted hostname */
  int                   port;                           /* Port number */
  char                  filename[HTTP_FILE_NAME_SIZE];  /* 文件名称 */
  char                  path[HTTP_PATH_SIZE];           /* 路径 */
  bool                  is_mngt;
  struct sockaddr_in    server_addr;                    /* 暂时只支持ipv4地址 */
} http_url_t;

int random_number(int max);

extern void http_free_util(http_util_t *util);

int http_post_header(http_util_t *util, const char *key, const char *val);

int http_flush(http_util_t *util);

char *http_sock_ntop(const struct sockaddr *sa, socklen_t salen);

/*
 * echo_create_unix_socket - 创建unix 字节流套接字
 * @path:文件名
 * @is_nblock: 是否阻塞
 * @is_mgnt: 是否为管理口
 *
 *
 */
int http_create_socket(const struct sockaddr *sa, int len, bool is_nblock, bool is_mgnt);

int http_fd_write(int fd, const void *buf, int bufsize, double timeout);

int http_fd_read(int fd, void *buf, int bufsize, double timeout);

char *http_read_http_response_head(int fd);

int http_parse_http_response_head(http_util_t *util, http_stat_t *hs, char *buf);

bool http_skip_short_body (int fd, size_t contlen);

http_util_t *http_create_util(void);

http_url_t *http_create_url(char *url, bool is_mngt, http_err_t *err);

#endif /* _HTTP_UTIL_H_ */

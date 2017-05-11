/*
 * Copyright (c) 2013-8-1 Ruijie Network. All rights reserved.
 */
/*
 * http_util.c
 * Original Author: yanggang@ruijie.com.cn, 2013-8-1
 *
 * History
 * --------------------
 *
 *
 */

#include <sys/param.h>
#include <locale.h>
#include <time.h>
#include <utime.h>
#include <ctype.h>

#define HTTP_NOMGMT_VRF
#ifndef HTTP_NOMGMT_VRF
#include <lib_net/net_socket.h>
#include <rg_net/networklayer/vrf_public.h>
#endif
#include <netdb.h>
#include <stdio.h>

#include "http_include.h"
#include "http_option.h"

http_stat_t http_stat;

/* "Touch" FILE, i.e. make its mtime ("modified time") equal the time
   specified with TM.  The atime ("access time") is set to the current
   time.  */

/*
 * 修订文件的 modified time
 */
void touch (const char *file, time_t tm)
{
  struct utimbuf times;

  times.modtime = tm;
  times.actime = time (NULL);
  if (utime (file, &times) == -1) {
      HTTP_DPRINT("utime(%s): %s\n", file, strerror (errno));
  }
}

/*
 * 根据fmt格式化时钟
 */
static char *fmttime(time_t t, const char *fmt)
{
  static char output[32];
  struct tm *tm = localtime(&t);

  if (!tm) {
      return NULL;
  }

  if (!strftime(output, sizeof(output), fmt, tm)) {
      return NULL;
  }

  return output;
}

/* 根据TM返回指向时间格式为(YYYY-MM-DD hh:mm:ss) 静态缓存  */
static char *datetime_str(time_t t)
{
  return fmttime(t, "%Y-%m-%d %H:%M:%S");
}

static bool check_end (const char *p)
{
    if (!p) {
        return FALSE;
    }

    while (isspace((unsigned char)*p)) {
        ++p;
    }

    if (!*p || (p[0] == 'G' && p[1] == 'M' && p[2] == 'T')
        || ((p[0] == '+' || p[0] == '-') && isdigit((int)p[1]))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * http_atotm - 将字符串数组转换为time_t
 */
time_t http_atotm (const char *time_string)
{
  static const char *time_formats[] = {
    "%a, %d %b %Y %T",          /* rfc1123: Thu, 29 Jan 1998 22:12:57 */
    "%A, %d-%b-%y %T",          /* rfc850:  Thursday, 29-Jan-98 22:12:57 */
    "%a %b %d %T %Y",           /* asctime: Thu Jan 29 22:12:57 1998 */
    "%a, %d-%b-%Y %T"           /* cookies: Thu, 29-Jan-1998 22:12:57
                                   (used in Set-Cookie, defined in the
                                   Netscape cookie specification.) */
  };
  const char *oldlocale;
  char savedlocale[256];
  size_t i;
  time_t ret = (time_t) -1;

  /* Solaris strptime fails to recognize English month names in
     non-English locales, which we work around by temporarily setting
     locale to C before invoking strptime.  */
    oldlocale = setlocale(LC_TIME, NULL);
    if (oldlocale) {
        size_t l = strlen(oldlocale);
        if (l >= sizeof savedlocale) {
            savedlocale[0] = '\0';
        } else {
            memcpy(savedlocale, oldlocale, l);
        }
    } else {
        savedlocale[0] = '\0';
    }

    setlocale(LC_TIME, "C");

    for (i = 0; i < countof (time_formats); i++) {
        struct tm t;

        extern char      *strptime(const char *, const char *, struct tm *);


        /* Some versions of strptime use the existing contents of struct
         tm to recalculate the date according to format.  Zero it out
         to prevent stack garbage from influencing strptime.  */
        memset(&t, 0, sizeof(struct tm));

        if (check_end(strptime(time_string, time_formats[i], &t))) {
            ret = timegm(&t);
            break;
        }
    }

    /* Restore the previous locale. */
    if (savedlocale[0]) {
        setlocale(LC_TIME, savedlocale);
    }

    return ret;
}

/*
 * http_select_fd
 * @fd:连接句柄
 * @maxtime: 超时时间
 *
 * 返回值: 0 表示超时
 *        >0 fd接收到消息
 *        <0 出现错误
 */
static int http_select_fd(int fd, double maxtime)
{
    fd_set fdset;
    fd_set *rd;
    fd_set *wr;
    struct timeval tmout;
    int result;

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    rd = &fdset;
    wr = NULL;

    if (maxtime == -1) {
        maxtime = 5;
    }

    tmout.tv_sec = (long)maxtime;
    tmout.tv_usec = 1000000 * (maxtime - (long)maxtime);

    do {
        result = select(fd + 1, rd, wr, NULL, &tmout);
    } while (result < 0 && errno == EINTR);

    if (result == 0) {
        errno = ETIMEDOUT;
    }

    return result;
}

/* 向fd描述符中写 "n"字节数据 */
static ssize_t http_writen(int fd, const void *vptr, size_t n)
{
    size_t          nleft;
    ssize_t         nwritten;
    const char      *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) {
                continue;
            }

            return (-1); /* error */
        }

        nleft -= nwritten;
        ptr += nwritten;
    }

    return (n);
}

/*
 * http_fd_write - 向fd描述符中写 "bufsize"字节数据
 *
 * @fd
 * @buf
 * @timeout
 *
 * 返回0成功
 */
int http_fd_write(int fd, const void *buf, int bufsize, double timeout)
{
    int ret;

    HTTP_DPRINT("http_fd_write buf:%s", (char *)buf);
    if (fd == INVALID_SOCKET) {
        return -1;
    }

    ret = http_writen(fd, buf, bufsize);
    if (ret != bufsize) {
        return -1;
    }

    return 0;
}

/*
 * http_fd_read - 从fd读取不超过bufsize字节大小的数据到buf中,
 *
 * 如果timeout为非0,当过timeout时间后没有数据的时候，操作将终止，并返回-1
 * 如果timeout为-1，将使用默认值5秒
 *
 * @return: < 0 读失败
 *          == 0 表示读到结尾
 *          > 0 表示读到的内容
 */
int http_fd_read(int fd, void *buf, int bufsize, double timeout)
{
    int res;

    if (http_select_fd(fd, timeout) <= 0) {
        return -1;
    }

    do {
        res = read(fd, buf, bufsize);
    } while (res == -1 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN));

    return res;
}

static int http_fd_peek(int fd, char *buf, int bufsize, double timeout)
{
    int res;

    if (http_select_fd(fd, timeout) <= 0) {
        return -1;
    }

    do {
        res = recv(fd, buf, bufsize, MSG_PEEK);
    } while (res == -1 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN));

    return res;
}

/*****************************/

int http_flush(http_util_t *util)
{
    int len;

    len = util->bufidx;

    if (len > 0) {
        util->bufidx = 0;

        return http_fd_write(util->socket, util->buf, len, util->timeout);
    }

    return HTTP_OK;
}

/*
 * 发送报文
 *
 * 成功返回0
 */
static int http_send_raw(http_util_t *util, const void *data, int len)
{
    int i;

    /* 拷贝数据到buf, 移动bufidx, 当buf满了，将buf发送出去  */
    if (util->mode & HTTP_UTIL_IO) {
        i = HTTP_UTIL_BUFLEN - util->bufidx;
        while (len >= i) {
            memcpy(util->buf + util->bufidx, data, i);
            util->bufidx = HTTP_UTIL_BUFLEN;
            util->error = http_flush(util);
            if (util->error != HTTP_OK) {
                return util->error;
            }
            data += i;
            len -= i;
            i = HTTP_UTIL_BUFLEN;
        }
        memcpy(util->buf + util->bufidx, data, len);
        util->bufidx += len;

        return HTTP_OK;
    }

    util->error = http_fd_write(util->socket, data, len, util->timeout);

    return util->error;
}
/**********************/
#if 1

/*
 * terminator回调函数为三个参数: const char *start, const char *peeked, int peeklen
 *
 * @start: 块数据的起始地址
 * @peeked: 开始peek的起始地址
 * @peeklen: peek数据的大小
 */
typedef const char *(*http_hunk_terminator_t)(const char *, const char *, int);

/*
 * 从fd中读取大量数据直到结束标签，该结束标签由terminator回调函数确定，
 * 例如结束标签为一行，那这块数据就是一行数据，如果结束标签为"\r\n\r\n",
 * 那么可以用来读取HTTP响应的报文头.一旦确定边界，该函数返回数据(最大量到结束标签)
 * 的malloc分配的存储
 *
 * terminator回调函数为三个参数: const char *start, const char *peeked, int peeklen
 * 块数据的起始地址、开始peek的起始地址、peek数据的大小
 *
 * 该函数能获取到需要的数据，而不越界获取，因此可以使用read函数读取之后的数据
 * 实现原理：
 *
 * 1. peek传入的数据
 * 2. 通过之前读取到的数据与peek到的数据，确定是否读到结束标签
 *
 * 2.1 如果是的话，read数据直到结束标签为止
 * 2.2 如果不是的话，跳转到第1步
 *
 *
 * @sizehint:初始化缓存大小，该大小应该能容纳典型例子的大小
 * @maxsize: 最大允许分配内存量,如果为0，不做任何限制
 *
 * @return:
 *        1 如果读取错误，返回NULL，
 *        2 当是EOF并且没有数据的情况下，返回NULL 并且errno设置为0
 *        3 当已经读取了部分数据，但出现EOF，会返回数据，但该数据明细是不包含结束标签的
 *        4 会返回数据，包含结束标签
 */
static char *http_fd_read_hunk(int fd, http_hunk_terminator_t terminator, long sizehint, long maxsize)
{
    int         tail; /* tail position in HUNK */
    int         pklen, rdlen, remain;
    long        bufsize = sizehint;
    char        *hunk;
    char        *p;
    const char  *end;

    HTTP_MALLOC(hunk, bufsize);
    if (hunk == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    tail = 0;
    assert(!maxsize || maxsize >= bufsize);
    while (TRUE) {
        /* 预读可用数据  */
        pklen = http_fd_peek(fd, hunk + tail, bufsize - 1 - tail, -1);
        if (pklen < 0) {
            HTTP_FREE(hunk);
            return NULL;
        }

        end = terminator(hunk, hunk + tail, pklen);
        if (end) {
            /* 该数据包含结束标签.  */
            remain = end - (hunk + tail);
            assert(remain >= 0);
            if (remain == 0) {
                /* 不需要再读任何数据 */
                hunk[tail] = '\0';
                return hunk;
            }

            if (bufsize - 1 < tail + remain) {
                bufsize = tail + remain + 1;
                HTTP_REALLOC(p, hunk, bufsize);
                if (p == NULL) {
                    HTTP_FREE(hunk);
                    return NULL;
                }
            }
        } else {
            /* 没有结束标签: 读取.  */
            remain = pklen;
        }

        /*
         * 读取数据： 不能确定能读取多少数据(可能一下TCP协议栈返回的数据比MSG_PEEK少)
         */
        rdlen = http_fd_read(fd, hunk + tail, remain, 0);
        if (rdlen < 0) {
            HTTP_FREE(hunk);
            return NULL;
        }
        tail += rdlen;
        hunk[tail] = '\0';

        if (rdlen == 0) {
            if (tail == 0) {
                /* EOF without anything having been read */
                HTTP_FREE(hunk);
                errno = 0;
                return NULL;
            } else {
                /* EOF seen: return the data we've read. */
                return hunk;
            }
        }
        if (end && rdlen == remain) {
            /* 读取的数据已经包含结尾标签 */
            return hunk;
        }

        /* Keep looping until all the data arrives. */
        if (tail == bufsize - 1) {
            /* 在缓冲不超过maxsize大小的情况下扩大一倍缓冲大小 */
            if (maxsize && bufsize >= maxsize) {
                HTTP_FREE(hunk);
                errno = ENOMEM;
                return NULL;
            }
            bufsize <<= 1;
            if (maxsize && bufsize > maxsize)
                bufsize = maxsize;
            HTTP_REALLOC(p, hunk, bufsize);
            if (p == NULL) {
                HTTP_FREE(hunk);
                return NULL;
            }
        }
    }

    return NULL;
}

static const char *line_terminator(const char *start, const char *peeked, int peeklen)
{
    const char *p = memchr(peeked, '\n', peeklen);
    if (p) {
        /* p+1 because the line must include '\n' */
        return p + 1;
    }
    return NULL;
}



/*
 * http_fd_read_line 从fd中读取一行数据, 但一行数据不会超过FD_READ_LINE_MAX(2048)
 *
 * @return 如果发生错误，或者没有数据可以读取，返回NULL。
 *          如何errno为0 就是没有数据可以读取
 *          其他情况,就是errno指示的错误
 *
 */
char *http_fd_read_line(int fd)
{
    return http_fd_read_hunk(fd, line_terminator, 128, FD_READ_LINE_MAX);
}

/*
 * http_response_head_terminator - http响应报文头结束
 *
 * 从缓冲 [start, peeked + peeklen) 找到一个空行(\r\n\r\n)
 * 如果找到返回指向空行之后的指针
 *
 * [start, peeked) 这里的数据为已经读取的数据
 * [peeked, peeked + peeklen) 这个为预读的数据
 *
 *
 */
static const char *http_response_head_terminator(const char *start, const char *peeked, int peeklen)
{
    const char *p, *end;

    /* 如果是第一次预读, 必须要保证有HTTP */
    if (start == peeked && 0 != memcmp(start, "HTTP", MIN(peeklen, 4))) {
        HTTP_DPRINT("the first line is not HTTP STATUS");
        return start;
    }

    /* Look for "\n[\r]\n", and return the following position if found.
     Start two chars before the current to cover the possibility that
     part of the terminator (e.g. "\n\r") arrived in the previous
     batch.  */
    p = peeked - start < 2 ? start : peeked - 2;
    end = peeked + peeklen;

    /* Check for \n\r\n or \n\n anywhere in [p, end-2). */
    for (; p < end - 2; p++) {
        if (*p == '\n') {
            if (p[1] == '\r' && p[2] == '\n') {
                return p + 3;
            }
            if (p[1] == '\n') {
                return p + 2;
            }
        }
    }
    /* p==end-2: check for \n\n directly preceding END. */
    if (p[0] == '\n' && p[1] == '\n') {
        return p + 2;
    }

    return NULL;
}

/**
 * http_read_http_response_head - 接收http报文响应
 * @fd:连接句柄
 *
 * 功能：接收http报文响应
 *
 * 返回值： 如果发生错误，或者没有数据可以读取，返回NULL。
 *        如何errno为0 就是没有数据可以读取
 *        其他情况,就是errno指示的错误
 */
char *http_read_http_response_head(int fd)
{
    return http_fd_read_hunk(fd, http_response_head_terminator, 512, HTTP_RESPONSE_MAX_SIZE);
}

#endif

/*****************************/
bool hostname_verify(char *name) __attribute__((weak));
int dns_domain_to_ip_block(char *domain, struct sockaddr_storage *p_ip, struct timeval *timeout) __attribute__((weak));

/* 转换addr地址  */
static int http_tcp_gethost(const char *addr, struct in_addr *inaddr)
{
    int                         ret;
    u_int32_t                   iadd;
    struct timeval              timeout;
    struct sockaddr_storage     addr_stg;            /* 存放转换后的IP地址 */

    iadd = inet_addr(addr);
    if (iadd != INADDR_NONE) {
        inaddr->s_addr = iadd;
        return 0;
    }

    if (hostname_verify == 0 || dns_domain_to_ip_block == 0) {
        return -1;
    }

    if (hostname_verify((char *)addr) == FALSE) {
        return -1;
    }

    /* 域名解析 */
    timeout.tv_sec = 2;
    timeout.tv_usec = 0; /* 表示最多阻塞2秒，2秒后还没有返回结果就失败 */

    ret = dns_domain_to_ip_block((char *)addr, &addr_stg, &timeout);
    if (ret < 0) {
        return -1;
    }

    if (addr_stg.ss_family == AF_INET) {
        inaddr->s_addr = ((struct sockaddr_in *)&addr_stg)->sin_addr.s_addr;
    }

    return 0;
}

/*
 * http_util_clean_string - 删除字符串的前后空格,包括回车换行
 * @pbuf: 待处理字符串指针
 *
 * 功能：删除字符串的前后空格,包括回车换行
 *
 * 返回值: 无
 */
static void http_util_clean_string(char *str)
{
    char *start;
    char *end;
    char *p;

    start = str;
    end = str;
    p = str;
    while (*p) {
        switch (*p) {
        case ' ':
        case '\r':
        case '\n':
            if (str != start) {
                *start = *p;
                start++;
            }
            break;
        default:
            *start = *p;
            start++;
            end = start;
            break;
        }
        p++;
    }

    *end = '\0';
}

/*
 * http_parse_url - 解析url字符串
 * @url: 需要解析的url字符串
 * @host: 服务器地址, 输出参数
 * @port: 端口号, 输出参数
 * @path: 路径, 输出参数
 * @filename: 文件名, 输出参数
 *
 * 功能：解析url字符串，获取服务器地址、端口号、路径、文件名
 *      举例：192.168.23.137:8080/version/web_management_pack.upd
 *
 * return: 无
 */
static void http_parse_url(char *url, char *host, int *port, char *path, char *filename)
{
    int     port_len;
    int     host_len;
    char    port_str[HTTP_PORT_STR_LEN];
    char    *path_pos, *port_pos, *file_pos, *c;

    /* 获取文件下载路径 */
    path_pos = strstr(url, "/");
    if (path_pos == NULL) {
        path_pos = url + strlen(url);
        strncpy(path, "/", HTTP_PATH_SIZE - 1);
    } else {
        strncpy(path, path_pos, HTTP_PATH_SIZE - 1);
    }

    /* 获取host和端口号, 如果url没有给出端口号则使用默认端口80 */
    memset(port_str, 0, sizeof(port_str));
    port_pos = strchr(url, ':');
    if (port_pos && port_pos < path_pos) {
        host_len = (port_pos - url < HTTP_HOST_SIZE) ? (port_pos - url) : (HTTP_HOST_SIZE - 1);
        strncpy(host, url, host_len);

        port_pos++; /* 跳过':' */
        port_len = (path_pos - port_pos < HTTP_PORT_STR_LEN)
                    ? (path_pos - port_pos) : (HTTP_PORT_STR_LEN - 1);
        strncpy(port_str, port_pos, port_len);
        http_util_clean_string(port_str);
        *port = atoi(port_str);
        if (*port == 0) {
            *port = HTTP_DEFAULT_PORT;
        }
    } else {
        host_len = (path_pos - url < HTTP_HOST_SIZE) ? (path_pos - url) : (HTTP_HOST_SIZE - 1);
        strncpy(host, url, host_len);
        *port = HTTP_DEFAULT_PORT;
    }

    /* 获取文件名,用以保存接收到的文件数据 */
    file_pos = strrchr(url, '/');
    if (file_pos == NULL || (file_pos == url + strlen(url) - 1)) {
        /* 找不到'/'或'/'恰好在url末尾, 表示url未给出文件名, 默认文件名为index.html */
        (void)strncpy(filename, "index.html", HTTP_FILE_NAME_SIZE - 1);
    } else {
        file_pos++;
        (void)strncpy(filename, file_pos, HTTP_FILE_NAME_SIZE - 1);
        c = strchr(filename, '?');
        if (c != NULL) {
            *c = '\0';
        }
    }

    return;
}

/**
 * http_sock_ntop - 根据sockaddr和长度返回地址值
 * @sa:地址信息
 * @salen：地址信息长度
 *
 * 功能：根据sockaddr和长度返回地址值
 *
 * 返回值：成功返回指针，失败返回NULL
 */
char *http_sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char portstr[7];
    static char str[128]; /* Unix domain is largest */

    switch (sa->sa_family)case AF_INET: {
        struct sockaddr_in *sin = (struct sockaddr_in *) sa;

        if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL) {
            return NULL;
        }
        if (ntohs(sin->sin_port) != 0) {
            snprintf(portstr, sizeof(portstr), ".%d", ntohs(sin->sin_port));
            strcat(str, portstr);
        }
        return str;
        /* end sock_ntop */

#ifdef  IPV6
        case AF_INET6:
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

        if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL) {
            return NULL;
        }
        if (ntohs(sin6->sin6_port) != 0) {
            snprintf(portstr, sizeof(portstr), ".%d", ntohs(sin6->sin6_port));
            strcat(str, portstr);
        }
        return str;

#endif

#ifdef  AF_UNIX
        case AF_UNIX: {
            struct sockaddr_un *unp = (struct sockaddr_un *) sa;

            /* OK to have no pathname bound to the socket: happens on
             *       every connect() unless client calls bind() first.
             */
            if (unp->sun_path[0] == 0) {
                strcpy(str, "(no pathname bound)");
            } else {
                snprintf(str, sizeof(str), "%s", unp->sun_path);
            }
            return str;
        }
#endif
        default:
        snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
                sa->sa_family, salen);
        return str;
    }

    return NULL;
}

/**
 * http_create_url - 解析url值
 * @url：指向要解析的url指针
 * @err：如果解析出错，存放解析出错值
 *
 * 功能：解析url值到http_url_t结构体中
 *
 * 返回值：成功返回指向http_url_t指针，失败返回NULL
 */
http_url_t *http_create_url(char *url, bool is_mngt, http_err_t *err)
{
    int         ret;
    char        *url_tmp;
    http_url_t  *http_url;

    if (url == NULL) {
        *err = HTTP_EINVAL;
        return NULL;
    }

    /* 下载路径必须以"http://"开头 */
    if (strncasecmp(url, "http://", strlen("http://")) != 0) {
        HTTP_DPRINT("Only HTTP protocol is supported! url:%s", url);
        *err = HTTP_URLERROR;
        return NULL;
    }
    url_tmp = url + strlen("http://");

    HTTP_MALLOC(http_url, sizeof(http_url_t));
    if (http_url == NULL) {
        HTTP_DPRINT("malloc for url fail");
        *err = HTTP_EMEMORY;
        return NULL;
    }

    http_parse_url(url_tmp, http_url->host, &http_url->port, http_url->path, http_url->filename);
    http_url->url = url;

    HTTP_DPRINT("host [%s] port [%d] path [%s] filename [%s]", http_url->host, http_url->port,
        http_url->path, http_url->filename);

    /* 使用linux自带的gethostbyname函数来进行域名解析 */
    HTTP_DPRINT("[%s %d]: dns parsed by linux gethostbyname func, host(%s)",
        __FUNCTION__, __LINE__, http_url->host);
    struct hostent *hptr;
    char **pptr;
    char str[32];
    if ((hptr = gethostbyname(http_url->host)) == NULL) {
        HTTP_DPRINT("[%s %d]: %s fail to gethostbyname",
            __FUNCTION__, __LINE__, http_url->host);
        ret = HTTP_OTHER;
    }
    if (hptr->h_addrtype != AF_INET) {
        HTTP_DPRINT("[%s %d]: %s invalid ss_family",
            __FUNCTION__, __LINE__, http_url->host);
        ret = HTTP_OTHER;
    }
    if (*(hptr->h_addr_list) == NULL) {
        HTTP_DPRINT("[%s %d]: %s invalid ip address",
            __FUNCTION__, __LINE__, http_url->host);
        ret = HTTP_OTHER;
    }
    pptr = hptr->h_addr_list;
    for (; *pptr != NULL; pptr++) {
        HTTP_DPRINT("[%s %d]: ip address(%s)",
            __FUNCTION__, __LINE__, inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
    }
    http_url->server_addr.sin_addr = *((struct in_addr *)hptr->h_addr_list[0]);
    ret = HTTP_OK;
    HTTP_DPRINT("[%s %d]: dns parsed by linux gethostbyname func, ret(%d)",
        __FUNCTION__, __LINE__, ret);
    /* end of 使用linux自带的gethostbyname函数来进行域名解析 */

    if (ret != 0) {
        *err = HTTP_HOSTERR;
        HTTP_DPRINT("gethost to addr fail");
        HTTP_FREE(http_url);
        return NULL;
    }

    http_url->is_mngt = is_mngt;
    http_url->server_addr.sin_family = AF_INET;
    http_url->server_addr.sin_port = htons(http_url->port);
    HTTP_DPRINT("ip_addr [%s]", http_sock_ntop((struct sockaddr *)&http_url->server_addr,
        sizeof(http_url->server_addr)));

    return http_url;
}

int lsm_vrf_name2index(const char *vrf_name) __attribute__((weak));

/*
 * http_create_socket - 创建unix字节流套接字
 * @sa:要连接的地址信息
 * @len：地址信息长度
 * @is_nblock: 是否阻塞
 * @is_mgnt: 是否为管理口
 *
 * 功能：创建unix字节流套接字
 *
 * 返回值：成功返回0；失败返回-1
 */
int http_create_socket(const struct sockaddr *sa, int len, bool is_nblock, bool is_mgnt)
{
    int     val;
    int     sockfd;
    int     vrf_index;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        HTTP_DPRINT("errno:%s", strerror(errno));
        return -1;
    }

    if (is_nblock) {
        val = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, val | O_NONBLOCK);
    }

#ifndef HTTP_NOMGMT_VRF
    if (is_mgnt) {
        if (lsm_vrf_name2index == 0) {
            HTTP_DPRINT("no suport lsm_vrf_name2index");
            close(sockfd);
            return -1;
        }

        HTTP_DPRINT("lsm_vrf_name2index %p", lsm_vrf_name2index);
        vrf_index = lsm_vrf_name2index(MGMT_VRF);
        if (vrf_index < 0) {
            HTTP_DPRINT("lsm_vrf_name2index name:%s error:%d", MGMT_VRF, vrf_index);
            close(sockfd);
            return -1;
        }

        if (setsockopt(sockfd, SOL_IP, IP_VRF_INCOMING, &vrf_index, sizeof(vrf_index)) < 0) {
            HTTP_DPRINT("set vrf incoming option error.");
            close(sockfd);
            return -1;
        }
        if (setsockopt(sockfd, SOL_IP, IP_VRF_SEND, &vrf_index, sizeof(vrf_index)) < 0) {
            HTTP_DPRINT("set vrf send option error");
            close(sockfd);
            return -1;
        }
    }
#endif
    HTTP_DPRINT("ip_addr [%s]", http_sock_ntop(sa, len));
    if (connect(sockfd, sa, len) < 0) {
        HTTP_DPRINT("[http-tty]-->connect tty-server errno %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

static int http_send_header(http_util_t *util, const char *s)
{
    const char *t;

    do {
        t = strchr(s, '\n'); /* disallow \n in HTTP headers */
        if (!t) {
            t = s + strlen(s);
        }

        if (http_send_raw(util, s, t - s)) {
            return util->error;
        }
        s = t + 1;
    } while (*t);

    return HTTP_OK;
}

int http_post_header(http_util_t *util, const char *key, const char *val)
{
    if (key) {
        if (http_send_header(util, key)) {
            return util->error;
        }
        if (val && (http_send_raw(util, ": ", 2) || http_send_header(util, val))) {
            return util->error;
        }
    }

    return http_send_raw(util, "\r\n", 2);
}

/* 发送get请求 */
static int http_get_request(http_util_t *util, int restval, char *path, char *host, int port)
{
    int             ret;
    char            buf[64];
    const char      *meth;

    meth = "GET";
    sprintf(util->tmpbuf, "%s /%s HTTP/1.1", meth, (*path == '/' ? path + 1 : path));
    ret = http_post_header(util, util->tmpbuf, NULL);
    if (ret != HTTP_OK) {
        HTTP_DPRINT("errno:%s", strerror(errno));
        return ret;
    }

    if (port != 80) {
        sprintf(util->tmpbuf, "%s:%d", host, port);
    } else {
        strcpy(util->tmpbuf, host);
    }

    ret = http_post_header(util, "Host", util->tmpbuf);
    if (ret != HTTP_OK) {
        return ret;
    }

    if (restval > 0) {
        snprintf(buf, sizeof(buf), "bytes=%d-", restval);
        http_post_header(util, "Range", buf);
    }

    if ((ret = http_post_header(util, "Accept", "*/*"))
            || (ret = http_post_header(util, "Accept-Language", "zh-cn"))
            || (ret = http_post_header(util, "User-Agent", "RG/Device 11.x"))) {
        return ret;
    }

    /* 发送报文结束标签 */
    ret = http_post_header(util, NULL, NULL);
    if (ret != HTTP_OK) {
        return ret;
    }

    http_flush(util);

    return HTTP_OK;
}

/*
 * http_tag_cmp 比较两个标签
 *
 * @return: 返回0表示相等
 */
int http_tag_cmp(char *s, char *t)
{
    register int c1;
    register int c2;

    while (TRUE) {
        c1 = *s;
        c2 = *t;

        if (!c1 || c1 == '"') {
            break;
        }
        if (c2 != '-') {
            if (c1 != c2) {
                if (c1 >= 'A' && c1 <= 'Z') {
                    c1 += 'a' - 'A';
                }
                if (c2 >= 'A' && c2 <= 'Z') {
                    c2 += 'a' - 'A';
                }
            }
            if (c1 != c2) {
                if (c2 != '*') {
                    return 1;
                }
                c2 = *++t;
                if (!c2)
                    return 0;
                if (c2 >= 'A' && c2 <= 'Z')
                    c2 += 'a' - 'A';
                for (;;) {
                    c1 = *s;
                    if (!c1 || c1 == '"')
                        break;
                    if (c1 >= 'A' && c1 <= 'Z')
                        c1 += 'a' - 'A';
                    if (c1 == c2 && !http_tag_cmp(s + 1, t + 1))
                        return 0;
                    s++;
                }
                break;
            }
        }
        s++;
        t++;
    }

    if (*t == '*' && !t[1]) {
        return 0;
    }

    return *t;
}

/*
 * parse_content_range - 解析Content-Range头部并提取信息
 *
 * @hdr:  http头
 * @first_byte_ptr: 输出
 * @last_byte_ptr: 输出
 * @entity_length_ptr: 输出
 *
 * @return
 */
bool parse_content_range (const char *hdr, size_t *first_byte_ptr,
        size_t *last_byte_ptr, size_t *entity_length_ptr)
{
    size_t num;

    /* Ancient versions of Netscape proxy server, presumably predating
     rfc2068, sent out `Content-Range' without the "bytes"
     specifier.  */
    if (strncasecmp(hdr, "bytes", 5) == 0) {
        hdr += 5;
        /* "JavaWebServer/1.1.1" sends "bytes: x-y/z", contrary to the
         HTTP spec. */
        if (*hdr == ':')
            ++hdr;
        while (isspace((unsigned char)*hdr))
            ++hdr;
        if (!*hdr)
            return FALSE;
    }

    if (*hdr == '*') {
        ++hdr;
        if (*hdr == '*') {
            num = -1;
        } else {
            for (num = 0; isdigit((int)*hdr); hdr++)
                num = 10 * num + (*hdr - '0');
        }
        *entity_length_ptr = num;

        return TRUE;
    }

    if (!isdigit((int)*hdr)) {
        return FALSE;
    }
    for (num = 0; isdigit((int)*hdr); hdr++)
        num = 10 * num + (*hdr - '0');
    if (*hdr != '-' || !isdigit((int)*(hdr + 1)))
        return FALSE;
    *first_byte_ptr = num;
    ++hdr;
    for (num = 0; isdigit((int)*hdr); hdr++)
        num = 10 * num + (*hdr - '0');
    if (*hdr != '/' || !isdigit((int)*(hdr + 1)))
        return FALSE;
    *last_byte_ptr = num;
    ++hdr;
    if (*hdr == '*') {
        num = -1;
    } else {
        for (num = 0; isdigit((int)*hdr); hdr++)
            num = 10 * num + (*hdr - '0');
    }
    *entity_length_ptr = num;

    return TRUE;
}

/*
 * 解析头
 */
int http_parse_rsp_head(http_response_t *rsp, http_stat_t *hs, char *key, char *val)
{
    bool ret;
    HTTP_DPRINT("%s: %s", key, val);

    if (http_tag_cmp(key, "Content-Length") == 0) {
        rsp->content_len = atol(val);
    } else if (http_tag_cmp(key, "Content-Type") == 0) {

    } else if (http_tag_cmp(key, "Connection") == 0) {
        if (!http_tag_cmp(val, "keep-alive")) {
            rsp->keep_alive = TRUE;
        }
        if (!http_tag_cmp(val, "close")) {
            rsp->keep_alive = FALSE;
        }
    } else if (http_tag_cmp(key, "Accept-Ranges") == 0) {
        if (strcmp(val, "none") == 0) {
            rsp->accept_ranges = FALSE;
        } else if (strcmp(val, "bytes") == 0) {
            rsp->accept_ranges = TRUE;
        }
    } else if (http_tag_cmp(key, "Content-Range") == 0) {
        ret = parse_content_range(val, &rsp->range_begin, &rsp->range_end, &rsp->file_len);
        if (ret != TRUE) {
            HTTP_DPRINT("parse content-range fail");
            return HTTP_ERR;
        }
        HTTP_DPRINT("%ld-%ld/%ld", (long)rsp->range_begin, (long)rsp->range_end, (long)rsp->file_len);
    } else if (http_tag_cmp(key, "Last-Modified") == 0) {

        strncpy(rsp->remote_time, val, sizeof(rsp->remote_time) - 1);
        rsp->modify_tstamp = http_atotm (val);
        if (rsp->modify_tstamp == (time_t) (-1)) {
            HTTP_DPRINT ("Last-modified header invalid -- time-stamp ignored.\n");
        }
    }

    return HTTP_OK;
}

/*
 * http_getline - 从buf中获得一行数据
 *
 * @buf: 数据
 * @line: 输出,
 * @size: 一行的大小, 如果一行数据超过size大小, 返回错误
 *
 * @return: 返回指向跳过一行的数据
 */
char *http_getline(const char *buf, char *line, int size)
{
    char *end;
    int len;

    end = strstr(buf, "\r\n");
    if (end == NULL) {
        /* 没有找到 */
        return NULL;
    }

    if (end == buf) {
        /*  */
        line[0] = '\0';
        end +=2;
        return end;
    }

    len = end - buf;
    if (len + 1 > size) {
        /* 超过最大范围 */
        return NULL;
    }
    memcpy(line, buf, len);
    line[len] = '\0';

    end +=2;

    return end;
}

/* 解析处理http响应状态 */
int http_parse_resp_status(http_response_t *rsp, char *header)
{
    const char *p, *end;

    HTTP_DPRINT("status: %s", header);

    p = header;
    end = header + strlen(header) - 1;

    /* "HTTP" */
    if (end - p < 4 || 0 != strncmp(p, "HTTP", 4)) {
        return -1;
    }
    p += 4;

    /* Match the HTTP version.  This is optional because Gnutella
     servers have been reported to not specify HTTP version.  */
    if (p < end && *p == '/') {
        ++p;
        while (p < end && isdigit((int)*p)) {
            ++p;
        }
        if (p < end && *p == '.') {
            ++p;
        }
        while (p < end && isdigit((int)*p)) {
            ++p;
        }
    }

    while (p < end && isspace((unsigned char)*p)) {
        ++p;
    }
    if (end - p < 3 || !isdigit((int)p[0]) || !isdigit((int)p[1]) || !isdigit((int)p[2])) {
        return -1;
    }

    rsp->status = 100 * (p[0] - '0') + 10 * (p[1] - '0') + (p[2] - '0');
    p += 3;
    if (p < end && isspace((unsigned char)*p)) {
        ++p;
    } else {
        return -1;
    }

    HTTP_DPRINT("rsp->status:%d status message: %s", rsp->status, p);

    return HTTP_OK;
}

static int http_process_response_head(http_util_t *util, http_stat_t *hs)
{
    int  statcode;
    http_response_t *http_rsp;

    http_rsp = &util->http_rsp;
    statcode = http_rsp->status;

    HTTP_DPRINT("statcode:%d.", statcode);
    if (statcode == HTTP_STATUS_UNAUTHORIZED) {
        /* Authorization is required.  */

        /* 暂不支持 */

        return HTTP_AUTHFAILED;
    }

    /* 申请的字节数超出文件的实际大小范围，服务器给出该响应。 */
    if (statcode == HTTP_STATUS_RANGE_NOT_SATISFIABLE) {
        /* HTTP/1.1 416 Requested Range Not Satisfiable */
        HTTP_DPRINT("The file is already fully retrieved; nothing to do.");
        return HTTP_UNNEEDED;
    }

    /* 响应OK，但本地存在的文件已经大于等于传过来的文件大小 */
    if (statcode == HTTP_STATUS_OK && http_rsp->range_begin == 0
            && hs->restval >= http_rsp->content_len) {
        HTTP_DPRINT("The file is already fully retrieved; nothing to do.");
        return HTTP_UNNEEDED;
    }

    /* 当断点续传处理字段不为0时，它的值需要与重传的值一样 */
    if (http_rsp->range_begin != 0 && http_rsp->range_begin != hs->restval) {
        return HTTP_RANGEERR;
    }

    /* 206响应，续传起始地址不能为0. */
    if (H_PARTIAL (statcode) && http_rsp->range_begin == 0) {
        return HTTP_HDR;
    }

    /* 重定向 */
    if (H_REDIRECTED(statcode) || statcode == HTTP_STATUS_MULTIPLE_CHOICES) {
        return HTTP_NEWLOCATION;
    }

    /* 默认20X表示成功 */
    if (H_20X(statcode)) {
        return HTTP_OK;
    }

    HTTP_DPRINT("statcode:%d is not suport.", statcode);

    return HTTP_HDR;
}

/* 解析http报文头数据 */
int http_parse_http_response_head(http_util_t *util, http_stat_t *hs, char *buf)
{
    char header[512];
    char *c;
    char *s;
    char *t;
    int ret;

    memset(&util->http_rsp, 0, sizeof(util->http_rsp));
    util->http_rsp.content_len = -1;
    util->http_rsp.modify_tstamp = -1;
    c = buf;
    c = http_getline(c, header, sizeof(header));
    if (c == NULL) {
        /* 报文不是以\r\n结尾 */
        HTTP_DPRINT("parse response header error because the packet have't CRLF");
        return HTTP_ERR;
    }
    ret = http_parse_resp_status(&util->http_rsp, header);
    if (ret != HTTP_OK) {
        HTTP_DPRINT("http_parse_resp_status error");
        return HTTP_ERR;
    }

    while (TRUE) {
        c = http_getline(c, header, sizeof(header));
        if (c == NULL) {
            /* 报文不是以\r\n结尾 */
            HTTP_DPRINT("parse response header error because the packet have't CRLFCRLF");
            return HTTP_ERR;
        }
        if (header[0] == '\0') {
            /* head报文解析完毕 */
            break;
        }
        HTTP_DPRINT("HTTP header: %s", header);

        /* 解析头 */
        s = strchr(header, ':');
        if (s == NULL) {
            continue;
        }

        /* 将key与值解析出来 */
        *s = '\0';
        do {
            s++;
        } while (*s && *s <= 32);
        if (*s == '"') {
            s++;
        }
        t = s + strlen(s) - 1;
        while (t > s && *t <= 32) {
            t--;
        }
        if (t >= s && *t == '"') {
            t--;
        }
        t[1] = '\0';

        ret = http_parse_rsp_head(&util->http_rsp, hs, header, s);
        if (ret != HTTP_OK) {
            return ret;
        }
    }

    ret = http_process_response_head(util, hs);
    if (ret != HTTP_OK) {
        return ret;
    }

    return HTTP_OK;
}

/**
 * http_create_util - 创建util结构体
 *
 * 功能：创建util结构体
 *
 * 返回值：成功返回指针；失败返回NULL
 */
http_util_t *http_create_util(void)
{
    http_util_t     *util;

    HTTP_MALLOC(util, sizeof(http_util_t));
    if (util == NULL) {
        return NULL;
    }

    util->socket = INVALID_SOCKET;
    util->timeout = 5;
    util->mode |= HTTP_UTIL_IO;

    return util;
}

void http_free_util(http_util_t *util)
{
    if (util == NULL) {
        return;
    }

    if (util->socket != INVALID_SOCKET) {
        CLOSE_INVALIDATE(util->socket);
    }

    HTTP_FREE(util);
}

bool http_skip_short_body (int fd, size_t contlen)
{
  enum {
    SKIP_SIZE = 512,                /* size of the download buffer */
    SKIP_THRESHOLD = 8192        /* the largest size we read */
  };
  char dlbuf[SKIP_SIZE + 1];

  dlbuf[SKIP_SIZE] = '\0';        /* so DEBUGP can safely print it */

  /* We shouldn't get here with unknown contlen.  (This will change
     with HTTP/1.1, which supports "chunked" transfer.)  */
  //assert (contlen != -1);

  /* If the body is too large, it makes more sense to simply close the
     connection than to try to read the body.  */
  if (contlen > SKIP_THRESHOLD) {
    return FALSE;
  }

  HTTP_DPRINT("Skipping %ld bytes of body[", (long)contlen);

  while (contlen > 0)
    {
      int ret = http_fd_read(fd, dlbuf, MIN (contlen, SKIP_SIZE), -1);
      if (ret <= 0)
        {
          /* Don't normally report the error since this is an
             optimization that should be invisible to the user.  */
          HTTP_DPRINT("] aborting (%s)", strerror(errno));
          return FALSE;
        }
      contlen -= ret;
      /* Safe even if %.*s bogusly expects terminating \0 because
         we've zero-terminated dlbuf above.  */
      HTTP_DPRINT("%.*s", ret, dlbuf);
    }

  HTTP_DPRINT("] done.\n");

  return TRUE;
}

/* Write data in BUF to OUT.  However, if *SKIP is non-zero, skip that
   amount of data and decrease SKIP.  Increment *TOTAL by the amount
   of data written.  */

static int write_data(FILE *out, const char *buf, int bufsize, size_t *skip)
{
    if (!out) {
        return 1;
    }
    if (*skip > bufsize) {
        *skip -= bufsize;
        return 1;
    }
    if (*skip) {
        buf += *skip;
        bufsize -= *skip;
        *skip = 0;
        if (bufsize == 0) {
            return 1;
        }
    }

    fwrite(buf, 1, bufsize, out);
    fflush(out);

    return !ferror(out);
}

static int http_change_progess(http_stat_action_t status, http_stat_t *hs, size_t len)
{
    http_progress_t *progress;

    if (hs->progress == NULL) {
        return 0;
    }

    progress = hs->progress;
    switch (status) {
    case HTTP_STAT_BEGIN:
        progress->len = 0;
        break;
    case HTTP_STAT_LENGTH:
        progress->len = hs->restval;
        progress->file_len = len;
        break;
    case HTTP_STAT_READ:
        progress->len += len;
        if (progress->notify != NULL) {
            if (progress->notify(progress, len) != 0) {
                return -1;
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

/*
 *
 *
 * @toread: 如果值为 -1 就一直读数据直到读到EOF
 *
 * @return -1 表示读socket失败  -2表示写文件失败 -3用户主动取消
 */
int http_fd_read_body(int fd, FILE *fp, http_stat_t *hs, size_t toread, size_t skip)
{
    char buf[1024];
    size_t contlen;
    int ret;
    int read_len;
    int rety;

    if (toread == -1) {
        return -1;
    }

    rety = 5;
    contlen = toread;
    while (contlen > 0) {
        read_len = http_fd_read(fd, buf, MIN(contlen, 1024), 1);
        if (read_len <= 0) {
            if (errno == ETIMEDOUT && rety > 0) {
                rety--;
                if (http_change_progess(HTTP_STAT_READ, hs, 0) != 0) {
                    HTTP_DPRINT("cancle by user.\n");
                    return -3;
                }
                continue;
            }
            HTTP_DPRINT("aborting (%s).\n", strerror(errno));
            return -1;
        }
        contlen -= read_len;
        rety = 5;

        /* 保存 */
        ret = write_data(fp, buf, read_len, &skip);
        if (ret == 0) {
            HTTP_DPRINT("write_data fail");
            return -2;
        }

        if (http_change_progess(HTTP_STAT_READ, hs, read_len) != 0) {
            HTTP_DPRINT("cancle by user.\n");
            return -3;
        }
    }

    return 0;
}

/*********************/
static http_err_t http_get(http_url_t *http_url, http_util_t *util, http_stat_t *hs)
{
    int             ret;
    char            *head;
    FILE            *fp;
    size_t          skip;
    http_response_t *http_rsp;

    if (util->socket == INVALID_SOCKET) {
        util->socket = http_create_socket((struct sockaddr *)&http_url->server_addr,
            sizeof(http_url->server_addr), FALSE, http_url->is_mngt);
        if (util->socket < 0) {
            HTTP_DPRINT("errno:%s", strerror(errno));
            return HTTP_CONERROR;
        }

        HTTP_DPRINT("connect %s success util->socket:%d",
            http_sock_ntop((struct sockaddr *)&http_url->server_addr, sizeof(http_url->server_addr)),
            util->socket);
    }

    /* 发送get请求 */
    ret = http_get_request(util, hs->restval, http_url->path, http_url->host, http_url->port);
    if (ret != 0) {
        HTTP_DPRINT("errno %s", strerror(errno));
        CLOSE_INVALIDATE(util->socket);
        return HTTP_WRITEFAILED;    /* 写失败 */
    }

    HTTP_DPRINT("http_post_request success");

    head = http_read_http_response_head(util->socket);
    if (head == NULL) {
        if (errno == 0) {
            HTTP_DPRINT("No data received");
            CLOSE_INVALIDATE(util->socket);
            return HTTP_EOF;
        }

        HTTP_DPRINT("Read error (%s) in headers", strerror(errno));
        CLOSE_INVALIDATE(util->socket);
        return HTTP_ERR;
    }

    HTTP_DPRINT("ret:%d\n---response begin---\n%s---response end---\n", ret, head);

    ret = http_parse_http_response_head(util, hs, head);
    if (ret != HTTP_OK) {
        HTTP_DPRINT("parse head error.\n");
        CLOSE_INVALIDATE(util->socket);
        HTTP_FREE(head);
        return ret;
    }
    HTTP_FREE(head);

    /* Download the request body.  */
    /* YGXXX 如果响应有content-length就是要这个，如果没有就需要读sock读到EOF
     *  目前先只处理报文一定要有content-length
     */
    http_rsp = &util->http_rsp;
    if (http_rsp->content_len == -1) {
        HTTP_DPRINT("can't find content-length.\n");
        CLOSE_INVALIDATE(util->socket);
        return HTTP_EOF;
    }
    HTTP_DPRINT("http_rsp->file_len:%d http_rsp->content_len:%d", (int)http_rsp->file_len,
            (int)http_rsp->content_len);

    HTTP_DPRINT("************");

    if (http_rsp->modify_tstamp != (time_t) (-1)) {
        if (http_opt.timestamping && hs->restval > 0 && http_rsp->modify_tstamp != hs->orig_file_tstamp) {
            //删除文件
            unlink(hs->local_file);
            HTTP_DPRINT("unlink %s.\n", hs->local_file);
            CLOSE_INVALIDATE(util->socket);
            return HTTP_RETRY;
        }
    }

    HTTP_DPRINT("************");

    fp = fopen(hs->local_file, "ab");
    if (fp == NULL) {
        HTTP_DPRINT("fopen file error %s.", strerror(errno));
        CLOSE_INVALIDATE(util->socket);
        return HTTP_FOPENERR;
    }

    HTTP_DPRINT("************");

    if (http_rsp->file_len == 0) {
        http_rsp->file_len = http_rsp->content_len;
    }
    http_change_progess(HTTP_STAT_LENGTH, hs, http_rsp->file_len);

    skip = 0;
    if (hs->restval > 0 && http_rsp->range_begin == 0) {
      /* 如果服务器忽略range请求，http_fd_read_body函数需要跳过hs->restval的字节 */
        skip = hs->restval;
    }

    HTTP_DPRINT("************");

    ret = http_fd_read_body(util->socket, fp, hs, http_rsp->content_len, skip);
    if (ret < 0) {
        HTTP_DPRINT(" errno %s", strerror(errno));
        fclose (fp);

        if (http_rsp->modify_tstamp != (time_t) (-1)) {
            touch (hs->local_file, http_rsp->modify_tstamp);
            HTTP_DPRINT("modify %s time %s  success", hs->local_file, http_rsp->remote_time);
        }

        if (ret == -2) {
            return HTTP_FWRITEERR;
        }
        /* 关闭连接 */
        CLOSE_INVALIDATE(util->socket);

        if (ret == -3) {
            return HTTP_USER_CANCLE;
        }

        return HTTP_READERR;
    }
    HTTP_DPRINT("save  success");

    if (http_rsp->keep_alive != TRUE) {
        CLOSE_INVALIDATE(util->socket);
    }

    fclose (fp);

    if (http_rsp->modify_tstamp != (time_t) (-1)) {
        touch (hs->local_file, http_rsp->modify_tstamp);
        HTTP_DPRINT("modify %s time %s  success", hs->local_file, http_rsp->remote_time);
    }

    return HTTP_OK;
}

static void http_free_hstat (http_stat_t *hs)
{
    if (hs == NULL) {
        return;
    }
    HTTP_FREE(hs->local_file);

    hs->local_file = NULL;
}

int random_number(int max)
{
    static int rnd_seeded;
    double bounded;
    int rnd;

    if (!rnd_seeded) {
        srand((unsigned)time(NULL) ^ (unsigned)getpid());
        rnd_seeded = 1;
    }
    rnd = rand();

    /* Like rand() % max, but uses the high-order bits for better
     randomness on architectures where rand() is implemented using a
     simple congruential generator.  */

    bounded = (double)max * rnd / (RAND_MAX + 1.0);

    return (int)bounded;
}

static double random_float(void)
{
  return (random_number(10000) / 10000.0
          + random_number(10000) / (10000.0 * 10000.0)
          + random_number(10000) / (10000.0 * 10000.0 * 10000.0)
          + random_number(10000) / (10000.0 * 10000.0 * 10000.0 * 10000.0));
}

static void http_sleep(double seconds)
{
  struct timeval sleep;

  sleep.tv_sec = (long)seconds;
  sleep.tv_usec = 1000000 * (seconds - (long) seconds);
  select(0, NULL, NULL, NULL, &sleep);
}

static void http_sleep_between_retrievals(int count)
{
    double  waitsecs;

    if (http_opt.wait) {
        if (!http_opt.random_wait || count > 1) {
            /* If random-wait is not specified, or if we are sleeping
             between retries of the same download, sleep the fixed
             interval.  */
            http_sleep(http_opt.wait);
        } else {
            /* Sleep a random amount of time averaging in opt.wait
             seconds.  The sleeping amount ranges from 0.5*opt.wait to
             1.5*opt.wait.  */
            waitsecs = (0.5 + random_float()) * http_opt.wait;
            HTTP_DPRINT("sleep_between_retrievals: avg=%f,sleep=%f", http_opt.wait, waitsecs);
            http_sleep(waitsecs);
        }
    }
}

/*
 * http_loop - 循环下载
 * @url: 下载地址url
 * @output_file: 输出文件
 * @ntry: 重试次数, 当为0表示不重试
 * @is_mgnt：是否是管理口
 *
 * 功能：循环下载文件
 *
 * 返回值：http_err_t
 */
static http_err_t http_get_loop(http_url_t *url, http_progress_t *progress, char *output_file, int ntry)
{
    int             count;
    char            *tms;
    http_err_t      ret;
    http_stat_t     hstat;        /* HTTP status */
    http_stat_t     *hs;          /* HTTP status */
    struct stat     st;
    http_util_t     *util;

    if (output_file == NULL) {
        output_file = url->filename;
    }

    ret = HTTP_OK;
    hs = &hstat;
    memset(hs, 0, sizeof(http_stat_t));
    hs->progress = progress;
    hs->local_file = strdup(output_file);
    if (hs->local_file == NULL) {
        HTTP_DPRINT("malloc for local_file fail");
        return HTTP_EMEMORY;
    }

    util = http_create_util();
    if (util == NULL) {
        ret = HTTP_EMEMORY;
        HTTP_DPRINT("malloc for util fail");
        goto exit;
    }
    http_change_progess(HTTP_STAT_BEGIN, hs, 0);

    /* 先发送HEAD报文将基本信息，如时间戳、文件大小拿到 */
    HTTP_DPRINT("local_file:%s", hs->local_file);
    count = 0;
    while(count <= ntry) {
        if (count != 0) {       /* 第一次下载不需要等待 */
            http_sleep_between_retrievals(count);
        }

        count++;

        /* Get the current time string */
        tms = datetime_str(time (NULL));
        HTTP_DPRINT("--%s--", tms);

        /* Decide whether or not to restart */
        if (stat(hs->local_file, &st) == 0 && S_ISREG(st.st_mode)) {
            /* When -c is used, continue from on-disk size.  (Can't use
             hstat.len even if count>1 because we don't want a failed
             first attempt to clobber existing data.)  */
            hs->restval = st.st_size;
            hs->orig_file_tstamp = st.st_mtime;
        } else {
            hs->restval = 0;
            hs->orig_file_tstamp = (time_t)-1;
        }

        ret = http_get(url, util, hs);
        switch (ret) {
        case HTTP_ERR:
        case HTTP_EOF:
        case HTTP_CONERROR:
        case HTTP_READERR:
        case HTTP_WRITEFAILED:
        case HTTP_RANGEERR:
            /* 非严重错误, 根据重试次数判断是否继续循环 */
            if (count <= ntry) {
                HTTP_DPRINT("Retrying.\n\n");
            } else {
                HTTP_DPRINT("Giving up.\n\n");
            }
            continue;
        case HTTP_FWRITEERR:
        case HTTP_FOPENERR:
            /* Another fatal error.  */
            HTTP_DPRINT("Cannot write to %s (%s).\n", hs->local_file, strerror (errno));
            goto exit;
        case HTTP_HDR:
        case HTTP_AUTHFAILED:
        case HTTP_NEWLOCATION:
            /* Fatal errors just return from the function.  */
            HTTP_DPRINT("Fatal errors just return from the function");
            goto exit;
        case HTTP_UNNEEDED:
            /* The file was already fully retrieved. */
            HTTP_DPRINT("The file was already fully retrieved.\n");
            goto exit;
        case HTTP_OK:
            /* Deal with you later.  */
            break;
        case HTTP_USER_CANCLE:
            /* 用户主动取消 */
            break;
        case HTTP_RETRY:
            /* 修订时间比较不一致，重新进行下载 */
            count--;
            continue;
        default:
            /* All possibilities should have been exhausted.  */
            HTTP_DPRINT("unknow");
            goto exit;
        }
        tms = datetime_str(time (NULL));
        HTTP_DPRINT("end download %d --%s--", ret, tms);

        break;
    }

exit:
    http_free_hstat(hs);
    http_free_util(util);

    return ret;
}

static http_err_t http_download(http_progress_t *progress, char *url, char *output_file, int nretry, bool is_mgnt)
{
    http_err_t  ret;
    http_url_t  *http_url;

    http_url = http_create_url(url, FALSE, &ret);
    if (http_url == NULL) {
        HTTP_DPRINT("http_create_url %d fail", ret);
        return ret;
    }

    ret = http_get_loop(http_url, progress, output_file, nretry);
    if (ret == HTTP_OK) {
        HTTP_DPRINT("download file [%s] success", url);
    } else if (ret == HTTP_UNNEEDED) {
        HTTP_DPRINT("the file [%s] has already exist!", url);
    } else {
        HTTP_DPRINT("download file [%s]fail %d", url, ret);
    }
    HTTP_FREE(http_url);

    return ret;
}

/**
 * http_download_file - 根据url地址下载文件
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 *
 * @return:
 *
 */
http_err_t http_download_file(char *url, char *output_file)
{
    return http_download(NULL, url, output_file, http_opt.ntry, FALSE);
}

/**
 * http_download_file_ntry
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 * @ntry: 重试次数, 当为0表示不重试
 */
http_err_t http_download_file_ntry(char *url, char *output_file, int ntry)
{
    return http_download(NULL, url, output_file, ntry, FALSE);
}

/**
 * http_download_file_progress - 支持显示进度
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 * @ntry: 重试次数, 当为0表示不重试
 */
http_err_t http_download_file_progress(http_progress_t *progress, char *url, char *output_file, int ntry)
{
    return http_download(progress, url, output_file, ntry, FALSE);
}

/**
 * http_mgnt_download_file_progress - 支持显示进度
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 * @ntry: 重试次数, 当为0表示不重试
 */
http_err_t http_mgnt_download_file_progress(http_progress_t *progress, char *url, char *output_file, int ntry)
{
    return http_download(progress, url, output_file, ntry, TRUE);
}

/**
 * http_mgnt_download_file - 使用管理口下载文件
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 *
 * @return:
 *
 */
http_err_t http_mgnt_download_file(char *url, char *output_file)
{
    return http_download(NULL, url, output_file, http_opt.ntry, TRUE);
}

http_err_t http_mngt_download_file_ntry(char *url, char *output_file, int ntry)
{
    return http_download(NULL, url, output_file, ntry, TRUE);
}

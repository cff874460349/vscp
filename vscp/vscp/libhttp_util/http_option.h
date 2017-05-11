/* http_option.h */

#ifndef _HTTP_OPTION_H_
#define _HTTP_OPTION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct http_options_s {
    int     ntry;                   /* 重试次数 */
    double  read_timeout;           /* read/write timeout. */
    double  dns_timeout;            /* DNS timeout. */
    double  connect_timeout;        /* connect timeout. */
    double  wait;                   /* The wait period between retrievals. */
    bool    random_wait;            /* 是否为随机等待 [0.5 - 1.5] * wait */
    bool    timestamping;           /* TRUE 使用时间戳比较,只要时间戳不相等，就会删除文件，重新下载 */
    bool    debug;                  /* TRUE 开启debug */
} http_options_t;

extern http_options_t http_opt;

#endif /* _HTTP_OPTION_H_ */

/*
 * Copyright (c) 2013-8-5 Ruijie Network. All rights reserved.
 */
/*
 * http_option.c
 * Original Author: yanggang@ruijie.com.cn, 2013-8-5
 * 
 * History
 * --------------------
 *
 *
 */
#include "http_option.h"

http_options_t http_opt = {
     5,  //int ntry;                 /* 重试次数 */
     5,  //double read_timeout;      /* read/write timeout. */
     5,  //double dns_timeout;       /* DNS timeout. */
     5,  //double connect_timeout;   /* connect timeout. */
     30, //double wait;              /* The wait period between retrievals. */
     FALSE,  //bool random_wait;     /* 是否为随机等待 [0.5 - 1.5] * wait */
     TRUE, //bool timestamping;      /* TRUE 使用时间戳比较,只要时间戳不相等，就会删除文件，重新下载 */
     FALSE //bool debug;             /* TRUE 开启debug */
};

void http_open_debug(void)
{
    http_opt.debug = TRUE;
}

void http_close_debug(void)
{
    http_opt.debug = FALSE;
}

void http_set_options(http_options_t *http_opt)
{
    memcpy(&http_opt, http_opt, sizeof(http_options_t));
}

/* http_include.h */

#ifndef _HTTP_INCLUDE_H_
#define _HTTP_INCLUDE_H_

#include <stdio.h>

typedef enum {
    HTTP_OK = 0,                /* OK 没有错误 */
    HTTP_EMEMORY,               /* 内存不足 */
    HTTP_EINVAL,                /* 错误的参数 */
    HTTP_HOSTERR,               /* host错误 */
    HTTP_CONERROR,              /* 连接失败 */
    HTTP_NEWLOCATION,           /* 重定向 */
    HTTP_URLERROR,              /* URL错误 */
    HTTP_FOPENERR,              /* 文件打开失败 */
    HTTP_FWRITEERR,             /* 文件写错误 */
    HTTP_EOF,                   /* 到达EOF */
    HTTP_ERR,                   /* http 错误 */
    HTTP_UNNEEDED,              /* 没有必要继续下载 */
    HTTP_READERR,               /* 读失败 */
    HTTP_RETRY,                 /* 重试一次 */
    HTTP_RANGEERR,              /* range 错误 */
    HTTP_AUTHFAILED,            /* 认证失败 */
    HTTP_WRITEFAILED,           /* 写失败 */
    HTTP_FILE_NOTEXIST,         /* 文件不存在 */
    HTTP_HDR,                   /* HTTP头出错 */
    HTTP_UPLOAD_FAIL,           /* HTTP上传失败 */
    HTTP_USER_CANCLE,           /* 用户主动取消 */
    HTTP_OTHER
} http_err_t;

struct http_progress_s;

/**
 * http_progress_notify_t - 接收一定报文后调用函数
 *
 * @len: 当前接收报文的大小,为0表示接收超时
 *
 * 接收一定报文后调用函数,用户可以在该接口中打印下载进度、控制是否要结束
 *
 * @return: 0  成功
 *          -1 用户主动取消下载
 */
typedef int (*http_progress_notify_t)(struct http_progress_s *progress, int len);

/* 下载进度信息 */
typedef struct http_progress_s {
    size_t       len;                /* 总共已经接收的长度 */
    size_t       file_len;           /* 文件总大小 */
    int          statcode;           /* 状态 */
    double       dltime;             /* time it took to download the data */
    http_progress_notify_t notify;
    void         *arg;               /* 用户参数 */
} http_progress_t;

/**
 * http_download_file - 根据url地址下载文件
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 *
 * 目前只支持http协议下载
 *
 * @return http_err_t
 */
extern http_err_t http_download_file(char *url, char *output_file);

/**
 * http_download_file_ntry
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 * @ntry: 重试次数, 当为0表示不重试
 */
extern http_err_t http_download_file_ntry(char *url, char *output_file, int ntry);

extern http_err_t http_download_file_progress(http_progress_t *progress, char *url,
    char *output_file, int ntry);

/**
 * http_mgnt_download_file - 使用管理口下载文件
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @output_file: 保存文件路径
 *
 * @return:
 *
 */
extern http_err_t http_mgnt_download_file(char *url, char *output_file);

extern http_err_t http_mngt_download_file_ntry(char *url, char *output_file, int ntry);

extern http_err_t http_mgnt_download_file_progress(http_progress_t *progress, char *url,
    char *output_file, int ntry);

/**
 * http_upload_file - 上传本地文件到指定服务器
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @upload_file: 指定需要上传的文件
 *
 * @return:
 */
extern http_err_t http_upload_file(char *url, char *upload_file);

/**
 * http_upload_file - 上传本地文件到指定服务器
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @upload_file: 指定需要上传的文件
 *
 * @return:
 */
extern http_err_t http_mgnt_upload_file(char *url, char *upload_file);

/*
 * http_open_debug - 开启debug
 */
extern void http_open_debug(void);

/*
 * http_open_debug - 关闭debug
 */
extern void http_close_debug(void);

#endif /* _HTTP_INCLUDE_H_ */

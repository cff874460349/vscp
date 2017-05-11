/*
 * Copyright (c) 2013-8-6 Ruijie Network. All rights reserved.
 */
/*
 * http_upload.c
 * Original Author: yanggang@ruijie.com.cn, 2013-8-6
 * 
 * History
 * --------------------
 *
 *
 */
#include "http_upload.h"
#include "http_util.h"

#define HTTP_POST_BOUNDARY_LEN          46                  /* boundary长度 包括 \0 */
#define HTTP_POST_B64_SIZE              64                  /* Base64转换表大小 */
#define HTTP_POST_ATTRIBUTE_LEN         512                 /* form表单中描述文件信息的部分 */
#define HTTP_POST_LAST_BOUND_LEN        ((HTTP_POST_BOUNDARY_LEN) + 6)
#define HTTP_POST_HEAD_LEN              1024                /* form表单头部 */


/* Base64编码转换表 */
static const char http_post_base64[HTTP_POST_B64_SIZE] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

/**
 * http_post_generate_boundary - 生成boundary边界参数
 * @buf:   缓冲区地址
 *
 * 无返回值
 */
static void http_post_generate_boundary(char *buf)
{
    const char *spec = "--------------------";
    int i, n;
    int rand;

    /* spec长度为20 根据snprintf的特性需要将size 写成 21 */
    n = snprintf(buf, 21, "%s", spec);
    for (i = n; i < HTTP_POST_BOUNDARY_LEN - 3; i++) {    /* 3是\r\n\0 字符空间 */
        rand = random_number(HTTP_POST_B64_SIZE);
        buf[i] = http_post_base64[rand];
    }
    buf[i] = '\0';
}

#if 0
/* 对sendfile的封装 */
static int post_file_data(int sock, int in_fd, size_t flen)
{
    int     ret;
    int     nwrite;
    off_t offset;

    offset = 0;
    nwrite = flen;
    while (nwrite > 0) {
        /* offset：作为输入时，是读的起始位置；作为输出时，值为offset+ret总和   */
        ret = sendfile(sock, in_fd, &offset, nwrite);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
                continue;
            }

            HTTP_DPRINT("sock %d<-->send error %s", sock, strerror(errno));

            return ret;
        }

        nwrite -= ret;
    }

    return HTTP_OK;
}
#else
/**
 * post_file_data - 发送文件数据部分
 * @post_sock: 套接字
 * @http_post_file_pos: 指向文件数据的指针
 * @flen: 发送字节大小
 *
 * 成功返回 0 错误返回非0
 */
static int post_file_data(int post_sock, int fd, size_t flen)
{
    int     result_len;
    int     send_len;
    char    head_data_buf[HTTP_POST_HEAD_LEN];

    memset(head_data_buf, 0, HTTP_POST_HEAD_LEN);

    /* 读取文件进行传输 */
    while (1) {
        result_len = read(fd, head_data_buf, HTTP_POST_HEAD_LEN);
        if (result_len < 0) {
            HTTP_DPRINT("fs_read error, errno=%d\n", errno);
            close(fd);
            return HTTP_READERR;
        } else if (result_len == 0) {
            close(fd);
            break;
        }

        send_len = send(post_sock, head_data_buf, result_len, 0);
        (void)memset(head_data_buf, 0, result_len);
        if (send_len != result_len) {
            HTTP_DPRINT("send error, errno=%d\n", errno);
            close(fd);
            return HTTP_WRITEFAILED;
        }
        /* 文件读取结束 */
        if (result_len < HTTP_POST_HEAD_LEN) {
            close(fd);
            break;
        }
    }

    return HTTP_OK;
}
#endif

static int http_post_upload_request_head(http_util_t *util , http_url_t *http_url,
        char *bound_buf, int hlen)
{
    int ret;
    const char *meth;
    char *path;
    char *host;
    int port;

    path = http_url->path;
    host = http_url->host;
    port = http_url->port;

    meth = "POST";

    HTTP_DPRINT("http_post_request success");

    sprintf(util->tmpbuf, "%s /%s HTTP/1.1", meth, (*path == '/' ? path + 1 : path));

    HTTP_DPRINT("http_post_request success");

    ret = http_post_header(util, util->tmpbuf, NULL);
    if (ret != HTTP_OK) {
        HTTP_DPRINT("errno:%s\r\n", strerror(errno));
        return ret;
    }

    HTTP_DPRINT("http_post_request success");

    if (port != 80) {
        sprintf(util->tmpbuf, "%s:%d", host, port);
    } else {
        strcpy(util->tmpbuf, host);
    }

    ret = http_post_header(util, "Host", util->tmpbuf);
    if (ret != HTTP_OK) {
        return ret;
    }

    //Content-Type:%s"%s%s
    sprintf(util->tmpbuf, "multipart/form-data; boundary=%s", bound_buf);
    ret = http_post_header(util, "Content-Type", util->tmpbuf);
    if (ret != HTTP_OK) {
        return ret;
    }

    if ((ret = http_post_header(util, "Accept", "*/*"))
        || (ret = http_post_header(util, "Accept-Language", "zh-cn"))
        || (ret = http_post_header(util, "User-Agent", "RG/Device 11.x"))) {
        return ret;
    }

    sprintf(util->tmpbuf, "%s", "sysreload=false; auth=YWRtaW46YWRtaW4%3D; currentURL=5.2");
    ret = http_post_header(util, "Cookie", util->tmpbuf);
    if (ret != HTTP_OK) {
        return ret;
    }

    /* 计算Content-Length */
    sprintf(util->tmpbuf, "%d", hlen);
    ret = http_post_header(util, "Content-Length", util->tmpbuf);
    if (ret != HTTP_OK) {
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

/* 文件属性信息(文件头) */
#define HTTP_POST_ATTRIBUTE(buf_com, bufsize_com, bound_buf, filename, file_num) \
    snprintf(buf_com, bufsize_com, \
        "--%s\r\n" \
        "Content-Disposition: form-data; " \
        "name=\"file%d\"; " \
        "filename=\"%s\"\r\n" \
        "Content-Type: application/octet-stream\r\n\r\n",\
        bound_buf, file_num, filename)

int http_post(http_url_t *http_url, http_util_t *util, char *upload_file)
{
    int ret;
    char *head;
    struct stat st;
    size_t file_size;
    int in_fd;
    char file_attribute_buf[HTTP_POST_ATTRIBUTE_LEN];
    char    bound_buf[HTTP_POST_BOUNDARY_LEN];
    char    last_bud_buf[HTTP_POST_LAST_BOUND_LEN];
    int hlen;       /* Content-Length */
    http_stat_t hstat;        /* HTTP status */
    http_stat_t *hs;

    hs = &hstat;
    memset(hs, 0, sizeof(http_stat_t));
    if (stat(upload_file, &st) != 0) {
        HTTP_DPRINT("upload_file:%s not exist\r\n", strerror(errno));
        return HTTP_FILE_NOTEXIST;
    }
    file_size = st.st_size;

    if (util->socket == INVALID_SOCKET) {
        HTTP_DPRINT("ip_addr:%s\r\n",
                http_sock_ntop((struct sockaddr *)&http_url->server_addr, sizeof(http_url->server_addr)));
        util->socket = http_create_socket((struct sockaddr *)&http_url->server_addr,
            sizeof(http_url->server_addr), FALSE, http_url->is_mngt);
        if (util->socket < 0) {
            HTTP_DPRINT("errno:%s\r\n", strerror(errno));
            return HTTP_CONERROR;
        }
    }
    HTTP_DPRINT("connect success util->socket:%d", util->socket);

    in_fd = open(upload_file, O_RDONLY);
    if (in_fd < 0) {
        return HTTP_FOPENERR;
    }

    hlen = 0;
    http_post_generate_boundary(bound_buf);
    HTTP_POST_ATTRIBUTE(file_attribute_buf, HTTP_POST_ATTRIBUTE_LEN, bound_buf,
            upload_file, 1);
    (void)snprintf(last_bud_buf, HTTP_POST_LAST_BOUND_LEN , "\r\n--%s--\r\n", bound_buf);

    hlen += st.st_size;
    hlen += strlen(file_attribute_buf);

    hlen += strlen(last_bud_buf);

    HTTP_DPRINT("begin send");
    /* 发送请求头 */
    if ((ret = http_post_upload_request_head(util, http_url, bound_buf, hlen))
        || (ret = http_fd_write(util->socket, file_attribute_buf, strlen(file_attribute_buf), -1))
        || (ret = post_file_data(util->socket, in_fd, file_size))
        || (ret = http_fd_write(util->socket, last_bud_buf, strlen(last_bud_buf), -1))
        ) {
        HTTP_DPRINT("Read error (%s) in headers.\n", strerror(errno));
        CLOSE_INVALIDATE(util->socket);
        close(in_fd);
        return ret;
    }
    close(in_fd);

    HTTP_DPRINT("begin recv");

    head = http_read_http_response_head(util->socket);
    if (head == NULL) {
        if (errno == 0) {
            HTTP_DPRINT("No data received.\n");
            CLOSE_INVALIDATE(util->socket);
            return HTTP_EOF;
        } else {
            HTTP_DPRINT("Read error (%s) in headers.\n", strerror(errno));
            CLOSE_INVALIDATE(util->socket);
            return HTTP_ERR;
        }
    }

    ret = http_parse_http_response_head(util, hs, head);
    if (ret != HTTP_OK) {
        HTTP_DPRINT("parse head error.\n");
        CLOSE_INVALIDATE(util->socket);
        HTTP_FREE(head);
        return ret;
    }
    HTTP_DPRINT("content_len:%ld status:%d.\n", (long)util->http_rsp.content_len,
        util->http_rsp.status);

    http_skip_short_body(util->socket, util->http_rsp.content_len);
    HTTP_FREE(head);
    CLOSE_INVALIDATE(util->socket);

    if (util->http_rsp.status != 200) {
        HTTP_DPRINT("status %d is not equal 200.\n", util->http_rsp.status);
        return HTTP_UPLOAD_FAIL;
    }

    HTTP_DPRINT("upload success.\n");

    return HTTP_OK;
}

static int http_post_loop(http_url_t *url, char *upload_file)
{
    int             ret;
    http_util_t     *util;

    util = http_create_util();
    if (util == NULL) {
        return HTTP_EMEMORY;
    }

    ret = http_post(url, util, upload_file);

    http_free_util(util);

    return ret;
}

static http_err_t http_upload(char *url, char *upload_file, bool is_mgnt)
{
    http_err_t  ret;
    http_url_t  *http_url;

    if (url == NULL || upload_file == NULL) {
        return HTTP_EINVAL;
    }

    http_url = http_create_url(url, is_mgnt, &ret);
    if (http_url == NULL) {
        HTTP_DPRINT("http_create_url fail");
        return ret;
    }
    ret = http_post_loop(http_url, upload_file);

    HTTP_FREE(http_url);

    return ret;
}

/**
 * http_upload_file - 上传本地文件到指定服务器
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @upload_file: 指定需要上传的文件
 *
 * @return:
 */
http_err_t http_upload_file(char *url, char *upload_file)
{
    return http_upload(url, upload_file, FALSE);
}

/**
 * http_upload_file - 上传本地文件到指定服务器
 *
 * @url: 指定url地址，如:http://10.10.10.195/switch.htm
 * @upload_file: 指定需要上传的文件
 *
 * @return:
 */
http_err_t http_mgnt_upload_file(char *url, char *upload_file)
{
    return http_upload(url, upload_file, TRUE);
}

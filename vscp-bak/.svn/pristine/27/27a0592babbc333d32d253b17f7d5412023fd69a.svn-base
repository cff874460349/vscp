/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "connection.h"
#include "vscp_debug.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* iop设备没有anl库，无法调用getaddrinfo对应的异步接口，采用alarm+sigsetjump机制伪线程会有问题 */
struct connection_info {
    char host[256];
    char port[16];
    struct addrinfo hints;
    struct addrinfo servinfo;   /* lock */

    bool is_resolved;         /* lock */
    pthread_t thread;           /* lock */
    pthread_mutex_t lock;   /* 多线程操作互斥 */
};

static struct connection_info *g_connection_info = NULL;

// from commandline.c
void output_buffer(FILE * stream, uint8_t * buffer, int length, int indent);

int create_socket(const char * portStr, int addressFamily)
{
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (0 != getaddrinfo(NULL, portStr, &hints, &res))
    {
        return -1;
    }

    for(p = res ; p != NULL && s == -1 ; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }

    freeaddrinfo(res);

    return s;
}

connection_t * connection_find(connection_t * connList,
                               struct sockaddr_storage * addr,
                               size_t addrLen)
{
    connection_t * connP;

    connP = connList;
    while (connP != NULL)
    {
        if ((connP->addrLen == addrLen)
         && (memcmp(&(connP->addr), addr, addrLen) == 0))
        {
            return connP;
        }
        connP = connP->next;
    }

    return connP;
}

connection_t * connection_new_incoming(connection_t * connList,
                                       int sock,
                                       struct sockaddr * addr,
                                       size_t addrLen)
{
    connection_t * connP;

    connP = (connection_t *)malloc(sizeof(connection_t));
    if (connP != NULL)
    {
        connP->sock = sock;
        memcpy(&(connP->addr), addr, addrLen);
        connP->addrLen = addrLen;
        connP->next = connList;
    }

    return connP;
}

//线程里面不要加vscp debug
void *connect_requests_thread(void *arg)
{
    struct addrinfo *servinfo = NULL;
    struct connection_info *info = (struct connection_info *)arg;

    if (0 != getaddrinfo(info->host, info->port, &info->hints, &servinfo) || servinfo == NULL) {
        pthread_mutex_lock(&info->lock);
        info->thread = 0;   /* 不管解析结果如何，执行完成后线程退出 */
        pthread_mutex_unlock(&info->lock);
    } else {
        pthread_mutex_lock(&info->lock);
        info->thread = 0;
        info->is_resolved = TRUE;
        memcpy(&info->servinfo, servinfo, sizeof(*servinfo));   //实际单节点
        pthread_mutex_unlock(&info->lock);
        free(servinfo);   /* 需要释放，注意 */
    }

    return NULL;
}

int connect_start_requests(char *host, char *port, struct addrinfo *phints)
{
    pthread_attr_t attr;

    int ret = -1;
    if (g_connection_info != NULL) {
        VSCP_DBG_ERR("connect_start_requests should not be call twice.\n");
        return ret;
    }
    g_connection_info = (struct connection_info *)malloc(sizeof(struct connection_info));
    if (g_connection_info == NULL) {
        VSCP_DBG_ERR("malloc failed\n");
        return ret;
    }
    memset(g_connection_info, 0, sizeof(struct connection_info));
    strcpy(g_connection_info->host, host);
    strcpy(g_connection_info->port, port);
    g_connection_info->hints.ai_family = phints->ai_family;
    g_connection_info->hints.ai_socktype = phints->ai_socktype;
    g_connection_info->is_resolved = FALSE;
    pthread_mutex_init(&g_connection_info->lock, NULL);


    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&g_connection_info->thread, &attr, connect_requests_thread,
        (void *)g_connection_info);
    if (ret != 0) {  /* failed */
        VSCP_DBG_ERR("connect_start_requests create thread failed, ret = %d\n\n", ret);
        pthread_mutex_unlock(&g_connection_info->lock);
        free(g_connection_info);
        g_connection_info = NULL;
    } else {
        VSCP_DBG_INIT("connect_start_requests success\n");
    }

    return ret;
}

int connect_cancel_requests(void)
{
    pthread_t thread;

    if (g_connection_info == NULL) {
        VSCP_DBG_ERR("g_connection_info = NULL\n");
        return -1;
    }
    //线程终止
    pthread_mutex_lock(&g_connection_info->lock);
    thread = g_connection_info->thread;
    pthread_mutex_unlock(&g_connection_info->lock);
    if (thread != 0) {  /* 线程还在，需要终止，主动vscp_uninit的时候会到该逻辑 */
        pthread_cancel(thread);
    }
    //锁销毁
    pthread_mutex_destroy(&g_connection_info->lock);
    //内存释放
    free(g_connection_info);
    g_connection_info = NULL;
    VSCP_DBG_INIT("connect_cancel_requests success\n");

    return 0;
}

//给vscp库调用，防止init/uninit操作导致内存泄漏与大量线程
void connection_cancle_host_resolve(void)
{
    if(g_connection_info != NULL) {
       (void)connect_cancel_requests();
    }
}

struct addrinfo *connection_get_requests_info(void)
{
    struct addrinfo *info = NULL;
    bool is_resolved;
    pthread_t thread;
    char host_ip[64];
    int ret = -1;

    if (g_connection_info == NULL) {
        VSCP_DBG_ERR("g_connection_info = NULL\n");
        return info;
    }
    pthread_mutex_lock(&g_connection_info->lock);
    is_resolved = g_connection_info->is_resolved;
    thread = g_connection_info->thread;
    pthread_mutex_unlock(&g_connection_info->lock);
    if (is_resolved == TRUE) {  //解析完成，并解析成功
        info = &g_connection_info->servinfo;    //已经解析完了，线程不存在了，不用加锁
        ret = getnameinfo(info->ai_addr, info->ai_addrlen, host_ip, sizeof(host_ip), NULL, 0, NI_NUMERICHOST);
        if (ret) {
            VSCP_DBG_ERR("getnameinfo() failed, error = %d\n", ret);
            (void)connect_cancel_requests();
            info = NULL;
        } else {
            VSCP_DBG_INIT("resolve host success:%s --> %s\n", g_connection_info->host, host_ip);
        }
    } else if (thread == 0) {   //解析完成，但解析失败，释放资源
        VSCP_DBG_ERR("resolve host %s complete, but failed, release g_connection_info\n",
            g_connection_info->host);
        (void)connect_cancel_requests();
    } else {
        VSCP_DBG_INFO("thread connect_requests_thread() is resolving....\n");
    }

    return info;
}

connection_t * connection_create(connection_t * connList,
                                 int sock,
                                 char * host,
                                 char * port,
                                 int addressFamily)
{
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int s;
    struct sockaddr *sa = NULL;
    socklen_t sl = 0;
    connection_t * connP = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;

    //if (0 != getaddrinfo(host, port, &hints, &servinfo) || servinfo == NULL) return NULL;
    //创建解析线程，调用getaddrinfo解析域名
    if (g_connection_info == NULL) {
        if (connect_start_requests(host, port, &hints) != 0) {
            return NULL;
        }
    }
    //查询
    servinfo = connection_get_requests_info();
    if (servinfo == NULL) {
        return NULL;
    }
    // we test the various addresses
    s = -1;
    for(p = servinfo ; p != NULL && s == -1 ; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            sa = p->ai_addr;
            sl = p->ai_addrlen;
            if (-1 == connect(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }
    if (s >= 0)
    {
        connP = connection_new_incoming(connList, sock, sa, sl);
        close(s);
    }
    if (NULL != servinfo) {
        //free(servinfo);
        VSCP_DBG_INIT("resolve success, release resource\n");
        connect_cancel_requests();
    }

    return connP;
}

void connection_free(connection_t * connList)
{
    while (connList != NULL)
    {
        connection_t * nextP;

        nextP = connList->next;
        free(connList);

        connList = nextP;
    }
}

int connection_send(connection_t *connP,
                    uint8_t * buffer,
                    size_t length)
{
    int nbSent;
    size_t offset;

#ifdef WITH_LOGS
    char s[INET6_ADDRSTRLEN];
    in_port_t port;

    s[0] = 0;

    if (AF_INET == connP->addr.sin6_family)
    {
        struct sockaddr_in *saddr = (struct sockaddr_in *)&connP->addr;
        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin_port;
    }
    else if (AF_INET6 == connP->addr.sin6_family)
    {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&connP->addr;
        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin6_port;
    }

    fprintf(stderr, "Sending %d bytes to [%s]:%hu\r\n", length, s, ntohs(port));

    output_buffer(stderr, buffer, length, 0);
#endif

    offset = 0;
    while (offset != length)
    {
        nbSent = sendto(connP->sock, buffer + offset, length - offset, 0, (struct sockaddr *)&(connP->addr), connP->addrLen);
        if (nbSent == -1) return -1;
        offset += nbSent;
    }
    return 0;
}

uint8_t lwm2m_buffer_send(void * sessionH,
                          uint8_t * buffer,
                          size_t length,
                          void * userdata)
{
    connection_t * connP = (connection_t*) sessionH;

    if (connP == NULL)
    {
        fprintf(stderr, "#> failed sending %u bytes, missing connection\r\n", (uint32_t)length);
        return COAP_500_INTERNAL_SERVER_ERROR ;
    }

    if (-1 == connection_send(connP, buffer, length))
    {
        fprintf(stderr, "#> failed sending %u bytes\r\n", (uint32_t)length);
        return COAP_500_INTERNAL_SERVER_ERROR ;
    }

    return COAP_NO_ERROR;
}

bool lwm2m_session_is_equal(void * session1,
                            void * session2,
                            void * userData)
{
    return (session1 == session2);
}

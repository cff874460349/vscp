#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stdlib.h>
#include <rg_thread.h>

#include "app_gps_server.h"
#include "vscp_debug.h"

#define APP_GPS_SRV_PATH            "/tmp/app_gps_server"
#define SOCKET_RCV_BUF_LEN          1024 * 4


app_gps_server_t app_gps_server;

static int app_gps_server_msg_deal(int socket)
{
    int                 rcv_len = 0;
    int                 ret;
    tag_msg_info_t      *info;
    vscp_client_t       *data;
    if (socket < 0) {
        return -1;
    }
    VSCP_DBG_INFO("enter\n");
    data = app_gps_server.data;
    info = (tag_msg_info_t *)app_gps_server.socket_rcv_buf;
    memset(info, 0, sizeof(tag_msg_info_t) + SOCKET_RCV_BUF_LEN);

    ret = read(socket, info->msg, SOCKET_RCV_BUF_LEN);
    if (ret < 0) {
        VSCP_DBG_ERR("app gps server read from socket %d error, ret = %d\n", socket, ret);
        close(socket);
        return -1;
    } else {
        rcv_len = (ret == 0 ? SOCKET_RCV_BUF_LEN : ret);
        info->msg_len = rcv_len;
        VSCP_DBG_INFO("app gps server read from socket %d, read bytes:%d\n", socket, rcv_len);
    }
    if (vscp_debug_info) {
        fprintf(stdout, "\n-----------app gps server read--------------\n");
        fprintf(stdout, "%s", (char *)info->msg);
        fprintf(stdout, "\n--------------------end---------------------\n");
    }
    close(socket);

    VSCP_DBG_INFO("write info to object rfid\n");
    ret = vscp_rfid_update_data(data, data->ins_array[0].mac, (uint8_t *)info, 1);
    if (ret) {
       VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
    }

    VSCP_DBG_INFO("leave\n");
    return 0;
}

int app_gps_server_accept(struct rg_thread *thread)
{
    struct sockaddr_un  client_addr;
    int                 client_sockfd;
    int                 len;
    app_gps_server_t    *server;
    vscp_client_t       *data;

    server = (app_gps_server_t *)thread->arg;
    data = server->data;
    len = sizeof(client_addr);

    server->thread = rg_thread_add_read(data->vscp_master, app_gps_server_accept, (void *)&app_gps_server,
        server->server_sockfd);
    if (app_gps_server.thread == NULL) {
        VSCP_DBG_ERR("create rg-thread app_gps_server_accept failed\n");
        close(server->server_sockfd);
        return -1;
    }

 
    client_sockfd = accept(app_gps_server.server_sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&len);

    (void)app_gps_server_msg_deal(client_sockfd);

    return 0;
}

int app_gps_server_init(vscp_client_t *data)
{
    struct sockaddr_un  server_addr;
    int                 server_sockfd;

    app_gps_server.socket_rcv_buf = malloc(sizeof(tag_msg_info_t) + SOCKET_RCV_BUF_LEN);
    if (app_gps_server.socket_rcv_buf == NULL) {
        VSCP_DBG_ERR("malloc %d(size) for socket receive buf error\n", SOCKET_RCV_BUF_LEN);
        return -1;
    }
    app_gps_server.data = data;

    unlink(APP_GPS_SRV_PATH);

    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, APP_GPS_SRV_PATH);

    bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(server_sockfd, 10);

    app_gps_server.server_sockfd = server_sockfd;
    app_gps_server.thread = rg_thread_add_read(data->vscp_master, app_gps_server_accept,
        &app_gps_server, server_sockfd);
    if (app_gps_server.thread == NULL) {
        VSCP_DBG_ERR("create rg-thread app_gps_server_accept failed\n");
        close(server_sockfd);
        return -1;
    }
    return 0;
}



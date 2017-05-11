#ifndef _APP_GPS_SERVER_H_
#define _APP_GPS_SERVER_H_

#include "vscp_client.h"

typedef struct app_gps_server_ {
    vscp_client_t *data;
    int server_sockfd;
    struct rg_thread *thread;
    char *socket_rcv_buf;
} app_gps_server_t;

extern app_gps_server_t app_gps_server;

extern int app_gps_server_init(vscp_client_t *data);

#endif  /* _APP_GPS_SERVER_H_ */

#include "vscp_client.h"
#include "vscp_debug.h"
#include "app_gps_server.h"
#include "rfid_types.h"

#include <string.h>
#include <stdlib.h>
#include <rg_thread.h>
#include <signal.h>


static int g_quit = 0;
uint8_t macaddr[MAC_ADDR_LEN] = {0x00, 0xd0, 0x00, 0x21, 0x00, 0x01};

void handle_sigint(int signum)
{
    g_quit = 1;
}


int rssi_start(vscp_client_t *data)
{
    struct rg_thread thread;


    signal(SIGINT, handle_sigint);


    //main loop
    while ((0 == g_quit) && (rg_thread_fetch(data->vscp_master, &thread)))
    {
        rg_thread_call (&thread);
    }

    VSCP_DBG_INFO("Leave rg-thread loop\r\n\n");

    return 0;
}


//./vscp_demo 56830
int  main(int argc, char **argv)
{
    vscp_client_t * context = NULL;
    int ret;
    char *basemac = "00-D0-00-21-00-00";

    context = (vscp_client_t *)malloc(sizeof(vscp_client_t));
    if (NULL == context) {
        VSCP_DBG_ERR("malloc vscp_client_t error\n");
        return -1;
    }

    memset(context, 0, sizeof(vscp_client_t));
    context->addressFamily = AF_INET;
    context->localPort = strdup("56831");
    context->localName = strdup(basemac);      // MAC地址,格式待定
    context->serverPort = strdup("5683");
    context->serverAddr = strdup("172.18.105.117");
    context->softwareVersion = strdup("BS_RGOS 11.0");
	context->hardwareType = strdup("BS V1.00");
	context->serialNumber = strdup("BS1234567890");

    context->ins_num = 1;
    context->ins_array[0].rfid_type = RFID_SEN_T_BASESTATION;
    context->ins_array[0].model = RFID_SEN_M_BASESTATION;
    memcpy(context->ins_array[0].mac, macaddr, MAC_ADDR_LEN);   


    context->vscp_master = rg_thread_master_create();
    if (context->vscp_master == NULL) {
        VSCP_DBG_ERR("Failed to create thread master.\n");
        goto error1;
    } else {
        VSCP_DBG_INFO("create rg-thread master success.\n");
    }

 
    (void)app_gps_server_init(context);

    ret = vscp_init(context);
    if (ret) {
        VSCP_DBG_ERR("init error\n");
        goto error2;
    }

    //开启服务,main loop,仅供参考，用户可以在这里加入自己的伪线程服务
	rssi_start(context);
    vscp_uninit(context);

    return 0;

error2:
    if (app_gps_server.thread) {
        RG_THREAD_READ_OFF(app_gps_server.thread);
    }
    free(app_gps_server.socket_rcv_buf);

    rg_thread_master_finish(context->vscp_master);
error1:
    free(context->localPort);
    free(context->localName);
    free(context->serverPort);
    free(context->serverAddr);
    free(context->softwareVersion);
    free(context->hardwareType);
    free(context->serialNumber);
    free(context);

    return -1;
}



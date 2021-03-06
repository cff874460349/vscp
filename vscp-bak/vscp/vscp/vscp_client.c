#include "vscp_client.h"
#include "vscp_debug.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <rg_thread.h>

#define MAX_PACKET_SIZE     1024
#define SERVER_DEMAIN_LEN   253     /* 来自于域名标准定义长度 */

extern lwm2m_object_t * get_object_device(vscp_client_t *data);
extern void free_object_device(lwm2m_object_t * objectP);
extern lwm2m_object_t * get_server_object(void);
extern void free_server_object(lwm2m_object_t * object);
extern lwm2m_object_t * get_security_object(vscp_client_t *data);
extern void free_security_object(lwm2m_object_t * objectP);
extern char * get_server_uri(lwm2m_object_t * objectP, uint16_t secObjInstID);
extern lwm2m_object_t * get_object_firmware(vscp_client_t *data);
extern void free_object_firmware(lwm2m_object_t * objectP);
extern lwm2m_object_t *rfid_object_get(vscp_client_t *data);
extern void rfid_object_free(vscp_client_t *data);
extern char *mac2str(uint8_t *macaddr, char *buf);
extern int vscp_debug_timer(struct rg_thread * thread);

/* 防止库调用者重复init或者重复执行uninit，1表示已初始化 */
static int g_vscp_init_flag = 0;
lwm2m_status_t g_vscp_connect_status = STATE_DEREGISTERED;

void * lwm2m_connect_server(uint16_t secObjInstID,
                            void * userData)
{
    vscp_client_t * dataP;
    char * uri;
    char * host;
    char * port;
    connection_t * newConnP = NULL;

    dataP = (vscp_client_t *)userData;

    uri = get_server_uri(dataP->securityObjP, secObjInstID);

    if (uri == NULL) return NULL;

    VSCP_DBG_INFO("Connecting to %s\r\n", uri);

    // parse uri in the form "coaps://[host]:[port]"
    if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
    {
        host = uri+strlen("coaps://");
    }
    else if (0 == strncmp(uri, "coap://", strlen("coap://")))
    {
        host = uri+strlen("coap://");
    }
    else
    {
        goto exit;
    }
    port = strrchr(host, ':');
    if (port == NULL) goto exit;
    // remove brackets
    if (host[0] == '[')
    {
        host++;
        if (*(port - 1) == ']')
        {
            *(port - 1) = 0;
        }
        else goto exit;
    }
    // split strings
    *port = 0;
    port++;

    newConnP = connection_create(dataP->connList, dataP->sock, host, port, dataP->addressFamily);
    if (newConnP == NULL) {
        VSCP_DBG_ERR("Connection creation failed.\r\n");
    }
    else {
        dataP->connList = newConnP;
    }

exit:
    lwm2m_free(uri);
    return (void *)newConnP;
}


void print_state(lwm2m_context_t * lwm2mH)
{
    lwm2m_server_t * targetP;

    VSCP_DBG_ERR("State: ");
    switch(lwm2mH->state)
    {
    case STATE_INITIAL:
        VSCP_DBG_ERR("STATE_INITIAL");
        break;
    case STATE_BOOTSTRAP_REQUIRED:
        VSCP_DBG_ERR("STATE_BOOTSTRAP_REQUIRED");
        break;
    case STATE_BOOTSTRAPPING:
        VSCP_DBG_ERR("STATE_BOOTSTRAPPING");
        break;
    case STATE_REGISTER_REQUIRED:
        VSCP_DBG_ERR("STATE_REGISTER_REQUIRED");
        break;
    case STATE_REGISTERING:
        VSCP_DBG_ERR("STATE_REGISTERING");
        break;
    case STATE_READY:
        VSCP_DBG_ERR("STATE_READY");
        break;
    default:
        VSCP_DBG_ERR("Unknown !");
        break;
    }
    VSCP_DBG_ERR("\r\n");

    targetP = lwm2mH->bootstrapServerList;

    if (lwm2mH->bootstrapServerList == NULL)
    {
        VSCP_DBG_ERR("No Bootstrap Server.\r\n");
    }
    else
    {
        VSCP_DBG_ERR("Bootstrap Servers:\r\n");
        for (targetP = lwm2mH->bootstrapServerList ; targetP != NULL ; targetP = targetP->next)
        {
            VSCP_DBG_ERR(" - Security Object ID %d", targetP->secObjInstID);
            VSCP_DBG_ERR("\tHold Off Time: %lu s", (unsigned long)targetP->lifetime);
            VSCP_DBG_ERR("\tstatus: ");
            switch(targetP->status)
            {
            case STATE_DEREGISTERED:
                VSCP_DBG_ERR("DEREGISTERED\r\n");
                break;
            case STATE_BS_HOLD_OFF:
                VSCP_DBG_ERR("CLIENT HOLD OFF\r\n");
                break;
            case STATE_BS_INITIATED:
                VSCP_DBG_ERR("BOOTSTRAP INITIATED\r\n");
                break;
            case STATE_BS_PENDING:
                VSCP_DBG_ERR("BOOTSTRAP PENDING\r\n");
                break;
            case STATE_BS_FINISHED:
                VSCP_DBG_ERR("BOOTSTRAP FINISHED\r\n");
                break;
            case STATE_BS_FAILED:
                VSCP_DBG_ERR("BOOTSTRAP FAILED\r\n");
                break;
            default:
                VSCP_DBG_ERR("INVALID (%d)\r\n", (int)targetP->status);
            }
            VSCP_DBG_ERR("\r\n");
        }
    }

    if (lwm2mH->serverList == NULL)
    {
        VSCP_DBG_ERR("No LWM2M Server.\r\n");
    }
    else
    {
        VSCP_DBG_ERR("LWM2M Servers:\r\n");
        for (targetP = lwm2mH->serverList ; targetP != NULL ; targetP = targetP->next)
        {
            VSCP_DBG_ERR(" - Server ID %d", targetP->shortID);
            VSCP_DBG_ERR("\tstatus: ");
            switch(targetP->status)
            {
            case STATE_DEREGISTERED:
                VSCP_DBG_ERR("DEREGISTERED\r\n");
                break;
            case STATE_REG_PENDING:
                VSCP_DBG_ERR("REGISTRATION PENDING\r\n");
                break;
            case STATE_REGISTERED:
                VSCP_DBG_ERR("REGISTERED\tlocation: \"%s\"\tLifetime: %lus\r\n", targetP->location, (unsigned long)targetP->lifetime);
                break;
            case STATE_REG_UPDATE_PENDING:
                VSCP_DBG_ERR("REGISTRATION UPDATE PENDING\r\n");
                break;
            case STATE_DEREG_PENDING:
                VSCP_DBG_ERR("DEREGISTRATION PENDING\r\n");
                break;
            case STATE_REG_FAILED:
                VSCP_DBG_ERR("REGISTRATION FAILED\r\n");
                break;
            default:
                VSCP_DBG_ERR("INVALID (%d)\r\n", (int)targetP->status);
            }
            VSCP_DBG_ERR("\r\n");
        }
    }
}

extern lwm2m_status_t registration_getStatus(lwm2m_context_t * contextP);
/* 加载lwm2m_step定时器 */
int vscp_add_lwm2m_step_timer(struct rg_thread * thread)
{
    vscp_client_t *data = (vscp_client_t *)thread->arg;
    struct timeval tv;
	lwm2m_status_t tmp_status;

    tv.tv_sec =2;   //初始时间2s
    tv.tv_usec = 0;
    lwm2m_step(data->lwm2mH, &(tv.tv_sec));
	tmp_status = registration_getStatus(data->lwm2mH);
	switch (tmp_status) {
	case STATE_REGISTERED:
		if (g_vscp_connect_status != STATE_REGISTERED) {
			VSCP_LOG("\nIOP %s connected. \n", data->lwm2mH->endpointName);
		}
		g_vscp_connect_status = STATE_REGISTERED;
		break;
	case STATE_REG_FAILED:
		if (g_vscp_connect_status != STATE_REG_FAILED) {
			VSCP_LOG("\nIOP %s connect failed. \n", data->lwm2mH->endpointName);
		}
		g_vscp_connect_status = STATE_REG_FAILED;
		break;
	case STATE_REG_PENDING:
		if (g_vscp_connect_status != STATE_REG_PENDING) {
			VSCP_LOG("\nIOP %s connecting. \n", data->lwm2mH->endpointName);
		}
		g_vscp_connect_status = STATE_REG_PENDING;
		break;
	default:
		VSCP_LOG("\nIOP %s connect status unknown. \n", data->lwm2mH->endpointName);
		break;
	}
    data->lwm2m_step_thread =
        rg_thread_add_timer_timeval(data->vscp_master, vscp_add_lwm2m_step_timer, (void *)data, tv);
    if (data->lwm2m_step_thread == NULL) {
        VSCP_DBG_ERR("vscp_add_lwm2m_step_timer add timer rg-thread failed\n");
        return -1;
    } else {
        return 0;
    }	
}

int vscp_thread_client(struct rg_thread *thread)
{
    vscp_client_t *data = (vscp_client_t *)thread->arg;
    lwm2m_context_t * lwm2mH = data->lwm2mH;
    uint8_t buffer[MAX_PACKET_SIZE];
    int numBytes;

    struct sockaddr_storage addr;
    socklen_t addrLen;

    addrLen = sizeof(addr);
    /*
    * We retrieve the data received
    */
    data->client_thread = rg_thread_add_read_high(data->vscp_master, vscp_thread_client,
                                (void *)data, data->sock);
    if (data->client_thread == NULL) {
        VSCP_DBG_ERR("Register socket read err\n");
    }

    numBytes = recvfrom(data->sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);
    if (0 > numBytes)
    {
       VSCP_DBG_ERR("Error in recvfrom(): %d %s\r\n", errno, strerror(errno));
    }
    else if (0 < numBytes)
    {
       connection_t * connP;

       connP = connection_find(data->connList, &addr, addrLen);
       if (connP != NULL)
       {
           /*
            * Let liblwm2m respond to the query depending on the context
            */
           lwm2m_handle_packet(lwm2mH, buffer, numBytes, connP);
       }
       else
       {
           /*
            * This packet comes from an unknown peer
            */
           VSCP_DBG_ERR("received bytes ignored!\r\n");
       }
    }

    return 0;
}

int vscp_init(vscp_client_t *data)
{
    lwm2m_context_t *lwm2mH = NULL;
    int i, result;
    struct timeval tv;

    if (data == NULL) {
        VSCP_DBG_ERR("vscp_init param data null\n");
        return -1;
    }
    /* 防止重复执行init */
    if (g_vscp_init_flag == 1) {
        VSCP_DBG_ERR("vscp_init can not be called twice\n");
        return -1;
    }

    if (NULL == data->localPort) data->localPort = strdup("56830");
    if (NULL == data->localName) data->localName = strdup("rg_vscp_client");  // MAC地址
    if (NULL == data->serverPort) data->serverPort = strdup("5683");
    if (NULL == data->serverAddr) data->serverAddr = strdup("localhost");

    if (strlen(data->serverAddr) > SERVER_DEMAIN_LEN) {
        VSCP_DBG_ERR("data->serverAddr:%s, outof range %d", data->serverAddr, SERVER_DEMAIN_LEN);
        return -1;
    }
    if (data->ins_num > RSSI_INSTANCE_MAX_NUM) {
        VSCP_DBG_ERR("rfid instance:%d, outof range %d", data->ins_num, RSSI_INSTANCE_MAX_NUM);
        return -1;
    }

    //配置初始化信息打印到文件
    VSCP_DBG_INIT("\ndata->localPort:\t\t%s\n"
                  "data->localName:\t\t%s\n"
                  "data->serverPort:\t\t%s\n"
                  "data->serverAddr:\t\t%s\n"
                  "data->softwareVersion:\t\t%s\n"
                  "data->hardwareType:\t\t%s\n"
                  "data->serialNumber:\t\t%s\n",
                  data->localPort, data->localName, data->serverPort, data->serverAddr,
                  data->softwareVersion, data->hardwareType, data->serialNumber);
    char strmac[32];
    for (i = 0; i < data->ins_num; i++) {
        VSCP_DBG_INIT("instance[%d]: type %d, model %d, mac %s\n", i, data->ins_array[i].rfid_type,
            data->ins_array[i].model, mac2str(data->ins_array[i].mac, strmac));
    }

    /*
     *This call an internal function that create an IPv6 socket on the port 5683.
     */
    VSCP_DBG_INIT("Trying to bind vSCP Client to port %s\r\n", data->localPort);
    data->sock = create_socket(data->localPort, data->addressFamily);
    if (data->sock < 0)
    {
        VSCP_DBG_ERR("Failed to open socket: %d %s\r\n", errno, strerror(errno));
        return -1;
    }
    VSCP_DBG_INIT("create socket success...\n");
    /*
     * Now the main function fill an array with each object, this list will be later passed to liblwm2m.
     * Those functions are located in their respective object file.
     */
    //security
    data->objArray[OBJ_INDEX_SECURITY] = get_security_object(data);
    if (NULL == data->objArray[OBJ_INDEX_SECURITY])   //回退
    {
        VSCP_DBG_ERR("Failed to create security object\r\n");
        return -1;
    }
    data->securityObjP = data->objArray[OBJ_INDEX_SECURITY];

    //server
    data->objArray[OBJ_INDEX_SERVER] = get_server_object();
    if (NULL == data->objArray[OBJ_INDEX_SERVER])
    {
        VSCP_DBG_ERR("Failed to create server object\r\n");
        return -1;
    }

    //device
    data->objArray[OBJ_INDEX_DEVICE] = get_object_device(data);
    if (NULL == data->objArray[OBJ_INDEX_DEVICE])
    {
        VSCP_DBG_ERR("Failed to create Device object\r\n");
        return -1;
    }

    //rssi
    data->objArray[OBJ_INDEX_RSSI] = rfid_object_get(data);
    if (NULL == data->objArray[OBJ_INDEX_RSSI]) {
        VSCP_DBG_ERR("Failed to create rssi object\r\n");
        return -1;
    }

    //firmware
    data->objArray[OBJ_INDEX_FIRMWARE] = get_object_firmware(data);
    if (NULL == data->objArray[OBJ_INDEX_FIRMWARE]) {
        VSCP_DBG_ERR("Failed to create Fireware object\r\n");
        return -1;
    }
    VSCP_DBG_INIT("create obj success...\n");

    /*
     * The liblwm2m library is now initialized with the functions that will be in
     * charge of communication
     */
    lwm2mH = data->lwm2mH = lwm2m_init(data);
    if (NULL == data->lwm2mH)
    {
        VSCP_DBG_ERR("lwm2m_init() failed\r\n");
        return -1;
    }
    VSCP_DBG_INIT("call  lwm2m_init success...\n");
    /*
     * We configure the liblwm2m library with the name of the client - which shall be unique for each client -
     * the number of objects we will be passing through and the objects array
     */
    result = lwm2m_configure(lwm2mH, data->localName, NULL, NULL, OBJ_COUNT, data->objArray); //OBJ_COUNT-1
    if (result != 0)
    {
        VSCP_DBG_ERR("lwm2m_configure() failed: 0x%X\r\n", result);
        return -1;
    }


    VSCP_DBG_INIT("call lwm2m_configure success...\n");

    //注册伪线程
    //加载lwm2m_step定时器
    tv.tv_sec =2;   //初始时间2s
    tv.tv_usec = 0;
    lwm2m_step(lwm2mH, &(tv.tv_sec));
    VSCP_DBG_INIT("call lwm2m_step success...\n");

    data->lwm2m_step_thread = rg_thread_add_timer_timeval(data->vscp_master, vscp_add_lwm2m_step_timer,
                                    (void *)data, tv);
    if (data->lwm2m_step_thread == NULL) {
        VSCP_DBG_ERR("vscp_start add timer rg-thread failed\n");
        return -1;
    }
    VSCP_DBG_INIT("create rg_timer success...\n");

    //注册socket读伪线程
    data->client_thread = rg_thread_add_read_high(data->vscp_master, vscp_thread_client,
                            (void *)data, data->sock);
    if (data->client_thread == NULL) {
        VSCP_DBG_ERR("Register socket read err\n");
        return -1;
    }
    VSCP_DBG_INIT("create rg_socket success...\n");

    RG_THREAD_TIMER_ON(data->vscp_master, data->debug_thread, vscp_debug_timer, (void *)data, 2);
    if (data->debug_thread == NULL) {
        VSCP_DBG_ERR("create dbg_timer error\n");
        return -1;
    }
    VSCP_DBG_INIT("create rg debug timer success...\n");

    VSCP_DBG_INIT("LWM2M Client \"%s\" started on port %s.\r\nUse Ctrl-C to exit.\r\n\n",
        data->localName, data->localPort);
    VSCP_DBG_INIT("init done\n");

    g_vscp_init_flag = 1;

    return 0;
}

int vscp_uninit(vscp_client_t * data)
{
    if (data == NULL) {
        VSCP_DBG_ERR("vscp_uninit param data null\n");
        return -1;
    }
    /* 防止重复执行uninit */
    if (g_vscp_init_flag == 0) {
        VSCP_DBG_ERR("vscp_unint must be called after vscp_init\n");
        return -1;
    }
    //如果没有解析成功，会一直有线程在后台执行解析，这里需要cancel和释放malloc资源
    connection_cancle_host_resolve();
    free_security_object(data->objArray[OBJ_INDEX_SECURITY]);
    free_server_object(data->objArray[OBJ_INDEX_SERVER]);
    free_object_device(data->objArray[OBJ_INDEX_DEVICE]);
    rfid_object_free(data);
    free_object_firmware(data->objArray[OBJ_INDEX_FIRMWARE]);

    lwm2m_close(data->lwm2mH);
    close(data->sock);
    RG_THREAD_TIMER_OFF(data->lwm2m_step_thread);
    RG_THREAD_READ_OFF(data->client_thread);
    RG_THREAD_TIMER_OFF(data->debug_thread);

    connection_free(data->connList);
    data->connList = NULL;

    g_vscp_init_flag = 0;

    return 0;
}

/**
 * purl:资源变化的obj的url，目前是"/10241"
 * 函数功能:通告核心库资源变化;终止lwm2m_step定时器；执行lwm2m_step；重新加载lwm2m_step定时器
 **/
void vscp_update_notify(vscp_client_t *data, char *purl)
{
    lwm2m_uri_t uri;
    struct timeval tv;

    if (data == NULL || purl == NULL) {
        VSCP_DBG_ERR("vscp_update_notify, param error\n");
    }

    VSCP_DBG_INFO("vscp_update_notify, resource:%s\n", purl);
    //触发通告，后续可删除，由server进行控制
    if (lwm2m_stringToUri(purl, strlen(purl), &uri)) {
        lwm2m_resource_value_changed(data->lwm2mH, &uri);
        //对象资源变化，终止step定时器，立即执行step
        if (data->lwm2m_step_thread) {
            RG_THREAD_TIMER_OFF (data->lwm2m_step_thread);
        }
        tv.tv_sec =2;   //初始时间2s
        tv.tv_usec = 0;
        lwm2m_step(data->lwm2mH, &(tv.tv_sec));
        //重新加载step定时器
        data->lwm2m_step_thread =
            rg_thread_add_timer_timeval(data->vscp_master, vscp_add_lwm2m_step_timer, (void *)data, tv);
        if (data->lwm2m_step_thread == NULL) {
            VSCP_DBG_ERR("vscp_update_notify add timer rg-thread failed\n");
        }
        VSCP_DBG_INFO("vscp_update_notify %s success...\n", purl);
    } else {
        VSCP_DBG_ERR("vscp_update_notify call lwm2m_stringToUri err,purl:%s\n", purl);
    }
}


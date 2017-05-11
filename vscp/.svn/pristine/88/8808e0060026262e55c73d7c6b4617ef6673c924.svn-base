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
 *    Julien Vermillard - initial implementation
 *    Fabien Fleutot - Please refer to git log
 *    David Navarro, Intel Corporation - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/

/*
 * This object is single instance only, and provide firmware upgrade functionality.
 * Object ID is 5.
 */

/*
 * resources:
 * 0 package                   write
 * 1 package url               write
 * 2 update                    exec
 * 3 state                     read
 * 4 update supported objects  read/write
 * 5 update result             read
 */

//-----------OMA-TS-LightweightM2M-V1_0-20160407-C.pdf------------
/*
 * state:
 * 0    idle            befor download or after update
 * 1    downloading
 * 2    downloaded
 * 3    updating
 *
 * state change:(update = download + upgrade)
 * init 0
 * 0---->1 update(download)
 * 1---->2 download success
 * 2---->3 update(upgrade)
 * 3---->2 update(upgrade fail) 感觉有问题，不管是否升级成功，均回到状态0
 * 3---->0 update(upgrade success)
 */

 /*
  * update result:
  * 0   download/update
  * 1   update success
  * 2   poor storage
  * 3   poor mem
  * 4   downloading interrupted
  * 5   CRC error
  * 6   unsupported package type
  * 7   invalid url
  * 8   update fail
  */

#include "liblwm2m.h"
#include "vscp_debug.h"
#include "vscp_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include <http_util.h>
#ifdef CONFIG_RGOS_UPGRADE
#include <upgrade.h>
#endif
#include <rg_thread.h>

// ---- private object "Firmware" specific defines ----
// Resource Id's:
#define RES_M_PACKAGE                   0
#define RES_M_PACKAGE_URI               1
#define RES_M_UPDATE                    2
#define RES_M_STATE                     3
#define RES_O_UPDATE_SUPPORTED_OBJECTS  4
#define RES_M_UPDATE_RESULT             5
#define RES_O_PKG_NAME                  6
#define RES_O_PKG_VERSION               7

#define PACKAGE_URL_LEN                  511
#define PACKAGE_STORE_PATH              "/tmp/vscp/"
#define PACKAGE_FILE_NAME               "/tmp/vscp/rg-upgrade.bin"

//升级过程中，如果断电或者重启，从文件中恢复升级结果
#define PACKAGE_UPGRADE_RST_PATH        "/data/vscp"
#define PACKAGE_UPGRADE_RST             "/data/vscp/upgrade_result"

typedef struct
{
    char url[PACKAGE_URL_LEN + 1];
    uint8_t state;          /* 升级状态机 */
    bool supported;
    uint8_t result;         /* 升级执行结果 */
    pthread_mutex_t lock;   /* 多线程操作互斥 */

    /* 有升级任务了，定时器查询升级结果，升级完成了通告并退出定时器 */
    struct rg_thread *thread_notify;
    vscp_client_t *data;
} firmware_data_t;

enum fw_status {
    FW_STATUS_IDLE = 0,
    FW_STATUS_DOWNLOADING,
    FW_STATUS_DOWNLOADED,
    FW_STATUS_UPDATING,
};

enum fw_err_code {
    FW_ERR_NO = 0,
    FW_ERR_DOWNLOADING,     //过渡状态，如果获取到的是该状态值，说明设备下载过程掉电
    FW_ERR_POOR_STORAGE,
    FW_ERR_POOR_MEM,
    FW_ERR_INTERRUPT,
    FW_ERR_CRC_ERR,
    FW_ERR_UNSUPPORT_TYPE,
    FW_ERR_INVALID_URL,
    FW_ERR_UPDATE_FAIL,
    FW_ERR_UPGRADING,         //过渡状态，如果获取的是该值，说明正在调用升级框架时掉电了
};
//写文件，注意不要被多线程竞争
static void prv_store_upgrade_result_to_file(int result)
{
    int fd;
    char ch_result;

    if ((fd = open(PACKAGE_UPGRADE_RST, (O_RDWR | O_CREAT), S_IRWXU | S_IRWXG)) < 0) {
        return;
    }

    ch_result = result + '0';
    write(fd, &ch_result, sizeof(ch_result));
    close(fd);

    return;
}

//读文件，主要不要被多线程竞争
static int prv_restore_upgrade_result_from_file(void)
{
    int ret, fd, result = FW_ERR_NO;
    char ch_result;

    if (access(PACKAGE_UPGRADE_RST_PATH , 0) != 0) {  //没有该文件，创建并保存初值0
        ret = mkdir(PACKAGE_UPGRADE_RST_PATH, S_IRWXU|S_IRWXG);
        if (ret < 0) {
            VSCP_DBG_ERR("create path %s failed\n", PACKAGE_UPGRADE_RST_PATH);
            result = FW_ERR_NO;
            goto out;
        }
    }
    if ((fd = open(PACKAGE_UPGRADE_RST, (O_RDWR | O_CREAT), S_IRWXU | S_IRWXG)) < 0) {
        VSCP_DBG_ERR("open file %s failed\n", PACKAGE_UPGRADE_RST);
        result = FW_ERR_NO;
        goto out;
    }
    ret = read(fd, &ch_result, sizeof(ch_result));
    if (ret <= 0) { //可能是新创建的空文件，读取的长度为0
        VSCP_DBG_ERR("read from file %s error\n", PACKAGE_UPGRADE_RST);
        result = FW_ERR_NO;
    } else {
        result = ch_result - '0';
    }
    close(fd);

out:
    VSCP_DBG_INIT("restore result value:%d\n", result);
    return result;
}

/* 根据url执行下载操作，采用多线程下载，返回值只是线程的返回结果 */
static int prv_fireware_download_package(firmware_data_t *userData)
{
    int ret, fw_code = FW_ERR_NO;

    if (access(PACKAGE_STORE_PATH , 0) != 0) {
        ret = mkdir(PACKAGE_STORE_PATH, S_IRWXU|S_IRWXG|S_IRWXO);
        if (ret == -1) {
            if (errno != EEXIST) {
                VSCP_DBG_ERR("create path %s failed\n", PACKAGE_STORE_PATH);
                return FW_ERR_POOR_STORAGE;
            }
        }
        VSCP_DBG_INFO("create path %s success!\n", PACKAGE_STORE_PATH);
    }
    //清除之前的下载文件
    if (access(PACKAGE_FILE_NAME, 0) == 0) {
        remove(PACKAGE_FILE_NAME);
    }
    VSCP_DBG_INFO("start download upgrade file from url:%s...\n", userData->url);
    ret = http_download_file(userData->url, PACKAGE_FILE_NAME);

    switch (ret) {
    case HTTP_OK:
    case HTTP_UNNEEDED:
        fw_code = FW_ERR_NO;
        break;
    case HTTP_EMEMORY:
        fw_code = FW_ERR_POOR_MEM;
        break;
    case HTTP_FOPENERR:
    case HTTP_FWRITEERR:
        fw_code = FW_ERR_POOR_STORAGE;
        break;
    case HTTP_USER_CANCLE:
    case HTTP_CONERROR:
    case HTTP_FILE_NOTEXIST:
        fw_code = FW_ERR_INTERRUPT;
        break;
    case HTTP_URLERROR:
        fw_code = FW_ERR_INVALID_URL;
        break;
    default:
        fw_code = FW_ERR_UPDATE_FAIL;
        break;
    } /* end switch */

    if (fw_code != FW_ERR_NO) {
        VSCP_DBG_ERR("download package from url [%s] failed, err code:%d\n", userData->url, ret);
    } else {
        VSCP_DBG_INFO("download package from url [%s] success\n", userData->url);
    }
    return fw_code;
}

/* 将下载的包升级 */
static int prv_fireware_upgrade(char *package)
{
/* 执行rgos升级框架升级安装包 */
#ifdef CONFIG_RGOS_UPGRADE
    int ret = pkg_mgmt_install_pkg(package, 1, NULL);  //1表示强制升级
    if (ret != 0) { /* 返回值不为0表示升级失败 */
        VSCP_DBG_ERR("call pkg_mgmt_install_pkg failed, package:%s, err=%d\n", package, ret);
        return FW_ERR_UPDATE_FAIL;
    }
#endif
    //sleep(100);
    return FW_ERR_NO;
}

void *prv_firmware_upgrade_thread(void *arg)
{
    int ret;
    firmware_data_t *userData = (firmware_data_t *)arg;

    //开始下载升级包
    ret = prv_fireware_download_package(userData);
    pthread_mutex_lock(&userData->lock);
    if (ret != 0) {
        userData->state = FW_STATUS_IDLE;
        userData->result = ret;
        pthread_mutex_unlock(&userData->lock);
        prv_store_upgrade_result_to_file(ret);
        VSCP_DBG_ERR("call prv_fireware_download_package failed, url:%s, err=%d\n", userData->url, ret);
        return NULL;
    }
    VSCP_DBG_INFO("start update fireware package...\n");
    userData->state = FW_STATUS_UPDATING;
    userData->result = FW_ERR_UPGRADING;

    pthread_mutex_unlock(&userData->lock);

    //开始升级版本
    prv_store_upgrade_result_to_file(FW_ERR_UPGRADING);
    ret = prv_fireware_upgrade(PACKAGE_FILE_NAME);
    pthread_mutex_lock(&userData->lock);
    userData->state = FW_STATUS_IDLE;
    userData->result = ret;
    pthread_mutex_unlock(&userData->lock);
    prv_store_upgrade_result_to_file(ret);
    VSCP_DBG_INFO("update fireware package complete, userData->result:%d\n", userData->result);

    return NULL;
}

int thread_notify_timer(struct rg_thread * thread)
{
    firmware_data_t *userData = (firmware_data_t *)thread->arg;

    userData->thread_notify = NULL;
    pthread_mutex_lock(&userData->lock);
    uint8_t status = userData->state;
    pthread_mutex_unlock(&userData->lock);
    if (status == FW_STATUS_IDLE) {    /* 升级完成，通告控制器 */
        vscp_update_notify(userData->data, "/5/0");
        return 0;
    }
    //没升级完，继续加载定时器等待
    RG_THREAD_TIMER_ON(userData->data->vscp_master, userData->thread_notify, thread_notify_timer,
        (void *)userData, 1);

    return 0;

}

static uint8_t prv_firmware_read(uint16_t instanceId,
                                 int * numDataP,
                                 lwm2m_data_t ** dataArrayP,
                                 lwm2m_object_t * objectP)
{
    int i;
    uint8_t result;
    firmware_data_t * userData = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_data_new(3);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 3;
        (*dataArrayP)[0].id = 3;
        (*dataArrayP)[1].id = 4;
        (*dataArrayP)[2].id = 5;
    }

    i = 0;
    do
    {
        switch ((*dataArrayP)[i].id)
        {
        case RES_M_PACKAGE:
        case RES_M_PACKAGE_URI:
        case RES_M_UPDATE:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        case RES_M_STATE:
            // firmware update state (int)
            pthread_mutex_lock(&userData->lock);
            lwm2m_data_encode_int(userData->state, *dataArrayP + i);
            pthread_mutex_unlock(&userData->lock);
            result = COAP_205_CONTENT;
            break;

        case RES_O_UPDATE_SUPPORTED_OBJECTS:
            pthread_mutex_lock(&userData->lock);
            lwm2m_data_encode_bool(userData->supported, *dataArrayP + i);
            pthread_mutex_unlock(&userData->lock);
            result = COAP_205_CONTENT;
            break;

        case RES_M_UPDATE_RESULT:
            pthread_mutex_lock(&userData->lock);
            lwm2m_data_encode_int(userData->result, *dataArrayP + i);
            pthread_mutex_unlock(&userData->lock);
            result = COAP_205_CONTENT;
            break;

        default:
            result = COAP_404_NOT_FOUND;
        }

        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_firmware_write(uint16_t instanceId,
                                  int numData,
                                  lwm2m_data_t * dataArray,
                                  lwm2m_object_t * objectP)
{
    int i;
    uint8_t result;
    firmware_data_t * userData = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    i = 0;

    do
    {
        switch (dataArray[i].id)
        {
        case RES_M_PACKAGE:
            // inline firmware binary
            result = COAP_204_CHANGED;
            break;

        case RES_M_PACKAGE_URI:
            if (dataArray[i].type != LWM2M_TYPE_STRING
                || dataArray[i].value.asBuffer.length <= 0
                || dataArray[i].value.asBuffer.length > PACKAGE_URL_LEN) {
                result = COAP_400_BAD_REQUEST;
            }
            memset(userData->url, 0, PACKAGE_URL_LEN + 1);
            memcpy(userData->url, dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
            result = COAP_204_CHANGED;
            break;

        case RES_O_UPDATE_SUPPORTED_OBJECTS:
            if (lwm2m_data_decode_bool(&dataArray[i], &userData->supported) == 1)
            {
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}



static uint8_t prv_firmware_execute(uint16_t instanceId,
                                    uint16_t resourceId,
                                    uint8_t * buffer,
                                    int length,
                                    lwm2m_object_t * objectP)
{
    uint8_t state;

    firmware_data_t * userData = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0) return COAP_400_BAD_REQUEST;

    // for execute callback, resId is always set.
    switch (resourceId)
    {
    case RES_M_UPDATE:
        pthread_mutex_lock(&userData->lock);
        state = userData->state;
        pthread_mutex_unlock(&userData->lock);
        if (state == FW_STATUS_IDLE)
        {
            int rv;
            pthread_t thread;

            VSCP_DBG_INFO("FIRMWARE UPDATE\n");
            VSCP_DBG_INFO("start downloading fireware package...\n");
            userData->state = FW_STATUS_DOWNLOADING; /* 无下载任务，无需加锁 */
            userData->result = FW_ERR_DOWNLOADING;
            prv_store_upgrade_result_to_file(FW_ERR_DOWNLOADING);
            //必须创建线程下载，主线程不能挂住
            rv = pthread_create(&thread, NULL, prv_firmware_upgrade_thread, (void *)userData);
            if (rv != 0) {  /* failed */
                return COAP_500_INTERNAL_SERVER_ERROR;
            } else {
                RG_THREAD_TIMER_ON(userData->data->vscp_master, userData->thread_notify,
                    thread_notify_timer, (void *)userData, 2);
                return COAP_204_CHANGED;
            }
        }
        else
        {
            // firmware update already running
            return COAP_400_BAD_REQUEST;
        }
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

void display_firmware_object(lwm2m_object_t * object)
{
#ifdef WITH_LOGS
    firmware_data_t * userData = (firmware_data_t *)object->userData;
    fprintf(stdout, "  /%u: Firmware object:\r\n", object->objID);
    if (NULL != userData)
    {
        fprintf(stdout, "    state: %u, supported: %s, result: %u\r\n",
                userData->state, userData->supported?"true":"false", userData->result);
    }
#endif
}

lwm2m_object_t * get_object_firmware(vscp_client_t *data)
{
    /*
     * The get_object_firmware function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * firmwareObj;

    firmwareObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != firmwareObj)
    {
        memset(firmwareObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns its unique ID
         * The 5 is the standard ID for the optional object "Object firmware".
         */
        firmwareObj->objID = LWM2M_FIRMWARE_UPDATE_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        firmwareObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != firmwareObj->instanceList)
        {
            memset(firmwareObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(firmwareObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        firmwareObj->readFunc    = prv_firmware_read;
        firmwareObj->writeFunc   = prv_firmware_write;
        firmwareObj->executeFunc = prv_firmware_execute;
        firmwareObj->userData    = lwm2m_malloc(sizeof(firmware_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != firmwareObj->userData)
        {
            ((firmware_data_t*)firmwareObj->userData)->state = FW_STATUS_IDLE;
            ((firmware_data_t*)firmwareObj->userData)->supported = false;
            ((firmware_data_t*)firmwareObj->userData)->result = prv_restore_upgrade_result_from_file();
            pthread_mutex_init(&((firmware_data_t*)firmwareObj->userData)->lock, NULL);
            ((firmware_data_t*)firmwareObj->userData)->thread_notify = NULL;
            ((firmware_data_t*)firmwareObj->userData)->data = data;
        }
        else
        {
            lwm2m_free(firmwareObj);
            firmwareObj = NULL;
        }
    }

    return firmwareObj;
}

void free_object_firmware(lwm2m_object_t * objectP)
{
    if (NULL != objectP->userData)
    {
        pthread_mutex_destroy(&((firmware_data_t*)objectP->userData)->lock);
        RG_THREAD_TIMER_OFF(((firmware_data_t*)objectP->userData)->thread_notify);
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    if (NULL != objectP->instanceList)
    {
        lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }
    lwm2m_free(objectP);
}


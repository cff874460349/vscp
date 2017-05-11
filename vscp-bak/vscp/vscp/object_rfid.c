/*
 * Implements an object for rssi info
 *
 *                 Multiple
 * Object |  ID   | Instances | Mandatoty |
 *  rssi  | 10241 |    Yes    |    No     |
 *
 *  Ressources:
 *              Supported    Multiple
 *  Name     | ID | Operations | Instances | Mandatory |  Type   | Range | Units |      Description      |
 *  rfid_type|  0 |    R       |    No     |    Yes    | Integer |       |       |                       |
 *  mac      |  2 |    R       |    No     |    Yes    | Opaque  |       |       |                       |
 *  reserve  |  1 |    R       |    No     |    Yes    | Integer |       |       |                       |
 *  info     |  3 |    R       |    Yes    |    No     | Opaque  |       |       |                       |
 *
 *  ble info
 *  |  0(1B)  | mac(6B)  | rssi(1B) |
 *  epc info
 *  |  1(1B)  | epc(12B) | rssi(4B) |
 *  |  2(1B)  | epc(32B) | rssi(4B) |
 *  JYee info
 *  |  3(1B)  | nil(0B)  | info(14B)|
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "vscp_client.h"
#include "vscp_debug.h"
#include "rfid_types.h"

#define INFO_RES_M_RFID_TYPE        0
#define INFO_RES_M_MODEL            1
#define INFO_RES_M_MAC              2
#define INFO_RES_M_INFOLIST         3

#define BATCH_SEND_BUF_LEN          (1024 * 2)
#define BATCH_SEND_MAX_BYTES        1024        /* �����ĵ���info��С��ÿ���������ݴ�С */

typedef struct _tag_info_
{
    struct _tag_info_ * next;   // matches lwm2m_list_t::next
    uint16_t    id;     /* �������Ͳ��ܸ� */

    uint16_t    info_len;
	uint8_t     info[0];   /* TODO �������Ƕ೤����೤������malloc */
} tag_info_t;

typedef struct _info_instance_
{
    struct _info_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t        id; /* �������Ͳ��ܸ� */
	tag_info_t *   info_list;

    uint8_t         rfid_type;
    uint8_t         model;
	uint8_t         mac[MAC_ADDR_LEN];   //ÿ��������оƬmac,����mac��ע��ʱ�ṩ��
    uint16_t        info_num;
} info_instance_t;

//���ٶȴ��������ݸ�ʽ�����ﶨ��Ŀ��ֻ��Ϊ��log��ӡ��Ҫ
typedef struct acce_data_s {
    short x_data;           // x/y/z��Χ+-200����short����
    short y_data;
    short z_data;
    short coefficient;      // �Ŵ��ϵ����100������uchar����short����
} acce_data_t;

char *mac2str(uint8_t *macaddr, char *buf)
{
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", macaddr[0],macaddr[1],macaddr[2],macaddr[3],
        macaddr[4],macaddr[5]);

    return buf;
}

static int vscp_rfid_write_tag_def(uint8_t *buf, int buf_len)
{
    int i;
    if (vscp_debug_info != 0) {
        fprintf(stdout, "\n----------receive from controller -------------\n");
        fprintf(stdout, "data:\n");
        for (i = 0; i < buf_len; i++) {
            fprintf(stdout, "%02x ", buf[i]);
        }
        fprintf(stdout, "\n-------------END----------------\n");
    }

    return 0;
}

rfid_write_tag_info_t g_rfid_write_tag_func = vscp_rfid_write_tag_def;

/* ע��дtag���ݽӿ� */
int vscp_rfid_register_write_func(rfid_write_tag_info_t func)
{
    if (func == NULL) {
        VSCP_DBG_ERR("func err NULL\n");
        return -1;
    }
    g_rfid_write_tag_func = func;

    return 0;
}

//ͨ��mac����info_instancet_t�ṹ��
static info_instance_t *rfid_find_instance(lwm2m_object_t *infoObj, uint32_t ins_num, uint8_t macaddr[MAC_ADDR_LEN])
{
    int i;
    info_instance_t *infoP = NULL;

    for (i = 0; i < ins_num; i++) {
        infoP = (info_instance_t *)LWM2M_LIST_FIND(infoObj->instanceList, i);
        if (infoP == NULL) {
            continue;
         } else if (memcmp(infoP->mac, macaddr, MAC_ADDR_LEN) == 0) {   //�ҵ���
         	return infoP;
         }
    }
    return NULL;
}

void static rfid_format_print_data(info_instance_t * infoP, tag_info_t * rssiP)
{
    int i;

    fprintf(stdout, "type:%d, model:%d, info_len:%d\n", infoP->rfid_type,
                            infoP->model, rssiP->info_len);
    //խ����վgps���ݣ�json�ַ���
    if (infoP->rfid_type == RFID_SEN_T_BASESTATION && infoP->model == RFID_SEN_M_BASESTATION) {
        fprintf(stdout, "%s\n", (char *)rssiP->info);
    } else if (infoP->rfid_type == RFID_SEN_T_ACCE && infoP->model == RFID_SEN_M_ACCE) {
        /* ���ٶȴ�������Ŀ */
        acce_data_t *acce_data = (acce_data_t *)rssiP->info;
        fprintf(stdout, "x_data:%d, y_data:%d, z_data:%d, coefficient:%d\n", acce_data->x_data,
            acce_data->y_data, acce_data->z_data, acce_data->coefficient);
    } else {
        /* �������16�����ֽڸ�ʽ��ӡ */
        for (i = 0; i < rssiP->info_len; i++) {
            fprintf(stdout, "%02x ", rssiP->info[i]);
        }
    }
    fprintf(stdout, "\n");

}

//read/observe���ķ�װ
static uint8_t rfid_object_read(uint16_t instanceId,
                        int * numDataP,
                        lwm2m_data_t ** dataArrayP,
                        lwm2m_object_t * objectP)
{
    info_instance_t * infoP;
    int i;
    //fprintf(stdout ,"enter rfid_object_read, instanceId:%d, numDataP:%d\n", instanceId, *numDataP);
    infoP = (info_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, instanceId);
    if (NULL == infoP) {
        fprintf(stdout ,"enter rfid_object_read (NULL == infoP)\n");
        return COAP_404_NOT_FOUND;
    }
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
                INFO_RES_M_RFID_TYPE,
                INFO_RES_M_MODEL,
                INFO_RES_M_MAC,
                INFO_RES_M_INFOLIST
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    for (i = 0 ; i < *numDataP ; i++)
    {
        switch ((*dataArrayP)[i].id)
        {
        case INFO_RES_M_RFID_TYPE:
            lwm2m_data_encode_int(infoP->rfid_type, *dataArrayP + i);
            break;
        case INFO_RES_M_MODEL:
            lwm2m_data_encode_int(infoP->model, *dataArrayP + i);
            break;
        case INFO_RES_M_MAC:
            lwm2m_data_encode_opaque(infoP->mac, MAC_ADDR_LEN, *dataArrayP + i);
            break;
        case INFO_RES_M_INFOLIST:
        {
            //VSCP_DBG_ERR("enter rfid_object_read INFO_RES_M_INFOLIST");
            tag_info_t * rssiP;

            int ri, riCnt = infoP->info_num;   // reduced to 1 instance to fit in one block size

            //VSCP_DBG_ERR("instanceID[%d] updated, do read\n", instanceId);

            lwm2m_data_t *subTlvP = lwm2m_data_new(riCnt);
            for(ri = 0; ri < riCnt; ri++) {
                rssiP = (tag_info_t *)LWM2M_LIST_FIND(infoP->info_list, ri);
                if (rssiP) {
                    if(vscp_debug_info) {
                        VSCP_DBG_INFO("READ tag msg %d:\n", ri);
                        rfid_format_print_data(infoP, rssiP);
                    }
                   subTlvP[ri].id = ri;
                   lwm2m_data_encode_opaque((uint8_t *)rssiP->info, rssiP->info_len, subTlvP + ri);
                }
            }
            lwm2m_data_encode_instances(subTlvP, riCnt, *dataArrayP + i);
            break;
        }

        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

static uint8_t rfid_object_write(uint16_t instanceId,
                                  int numData,
                                  lwm2m_data_t * dataArray,
                                  lwm2m_object_t * objectP)
{
    int i;
    uint8_t result;
    info_instance_t *infoP = (info_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, instanceId);

    i = 0;
    if (infoP == NULL) {
        result = COAP_400_BAD_REQUEST;
        goto out;
    }
    do
    {
    
        switch (dataArray[i].id)
        {
        case INFO_RES_M_INFOLIST:
            if ((dataArray[i].type != LWM2M_TYPE_OPAQUE)
                || (dataArray[i].value.asBuffer.length <= 0)
                || (dataArray[i].value.asBuffer.buffer == NULL)) {
                result = COAP_400_BAD_REQUEST;
            }
            int data_len = (int)dataArray[i].value.asBuffer.length;
            uint8_t *data = malloc(data_len);
            if (data == NULL) {
                result = COAP_500_INTERNAL_SERVER_ERROR;
            } else {
                memcpy(data, dataArray[i].value.asBuffer.buffer, data_len);
                //for debug
                if (g_rfid_write_tag_func != vscp_rfid_write_tag_def) {
                    (void)vscp_rfid_write_tag_def(data, data_len);
                }
                if (g_rfid_write_tag_func(data, data_len) != 0) {
                    result = COAP_500_INTERNAL_SERVER_ERROR;
                } else {
                    result = COAP_204_CHANGED;
                }
                free(data);
            }
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);
out:
    return result;
}

//obj����
lwm2m_object_t *rfid_object_get(vscp_client_t *data)
{
    lwm2m_object_t * infoObj;

    infoObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != infoObj)
    {
        int i;
        info_instance_t * infoP;
        tag_info_t *rssiP;

        memset(infoObj, 0, sizeof(lwm2m_object_t));

        infoObj->objID = 10241;     //ͳһ����id��ҽ�����������ݶ��󣬹̶�ֵ������10241
        infoObj->readFunc = rfid_object_read;
        infoObj->writeFunc = rfid_object_write;

        infoP = (info_instance_t *)lwm2m_malloc(sizeof(info_instance_t) * data->ins_num);
        if (infoP == NULL) {
            lwm2m_free(infoObj);
            return NULL;
        }
        for (i=0 ; i < data->ins_num; i++, infoP++)
        {
            memset(infoP, 0, sizeof(info_instance_t));
            infoP->id = i;  /* �������i,��Ҫ�� */
            infoP->rfid_type = data->ins_array[i].rfid_type;
            infoP->model = data->ins_array[i].model;
            memcpy(infoP->mac, data->ins_array[i].mac, MAC_ADDR_LEN);

            infoP->info_num = 1;    /* ���ڳ�ʼ������û�б�ǩ���ݣ�Ĭ�ϴ���һ�����ֽڵ�ȫFF */
            //����tag_info_t��Ŀ���Ƿ�ֹ����������read����Դʱ���¿ͻ��˽���coredump
            rssiP = (tag_info_t *)lwm2m_malloc(sizeof(tag_info_t *) + sizeof(int));
            if (rssiP == NULL) {
                //����ʧ�ܽ���ӡerr��ʾ�������ع�����
                VSCP_DBG_ERR("instance[%d] malloc defaule tag_info_t struct failed\n", i);
            } else {
                memset(rssiP, 0, sizeof(tag_info_t *) + sizeof(int));
                rssiP->id = 0;
                rssiP->info_len = sizeof(int);
                memset(rssiP->info, 0xFF, sizeof(int));
                infoP->info_list = (tag_info_t *)LWM2M_LIST_ADD(infoP->info_list, rssiP);
            }

            infoObj->instanceList = LWM2M_LIST_ADD(infoObj->instanceList, infoP);
        }
    }

    return infoObj;
}

void rfid_object_free(vscp_client_t *data)
{
    uint32_t i;
    info_instance_t * infoP;
    lwm2m_object_t * object = data->objArray[OBJ_INDEX_RSSI];
    if (object == NULL) {
        return;
    }
    //����ձ�ǩ��Ϣ
    for (i = 0; i < data->ins_num; i++) {
        infoP = (info_instance_t *)LWM2M_LIST_FIND(object->instanceList, i);
        if (infoP) {
            LWM2M_LIST_FREE(infoP->info_list);
        }
    }
    //�����info_instance_t����Ϊ���Կ�����ģ������Կ��ͷţ���0������׵�ַ
    infoP = (info_instance_t *)LWM2M_LIST_FIND(object->instanceList, 0);
    if (infoP) {
        lwm2m_free(infoP);
    }
    if (object->userData != NULL)
    {
        lwm2m_free(object->userData);
        object->userData = NULL;
    }
    lwm2m_free(object);
}

//ִ����ȷ����instance id,ʧ�ܷ���-1
static int rfid_update_data_(vscp_client_t *data, uint8_t macaddr[MAC_ADDR_LEN], uint8_t *buf, int msg_num)
{
    lwm2m_object_t *infoObj = data->objArray[OBJ_INDEX_RSSI];
    info_instance_t *infoP;
    tag_info_t   *rssiP;
    tag_msg_info_t *p;
    int j;

    if (infoObj == NULL) {
        VSCP_DBG_ERR("(infoObj == NULL)\n");
        return -1;
    }
    infoP = rfid_find_instance(infoObj, data->ins_num, macaddr);
    if (NULL == infoP) {
        char strmac[32];
        VSCP_DBG_ERR("rssi_update_data instance_ %s not exist!\n", mac2str(macaddr, strmac));
        return -1;
    }

    //����ո���instance�ı�ǩ��Ϣinfo
    LWM2M_LIST_FREE(infoP->info_list);
    infoP->info_list = NULL;

    /* ���¸�instance�����ǩ��ֵ */
    infoP->info_num = msg_num;   /* TODO ��ǩ���� */
    p = (tag_msg_info_t *)buf;
    for (j = 0; j < infoP->info_num; j++) {
        rssiP = (tag_info_t *)lwm2m_malloc(sizeof(tag_info_t) + p->msg_len);
        if (NULL == rssiP) {
            VSCP_DBG_ERR("rssi_update_data_ malloc instance[%d] info[%d] error\n", infoP->id, j);
            LWM2M_LIST_FREE(infoP->info_list);
            infoP->info_list = NULL;
            return -1;
        }
        memset(rssiP, 0, sizeof(tag_info_t) + p->msg_len);
        rssiP->id=j;
        rssiP->info_len = p->msg_len;
        memcpy(rssiP->info, p->msg, p->msg_len);

        if (vscp_debug_info) {
            VSCP_DBG_INFO("STORE tag msg(%d) to instance[%d]:\n", j, infoP->id);
            rfid_format_print_data(infoP, rssiP);
        }
        infoP->info_list = (tag_info_t *)LWM2M_LIST_ADD(infoP->info_list, rssiP);
        p = (tag_msg_info_t *)(p->msg + p->msg_len);
    }
    //VSCP_DBG_ERR("instance [%d] changes\n", infoP->id);
    return infoP->id;
}

//ִ�гɹ�����0��ִ��ʧ�ܷ���-1
int vscp_rfid_update_data(vscp_client_t *data, uint8_t macaddr[MAC_ADDR_LEN], uint8_t *buf, int msg_num)
{
    char    url[16];
    int instanceID;

    VSCP_DBG_INFO("enter\n");
    if (data == NULL || buf == NULL || msg_num <=0) {
        VSCP_DBG_ERR("rssi_update_data param error!\n");
        return -1;
    }

	VSCP_DBG_INFO("updata mac : %x-%x-%x-%x-%x-%x----------------\n", 
            macaddr[0],macaddr[1],macaddr[2],macaddr[3],
            macaddr[4],macaddr[5]);
    instanceID = rfid_update_data_(data, macaddr, buf, msg_num);
    if (instanceID != -1) {
        sprintf(url, "/10241/%d", instanceID);
        vscp_update_notify(data, url);
        VSCP_DBG_INFO("update success & leave\n");
        return 0;
    } else {
        VSCP_DBG_ERR("vscp_rfid_update_data exec error!\n");
        return -1;
    }
}

//ִ�гɹ�����0��ִ��ʧ�ܷ���-1
int vscp_iot_update_data(vscp_client_t *data, uint8_t macaddr[MAC_ADDR_LEN], uint8_t *buf, int msg_num)
{
    int i;
    tag_msg_info_t *p, *batch_bufp;
    uint8_t *batch_buf;
    int batch_send_bytes = 0;
    int batch_send_nums = 0;

    VSCP_DBG_INFO("enter\n");
    if (data == NULL || buf == NULL || msg_num <=0) {
        VSCP_DBG_ERR("rssi_update_data param error!\n");
        return -1;
    }

    //�����Ϣ��������Ϣ���Ȳ�Ӧ�ô���1k
    p = (tag_msg_info_t *)buf;
    for (i = 0; i < msg_num; i++) {
        if (p->msg_len > BATCH_SEND_MAX_BYTES) {
            VSCP_DBG_ERR("msg(%d) len(%d) > 1024\n", i, p->msg_len);
            return -1;
        }
        p = (tag_msg_info_t *)(p->msg + p->msg_len);
    }

    batch_buf = malloc(BATCH_SEND_BUF_LEN);
    if (batch_buf == NULL) {
        VSCP_DBG_ERR("malloc error\n");
        return -1;
    }

    memset(batch_buf, 0, BATCH_SEND_BUF_LEN);
    p = (tag_msg_info_t *)buf;
    batch_bufp = (tag_msg_info_t *)batch_buf;
    for (i = 0; i < msg_num; i++) {
        if (batch_send_bytes + p->msg_len > BATCH_SEND_MAX_BYTES) {
            (void)vscp_rfid_update_data(data, macaddr, batch_buf, batch_send_nums);
            batch_send_bytes = 0;
            batch_send_nums = 0;
            batch_bufp = (tag_msg_info_t *)batch_buf;
            memset(batch_buf, 0 , BATCH_SEND_BUF_LEN);
        }
        batch_bufp->msg_len = p->msg_len;
        memcpy(batch_bufp->msg, p->msg, p->msg_len);
        batch_send_bytes += p->msg_len;
        batch_send_nums += 1;
        p = (tag_msg_info_t *)(p->msg + p->msg_len);
        batch_bufp = (tag_msg_info_t *)(batch_bufp->msg + batch_bufp->msg_len);
    }
    if (batch_send_nums) {
       (void)vscp_rfid_update_data(data, macaddr, batch_buf, batch_send_nums);
    }

    free(batch_buf);
    return 0;
}


#ifndef _VSCP_CLIENT_H_
#define _VSCP_CLIENT_H_

#include "liblwm2m.h"
#include "connection.h"

#define RSSI_INSTANCE_MAX_NUM           20   /* TODOʵ�ʸ����û����� */
#define MAC_ADDR_LEN                    6

typedef struct _vscp_client_ vscp_client_t;

//#define OBJ_COUNT 5
enum OBJ_INDEX {
    OBJ_INDEX_SECURITY = 0,
    OBJ_INDEX_SERVER,
    OBJ_INDEX_DEVICE,
    OBJ_INDEX_RSSI,
    OBJ_INDEX_FIRMWARE,
    OBJ_COUNT
};

typedef struct rfid_instance {
    uint8_t         rfid_type;
    uint8_t         model;
	uint8_t         mac[MAC_ADDR_LEN];   //ÿ��������оƬmac,����mac��ע��ʱ�ṩ��
} rfid_instance_t;

typedef struct tag_msg_info {
    uint16_t msg_len;
    uint8_t msg[0];
} tag_msg_info_t;

struct _vscp_client_
{
    lwm2m_object_t * securityObjP;
    int sock;
    connection_t * connList;
    int addressFamily;
	char *localPort;    /* ���ط������˿ںţ�Ĭ��Ϊ56830 */
    char *localName;    /* �豸���ƣ������Ա�����mac��ַ���mac��ַ��ʽΪXX-XX-XX-XX-XX-XX(��д) */
	char *serverPort;   /* �������˿ںţ�Ĭ��Ϊ5683 */
    char *serverAddr;   /* ��������ip��ַ����������Ĭ��Ϊlocalhost��Ŀǰ����������Ϊiotc.ruijie.com.cn */
    char *softwareVersion;  /* �豸����汾�� */
    char *hardwareType; /* �豸Ӳ������ */
    char *serialNumber; /* �豸���к� */
    lwm2m_object_t * objArray[OBJ_COUNT];
    lwm2m_context_t * lwm2mH;
    struct rg_thread_master *vscp_master;   /* �û������䴴����α�̵߳����� */
    /* ����lwm2m_step��ʱ��α�̣߳�vscp_update_notifyִ�� cancelʱ���õ� */
    struct rg_thread *lwm2m_step_thread;
    struct rg_thread *client_thread;
    struct rg_thread *debug_thread;

    //��������ʼ����Ϣ
    uint32_t ins_num;   /* �����������������ݶ�Ϊ20 */
    rfid_instance_t ins_array[RSSI_INSTANCE_MAX_NUM];  /* ������������Ϣ��mac/type����Ϣ */
} ;

#define MAX_TAG_NUM 10000
#define MAX_KYEE_DATA_TYPE 3

enum {
	KYEE_DATA_NONE,
	KYEE_DATA_HB,
	KYEE_DATA_OFF,
	KYEE_DATA_OUT
};

typedef struct {
	bool enable;
	int times;
	int times_limit;
	int inter;
} kyee_data_t;

typedef struct {
	int weight;
	int e2;
	float weight_value;
	float rate;
} ladrip_data_t;

typedef struct {
	bool enable;
	int inter;
	int times;
} ewell_data_t;

typedef struct {
	bool enable;
	int tag_num;
	kyee_data_t kyee_data_hb;
	kyee_data_t kyee_data_off;
	kyee_data_t kyee_data_out;
	char sensor_mac[MAC_ADDR_LEN];
	char tag_mac[MAX_TAG_NUM][MAC_ADDR_LEN];
} config_kyee_t;

typedef struct {
	bool enable;
	int tag_num;
	ladrip_data_t ladrip_data;
	char sensor_mac[MAC_ADDR_LEN];
	char tag_mac[MAX_TAG_NUM][MAC_ADDR_LEN];
	char tag_mac_connected[MAX_TAG_NUM];
} config_ladrip_t;

#define WELL_MAC_LEN 8
#define WELL_ID_LEN 4
#define WELL_SN_LEN 16
typedef struct {
	bool enable;
	int tag_num;
	ewell_data_t ewell_data_sts;
	ewell_data_t ewell_data_dat;
	char id[WELL_ID_LEN];
	char sn[WELL_SN_LEN];
	char sensor_mac[MAC_ADDR_LEN];
	char tag_mac[MAX_TAG_NUM][WELL_MAC_LEN];
} config_ewell_t;

/**
 * buf: tag����
 * buf_len: ���ݳ���
 * ����ֵ:ִ�гɹ�����0��ʧ�ܷ�������ֵ
 */
typedef int (*rfid_write_tag_info_t)(uint8_t *buf, int buf_len);

/* ע��дtag���ݽӿ�(����-->������-->�豸)��ע��ɹ�����0��ʧ�ܷ���-1 */
int vscp_rfid_register_write_func(rfid_write_tag_info_t func);

/**
 * ���±�ǩ���ݣ�ִ�гɹ�����0��ִ��ʧ�ܷ���-1
 *
 */
int vscp_rfid_update_data(vscp_client_t *data, uint8_t macaddr[MAC_ADDR_LEN], uint8_t *buf, int msg_num);

int vscp_iot_update_data(vscp_client_t *data, uint8_t macaddr[MAC_ADDR_LEN], uint8_t *buf, int msg_num);

/**
 * vscp��ʼ��:�������ӷ��������׽���;ע��obj��lwm2m core lib��ʼ��
 * ����ֵ:�ɹ�����0��ʧ�ܷ���-1
 *
 **/
int vscp_init(vscp_client_t *data);

int vscp_uninit(vscp_client_t * data);

/**
 * purl:��Դ�仯��obj��url��Ŀǰ��"/10241"
 * ��������:ͨ����Ŀ���Դ�仯;��ֹlwm2m_step��ʱ����ִ��lwm2m_step�����¼���lwm2m_step��ʱ��
 **/
void vscp_update_notify(vscp_client_t *data, char *purl);

#endif

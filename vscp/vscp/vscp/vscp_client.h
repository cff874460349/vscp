#ifndef _VSCP_CLIENT_H_
#define _VSCP_CLIENT_H_

#include "liblwm2m.h"
#include "connection.h"

#define RSSI_INSTANCE_MAX_NUM           20   /* TODO实际个数用户配置 */
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
	uint8_t         mac[MAC_ADDR_LEN];   //每个接收器芯片mac,整机mac在注册时提供；
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
	char *localPort;    /* 本地服务器端口号，默认为56830 */
    char *localName;    /* 设备名称，建议以本机的mac地址命令，mac地址格式为XX-XX-XX-XX-XX-XX(大写) */
	char *serverPort;   /* 控制器端口号，默认为5683 */
    char *serverAddr;   /* 控制器的ip地址或者域名，默认为localhost，目前控制器域名为iotc.ruijie.com.cn */
    char *softwareVersion;  /* 设备软件版本号 */
    char *hardwareType; /* 设备硬件类型 */
    char *serialNumber; /* 设备序列号 */
    lwm2m_object_t * objArray[OBJ_COUNT];
    lwm2m_context_t * lwm2mH;
    struct rg_thread_master *vscp_master;   /* 用户传入其创建的伪线程调度器 */
    /* 保存lwm2m_step定时器伪线程，vscp_update_notify执行 cancel时会用到 */
    struct rg_thread *lwm2m_step_thread;
    struct rg_thread *client_thread;
    struct rg_thread *debug_thread;

    //传感器初始化信息
    uint32_t ins_num;   /* 传感器个数，容量暂定为20 */
    rfid_instance_t ins_array[RSSI_INSTANCE_MAX_NUM];  /* 传感器基本信息，mac/type等信息 */
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
 * buf: tag数据
 * buf_len: 数据长度
 * 返回值:执行成功返回0，失败返回其他值
 */
typedef int (*rfid_write_tag_info_t)(uint8_t *buf, int buf_len);

/* 注册写tag数据接口(管理-->控制器-->设备)，注册成功返回0，失败返回-1 */
int vscp_rfid_register_write_func(rfid_write_tag_info_t func);

/**
 * 更新标签数据，执行成功返回0，执行失败返回-1
 *
 */
int vscp_rfid_update_data(vscp_client_t *data, uint8_t macaddr[MAC_ADDR_LEN], uint8_t *buf, int msg_num);

int vscp_iot_update_data(vscp_client_t *data, uint8_t macaddr[MAC_ADDR_LEN], uint8_t *buf, int msg_num);

/**
 * vscp初始化:创建连接服务器的套接字;注册obj，lwm2m core lib初始化
 * 返回值:成功返回0，失败返回-1
 *
 **/
int vscp_init(vscp_client_t *data);

int vscp_uninit(vscp_client_t * data);

/**
 * purl:资源变化的obj的url，目前是"/10241"
 * 函数功能:通告核心库资源变化;终止lwm2m_step定时器；执行lwm2m_step；重新加载lwm2m_step定时器
 **/
void vscp_update_notify(vscp_client_t *data, char *purl);

#endif

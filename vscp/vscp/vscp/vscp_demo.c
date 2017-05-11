#include "vscp_client.h"
#include "vscp_debug.h"
#include "rfid_types.h"

#include <string.h>
#include <stdlib.h>
#include <rg_thread.h>
#include <signal.h>
#include <unistd.h>

#define RSSI_INSTANCE_NUM           2

static int g_quit = 0;

uint8_t macaddr[RSSI_INSTANCE_NUM][MAC_ADDR_LEN] =
            {{0x00, 0xd0, 0x00, 0x00, 0x00, 0x00},
             {0x00, 0xd0, 0x01, 0x00, 0x00, 0x01}};

static int ins = 0;    //这个只用来说明标签个数动态变化情况的展示


void handle_sigint(int signum)
{
    g_quit = 1;
}


/**
 * 进入main loop，仅供用户参考
 *
 */
int rssi_start(vscp_client_t *data)
{
    struct rg_thread thread;

    /*
     * We catch Ctrl-C signal for a clean exit
     */
    signal(SIGINT, handle_sigint);


    //main loop
    while ((0 == g_quit) && (rg_thread_fetch(data->vscp_master, &thread)))
    {
        rg_thread_call (&thread);
    }

    VSCP_DBG_INFO("Leave rg-thread loop\r\n\n");

    return 0;
}

#if 1
#define ladrip_number 10000
int ladrip_mac_connected[ladrip_number]={0};
unsigned char ladrip_mac[ladrip_number][6];

int total=10;

char sensor_mac[6]; // 从控制台获取
vscp_client_t *client_global;

int packet_cnt=0;
static int iop_send_print(uint8_t *buf, int buf_len)
{
    int i;

    packet_cnt++;
//    if (vscp_debug_info != 0) {
        fprintf(stdout, "\n----------iop send cnt: %d -------------\n", packet_cnt);
        fprintf(stdout, "data:\n");
        for (i = 0; i < buf_len; i++) {
            fprintf(stdout, "%02x ", buf[i]);
        }
        fprintf(stdout, "\n-------------END----------------\n");
//    }

    return 0;
}

int log_b = 1;

int iop_send_data(char *data, int len)
{
//    vscp_client_t *data = (vscp_client_t *)thread->arg;
//    struct rg_thread *thread2;
    int i, ret;
//    struct timeval tv;

 //   ins++;

 //   int msg_num = 5;

    uint8_t *buf = malloc(len+16);
    if (buf == NULL) {
        return -1;
    }

    memset(buf, 0, len+16);
    tag_msg_info_t *p = (tag_msg_info_t *)buf;

    p->msg_len = len;
    memcpy(p->msg, data, p->msg_len);

    ret = vscp_rfid_update_data(client_global, sensor_mac, buf, 1);
    if (ret) {
       VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
    }
    free(buf);

  //  if (log_b == 1)
  //      iop_send_print(data, len);
    return 0;
}

int ladrip_send_weight(struct rg_thread *thread)
{
    struct rg_thread *thread2;
    int i, ret;
    struct timeval tv;

    char weight[]={0xff,0xff,0xff,0xff,0xff,0xff,
        0xEC, 0x08, 0x40, 0x00, 0x27, 0x04, 0x00, 0xF9, 0x53, 0x68 };

    

    for (i = 0; i < ladrip_number && i < total; i++) {
        if (ladrip_mac_connected[i] == 0)
            continue;
        
        memcpy(weight, ladrip_mac[i], 6);

        if (i == 0)
            fprintf(stdout, "\n------------- send weights ----------------\n");
        iop_send_data(weight, sizeof(weight));
    }


    tv.tv_sec =2;   //初始时间2s
    tv.tv_usec = 0;

    thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
              ladrip_send_weight, (void *)client_global, tv);
    if (thread2 == NULL) {
        VSCP_DBG_INFO("rg_thread_read_rssi CREAT timer failed\n");
        return -1;
    }

    return 0;
}

int e2_cnt=10;

struct rg_thread *e2_t=NULL;

struct rg_thread *scan_t = NULL;

static int power_num=0;

int ladrip_send_battery_cnt(struct rg_thread *thread)
{
    fprintf(stdout, "\n------------- total battery request num: %d ----------------\n", power_num);
    return;
}

int ladrip_send_e5(struct rg_thread *thread)
{
    char e5[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xe5,0x03,0x0, 0x0,0x0};

    fprintf(stdout, "\n\n\n\n\n\n\n\n\n\n\n\n-------------send E5 55555555555555555555555555555----------------\n");

    int i;
    for (i=0; i <ladrip_number&& i < total; i++){
        ladrip_mac_connected[i]=0;
        memcpy(e5, ladrip_mac[i],6);
        iop_send_data(e5,sizeof(e5));
    }    
}


int ladrip_send_e2(struct rg_thread *thread)
{

    int i;

    static int send_e2_num=0;

    e2_t=NULL;

    send_e2_num +=e2_cnt+e2_cnt+1;

    fprintf(stdout, "\n-------------send E2 num: %d----------------\n", send_e2_num);
    
    char e2_unused[] = {0xff,0xff,0xff,0xff,0xff,0xff,
        0xe2,0x10,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xc3,
        0x02,0x01,0x05, 0x03,0x02,0x01,0xa3};
    char e2[]={0xff,0xff,0xff,0xff,0xff,0xff,
        0xe2,0x1a,0x04,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xc3,
        0x07,0x09,0x4c,0x61,0x64,0x72,0x69,0x70,0x05,0x12,0x20,0x00,0x40,0x00,
        0x02,0x0a,0x00};


    log_b = 0;
    for (i = 0; i < ladrip_number&& i < total; i++){
        if (ladrip_mac_connected[i] == 1)
            continue; // 已经上线的ladrip不需要发送E2
    
        memcpy(e2_unused+10, ladrip_mac[i], 6);
        memcpy(e2+10, ladrip_mac[i], 6);
    
        for (int j =0; j < e2_cnt; j++) // 任意发送10个无用的E2
            iop_send_data(e2_unused, sizeof(e2_unused));
        iop_send_data(e2, sizeof(e2));// 发送1个有用的E2
        for (int j =0; j < e2_cnt; j++) // 发送10个无用的E2
            iop_send_data(e2_unused, sizeof(e2_unused));            
    }
    
    log_b = 1;
    
    return 0;
}


static int iop_send_data_func(uint8_t *buf, int buf_len)
{
    int i;
    static int e2_num=0;
    static int e4_num=0;
    static int reset_num=0;

    static int all_factory_num=0;
    static int factor_num=0;

    char mac[6];
    if (buf_len <= 6) {
        fprintf(stdout, "invalid message.\n");
        return -1;
    }

    memcpy(mac,buf,6);

    switch(buf[6]) {
    case 0x94: // 收到SCAN
    
        e2_num ++;
        

        if (e2_num >= 100) {
            ladrip_send_e5(NULL);
            return 0;
        }

        fprintf(stdout, "\n-------------scan num: %d----------------\n", e2_num);

        if (e2_t == NULL) {
            e2_t = rg_thread_add_timer(client_global->vscp_master, 
                      ladrip_send_e2, (void *)client_global, 6);
        }

        return 0;
    case 0x95: //收到connect事件，发送E4
        if (buf_len != 15) {
           fprintf(stdout, "\n-------------connect invalid----------------\n");
           return 0; 
        }

        char tmp_mac[7];
        memcpy(tmp_mac, buf+9, 6);
        tmp_mac[6]='\0';
        
        int ladrip_mac_[6];
        for (i=0; i<6; i++)
            ladrip_mac_[i]=tmp_mac[i];

  /*      char ff_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
        if (memcmp(ff_mac, tmp_mac, 6)==0) {
            fprintf(stdout, "\n-------------connect all ff-ff-ff-ff-ff-ff \n");
            for (i=0; i <ladrip_number&& i < total; i++){
                    if (ladrip_mac_connected[i] == 0) {
                        ladrip_mac_connected[i]=1;
        
                        char e4[]={0xff,0xff,0xff,0xff,0xff,0xff,
                            0xe4,0x0a,0x0, 0xff,0xff,0xff,0xff,0xff,0xff,
                            0x40,0x00,0x0};
                        memcpy(e4, ladrip_mac[i],6);
                        memcpy(e4+9, ladrip_mac[i],6);
                        iop_send_data(e4,sizeof(e4));
                    }
                
            }  

            return 0;
        } */

        e4_num++;
 /*       fprintf(stdout, "\n-------------connect  %x-%x-%x-%x-%x-%x  num: %d----------------\n", 
            ladrip_mac_[0],ladrip_mac_[1],ladrip_mac_[2],
            ladrip_mac_[3],ladrip_mac_[4],ladrip_mac_[5],
            e4_num); */


        for (i=0; i <ladrip_number&& i < total; i++){
            if (memcmp(ladrip_mac[i],tmp_mac, 6)== 0) {
                if (ladrip_mac_connected[i] == 0) {
                    ladrip_mac_connected[i]=1;
    
                    char e4[]={0xff,0xff,0xff,0xff,0xff,0xff,
                        0xe4,0x0a,0x0, 0xff,0xff,0xff,0xff,0xff,0xff,
                        0x40,0x00,0x0};
                    memcpy(e4, ladrip_mac[i],6);
                    memcpy(e4+9, ladrip_mac[i],6);
                    iop_send_data(e4,sizeof(e4));
                }
                
                break;
            }
        }
    
        return 0;
    case 0x9c:
        if (buf_len < 13) {
            fprintf(stdout, "invalid 0x9C message.\n");
            return -1;
        }

        for (i=0; i <ladrip_number&& i < total; i++){
            if (memcmp(ladrip_mac[i],mac, 6)== 0) {
                break;
            }
        }
        if (i == ladrip_number || i == total) {
            fprintf(stdout, "no such MAC in 0x9c.\n");
            return -1;            
        }


        switch(buf[12]) {
        case 0x0b: // 收到电量请求
            power_num++;
            fprintf(stdout, "\n-------------battery request num:%d ----------------\n", power_num);
            char battery[]={0xff,0xff,0xff,0xff,0xff,0xff,
                0xEC, 0x07, 0x40, 0x00, 0x2C, 0x00, 0x06, 0x01, 0x60};
            memcpy(battery, ladrip_mac[i], 6);
            iop_send_data(battery, sizeof(battery));
            return 0;
        case 0x11: // 收到zero-adc
        all_factory_num++;
        fprintf(stdout, "\n-------------all factory gram index request  num: %d----------------\n", all_factory_num);
            char zero_adc[]={0xff,0xff,0xff,0xff,0xff,0xff,
                0xEC,0x14,0x40,0x00, 0x2C, 0x00,0x03,0x01,0xFF,0xF5, 0x64, 0x7A, 
                0x01,  0x00, 0x00, 0x04, 0x40,  0x04,  0x00, 0x14, 0x0A, 0x32};
            memcpy(zero_adc, ladrip_mac[i], 6);
            iop_send_data(zero_adc, sizeof(zero_adc));            
            return 0;
        case 0x10: // 收到X g因子请求
            if (buf_len != 14) {
                fprintf(stdout, "invalid in 0x9c request xg_factor.\n");
                return -1;                 
            }

            
            char xg_factor[]={0xff,0xff,0xff,0xff,0xff,0xff,
                0xEC, 0x0B, 0x40, 0x00, 0x2C, 0x00, 0x08, 
                0x00, 0x01, 0xFF, 0xFE, 0x58, 0xC7};
            
            memcpy(xg_factor, ladrip_mac[i], 6);

            xg_factor[13]=buf[13];

            int g = xg_factor[13];

            factor_num++;
            fprintf(stdout, "\n-------------%d g factor request, all-factor num: %d----------------\n", 
                g, factor_num);

            
            char zero_factor[4]={0xFF,0xF5, 0x64, 0x7A};
            char factor[4]={0x00, 0x00, 0x04, 0x40};
            if (xg_factor[13] == 0x0)
                memcpy(xg_factor+15, zero_factor,4);
            else
                memcpy(xg_factor+15, factor, 4);
                
            iop_send_data(xg_factor, sizeof(xg_factor));
            return 0;
        default:
            fprintf(stdout, "unknown 0x9C message.\n");
            
            return 0;
        }
        return 0;
    case 0x81:
        reset_num++;
        fprintf(stdout, "\n------------- reset  num: %d ----------------\n", reset_num);
        char start[7]={0xff,0xff,0xff,0xff,0xff,0xff, 0xe0};
        iop_send_data(start, sizeof(start));
        return 0;
    default:
        fprintf(stdout, "unknown message.\n");
        return 0;
    }


    return 0;
}

int ladrip_scan(struct rg_thread *thread)
{
/*
    char buf[]={0xff,0xff,0xff,0xff,0xff,0xff,0x94 };

    fprintf(stdout, "\n------------- like receiving scan ----------------\n");
*/

    char buf[]={0xff,0xff,0xff,0xff,0xff,0xff,0x95,0x00,0x00,
                0xff,0xff,0xff,0xff,0xff,0xff};

    fprintf(stdout, "\n------------- like receiving connect ----------------\n");
    iop_send_data_func(buf, 15);

    rg_thread_add_timer(client_global->vscp_master, ladrip_scan, 
        (void *)client_global, 15);
    return 0;
}


#endif

unsigned int g_port4mac;
uint8_t g_tag_info[10] ={0xBB, 0x00, 0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00};
//vscp_update_notify接口使用说明(example)
//假设无线从标签读取rssi数据的伪线程为rg_thread_read_rssi
int rg_thread_read_rfid(struct rg_thread *thread)
{
    vscp_client_t *data = (vscp_client_t *)thread->arg;
    struct rg_thread *thread2;
    int i, ret;
    struct timeval tv;

    ins++;

    int msg_num = 5;

    uint8_t *buf = malloc(64);
    if (buf == NULL) {
        return -1;
    }

    memset(buf, 0, 64);
    tag_msg_info_t *p = (tag_msg_info_t *)buf;
    for (i=0; i < msg_num; i++) {   /* 产生消息 */
        p->msg_len = sizeof(g_tag_info);
        memcpy(p->msg, g_tag_info, p->msg_len);
        p->msg[1] = (g_port4mac + 10000 * (ins%2)) / 256;
        p->msg[2] = (g_port4mac + 10000 * (ins%2)) % 256 + i;
        p->msg[4] = i;
#if 0
        int j;
        printf("Generate msg %d:\n", i);
        for(j = 0; j < p->msg_len; j++) {
            printf("%02x", p->msg[j]);
        }
        printf("\n");
#endif
        p = (tag_msg_info_t *)(p->msg + p->msg_len);
    }
    ret = vscp_rfid_update_data(data, data->ins_array[ins%2].mac, buf, msg_num);
    if (ret) {
       VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
    }
    free(buf);

    tv.tv_sec =3;   //初始时间2s
    tv.tv_usec = 0;

    thread2 = rg_thread_add_timer_timeval(data->vscp_master, rg_thread_read_rfid, (void *)data, tv);
    if (thread2 == NULL) {
        VSCP_DBG_INFO("rg_thread_read_rssi CREAT timer failed\n");
        return -1;
    }

    return 0;
}
//./vscp_demo 56830 + sensor_mac + tag_mac_begin
// ./vscp_demo 56830 300001 200001 10 1 2
/* ./vscp_demo 56830 300001(sensor_mac) 
                                     200001(ladrip_mac) 
                                     10(total, ladrip_number) 
                                     1(start_data)
                                     2(E2 packet)
*/
int  main(int argc, char **argv)
{
    vscp_client_t * context = NULL;
    int i, ret;
    char *lport = argv[1];
    char lmac[18];
    char *basemac = "00-D0-00-00-00-01";


    if (argc  != 7) {
        printf("param err (acgc != 7)\n");
        return -1;
    }

#if 1
    vscp_rfid_register_write_func(iop_send_data_func);

    char *sensor_mac_str=argv[2];
    char *tag_mac_str_begin=argv[3];
    char *total_str=argv[4];
    char *start_data=argv[5];
    char *e2_cnt_str=argv[6];
    
    int mac_val;

    total=atoi(total_str);
    e2_cnt=atoi(e2_cnt_str);
    fprintf(stdout, "\n------------- total %d, unused_e2_cnt: %d----------------\n", 
        total,e2_cnt);
    

    // 构造sensor MAC
    for (i = 0; i< strlen(sensor_mac_str) && i<6; i++)
        sensor_mac[i]=sensor_mac_str[i]-'0';
/*
    // 构造第一个ladrip MAC
    for (i = 0; i< strlen(tag_mac_str_begin) && i<6; i++){
        for(int j =0; j < ladrip_number && j < total; j++)
            ladrip_mac[j][i]= tag_mac_str_begin[i]-'0';
    }

    // 构造第1  - x个 ladrip MAC
    for (i=0; i<ladrip_number && i < total; i++) {
        if (ladrip_mac[i][5] != 0xff) {
            ladrip_mac[i][]
        }
    } */

    mac_val = atoi(tag_mac_str_begin);
    int tmp_mac_val = mac_val;
    char str[32];
    for (i=0; i<ladrip_number && i < total; i++) {
        tmp_mac_val = mac_val + i;
        sprintf(str, "%d", tmp_mac_val);

        memset(ladrip_mac[i], 0 ,6);
        int k = strlen(str);
        for (int m=6-k; m < 6; m++)
            ladrip_mac[i][m] = str[m]-'0';
        
    }

    for (i=0; i<ladrip_number && i < total; i++) {
            fprintf(stdout, "\n-------------MAC  %x-%x-%x-%x-%x-%x----------------\n", 
            ladrip_mac[i][0],ladrip_mac[i][1],ladrip_mac[i][2],ladrip_mac[i][3],
            ladrip_mac[i][4],ladrip_mac[i][5]);
    }


#endif

    // sensor MAC, 这里初始化了lmac，但不会被用到，因为报文已经都
    // 封装好了
    strcpy(lmac, basemac);
    lmac[0] = lport[0];
    lmac[1] = lport[1];
    lmac[3] = lport[2];
    lmac[4] = lport[3];
    lmac[6] = lport[4];
    g_port4mac = strtol(lport, NULL, 10);

    context = (vscp_client_t *)malloc(sizeof(vscp_client_t));
    if (NULL == context)
        return -1; //xxx

#if 1
    client_global = context;
#endif

    memset(context, 0, sizeof(vscp_client_t));
    context->addressFamily = AF_INET;
    context->localPort = strdup(lport);
    context->localName = strdup(lmac);  // MAC地址,格式待定
    context->serverPort = strdup("5683");
//    context->serverAddr = strdup("iotc.ruijie.com.cn");
    context->serverAddr = strdup("192.168.23.78");

    context->softwareVersion = strdup("AP_RGOS 11.1(5)B8");
	context->hardwareType = strdup("AP120-W");
	context->serialNumber = strdup("1234567890");

    // tag mac，随机
    context->ins_num = RSSI_INSTANCE_NUM;
    for (i = 0; i < context->ins_num; i++) {
        context->ins_array[i].rfid_type = RFID_SEN_T_BLE; /* TODO */
        context->ins_array[i].model = RFID_SEN_M_BCM20736;
        memcpy(context->ins_array[i].mac, macaddr[i], MAC_ADDR_LEN);    /* TODO */
        context->ins_array[i].mac[4] = g_port4mac / 256;
        context->ins_array[i].mac[5] = g_port4mac % 256;
    }

    /* 创建伪线程,用户创建 */
    context->vscp_master = rg_thread_master_create();
    if (context->vscp_master == NULL) {
        VSCP_DBG_ERR("Failed to create thread master.\n");
        return -1;
    } else {
        VSCP_DBG_INIT("create rg-thread master success.\n");
    }

    /* 模仿用户更新数据 */
//    rg_thread_add_timer(context->vscp_master, rg_thread_read_rfid, (void *)context, 2);

//    rg_thread_add_timer(client_global->vscp_master, ladrip_scan, 
//        (void *)client_global, 10);

    rg_thread_add_timer(client_global->vscp_master, 
              ladrip_send_battery_cnt, (void *)client_global, 180);


    rg_thread_add_timer(client_global->vscp_master, 
              ladrip_send_e5, (void *)client_global, 600);

    if (strcmp(start_data, "1") == 0) {
        fprintf(stdout, "\n------------- start data ----------------\n");
        rg_thread_add_timer(context->vscp_master, ladrip_send_weight, (void *)context, 10);
    } else {
        fprintf(stdout, "\n------------- stop data ----------------\n");
    }

    ret = vscp_init(context);
    if (ret) {
        VSCP_DBG_ERR("init error\n");
        return -1;
    }
    //开启服务,main loop,仅供参考，用户可以在这里加入自己的伪线程服务
	rssi_start(context);
    vscp_uninit(context);
    //释放用户创建资源，如伪线程管理器，contex结构体等

    free(context->localPort);
    free(context->localName);
    free(context->serverPort);
    free(context->serverAddr);
    free(context->softwareVersion);
    free(context->hardwareType);
    free(context->serialNumber);

    free(context->vscp_master);

    free(context);
    return 0;
}

#include "vscp_client.h"
#include "vscp_debug.h"
#include "rfid_types.h"

#include <string.h>
#include <stdlib.h>
#include <rg_thread.h>
#include <signal.h>
#include <unistd.h>
#include<sys/time.h>


#define RSSI_INSTANCE_NUM           4

static int g_quit = 0;

static int stop=0;
uint8_t macaddr[RSSI_INSTANCE_NUM][MAC_ADDR_LEN] =
            {{0x00, 0xd0, 0x00, 0x00, 0x00, 0x00},
             {0x00, 0xd0, 0x01, 0x00, 0x00, 0x01},
             {0x00, 0xd0, 0x02, 0x00, 0x00, 0x01},
             {0x00, 0xd0, 0x03, 0x00, 0x00, 0x01}};

static int ins = 0;    //这个只用来说明标签个数动态变化情况的展示

int g_kyee_send_count = 0;
int g_ladrip_send_count = 0;
int g_ewell_send_count = 0;

typedef struct info_buf_ {
	uint8_t buf[4096];
	int used;
} info_buf_t;

info_buf_t info_buf_kyee;
info_buf_t info_buf_ladrip;
info_buf_t info_buf_ewell;

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
//#define ladrip_number 10000
//int config_ladrip.tag_mac_connected[ladrip_number]={0};
//unsigned char ladrip_mac[ladrip_number][6];
config_kyee_t config_kyee;
config_ladrip_t config_ladrip;
config_ewell_t config_ewell;

int total=10;

//char sensor_mac[6]; // 从控制台获取
vscp_client_t *client_global;

int packet_cnt=0;
static int iop_send_print(uint8_t *buf, int buf_len)
{
    int i;

    packet_cnt++;
    if (vscp_debug_info != 0) {
        fprintf(stdout, "\n----------iop send cnt: %d -------------\n", packet_cnt);
        fprintf(stdout, "data:\n");
        for (i = 0; i < buf_len; i++) {
            fprintf(stdout, "%02x ", buf[i]);
        }
        fprintf(stdout, "\n-------------END----------------\n");
    }

    return 0;
}

int log_b = 1;

unsigned now()
{    
	time_t now;
    struct timeval tv;
	
    gettimeofday(&tv, NULL);
    now = tv.tv_sec;

	return now;
}

int iop_send_data(int type, char *sensor_mac, char *data, int len)
{
//    vscp_client_t *data = (vscp_client_t *)thread->arg;
//    struct rg_thread *thread2;
    int i, ret;
//    struct timeval tv;

 //   ins++;

 //   int msg_num = 5;

#if 0
    uint8_t *buf = malloc(len+16);
    if (buf == NULL) {
        return -1;
    }

    memset(buf, 0, len+16);
    tag_msg_info_t *p = (tag_msg_info_t *)buf;

    p->msg_len = len;
    memcpy(p->msg, data, p->msg_len);
#endif
    static int msg_num_kyee = 0;
    static int msg_num_ladrip = 0;
    static int msg_num_ewell = 0;
	int m;

	if (type == 1) {
		m = config_kyee.tag_num > 10 ? 10 : config_kyee.tag_num;
		if (info_buf_kyee.used + len >= 4096) {
			ret = vscp_iot_update_data(client_global, sensor_mac, info_buf_kyee.buf, msg_num_kyee);
			if (ret) {
			   VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
			}
			memset(&info_buf_kyee, 0, sizeof(info_buf_kyee));
			msg_num_kyee = 0;
		}
		tag_msg_info_t *p = (tag_msg_info_t *)(info_buf_kyee.buf+info_buf_kyee.used);
		p->msg_len = len;
		info_buf_kyee.used += 2;
		memcpy(p->msg, data, p->msg_len);
		msg_num_kyee++;
		info_buf_kyee.used += p->msg_len;
		if ((info_buf_kyee.used >= 4096) || ((msg_num_kyee%m)==0)) {
			ret = vscp_iot_update_data(client_global, sensor_mac, info_buf_kyee.buf, msg_num_kyee);
			if (ret) {
			   VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
			}
			memset(&info_buf_kyee, 0, sizeof(info_buf_kyee));
			msg_num_kyee = 0;
		}
	} else if (type == 2) {
		m = config_ladrip.tag_num > 10 ? 10 : config_ladrip.tag_num;
		if (info_buf_ladrip.used + len >= 4096) {
			ret = vscp_iot_update_data(client_global, sensor_mac, info_buf_ladrip.buf, msg_num_ladrip);
			if (ret) {
			   VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
			}
			memset(&info_buf_ladrip, 0, sizeof(info_buf_ladrip));
			msg_num_ladrip = 0;
		}
		tag_msg_info_t *p = (tag_msg_info_t *)(info_buf_ladrip.buf+info_buf_ladrip.used);
		p->msg_len = len;
		info_buf_ladrip.used += 2;
		memcpy(p->msg, data, p->msg_len);
		msg_num_ladrip++;
		info_buf_ladrip.used += p->msg_len;
		if ((info_buf_ladrip.used >= 4096) || ((msg_num_ladrip%m)==0)) {
			ret = vscp_iot_update_data(client_global, sensor_mac, info_buf_ladrip.buf, msg_num_ladrip);
			if (ret) {
			   VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
			}
			memset(&info_buf_ladrip, 0, sizeof(info_buf_ladrip));
			msg_num_ladrip = 0;
		}
	} else if (type == 3) {
		m = config_ewell.tag_num > 10 ? 10 : config_ewell.tag_num;
		if (info_buf_ewell.used + len >= 4096) {
			ret = vscp_iot_update_data(client_global, sensor_mac, info_buf_ewell.buf, msg_num_ewell);
			if (ret) {
			   VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
			}
			memset(&info_buf_ewell, 0, sizeof(info_buf_ewell));
			msg_num_ewell = 0;
			//printf("batch send ewell len %d num %d\n", info_buf_ewell.used, msg_num_ewell);
		}
		tag_msg_info_t *p = (tag_msg_info_t *)(info_buf_ewell.buf+info_buf_ewell.used);
		p->msg_len = len;
		info_buf_ewell.used += 2;
		memcpy(p->msg, data, p->msg_len);
		info_buf_ewell.used += p->msg_len;
		msg_num_ewell++;
		if ((info_buf_ewell.used >= 4096) || ((msg_num_ewell%m)==0)) {
			ret = vscp_iot_update_data(client_global, sensor_mac, info_buf_ewell.buf, msg_num_ewell);
			if (ret) {
			   VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
			}
			//printf("batch send ewell len %d num %d\n", info_buf_ewell.used, msg_num_ewell);
			memset(&info_buf_ewell, 0, sizeof(info_buf_ewell));
			msg_num_ewell = 0;
		}
		//printf("send ewell len %d num %d\n", info_buf_ewell.used, msg_num_ewell);
	} 
    //ret = vscp_iot_update_data(client_global, sensor_mac, info_buf, msg_num);
    //if (ret) {
    //   VSCP_DBG_ERR("rg_thread_read_rssi() CALL rssi_update_data() failed!\n");
    //}
    //free(buf);

	/*if (memcmp(sensor_mac, config_kyee.sensor_mac, MAC_ADDR_LEN) == 0) {
		g_kyee_send_count++;
	} else if (memcmp(sensor_mac, config_ladrip.sensor_mac, MAC_ADDR_LEN) == 0) {
		g_ladrip_send_count++;
	} else if (memcmp(sensor_mac, config_ewell.sensor_mac, MAC_ADDR_LEN) == 0) {
		g_ewell_send_count++;
	}*/

	//static unsigned start = 0;
	//if (start == 0) {
	//	start = now();
	//	VSCP_LOG("start: %u\n", start);
	//}
	
//	VSCP_LOG("iop send cnt: kyee %d ladrip %d ewell %d\n",
//				g_kyee_send_count, g_ladrip_send_count, g_ewell_send_count);
	//fprintf(stdout,"\r[%u]iop send cnt: kyee %d ladrip %d ewell %d", now(),
	//								g_kyee_send_count, g_ladrip_send_count, g_ewell_send_count);

//    if (log_b == 1)
//        iop_send_print(data, len);
    return 0;
}

int kyee_send_hb(struct rg_thread *thread)
{
    struct rg_thread *thread2=NULL;
    int i, ret, n;
    struct timeval tv;
	char c;

    char hb[]={/*0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
		0xAA, 0x55, 0xC0, 0x01, 0x00, 0x01, 0x05, */0xBB, 0xFF, 0xFF, 0x00, 0x04/*, 0x53, 0x57, 0x7E*/ };

    for (i = 0; i < MAX_TAG_NUM && i < config_kyee.tag_num; i++) {
	 	c = ((config_kyee.tag_mac[i][2] << 4) | config_kyee.tag_mac[i][3]);
        hb[1] = c;
		c = ((config_kyee.tag_mac[i][4] << 4) | config_kyee.tag_mac[i][5]);
		hb[2] = c;

//        if (i == 0)
//            fprintf(stdout, "\n------------- send kyee heartbeat data ----------------\n");
        iop_send_data(1, config_kyee.sensor_mac, hb, sizeof(hb));
    }


    tv.tv_sec = config_kyee.kyee_data_hb.inter;   //初始时间2s
    tv.tv_usec = 0;
	n = 1;
	if (config_kyee.kyee_data_hb.times == 0) {
		do {
			thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
	              kyee_send_hb, (void *)client_global, tv);
		    if (thread2 == NULL) {
		        VSCP_LOG("kyee_send_hb CREATE timer failed\n");
		        sleep(n);
		    }
			n += 1;
		} while (thread2 == NULL);
	} else if (--config_kyee.kyee_data_hb.times_limit > 0) {
		do {
			thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
	              kyee_send_hb, (void *)client_global, tv);
		    if (thread2 == NULL) {
		        VSCP_LOG("kyee_send_hb CREATE timer failed\n");
		        sleep(n);
		    }
			n += 1;
		} while (thread2 == NULL);
	}

    return 0;
}

int kyee_send_off(struct rg_thread *thread)
{
    struct rg_thread *thread2=NULL;
    int i, ret, n;
    struct timeval tv;
	char c;

    char off[]={ 0xBC, 0xFF, 0xFF, 0x00, 0x04 };

    for (i = 0; i < MAX_TAG_NUM && i < config_kyee.tag_num; i++) {
	 	c = ((config_kyee.tag_mac[i][2] << 4) | config_kyee.tag_mac[i][3]);
        off[1] = c;
		c = ((config_kyee.tag_mac[i][4] << 4) | config_kyee.tag_mac[i][5]);
		off[2] = c;

//        if (i == 0)
//            fprintf(stdout, "\n------------- send kyee offline data ----------------\n");
        iop_send_data(1, config_kyee.sensor_mac, off, sizeof(off));
    }


    tv.tv_sec = config_kyee.kyee_data_off.inter;   //初始时间2s
    tv.tv_usec = 0;
	n = 1;
	if (config_kyee.kyee_data_off.times == 0) {
		do {
			thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
	              kyee_send_off, (void *)client_global, tv);
		    if (thread2 == NULL) {
		        VSCP_LOG("kyee_send_off CREATE timer failed\n");
		        sleep(n);
		    }
			n += 1;
		} while (thread2 == NULL);
	} else if (--config_kyee.kyee_data_off.times_limit > 0) {
	    do {
			thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
	              kyee_send_off, (void *)client_global, tv);
		    if (thread2 == NULL) {
		        VSCP_LOG("kyee_send_off CREATE timer failed\n");
		        sleep(n);
		    }
			n += 1;
		} while (thread2 == NULL);
	}


    return 0;
}

int kyee_send_out(struct rg_thread *thread)
{
    struct rg_thread *thread2=NULL;
    int i, ret, n;
    struct timeval tv;
	char c;

    char off[]={ 0xBD, 0xFF, 0xFF, 0x00, 0x04 };

    for (i = 0; i < MAX_TAG_NUM && i < config_kyee.tag_num; i++) {
     //   if (config_ladrip.tag_mac_connected[i] == 0)
     //       continue;
	 	c = ((config_kyee.tag_mac[i][2] << 4) | config_kyee.tag_mac[i][3]);
        off[1] = c;
		c = ((config_kyee.tag_mac[i][4] << 4) | config_kyee.tag_mac[i][5]);
		off[2] = c;

//       if (i == 0)
//            fprintf(stdout, "\n------------- send out data ----------------\n");
        iop_send_data(1, config_kyee.sensor_mac, off, sizeof(off));
    }

	tv.tv_sec = config_kyee.kyee_data_out.inter;	//初始时间2s
	tv.tv_usec = 0;
	n = 1;
	if (config_kyee.kyee_data_out.times == 0) {
	   do {
			thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
	              kyee_send_out, (void *)client_global, tv);
		    if (thread2 == NULL) {
		        VSCP_LOG("kyee_send_out CREATE timer failed\n");
		        sleep(n);
		    }
			n += 1;
		} while (thread2 == NULL);
	} else if (--config_kyee.kyee_data_out.times_limit > 0) {
	   do {
			thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
	              kyee_send_out, (void *)client_global, tv);
		    if (thread2 == NULL) {
		        VSCP_LOG("kyee_send_out CREATE timer failed\n");
		        sleep(n);
		    }
			n += 1;
		} while (thread2 == NULL);
	}

    return 0;
}

int ewell_send_sts(struct rg_thread *thread)
{
	int i, n=1;
    struct timeval tv;
    struct rg_thread *thread2=NULL;
    char sts[] = {0x01,0x02,0x23,0x53,0x54,0x53,0x31,0x32,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
		0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x30,0x30,0x30,0x30,0x31,0x39,0x38,0x39,0x46,0x46,0x30,0x30,
		0x30,0x30,0x30,0x30,0x31,0x38,0x42,0x41,0x03,0x02,0x25,0x53,0x54,0x53,0x31,0x32,0x31,0x32,0x33,0x34,0x30,
		0x30,0x30,0x30,0x30,0x35,0x30,0x33,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x30,0x30,0x30,0x30,0x31,0x39,
		0x38,0x39,0x46,0x46,0x30,0x30,0x30,0x30,0x30,0x30,0x33,0x34,0x33,0x46,0x03,0x04};

//    fprintf(stdout, "\n\n\n\n\n\n\n\n\n\n\n\n-------------send ewell sts ----------------\n");

    for (i=0; i<4; i++) {
		sts[8+i] = config_ewell.id[i];
	}
	for (i=0; i<16; i++) {
		sts[12+i] = config_ewell.sn[i];
	}
    iop_send_data(3, config_ewell.sensor_mac, sts,sizeof(sts));

	tv.tv_sec = 120;
    tv.tv_usec = 0;

	do {
		thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
              ewell_send_sts, (void *)client_global, tv);
	    if (thread2 == NULL) {
	        VSCP_LOG("ewell_send_sts CREATE timer failed\n");
	        sleep(n);
	    }
		n += 1;
	} while (thread2 == NULL);

    return 0;
}

int ewell_send_dat(struct rg_thread *thread)
{
    char dat[] = {0x01,0x02,0x23,0x44,0x41,0x54,0x31,0x32,0xff,0xff,0xff,0xff,0x30,0x30,0x31,0x31,0xff,0xff,0xff,0xff,0xff,
		0xff,0xff,0xff,0x30,0x37,0x42,0x41,0x32,0x42,0x30,0x30,0x32,0x38,0x36,0x39,0x46,0x41,0x30,0x34,0x30,
		0x46,0x33,0x32,0x38,0x35,0x42,0x45,0x03,0x04};
    struct timeval tv;
    struct rg_thread *thread2=NULL;
	int n = 1;

//    fprintf(stdout, "\n\n\n\n\n\n\n\n\n\n\n\n-------------send ewell dat ----------------\n");

    int i,j;
	memcpy(&dat[8], config_ewell.id, WELL_ID_LEN);
    for (i = 0; i < MAX_TAG_NUM && i < config_ewell.tag_num; i++){
		for (j = 0; j < 8; j++) {
			//fprintf(stdout, "\n-------------%02x ----------------\n", config_ewell.tag_mac[i][j]+48);
			dat[j+16] = config_ewell.tag_mac[i][j]+48;
		}
        //memcpy(dat+16, buf, 8);
        iop_send_data(3, config_ewell.sensor_mac, dat,sizeof(dat));
    }
	tv.tv_sec = 0;
    tv.tv_usec = (config_ewell.ewell_data_dat.inter*1000000)/config_ewell.ewell_data_dat.times;

	do {
		thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
              ewell_send_dat, (void *)client_global, tv);
	    if (thread2 == NULL) {
	        VSCP_LOG("ewell_send_dat CREATE timer failed\n");
	        sleep(n);
	    }
		n += 1;
	} while (thread2 == NULL);

    return 0;
}

/*****************************************************
作  者:chenfufeng
日  期:2017-05-02
描  述:模拟正常滴液
******************************************************/
float ladrip_calc_currWeigth(float curr_weigth, float rate)
{
	float left_weight = curr_weigth - 0.001*rate;
	
	if (left_weight >= 0){
		config_ladrip.ladrip_data.weight_value = left_weight;
	}else{
		config_ladrip.ladrip_data.weight_value = 0;
	}
}

int ladrip_send_weight(struct rg_thread *thread)
{
    struct rg_thread *thread2 = NULL;
    int i, ret, n;
//    int i, ret;
    struct timeval tv;


	// F5 55 8C 为0ml数据报文
    char weight[]={0xff,0xff,0xff,0xff,0xff,0xff,
        0xEC, 0x08, 0x40, 0x00, 0x27, 0x04, 0x00, 0xF9, 0x53, 0x68 };

//    if (stop==1)
 //       return 0;

	/*计算滴液量*/
	config_ladrip.ladrip_data.weight_value = ladrip_calc_currWeigth(config_ladrip.ladrip_data.weight_value,config_ladrip.ladrip_data.rate);

    for (i = 0; i < MAX_TAG_NUM && i < config_ladrip.tag_num; i++) {
     //   if (config_ladrip.tag_mac_connected[i] == 0)
     //       continue;
        
        memcpy(weight, config_ladrip.tag_mac[i], 6);

//        if (i == 0)
//            fprintf(stdout, "\n------------- send ladrip weights ----------------\n");
        iop_send_data(2, config_ladrip.sensor_mac, weight, sizeof(weight));
    }


    tv.tv_sec = 2;   //初始时间2s
    tv.tv_usec = 0;
	n = 1;
	do {
		thread2 = rg_thread_add_timer_timeval(client_global->vscp_master, 
              ladrip_send_weight, (void *)client_global, tv);
	    if (thread2 == NULL) {
	        VSCP_LOG("ladrip_send_weight CREATE timer failed\n");
	        sleep(n);
	    }
		n += 1;
	} while (thread2 == NULL);

    return 0;
}

int e2_cnt=10;

struct rg_thread *e2_t=NULL;

struct rg_thread *scan_t = NULL;

static int power_num=0;

int ladrip_send_battery_cnt(struct rg_thread *thread)
{
//    fprintf(stdout, "\n------------- total battery request num: %d ----------------\n", power_num);
    return 0;
}

int ladrip_send_e5(struct rg_thread *thread)
{
    char e5[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xe5,0x03,0x0, 0x0,0x0};

//    fprintf(stdout, "\n\n\n\n\n\n\n\n\n\n\n\n-------------send E5 55555555555555555555555555555----------------\n");

    int i;
    for (i = 0; i < MAX_TAG_NUM && i < config_ladrip.tag_num; i++){
        config_ladrip.tag_mac_connected[i]=0;
        memcpy(e5, config_ladrip.tag_mac[i],6);
        iop_send_data(2, config_ladrip.sensor_mac, e5,sizeof(e5));
    }    
}


int ladrip_send_e2(struct rg_thread *thread)
{

    int i;

    static int send_e2_num=0;

    e2_t=NULL;

 //   if (stop == 1)
 //       return 0;

    send_e2_num +=e2_cnt+e2_cnt+1;

//    fprintf(stdout, "\n-------------send E2 num: %d----------------\n", send_e2_num);
    
    char e2_unused[] = {0xff,0xff,0xff,0xff,0xff,0xff,
        0xe2,0x10,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xc3,
        0x02,0x01,0x05, 0x03,0x02,0x01,0xa3};
    char e2[]={0xff,0xff,0xff,0xff,0xff,0xff,
        0xe2,0x1a,0x04,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xc3,
        0x07,0x09,0x4c,0x61,0x64,0x72,0x69,0x70,0x05,0x12,0x20,0x00,0x40,0x00,
        0x02,0x0a,0x00};
    char e2_2[]={0xff,0xff,0xff,0xff,0xff,0xff,
        0xe2,0x1a,0x04,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xc3,
        0x07,0x09,0x4c,0x61,0x64,0x72,0x69,0x70,0x05,0x12,0x20,0x00,0x40,0x00,
        0x02,0x0a,0x00};    


    log_b = 0;
    for (i = 0; i < MAX_TAG_NUM && i < config_ladrip.tag_num; i++){
        if (config_ladrip.tag_mac_connected[i] == 1)
            continue; // 已经上线的ladrip不需要发送E2
    
        memcpy(e2_unused+10, config_ladrip.tag_mac[i], 6);
        memcpy(e2+10, config_ladrip.tag_mac[i], 6);
    
        for (int j =0; j < e2_cnt; j++) // 任意发送10个无用的E2
            iop_send_data(2, config_ladrip.sensor_mac, e2_unused, sizeof(e2_unused));
        iop_send_data(2, config_ladrip.sensor_mac, e2, sizeof(e2));// 发送1个有用的E2
        iop_send_data(2, config_ladrip.sensor_mac, e2, sizeof(e2));// 发送1个有用的E2
        iop_send_data(2, config_ladrip.sensor_mac, e2_2, sizeof(e2_2));
        for (int j =0; j < e2_cnt; j++) // 发送10个无用的E2
            iop_send_data(2, config_ladrip.sensor_mac, e2_unused, sizeof(e2_unused));            
    }
    
    log_b = 1;

    // 测试错误的E4
        char e4_1[]={0x1f,0xff,0xff,0xff,0xff,0xff,
        0xe4,0x09,0x0, 0xff,0xff,0xff,0xff,0xff,0xff,
        0x40,0x00,0x00};
    iop_send_data(2, config_ladrip.sensor_mac, e4_1,sizeof(e4_1));

            char e4_2[]={0x1f,0xff,0xff,0xff,0xff,0xff,
        0xe4,0x0a,0x0, 0xff,0xff,0xff,0xff,0xff,0xff,
        0x40,0x00};
    iop_send_data(2, config_ladrip.sensor_mac, e4_2,sizeof(e4_2));
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
	
	if (!config_ladrip.enable) return 0;
	
    memcpy(mac,buf,6);

    switch(buf[6]) {
    case 0x94: // 收到SCAN

    	if (e2_cnt == 100)
        	ladrip_send_e5(NULL);
    
        e2_num ++;
        
/*
        if (e2_num == 20) {
         //   ladrip_send_e5(NULL);
            rg_thread_add_timer(client_global->vscp_master, 
                      ladrip_send_e5, (void *)client_global, 6);
            stop=1;
            return 0;
        }

        if (e2_num >= 20)
            return 0;
        */

//        fprintf(stdout, "\n-------------scan num: %d----------------\n", e2_num);

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
                    if (config_ladrip.tag_mac_connected[i] == 0) {
                        config_ladrip.tag_mac_connected[i]=1;
        
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


        for (i = 0; i < MAX_TAG_NUM && i < config_ladrip.tag_num; i++){
            if (memcmp(config_ladrip.tag_mac[i],tmp_mac, 6)== 0) {
                if (config_ladrip.tag_mac_connected[i] == 0) {
                    config_ladrip.tag_mac_connected[i]=1;
    
                    char e4[]={0xff,0xff,0xff,0xff,0xff,0xff,
                        0xe4,0x0a,0x0, 0xff,0xff,0xff,0xff,0xff,0xff,
                        0x40,0x00,0x0};
                    memcpy(e4, config_ladrip.tag_mac[i],6);
                    memcpy(e4+9, config_ladrip.tag_mac[i],6);
                    iop_send_data(2, config_ladrip.sensor_mac, e4,sizeof(e4));
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

        for (i = 0; i < MAX_TAG_NUM && i < config_ladrip.tag_num; i++){
            if (memcmp(config_ladrip.tag_mac[i],mac, 6)== 0) {
                break;
            }
        }
        if (i == MAX_TAG_NUM || i == config_ladrip.tag_num) {
            fprintf(stdout, "no such MAC in 0x9c.\n");
            return -1;            
        }


        switch(buf[12]) {
        case 0x0b: // 收到电量请求
            power_num++;
//            fprintf(stdout, "\n-------------battery request num:%d ----------------\n", power_num);
            char battery[]={0xff,0xff,0xff,0xff,0xff,0xff,
                0xEC, 0x07, 0x40, 0x00, 0x2C, 0x00, 0x06, 0x01, 0x60};
            memcpy(battery, config_ladrip.tag_mac[i], 6);
            iop_send_data(2, config_ladrip.sensor_mac, battery, sizeof(battery));
            return 0;
        case 0x11: // 收到zero-adc
        all_factory_num++;
//        fprintf(stdout, "\n-------------all factory gram index request  num: %d----------------\n", all_factory_num);
            char zero_adc[]={0xff,0xff,0xff,0xff,0xff,0xff,
                0xEC,0x14,0x40,0x00, 0x2C, 0x00,0x03,0x01,0xFF,0xF5, 0x64, 0x7A, 
                0x01,  0x00, 0x00, 0x04, 0x40,  0x04,  0x00, 0x14, 0x0A, 0x32};
            memcpy(zero_adc, config_ladrip.tag_mac[i], 6);
            iop_send_data(2, config_ladrip.sensor_mac, zero_adc, sizeof(zero_adc));            
            return 0;
        case 0x10: // 收到X g因子请求
            if (buf_len != 14) {
//                fprintf(stdout, "invalid in 0x9c request xg_factor.\n");
                return -1;                 
            }

            
            char xg_factor[]={0xff,0xff,0xff,0xff,0xff,0xff,
                0xEC, 0x0B, 0x40, 0x00, 0x2C, 0x00, 0x08, 
                0x00, 0x01, 0xFF, 0xFE, 0x58, 0xC7};
            
            memcpy(xg_factor, config_ladrip.tag_mac[i], 6);

            xg_factor[13]=buf[13];

            int g = xg_factor[13];

            factor_num++;
//            fprintf(stdout, "\n-------------%d g factor request, all-factor num: %d----------------\n", 
//               g, factor_num);

            
            char zero_factor[4]={0xFF,0xF5, 0x64, 0x7A};
            char factor[4]={0x00, 0x00, 0x04, 0x40};
            if (xg_factor[13] == 0x0)
                memcpy(xg_factor+15, zero_factor,4);
            else
                memcpy(xg_factor+15, factor, 4);
                
            iop_send_data(2, config_ladrip.sensor_mac, xg_factor, sizeof(xg_factor));
            return 0;
        default:
            fprintf(stdout, "unknown 0x9C message.\n");
            
            return 0;
        }
        return 0;
    case 0x81:
        reset_num++;
//        fprintf(stdout, "\n------------- reset  num: %d ----------------\n", reset_num);
        char start[7]={0xff,0xff,0xff,0xff,0xff,0xff, 0xe0};
        iop_send_data(2, config_ladrip.sensor_mac, start, sizeof(start));

			for (i = 0; i < MAX_TAG_NUM && i < config_ladrip.tag_num; i++){
                  config_ladrip.tag_mac_connected[i]=0;
            }
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

//    fprintf(stdout, "\n------------- like receiving connect ----------------\n");
    iop_send_data_func(buf, 15);

    rg_thread_add_timer(client_global->vscp_master, ladrip_scan, 
        (void *)client_global, 5);
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
                                     ip
*/
int  main(int argc, char **argv)
{
    vscp_client_t * context = NULL;
    int i, ret;
    char *lport = argv[1];
    char lmac[18]={0};
    char *basemac = "00-D0-00-00-00-01";


    if (argc != 30) {
        printf("param err (acgc != 30)\n");
        return -1;
    }

#if 1
    vscp_rfid_register_write_func(iop_send_data_func);

    char *ip = argv[2];
    char *iop_mac=argv[3];
	char *kyee = argv[4];
	char *kyee_mac_begin = argv[5];
	char *kyee_num = argv[6];
	char *kyee_data_hb = argv[7];
	char *kyee_data_hb_times = argv[8];
	char *kyee_data_hb_inter = argv[9];
	char *kyee_data_off = argv[10];
	char *kyee_data_off_times = argv[11];
	char *kyee_data_off_inter = argv[12];
	char *kyee_data_out = argv[13];
	char *kyee_data_out_times = argv[14];
	char *kyee_data_out_inter = argv[15];
	char *ladrip = argv[16];
	char *ladrip_mac_begin = argv[17];
	char *ladrip_num = argv[18];
	char *ladrip_data_weight = argv[19];
	char *ladrip_data_e2 = argv[20];
	/*Begin: Added by chenfufeng, 2017/05/02,增加模拟正常滴液功能*/
	char *ladrip_weight_value = argv[30];
	char *ladrip_rate = argv[31];
	/*End: Added by chenfufeng, 2017/05/02,增加模拟正常滴液功能*/
	char *ewell = argv[21];
	char *ewell_mac_begin = argv[22];
	char *ewell_num = argv[23];
	char *ewell_data_sts = argv[24];
	char *ewell_data_dat = argv[25];
	char *ewell_data_dat_inter = argv[26];
	char *ewell_data_dat_times = argv[27];
	test_name = argv[28];
	test_id = argv[29];
    
    int mac_val;

    //fprintf(stdout, "\n------------- total %d, unused_e2_cnt: %d----------------\n");

	if (strcmp(kyee, "1") == 0) {
		config_kyee.enable = true;
	}
	config_kyee.tag_num = atoi(kyee_num);
    if (strcmp(kyee_data_hb, "1") == 0) {
		config_kyee.kyee_data_hb.enable = true;
		config_kyee.kyee_data_hb.times_limit = config_kyee.kyee_data_hb.times = atoi(kyee_data_hb_times);
		config_kyee.kyee_data_hb.inter = atoi(kyee_data_hb_inter)==0?10:atoi(kyee_data_hb_inter);
	}
	if (strcmp(kyee_data_off, "1") == 0) {
		config_kyee.kyee_data_off.enable = true;
		config_kyee.kyee_data_off.times_limit = config_kyee.kyee_data_off.times = atoi(kyee_data_off_times);
		config_kyee.kyee_data_off.inter = atoi(kyee_data_off_inter)==0?10:atoi(kyee_data_off_inter);
	}
	if (strcmp(kyee_data_out, "1") == 0) {
		config_kyee.kyee_data_out.enable = true;
		config_kyee.kyee_data_out.times_limit = config_kyee.kyee_data_out.times = atoi(kyee_data_out_times);
		config_kyee.kyee_data_out.inter = atoi(kyee_data_out_inter)==0?10:atoi(kyee_data_out_inter);
	}
	mac_val = atoi(kyee_mac_begin);
    int tmp_mac_val = mac_val;
    char str[32];
    for (i=0; i<MAX_TAG_NUM && i < config_kyee.tag_num; i++) {
        tmp_mac_val = mac_val + i;
        sprintf(str, "%06d", tmp_mac_val);

        memset(config_kyee.tag_mac[i], 0 ,6);
        int k = strlen(str);
        for (int m=6-k; m < 6; m++)
            config_kyee.tag_mac[i][m] = str[m]-'0';
        
    }
    for (i=0; i<MAX_TAG_NUM && i < config_kyee.tag_num; i++) {
            fprintf(stdout, "\n-------------KYEE MAC  %x-%x-%x-%x-%x-%x----------------\n", 
            config_kyee.tag_mac[i][0],config_kyee.tag_mac[i][1],config_kyee.tag_mac[i][2],config_kyee.tag_mac[i][3],
            config_kyee.tag_mac[i][4],config_kyee.tag_mac[i][5]);
    }

	
    // 构造sensor MAC
    //for (i = 0; i< strlen(sensor_mac_str) && i<6; i++)
    //    sensor_mac[i]=sensor_mac_str[i]-'0';
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

	
    if (strcmp(ladrip, "1") == 0) {
		config_ladrip.enable = 1;
	}
	config_ladrip.tag_num = atoi(ladrip_num);
    if (strcmp(ladrip_data_e2, "1") == 0) {
		config_ladrip.ladrip_data.e2 = 1;
	}
	if (strcmp(ladrip_data_weight, "1") == 0) {
		config_ladrip.ladrip_data.weight= 1;
		config_ladrip.ladrip_data.weight_value = atoi(ladrip_weight_value)==0?100.0:(float)atoi(ladrip_weight_value);
		config_ladrip.ladrip_data.rate = atoi(ladrip_rate)==0?0:(float)atoi(ladrip_rate);
	}
	
    mac_val = atoi(ladrip_mac_begin);
    tmp_mac_val = mac_val;
    for (i=0; i<MAX_TAG_NUM && i < config_ladrip.tag_num; i++) {
        tmp_mac_val = mac_val + i;
        sprintf(str, "%06d", tmp_mac_val);

        memset(config_ladrip.tag_mac[i], 0 ,6);
        int k = strlen(str);
        for (int m=6-k; m < 6; m++)
            config_ladrip.tag_mac[i][m] = str[m]-'0';
        
    }

    for (i=0; i<MAX_TAG_NUM && i < config_ladrip.tag_num; i++) {
            fprintf(stdout, "\n-------------LADRIP MAC  %x-%x-%x-%x-%x-%x----------------\n", 
            config_ladrip.tag_mac[i][0],config_ladrip.tag_mac[i][1],config_ladrip.tag_mac[i][2],config_ladrip.tag_mac[i][3],
            config_ladrip.tag_mac[i][4],config_ladrip.tag_mac[i][5]);
    }

	if (strcmp(ewell, "1") == 0) {
		config_ewell.enable = 1;
	}
	config_ewell.tag_num = atoi(ewell_num);
    if (strcmp(ewell_data_sts, "1") == 0) {
		config_ewell.ewell_data_sts.enable = true;
	}
	if (strcmp(ewell_data_dat, "1") == 0) {
		config_ewell.ewell_data_dat.enable = true;
		config_ewell.ewell_data_dat.inter = atoi(ewell_data_dat_inter);
		config_ewell.ewell_data_dat.times = atoi(ewell_data_dat_times);
	}

	mac_val = atoi(ewell_mac_begin);
    tmp_mac_val = mac_val;
    for (i=0; i<MAX_TAG_NUM && i < config_ewell.tag_num; i++) {
        tmp_mac_val = mac_val + i;
        sprintf(str, "%08d", tmp_mac_val);

        memset(config_ewell.tag_mac[i], 0 ,8);
        int k = strlen(str);
        for (int m=8-k; m < 8; m++)
            config_ewell.tag_mac[i][m] = str[m]-'0';
        
    }

    for (i=0; i<MAX_TAG_NUM && i < config_ewell.tag_num; i++) {
            fprintf(stdout, "\n-------------EWELL MAC  %x-%x-%x-%x-%x-%x-%x-%x----------------\n", 
            config_ewell.tag_mac[i][0],config_ewell.tag_mac[i][1],config_ewell.tag_mac[i][2],config_ewell.tag_mac[i][3],
            config_ewell.tag_mac[i][4],config_ewell.tag_mac[i][5],config_ewell.tag_mac[i][6],config_ewell.tag_mac[i][7]);
    }
	
#endif

    // sensor MAC, 这里初始化了lmac，但不会被用到，因为报文已经都
    // 封装好了
    mac_val = atoi(iop_mac);
	sprintf(str, "%06d", mac_val);
    sprintf(lmac, "%02d-%02d-%02d-%02d-%02d-%02d", 
		str[0]-'0', str[1]-'0', str[2]-'0', str[3]-'0', str[4]-'0', str[5]-'0');
    //lmac[0] = lport[0];
    //lmac[1] = lport[1];
    //lmac[3] = lport[2];
    //lmac[4] = lport[3];
    //lmac[6] = lport[4];
	fprintf(stdout, "\n------------- LMAC	%s----------------\n", lmac); 
		
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
    context->serverAddr = strdup(ip);

    context->softwareVersion = strdup("AP_RGOS 11.1(5)B8");
	context->hardwareType = strdup("AP120-W 1.20");
	context->serialNumber = strdup("1234567890");

    // tag mac，随机
    context->ins_num = RSSI_INSTANCE_NUM;
    /*for (i = 0; i < 2; i++) {
        context->ins_array[i].rfid_type = RFID_SEN_T_BLE;
        context->ins_array[i].model = RFID_SEN_M_BCM20736;
        memcpy(context->ins_array[i].mac, macaddr[i], MAC_ADDR_LEN);
        context->ins_array[i].mac[4] = g_port4mac / 256;
        context->ins_array[i].mac[5] = g_port4mac % 256;
    }*/
    // reserved
    context->ins_array[0].rfid_type = /*RFID_SEN_T_MWF*/0;
    context->ins_array[0].model = /*RFID_SEN_M_NRF905*/0;
    memcpy(context->ins_array[0].mac, macaddr[0], MAC_ADDR_LEN);
    context->ins_array[0].mac[4] = mac_val / 256;
    context->ins_array[0].mac[5] = mac_val % 256;
	// kyee
	context->ins_array[1].rfid_type = /*RFID_SEN_T_MWF*/3;
    context->ins_array[1].model = /*RFID_SEN_M_NRF905*/2;
    memcpy(context->ins_array[1].mac, macaddr[1], MAC_ADDR_LEN);
    context->ins_array[1].mac[4] = mac_val / 256;
    context->ins_array[1].mac[5] = mac_val % 256;
	memcpy(config_kyee.sensor_mac, context->ins_array[1].mac, MAC_ADDR_LEN);
	// lianxin
	context->ins_array[2].rfid_type = /*RFID_SEN_T_MWF*/1;
	context->ins_array[2].model = /*RFID_SEN_M_NRF905*/0;
	memcpy(context->ins_array[2].mac, macaddr[2], MAC_ADDR_LEN);
	context->ins_array[2].mac[4] = mac_val / 256;
	context->ins_array[2].mac[5] = mac_val % 256;
	memcpy(config_ladrip.sensor_mac, context->ins_array[2].mac, MAC_ADDR_LEN);
	// ewell
	context->ins_array[3].rfid_type = /*RFID_SEN_T_MWF*/4;
    context->ins_array[3].model = /*RFID_SEN_M_NRF905*/2;
    memcpy(context->ins_array[3].mac, macaddr[3], MAC_ADDR_LEN);
    context->ins_array[3].mac[4] = mac_val / 256;
    context->ins_array[3].mac[5] = mac_val % 256;
	memcpy(config_ewell.sensor_mac, context->ins_array[3].mac, MAC_ADDR_LEN);
	srand((unsigned)getpid());


	sprintf(config_ewell.id, "%04d", atoi(test_id));
     

	sprintf(config_ewell.sn, "0000%04d11111111", rand()%10000);
	fprintf(stdout, "\n-------------EWELL ID  %x-%x-%x-%x----------------\n", 
				config_ewell.id[0], config_ewell.id[1], config_ewell.id[2], config_ewell.id[3]);
	fprintf(stdout, "\n-------------EWELL SN  %x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x----------------\n", 
				config_ewell.sn[0], config_ewell.sn[1], config_ewell.sn[2], config_ewell.sn[3],
				config_ewell.sn[4], config_ewell.sn[5], config_ewell.sn[6], config_ewell.sn[7],
				config_ewell.sn[8], config_ewell.sn[9], config_ewell.sn[10], config_ewell.sn[11],
				config_ewell.sn[12], config_ewell.sn[13], config_ewell.sn[14], config_ewell.sn[15]);
	

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
	
	int r;
	r = 60+rand()%61;
	fprintf(stdout, "\n-------------rand %u----------------\n", r);
	if (config_ladrip.enable) {
	    rg_thread_add_timer(client_global->vscp_master, 
	              ladrip_send_battery_cnt, (void *)client_global, 180);


	    if (config_ladrip.ladrip_data.e2 == 100) {
	       fprintf(stdout, "\n------------- send ladrip E5 ----------------\n");
	       rg_thread_add_timer(client_global->vscp_master, 
	              ladrip_send_e5, (void *)client_global, 1);
	    }

	    if (config_ladrip.ladrip_data.weight) {
	        fprintf(stdout, "\n------------- start ladrip data ----------------\n");
	        rg_thread_add_timer(context->vscp_master, ladrip_send_weight, (void *)context, r);
	    } else {
	        fprintf(stdout, "\n------------- stop ladrip data ----------------\n");
	    }
	}

	r = 60+rand()%61;
	fprintf(stdout, "\n-------------rand %u----------------\n", r);

	if (config_kyee.enable) {
		if(config_kyee.kyee_data_hb.enable) {
			fprintf(stdout, "\n------------- start kyee hb data ----------------\n");
        	rg_thread_add_timer(context->vscp_master, kyee_send_hb, (void *)context, r);
		}
		if(config_kyee.kyee_data_off.enable) {
			fprintf(stdout, "\n------------- start kyee off data ----------------\n");
			rg_thread_add_timer(context->vscp_master, kyee_send_off, (void *)context, r+config_kyee.kyee_data_off.inter);
		}
		if(config_kyee.kyee_data_out.enable) {
			fprintf(stdout, "\n------------- start kyee out data ----------------\n");
			rg_thread_add_timer(context->vscp_master, kyee_send_out, (void *)context, r+config_kyee.kyee_data_out.inter);
		}
	}

	r = 60+rand()%61;
	fprintf(stdout, "\n-------------rand %u----------------\n", r);

	if (config_ewell.enable) {
	    if (config_ewell.ewell_data_sts.enable) {
	       fprintf(stdout, "\n------------- start ewell sts ----------------\n");
	       rg_thread_add_timer(client_global->vscp_master, 
	              ewell_send_sts, (void *)client_global, r);
	    }

	    if (config_ewell.ewell_data_dat.enable) {
	        fprintf(stdout, "\n------------- start ewell dat ----------------\n");
	        rg_thread_add_timer(context->vscp_master, ewell_send_dat, (void *)context, 10);
	    } else {
	        fprintf(stdout, "\n------------- stop ewell dat ----------------\n");
	    }
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

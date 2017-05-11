/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/*
 * rfid_data_types.h
 * Original Author: wangkezeng@ruijie.com.cn, 2016-03-07
 * 2016-07-23  liuguoqing 该文件移植到vscp库，type和model由vscp统一管理
 *
 * RFID模块标签数据类型定义头文件
 *
 * History
 *
 */

#ifndef _RFID_TYPES_H_
#define _RFID_TYPES_H_

/* Sensor Model Defines */
typedef enum {
    RFID_SEN_M_MIN = 0,
    RFID_SEN_M_BCM20736 = RFID_SEN_M_MIN,
    RFID_SEN_M_CC2541,
    RFID_SEN_M_NRF905,
    RFID_SEN_M_BASESTATION = 100,   /* 窄带基站项目 */
    RFID_SEN_M_ACCE,                /* 加速度传感器项目 */
    RFID_SEN_M_MAX,
} RFID_SEN_M_E;

/* Sensor Type Defines */
typedef enum {
    RFID_SEN_T_INVALID = 0,
    RFID_SEN_T_BLE,                             /* 蓝牙低功耗, 2.45GHz */
    RFID_SEN_T_IBEACON,                         /* IBeacon, 2.45GHz */
    RFID_SEN_T_MWF,                             /* 微波频射频, 433.92MHz */
    RFID_SEN_T_UHF,                             /* 超高频射频，在860至960MHz范围之内 */
    RFID_SEN_T_BASESTATION = 101,
    RFID_SEN_T_ACCE,
    RFID_SEN_T_MAX,
} RFID_SEN_T_E;

#endif  /* _RFID_TYPES_H_ */


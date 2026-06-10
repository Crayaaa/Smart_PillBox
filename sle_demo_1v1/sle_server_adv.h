/*
 * Copyright (c) 2024-2025, Qi Yaolong.
 * dragon@hbu.edu.cn
 * HeBei University, China.
 * 
 * 保留所有权利。除非遵守以下条款，否则您不得使用此文件：
 * 本文件及其内容仅用于个人学习用途，在此范围之外的使用请先获得版权持有人的授权。
 *
 * 本程序由版权持有人和贡献人“按原样”提供，不提供任何明示或暗示担保，包括但不局限于对适销性和特定
 * 用途适合性的暗示担保。在任何情况下，版权持有人或贡献人对因使用本程序而导致的任何直接、间接、附带、
 * 特殊、典型或后果性损害（包括但不限于购买替代商品或服务；使用损失、数据丢失或利润损失；业务中断）
 * 不承担任何责任，无论其如何产生，也不论其责任理论为何，无论是合同、严格责任还是侵权（包括疏忽或
 * 其他），即使已告知此类损害的可能性。
 */

#ifndef SLE_SERVER_ADV_H
#define SLE_SERVER_ADV_H

// 设备公开信息数据结构中的数据子结构。
// 设备公开信息数据可以包含多个数据子结构，每个子结构承载一种类型的数据。
// SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022），6.5.1节。
typedef struct sle_adv_common_value {
    uint8_t type;   // 数据类型指示(1字节)
    uint8_t length; // 数据长度指示(1字节)
    uint8_t value;  // 数据内容部分
} sle_adv_common_t;

typedef enum sle_adv_channel {
    SLE_ADV_CHANNEL_MAP_77                 = 0x01,
    SLE_ADV_CHANNEL_MAP_78                 = 0x02,
    SLE_ADV_CHANNEL_MAP_79                 = 0x04,
    SLE_ADV_CHANNEL_MAP_DEFAULT            = 0x07
} sle_adv_channel_map_t;

typedef enum sle_adv_data {
    SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL                              = 0x01,   /* 发现等级 */
    SLE_ADV_DATA_TYPE_ACCESS_MODE                                  = 0x02,   /* 星闪接入层能力 */
    SLE_ADV_DATA_TYPE_SERVICE_DATA_16BIT_UUID                      = 0x03,   /* 标准服务数据信息 */
    SLE_ADV_DATA_TYPE_SERVICE_DATA_128BIT_UUID                     = 0x04,   /* 自定义服务数据信息 */
    SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_16BIT_SERVICE_UUIDS         = 0x05,   /* 完整标准服务标识列表 */
    SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_128BIT_SERVICE_UUIDS        = 0x06,   /* 完整自定义服务标识列表 */
    SLE_ADV_DATA_TYPE_INCOMPLETE_LIST_OF_16BIT_SERVICE_UUIDS       = 0x07,   /* 部分标准服务标识列表 */
    SLE_ADV_DATA_TYPE_INCOMPLETE_LIST_OF_128BIT_SERVICE_UUIDS      = 0x08,   /* 部分自定义服务标识列表 */
    SLE_ADV_DATA_TYPE_SERVICE_STRUCTURE_HASH_VALUE                 = 0x09,   /* 服务结构散列值 */
    SLE_ADV_DATA_TYPE_SHORTENED_LOCAL_NAME                         = 0x0A,   /* 设备缩写本地名称 */
    SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME                          = 0x0B,   /* 设备完整本地名称 */
    SLE_ADV_DATA_TYPE_TX_POWER_LEVEL                               = 0x0C,   /* 广播发送功率 */
    SLE_ADV_DATA_TYPE_SLB_COMMUNICATION_DOMAIN                     = 0x0D,   /* SLB通信域域名 */
    SLE_ADV_DATA_TYPE_SLB_MEDIA_ACCESS_LAYER_ID                    = 0x0E,   /* SLB媒体接入层标识 */
    SLE_ADV_DATA_TYPE_EXTENDED                                     = 0xFE,   /* 数据类型扩展 */
    SLE_ADV_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA                   = 0xFF    /* 厂商自定义信息 */
} sle_adv_data_type;

void sle_set_announce_name(char *name);

errcode_t sle_announce_register_cbks(void);

errcode_t sle_server_adv_init(void);

#endif

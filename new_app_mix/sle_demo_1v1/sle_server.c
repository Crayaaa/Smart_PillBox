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

#include "osal_debug.h"
#include "osal_task.h"
#include "osal_addr.h"
#include "securec.h"
#include "pinctrl.h"
#include "errcode.h"

// 海思SDK
#include "sle_common.h"                     // SLE Common API. SLE公共API
#include "sle_errcode.h"                    // SLE Error Code API. SLE错误码API
#include "sle_ssap_server.h"                // SLE Service Access Protocol Server API. SLE服务访问协议服务端API
#include "sle_device_discovery.h"           // SLE Device Announce & Seek Manager API. SLE设备公开与扫描管理API
#include "sle_connection_manager.h"         // SLE Connection Manager API. SLE连接管理API

// OpenHarmony SDK
#include "ohos_sle_common.h"                // SLE Public API. SLE公共API
#include "ohos_sle_errcode.h"               // SLE Error Code API. SLE错误码API
#include "ohos_sle_ssap_server.h"           // SLE Service Access Protocol Server API. SLE服务访问协议服务端API
#include "ohos_sle_ssap_client.h"           // SLE Service Access Protocol Client API. SLE服务访问协议客户端API
#include "ohos_sle_device_discovery.h"      // SLE Device Announce & Seek Manager API. SLE设备公开与扫描管理API
#include "ohos_sle_connection_manager.h"    // SLE Connection Manager API. SLE连接管理API

#include "sle_server.h"                     // SLE服务端接口
#include "sle_server_adv.h"                 // SLE服务端广播接口

// 宏定义
#define PROPERTY_VALUE_LEN 8                // 属性值长度8字节
#define UUID_INDEX 14                       // uuid最后两字节索引
#define UUID_16BIT_LEN 2                    // uuid长度16bit
#define UUID_128BIT_LEN 16                  // uuid长度128bit
#define SLE_ADV_HANDLE_DEFAULT 1            // 广播ID（设备公开ID/设备公开句柄），取值范围[0, 0xFF]。在sle_server_adv.c中也有定义
#define SLE_SERVER_LOG "[server]"           // 日志标签
#define SLE_UUID_SERVER_SERVICE 0x2222      // 服务UUID
#define SLE_UUID_SERVER_NTF_REPORT 0x2323   // 属性UUID

// 属性权限：可读可写
#define PROPERTY_PERMISSION (SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE)

// 操作指示：数据值可被读取，数据值可被写入，写入后产生反馈给客户端
#define OPERATION_INDICATION (SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE)

// 属性描述符权限：可读可写
#define DESCRIPTOR_PERMISSION (SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE)

// 星闪标准服务标识 基础标识（Base UUID）：37BEA880-FC70-11EA-B720-000000000000
// 参见：https://sparklink.org.cn/id/list_ssid.php
// 
// Base UUID后面6字节是媒体接入层标识（在某个网段内，分配给网络设备的用于网络通信寻址的唯一标识）
// 产品开发时，需要遵守星闪联盟团体标准T/XS 00002：媒体接入层标识分配机制
// 参见：
// https://sparklink.org.cn/id
// https://sparklink.org.cn/id/list_macid.php
static uint8_t g_sle_uuid_base[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
                                    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// SLE Server App UUID
// SLE服务端应用的UUID（通用唯一标识）。长度为2字节
// 参见：
// device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h
// device\soc\hisilicon\ws63v100\adapter\hals\communication\sle_lite\include\ohos_sle_common.h
static char g_sle_uuid_app_uuid[UUID_16BIT_LEN] = {0x12, 0x34};

// server property value.
// 属性信息结构体（SsapsPropertyInfo）中，响应的数据（value）。即属性值
static char g_sle_property_value[PROPERTY_VALUE_LEN] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

// sle connect acb handle
// SLE连接ID
static uint16_t g_sle_conn_id = 0;

// sle server handle
// SLE服务端ID
static uint8_t g_server_id = 0;

// sle service handle
// SLE服务句柄
static uint16_t g_service_handle = 0;

// sle ntf property handle
// SLE属性句柄
static uint16_t g_property_handle = 0;

// SLE连接状态。默认为未连接
static SleAcbStateType g_sle_conn_state = OH_SLE_ACB_STATE_NONE;

// SLE配对状态。默认为未配对
static SlePairStateType g_sle_pair_state = OH_SLE_PAIR_NONE;

/**
 * @brief 设置SLE UUID
 * @param u2 UUID后两字节
 * @param out SLE UUID
 */
static void sle_uuid_set(uint16_t u2, SleUuid *out)
{
    // 设置基础标识（Base UUID）：37BEA880-FC70-11EA-B720-000000000000
    errcode_t ret;
    ret = memcpy_s(out->uuid, SLE_UUID_LEN, g_sle_uuid_base, SLE_UUID_LEN); // 星闪UUID长度：16字节
    if (ret != EOK)
    {
        printf("%s [sle_uuid_set] memcpy fail\n", SLE_SERVER_LOG);
        out->len = 0;
        return;
    }

    // 设置UUID长度：2字节
    out->len = UUID_16BIT_LEN;

    // 设置UUID后两字节
    // *(uint8_t *)((&out->uuid[UUID_INDEX]) + 1) = (uint8_t)((u2) >> 0x8);
    // *(uint8_t *)(&out->uuid[UUID_INDEX]) = (uint8_t)(u2);
    out->uuid[UUID_INDEX + 1] = (uint8_t)((u2) >> 0x8);
    out->uuid[UUID_INDEX] = (uint8_t)(u2);
}

/**
 * @brief 打印SLE UUID
 * @param uuid SLE UUID
 */
static void sle_uuid_print(SleUuid *uuid)
{
    if (uuid == NULL)
    {
        printf("%s UUID is null\r\n", SLE_SERVER_LOG);
        return;
    }
    if (uuid->len == UUID_16BIT_LEN)        // UUID长度为16bit
    {
        printf("%s UUID: %02x %02x\r\n", SLE_SERVER_LOG, uuid->uuid[14], uuid->uuid[15]); // 14 15: uuid index
    }
    else if (uuid->len == UUID_128BIT_LEN)  // UUID长度为128bit
    {
        printf("%s UUID: ", SLE_SERVER_LOG);
        for (uint8_t idx = 0; idx < UUID_128BIT_LEN; idx++)
        {
            printf("%02x", uuid->uuid[idx]);
        }
        printf("\r\n");
    }
}

// 开始定义 SLE连接管理 回调函数---------------------------------------------------

/**
 * @brief callback function for connection state changed. 连接状态改变的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id    连接 ID
 * @param  [in]  addr       地址
 * @param  [in]  conn_state 连接状态
 * @param  [in]  pair_state 配对状态（指的是连接状态改变时的配对状态，例如OH_SLE_PAIR_PAIRED）
 * @param  [in]  disc_reason 断链原因
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_connect_state_changed_cbk(uint16_t conn_id, const SleAddr *addr,
                                          SleAcbStateType conn_state, SlePairStateType pair_state, SleDiscReasonType disc_reason)
{
    g_sle_conn_state = conn_state; // 更新连接状态

    // 打印参数值
    printf("%s [sle_connect_state_changed_cbk] conn id:0x%02x, conn state:0x%x, pair state:0x%x, "
           "disc reason:0x%x, addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
           SLE_SERVER_LOG, conn_id, conn_state, pair_state, disc_reason,
           addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3], addr->addr[4], addr->addr[5]);

    // 连接状态改变为已连接
    if (conn_state == OH_SLE_ACB_STATE_CONNECTED)
    {
        g_sle_conn_id = conn_id;    // 保存SLE连接ID
    }

    // 连接状态改变为已断开
    else if (conn_state == OH_SLE_ACB_STATE_DISCONNECTED)
    {
        g_sle_conn_id = 0;                   // SLE连接ID清零
        g_sle_pair_state = OH_SLE_PAIR_NONE; // 配对状态改变为未配对

        // 开始广播（开始设备公开）
        // 参数：广播ID（设备公开ID/设备公开句柄）
        // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
        // sle_start_announce接口
        errcode_t ret = SleStartAnnounce(SLE_ADV_HANDLE_DEFAULT);
        if (ret != ERRCODE_SLE_SUCCESS)
        {
            printf("%s [sle_connect_state_changed_cbk] sle start announce fail :%x\r\n", SLE_SERVER_LOG, ret);
        }
    }
}

/**
 * @brief callback function for pair complete. 配对完成的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id 连接 ID。
 * @param  [in]  addr    地址。
 * @param  [in]  status  执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_pair_complete_cbk(uint16_t conn_id, const SleAddr *addr, errcode_t status)
{
    // 打印参数值
    printf("%s [sle_pair_complete_cbk] conn id:%02x, status:%x, addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
           SLE_SERVER_LOG, conn_id, status,
           addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3], addr->addr[4], addr->addr[5]);
    
    // 配对成功
    if (status == ERRCODE_SUCC)
    {
        g_sle_pair_state = OH_SLE_PAIR_PAIRED;  // 配对状态改变为已配对
    }
}

/**
 * @brief  连接参数更新请求完成前的回调函数。
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id    连接 ID。
 * @param  [in]  status     执行结果错误码。
 * @param  [in]  param      连接参数。
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_connect_param_update_req_cbk(uint16_t conn_id, errcode_t status, const sle_connection_param_update_req_t *param)
{
    // 打印参数值
    printf("%s [sle_connect_param_update_req_cbk] conn id:%02x, status:%x, "
           "interval min:%02x, interval max:%02x, max latency:%02x, supervision timeout:%02x\r\n",
           SLE_SERVER_LOG, conn_id, status, param->interval_min, param->interval_max,
           param->max_latency, param->supervision_timeout);
}

/**
 * @brief  连接参数更新的回调函数。
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id    连接 ID。
 * @param  [in]  status     执行结果错误码。
 * @param  [in]  param      连接参数。
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_connect_param_update_cbk(uint16_t conn_id, errcode_t status, const SleConnectionParamUpdateEvt *param)
{
    // 打印参数值
    printf("%s [sle_connect_param_update_cbk] conn id:%02x, status:%x, interval:%02x, latency:%02x, supervision:%02x\r\n",
           SLE_SERVER_LOG, conn_id, status, param->interval, param->latency, param->supervision);
}

/**
 * @brief  认证完成的回调函数。
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id 连接 ID。
 * @param  [in]  addr    地址。
 * @param  [in]  status  执行结果错误码。
 * @param  [in]  evt     认证事件。
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_auth_complete_cbk(uint16_t conn_id, const SleAddr *addr, errcode_t status, const sle_auth_info_evt_t *evt)
{
    // 打印参数值
    printf("%s [sle_auth_complete_cbk] conn id:%02x, status:%x, addr:%02x:%02x:%02x:%02x:%02x:%02x, "
           "link key:%02x, crypto algo:%x, key deriv algo:%x, integr chk ind:%x\r\n",
           SLE_SERVER_LOG, conn_id, status,
           addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3], addr->addr[4], addr->addr[5],
           evt->link_key, evt->crypto_algo, evt->key_deriv_algo, evt->integr_chk_ind);
}

/**
 * @brief  读取rssi的回调函数。
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id 连接 ID。
 * @param  [in]  rssi    rssi。
 * @param  [in]  status  执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_read_rssi_cbk(uint16_t conn_id, int8_t rssi, errcode_t status)
{
    // 打印参数值
    printf("%s [sle_read_rssi_cbk] conn id:%02x, rssi:%d, status:%x\r\n",
           SLE_SERVER_LOG, conn_id, rssi, status);
}

/**
 * @brief register callback functions for connection. 注册SLE连接管理回调函数
 */
static errcode_t sle_conn_register_cbks(void)
{
    SleConnectionCallbacks conn_cbks = {0};                               // SLE连接管理回调函数接口
    conn_cbks.connectStateChangedCb = sle_connect_state_changed_cbk;      // 连接状态改变的回调函数
    conn_cbks.connectParamUpdateReqCb = sle_connect_param_update_req_cbk; // 连接参数更新请求完成前的回调函数
    conn_cbks.connectParamUpdateCb = sle_connect_param_update_cbk;        // 连接参数更新的回调函数
    conn_cbks.authCompleteCb = sle_auth_complete_cbk;                     // 认证完成的回调函数
    conn_cbks.pairCompleteCb = sle_pair_complete_cbk;                     // 配对完成的回调函数
    conn_cbks.readRssiCb = sle_read_rssi_cbk;                             // 读取rssi的回调函数

    // 注册 SLE连接管理 回调函数
    return SleConnectionRegisterCallbacks(&conn_cbks);
}

// 结束定义 SLE连接管理 回调函数---------------------------------------------------

// 开始定义 SSAP服务端 回调函数---------------------------------------------------

/**
 * @brief callback function for mtu changed. mtu大小改变的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  server_id 服务端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  mtu_size  mtu 大小。
 * @param  [in]  status    执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_mtu_changed_cbk(uint8_t server_id, uint16_t conn_id, SsapcExchangeInfo *mtu_size,
                                  errcode_t status)
{
    // 打印参数值
    printf("%s [ssaps_mtu_changed_cbk] server id:%x, conn id:%x, mtu size:%x, status:%x\r\n",
           SLE_SERVER_LOG, server_id, conn_id, mtu_size->mtuSize, status);
}

/**
 * @brief callback function for start service. 启动服务的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  server_id 服务端 ID。
 * @param  [in]  handle    服务属性句柄。
 * @param  [in]  status    执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_start_service_cbk(uint8_t server_id, uint16_t handle, errcode_t status)
{
    // 打印参数值
    printf("%s [ssaps_start_service_cbk] server id:%d, handle:%x, status:%x\r\n",
           SLE_SERVER_LOG, server_id, handle, status);
}

/**
 * @brief callback function for add service. 添加服务（服务注册）的回调函数。目前版本SDK不支持该回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  server_id 服务端 ID。
 * @param  [in]  uuid      服务uuid。
 * @param  [in]  handle    服务属性句柄。
 * @param  [in]  status    执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_add_service_cbk(uint8_t server_id, SleUuid *uuid, uint16_t handle, errcode_t status)
{
    // 打印参数值
    printf("%s [ssaps_add_service_cbk] server id:%x, handle:%x, status:%x\r\n",
           SLE_SERVER_LOG, server_id, handle, status);
    sle_uuid_print(uuid);
}

/**
 * @brief callback function for add property. 添加特征（特征注册）的回调函数。目前版本SDK不支持该回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  server_id      服务端 ID。
 * @param  [in]  uuid           特征 uuid。
 * @param  [in]  service_handle 服务属性句柄。
 * @param  [in]  handle         特征属性句柄。
 * @param  [in]  status         执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_add_property_cbk(uint8_t server_id, SleUuid *uuid, uint16_t service_handle,
                                   uint16_t handle, errcode_t status)
{
    // 打印参数值
    printf("%s [ssaps_add_property_cbk] server id:%x, service handle:%x, handle:%x, status:%x\r\n",
           SLE_SERVER_LOG, server_id, service_handle, handle, status);
    sle_uuid_print(uuid);
}

/**
 * @brief callback function for add descriptor. 添加描述符（特征描述符注册）的回调函数。目前版本SDK不支持该回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  server_id       服务端 ID。
 * @param  [in]  uuid            特征 uuid。
 * @param  [in]  service_handle  服务属性句柄。
 * @param  [in]  property_handle 特征描述符属性句柄。
 * @param  [in]  status          执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_add_descriptor_cbk(uint8_t server_id, SleUuid *uuid, uint16_t service_handle,
                                     uint16_t property_handle, errcode_t status)
{
    // 打印参数值
    printf("%s [ssaps_add_descriptor_cbk] server id:%x, service handle:%x, property handle:%x, status:%x\r\n",
           SLE_SERVER_LOG, server_id, service_handle, property_handle, status);
    sle_uuid_print(uuid);
}

/**
 * @brief callback function for delete all service. 删除全部服务的回调函数。目前版本SDK不支持该回调函数
 * @attention  1. 该回调函数运行于bts线程，不能阻塞或长时间等待。
 * @attention  2. devices由bts申请内存，也由bts释放，回调中不应释放。
 * @param  [in]  server_id 服务端 ID。
 * @param  [in]  status    执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_delete_all_service_cbk(uint8_t server_id, errcode_t status)
{
    // 打印参数值
    printf("%s [ssaps_delete_all_service_cbk] server id:%x, status:%x\r\n",
           SLE_SERVER_LOG, server_id, status);
}

/**
 * @brief callback function for read request. 收到远端读请求的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  server_id    服务端 ID。
 * @param  [in]  conn_id      连接 ID。
 * @param  [in]  read_cb_para 读请求参数。
 * @param  [in]  status       执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h （该文件中此接口第一个参数定义有误，应该是serverId）
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_read_request_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_read_cb_t *read_cb_para,
                            errcode_t status)
{
    // 打印参数值
    printf("%s [ssaps_read_request_cbk] server id:0x%x, conn id:0x%x, handle:0x%x, type:0x%x, status:0x%x\r\n",
           SLE_SERVER_LOG, server_id, conn_id, read_cb_para->handle, read_cb_para->type, status);
}

/**
 * @brief callback function for write request. 收到远端写请求的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  server_id     服务端 ID。
 * @param  [in]  conn_id       连接 ID。
 * @param  [in]  write_cb_para 写请求参数。
 * @param  [in]  status        执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h （该文件中此接口第一个参数定义有误，应该是serverId）
 * @see  device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_write_request_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_write_cb_t *write_cb_para,
                             errcode_t status)
{
    (void)server_id;
    (void)conn_id;
    (void)status;
    
    write_cb_para->value[write_cb_para->length - 1] = '\0';     // 放置字符串结束符
    printf("%s [ssaps_write_request_cbk] receive client write request: %s\r\n", SLE_SERVER_LOG, write_cb_para->value);

    // 向SLE客户端发送响应数据
    char *data = "Response from SLE server!\n";
    int ret = sle_server_send_data((uint8_t *)data, strlen(data));
}

/**
 * @brief register callback functions for ssaps. 注册SSAP服务端回调函数
 */
static errcode_t sle_ssaps_register_cbks(void)
{
    SsapsCallbacks ssaps_cbk = {0};                              // SSAP服务端 回调函数接口
    ssaps_cbk.addServiceCb = ssaps_add_service_cbk;              // 添加服务（服务注册）的回调函数
    ssaps_cbk.addPropertyCb = ssaps_add_property_cbk;            // 添加特征（特征注册）的回调函数
    ssaps_cbk.addDescriptorCb = ssaps_add_descriptor_cbk;        // 添加描述符（特征描述符注册）的回调函数
    ssaps_cbk.startServiceCb = ssaps_start_service_cbk;          // 启动服务的回调函数
    ssaps_cbk.deleteAllServiceCb = ssaps_delete_all_service_cbk; // 删除全部服务的回调函数
    ssaps_cbk.mtuChangedCb = ssaps_mtu_changed_cbk;              // mtu大小改变的回调函数
    ssaps_cbk.readRequestCb = ssaps_read_request_cbk;            // 收到远端读请求的回调函数
    ssaps_cbk.writeRequestCb = ssaps_write_request_cbk;          // 收到远端写请求的回调函数

    // 注册 SSAP服务端 回调函数
    return SsapsRegisterCallbacks(&ssaps_cbk);
}

// 结束定义 SSAP服务端 回调函数---------------------------------------------------

// 开始定义 添加SLE服务端 相关函数-------------------------------------------------

/**
 * @brief add service for sle server. 添加SLE服务端服务
 */
static errcode_t sle_server_add_service(void)
{
    errcode_t ret;
    SleUuid service_uuid = {0};

    // 设置SSAP服务UUID
    sle_uuid_set(SLE_UUID_SERVER_SERVICE, &service_uuid);

    // 添加一个SSAP服务
    // 参数包括：服务端ID、服务UUID、是否是首要(primary)服务、服务句柄
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_add_service_sync接口
    ret = SsapsAddServiceSync(g_server_id, &service_uuid, 1, &g_service_handle);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s sle uuid add service fail, ret:%x\r\n", SLE_SERVER_LOG, ret);
        return ERRCODE_SLE_FAIL;
    }

    return ERRCODE_SLE_SUCCESS;
}

/**
 * @brief add property and property descriptor for sle server. 添加SLE服务端属性和属性描述符
 */
static errcode_t sle_server_add_property(void)
{
    errcode_t ret;
    SsapsPropertyInfo property = {0};   // 属性
    SsapsDescInfo descriptor = {0};     // 属性描述符
    uint8_t ntf_value[] = {0x01, 0x02}; // 属性描述符的数据

    // 属性---------------------------------------------------

    sle_uuid_set(SLE_UUID_SERVER_NTF_REPORT, &property.uuid); // 属性UUID
    property.permissions = PROPERTY_PERMISSION;               // 属性权限
    property.operateIndication = OPERATION_INDICATION;        // 操作指示
    property.valueLen = PROPERTY_VALUE_LEN;                   // 响应的数据长度

    // 响应的数据
    property.value = (uint8_t *)osal_vmalloc(sizeof(g_sle_property_value));
    if (property.value == NULL)
    {
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(property.value, sizeof(g_sle_property_value), g_sle_property_value,
                 sizeof(g_sle_property_value)) != EOK)
    {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }

    // 添加一个属性
    // 参数包括：服务端ID、服务句柄、属性信息、属性句柄
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_add_property_sync接口
    ret = SsapsAddPropertySync(g_server_id, g_service_handle, &property, &g_property_handle);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s add property fail, ret: %x\r\n", SLE_SERVER_LOG, ret);
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }

    // 属性描述符------------------------------------------------

    descriptor.permissions = DESCRIPTOR_PERMISSION;         // 属性描述符权限
    descriptor.operateIndication = OPERATION_INDICATION;    // 操作指示
    descriptor.type = SSAP_DESCRIPTOR_CLIENT_CONFIGURATION; // 属性描述符类型：客户端配置描述符

    // 数据
    descriptor.value = (uint8_t *)osal_vmalloc(sizeof(ntf_value));
    if (descriptor.value == NULL)
    {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(descriptor.value, sizeof(ntf_value), ntf_value, sizeof(ntf_value)) != EOK)
    {
        osal_vfree(property.value);
        osal_vfree(descriptor.value);
        return ERRCODE_SLE_FAIL;
    }

    // 添加一个属性描述符
    // 参数包括：服务端ID、服务句柄、描述符所在的属性句柄、属性描述符
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_add_descriptor_sync接口
    ret = SsapsAddDescriptorSync(g_server_id, g_service_handle, g_property_handle, &descriptor);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s add descriptor fail, ret: %x\r\n", SLE_SERVER_LOG, ret);
        osal_vfree(property.value);
        osal_vfree(descriptor.value);
        return ERRCODE_SLE_FAIL;
    }

    osal_vfree(property.value);
    osal_vfree(descriptor.value);
    return ERRCODE_SLE_SUCCESS;
}

/**
 * @brief 添加SLE服务端、服务、属性和属性描述符
 */
static errcode_t sle_server_add(void)
{
    errcode_t ret;

    // 注册SLE服务端
    SleUuid app_uuid = {0};  // SLE服务端应用的UUID（通用唯一标识）
    app_uuid.len = sizeof(g_sle_uuid_app_uuid);
    if (memcpy_s(app_uuid.uuid, app_uuid.len, g_sle_uuid_app_uuid, sizeof(g_sle_uuid_app_uuid)) != EOK)
    {
        return ERRCODE_SLE_FAIL;
    }
    // 注册SSAP服务端，参数包括：应用UUID、服务端ID
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_register_server接口
    ssapsRegisterServer(&app_uuid, &g_server_id);

    // 添加SLE服务端服务
    if (sle_server_add_service() != ERRCODE_SLE_SUCCESS)
    {
        SsapsUnregisterServer(g_server_id); // 注销SSAP服务端
        return ERRCODE_SLE_FAIL;
    }

    // 添加SLE服务端属性和属性描述符
    if (sle_server_add_property() != ERRCODE_SLE_SUCCESS)
    {
        SsapsUnregisterServer(g_server_id); // 注销SSAP服务端
        return ERRCODE_SLE_FAIL;
    }

    printf("%s [sle_server_add] server id:%x, service handle:%x, property handle:%x\r\n",
           SLE_SERVER_LOG, g_server_id, g_service_handle, g_property_handle);

    // 启动SLE服务端服务（开始一个SSAP服务）
    // 参数包括：服务端ID、服务句柄
    // 服务开启结果将在 ssaps_start_service_callback 中返回
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_start_service接口
    ret = SsapsStartService(g_server_id, g_service_handle);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_add] start service fail, ret: %x\r\n", SLE_SERVER_LOG, ret);
        return ERRCODE_SLE_FAIL;
    }

    return ERRCODE_SLE_SUCCESS;
}

// 结束定义 添加SLE服务端 相关函数-------------------------------------------------

/**
 * @brief initialize sle server. 初始化SLE服务端
 * @return 错误码
 */
errcode_t sle_server_init(void)
{
    errcode_t ret;  // 错误码

    // 注册SLE设备公开回调函数
    ret = sle_announce_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] sle_announce_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    // 注册SLE连接管理回调函数
    ret = sle_conn_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] sle_conn_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    // 注册SSAP服务端回调函数
    ret = sle_ssaps_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] sle_ssaps_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    // 使能SLE协议栈（开启SLE）
    ret = EnableSle();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] enable_sle fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    // 添加SLE服务端、服务、属性和属性描述符
    ret = sle_server_add();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] sle_server_add fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    // 初始化SLE服务端的广播参数、广播数据和扫描响应数据，然后启动广播
    ret = sle_server_adv_init();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] sle_server_adv_init fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    printf("%s [sle_server_init] init ok\r\n", SLE_SERVER_LOG);
    return ERRCODE_SLE_SUCCESS;
}

/**
 * @brief 服务端通过property handle（属性句柄）向客户端发送数据
 * @param data 待发送的数据
 * @param len 数据长度
 * @return 错误码
 */
static errcode_t sle_server_send_notify_by_handle(const uint8_t *data, uint8_t len)
{
    // notification/indication information. 通知或指示信息
    SsapsNtfInd param = {0};

    // 属性句柄
    param.handle = g_property_handle;

    // 属性类型：SSAP_PROPERTY_TYPE_VALUE（属性值）
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h中的
    // ssap_property_type_t
    param.type = SSAP_PROPERTY_TYPE_VALUE;

    // 通知/指示 数据长度
    param.valueLen = len + 1; // 容纳字符串结束符。非星闪协议规定

    // 通知/指示 数据内容
    param.value = osal_vmalloc(param.valueLen);
    if (param.value == NULL)
    {
        printf("[sle_server_send_notify_by_handle] osal_vmalloc fail\r\n");
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(param.value, param.valueLen, data, len) != EOK)
    {
        printf("[sle_server_send_notify_by_handle] memcpy_s fail\r\n");
        osal_vfree(param.value);
        return ERRCODE_SLE_FAIL;
    }

    // 向对端发送通知或指示
    // 具体发送状态取决于属性描述符：客户端属性配置
    // value = 0x0000：不允许通知和指示
    // value = 0x0001：允许通知
    // value = 0x0002：允许指示
    // 如果向全部对端发送则conn_id = 0xffff
    // 参见：
    // device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_notify_indicate接口
    if (SsapsNotifyIndicate(g_server_id, g_sle_conn_id, &param) != ERRCODE_SUCC)
    {
        printf("[sle_server_send_notify_by_handle] ssaps notify indicate fail\r\n");
        osal_vfree(param.value);
        return ERRCODE_SLE_FAIL;
    }

    osal_vfree(param.value);
    return ERRCODE_SLE_SUCCESS;
}

/**
 * @brief 服务端向客户端发送数据
 * @param data 待发送数据
 * @param length 数据长度
 * @return 错误码
 */
int sle_server_send_data(uint8_t *data, uint8_t length)
{
    // 二次封装是为了以后扩展便利。目前是调用sle_server_send_notify_by_handle接口
    int ret;
    ret = sle_server_send_notify_by_handle(data, length);   // 服务端通过property handle（属性句柄）向客户端发送数据
    return ret;
}

/**
 * @brief 等待客户端连接完成
 */
void sle_wait_client_connected(void)
{
    while (g_sle_conn_state != OH_SLE_ACB_STATE_CONNECTED)
    {
        osal_msleep(100); // 轮询间隔100ms
    }
}

/**
 * @brief 等待客户端配对完成
 */
void sle_wait_client_paired(void)
{
    while (g_sle_pair_state != OH_SLE_PAIR_PAIRED)
    {
        osal_msleep(100); // 轮询间隔100ms
    }
}

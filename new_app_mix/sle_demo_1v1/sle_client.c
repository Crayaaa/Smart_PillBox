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

#include "string.h"
#include "common_def.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "securec.h"
#include "pinctrl.h"
#include "errcode.h"

// 海思SDK
#include "sle_common.h"                     // SLE Common API. SLE公共API
#include "sle_errcode.h"                    // SLE Error Code API. SLE错误码API
#include "sle_ssap_client.h"                // SLE Service Access Protocol Client API. SLE服务访问协议客户端API
#include "sle_device_discovery.h"           // SLE Device Announce & Seek Manager API. SLE设备公开与扫描管理API
#include "sle_connection_manager.h"         // SLE Connection Manager API. SLE连接管理API

// OpenHarmony SDK
#include "ohos_sle_common.h"                // SLE Public API. SLE公共API
#include "ohos_sle_errcode.h"               // SLE Error Code API. SLE错误码API
#include "ohos_sle_ssap_server.h"           // SLE Service Access Protocol Server API. SLE服务访问协议服务端API
#include "ohos_sle_ssap_client.h"           // SLE Service Access Protocol Client API. SLE服务访问协议客户端API
#include "ohos_sle_device_discovery.h"      // SLE Device Announce & Seek Manager API. SLE设备公开与扫描管理API
#include "ohos_sle_connection_manager.h"    // SLE Connection Manager API. SLE连接管理API

#include "sle_client.h"

// 宏定义
#define SERVER_NAME_MAX_LENGTH 16     // 要连接的SLE服务端名称的最大长度
#define SLE_MTU_SIZE_DEFAULT 512      // 默认MTU大小（在ssap信息交换结构体中）
#define SLE_SEEK_INTERVAL_DEFAULT 100 // 扫描间隔。单位0.125ms，取值范围[0x0004, 0xFFFF]
#define SLE_SEEK_WINDOW_DEFAULT 100   // 扫描窗口。单位0.125ms，取值范围[0x0004, 0xFFFF]
#define UUID_16BIT_LEN 2              // UUID长度16bit
#define UUID_128BIT_LEN 16            // UUID长度128bit
#define SLE_CLIENT_LOG "[client]"     // 日志标签

// SLE Client App UUID
// SLE客户端应用的UUID（通用唯一标识）。长度为16字节
// 参见：
// device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h
// device\soc\hisilicon\ws63v100\adapter\hals\communication\sle_lite\include\ohos_sle_common.h
// 
// 星闪标准服务标识 基础标识（Base UUID）：37BEA880-FC70-11EA-B720-000000000000
// 参见：https://sparklink.org.cn/id/list_ssid.php
// 
// Base UUID后面6字节是媒体接入层标识（在某个网段内，分配给网络设备的用于网络通信寻址的唯一标识）
// 产品开发时，需要遵守星闪联盟团体标准T/XS 00002：媒体接入层标识分配机制
// 参见：
// https://sparklink.org.cn/id
// https://sparklink.org.cn/id/list_macid.php
static char g_sle_uuid_app_uuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
                                     0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Service discovery result
// 服务发现结果
// SSAP : SLE Service Access Protocol，SLE服务访问协议
// SSAPC: SLE Service Access Protocol Client，SLE服务访问协议客户端
static ssapc_find_service_result_t g_sle_find_service_result = {0};

// SLE device announce & seek callback function
// SLE设备公开与扫描 回调函数接口
static sle_announce_seek_callbacks_t g_sle_seek_cbk = {0};

// SLE connection manager callback function
// SLE连接管理 回调函数接口
static SleConnectionCallbacks g_sle_connect_cbk = {0};

// SSAP Client callback function
// SSAP客户端 回调函数接口
static ssapc_callbacks_t g_sle_ssapc_cbk = {0};

// SLE remote address
// SLE远程设备（对端设备）地址
static SleAddr g_sle_remote_addr = {0};

// SLE connection state
// SLE连接状态。1: connected, 0: disconnected
static uint8_t g_sle_connected = 0;

// SLE service found state
// SLE服务发现状态。0: not found, 1: found
static uint8_t g_sle_service_found = 0;

// SLE connection id
// SLE连接ID
static uint16_t g_sle_conn_id = 0;

// SLE client id
// SLE客户端ID。本案例设置为0
static uint8_t g_client_id = 0;

// SLE local address
// 本地设备地址。每个设备的地址不同，需要根据实际情况修改，或者使用随机地址
static uint8_t g_sle_local_addr[SLE_ADDR_LEN] = {0};

// SLE server name
// 要连接的SLE服务端名称（广播设备名称）
static uint8_t g_sle_server_name[SERVER_NAME_MAX_LENGTH] = "";

/**
 * @brief Set SLE server name. 设置要连接的SLE服务端名称
 * @param name SLE server name.
 */
void sle_set_server_name(uint8_t *name)
{
    memset(g_sle_server_name, 0, sizeof(g_sle_server_name));
    errno_t ret = memcpy_s(g_sle_server_name, strlen(name), name, strlen(name));
    if (ret != EOK)
    {
        printf("[sle_set_server_name] memcpy fail\r\n");
    }
}

/**
 * @brief Get SLE connection state. 获取SLE连接状态
 * @return 1: connected, 0: disconnected.
 */
uint8_t sle_is_connected(void)
{
    return g_sle_connected;
}

/**
 * @brief Get SLE connection id. 获取SLE连接ID
 * @return SLE connection id.
 */
uint16_t sle_get_conn_id(void)
{
    return g_sle_conn_id;
}

/**
 * @brief set device seek parameter, then start device seek.
 * 设置 设备公开 扫描参数，并开始 设备公开 扫描
 */
static void sle_start_scan(void)
{
    // Seek scan parameter. 设备发现扫描参数
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_param_t
    SleSeekParam param = {0};

    // 设置本端SLE地址类型
    // SLE地址类型：
    // SLE_ADDRESS_TYPE_PUBLIC：公有地址
    // SLE_ADDRESS_TYPE_RANDOM：随机地址
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_common.h中的
    // sle_addr_type_t
    param.ownaddrtype = SLE_ADDRESS_TYPE_PUBLIC;

    // 设置重复过滤开关
    // 0：关闭，1：开启
    param.filterduplicates = 0;

    // 设置设备发现过滤类型（扫描设备使用的过滤类型）
    // SLE_SEEK_FILTER_ALLOW_ALL：允许来自任何人的设备发现数据包
    // SLE_SEEK_FILTER_ALLOW_WLST：只允许来自白名单设备的设备发现数据包，预留
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_filter_t
    param.seekfilterpolicy = SLE_SEEK_FILTER_ALLOW_ALL;

    // 设置扫描设备所使用的PHY（物理层）类型，即设置通信带宽
    // SLE_SEEK_PHY_1M: 1M PHY
    // SLE_SEEK_PHY_2M: 2M PHY
    // SLE_SEEK_PHY_4M: 4M PHY
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_phy_t
    param.seekphys = SLE_SEEK_PHY_1M;

    // 设置扫描类型
    // SLE_SEEK_PASSIVE: 被动扫描
    // SLE_SEEK_ACTIVE: 主动扫描
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_type_t
    param.seekType[0] = SLE_SEEK_ACTIVE;

    // 设置扫描间隔
    // 单位0.125ms，取值范围[0x0004, 0xFFFF]
    param.seekInterval[0] = SLE_SEEK_INTERVAL_DEFAULT;

    // 设置扫描窗口
    // 单位0.125ms，取值范围[0x0004, 0xFFFF]
    param.seekWindow[0] = SLE_SEEK_WINDOW_DEFAULT;

    // 设置 设备公开 扫描参数
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_set_seek_param接口
    SleSetSeekParam(&param);

    // 开始 设备公开 扫描
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_start_seek接口
    SleStartSeek();
}

// 开始定义 SLE设备公开与扫描 回调函数--------------------------------------------

/**
 * @brief callback function for enable sle. SLE协议栈使能的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  status 执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_client_sle_enable_cbk(errcode_t status)
{
    if (status != ERRCODE_SUCC)
    {
        osal_printk("%s [sle_client_sle_enable_cbk] status error\r\n", SLE_CLIENT_LOG);
    }
    else
    {
        // 设置 设备公开 扫描参数，并开始 设备公开 扫描
        // 如果不调用SleStopSeek()，则一直扫描
        sle_start_scan();
    }
}

/**
 * @brief callback function for enable seek. 扫描使能的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  status 执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_client_seek_enable_cbk(errcode_t status)
{
    if (status != ERRCODE_SUCC)
    {
        osal_printk("%s [sle_client_seek_enable_cbk] status error\r\n", SLE_CLIENT_LOG);
    }
}

/**
 * @brief callback function for seek result. 扫描结果上报的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  seek_result_data 扫描结果数据。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_client_seek_result_info_cbk(SleSeekResultInfo *seek_result_data)
{
    // 扫描结果数据为空
    if (seek_result_data == NULL)
    {
        osal_printk("%s [sle_client_seek_result_info_cbk] seek_result_data is NULL\r\n", SLE_CLIENT_LOG);
        return;
    }

    // 目标设备（要连接的SLE服务端）
    // SLE服务端的设备公开数据有自己的结构，其中包含了服务端名称。
    // 在后面的"sle_announce_customize"案例中会详细介绍设备公开数据。
    // 本案例只是使用子串查找的方式，简单的判断设备公开数据中是否包含了要连接的SLE服务端名称。
    // 这种方式虽然不精准，但能够避免由于过早接触太多技术细节，导致理解困难。
    if (strstr((const char *)seek_result_data->data, g_sle_server_name) != NULL)
    {
        // 打印扫描结果中的SLE设备地址
        osal_printk("%s [sle_client_seek_result_info_cbk] target found, addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    SLE_CLIENT_LOG,
                    seek_result_data->addr.addr[0], seek_result_data->addr.addr[1],
                    seek_result_data->addr.addr[2], seek_result_data->addr.addr[3],
                    seek_result_data->addr.addr[4], seek_result_data->addr.addr[5]);

        // 保存目标设备地址
        memcpy_s(&g_sle_remote_addr, sizeof(sle_addr_t), &seek_result_data->addr, sizeof(sle_addr_t));

        // 停止 设备公开 扫描
        // 本案例的SLE客户端只需要连接一个SLE服务端，所以找到目标设备后，停止扫描
        SleStopSeek();
    }
}

/**
 * @brief callback function for disable seek. 扫描关闭的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  status 执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_client_seek_disable_cbk(errcode_t status)
{
    if (status != ERRCODE_SUCC)
    {
        osal_printk("%s [sle_client_seek_disable_cbk] status error: %x\r\n", SLE_CLIENT_LOG, status);
    }
    else
    {
        // 向目标设备发起连接请求
        SleConnectRemoteDevice(&g_sle_remote_addr);
    }
}

/**
 * @brief register callback functions for seek. 注册SLE设备扫描回调函数
 */
static void sle_client_seek_cbk_register(void)
{
    g_sle_seek_cbk.sle_enable_cb = sle_client_sle_enable_cbk;        // SLE协议栈使能的回调函数
    g_sle_seek_cbk.seek_enable_cb = sle_client_seek_enable_cbk;      // 扫描使能的回调函数
    g_sle_seek_cbk.seek_result_cb = sle_client_seek_result_info_cbk; // 扫描结果上报的回调函数
    g_sle_seek_cbk.seek_disable_cb = sle_client_seek_disable_cbk;    // 扫描关闭的回调函数

    // 注册 SLE设备公开与扫描 回调函数
    sle_announce_seek_register_callbacks(&g_sle_seek_cbk);
}

// 结束定义 SLE设备公开与扫描 回调函数---------------------------------------------

// 开始定义 SLE连接管理 回调函数--------------------------------------------------

/**
 * @brief callback function for connection state changed. 连接状态改变的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id    连接 ID
 * @param  [in]  addr       SLE远程设备（对端设备）地址
 * @param  [in]  conn_state 连接状态
 * @param  [in]  pair_state 配对状态
 * @param  [in]  disc_reason 断链原因
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_client_connect_state_changed_cbk(uint16_t conn_id, const SleAddr *addr,
                                                 SleAcbStateType conn_state, SlePairStateType pair_state,
                                                 SleDiscReasonType disc_reason)
{
    // 打印连接状态信息
    osal_printk("%s [sle_client_connect_state_changed_cbk] conn id:0x%02x, conn state:0x%x, pair state:0x%x, "
                "disc reason:0x%x, remote addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                SLE_CLIENT_LOG, conn_id, conn_state, pair_state, disc_reason, 
                addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3], addr->addr[4], addr->addr[5]);

    // 连接状态改变为已连接
    if (conn_state == SLE_ACB_STATE_CONNECTED)
    {
        // 保存SLE连接ID
        g_sle_conn_id = conn_id;

        // 设置SLE连接状态
        g_sle_connected = 1;

        // 配对远程设备
        if (pair_state == SLE_PAIR_NONE)
        {
            osal_printk("%s [sle_client_connect_state_changed_cbk] connected! start pair...\r\n", SLE_CLIENT_LOG);
            SlePairRemoteDevice(addr);
        }
    }

    // 连接状态改变为已断开
    else if (conn_state == SLE_ACB_STATE_DISCONNECTED)
    {
        // 设置 设备公开 扫描参数，并开始 设备公开 扫描
        osal_printk("%s [sle_client_connect_state_changed_cbk] disconnected! start scan...\r\n", SLE_CLIENT_LOG);
        sle_start_scan();
    }

    // 连接状态改变为未连接
    else if (conn_state == SLE_ACB_STATE_NONE)
    {
        osal_printk("%s [sle_client_connect_state_changed_cbk] state none.\r\n", SLE_CLIENT_LOG);
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
static void sle_client_pair_complete_cbk(uint16_t conn_id, const SleAddr *addr, errcode_t status)
{
    // 打印配对信息
    osal_printk("%s [sle_client_pair_complete_cbk] pair complete. conn id:%02x, status:%x, "
                "remote addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                SLE_CLIENT_LOG, conn_id, status,
                addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3], addr->addr[4], addr->addr[5]);

    // 配对成功
    if (status == ERRCODE_SUCC)
    {
        // 交换信息（Exchange Info），主要是协商MTU大小
        SsapcExchangeInfo info = {0};        // 定义SSAP交换信息
        info.mtuSize = SLE_MTU_SIZE_DEFAULT; // 设置MTU大小
        info.version = 1;                    // 设置版本

        // 发送交换信息请求，MTU改变结果将在ssapc_exchange_info_callback中返回
        // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
        // ssapc_exchange_info_req接口
        osal_printk("%s [sle_client_pair_complete_cbk] paird! start exchange info...\r\n", SLE_CLIENT_LOG);
        SsapcExchangeInfoReq(1, conn_id, &info); // 此处没有使用默认的client ID 0。原值：0
    }
}

/**
 * @brief register callback functions for connection. 注册SLE连接管理回调函数
 */
static void sle_client_connect_cbk_register(void)
{

    g_sle_connect_cbk.connectStateChangedCb = sle_client_connect_state_changed_cbk; // 连接状态改变的回调函数
    g_sle_connect_cbk.connectParamUpdateReqCb = NULL;                               // 连接参数更新请求的回调函数
    g_sle_connect_cbk.connectParamUpdateCb = NULL;                                  // 连接参数更新的回调函数
    g_sle_connect_cbk.authCompleteCb = NULL;                                        // 认证完成的回调函数
    g_sle_connect_cbk.pairCompleteCb = sle_client_pair_complete_cbk;                // 配对完成的回调函数
    g_sle_connect_cbk.readRssiCb = NULL;                                            // 读取rssi的回调函数

    // 注册 SLE连接管理 回调函数
    SleConnectionRegisterCallbacks(&g_sle_connect_cbk);
}

// 结束定义 SLE连接管理 回调函数--------------------------------------------------

// 开始定义 SSAP客户端 回调函数---------------------------------------------------

/**
 * @brief callback function for exchange info. 交换信息的回调函数（MTU改变的回调函数）。收到交换信息的对端响应时调用。
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  param     交换信息。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void sle_client_exchange_info_cbk(uint8_t client_id, uint16_t conn_id, ssap_exchange_info_t *param,
                                         errcode_t status)
{
    // 打印交换信息
    osal_printk("%s [sle_client_exchange_info_cbk] client id:%d, status:%d, mtu size:%d, version:%d\r\n",
                SLE_CLIENT_LOG, client_id, status, param->mtu_size, param->version);

    // 交换信息成功
    if (status == ERRCODE_SUCC)
    {
        // 查找服务（Find Structure）

        // SSAP find operation parameter. 定义SSAP查找参数
        ssapc_find_structure_param_t find_param = {0};

        // 查找类型: SSAP_FIND_TYPE_PRIMARY_SERVICE
        // SSAP_FIND_TYPE_SERVICE_STRUCTURE = 0x00, 服务结构
        // SSAP_FIND_TYPE_PRIMARY_SERVICE   = 0x01, 首要服务
        // SSAP_FIND_TYPE_REFERENCE_SERVICE = 0x02, 引用服务
        // SSAP_FIND_TYPE_PROPERTY          = 0x03, 属性
        // SSAP_FIND_TYPE_METHOD            = 0x04, 方法
        // SSAP_FIND_TYPE_EVENT             = 0x05, 事件
        // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h中的
        // ssap_find_type_t
        find_param.type = SSAP_FIND_TYPE_PRIMARY_SERVICE;

        // 起始句柄
        find_param.start_hdl = 1;

        // 结束句柄
        find_param.end_hdl = 0xFFFF;

        // 查找服务、属性、描述符
        // 服务发现结果将在 ssapc_find_structure_callback 和 ssapc_find_structure_complete_callback 中返回
        // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
        // ssapc_find_structure接口
        osal_printk("%s [sle_client_exchange_info_cbk] start find structure...\r\n", SLE_CLIENT_LOG);
        ssapc_find_structure(client_id, conn_id, &find_param); // 此处使用默认的client ID 0
    }
}

/**
 * @brief callback function for find structure. 服务发现的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  service   发现的服务信息。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void sle_client_find_structure_cbk(uint8_t client_id, uint16_t conn_id,
                                          ssapc_find_service_result_t *service,
                                          errcode_t status)
{
    // 打印服务发现信息
    osal_printk("%s [sle_client_find_structure_cbk] client id:%d, conn id:%d, status:%d, "
                "start hdl:0x%02x, end hdl:0x%02x, uuid len:%d\r\n",
                SLE_CLIENT_LOG, client_id, conn_id, status, service->start_hdl, service->end_hdl, service->uuid.len);
    // 打印服务UUID
    if (service->uuid.len == UUID_16BIT_LEN)        // UUID长度为16bit
    {
        osal_printk("%s [sle_client_find_structure_cbk] structure uuid:0x%02x 0x%02x\r\n",
                    SLE_CLIENT_LOG, service->uuid.uuid[14], service->uuid.uuid[15]); /* 14 15: uuid index */
    }
    else if (service->uuid.len == UUID_128BIT_LEN)  // UUID长度为128bit
    {
        for (uint8_t idx = 0; idx < UUID_128BIT_LEN; idx++)
        {
            osal_printk("%s [sle_client_find_structure_cbk] structure uuid[0x%x]:0x%02x\r\n",
                        SLE_CLIENT_LOG, idx, service->uuid.uuid[idx]);
        }
    }

    // 服务发现成功
    if (status == ERRCODE_SUCC)
    {
        // 保存服务起始句柄
        g_sle_find_service_result.start_hdl = service->start_hdl;

        // 保存服务结束句柄
        g_sle_find_service_result.end_hdl = service->end_hdl;

        // 保存服务UUID
        memcpy_s(&g_sle_find_service_result.uuid, sizeof(sle_uuid_t), &service->uuid, sizeof(sle_uuid_t));

        // 设置SLE服务发现状态，此时就可以发送数据到服务端了
        // SsapWriteReq函数，是需要param.handle的（即服务的起始句柄），
        // 也就是说，客户端发送数据，需要在发现服务之后，而不是连接成功之后
        g_sle_service_found = 1;
    }
}

/**
 * @brief callback function for find property. 属性发现的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  property  属性。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void sle_client_find_property_cbk(uint8_t client_id, uint16_t conn_id,
                                         ssapc_find_property_result_t *property, errcode_t status)
{
    // 打印属性发现信息
    osal_printk("%s [sle_client_find_property_cbk] client id:%d, conn id:%d, operate indication:%d, "
                "descriptors count:%d, status:%d, handle:%d\r\n",
                SLE_CLIENT_LOG, client_id, conn_id, property->operate_indication,
                property->descriptors_count, status, property->handle);
    
    // 打印属性描述符类型
    for (uint16_t idx = 0; idx < property->descriptors_count; idx++)
    {
        osal_printk("%s [sle_client_find_property_cbk] descriptors type [0x%x]: 0x%02x.\r\n",
                    SLE_CLIENT_LOG, idx, property->descriptors_type[idx]);
    }

    // 打印属性UUID
    if (property->uuid.len == UUID_16BIT_LEN)       // UUID长度为16bit
    {
        osal_printk("%s [sle_client_find_property_cbk] uuid: 0x%02x 0x%02x.\r\n",
                    SLE_CLIENT_LOG, property->uuid.uuid[14], property->uuid.uuid[15]); /* 14 15: uuid index */
    }
    else if (property->uuid.len == UUID_128BIT_LEN) // UUID长度为128bit
    {
        for (uint16_t idx = 0; idx < UUID_128BIT_LEN; idx++)
        {
            osal_printk("%s [sle_client_find_property_cbk] uuid [0x%x]: 0x%02x.\r\n",
                        SLE_CLIENT_LOG, idx, property->uuid.uuid[idx]);
        }
    }
}

/**
 * @brief callback function for find structure complete. 服务发现完成的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id        客户端 ID。
 * @param  [in]  conn_id          连接 ID。
 * @param  [in]  structure_result 入参回传。
 * @param  [in]  status           执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void sle_client_find_structure_cmp_cbk(uint8_t client_id, uint16_t conn_id,
                                              ssapc_find_structure_result_t *structure_result,
                                              errcode_t status)
{
    // 打印服务发现信息
    osal_printk("%s [sle_client_find_structure_cmp_cbk] client id:%d, conn id:%d, status:%d, type:%d, uuid len:%d\r\n",
                SLE_CLIENT_LOG, client_id, conn_id, status, structure_result->type, structure_result->uuid.len);
    
    // 打印服务UUID
    if (structure_result->uuid.len == UUID_16BIT_LEN)       // UUID长度为16bit
    {
        osal_printk("%s [sle_client_find_structure_cmp_cbk] structure uuid:0x%02x 0x%02x\r\n",
                    SLE_CLIENT_LOG,
                    structure_result->uuid.uuid[14], structure_result->uuid.uuid[15]); /* 14 15: uuid index */
    }
    else if (structure_result->uuid.len == UUID_128BIT_LEN) // UUID长度为128bit
    {
        for (uint8_t idx = 0; idx < UUID_128BIT_LEN; idx++)
        {
            osal_printk("%s [sle_client_find_structure_cmp_cbk] structure uuid[0x%x]:0x%02x\r\n",
                        SLE_CLIENT_LOG, idx, structure_result->uuid.uuid[idx]);
        }
    }
}

/**
 * @brief callback function for receiving write response. 收到写响应的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id    客户端 ID。
 * @param  [in]  conn_id      连接 ID。
 * @param  [in]  write_result 写结果。
 * @param  [in]  status       执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void sle_client_write_cfm_cbk(uint8_t client_id, uint16_t conn_id,
                                     ssapc_write_result_t *write_result, errcode_t status)
{
    // 打印写响应信息
    osal_printk("%s [sle_client_write_cfm_cbk] client id:%d, conn id:%d, status:%d, "
                "property handle:0x%02x, property type:0x%02x\r\n",
                SLE_CLIENT_LOG, client_id, conn_id, status, write_result->handle, write_result->type);
}

/**
 * @brief callback function for receiving read response. 收到读响应的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  read_data 读取到的数据。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void sle_client_read_cfm_cbk(uint8_t client_id, uint16_t conn_id,
                                    ssapc_handle_value_t *read_data, errcode_t status)
{
    // 打印读响应信息
    osal_printk("%s [sle_client_read_cfm_cbk] client id:%d, conn id:%d, status:%d, "
                "property handle:0x%02x, property type:0x%02x, len:%d\r\n",
                SLE_CLIENT_LOG, client_id, conn_id, status, read_data->handle, read_data->type, read_data->data_len);

    // 打印读取到的数据
    for (uint16_t idx = 0; idx < read_data->data_len; idx++)
    {
        osal_printk("%s [sle_client_read_cfm_cbk] [0x%x] 0x%02x\r\n", SLE_CLIENT_LOG, idx, read_data->data[idx]);
    }
}

/**
 * @brief callback function for notification. 收到服务端通知的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  data      数据。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void ssapc_notification_cbk(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                            errcode_t status)
{
    (void)client_id;
    (void)conn_id;
    (void)status;

    data->data[data->data_len - 1] = '\0';  // 放置字符串结束符
    printf("%s [ssapc_notification_cbk] receive server notification: %s\r\n", SLE_CLIENT_LOG, data->data);
}

/**
 * @brief callback function for indication. 收到服务端指示的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  data      数据。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void ssapc_indication_cbk(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                          errcode_t status)
{
    (void)client_id;
    (void)conn_id;
    (void)status;

    data->data[data->data_len - 1] = '\0';  // 放置字符串结束符
    printf("%s [ssapc_indication_cbk] receive server indication: %s\r\n", SLE_CLIENT_LOG, data->data);
}

/**
 * @brief register callback functions for ssapc. 注册SSAP客户端回调函数
 */
static void sle_client_ssapc_cbk_register(void)
{
    g_sle_ssapc_cbk.exchange_info_cb = sle_client_exchange_info_cbk;           // mtu改变的回调函数
    g_sle_ssapc_cbk.find_structure_cb = sle_client_find_structure_cbk;         // 服务发现的回调函数
    g_sle_ssapc_cbk.find_structure_cmp_cb = sle_client_find_structure_cmp_cbk; // 服务发现完成的回调函数
    g_sle_ssapc_cbk.ssapc_find_property_cbk = sle_client_find_property_cbk;    // 属性发现的回调函数
    g_sle_ssapc_cbk.write_cfm_cb = sle_client_write_cfm_cbk;                   // 收到写响应的回调函数
    g_sle_ssapc_cbk.read_cfm_cb = sle_client_read_cfm_cbk;                     // 收到读响应的回调函数
    g_sle_ssapc_cbk.notification_cb = ssapc_notification_cbk;                  // 收到通知的回调函数
    g_sle_ssapc_cbk.indication_cb = ssapc_indication_cbk;                      // 收到指示的回调函数

    // 注册 SSAP客户端 回调函数
    ssapc_register_callbacks(&g_sle_ssapc_cbk);
}

// 结束定义 SSAP客户端 回调函数---------------------------------------------------

/**
 * @brief 注册SLE客户端
 */
static errcode_t sle_client_register(void)
{
    errcode_t ret;

    // 将SLE客户端应用的UUID放入SleUuid结构体。SleUuid结构体与海思接口的sle_uuid_t结构体一致
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h中的sle_uuid_t
    SleUuid app_uuid = {0};
    app_uuid.len = sizeof(g_sle_uuid_app_uuid);
    if (memcpy_s(app_uuid.uuid, app_uuid.len, g_sle_uuid_app_uuid, sizeof(g_sle_uuid_app_uuid)) != EOK)
    {
        return ERRCODE_SLE_FAIL;
    }

    // 注册SSAP客户端，参数包括：应用UUID、客户端ID
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
    // ssapc_register_client接口
    printf("%s ssapc register client.\r\n", SLE_CLIENT_LOG);
    ret = SsapcRegisterClient(&app_uuid, &g_client_id);

    return ret;
}

/**
 * @brief 设置本地设备地址
 */
static void sle_client_set_local_address(void)
{
    random_mac_addr(g_sle_local_addr);      // 生成随机地址
    printf("local addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
           g_sle_local_addr[0], g_sle_local_addr[1], g_sle_local_addr[2],
           g_sle_local_addr[3], g_sle_local_addr[4], g_sle_local_addr[5]);
    SleAddr local_address;

    // 设置本地设备地址类型：公有地址
    // SLE_ADDRESS_TYPE_PUBLIC：公有地址
    // SLE_ADDRESS_TYPE_RANDOM：随机地址
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_common.h中的
    // sle_addr_type_t
    local_address.type = SLE_ADDRESS_TYPE_PUBLIC;

    // 设置本地设备地址
    (void)memcpy_s(local_address.addr, SLE_ADDR_LEN, g_sle_local_addr, SLE_ADDR_LEN);

    // 设置本地设备地址
    SleSetLocalAddr(&local_address);
}

/**
 * @brief 初始化SLE客户端
 */
void sle_client_init()
{
    // 注册SLE客户端
    sle_client_register();

    // 注册SLE设备发现回调函数
    sle_client_seek_cbk_register();

    // 注册SLE连接管理回调函数
    sle_client_connect_cbk_register();

    // 注册SSAP客户端回调函数
    sle_client_ssapc_cbk_register();

    // 使能SLE协议栈（开启SLE）
    EnableSle();

    // 设置本地设备地址（必须在开启SLE之后）
    sle_client_set_local_address();
}

/**
 * @brief 客户端通过property handle（属性句柄）向服务端发送数据
 * @param data 待发送数据
 * @param len 数据长度
 * @return 执行结果错误码
 */
static errcode_t sle_client_send_report_by_handle(const uint8_t *data, uint8_t len)
{
    // SSAP send parameter. 写请求参数
    ssapc_write_param_t param = {0};  

    // 属性句柄
    param.handle = g_sle_find_service_result.start_hdl;

    // 属性类型：SSAP_PROPERTY_TYPE_VALUE（属性值）
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h中的
    // ssap_property_type_t
    param.type = SSAP_PROPERTY_TYPE_VALUE;

    // 数据长度
    param.data_len = len + 1; // 容纳字符串结束符。非星闪协议规定

    // 数据内容
    param.data = osal_vmalloc(param.data_len);
    if (param.data == NULL)
    {
        printf("%s [sle_client_send_report_by_handle] osal_vmalloc fail\r\n", SLE_CLIENT_LOG);
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(param.data, param.data_len, data, len) != EOK)
    {
        printf("%s [sle_client_send_report_by_handle] memcpy_s fail\r\n", SLE_CLIENT_LOG);
        osal_vfree(param.data);
        return ERRCODE_SLE_FAIL;
    }

    // 发起写请求
    // 写结果将在 ssapc_write_cfm_callback 中返回
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
    // ssapc_write_req接口
    if (SsapWriteReq(g_client_id, g_sle_conn_id, &param) != ERRCODE_SUCC)
    {
        printf("%s [sle_client_send_report_by_handle] SsapWriteReq fail\r\n", SLE_CLIENT_LOG);
        osal_vfree(param.data);
        return ERRCODE_SLE_FAIL;
    }

    osal_vfree(param.data);
    return ERRCODE_SLE_SUCCESS;
}

/**
 * @brief 客户端向服务端发送数据
 * @param data 待发送数据
 * @param length 数据长度
 * @return 执行结果错误码
 */
int sle_client_send_data(uint8_t *data, uint8_t length)
{
    // 二次封装是为了以后扩展便利。目前是调用sle_client_send_report_by_handle接口
    int ret;
    ret = sle_client_send_report_by_handle(data, length); // 客户端通过property handle（属性句柄）向服务端发送数据
    return ret;
}

/**
 * @brief wait SLE service found. 等待SLE服务发现。
 * @par 客户端发送数据，需要在发现服务之后，而不是连接成功之后。
 */
void sle_wait_service_found(void)
{
    while (g_sle_service_found == 0)
    {
        osal_msleep(100); // 轮询间隔100ms
    }
}

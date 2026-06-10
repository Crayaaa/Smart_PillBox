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
#include "securec.h"
#include "errcode.h"
#include "osal_addr.h"
#include "sle_common.h"
#include "sle_device_discovery.h"
#include "sle_errcode.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "ohos_sle_common.h"
#include "ohos_sle_errcode.h"
#include "ohos_sle_ssap_server.h"
#include "ohos_sle_ssap_client.h"
#include "ohos_sle_device_discovery.h"
#include "ohos_sle_connection_manager.h"
#include "sle_server.h"
#include "sle_server_adv.h"

// 设备名称最大长度
#define NAME_MAX_LENGTH 16

// 连接间隔最小取值，取值范围[0x001E,0x3E80]
// 连接调度间隔12.5ms，单位125us
#define SLE_CONN_INTV_MIN_DEFAULT 0x64

// 连接间隔最大取值，取值范围[0x001E,0x3E80]
// 连接调度间隔12.5ms，单位125us
#define SLE_CONN_INTV_MAX_DEFAULT 0x64

// 最小设备公开周期, 0x000020~0xffffff, 单位125us
// 连接调度间隔25ms，单位125us
#define SLE_ADV_INTERVAL_MIN_DEFAULT 0xC8

// 最大设备公开周期, 0x000020~0xffffff, 单位125us
// 连接调度间隔25ms，单位125us
#define SLE_ADV_INTERVAL_MAX_DEFAULT 0xC8

// 最大超时时间，取值范围[0x000A,0x0C80]
// 超时时间5000ms，单位10ms
#define SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT 0x1F4

// 最大休眠连接间隔，取值范围[0x0000,0x01F3]
// 超时时间4990ms，单位10ms
#define SLE_CONN_MAX_LATENCY 0x1F3

// 广播发送功率
#define SLE_ADV_TX_POWER 10

// 广播ID（设备公开ID/设备公开句柄），取值范围[0, 0xFF]
#define SLE_ADV_HANDLE_DEFAULT 1

// 最大广播数据长度
#define SLE_ADV_DATA_LEN_MAX 251

// 广播设备名称
static uint8_t g_sle_local_name[NAME_MAX_LENGTH] = "";

// 服务端初始化延时时间
// #define SLE_SERVER_INIT_DELAY_MS 1000

// 日志标签
#define SLE_SERVER_LOG "[sle server]"

/**
 * @brief Set SLE announce name. 设置广播设备名称
 * @param name 广播设备名称
 */
void sle_set_announce_name(char *name)
{
    memset(g_sle_local_name, 0, sizeof(g_sle_local_name));
    errno_t ret = memcpy_s(g_sle_local_name, sizeof(g_sle_local_name), name, strlen(name));
    if (ret != EOK)
    {
        printf("[sle_set_announce_name] memcpy fail\r\n");
    }
}

/// @brief 设置广播设备名称
static uint16_t sle_set_adv_local_name(uint8_t *adv_data, uint16_t max_len)
{
    errno_t ret;
    uint8_t index = 0;

    uint8_t *local_name = g_sle_local_name;
    uint8_t local_name_len = sizeof(g_sle_local_name) - 1;
    printf("%s local_name_len = %d\r\n", SLE_SERVER_LOG, local_name_len);
    printf("%s local_name: ", SLE_SERVER_LOG);
    for (uint8_t i = 0; i < local_name_len; i++)
    {
        printf("0x%02x ", local_name[i]);
    }
    printf("\r\n");
    adv_data[index++] = local_name_len + 1;
    adv_data[index++] = SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME; // 设备完整本地名称
    ret = memcpy_s(&adv_data[index], max_len - index, local_name, local_name_len);
    if (ret != EOK)
    {
        printf("%s [sle_set_adv_local_name] memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    return (uint16_t)index + local_name_len;
}

/// @brief 设置广播数据
static uint16_t sle_set_adv_data(uint8_t *adv_data)
{
    size_t len = 0;
    uint16_t idx = 0;
    errno_t ret = 0;

    // 设置发现等级
    len = sizeof(struct sle_adv_common_value);
    struct sle_adv_common_value adv_disc_level = {
        .length = len - 1,
        .type = SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL, // 类型：发现等级
        .value = SLE_ANNOUNCE_LEVEL_NORMAL,        // 值：一般可发现
    };
    ret = memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_disc_level, len);
    if (ret != EOK)
    {
        printf("%s adv_disc_level memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    idx += len;

    // 设置接入层能力
    len = sizeof(struct sle_adv_common_value);
    struct sle_adv_common_value adv_access_mode = {
        .length = len - 1,
        .type = SLE_ADV_DATA_TYPE_ACCESS_MODE, // 类型：接入层能力
        .value = 0,                            // 值：0（mean what?）
    };
    ret = memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_access_mode, len);
    if (ret != EOK)
    {
        printf("%s adv_access_mode memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    idx += len;

    return idx;
}

/// @brief 设置扫描响应数据
static uint16_t sle_set_scan_response_data(uint8_t *scan_rsp_data)
{
    uint16_t idx = 0;
    errno_t ret;
    size_t scan_rsp_data_len = sizeof(struct sle_adv_common_value);

    // 设置广播发送功率
    struct sle_adv_common_value tx_power_level = {
        .length = scan_rsp_data_len - 1,
        .type = SLE_ADV_DATA_TYPE_TX_POWER_LEVEL, // 类型：广播发送功率
        .value = SLE_ADV_TX_POWER,                // 值：10
    };
    ret = memcpy_s(scan_rsp_data, SLE_ADV_DATA_LEN_MAX, &tx_power_level, scan_rsp_data_len);
    if (ret != EOK)
    {
        printf("%s sle scan response data memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    idx += scan_rsp_data_len;

    // 设置广播设备名称
    idx += sle_set_adv_local_name(&scan_rsp_data[idx], SLE_ADV_DATA_LEN_MAX - idx);
    return idx;
}

/// @brief 设置默认广播参数（设备公开参数）
static int sle_set_default_announce_param(void)
{
    errno_t ret;
    SleAnnounceParam param = {0}; // 设备公开参数
    uint8_t index;

    // sle地址
    // 每个设备的地址不同，需要根据实际情况修改，或者使用随机地址
    unsigned char local_addr[SLE_ADDR_LEN] = {0x78, 0x70, 0x60, 0x88, 0x96, 0x45};
    random_mac_addr(local_addr); // 随机地址
    printf("AnnounceParam.ownAddr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
           local_addr[0], local_addr[1], local_addr[2], local_addr[3], local_addr[4], local_addr[5]);

    // 设备公开类型：可连接可扫描
    param.announceMode = SLE_ANNOUNCE_MODE_CONNECTABLE_SCANABLE;

    // 广播ID（设备公开ID/设备公开句柄）
    param.announceHandle = SLE_ADV_HANDLE_DEFAULT;

    // G/T角色协商指示：期望做T可协商
    param.announceGtRole = SLE_ANNOUNCE_ROLE_T_CAN_NEGO;

    // 发现等级：一般可发现
    param.announceLevel = SLE_ANNOUNCE_LEVEL_NORMAL;

    // 设备公开信道：默认
    param.announceChannelMap = SLE_ADV_CHANNEL_MAP_DEFAULT;

    // 最小设备公开周期, 0x000020~0xffffff, 单位125us
    param.announceIntervalMin = SLE_ADV_INTERVAL_MIN_DEFAULT;

    // 最大设备公开周期, 0x000020~0xffffff, 单位125us
    param.announceIntervalMax = SLE_ADV_INTERVAL_MAX_DEFAULT;

    // 连接间隔最小取值，取值范围[0x001E,0x3E80]
    param.connIntervalMin = SLE_CONN_INTV_MIN_DEFAULT;

    // 连接间隔最大取值，取值范围[0x001E,0x3E80]
    param.connIntervalMax = SLE_CONN_INTV_MAX_DEFAULT;

    // 最大休眠连接间隔，取值范围[0x0000,0x01F3]
    param.connMaxLatency = SLE_CONN_MAX_LATENCY;

    // 最大超时时间，取值范围[0x000A,0x0C80]
    param.connSupervisionTimeout = SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT;

    // 设备地址类型：公有地址
    // SLE_ADDRESS_TYPE_PUBLIC：公有地址
    // SLE_ADDRESS_TYPE_RANDOM：随机地址
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_common.h中的
    // sle_addr_type_t
    param.ownAddr.type = SLE_ADDRESS_TYPE_PUBLIC;

    // 设置本端地址
    ret = memcpy_s(param.ownAddr.addr, SLE_ADDR_LEN, local_addr, SLE_ADDR_LEN);
    if (ret != EOK)
    {
        printf("%s [sle_set_default_announce_param] data memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }

    // 设置设备公开参数
    // 参数包括：设备公开ID、设备公开参数
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_set_announce_param接口
    return SleSetAnnounceParam(param.announceHandle, &param);
}

/// @brief 设置默认广播数据（设备公开数据）
static int sle_set_default_announce_data(void)
{
    errcode_t ret;
    uint8_t announce_data_len = 0;
    uint8_t seek_data_len = 0;
    SleAnnounceData data = {0};                  // 设备公开数据
    uint8_t adv_handle = SLE_ADV_HANDLE_DEFAULT; // 广播ID（设备公开ID/设备公开句柄）
    uint8_t announce_data[SLE_ADV_DATA_LEN_MAX] = {0};
    uint8_t seek_rsp_data[SLE_ADV_DATA_LEN_MAX] = {0};
    uint8_t data_index = 0;

    // 设置广播数据
    announce_data_len = sle_set_adv_data(announce_data);
    data.announceData = announce_data;
    data.announceDataLen = announce_data_len;
    printf("%s data.announce_data_len = %d\r\n", SLE_SERVER_LOG, data.announceDataLen);
    printf("%s data.announce_data: ", SLE_SERVER_LOG);
    for (data_index = 0; data_index < data.announceDataLen; data_index++)
    {
        printf("0x%02x ", data.announceData[data_index]);
    }
    printf("\r\n");

    // 设置扫描响应数据
    seek_data_len = sle_set_scan_response_data(seek_rsp_data);
    data.seekRspData = seek_rsp_data;
    data.seekRspDataLen = seek_data_len;
    printf("%s data.seek_rsp_data_len = %d\r\n", SLE_SERVER_LOG, data.seekRspDataLen);
    printf("%s data.seek_rsp_data: ", SLE_SERVER_LOG);
    for (data_index = 0; data_index < data.seekRspDataLen; data_index++)
    {
        printf("0x%02x ", data.seekRspData[data_index]);
    }
    printf("\r\n");

    // 设置设备公开数据
    // 参数包括：设备公开ID、设备公开数据
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_set_announce_data接口
    ret = SleSetAnnounceData(adv_handle, &data);
    if (ret == ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_set_default_announce_data] set announce data success.\r\n", SLE_SERVER_LOG);
    }
    else
    {
        printf("%s [sle_set_default_announce_data] set adv param fail.\r\n", SLE_SERVER_LOG);
    }

    return ERRCODE_SLE_SUCCESS;
}

/**
 * @brief callback function for enable announce. 广播使能的回调函数（设备公开使能的回调函数）
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  announce_id 公开ID。
 * @param  [in]  status       执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_device_discovery.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_announce_enable_cbk(uint32_t announce_id, errcode_t status)
{
    printf("%s [sle_announce_enable_cbk] id:%02x, state:%x\r\n", SLE_SERVER_LOG, announce_id,
                        status);
}

/**
 * @brief callback function for disable announce. 广播禁用的回调函数（设备公开关闭的回调函数）
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  announce_id 公开ID。
 * @param  [in]  status       执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_device_discovery.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_announce_disable_cbk(uint32_t announce_id, errcode_t status)
{
    printf("%s [sle_announce_disable_cbk] id:%02x, state:%x\r\n", SLE_SERVER_LOG, announce_id,
                        status);
}

/**
 * @brief callback function for announce terminal. 广播终止的回调函数（设备公开停止的回调函数）
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  announce_id 公开ID。
 * @see foundation\communication\sle\ohos_sle_device_discovery.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_announce_terminal_cbk(uint32_t announce_id)
{
    printf("%s [sle_announce_terminal_cbk] id:%02x\r\n", SLE_SERVER_LOG, announce_id);
}

/**
 * @brief callback function for enable server. 服务端使能的回调函数（SLE协议栈使能的回调函数）
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  status 执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_device_discovery.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_enable_cbk(errcode_t status)
{
    printf("%s [sle_enable_cbk] status:%x\r\n", SLE_SERVER_LOG, status);
    // osal_msleep(SLE_SERVER_INIT_DELAY_MS);

    // 在sle_server.c中定义
    // 添加sle服务端，初始化sle服务端的广播参数和数据，然后启动广播
    sle_enable_server_cbk();
}

/**
 * @brief SLE协议栈关闭的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  status 执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_device_discovery.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_disable_cbk(errcode_t status)
{
    printf("%s [sle_disable_cbk]\r\n", SLE_SERVER_LOG);
}

/// @brief register callback functions for Announce and Seek. 注册SLE设备公开和设备发现回调函数
errcode_t sle_announce_register_cbks(void)
{
    errcode_t ret;
    SleAnnounceSeekCallbacks seek_cbks = {0};
    seek_cbks.sleAnnounceEnableCb = sle_announce_enable_cbk;     // 设备公开使能的回调函数
    seek_cbks.sleAnnounceDisableCb = sle_announce_disable_cbk;   // 设备公开关闭的回调函数
    seek_cbks.sleAnnounceTerminalCb = sle_announce_terminal_cbk; // 设备公开停止的回调函数
    seek_cbks.sleEnableCb = sle_enable_cbk;                      // SLE协议栈使能的回调函数
    seek_cbks.sleDisableCb = sle_disable_cbk;                    // SLE协议栈关闭的回调函数
    ret = SleAnnounceSeekRegisterCallbacks(&seek_cbks);          // 注册SLE设备公开和设备发现回调函数
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_announce_register_cbks] register_callbacks fail :%x\r\n",
                            SLE_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

/// @brief 初始化sle服务端的广播参数和数据，然后启动广播
errcode_t sle_server_adv_init(void)
{
    errcode_t ret;

    // 设置默认广播参数（设备公开参数）
    sle_set_default_announce_param();

    // 设置默认广播数据（设备公开数据）
    sle_set_default_announce_data();

    // 开始广播（开始设备公开）
    // 参数：广播ID（设备公开ID/设备公开句柄）
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_start_announce接口
    ret = SleStartAnnounce(SLE_ADV_HANDLE_DEFAULT);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_adv_init] sle_start_announce fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    return ERRCODE_SLE_SUCCESS;
}

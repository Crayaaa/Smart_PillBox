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
#include "osal_debug.h"
#include "osal_task.h"
#include "osal_addr.h"
#include "securec.h"
#include "errcode.h"

// 海思SDK
#include "sle_common.h"                     // SLE Common API. SLE公共API
#include "sle_errcode.h"                    // SLE Error Code API. SLE错误码API
#include "sle_device_discovery.h"           // SLE Device Announce & Seek Manager API. SLE设备公开与扫描管理API

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
#define NAME_MAX_LENGTH 16                         // 设备名称最大长度
#define SLE_CONN_INTV_MIN_DEFAULT 0x64             // 连接间隔最小取值，取值范围[0x001E, 0x3E80]，单位125us。12.5ms
#define SLE_CONN_INTV_MAX_DEFAULT 0x64             // 连接间隔最大取值，取值范围[0x001E, 0x3E80]，单位125us。12.5ms
#define SLE_ADV_INTERVAL_MIN_DEFAULT 0xC8          // 最小设备公开周期，取值范围[0x000020, 0xffffff]，单位125us。25ms
#define SLE_ADV_INTERVAL_MAX_DEFAULT 0xC8          // 最大设备公开周期，取值范围[0x000020, 0xffffff]，单位125us。25ms
#define SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT 0x1F4 // 最大超时时间，取值范围[0x000A, 0x0C80]，单位10ms。5000ms
#define SLE_CONN_MAX_LATENCY 0x1F3                 // 最大休眠连接间隔，取值范围[0x0000, 0x01F3]，单位10ms。4990ms
#define SLE_ADV_TX_POWER 10                        // 广播发送功率
#define SLE_ADV_HANDLE_DEFAULT 1                   // 广播ID（设备公开ID/设备公开句柄），取值范围[0, 0xFF]
#define SLE_ADV_DATA_LEN_MAX 251                   // 最大广播数据长度
#define SLE_SERVER_LOG "[sle server]"              // 日志标签

// 广播设备名称，也是本地设备名称
static uint8_t g_sle_local_name[NAME_MAX_LENGTH] = "";

// 本地设备地址。每个设备的地址不同，需要根据实际情况修改，或者使用随机地址
static uint8_t g_sle_local_addr[SLE_ADDR_LEN] = {0};

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

// 开始定义 SLE设备公开与扫描 回调函数---------------------------------------------------------

/**
 * @brief callback function for enable announce. 广播使能的回调函数（设备公开使能的回调函数）
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  announce_id  公开ID。
 * @param  [in]  status       执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_device_discovery.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_announce_enable_cbk(uint32_t announce_id, errcode_t status)
{
    // 打印参数值
    printf("%s [sle_announce_enable_cbk] id:%02x, state:%x\r\n", SLE_SERVER_LOG, announce_id, status);
}

/**
 * @brief callback function for disable announce. 广播禁用的回调函数（设备公开关闭的回调函数）
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  announce_id  公开ID。
 * @param  [in]  status       执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_device_discovery.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h
 */
static void sle_announce_disable_cbk(uint32_t announce_id, errcode_t status)
{
    // 打印参数值
    printf("%s [sle_announce_disable_cbk] id:%02x, state:%x\r\n", SLE_SERVER_LOG, announce_id, status);
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
    // 打印参数值
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
    // 打印参数值
    printf("%s [sle_enable_cbk] status:%x\r\n", SLE_SERVER_LOG, status);
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
    // 打印参数值
    printf("%s [sle_disable_cbk] status:%x\r\n", SLE_SERVER_LOG, status);
}

/**
 * @brief register callback functions for announce. 注册SLE设备公开回调函数
 * @return 错误码
 */
errcode_t sle_announce_register_cbks(void)
{
    SleAnnounceSeekCallbacks seek_cbks = {0};
    seek_cbks.sleEnableCb = sle_enable_cbk;                      // SLE协议栈使能的回调函数
    seek_cbks.sleDisableCb = sle_disable_cbk;                    // SLE协议栈关闭的回调函数
    seek_cbks.sleAnnounceEnableCb = sle_announce_enable_cbk;     // 设备公开使能的回调函数
    seek_cbks.sleAnnounceDisableCb = sle_announce_disable_cbk;   // 设备公开关闭的回调函数
    seek_cbks.sleAnnounceTerminalCb = sle_announce_terminal_cbk; // 设备公开停止的回调函数

    // 注册 SLE设备公开与扫描 回调函数
    return SleAnnounceSeekRegisterCallbacks(&seek_cbks);
}

// 结束定义 SLE设备公开与扫描 回调函数---------------------------------------------------------

// 设备公开参数（广播参数） 开始---------------------------------------------------------------

/**
 * @brief 设置设备公开参数（广播参数）
 */
static int sle_server_set_announce_param(void)
{
    // 设备公开参数
    SleAnnounceParam param = {0};

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

    // 最小设备公开周期，取值范围[0x000020, 0xffffff]，单位125us
    param.announceIntervalMin = SLE_ADV_INTERVAL_MIN_DEFAULT;

    // 最大设备公开周期，取值范围[0x000020, 0xffffff]，单位125us
    param.announceIntervalMax = SLE_ADV_INTERVAL_MAX_DEFAULT;

    // 连接间隔最小取值，取值范围[0x001E, 0x3E80]，单位125us
    param.connIntervalMin = SLE_CONN_INTV_MIN_DEFAULT;

    // 连接间隔最大取值，取值范围[0x001E, 0x3E80]，单位125us
    param.connIntervalMax = SLE_CONN_INTV_MAX_DEFAULT;

    // 最大休眠连接间隔，取值范围[0x0000, 0x01F3]，单位10ms
    param.connMaxLatency = SLE_CONN_MAX_LATENCY;

    // 最大超时时间，取值范围[0x000A, 0x0C80]，单位10ms
    param.connSupervisionTimeout = SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT;

    // 设备地址类型：公有地址
    // SLE_ADDRESS_TYPE_PUBLIC：公有地址
    // SLE_ADDRESS_TYPE_RANDOM：随机地址
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_common.h中的
    // sle_addr_type_t
    param.ownAddr.type = SLE_ADDRESS_TYPE_PUBLIC;

    // 定义本地设备地址
    // 这个地址在客户端的seek_result_info_cbk（扫描结果上报）回调函数中是可以直接拿到的
    random_mac_addr(g_sle_local_addr);     // 生成随机地址
    printf("%s [sle_server_set_announce_param] local addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n", SLE_SERVER_LOG, 
           g_sle_local_addr[0], g_sle_local_addr[1], g_sle_local_addr[2],
           g_sle_local_addr[3], g_sle_local_addr[4], g_sle_local_addr[5]);

    // 设备地址：本地设备地址
    errno_t ret = memcpy_s(param.ownAddr.addr, SLE_ADDR_LEN, g_sle_local_addr, SLE_ADDR_LEN);
    if (ret != EOK)
    {
        printf("%s [sle_server_set_announce_param] data memcpy fail\r\n", SLE_SERVER_LOG);
        return -1;
    }

    // 设置设备公开参数
    // 参数包括：设备公开ID、设备公开参数
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_set_announce_param接口
    return SleSetAnnounceParam(param.announceHandle, &param);
}

// 设备公开参数（广播参数） 结束---------------------------------------------------------------

// 设备公开数据（广播数据和扫描响应数据） 开始---------------------------------------------------

/**
 * @brief 设置广播数据
 * @see SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022）
 * @param adv_data 广播数据
 * @return 广播数据长度
 */
static uint16_t sle_server_set_announce_data(uint8_t *adv_data)
{
    size_t len = 0;     // 数据子结构长度
    uint16_t idx = 0;   // 数据索引
    errno_t ret = 0;    // 错误码

    // 设置发现等级
    // SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022），6.5.2.2节。
    len = sizeof(struct sle_adv_common_value);     // 数据子结构长度
    struct sle_adv_common_value adv_disc_level = { // 数据子结构：发现等级
        .type = SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL, // 数据类型指示：发现等级。指示被发现方的发现等级
        .length = len - 1,                         // 数据长度指示：2 字节。包括类型（type）和值（value）。与海思保持一致
        .value = SLE_ANNOUNCE_LEVEL_NORMAL,        // 数据内容部分：一般可发现
    };
    ret = memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_disc_level, len);
    if (ret != EOK)
    {
        printf("%s adv_disc_level memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    idx += len; // 数据索引增加

    // 设置星闪接入层能力
    // SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022），6.5.2.3节。
    len = sizeof(struct sle_adv_common_value);      // 数据子结构长度
    struct sle_adv_common_value adv_access_mode = { // 数据子结构：星闪接入层能力
        .type = SLE_ADV_DATA_TYPE_ACCESS_MODE,      // 数据类型指示：星闪接入层能力。指示设备支持的星闪接入层能力
        .length = len - 1,                          // 数据长度指示：2 字节。包括类型（type）和值（value）。与海思保持一致
        .value = 2,                                 // 数据内容部分：2（支持SLE，不支持SLB）
    };
    ret = memcpy_s(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx, &adv_access_mode, len);
    if (ret != EOK)
    {
        printf("%s adv_access_mode memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    idx += len; // 数据索引增加

    // 返回广播数据长度
    return idx;
}

/**
 * @brief 设置广播设备名称
 * @see SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022）
 * @param adv_data 数据指针
 * @param max_len 最大长度
 * @return 数据长度
 */
static uint16_t sle_set_announce_local_name(uint8_t *adv_data, uint16_t max_len)
{
    errno_t ret;                                                    // 错误码
    uint8_t index = 0;                                              // 数据索引
    uint8_t *local_name = g_sle_local_name;                         // 广播设备名称
    uint8_t local_name_len = (uint8_t)strlen((char *)local_name);   // 广播设备名称长度

    // 打印广播设备名称
    // printf("%s local_name_len = %d\r\n", SLE_SERVER_LOG, local_name_len);
    // printf("%s local_name: ", SLE_SERVER_LOG);
    // for (uint8_t i = 0; i < local_name_len; i++)
    // {
    //     printf("0x%02x ", local_name[i]);
    // }
    // printf("\r\n");

    // 数据子结构：设备完整本地名称
    // SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022），6.5.2.7节。
    // 这里采用的顺序是：数据长度指示、数据类型指示、数据内容部分。与海思保持一致
    adv_data[index++] = local_name_len + 1;                     // 数据长度指示：包括类型（type）和值（value）。与海思保持一致
    adv_data[index++] = SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME;  // 数据类型指示：设备完整本地名称
    ret = memcpy_s(&adv_data[index], max_len - index, local_name, local_name_len);  // 数据内容部分：广播设备名称
    if (ret != EOK)
    {
        printf("%s [sle_set_announce_local_name] memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }

    // 返回数据子结构长度
    return (uint16_t)index + local_name_len;
}

/**
 * @brief 设置扫描响应数据
 * @see SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022）
 * @param scan_rsp_data 扫描响应数据
 * @return 扫描响应数据长度
 */
static uint16_t sle_server_set_scan_response_data(uint8_t *scan_rsp_data)
{
    size_t len = 0;   // 数据子结构长度
    uint16_t idx = 0; // 数据索引
    errno_t ret;      // 错误码

    // 设置广播发送功率
    // SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022），6.5.2.8节。
    len = sizeof(struct sle_adv_common_value);      // 数据子结构长度
    struct sle_adv_common_value tx_power_level = {  // 数据子结构：广播发送功率
        .type = SLE_ADV_DATA_TYPE_TX_POWER_LEVEL,   // 数据类型指示：广播发送功率
        .length = len - 1,                          // 数据长度指示：2 字节。包括类型（type）和值（value）。与海思保持一致
        .value = SLE_ADV_TX_POWER,                  // 数据内容部分：10。单位dBm，发送功率值范围-127dBm~127dBm
    };
    ret = memcpy_s(scan_rsp_data, SLE_ADV_DATA_LEN_MAX, &tx_power_level, len);
    if (ret != EOK)
    {
        printf("%s tx_power_level memcpy fail\r\n", SLE_SERVER_LOG);
        return 0;
    }
    idx += len; // 数据索引增加

    // 设置广播设备名称
    // SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022），6.5.2.7节。
    idx += sle_set_announce_local_name(&scan_rsp_data[idx], SLE_ADV_DATA_LEN_MAX - idx); // 数据索引增加

    // 返回扫描响应数据长度
    return idx;
}

/**
 * @brief 设置设备公开数据（广播数据和扫描响应数据）
 * @see SparkLink团体标准：基础服务层设备发现与服务管理（T/XS 20001-2022）
 * @return 错误码
 */
static int sle_server_set_announce_and_scan_response_data(void)
{
    SleAnnounceData data = {0};                        // 设备公开数据
    uint8_t announce_data_len = 0;                     // 广播数据长度
    uint8_t seek_data_len = 0;                         // 扫描响应数据长度
    uint8_t announce_data[SLE_ADV_DATA_LEN_MAX] = {0}; // 广播数据
    uint8_t seek_rsp_data[SLE_ADV_DATA_LEN_MAX] = {0}; // 扫描响应数据
    uint8_t adv_handle = SLE_ADV_HANDLE_DEFAULT;       // 广播ID（设备公开ID/设备公开句柄）
    errcode_t ret;                                     // 错误码

    // 设置广播数据
    announce_data_len = sle_server_set_announce_data(announce_data);  // 设置广播数据，并获取广播数据长度
    data.announceData = announce_data;                                // 设置广播数据
    data.announceDataLen = announce_data_len;                         // 设置广播数据长度

    // 设置扫描响应数据
    seek_data_len = sle_server_set_scan_response_data(seek_rsp_data); // 设置扫描响应数据，并获取扫描响应数据长度
    data.seekRspData = seek_rsp_data;                                 // 设置扫描响应数据
    data.seekRspDataLen = seek_data_len;                              // 设置扫描响应数据长度

    // 设置设备公开数据
    // 参数包括：设备公开ID、设备公开数据
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_set_announce_data接口
    ret = SleSetAnnounceData(adv_handle, &data);
    if (ret == ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_set_announce_and_scan_response_data] success.\r\n", SLE_SERVER_LOG);
    }
    else
    {
        printf("%s [sle_server_set_announce_and_scan_response_data] fail.\r\n", SLE_SERVER_LOG);
    }

    return ret;
}

// 设备公开数据（广播数据和扫描响应数据） 结束---------------------------------------------------

/**
 * @brief 设置本地设备地址
 */
static void sle_server_set_local_address(void)
{
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
 * @brief 设置本地设备名称
 */
static void sle_server_set_local_name(void)
{
    errcode_t ret; // 错误码

    // 设置本地设备名称
    ret = sle_set_local_name(g_sle_local_name, strlen((char *)g_sle_local_name));
    if (ret != ERRCODE_SUCC)
    {
        printf("[sle_server_set_local_name] set local name fail: %x\r\n", ret);
    }
}

/**
 * @brief 初始化SLE服务端的广播参数、广播数据和扫描响应数据，然后启动广播
 * @return 错误码
 */
errcode_t sle_server_adv_init(void)
{
    errcode_t ret;

    // 设置设备公开参数（广播参数）
    sle_server_set_announce_param();

    // 设置设备公开数据（广播数据和扫描响应数据）
    sle_server_set_announce_and_scan_response_data();

    // 设置本地设备地址
    sle_server_set_local_address();

    // 设置本地设备名称
    sle_server_set_local_name();

    // 开始广播（开始设备公开）
    // 参数：广播ID（设备公开ID/设备公开句柄）
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_start_announce接口
    ret = SleStartAnnounce(SLE_ADV_HANDLE_DEFAULT);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_adv_init] sle start announce fail: %x\r\n", SLE_SERVER_LOG, ret);
    }

    return ret;
}

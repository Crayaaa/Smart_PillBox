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
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "securec.h"
#include "sle_device_discovery.h"
#include "sle_connection_manager.h"
#include "sle_ssap_client.h"
#include "sle_errcode.h"
#include "ohos_sle_common.h"
#include "ohos_sle_errcode.h"
#include "ohos_sle_ssap_server.h"
#include "ohos_sle_ssap_client.h"
#include "ohos_sle_device_discovery.h"
#include "ohos_sle_connection_manager.h"
#include "pinctrl.h"
#include "errcode.h"
#include "sle_client.h"

#define SLE_MTU_SIZE_DEFAULT 512      // ssap信息交换结构体：mtu大小
#define SLE_SEEK_INTERVAL_DEFAULT 100 // 扫描间隔，取值范围[0x0004, 0xFFFF]
#define SLE_SEEK_WINDOW_DEFAULT 100   // 扫描窗口，取值范围[0x0004, 0xFFFF]
#define UUID_16BIT_LEN 2              // uuid长度16bit
#define UUID_128BIT_LEN 16            // uuid长度128bit
// #define UUID_LEN_2 2                    // uuid长度2字节。暂未用到
// #define SLE_WAIT_SLE_CORE_READY_MS 5000 // 等待sle内核就绪时间。暂未用到
#define SLE_TASK_DELAY_MS 1000        // 任务延时
#define SLE_CLIENT_LOG "[sle client]" // 日志标签

// App uuid
// 应用的uuid，16字节
static char g_sle_uuid_app_uuid[] = {0x39, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
                                     0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// SLE连接ID和服务发现结果数组
static sle_conn_and_service_t g_conn_and_service_arr[128] = {0};

// SLE device announce callback function
// SLE设备公开 回调函数接口
static sle_announce_seek_callbacks_t g_sle_seek_cbk = {0};

// SLE connection manager callback function
// SLE连接管理 回调函数接口
static SleConnectionCallbacks g_sle_connect_cbk = {0};

// ssap client callback function
// ssap客户端 回调函数接口
static ssapc_callbacks_t g_sle_ssapc_cbk = {0};

// SLE remote address
// SLE远程设备（对端设备）地址
static SleAddr g_sle_remote_addr = {0};

// SSAP send parameter
// SSAP句柄和值
ssapc_write_param_t g_sle_send_param = {0};

// 设置一个连接ID统计的变量
uint8_t g_num_conn = 0;

// SLE client id
// SLE客户端ID
uint8_t g_client_id = 0;

// SLE server name
// SLE服务端名称（广播设备名称）
char g_sle_server_name[128] = "";

/**
 * @brief Set SLE server name. 设置SLE服务端名称
 * @param name SLE server name.
 */
void sle_set_server_name(char *name)
{
    memcpy_s(g_sle_server_name, strlen(name), name, strlen(name));
}

/**
 * @brief Get SLE connection id. 获取SLE连接ID和服务发现结果数组
 * @return SLE connection id.
 */
sle_conn_and_service_t *sle_get_conn_and_service(void)
{
    return g_conn_and_service_arr;
}

/**
 * @brief Get SSAP send parameter. 获取SSAP句柄和值
 * @return SSAP send parameter.
 */
ssapc_write_param_t *sle_get_send_param(void)
{
    return &g_sle_send_param;
}

/**
 * @brief set device seek parameter, then start device seek.
 * 设置 设备公开 扫描参数，并开始 设备公开 扫描
 */
void sle_start_scan(void)
{
    // Seek scan parameter. 设备发现扫描参数
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_param_t
    SleSeekParam param = {0};

    // 设置本端地址类型：公有地址
    // SLE_ADDRESS_TYPE_PUBLIC：公有地址
    // SLE_ADDRESS_TYPE_RANDOM：随机地址
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_common.h中的
    // sle_addr_type_t
    param.ownaddrtype = SLE_ADDRESS_TYPE_PUBLIC;

    // 重复过滤开关，0：关闭，1：开启
    param.filterduplicates = 0;

    // 扫描设备使用的过滤类型
    // SLE_SEEK_FILTER_ALLOW_ALL：允许来自任何人的设备发现数据包
    // SLE_SEEK_FILTER_ALLOW_WLST：只允许来自白名单设备的设备发现数据包，预留
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_filter_t
    param.seekfilterpolicy = SLE_SEEK_FILTER_ALLOW_ALL;

    // 扫描设备所使用的PHY
    // SLE_SEEK_PHY_1M: 1M PHY
    // SLE_SEEK_PHY_2M: 2M PHY
    // SLE_SEEK_PHY_4M: 4M PHY
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_phy_t
    param.seekphys = SLE_SEEK_PHY_1M;

    // 扫描类型
    // SLE_SEEK_PASSIVE: 被动扫描
    // SLE_SEEK_ACTIVE: 主动扫描
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
    // sle_seek_type_t
    param.seekType[0] = SLE_SEEK_ACTIVE;

    // 扫描间隔，取值范围[0x0004, 0xFFFF]
    param.seekInterval[0] = SLE_SEEK_INTERVAL_DEFAULT;

    // 扫描窗口，取值范围[0x0004, 0xFFFF]
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
        osal_msleep(SLE_TASK_DELAY_MS);

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
    if (seek_result_data == NULL)
    {
        osal_printk("%s [sle_client_seek_result_info_cbk] seek_result_data is NULL\r\n", SLE_CLIENT_LOG);
        return;
    }

    // 目标设备
    if (strstr((const char *)seek_result_data->data, g_sle_server_name) != NULL)
    {
        // 打印扫描结果中的SLE设备地址
        osal_printk("%s [sle_client_seek_result_info_cbk] target found, addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    SLE_CLIENT_LOG,
                    seek_result_data->addr.addr[0], seek_result_data->addr.addr[1],
                    seek_result_data->addr.addr[2], seek_result_data->addr.addr[3],
                    seek_result_data->addr.addr[4], seek_result_data->addr.addr[5]);

        // 设置SLE远程设备地址
        memcpy_s(&g_sle_remote_addr, sizeof(sle_addr_t), &seek_result_data->addr, sizeof(sle_addr_t));

        // 向对端设备发起连接请求
        SleConnectRemoteDevice(&g_sle_remote_addr);
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
        osal_printk("%s [sle_client_seek_disable_cbk] status error = %x\r\n", SLE_CLIENT_LOG, status);
    }
}

/**
 * @brief register callback functions for seek. 注册SLE设备发现回调函数
 */
static void sle_client_seek_cbk_register(void)
{
    g_sle_seek_cbk.sle_enable_cb = sle_client_sle_enable_cbk;        // SLE协议栈使能的回调函数
    g_sle_seek_cbk.seek_enable_cb = sle_client_seek_enable_cbk;      // 扫描使能的回调函数
    g_sle_seek_cbk.seek_result_cb = sle_client_seek_result_info_cbk; // 扫描结果上报的回调函数
    g_sle_seek_cbk.seek_disable_cb = sle_client_seek_disable_cbk;    // 扫描关闭的回调函数
    sle_announce_seek_register_callbacks(&g_sle_seek_cbk);           // 注册SLE设备发现回调函数
}

/**
 * @brief callback function for connection state changed. 连接状态改变的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. 指针由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  conn_id    连接 ID
 * @param  [in]  addr       地址
 * @param  [in]  conn_state 连接状态
 * @param  [in]  pair_state 配对状态
 * @param  [in]  disc_reason 断链原因
 * @see foundation\communication\sle\ohos_sle_connection_manager.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_connection_manager.h
 */
static void sle_client_connect_state_changed_cbk(uint16_t conn_id, const SleAddr *addr,
                                                 SleAcbStateType conn_state, SlePairStateType pair_state, SleDiscReasonType disc_reason)
{

    osal_printk("%s [sle_client_connect_state_changed_cbk] conn_id:0x%02x, conn_state:0x%x, pair_state:0x%x, \
        disc_reason:0x%x\r\n",
                SLE_CLIENT_LOG, conn_id, conn_state, pair_state, disc_reason);
    osal_printk("%s [sle_client_connect_state_changed_cbk] addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                SLE_CLIENT_LOG, addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3],
                addr->addr[4], addr->addr[5]);

    // 连接状态改变为已连接
    if (conn_state == SLE_ACB_STATE_CONNECTED)
    {
        g_conn_and_service_arr[g_num_conn].conn_id = conn_id; // 保存连接ID
        g_num_conn++;                                         // 连接ID统计+1

        osal_printk("%s [sle_client_connect_state_changed_cbk] SLE_ACB_STATE_CONNECTED\r\n", SLE_CLIENT_LOG);

        // SSAP exchange info. SSAP交换信息
        SsapcExchangeInfo info = {0};

        // mtu大小
        info.mtuSize = SLE_MTU_SIZE_DEFAULT;

        // 版本
        info.version = 1;

        // 发送交换info请求，mtu改变结果将在ssapc_exchange_info_callback中返回
        // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
        // ssapc_exchange_info_req接口
        SsapcExchangeInfoReq(0, conn_id, &info);

        // 配对远程设备
        // SlePairRemoteDevice(addr);
    }

    // 连接状态改变为未连接
    else if (conn_state == SLE_ACB_STATE_NONE)
    {
        osal_printk("%s [sle_client_connect_state_changed_cbk] SLE_ACB_STATE_NONE\r\n", SLE_CLIENT_LOG);
    }

    // 连接状态改变为已断开
    else if (conn_state == SLE_ACB_STATE_DISCONNECTED)
    {
        osal_printk("%s [sle_client_connect_state_changed_cbk] SLE_ACB_STATE_DISCONNECTED\r\n", SLE_CLIENT_LOG);

        // 遍历g_conn_and_service_arr，找到对应的连接ID，然后将这个数组元素从g_conn_and_service_arr中去除，
        // 并且将数组后面的元素前移，最后把空出来的数组元素清零
        for (int i = 0; i < g_num_conn; i++)
        {
            if (g_conn_and_service_arr[i].conn_id == conn_id) // 找到了对应的连接ID
            {
                // 将这个数组元素从g_conn_and_service_arr中去除，并且将数组后面的元素前移
                int j;
                for (j = i; j < g_num_conn - 1; j++)
                {
                    g_conn_and_service_arr[j] = g_conn_and_service_arr[j + 1];
                }
                // 把空出来的数组元素清零
                memset(&g_conn_and_service_arr[j], 0, sizeof(sle_conn_and_service_t));
                // 连接ID统计-1
                g_num_conn--;
            }
        }

        // 如果不调用SleStopSeek()，则一直扫描，所以不需要再次扫描
        // sle_start_scan(); // 设置 设备公开 扫描参数，并开始 设备公开 扫描
    }

    else
    {
        osal_printk("%s [sle_client_connect_state_changed_cbk] status error\r\n", SLE_CLIENT_LOG);
    }
}

/**
 * @brief register callback functions for connection. 注册SLE连接管理回调函数
 */
static void sle_client_connect_cbk_register(void)
{
    // 连接状态改变的回调函数
    g_sle_connect_cbk.connectStateChangedCb = sle_client_connect_state_changed_cbk;

    // 注册SLE连接管理回调函数
    SleConnectionRegisterCallbacks(&g_sle_connect_cbk);
}

/**
 * @brief callback function for exchange info. mtu改变的回调函数
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
    osal_printk("%s [sle_client_exchange_info_cbk] client id:%d status:%d\r\n",
                SLE_CLIENT_LOG, client_id, status);
    osal_printk("%s [sle_client_exchange_info_cbk] mtu size: %d, version: %d\r\n",
                SLE_CLIENT_LOG, param->mtu_size, param->version);

    if (status == ERRCODE_SUCC)
    {
        ssapc_find_structure_param_t find_param = {0}; // SSAP find operation parameter. SSAP查找参数

        // 查找类型：SSAP_FIND_TYPE_PRIMARY_SERVICE
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

        // 查找服务、特征、描述符
        // 服务发现结果将在 ssapc_find_structure_callback 和 ssapc_find_structure_complete_callback 中返回
        // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
        // ssapc_find_structure接口
        ssapc_find_structure(client_id, conn_id, &find_param);
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
    osal_printk("%s [sle_client_find_structure_cbk] client: %d conn_id:%d status: %d \r\n",
                SLE_CLIENT_LOG, client_id, conn_id, status);
    osal_printk("%s [sle_client_find_structure_cbk] start_hdl:[0x%02x], end_hdl:[0x%02x], uuid len:%d\r\n",
                SLE_CLIENT_LOG, service->start_hdl, service->end_hdl, service->uuid.len);

    // 遍历g_conn_and_service_arr，找到对应的连接ID，然后保存服务发现结果
    for (int i = 0; i < g_num_conn; i++)
    {
        if (g_conn_and_service_arr[i].conn_id == conn_id) // 找到了对应的连接ID
        {
            // 服务起始句柄
            g_conn_and_service_arr[i].find_service_result.start_hdl = service->start_hdl;
            // 服务结束句柄
            g_conn_and_service_arr[i].find_service_result.end_hdl = service->end_hdl;
            // 服务UUID
            memcpy_s(&g_conn_and_service_arr[i].find_service_result.uuid, sizeof(sle_uuid_t),
                     &service->uuid, sizeof(sle_uuid_t));
        }
    }
}

/**
 * @brief callback function for find property. 属性发现的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  property  特征。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
static void sle_client_find_property_cbk(uint8_t client_id, uint16_t conn_id,
                                         ssapc_find_property_result_t *property, errcode_t status)
{
    osal_printk("%s [sle_client_find_property_cbk] client id: %d, conn id: %d, operate ind: %d, "
                "descriptors count: %d status:%d property->handle %d\r\n",
                SLE_CLIENT_LOG,
                client_id, conn_id, property->operate_indication,
                property->descriptors_count, status, property->handle);
    g_sle_send_param.handle = property->handle;
    g_sle_send_param.type = SSAP_PROPERTY_TYPE_VALUE; // 属性类型：SSAP_PROPERTY_TYPE_VALUE（特征值）
}

/**
 * @brief callback function for find structure complete. 查找完成（发现特征完成）的回调函数
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
    unused(conn_id);

    osal_printk("%s [sle_client_find_structure_cmp_cbk] client id:%d status:%d type:%d uuid len:%d \r\n",
                SLE_CLIENT_LOG, client_id, status, structure_result->type, structure_result->uuid.len);
}

/**
 * @brief callback function for write cfm. 收到写响应的回调函数
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
    osal_printk("%s [sle_client_write_cfm_cbk] conn_id:%d client id:%d status:%d handle:%02x type:%02x\r\n",
                SLE_CLIENT_LOG, conn_id, client_id, status, write_result->handle, write_result->type);
}

/**
 * @brief Callback invoked when receive read response. 收到读响应的回调函数
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
    osal_printk("[sle_client_read_cfm_cbk] client id:0x%x conn id:0x%x status:0x%x\r\n", client_id, conn_id, status);
    osal_printk("[sle_client_read_cfm_cbk] handle:0x%x, type:0x%x, len:0x%x\r\n", read_data->handle, read_data->type,
                read_data->data_len);
    for (uint16_t idx = 0; idx < read_data->data_len; idx++)
    {
        osal_printk("[sle_client_read_cfm_cbk] [0x%x] 0x%02x\r\n", idx, read_data->data[idx]);
    }
}

/**
 * @brief register callback functions for ssapc. 注册SSAP客户端回调函数
 *
 * @param [in] ssapc_notification_cbk 收到通知的回调函数
 * @param [in] ssapc_indication_cbk 收到指示的回调函数
 */
static void sle_client_ssapc_cbk_register(ssapc_notification_callback ssapc_notification_cbk,
                                          ssapc_indication_callback ssapc_indication_cbk)
{
    g_sle_ssapc_cbk.exchange_info_cb = sle_client_exchange_info_cbk;           // mtu改变的回调函数
    g_sle_ssapc_cbk.find_structure_cb = sle_client_find_structure_cbk;         // 服务发现的回调函数
    g_sle_ssapc_cbk.ssapc_find_property_cbk = sle_client_find_property_cbk;    // 属性（特征）发现的回调函数
    g_sle_ssapc_cbk.find_structure_cmp_cb = sle_client_find_structure_cmp_cbk; // 查找完成（发现特征完成）的回调函数
    g_sle_ssapc_cbk.write_cfm_cb = sle_client_write_cfm_cbk;                   // 收到写响应的回调函数
    g_sle_ssapc_cbk.read_cfm_cb = sle_client_read_cfm_cbk;                     // 收到读响应的回调函数
    g_sle_ssapc_cbk.notification_cb = ssapc_notification_cbk;                  // 收到通知的回调函数
    g_sle_ssapc_cbk.indication_cb = ssapc_indication_cbk;                      // 收到指示的回调函数
    ssapc_register_callbacks(&g_sle_ssapc_cbk);                                // 注册SSAP客户端回调函数
}

/**
 * @brief 注册ssap客户端
 */
static errcode_t sle_uuid_client_register(void)
{
    errcode_t ret;

    // App uuid
    SleUuid app_uuid = {0};
    printf("[uuid client] ssapc_register_client \r\n");
    app_uuid.len = sizeof(g_sle_uuid_app_uuid);
    if (memcpy_s(app_uuid.uuid, app_uuid.len, g_sle_uuid_app_uuid, sizeof(g_sle_uuid_app_uuid)) != EOK)
    {
        return ERRCODE_SLE_FAIL;
    }

    // 注册SSAP客户端
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
    // ssapc_register_client接口
    ret = SsapcRegisterClient(&app_uuid, &g_client_id);

    return ret;
}

/**
 * @brief client通过handle向server发送数据：report
 * @param data 待发送数据
 * @param len 数据长度
 * @return 执行结果错误码
 */
errcode_t sle_client_send_report_by_handle(const uint8_t *data, uint8_t len)
{
    // SSAP send parameter. 写请求参数
    ssapc_write_param_t param = {0};

    // 向所有连接发送数据
    int ret = 0;
    for (int i = 0; i < g_num_conn; i++)
    {
        // g_conn_and_service_arr[i].find_service_result.start_hdl为0时，表示没有找到服务，不能发送数据
        // 或者也可以用g_conn_and_service_arr[i].find_service_result.uuid.len来判断是否找到服务
        if (g_conn_and_service_arr[i].find_service_result.start_hdl == 0)
        {
            continue; // 没有找到服务，不发送数据
        }

        // 属性句柄
        param.handle = g_conn_and_service_arr[i].find_service_result.start_hdl;

        // 属性类型：SSAP_PROPERTY_TYPE_VALUE（特征值）
        // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h中的
        // ssap_property_type_t
        param.type = SSAP_PROPERTY_TYPE_VALUE;

        // 数据长度
        param.data_len = len + 1; // 非星闪协议规定

        // 数据内容
        param.data = osal_vmalloc(param.data_len);
        if (param.data == NULL)
        {
            printf("[sle_client_send_report_by_handle] osal_vmalloc fail\r\n");
            return ERRCODE_SLE_FAIL;
        }
        if (memcpy_s(param.data, param.data_len, data, len) != EOK)
        {
            osal_vfree(param.data);
            return ERRCODE_SLE_FAIL;
        }

        // 发起写请求
        // 写结果将在 ssapc_write_cfm_callback 中返回
        // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h中的
        // ssapc_write_req接口
        ret = SsapWriteReq(g_client_id, g_conn_and_service_arr[i].conn_id, &param);
        if (ret != ERRCODE_SUCC)
        {
            printf("SsapWriteReq error:%d connid:%d\r\n", ret, g_conn_and_service_arr[i].conn_id);
        }

        osal_vfree(param.data);
    }

    return ERRCODE_SUCC;
}

/**
 * @brief 客户端向服务端发送数据
 * @param data 待发送数据
 * @param length 数据长度
 * @return 执行结果错误码
 */
int sle_client_send_data(uint8_t *data, uint8_t length)
{
    int ret;
    ret = sle_client_send_report_by_handle(data, length); // client通过handle向server发送数据
    return ret;
}

/**
 * @brief 检查SLE客户端是否已连接
 * @return true表示已连接，false表示未连接
 */
bool sle_client_is_connected(void)
{
    return (g_num_conn > 0);
}

/**
 * @brief callback function for notification. 收到通知的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  data      数据。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
void ssapc_notification_cbk(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                            errcode_t status)
{
    (void)client_id;
    (void)conn_id;
    (void)status;

    data->data[data->data_len - 1] = '\0';
    printf("[ssapc_notification_cbk] server_send_data: %s\r\n", data->data);
}

/**
 * @brief callback function for indication. 收到指示的回调函数
 * @attention  1. 该回调函数运行于SLE service线程，不能阻塞或长时间等待。
 * @attention  2. pointer由SLE service申请内存，也由SLE service释放，回调中不应释放。
 * @param  [in]  client_id 客户端 ID。
 * @param  [in]  conn_id   连接 ID。
 * @param  [in]  data      数据。
 * @param  [in]  status    执行结果错误码。
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_client.h
 */
void ssapc_indication_cbk(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                          errcode_t status)
{
    (void)client_id;
    (void)conn_id;
    (void)status;

    data->data[data->data_len - 1] = '\0';
    printf("[ssapc_indication_cbk] server_send_data: %s\r\n", data->data);
}

/**
 * @brief 初始化SLE客户端
 */
void sle_client_init()
{
    // 本地设备地址
    // 每个设备的地址不同，需要根据实际情况修改，或者使用随机地址
    uint8_t local_addr[SLE_ADDR_LEN] = {0x13, 0x67, 0x5c, 0x07, 0x00, 0x51}; // 0x04-->0x07
    random_mac_addr(local_addr);                                             // 随机地址
    printf("local_addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
           local_addr[0], local_addr[1], local_addr[2], local_addr[3], local_addr[4], local_addr[5]);
    SleAddr local_address;

    // 设置本地设备地址类型：公有地址
    // SLE_ADDRESS_TYPE_PUBLIC：公有地址
    // SLE_ADDRESS_TYPE_RANDOM：随机地址
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_common.h中的
    // sle_addr_type_t
    local_address.type = SLE_ADDRESS_TYPE_PUBLIC;

    // 设置本地设备地址
    (void)memcpy_s(local_address.addr, SLE_ADDR_LEN, local_addr, SLE_ADDR_LEN);

    // 注册ssap客户端
    sle_uuid_client_register();

    // 注册SLE设备发现回调函数
    sle_client_seek_cbk_register();

    // 注册SLE连接管理回调函数
    sle_client_connect_cbk_register();

    // 注册SSAP客户端回调函数
    sle_client_ssapc_cbk_register(ssapc_notification_cbk, ssapc_indication_cbk);

    // 使能SLE协议栈（开启SLE）
    EnableSle();

    // 设置本地设备地址（必须在开启SLE之后）
    SleSetLocalAddr(&local_address);
}

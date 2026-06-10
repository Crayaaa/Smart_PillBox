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

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "securec.h"
#include "sle_common.h"
#include "osal_debug.h"
#include "sle_errcode.h"
#include "osal_addr.h"
#include "osal_task.h"
#include "sle_connection_manager.h"
#include "sle_device_discovery.h"
#include "ohos_sle_common.h"
#include "ohos_sle_errcode.h"
#include "ohos_sle_ssap_server.h"
#include "ohos_sle_ssap_client.h"
#include "ohos_sle_device_discovery.h"
#include "ohos_sle_connection_manager.h"
#include "pinctrl.h"
#include "errcode.h"
#include "sle_server_adv.h"
#include "sle_server.h"

#define OCTET_BIT_LEN 8 // 8位
#define UUID_LEN_2 2    // uuid长度2字节
#define UUID_INDEX 14   // 14: uuid index
// #define BT_INDEX_4 4      // 4: bt index
// #define BT_INDEX_0 0      // 0: bt index

// 广播ID（设备公开ID/设备公开句柄），取值范围[0, 0xFF]
// 在sle_server_adv.c中也有定义
#define SLE_ADV_HANDLE_DEFAULT 1

// sle服务端app的UUID(通用唯一识别码)
static char g_sle_uuid_app_uuid[UUID_LEN_2] = {0x12, 0x34};

// server notify property uuid.
// 特征信息结构体（SsapsPropertyInfo）中，响应的数据（value）
static char g_sle_property_value[OCTET_BIT_LEN] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

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

// SLE连接状态
SleAcbStateType g_sle_conn_state = OH_SLE_ACB_STATE_NONE;

// SLE配对状态
SlePairStateType g_sle_pair_state = OH_SLE_PAIR_NONE;

// uuid长度16bit
#define UUID_16BIT_LEN 2

// uuid长度128bit
#define UUID_128BIT_LEN 16

// 日志标签
#define SLE_SERVER_LOG "[sle server]"

// sle base uuid
static uint8_t g_sle_base[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
                               0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/// @brief 16bit uuid encode. 16位uuid编码
/// @param _ptr uuid pointer
/// @param data uuid data
static void encode2byte_little(uint8_t *_ptr, uint16_t data)
{
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((data) >> 0x8);
    *(uint8_t *)(_ptr) = (uint8_t)(data);
}

/// @brief sle uuid set base
/// @param out sle uuid
static void sle_uuid_set_base(SleUuid *out)
{
    errcode_t ret;
    ret = memcpy_s(out->uuid, SLE_UUID_LEN, g_sle_base, SLE_UUID_LEN); // 星闪UUID长度：16
    if (ret != EOK)
    {
        printf("%s [sle_uuid_set_base] memcpy fail\n", SLE_SERVER_LOG);
        out->len = 0;
        return;
    }
    out->len = UUID_LEN_2;
}

/// @brief set sle uuid. 设置sle uuid
/// @param u2 后两字节
/// @param out sle uuid
static void sle_uuid_setu2(uint16_t u2, SleUuid *out)
{
    sle_uuid_set_base(out);
    out->len = UUID_LEN_2;
    encode2byte_little(&out->uuid[UUID_INDEX], u2);
}

/// @brief sle uuid print. 打印sle uuid
/// @param uuid sle uuid
static void sle_uuid_print(SleUuid *uuid)
{
    if (uuid == NULL)
    {
        printf("%s uuid_print,uuid is null\r\n", SLE_SERVER_LOG);
        return;
    }
    if (uuid->len == UUID_16BIT_LEN)
    {
        printf("%s uuid: %02x %02x.\n", SLE_SERVER_LOG,
               uuid->uuid[14], uuid->uuid[15]); /* 14 15: uuid index */
    }
    else if (uuid->len == UUID_128BIT_LEN)
    {
        printf("%s uuid: \n", SLE_SERVER_LOG); /* 14 15: uuid index */
        printf("%s 0x%02x 0x%02x 0x%02x \n", SLE_SERVER_LOG, uuid->uuid[0], uuid->uuid[1],
               uuid->uuid[2], uuid->uuid[3]);
        printf("%s 0x%02x 0x%02x 0x%02x \n", SLE_SERVER_LOG, uuid->uuid[4], uuid->uuid[5],
               uuid->uuid[6], uuid->uuid[7]);
        printf("%s 0x%02x 0x%02x 0x%02x \n", SLE_SERVER_LOG, uuid->uuid[8], uuid->uuid[9],
               uuid->uuid[10], uuid->uuid[11]);
        printf("%s 0x%02x 0x%02x 0x%02x \n", SLE_SERVER_LOG, uuid->uuid[12], uuid->uuid[13],
               uuid->uuid[14], uuid->uuid[15]);
    }
}

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
    printf("%s [ssaps_mtu_changed_cbk] server_id:%x, conn_id:%x, mtu_size:%x, status:%x\r\n",
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
    printf("%s [ssaps_start_service_cbk] server_id:%d, handle:%x, status:%x\r\n", SLE_SERVER_LOG,
           server_id, handle, status);
}

/**
 * @brief callback function for add service. 添加服务（服务注册）的回调函数
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
    printf("%s [ssaps_add_service_cbk] server_id:%x, handle:%x, status:%x\r\n", SLE_SERVER_LOG,
           server_id, handle, status);
    sle_uuid_print(uuid);
}

/**
 * @brief callback function for add property. 添加特征（特征注册）的回调函数
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
    printf("%s [ssaps_add_property_cbk] server_id:%x, service_handle:%x,handle:%x, status:%x\r\n",
           SLE_SERVER_LOG, server_id, service_handle, handle, status);
    sle_uuid_print(uuid);
}

/**
 * @brief callback function for add descriptor. 添加描述符（特征描述符注册）的回调函数
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
    printf("%s [ssaps_add_descriptor_cbk] server_id:%x, service_handle:%x, property_handle:%x, \
        status:%x\r\n",
           SLE_SERVER_LOG, server_id, service_handle, property_handle, status);
    sle_uuid_print(uuid);
}

/**
 * @brief callback function for delete all service. 删除全部服务的回调函数
 * @attention  1. 该回调函数运行于bts线程，不能阻塞或长时间等待。
 * @attention  2. devices由bts申请内存，也由bts释放，回调中不应释放。
 * @param  [in]  server_id 服务端 ID。
 * @param  [in]  status    执行结果错误码。
 * @see foundation\communication\sle\ohos_sle_ssap_server.h
 * @see device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h
 */
static void ssaps_delete_all_service_cbk(uint8_t server_id, errcode_t status)
{
    printf("%s [ssaps_delete_all_service_cbk] server_id:%x, status:%x\r\n", SLE_SERVER_LOG,
           server_id, status);
}

/// @brief register callback functions for ssaps. 注册SSAP服务端回调函数
static errcode_t sle_ssaps_register_cbks(SsapsReadRequestCallback ssaps_read_request_cbk,
                                         SsapsWriteRequestCallback ssaps_write_request_cbk)
{
    errcode_t ret;
    SsapsCallbacks ssaps_cbk = {0};
    ssaps_cbk.addServiceCb = ssaps_add_service_cbk;              // 添加服务（服务注册）的回调函数
    ssaps_cbk.addPropertyCb = ssaps_add_property_cbk;            // 添加特征（特征注册）的回调函数
    ssaps_cbk.addDescriptorCb = ssaps_add_descriptor_cbk;        // 添加描述符（特征描述符注册）的回调函数
    ssaps_cbk.startServiceCb = ssaps_start_service_cbk;          // 启动服务的回调函数
    ssaps_cbk.deleteAllServiceCb = ssaps_delete_all_service_cbk; // 删除全部服务的回调函数
    ssaps_cbk.mtuChangedCb = ssaps_mtu_changed_cbk;              // mtu大小改变的回调函数
    ssaps_cbk.readRequestCb = ssaps_read_request_cbk;            // 收到远端读请求的回调函数
    ssaps_cbk.writeRequestCb = ssaps_write_request_cbk;          // 收到远端写请求的回调函数
    ret = SsapsRegisterCallbacks(&ssaps_cbk);                    // 注册SSAP服务端回调函数
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_ssaps_register_cbks] ssaps_register_callbacks fail :%x\r\n", SLE_SERVER_LOG,
               ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

/// @brief add service for sle server. 添加sle服务端服务
static errcode_t sle_uuid_server_service_add(void)
{
    errcode_t ret;
    SleUuid service_uuid = {0};

    // SSAP服务UUID
    sle_uuid_setu2(SLE_UUID_SERVER_SERVICE, &service_uuid);

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

/// @brief add property and property descriptor for sle server. 添加sle服务端特征和特征描述符
static errcode_t sle_uuid_server_property_add(void)
{
    errcode_t ret;
    SsapsPropertyInfo property = {0}; // SSAP特征
    SsapsDescInfo descriptor = {0};   // SSAP特征描述符
    uint8_t ntf_value[] = {0x01, 0x02};

    // 特征权限：可读可写
    property.permissions = SLE_UUID_TEST_PROPERTIES;

    // 操作指示：数据值可被读取，数据值可被写入，写入后产生反馈给客户端
    property.operateIndication = SLE_UUID_TEST_OPERATION_INDICATION;

    // SSAP特征UUID
    sle_uuid_setu2(SLE_UUID_SERVER_NTF_REPORT, &property.uuid);

    // 响应的数据长度
    property.valueLen = OCTET_BIT_LEN;

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

    // 添加一个SSAP特征
    // 参数包括：服务端ID、服务句柄、SSAP特征信息、特征句柄
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_add_property_sync接口
    ret = SsapsAddPropertySync(g_server_id, g_service_handle, &property, &g_property_handle);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s sle add property fail, ret:%x\r\n", SLE_SERVER_LOG, ret);
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }

    // 特征描述符权限：可读可写
    descriptor.permissions = SLE_UUID_TEST_DESCRIPTOR;

    // 特征描述符类型：客户端配置描述符
    descriptor.type = SSAP_DESCRIPTOR_CLIENT_CONFIGURATION;

    // 操作指示：数据值可被读取，数据值可被写入，写入后产生反馈给客户端
    descriptor.operateIndication = SLE_UUID_TEST_OPERATION_INDICATION;

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

    // 添加一个SSAP特征描述符
    // 参数包括：服务端ID、服务句柄、描述符所在的特征句柄、特征描述符
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_add_descriptor_sync接口
    ret = SsapsAddDescriptorSync(g_server_id, g_service_handle, g_property_handle, &descriptor);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s sle add descriptor fail, ret:%x\r\n", SLE_SERVER_LOG, ret);
        osal_vfree(property.value);
        osal_vfree(descriptor.value);
        return ERRCODE_SLE_FAIL;
    }

    osal_vfree(property.value);
    osal_vfree(descriptor.value);
    return ERRCODE_SLE_SUCCESS;
}

/// @brief add sle server. 添加sle服务端
/// @callergraph sle_server_add --> sle_enable_server_cbk
static errcode_t sle_server_add(void)
{
    errcode_t ret;
    sle_uuid_t app_uuid = {0}; // sle app UUID(通用唯一识别码)

    // 注册sle服务端
    printf("%s sle add service in\r\n", SLE_SERVER_LOG);
    app_uuid.len = sizeof(g_sle_uuid_app_uuid);
    if (memcpy_s(app_uuid.uuid, app_uuid.len, g_sle_uuid_app_uuid, sizeof(g_sle_uuid_app_uuid)) != EOK)
    {
        return ERRCODE_SLE_FAIL;
    }
    ssapsRegisterServer(&app_uuid, &g_server_id); // 注册SSAP服务端

    // 添加sle服务端服务
    if (sle_uuid_server_service_add() != ERRCODE_SLE_SUCCESS)
    {
        SsapsUnregisterServer(g_server_id); // 注销SSAP服务端
        return ERRCODE_SLE_FAIL;
    }

    // 添加sle服务端特征和特征描述符
    if (sle_uuid_server_property_add() != ERRCODE_SLE_SUCCESS)
    {
        SsapsUnregisterServer(g_server_id); // 注销SSAP服务端
        return ERRCODE_SLE_FAIL;
    }
    printf("%s sle add service, server_id:%x, service_handle:%x, property_handle:%x\r\n",
           SLE_SERVER_LOG, g_server_id, g_service_handle, g_property_handle);

    // 启动sle服务端服务（开始一个SSAP服务）
    // 参数包括：服务端ID、服务句柄
    // 服务开启结果将在 ssaps_start_service_callback 中返回
    // 参考：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_start_service接口
    ret = SsapsStartService(g_server_id, g_service_handle);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s sle add service fail, ret:%x\r\n", SLE_SERVER_LOG, ret);
        return ERRCODE_SLE_FAIL;
    }
    printf("%s sle add service out\r\n", SLE_SERVER_LOG);

    return ERRCODE_SLE_SUCCESS;
}

/// @brief server通过handle向client发送数据：notify
errcode_t sle_server_send_notify_by_handle(const uint8_t *data, uint8_t len)
{
    // notification/indication information. 通知或指示信息
    SsapsNtfInd param = {0};

    // 属性句柄
    param.handle = g_property_handle;

    // 属性类型：SSAP_PROPERTY_TYPE_VALUE（特征值）
    // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_stru.h中的
    // ssap_property_type_t
    param.type = SSAP_PROPERTY_TYPE_VALUE;

    // 通知/指示 数据长度
    param.valueLen = len + 1; // 非星闪协议规定

    // 通知/指示 数据内容
    param.value = osal_vmalloc(param.valueLen);
    if (param.value == NULL)
    {
        printf("[sle_server_send_notify_by_handle] osal_vmalloc fail\r\n");
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(param.value, param.valueLen, data, len) != EOK)
    {
        osal_vfree(param.value);
        return ERRCODE_SLE_FAIL;
    }

    // 向对端发送通知或指示
    // 具体发送状态取决于特征描述符：客户端特征配置
    // value = 0x0000：不允许通知和指示
    // value = 0x0001：允许通知
    // value = 0x0002：允许指示
    // 如果向全部对端发送则conn_id = 0xffff
    // 参见：
    // device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_ssap_server.h中的
    // ssaps_notify_indicate接口
    if (SsapsNotifyIndicate(g_server_id, g_sle_conn_id, &param) != ERRCODE_SUCC)
    {
        printf("[sle_server_send_notify_by_handle] SsapsNotifyIndicate fail\r\n");
        osal_vfree(param.value);
        return ERRCODE_SLE_FAIL;
    }

    osal_vfree(param.value);
    return ERRCODE_SLE_SUCCESS;
}

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

    printf("%s [sle_connect_state_changed_cbk] conn_id:0x%02x, conn_state:0x%x, pair_state:0x%x, \
        disc_reason:0x%x\r\n",
           SLE_SERVER_LOG, conn_id, conn_state, pair_state, disc_reason);
    printf("%s [sle_connect_state_changed_cbk] addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
           SLE_SERVER_LOG, addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3],
           addr->addr[4], addr->addr[5]);

    // 连接状态改变为已连接
    if (conn_state == OH_SLE_ACB_STATE_CONNECTED)
    {
        g_sle_conn_id = conn_id;
    }

    // 连接状态改变为已断开
    else if (conn_state == OH_SLE_ACB_STATE_DISCONNECTED)
    {
        g_sle_conn_id = 0;
        g_sle_pair_state = OH_SLE_PAIR_NONE; // 配对状态改变为未配对

        // 开始广播（开始设备公开）
        // 参数：广播ID（设备公开ID/设备公开句柄）
        // 参见：device\soc\hisilicon\ws63v100\sdk\include\middleware\services\bts\sle\sle_device_discovery.h中的
        // sle_start_announce接口
        errcode_t ret = SleStartAnnounce(SLE_ADV_HANDLE_DEFAULT);
        if (ret != ERRCODE_SLE_SUCCESS)
        {
            printf("%s [sle_connect_state_changed_cbk] sle_start_announce fail :%x\r\n", SLE_SERVER_LOG, ret);
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
    printf("%s [sle_pair_complete_cbk] pair complete conn_id:%02x, status:%x\r\n", SLE_SERVER_LOG,
           conn_id, status);
    printf("%s [sle_pair_complete_cbk] pair complete addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
           SLE_SERVER_LOG, addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3],
           addr->addr[4], addr->addr[5]);
    if (status == ERRCODE_SUCC)
    {
        g_sle_pair_state = OH_SLE_PAIR_PAIRED;
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
    printf("%s [sle_connect_param_update_req_cbk] conn_id:%02x, status:%x, interval_min:%02x, interval_max:%02x, max_latency:%02x, supervision_timeout:%02x\r\n",
           SLE_SERVER_LOG, conn_id, status, param->interval_min, param->interval_max, param->max_latency, param->supervision_timeout);
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
    printf("%s [sle_connect_param_update_cbk] conn_id:%02x, status:%x, interval:%02x, latency:%02x, supervision:%02x\r\n",
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
    printf("%s [sle_auth_complete_cbk] auth complete conn_id:%02x, status:%x\r\n", SLE_SERVER_LOG,
           conn_id, status);
    printf("%s [sle_auth_complete_cbk] auth complete addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
           SLE_SERVER_LOG, addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3],
           addr->addr[4], addr->addr[5]);

    printf("%s [sle_auth_complete_cbk] auth complete link_key:%02x, crypto_algo:%x, key_deriv_algo:%x, integr_chk_ind:%x\r\n",
           SLE_SERVER_LOG, evt->link_key, evt->crypto_algo, evt->key_deriv_algo, evt->integr_chk_ind);
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
    printf("%s [sle_read_rssi_cbk] conn_id:%02x, rssi:%d, status:%x\r\n", SLE_SERVER_LOG,
           conn_id, rssi, status);
}

/// @brief register callback functions for connection. 注册SLE连接管理回调函数
static errcode_t sle_conn_register_cbks(void)
{
    errcode_t ret;
    SleConnectionCallbacks conn_cbks = {0};
    conn_cbks.connectStateChangedCb = sle_connect_state_changed_cbk;      // 连接状态改变的回调函数
    conn_cbks.connectParamUpdateReqCb = sle_connect_param_update_req_cbk; // 连接参数更新请求完成前的回调函数
    conn_cbks.connectParamUpdateCb = sle_connect_param_update_cbk;        // 连接参数更新的回调函数
    conn_cbks.authCompleteCb = sle_auth_complete_cbk;                     // 认证完成的回调函数
    conn_cbks.pairCompleteCb = sle_pair_complete_cbk;                     // 配对完成的回调函数
    conn_cbks.readRssiCb = sle_read_rssi_cbk;                             // 读取rssi的回调函数

    ret = SleConnectionRegisterCallbacks(&conn_cbks); // 注册SLE连接管理回调函数
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_conn_register_cbks] sle_connection_register_callbacks fail :%x\r\n",
               SLE_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
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
void ssaps_read_request_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_read_cb_t *read_cb_para,
                            errcode_t status)
{
    printf("[ssaps_read_request_cbk] server_id:0x%x, conn_id:0x%x, handle:0x%x, type:0x%x, status:0x%x\r\n",
           server_id, conn_id, read_cb_para->handle, read_cb_para->type, status);
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
// 外部变量声明，用于更新药品显示
extern int pill_name;
extern void update_pill_display(void);

void ssaps_write_request_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_write_cb_t *write_cb_para,
                             errcode_t status)
{
    (void)server_id;
    (void)conn_id;
    (void)status;
    
    if (write_cb_para->length > 0) {
        write_cb_para->value[write_cb_para->length - 1] = '\0';
        printf("[ssaps_write_request_cbk] client_send_data: %s (length: %d)\r\n", 
               write_cb_para->value, write_cb_para->length);

        // 根据接收到的数据更新药品名称
        if (write_cb_para->length > 0) {
            char received_char = write_cb_para->value[0];
            printf("[ssaps_write_request_cbk] Received character: %c\r\n", received_char);
            
            // 根据接收到的字符更新药品名称
            switch (received_char) {
                case 'A':
                    pill_name = 0; // 999感冒灵
                    printf("[ssaps_write_request_cbk] Switching to: 999感冒灵\r\n");
                    break;
                case 'B':
                    pill_name = 1; // 左氧氟沙星滴眼液
                    printf("[ssaps_write_request_cbk] Switching to: 左氧氟沙星滴眼液\r\n");
                    break;
                case 'C':
                    pill_name = 2; // 浦蓝地消炎片
                    printf("[ssaps_write_request_cbk] Switching to: 浦蓝地消炎片\r\n");
                    break;
                case 'D':
                    pill_name = 3; // 双黄连口服液
                    printf("[ssaps_write_request_cbk] Switching to: 双黄连口服液\r\n");
                    break;
                default:
                    printf("[ssaps_write_request_cbk] Unknown character: %c, keeping current pill\r\n", received_char);
                    break;
            }
            
            // 更新OLED显示
            update_pill_display();
        }
    }

    // 向SLE客户端发送响应数据
    char *data = "Response from SLE server!\n";
    int ret = sle_server_send_data((uint8_t *)data, strlen(data));
}

/// @brief initialize sle server. 初始化SLE服务端
errcode_t sle_server_init()
{
    errcode_t ret;

    // 注册SLE设备公开和设备发现回调函数
    ret = sle_announce_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] sle_announce_register_cbks fail :%x\r\n",
               SLE_SERVER_LOG, ret);
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
    ret = sle_ssaps_register_cbks(ssaps_read_request_cbk, ssaps_write_request_cbk);
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] sle_ssaps_register_cbks fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    // 使能SLE协议栈（开启SLE）
    // 在SLE协议栈使能的回调函数(sle_enable_cbk)中：添加sle服务端，初始化sle服务端的广播参数和数据，然后启动广播
    ret = EnableSle();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_server_init] enable_sle fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    printf("%s [sle_server_init] init ok\r\n", SLE_SERVER_LOG);
    return ERRCODE_SLE_SUCCESS;
}

/// @brief callback function for enable server. 服务端使能的回调函数（SLE协议栈使能的回调函数）
/// 添加sle服务端，初始化sle服务端的广播参数和数据，然后启动广播
/// @callergraph sle_enable_server_cbk --> sle_enable_cbk --> sle_announce_register_cbks --> sle_server_init
errcode_t sle_enable_server_cbk(void)
{
    errcode_t ret;

    // 添加sle服务端
    ret = sle_server_add();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_enable_cbk] sle_server_add fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    // 初始化sle服务端的广播参数和数据，然后启动广播
    ret = sle_server_adv_init();
    if (ret != ERRCODE_SLE_SUCCESS)
    {
        printf("%s [sle_enable_cbk] sle_server_adv_init fail :%x\r\n", SLE_SERVER_LOG, ret);
        return ret;
    }

    return ERRCODE_SLE_SUCCESS;
}

/// @brief server向client发送数据
/// @param data 待发送数据
/// @param length 数据长度
int sle_server_send_data(uint8_t *data, uint8_t length)
{
    int ret;
    ret = sle_server_send_notify_by_handle(data, length);
    return ret;
}

/// @brief 等待客户端配对完成
void sle_wait_client_paired(void)
{
    while (g_sle_pair_state != OH_SLE_PAIR_PAIRED)
    {
        osal_msleep(100); // 轮询间隔100ms
    }
}

/// @brief 等待客户端连接完成
void sle_wait_client_connected(void)
{
    while (g_sle_conn_state != OH_SLE_ACB_STATE_CONNECTED)
    {
        osal_msleep(100); // 轮询间隔100ms
    }
}

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

#ifndef SLE_CLIENT_H
#define SLE_CLIENT_H

#include "sle_ssap_client.h"

// SLE connection id and service discovery result
// SLE连接ID和服务发现结果
typedef struct
{
    uint16_t conn_id;                                // SLE connection id. SLE连接ID
    ssapc_find_service_result_t find_service_result; // Service discovery result. 服务发现结果
} sle_conn_and_service_t;

void sle_set_server_name(char *name);
void sle_client_init(void);
void sle_start_scan(void);
int sle_client_send_data(uint8_t *data, uint8_t length);
sle_conn_and_service_t *sle_get_conn_and_service(void);
ssapc_write_param_t *sle_get_send_param(void);

#endif
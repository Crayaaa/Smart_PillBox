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

#include <stdio.h>     // 标准输入输出
#include <unistd.h>    // POSIX标准接口
#include "ohos_init.h" // 用于初始化服务(services)和功能(features)
#include "cmsis_os2.h" // CMSIS-RTOS API V2

#include "sle_server.h"     // SLE服务端接口
#include "sle_server_adv.h" // SLE服务端广播接口

// 主线程函数
static void SleTask(char *arg)
{
    (void)arg;

    // 设置SLE服务端广播设备名称
    sle_set_announce_name("sle_server");

    // 初始化SLE服务端
    sle_server_init();

    // 服务端向客户端发送数据。此处默认注释掉，如需使用请取消注释
#if 0
    // 等待客户端连接完成，连接完成后才能发送数据
    sle_wait_client_connected();

    // 向SLE客户端发送数据
    char data[16] = {0};
    int count = 1;
    while (1)
    {
        sprintf(data, "%d", count);
        int ret = sle_server_send_data((uint8_t *)data, strlen(data));
        count++;
        osDelay(100);
    }
#endif
}

// 入口函数
static void SleServerDemo(void)
{
    // 定义线程属性
    osThreadAttr_t attr;
    attr.name = "SleTask";
    attr.stack_size = 1024 * 10; // CHIP_WS63
    attr.priority = osPriorityNormal;

    // 创建线程，运行主线程函数
    if (osThreadNew(SleTask, NULL, &attr) == NULL)
        printf("[SleServerDemo] Falied to create SleTask!\n");
    else
        printf("[SleServerDemo] Create SleTask successfully!\n");
}

// 运行入口函数
APP_FEATURE_INIT(SleServerDemo);

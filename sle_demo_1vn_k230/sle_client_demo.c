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

#include <stdio.h>      // 标准输入输出
#include <unistd.h>     // POSIX标准接口
#include <string.h>     // 字符串处理
#include <stdbool.h>    // 布尔类型
#include "ohos_init.h"  // 用于初始化服务(services)和功能(features)
#include "cmsis_os2.h"  // CMSIS-RTOS API V2
#include "pinctrl.h"    // 引脚控制
#include "uart.h"       // 串口驱动
#include "watchdog.h"   // 看门狗
#include "osal_debug.h" // OSAL调试
#include "soc_osal.h"   // SOC OSAL
#include "app_init.h"   // 应用初始化

#include "sle_client.h" // SLE客户端接口

// 串口相关定义
#define UART_BAUDRATE 115200
#define UART_DATA_BITS 3
#define UART_STOP_BITS 1
#define UART_PARITY_BIT 0
#define UART_TRANSFER_SIZE 18
#define CONFIG_UART1_TXD_PIN 15
#define CONFIG_UART1_RXD_PIN 16
#define CONFIG_UART1_PIN_MODE 1
#define CONFIG_UART1_BUS_ID 1
#define CONFIG_UART_INT_WAIT_MS 5

// 串口接收缓冲区
static uint8_t g_app_uart_rx_buff[UART_TRANSFER_SIZE] = {0};
static uart_buffer_config_t g_app_uart_buffer_config = {
    .rx_buffer = g_app_uart_rx_buff,
    .rx_buffer_size = UART_TRANSFER_SIZE
};

// 串口接收标志
static volatile bool g_uart_data_received = false;

// 上一次接收到的完整数据包，用于去重
static uint8_t g_last_received_packet[UART_TRANSFER_SIZE] = {0};
static uint16_t g_last_packet_length = 0;

/**
 * @brief 打印hex数据
 */
static void print_hex_data(const char* prefix, const uint8_t* data, int length)
{
    printf("%s: ", prefix);
    for (int i = 0; i < length; i++) {
        printf("0x%02X ", data[i]);
    }
    printf("(%d bytes)\r\n", length);
}

/**
 * @brief 检查是否为有效的hex数据
 */
static bool is_valid_hex_data(const uint8_t* data, int length)
{
    // 检查是否全为0（通常表示无效数据）
    bool all_zero = true;
    for (int i = 0; i < length; i++) {
        if (data[i] != 0) {
            all_zero = false;
            break;
        }
    }
    return !all_zero;
}

/**
 * @brief 串口引脚初始化
 */
static void app_uart_init_pin(void)
{
    uapi_pin_set_mode(CONFIG_UART1_TXD_PIN, CONFIG_UART1_PIN_MODE);
    uapi_pin_set_mode(CONFIG_UART1_RXD_PIN, CONFIG_UART1_PIN_MODE);
}

/**
 * @brief 串口配置初始化
 */
static void app_uart_init_config(void)
{
    uart_attr_t attr = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_BITS,
        .stop_bits = UART_STOP_BITS,
        .parity = UART_PARITY_BIT
    };

    uart_pin_config_t pin_config = {
        .tx_pin = S_MGPIO0, 
        .rx_pin = S_MGPIO1, 
        .cts_pin = PIN_NONE, 
        .rts_pin = PIN_NONE
    };
    
    uapi_uart_deinit(CONFIG_UART1_BUS_ID); // 重点，UART初始化之前需要去初始化，否则会报0x80001044
    int res = uapi_uart_init(CONFIG_UART1_BUS_ID, &pin_config, &attr, NULL, &g_app_uart_buffer_config);
    if (res != 0) {
        printf("uart init failed res = %02x\r\n", res);
    }
}

/**
 * @brief 串口中断接收处理函数
 */
static void app_uart_read_int_handler(const void *buffer, uint16_t length, bool error)
{
    (void)error;
    if (buffer == NULL || length == 0) {
        osal_printk("uart%d int mode transfer illegal data!\r\n", CONFIG_UART1_BUS_ID);
        return;
    }

    uint8_t *buff = (uint8_t *)buffer;
    if (memcpy_s(g_app_uart_rx_buff, length, buff, length) != EOK) {
        osal_printk("uart%d int mode data copy fail!\r\n", CONFIG_UART1_BUS_ID);
        return;
    }
    
    // 打印接收到的hex数据
    printf("[UART_INT_RX] ");
    print_hex_data("Received", buff, length);
    
    // 检查是否为有效数据
    if (is_valid_hex_data(buff, length)) {
        // 检查是否为重复的数据包
        bool is_duplicate_packet = false;
        if (length == g_last_packet_length) {
            is_duplicate_packet = true;
            for (int i = 0; i < length; i++) {
                if (buff[i] != g_last_received_packet[i]) {
                    is_duplicate_packet = false;
                    break;
                }
            }
        }
        
        if (is_duplicate_packet) {
            printf("[UART_INT_RX] Duplicate packet received - Skipping all SLE transmissions\r\n");
        } else {
            // 处理接收到的每个字节
            for (int i = 0; i < length; i++) {
                char sle_data[2] = {0}; // 存储要发送的字符
                bool should_send = false;
                
                // 根据接收到的数据判断要发送的字符
                switch (buff[i]) {
                    case 0xAA:
                        sle_data[0] = 'A';
                        should_send = true;
                        printf("[UART_INT_RX] Command 0xAA received - Sending 'A' via SLE\r\n");
                        break;
                    case 0xBB:
                        sle_data[0] = 'B';
                        should_send = true;
                        printf("[UART_INT_RX] Command 0xBB received - Sending 'B' via SLE\r\n");
                        break;
                    case 0xCC:
                        sle_data[0] = 'C';
                        should_send = true;
                        printf("[UART_INT_RX] Command 0xCC received - Sending 'C' via SLE\r\n");
                        break;
                    case 0xDD:
                        sle_data[0] = 'D';
                        should_send = true;
                        printf("[UART_INT_RX] Command 0xDD received - Sending 'D' via SLE\r\n");
                        break;
                    default:
                        printf("[UART_INT_RX] Unknown command: 0x%02X - No action taken\r\n", buff[i]);
                        break;
                }
                
                // 如果需要发送数据，则通过SLE发送
                if (should_send) {
                    int ret = sle_client_send_data((uint8_t *)sle_data, strlen(sle_data));
                    if (ret == 0) {
                        printf("[UART_INT_RX] Successfully sent '%c' via SLE\r\n", sle_data[0]);
                    } else {
                        printf("[UART_INT_RX] Failed to send '%c' via SLE, error: %d\r\n", sle_data[0], ret);
                    }
                }
            }
            
            // 保存当前数据包用于下次去重比较
            g_last_packet_length = length;
            if (memcpy_s(g_last_received_packet, UART_TRANSFER_SIZE, buff, length) != EOK) {
                printf("[UART_INT_RX] Failed to save packet for deduplication\r\n");
            }
        }
    } else {
        printf("[UART_INT_RX] Invalid data received (all zeros)\r\n");
    }
    
    g_uart_data_received = true;
}

/**
 * @brief 初始化串口功能
 */
static void uart_init(void)
{
    // 串口引脚初始化
    app_uart_init_pin();
    // 串口配置初始化
    app_uart_init_config();
    
    // 注册串口接收中断回调
    osal_printk("uart%d int mode register receive callback start!\r\n", CONFIG_UART1_BUS_ID);
    if (uapi_uart_register_rx_callback(CONFIG_UART1_BUS_ID, UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE, 1,
                                       app_uart_read_int_handler) == ERRCODE_SUCC) {
        osal_printk("uart%d int mode register receive callback succ!\r\n", CONFIG_UART1_BUS_ID);
        printf("[UART] ✅ UART initialized successfully!\r\n");
        printf("[UART] 📋 Supported commands:\r\n");
        printf("[UART]    0xAA -> Send 'A' via SLE\r\n");
        printf("[UART]    0xBB -> Send 'B' via SLE\r\n");
        printf("[UART]    0xCC -> Send 'C' via SLE\r\n");
        printf("[UART]    0xDD -> Send 'D' via SLE\r\n");
        printf("[UART] 🔄 Only NEW packets trigger SLE transmission (duplicate packets are ignored)\r\n");
    } else {
        printf("[UART] ❌ Failed to register UART receive callback\r\n");
    }
}

// 主线程函数
static void SleTask(char *arg)
{
    (void)arg;

    // 初始化串口功能
    uart_init();

    // 设置SLE服务端名称，要与服务端一致
    sle_set_server_name("sle_server");

    // 初始化SLE客户端
    sle_client_init();

    printf("[SleTask] ✅ SLE Client and UART initialized successfully!\r\n");
    printf("[SleTask] 📡 Waiting for SLE server connection...\r\n");
    
    // 开始扫描SLE服务端
    printf("[SleTask] 🔍 Starting scan for SLE server 'sle_server'...\r\n");
    sle_start_scan();
    
    // 等待连接建立
    int connection_attempts = 0;
    bool connected = false;
    
    while (!connected && connection_attempts < 100) { // 最多等待100次，每次100ms
        // 检查实际的连接状态
        connected = sle_client_is_connected();
        
        if (!connected) {
            osDelay(100);
            connection_attempts++;
            
            if (connection_attempts % 10 == 0) {
                printf("[SleTask] 🔍 Still scanning for server... (attempt %d/100)\r\n", connection_attempts);
            }
        } else {
            printf("[SleTask] 🔗 SLE connection established!\r\n");
        }
    }
    
    if (!connected) {
        printf("[SleTask] ❌ Failed to connect to SLE server after 10 seconds\r\n");
        printf("[SleTask] 📟 UART will still work, but SLE communication is disabled\r\n");
    } else {
        printf("[SleTask] 📟 UART ready to receive commands (0xAA/0xBB/0xCC/0xDD) with packet deduplication...\r\n");
    }

    // 主循环 - 保持程序运行
    int count = 1;
    while (1)
    {
        // 心跳包已禁用 - 只通过UART命令发送数据
        count++;
        osDelay(100); // 100ms延时
    }
}

// 入口函数
static void SleClientDemo(void)
{
    // 定义线程属性
    osThreadAttr_t attr;
    attr.name = "SleTask";
    attr.stack_size = 1024 * 60; // CHIP_WS63
    attr.priority = osPriorityNormal;

    // 创建线程，运行主线程函数
    if (osThreadNew(SleTask, NULL, &attr) == NULL)
        printf("[SleClientDemo] Falied to create SleTask!\n");
    else
        printf("[SleClientDemo] Create SleTask successfully!\n");
}

// 运行入口函数
APP_FEATURE_INIT(SleClientDemo);

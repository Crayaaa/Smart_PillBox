/*
 * Copyright (c) 2024-2025, Qi Yaolong.
 * dragon@hbu.edu.cn
 * HeBei University, China.
 * 
 * 保留所有权利。除非遵守以下条款，否则您不得使用此文件：
 * 本文件及其内容仅用于个人学习用途，在此范围之外的使用请先获得版权持有人的授权。
 *
 * 本程序由版权持有人和贡献人"按原样"提供，不提供任何明示或暗示担保，包括但不局限于对适销性和特定
 * 用途适合性的暗示担保。在任何情况下，版权持有人或贡献人对因使用本程序而导致的任何直接、间接、附带、
 * 特殊、典型或后果性损害（包括但不限于购买替代商品或服务；使用损失、数据丢失或利润损失；业务中断）
 * 不承担任何责任，无论其如何产生，也不论其责任理论为何，无论是合同、严格责任还是侵权（包括疏忽或
 * 其他），即使已告知此类损害的可能性。
 */

#include <stdio.h>     // 标准输入输出
#include <unistd.h>    // POSIX标准接口
#include <string.h>    // 字符串处理
#include "ohos_init.h" // 用于初始化服务(services)和功能(features)
#include "cmsis_os2.h" // CMSIS-RTOS API V2

#include "sle_server.h"     // SLE服务端接口
#include "sle_server_adv.h" // SLE服务端广播接口

// UART相关头文件
#include "pinctrl.h"
#include "uart.h"
#include "watchdog.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "app_init.h"

// UART配置参数
#define UART_BAUDRATE 9600
#define UART_DATA_BITS 3
#define UART_STOP_BITS 1
#define UART_PARITY_BIT 0
#define UART_TRANSFER_SIZE 18
#define CONFIG_UART1_TXD_PIN 15
#define CONFIG_UART1_RXD_PIN 16
#define CONFIG_UART1_PIN_MODE 1
#define CONFIG_UART1_BUS_ID 1
#define CONFIG_UART_INT_WAIT_MS 5

#define UART_TASK_STACK_SIZE 0x1000
#define UART_TASK_DURATION_MS 1000
#define UART_TASK_PRIO 17

// UART数据缓冲区
static uint8_t g_app_uart_tx_buff[UART_TRANSFER_SIZE] = {0};  // 清空测试数据
static uint8_t g_app_uart_rx_buff[UART_TRANSFER_SIZE] = {0};

#define CONFIG_UART_POLL_TRANSFER_MODE 1
#if defined(CONFIG_UART_INT_TRANSFER_MODE)
static uint8_t g_app_uart_int_rx_flag = 0;
#endif

static uart_buffer_config_t g_app_uart_buffer_config = {.rx_buffer = g_app_uart_rx_buff,
                                                        .rx_buffer_size = UART_TRANSFER_SIZE};

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
 * @brief 初始化UART引脚
 */
static void app_uart_init_pin(void)
{
    uapi_pin_set_mode(CONFIG_UART1_TXD_PIN, CONFIG_UART1_PIN_MODE);
    uapi_pin_set_mode(CONFIG_UART1_RXD_PIN, CONFIG_UART1_PIN_MODE);
}

/**
 * @brief 初始化UART配置
 */
static void app_uart_init_config(void)
{
    uart_attr_t attr = {.baud_rate = UART_BAUDRATE,
                        .data_bits = UART_DATA_BITS,
                        .stop_bits = UART_STOP_BITS,
                        .parity = UART_PARITY_BIT};

    uart_pin_config_t pin_config = {.tx_pin = S_MGPIO0, .rx_pin = S_MGPIO1, .cts_pin = PIN_NONE, .rts_pin = PIN_NONE};
    uapi_uart_deinit(CONFIG_UART1_BUS_ID); // 重点，UART初始化之前需要去初始化，否则会报0x80001044
    int res = uapi_uart_init(CONFIG_UART1_BUS_ID, &pin_config, &attr, NULL, &g_app_uart_buffer_config);
    if (res != 0) {
        printf("uart init failed res = %02x\r\n", res);
    }
}

/**
 * @brief 处理UART接收到的数据
 */
static void process_uart_data(const uint8_t* data, int length)
{
    if (!is_valid_hex_data(data, length)) {
        printf("[UART] Invalid data received (all zeros)\r\n");
        return;
    }
    
    // 处理特定的hex命令
    switch (data[0]) {
        case 0xAA:
            printf("[UART] Command 0xAA received - Action A\r\n");
            break;
        case 0xBB:
            printf("[UART] Command 0xBB received - Action B\r\n");
            break;
        case 0xCC:
            printf("[UART] Command 0xCC received - Action C\r\n");
            break;
        case 0xDD:
            printf("[UART] Command 0xDD received - Action D\r\n");
            break;
        default:
            printf("[UART] Unknown command: 0x%02X\r\n", data[0]);
            break;
    }
}

/**
 * @brief 处理从SLE客户端接收到的数据
 * 这个函数会被sle_server.c中的ssaps_write_request_cbk调用
 * 根据接收到的字符发送对应的十六进制数据到UART
 */
void process_received_data(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0) {
        printf("[SLE->UART] Invalid data received\r\n");
        return;
    }
    
    printf("[SLE->UART] 📥 Received data from SLE client: %.*s\r\n", length, data);
    
    // 根据接收到的字符发送对应的十六进制数据
    uint8_t hex_data = 0;
    char received_char = data[0];
    
    switch (received_char) {
        case 'A':
            hex_data = 0x41;  // A -> 0x41
            printf("[SLE->UART] 📤 Sending 0x41 for character 'A'\r\n");
            break;
        case 'B':
            hex_data = 0x42;  // B -> 0x42
            printf("[SLE->UART] 📤 Sending 0x42 for character 'B'\r\n");
            break;
        case 'C':
            hex_data = 0x43;  // C -> 0x43
            printf("[SLE->UART] 📤 Sending 0x43 for character 'C'\r\n");
            break;
        case 'D':
            hex_data = 0x44;  // D -> 0x44
            printf("[SLE->UART] 📤 Sending 0x44 for character 'D'\r\n");
            break;
        default:
            printf("[SLE->UART] ❌ Unknown character: %c, no UART data sent\r\n", received_char);
            return;
    }
    
    // 发送十六进制数据到UART
    int sent_bytes = uapi_uart_write(CONFIG_UART1_BUS_ID, &hex_data, 1, 0);
    if (sent_bytes == 1) {
        printf("[SLE->UART] ✅ UART send successful (0x%02X, %d bytes)\r\n", hex_data, sent_bytes);
    } else {
        printf("[SLE->UART] ❌ UART send failed (sent: %d, expected: 1)\r\n", sent_bytes);
    }
    
}

/**
 * @brief UART数据接收任务
 * 只负责接收UART数据，不主动发送测试数据
 */
static void uart_communication_task(void)
{
    (void)uapi_watchdog_kick();
    
    // 清空接收缓冲区
    memset(g_app_uart_rx_buff, 0, UART_TRANSFER_SIZE);
    
    // 读取UART数据 - 设置超时时间，避免立即返回
    int read_len = uapi_uart_read(CONFIG_UART1_BUS_ID, g_app_uart_rx_buff, UART_TRANSFER_SIZE, 100); // 100ms超时
    if (read_len > 0) {
        printf("[UART] 📥 Received from UART: ");
        print_hex_data("", g_app_uart_rx_buff, read_len);
        process_uart_data(g_app_uart_rx_buff, read_len);
    } else if (read_len == 0) {
        // 超时，没有数据 - 可以添加调试信息
        static int timeout_count = 0;
        timeout_count++;
        if (timeout_count % 10 == 0) { // 每10次超时输出一次调试信息
            printf("[UART] ⏰ No data received (timeout count: %d)\r\n", timeout_count);
        }
    } else {
        printf("[UART] ❌ UART read error: %d\r\n", read_len);
    }
}

/**
 * @brief UART串口通信线程函数
 */
static void UartTask(char *arg)
{
    (void)arg;
    
    printf("[UART_TASK] 🚀 Starting UART communication thread...\r\n");
    
    // 初始化UART引脚
    app_uart_init_pin();
    
    // 初始化UART配置
    app_uart_init_config();
    
    printf("[UART_TASK] ✅ UART initialized, starting hex data communication...\r\n");
    
    // 发送测试数据验证串口发送功能
    printf("[UART_TASK] 🧪 Sending test data to verify UART transmission...\r\n");
    uint8_t test_data[] = {0x41, 0x42, 0x43, 0x44}; // A, B, C, D
    for (int i = 0; i < 4; i++) {
        int sent = uapi_uart_write(CONFIG_UART1_BUS_ID, &test_data[i], 1, 0);
        printf("[UART_TASK] 📤 Sent test data 0x%02X, result: %d\r\n", test_data[i], sent);
        osal_mdelay(500); // 间隔500ms发送
    }
    printf("[UART_TASK] ✅ Test data sent, starting normal communication...\r\n");
    
    // UART通信主循环
    while (1) {
        // 执行UART数据收发
        uart_communication_task();
        
        // 延时100ms，更频繁地检查数据
        osal_mdelay(100);
    }
}

// 主线程函数
static void SleTask(char *arg)
{
    (void)arg;
 
    // 设置SLE服务端广播设备名称
    sle_set_announce_name("sle_server");
 
    // 初始化SLE服务端
    sle_server_init();
    
    printf("[SLE_TASK] ✅ SLE server initialized successfully!\r\n");
 
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
    // 定义SLE线程属性
    osThreadAttr_t sle_attr;
    sle_attr.name = "SleTask";
    sle_attr.stack_size = 1024 * 10; // CHIP_WS63
    sle_attr.priority = osPriorityNormal;
    
    // 定义UART线程属性
    osThreadAttr_t uart_attr;
    uart_attr.name = "UartTask";
    uart_attr.stack_size = UART_TASK_STACK_SIZE;
    uart_attr.priority = osPriorityNormal;
 
    // 创建SLE服务端线程
    if (osThreadNew(SleTask, NULL, &sle_attr) == NULL)
        printf("[SleServerDemo] ❌ Failed to create SleTask!\n");
    else
        printf("[SleServerDemo] ✅ Create SleTask successfully!\n");
        
    // 创建UART串口通信线程
    if (osThreadNew(UartTask, NULL, &uart_attr) == NULL)
        printf("[SleServerDemo] ❌ Failed to create UartTask!\n");
    else
        printf("[SleServerDemo] ✅ Create UartTask successfully!\n");
        
    printf("[SleServerDemo] 🎉 Both SLE and UART tasks are running!\n");
    printf("[SleServerDemo] 📋 SLE->UART Mapping: A->0x41, B->0x42, C->0x43, D->0x44\n");
    
    // 测试映射功能
    printf("[SleServerDemo] 🧪 Testing SLE->UART mapping...\n");
    uint8_t test_data[] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < 4; i++) {
        printf("[TEST] Testing character '%c' -> ", test_data[i]);
        process_received_data(&test_data[i], 1);
        osal_mdelay(100); // 短暂延时
    }
}
 
 // 运行入口函数
 APP_FEATURE_INIT(SleServerDemo);
 
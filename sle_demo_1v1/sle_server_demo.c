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

// UART相关头文件
#include "pinctrl.h"
#include "uart.h"
#include "watchdog.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "app_init.h"

#include "sle_server.h"     // SLE服务端接口
#include "sle_server_adv.h" // SLE服务端广播接口

// UART配置宏定义
#define UART_BAUDRATE 9600
#define UART_DATA_BITS 3
#define UART_STOP_BITS 1
#define UART_PARITY_BIT 0
#define UART_TRANSFER_SIZE 1
#define CONFIG_UART1_TXD_PIN 15
#define CONFIG_UART1_RXD_PIN 16
#define CONFIG_UART1_PIN_MODE 1
#define CONFIG_UART1_BUS_ID 1
#define CONFIG_UART_INT_WAIT_MS 5

#define UART_TASK_STACK_SIZE 0x1000
#define UART_TASK_DURATION_MS 1000
#define UART_TASK_PRIO 17

// UART缓冲区
static uint8_t g_app_uart_tx_buff[UART_TRANSFER_SIZE] = "A";
static uint8_t g_app_uart_rx_buff[UART_TRANSFER_SIZE] = {0};

// 用于通过串口发送SLE接收的数据
#define SLE_UART_TX_BUF_SIZE 256
static char g_sle_uart_tx_buf[SLE_UART_TX_BUF_SIZE] = {0};

#define CONFIG_UART_POLL_TRANSFER_MODE 1
#if defined(CONFIG_UART_POLL_TRANSFER_MODE)
// 轮询读取uart值
#endif

#if defined(CONFIG_UART_INT_TRANSFER_MODE)
static uint8_t g_app_uart_int_rx_flag = 0;
#endif

static uart_buffer_config_t g_app_uart_buffer_config = {.rx_buffer = g_app_uart_rx_buff,
                                                        .rx_buffer_size = UART_TRANSFER_SIZE};

// SLE连接状态标志
static volatile bool g_sle_connected = false;

// UART发送函数 - 声明为全局函数供SLE服务端调用
int uart_send_data(const char *data, uint8_t length)
{
    if (data == NULL || length == 0) {
        return -1;
    }
    
    // 发送数据到UART
    int ret = uapi_uart_write(CONFIG_UART1_BUS_ID, (uint8_t *)data, length, 0);
    if (ret == length) {
        osal_printk("UART send data successfully: %s (len:%d)\r\n", data, length);
        return 0;
    } else {
        osal_printk("UART send data failed: %d\r\n", ret);
        return -1;
    }
}

// UART初始化函数
static void app_uart_init_pin(void)
{
    uapi_pin_set_mode(CONFIG_UART1_TXD_PIN, CONFIG_UART1_PIN_MODE);
    uapi_pin_set_mode(CONFIG_UART1_RXD_PIN, CONFIG_UART1_PIN_MODE);
}

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

#if defined(CONFIG_UART_INT_TRANSFER_MODE)
static void app_uart_read_int_handler(const void *buffer, uint16_t length, bool error)
{
    unused(error);
    if (buffer == NULL || length == 0) {
        osal_printk("uart%d int mode transfer illegal data!\r\n", CONFIG_UART1_BUS_ID);
        return;
    }

    uint8_t *buff = (uint8_t *)buffer;
    if (memcpy_s(g_app_uart_rx_buff, length, buff, length) != EOK) {
        osal_printk("uart%d int mode data copy fail!\r\n", CONFIG_UART1_BUS_ID);
        return;
    }
    osal_printk("uart write data = %s\r\n", buff);
    g_app_uart_int_rx_flag = 1;
}

static void app_uart_write_int_handler(const void *buffer, uint32_t length, const void *params)
{
    unused(params);
    uint8_t *buff = (void *)buffer;
    for (uint8_t i = 0; i < length; i++) {
        osal_printk("uart%d write data[%d] = %d\r\n", CONFIG_UART1_BUS_ID, i, buff[i]);
    }
}
#endif

// UART任务函数
void *uart_task(void *arg)
{
    (void)arg;
    /* UART pinmux. */
    app_uart_init_pin();
    /* UART init config. */
    app_uart_init_config();

#if defined(CONFIG_UART_INT_TRANSFER_MODE)
    osal_printk("uart%d int mode register receive callback start!\r\n", CONFIG_UART1_BUS_ID);
    if (uapi_uart_register_rx_callback(CONFIG_UART1_BUS_ID, UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE, 1,
                                       app_uart_read_int_handler) == ERRCODE_SUCC) {
        osal_printk("uart%d int mode register receive callback succ!\r\n", CONFIG_UART1_BUS_ID);
    }
#endif
    while (1) {
        osal_mdelay(UART_TASK_DURATION_MS);
#if defined(CONFIG_UART_POLL_TRANSFER_MODE)
        // 周期性将最新的SLE接收数据通过UART发送，500ms节奏
        uint8_t len = sle_get_received_data(g_sle_uart_tx_buf, sizeof(g_sle_uart_tx_buf));
        if (len > 0) {
            // 若末尾包含字符串终止符'\0'，则不发送该字节，仅发送有效数据
            if (g_sle_uart_tx_buf[len - 1] == '\0') {
                len--;
            }
            (void)uapi_watchdog_kick();
            int wret = uapi_uart_write(CONFIG_UART1_BUS_ID, (uint8_t *)g_sle_uart_tx_buf, len, 0);
            if (wret == len) {
                osal_printk("uart%d poll mode sent SLE data: %s (len:%d)\r\n", CONFIG_UART1_BUS_ID, g_sle_uart_tx_buf, len);
                // 发送成功后清空缓存，避免重复发送，等待下一次数据变化
                sle_clear_received_data();
            } else {
                osal_printk("uart%d poll mode send SLE data failed: %d (len:%d)\r\n", CONFIG_UART1_BUS_ID, wret, len);
            }
        } 
        
#endif
#if defined(CONFIG_UART_INT_TRANSFER_MODE)
        while (g_app_uart_int_rx_flag != 1) {
            osal_mdelay(CONFIG_UART_INT_WAIT_MS);
        }
        g_app_uart_int_rx_flag = 0;
        osal_printk("uart%d int mode send back!\r\n", CONFIG_UART1_BUS_ID);
        if (uapi_uart_write_int(CONFIG_UART1_BUS_ID, g_app_uart_tx_buff, UART_TRANSFER_SIZE, 0,
                                app_uart_write_int_handler) == ERRCODE_SUCC) {
            osal_printk("uart%d int mode send back succ!\r\n", CONFIG_UART1_BUS_ID);
        }
#endif
    }
    return NULL;
}

// 主线程函数
static void SleTask(void *arg)
{
    (void)arg;

    // 设置SLE服务端广播设备名称
    sle_set_announce_name("sle_server");

    // 初始化SLE服务端
    sle_server_init();

    // 等待客户端连接完成
    printf("[SleTask] Waiting for client connection...\n");
    sle_wait_client_connected();
    g_sle_connected = true;
    printf("[SleTask] Client connected! SLE service ready.\n");

    // 服务端向客户端发送数据。此处默认注释掉，如需使用请取消注释
#if 0
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
    uart_attr.priority = UART_TASK_PRIO;

    // 创建SLE线程
    if (osThreadNew(SleTask, NULL, &sle_attr) == NULL)
        printf("[SleServerDemo] Failed to create SleTask!\n");
    else
        printf("[SleServerDemo] Create SleTask successfully!\n");

    // 创建UART线程
    if (osThreadNew(uart_task, NULL, &uart_attr) == NULL)
        printf("[SleServerDemo] Failed to create UartTask!\n");
    else
        printf("[SleServerDemo] Create UartTask successfully!\n");
}

// 运行入口函数
APP_FEATURE_INIT(SleServerDemo);

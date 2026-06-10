/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pinctrl.h"
#include "uart.h"
#include "watchdog.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "app_init.h"

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

#define UART_TASK_STACK_SIZE 0x1000
#define UART_TASK_DURATION_MS 1000
#define UART_TASK_PRIO 17

// 发送hex数据而不是字符串
static uint8_t g_app_uart_tx_buff[UART_TRANSFER_SIZE] = {0xAA, 0xBB, 0xCC, 0xDD, 0x01, 0x02, 0x03, 0x04};
static uint8_t g_app_uart_rx_buff[UART_TRANSFER_SIZE] = {0};

#define CONFIG_UART_POLL_TRANSFER_MODE 1
#if defined(CONFIG_UART_POLL_TRANSFER_MODE)
// 轮询读取uart值
#endif

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
    
    // 打印接收到的hex数据
    printf("[UART_INT_RX] ");
    print_hex_data("Received", buff, length);
    
    // 检查是否为有效数据
    if (is_valid_hex_data(buff, length)) {
        // 处理特定的hex命令
        switch (buff[0]) {
            case 0xAA:
                printf("[UART_INT_RX] Command 0xAA received - Action A\r\n");
                break;
            case 0xBB:
                printf("[UART_INT_RX] Command 0xBB received - Action B\r\n");
                break;
            case 0xCC:
                printf("[UART_INT_RX] Command 0xCC received - Action C\r\n");
                break;
            case 0xDD:
                printf("[UART_INT_RX] Command 0xDD received - Action D\r\n");
                break;
            default:
                printf("[UART_INT_RX] Unknown command: 0x%02X\r\n", buff[0]);
                break;
        }
    } else {
        printf("[UART_INT_RX] Invalid data received (all zeros)\r\n");
    }
    
    g_app_uart_int_rx_flag = 1;
}

static void app_uart_write_int_handler(const void *buffer, uint32_t length, const void *params)
{
    unused(params);
    uint8_t *buff = (void *)buffer;
    printf("[UART_INT_TX] ");
    print_hex_data("Sent", buff, length);
}
#endif

void *uart_task(const char *arg)
{
    unused(arg);
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
    printf("[UART_TASK] ✅ UART initialized, starting hex data communication...\r\n");
    
    while (1) {
        osal_mdelay(UART_TASK_DURATION_MS);
#if defined(CONFIG_UART_POLL_TRANSFER_MODE)
        printf("[UART_POLL] 📤 Sending hex data...\r\n");
        print_hex_data("[UART_POLL_TX]", g_app_uart_tx_buff, UART_TRANSFER_SIZE);
        
        (void)uapi_watchdog_kick();
        
        // 清空接收缓冲区
        memset(g_app_uart_rx_buff, 0, UART_TRANSFER_SIZE);
        
        // 发送hex数据
        int sent_bytes = uapi_uart_write(CONFIG_UART1_BUS_ID, g_app_uart_tx_buff, UART_TRANSFER_SIZE, 0);
        if (sent_bytes == UART_TRANSFER_SIZE) {
            printf("[UART_POLL] ✅ Send successful (%d bytes)\r\n", sent_bytes);
        } else {
            printf("[UART_POLL] ❌ Send failed (sent: %d, expected: %d)\r\n", sent_bytes, UART_TRANSFER_SIZE);
        }
        
        // 稍微延迟等待数据传输
        osal_mdelay(50);
        
        // 读取hex数据
        int read_len = uapi_uart_read(CONFIG_UART1_BUS_ID, g_app_uart_rx_buff, UART_TRANSFER_SIZE, 0);
        if (read_len > 0) {
            printf("[UART_POLL] 📥 ");
            print_hex_data("Received", g_app_uart_rx_buff, read_len);
            
            // 检查是否为有效数据
            if (is_valid_hex_data(g_app_uart_rx_buff, read_len)) {
                // 处理特定的hex命令
                switch (g_app_uart_rx_buff[0]) {
                    case 0xAA:
                        printf("[UART_POLL] Command 0xAA received - Action A\r\n");
                        break;
                    case 0xBB:
                        printf("[UART_POLL] Command 0xBB received - Action B\r\n");
                        break;
                    case 0xCC:
                        printf("[UART_POLL] Command 0xCC received - Action C\r\n");
                        break;
                    case 0xDD:
                        printf("[UART_POLL] Command 0xDD received - Action D\r\n");
                        break;
                    default:
                        printf("[UART_POLL] Unknown command: 0x%02X\r\n", g_app_uart_rx_buff[0]);
                        break;
                }
                
                // 比较发送和接收的数据
                bool data_match = true;
                for (int i = 0; i < read_len && i < UART_TRANSFER_SIZE; i++) {
                    if (g_app_uart_tx_buff[i] != g_app_uart_rx_buff[i]) {
                        data_match = false;
                        break;
                    }
                }
                
                if (data_match && read_len == UART_TRANSFER_SIZE) {
                    printf("[UART_POLL] 🎉 LOOPBACK TEST PASSED! Hex data matches!\r\n");
                } else {
                    printf("[UART_POLL] ⚠️  Data mismatch or incomplete (sent: %d, received: %d)\r\n", 
                           UART_TRANSFER_SIZE, read_len);
                }
            } else {
                printf("[UART_POLL] Invalid data received (all zeros)\r\n");
            }
        } else {
            printf("[UART_POLL] ⚠️  No data received (read_len=%d)\r\n", read_len);
            printf("[UART_POLL] 🔧 Check if GPIO-%d and GPIO-%d are connected\r\n", 
                   CONFIG_UART1_TXD_PIN, CONFIG_UART1_RXD_PIN);
        }
#endif
#if defined(CONFIG_UART_INT_TRANSFER_MODE)
        while (g_app_uart_int_rx_flag != 1) {
            osal_mdelay(CONFIG_UART_INT_WAIT_MS);
        }
        g_app_uart_int_rx_flag = 0;
        printf("[UART_INT] 📤 Sending hex data response...\r\n");
        if (uapi_uart_write_int(CONFIG_UART1_BUS_ID, g_app_uart_tx_buff, UART_TRANSFER_SIZE, 0,
                                app_uart_write_int_handler) == ERRCODE_SUCC) {
            printf("[UART_INT] ✅ Interrupt mode send successful\r\n");
        } else {
            printf("[UART_INT] ❌ Interrupt mode send failed\r\n");
        }
#endif
    }
    return NULL;
}

static void uart_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)uart_task, 0, "UartTask", UART_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, UART_TASK_PRIO);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
}

/* Run the uart_entry. */
app_run(uart_entry);
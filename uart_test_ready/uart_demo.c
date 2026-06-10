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
 
 static uint8_t g_app_uart_tx_buff[UART_TRANSFER_SIZE] = "hello uart1";
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
 
static void app_uart_init_pin(void)
{
    printf("[UART] Setting pin modes: TX=GPIO-%d, RX=GPIO-%d, Mode=%d\r\n", 
           CONFIG_UART1_TXD_PIN, CONFIG_UART1_RXD_PIN, CONFIG_UART1_PIN_MODE);
    
    int ret1 = uapi_pin_set_mode(CONFIG_UART1_TXD_PIN, CONFIG_UART1_PIN_MODE);
    int ret2 = uapi_pin_set_mode(CONFIG_UART1_RXD_PIN, CONFIG_UART1_PIN_MODE);
    
    if (ret1 != 0) {
        printf("[UART] ❌ Failed to set TX pin mode: %d\r\n", ret1);
    } else {
        printf("[UART] ✅ TX pin (GPIO-%d) configured\r\n", CONFIG_UART1_TXD_PIN);
    }
    
    if (ret2 != 0) {
        printf("[UART] ❌ Failed to set RX pin mode: %d\r\n", ret2);
    } else {
        printf("[UART] ✅ RX pin (GPIO-%d) configured\r\n", CONFIG_UART1_RXD_PIN);
    }
}
 
static void app_uart_init_config(void)
{
    printf("[UART] Initializing UART%d configuration...\r\n", CONFIG_UART1_BUS_ID);
    printf("[UART] Parameters: Baudrate=%d, DataBits=%d, StopBits=%d, Parity=%d\r\n", 
           UART_BAUDRATE, UART_DATA_BITS, UART_STOP_BITS, UART_PARITY_BIT);
    
    uart_attr_t attr = {.baud_rate = UART_BAUDRATE,
                        .data_bits = UART_DATA_BITS,
                        .stop_bits = UART_STOP_BITS,
                        .parity = UART_PARITY_BIT};

    uart_pin_config_t pin_config = {.tx_pin = S_MGPIO15, .rx_pin = S_MGPIO16, .cts_pin = PIN_NONE, .rts_pin = PIN_NONE};
    
    printf("[UART] Pin config: TX=S_MGPIO15, RX=S_MGPIO16\r\n");
    printf("[UART] De-initializing UART%d first...\r\n", CONFIG_UART1_BUS_ID);
    uapi_uart_deinit(CONFIG_UART1_BUS_ID); // 重点，UART初始化之前需要去初始化，否则会报0x80001044
    
    printf("[UART] Initializing UART%d...\r\n", CONFIG_UART1_BUS_ID);
    int res = uapi_uart_init(CONFIG_UART1_BUS_ID, &pin_config, &attr, NULL, &g_app_uart_buffer_config);
    if (res != 0) {
        printf("[UART] ❌ UART init failed! Error code: 0x%08X\r\n", res);
        printf("[UART] Common error codes:\r\n");
        printf("[UART] - 0x80000001: Invalid parameter\r\n");
        printf("[UART] - 0x80001044: UART already initialized\r\n");
        printf("[UART] - 0x80001002: Memory allocation failed\r\n");
        printf("[UART] Please check:\r\n");
        printf("[UART] 1. Pin configuration (GPIO-15/16 with mode 1)\r\n");
        printf("[UART] 2. UART parameters (8N1@115200)\r\n");
        printf("[UART] 3. Hardware connections\r\n");
        printf("[UART] 4. Pin multiplexing conflicts\r\n");
    } else {
        printf("[UART] ✅ UART init success!\r\n");
        printf("[UART] Configuration: %dN%d@%d bps\r\n", 
               UART_DATA_BITS, UART_STOP_BITS, UART_BAUDRATE);
        printf("[UART] Buffer size: %d bytes\r\n", UART_TRANSFER_SIZE);
        printf("[UART] 🔗 Ready for loopback test - connect GPIO-15 to GPIO-16\r\n");
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
 
void *uart_task(const char *arg)
{
    unused(arg);
    
    printf("[UART_TASK] 🚀 Starting UART loopback test...\r\n");
    printf("[UART_TASK] 📋 Test configuration:\r\n");
    printf("[UART_TASK] - TX Pin: GPIO-%d\r\n", CONFIG_UART1_TXD_PIN);
    printf("[UART_TASK] - RX Pin: GPIO-%d\r\n", CONFIG_UART1_RXD_PIN);
    printf("[UART_TASK] - Baudrate: %d bps\r\n", UART_BAUDRATE);
    printf("[UART_TASK] - Test message: \"%s\"\r\n", g_app_uart_tx_buff);
    printf("[UART_TASK] 🔗 Please connect GPIO-%d to GPIO-%d for loopback test\r\n", 
           CONFIG_UART1_TXD_PIN, CONFIG_UART1_RXD_PIN);
    
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
    printf("[UART_TASK] ✅ UART initialization completed, starting loopback test loop...\r\n");
    
    while (1) {
        osal_mdelay(UART_TASK_DURATION_MS);
#if defined(CONFIG_UART_POLL_TRANSFER_MODE)
        printf("[UART_LOOPBACK] 📤 Sending: \"%s\" (%d bytes)\r\n", g_app_uart_tx_buff, UART_TRANSFER_SIZE);
        (void)uapi_watchdog_kick();
        
        // 清空接收缓冲区
        memset(g_app_uart_rx_buff, 0, UART_TRANSFER_SIZE);
        
        if (uapi_uart_write(CONFIG_UART1_BUS_ID, g_app_uart_tx_buff, UART_TRANSFER_SIZE, 0) == UART_TRANSFER_SIZE) {
            printf("[UART_LOOPBACK] ✅ Send successful\r\n");
        } else {
            printf("[UART_LOOPBACK] ❌ Send failed\r\n");
        }
        
        // 增加延迟等待数据传输完成
        osal_mdelay(50);
        
        // 多次尝试读取数据
        int read_len = 0;
        int retry_count = 0;
        const int max_retries = 5;
        
        while (retry_count < max_retries && read_len <= 0) {
            read_len = uapi_uart_read(CONFIG_UART1_BUS_ID, g_app_uart_rx_buff, UART_TRANSFER_SIZE, 0);
            if (read_len <= 0) {
                printf("[UART_LOOPBACK] 🔄 Retry %d/%d, waiting for data...\r\n", retry_count + 1, max_retries);
                osal_mdelay(20);
                retry_count++;
            }
        }
        
        if (read_len > 0) {
            printf("[UART_LOOPBACK] 📥 Received: \"%s\" (%d bytes)\r\n", g_app_uart_rx_buff, read_len);
            
            // 比较发送和接收的数据
            if (strncmp((char*)g_app_uart_tx_buff, (char*)g_app_uart_rx_buff, strlen((char*)g_app_uart_tx_buff)) == 0) {
                printf("[UART_LOOPBACK] 🎉 LOOPBACK TEST PASSED! Data matches!\r\n");
            } else {
                printf("[UART_LOOPBACK] ❌ LOOPBACK TEST FAILED! Data mismatch!\r\n");
                printf("[UART_LOOPBACK] Expected: \"%s\"\r\n", g_app_uart_tx_buff);
                printf("[UART_LOOPBACK] Received: \"%s\"\r\n", g_app_uart_rx_buff);
            }
        } else {
            printf("[UART_LOOPBACK] ⚠️  No data received after %d retries (read_len=%d)\r\n", max_retries, read_len);
            printf("[UART_LOOPBACK] 🔧 Troubleshooting checklist:\r\n");
            printf("[UART_LOOPBACK] 1. Hardware: Connect GPIO-%d (TX) to GPIO-%d (RX)\r\n", 
                   CONFIG_UART1_TXD_PIN, CONFIG_UART1_RXD_PIN);
            printf("[UART_LOOPBACK] 2. Check if pins are used by other peripherals (I2C, SPI, etc.)\r\n");
            printf("[UART_LOOPBACK] 3. Verify pin multiplexing configuration\r\n");
            printf("[UART_LOOPBACK] 4. Check UART buffer and timing settings\r\n");
            
            // 尝试读取任何可能的数据（即使是部分数据）
            printf("[UART_LOOPBACK] 🔍 Checking for partial data...\r\n");
            for (int i = 0; i < UART_TRANSFER_SIZE; i++) {
                if (g_app_uart_rx_buff[i] != 0) {
                    printf("[UART_LOOPBACK] Found non-zero byte at position %d: 0x%02X\r\n", i, g_app_uart_rx_buff[i]);
                }
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
 /*
 Copyright (C) 2024 HiHope Open Source Organization .
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */
#include <stdio.h>     
#include <unistd.h>   
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "ohos_init.h"  
#include "cmsis_os2.h"  
#include "iot_gpio.h"   
#include "iot_gpio_ex.h"
#include "oled_ssd1306.h"
#include "sle_client.h" 
//#include "../uart_test/uart_demo.h"
// UART相关头文件
//#include "pinctrl.h"
//#include "uart.h"
//#include "watchdog.h"
//#include "osal_debug.h"
//#include "soc_osal.h"
//#include "app_init.h"

// UART相关宏定义
#define UART_BAUDRATE 115200
#define UART_DATA_BITS 3
#define UART_STOP_BITS 1
#define UART_PARITY_BIT 0
#define UART_TRANSFER_SIZE 4
#define CONFIG_UART1_TXD_PIN 15
#define CONFIG_UART1_RXD_PIN 16
#define CONFIG_UART1_PIN_MODE 1
#define CONFIG_UART1_BUS_ID 1
#define CONFIG_UART_INT_WAIT_MS 5
#define UART_TASK_DURATION_MS 1000
#define CONFIG_UART_POLL_TRANSFER_MODE 0
#define CONFIG_UART_INT_TRANSFER_MODE 1

// UART相关变量声明
// extern uint8_t g_app_uart_tx_buff[UART_TRANSFER_SIZE]; // 不再使用ASCII数据
extern uint8_t g_app_uart_rx_buff[UART_TRANSFER_SIZE];
extern uint8_t g_test_hex_data[];
extern uint8_t g_app_uart_int_rx_flag;

// UART相关函数声明
extern void app_uart_init_pin(void);
extern void app_uart_init_config(void);
extern void app_uart_read_int_handler(const void *buffer, uint16_t length, bool error);
extern void app_uart_write_int_handler(const void *buffer, uint32_t length, const void *params);
extern void print_received_data(const char* mode_name, const uint8_t* data, int32_t length);

// 时间结构体
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
} time_info_t;

// 全局时间变量
static time_info_t g_current_time = {2025, 9, 16, 19, 53, 16};
static char g_time_str[16];
static char g_date_str[16];
static osTimerId_t g_timer_id = NULL;
static int pill_name = 2;//0-999感冒灵，1-左氧氟沙星滴眼液，2-双黄连口服液
static int  rx_buffer[]={0xCC};
/**
 * @brief 格式化时间字符串
 */
static void format_time_string(void)
{
    // 格式化时间字符串 HH:MM:SS
    snprintf(g_time_str, sizeof(g_time_str), "%02d:%02d:%02d", 
             g_current_time.hour, g_current_time.minute, g_current_time.second);
    
    // 格式化日期字符串 YYYY.M.D
    snprintf(g_date_str, sizeof(g_date_str), "%04d.%d.%02d", 
             g_current_time.year, g_current_time.month, g_current_time.day);
}


void TempHumChinese(void)
{
    const uint32_t W = 16;
    const uint32_t H = 16;  // 添加高度定义
    uint8_t fonts[][32] = {
        /* [字库]：[HZK1616宋体] [数据排列]:从左到右从上到下 [取模方式]:横向8点左高位 [正负反色]:否 [去掉重复后]共2个字符
        [总字符库]："药品"*/
        {/*-- ID:0,字符:"药",ASCII编码:D2A9,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x04,0x40,0x04,0x44,0xFF,0xFE,0x04,0x40,0x08,0x40,0x10,0x44,0x22,0x7E,0x7C,0x84,
        0x09,0x04,0x10,0x44,0x7E,0x24,0x00,0x24,0x0E,0x04,0x70,0x04,0x20,0x28,0x00,0x10},
        {/*-- ID:1,字符:"品",ASCII编码:C6B7,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x10,0x1F,0xF8,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10,0x02,0x04,
        0x7F,0xFE,0x42,0x84,0x42,0x84,0x42,0x84,0x42,0x84,0x42,0x84,0x7E,0xFC,0x42,0x84}
        };
    uint8_t fonts1[][32] = {
        /* [字库]：[HZK1616宋体] [数据排列]:从左到右从上到下 [取模方式]:横向8点左高位 [正负反色]:否 [去掉重复后]共3个字符
        [总字符库]："感冒灵"*/
        {/*-- ID:0,字符:"感",ASCII编码:B8D0,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x50,0x00,0x48,0x7F,0xFC,0x40,0x40,0x7F,0xC0,0x40,0x48,0x5F,0x48,0x51,0x50,
        0x51,0x22,0x5F,0x52,0x90,0x8E,0x02,0x00,0x29,0x90,0x28,0xAC,0x48,0x24,0x07,0xE0},
        {/*-- ID:1,字符:"冒",ASCII编码:C3B0,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x04,0x7F,0xFE,0x40,0x04,0x5F,0xF4,0x40,0x04,0x5F,0xF4,0x40,0x04,0x1F,0xF0,
        0x10,0x10,0x1F,0xF0,0x10,0x10,0x1F,0xF0,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10},
        {/*-- ID:2,字符:"灵",ASCII编码:C1E9,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x08,0x7F,0xFC,0x00,0x08,0x3F,0xF8,0x00,0x08,0x7F,0xF8,0x01,0x08,0x01,0x00,
        0x21,0x08,0x19,0x18,0x0A,0xA0,0x02,0x80,0x04,0x40,0x08,0x30,0x30,0x0E,0xC0,0x04}
        };
    uint8_t fonts2[][32] = {
        /* [字库]：[HZK1616宋体] [数据排列]:从左到右从上到下 [取模方式]:横向8点左高位 [正负反色]:否 [去掉重复后]共8个字符
    [总字符库]："左氧氟沙星滴眼液"*/
        {/*-- ID:0,字符:"左",ASCII编码:D7F3,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x02,0x00,0x02,0x00,0x02,0x08,0xFF,0xFC,0x04,0x00,0x04,0x00,0x04,0x00,0x08,0x10,
        0x0F,0xF8,0x10,0x80,0x10,0x80,0x20,0x80,0x40,0x80,0x80,0x84,0x7F,0xFE,0x00,0x00},
        {/*-- ID:1,字符:"氧",ASCII编码:D1F5,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x10,0x00,0x1F,0xFC,0x20,0x00,0x2F,0xF8,0x40,0x00,0xBF,0xF8,0x08,0x88,0x05,0x08,
        0x3F,0xE8,0x02,0x08,0x1F,0xC8,0x02,0x08,0x7F,0xFA,0x02,0x0A,0x02,0x04,0x02,0x00},
        {/*-- ID:2,字符:"氟",ASCII编码:B7FA,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x10,0x08,0x1F,0xFC,0x10,0x00,0x2F,0xF8,0x40,0x00,0xBF,0xF8,0x0A,0x08,0x7F,0xC8,
        0x0A,0x48,0x7F,0xC8,0x4A,0x08,0x7F,0xE8,0x0A,0x28,0x0A,0xAA,0x12,0x4A,0x62,0x04},
        {/*-- ID:3,字符:"沙",ASCII编码:C9B3,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x40,0x20,0x40,0x18,0x40,0x08,0x40,0x81,0x50,0x61,0x48,0x22,0x46,0x0A,0x42,
        0x14,0x48,0x20,0x48,0xE0,0x50,0x20,0x20,0x20,0x40,0x20,0x80,0x23,0x00,0x2C,0x00},
        /*-- ID:4,字符:"星",ASCII编码:D0C7,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        {0x00,0x08,0x3F,0xFC,0x20,0x08,0x3F,0xF8,0x20,0x08,0x3F,0xF8,0x01,0x00,0x21,0x08,
        0x3F,0xFC,0x21,0x00,0x41,0x10,0xBF,0xF8,0x01,0x00,0x01,0x04,0xFF,0xFE,0x00,0x00},
        {/*-- ID:5,字符:"滴",ASCII编码:B5CE,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x80,0x40,0x44,0x2F,0xFE,0x22,0x08,0x01,0x14,0x87,0xFE,0x44,0x44,0x4D,0xF4,
        0x14,0x44,0x25,0xF4,0xE5,0x14,0x25,0x14,0x25,0xF4,0x25,0x04,0x24,0x14,0x24,0x08},
        {/*-- ID:6,字符:"眼",ASCII编码:D1DB,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x08,0x0B,0xFC,0x7E,0x08,0x4A,0x08,0x4B,0xF8,0x4A,0x08,0x7A,0x08,0x4B,0xF8,
        0x4A,0x84,0x7A,0x88,0x4A,0x50,0x4A,0x20,0x4A,0x10,0x7A,0x8E,0x4B,0x04,0x02,0x00},
        {/*-- ID:7,字符:"液",ASCII编码:D2BA,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x80,0x40,0x44,0x37,0xFE,0x10,0x20,0x81,0x20,0x61,0x3C,0x22,0x44,0x0A,0x64,
        0x16,0x98,0x2B,0x48,0xE2,0x50,0x22,0x20,0x22,0x50,0x22,0x8E,0x23,0x04,0x22,0x00}
        };
    uint8_t fonts3[][32] = {
        /* [字库]：[HZK1616宋体] [数据排列]:从左到右从上到下 [取模方式]:横向8点左高位 [正负反色]:否 [去掉重复后]共6个字符
        [总字符库]："双黄连口服液"*/
        {/*-- ID:0,字符:"双",ASCII编码:CBAB,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x00,0x00,0x04,0xFD,0xFE,0x04,0x84,0x44,0x84,0x44,0x84,0x28,0x88,0x28,0x48,
        0x10,0x48,0x10,0x50,0x28,0x20,0x28,0x30,0x44,0x50,0x44,0x88,0x81,0x0E,0x06,0x04},
        {/*-- ID:1,字符:"黄",ASCII编码:BBC6,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x04,0x40,0x04,0x50,0x3F,0xF8,0x04,0x40,0x04,0x44,0xFF,0xFE,0x01,0x10,0x1F,0xF8,
        0x11,0x10,0x1F,0xF0,0x11,0x10,0x1F,0xF0,0x10,0x00,0x04,0x60,0x18,0x18,0x60,0x04},
        {/*-- ID:2,字符:"连",ASCII编码:C1AC,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x80,0x40,0x88,0x2F,0xFC,0x21,0x00,0x01,0x40,0x02,0x50,0xE7,0xF8,0x20,0x40,
        0x20,0x40,0x20,0x48,0x2F,0xFC,0x20,0x40,0x20,0x40,0x50,0x46,0x8F,0xFC,0x00,0x00},
        {/*-- ID:3,字符:"口",ASCII编码:BFDA,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x00,0x00,0x08,0x3F,0xFC,0x20,0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x20,0x08,
        0x20,0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x3F,0xF8,0x20,0x08,0x00,0x00,0x00,0x00},
        {/*-- ID:4,字符:"服",ASCII编码:B7FE,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x02,0x08,0x3F,0xFC,0x22,0x88,0x22,0x88,0x22,0x88,0x3E,0x98,0x22,0x80,0x22,0xFC,
        0x22,0xA4,0x3E,0xA4,0x22,0xA8,0x22,0x90,0x22,0xA8,0x22,0xA8,0x4A,0xC6,0x84,0x84},
        {/*-- ID:5,字符:"液",ASCII编码:D2BA,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x80,0x40,0x44,0x37,0xFE,0x10,0x20,0x81,0x20,0x61,0x3C,0x22,0x44,0x0A,0x64,
        0x16,0x98,0x2B,0x48,0xE2,0x50,0x22,0x20,0x22,0x50,0x22,0x8E,0x23,0x04,0x22,0x00},
        };
    uint8_t fonts4[][32] = {
        /* [字库]：[HZK1616宋体] [数据排列]:从左到右从上到下 [取模方式]:横向8点左高位 [正负反色]:否 [去掉重复后]共6个字符
        [总字符库]："浦地蓝消炎片"*/
        {/*-- ID:0,字符:"浦",ASCII编码:C6D6,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x40,0x40,0x50,0x30,0x48,0x1F,0xFE,0x80,0x40,0x67,0xFC,0x24,0x44,0x04,0x44,
        0x17,0xFC,0x24,0x44,0xE4,0x44,0x27,0xFC,0x24,0x44,0x24,0x44,0x24,0x54,0x24,0x48},
        {/*-- ID:1,字符:"地",ASCII编码:B5D8,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x40,0x20,0x40,0x22,0x40,0x22,0x40,0x22,0x48,0x22,0x7C,0xFB,0xC8,0x26,0x48,
        0x22,0x48,0x22,0x48,0x22,0x68,0x22,0x50,0x3A,0x42,0xE2,0x02,0x41,0xFE,0x00,0x00},
        {/*-- ID:2,字符:"蓝",ASCII编码:C0B6,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x08,0x20,0x08,0x24,0xFF,0xFE,0x08,0x20,0x04,0x90,0x24,0xF8,0x25,0x00,0x26,0x40,
        0x24,0x20,0x04,0x08,0x3F,0xFC,0x24,0x48,0x24,0x48,0x24,0x48,0xFF,0xFE,0x00,0x00},
        {/*-- ID:3,字符:"消",ASCII编码:CFFB,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x40,0x42,0x48,0x31,0x50,0x10,0x48,0x83,0xFC,0x62,0x08,0x22,0x08,0x0B,0xF8,
        0x12,0x08,0x22,0x08,0xE3,0xF8,0x22,0x08,0x22,0x08,0x22,0x08,0x22,0x28,0x22,0x10},
        {/*-- ID:4,字符:"炎",ASCII编码:D1D7,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x01,0x00,0x01,0x08,0x11,0x18,0x11,0x20,0x22,0xC0,0x04,0x30,0x19,0x0C,0x61,0x04,
        0x01,0x10,0x11,0x30,0x11,0x40,0x22,0x80,0x02,0x40,0x04,0x30,0x18,0x0E,0x60,0x04},
        {/*-- ID:5,字符:"片",ASCII编码:C6AC,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
        0x00,0x80,0x20,0x80,0x20,0x80,0x20,0x80,0x20,0x84,0x3F,0xFE,0x20,0x00,0x20,0x00,
        0x3F,0xC0,0x20,0x40,0x20,0x40,0x20,0x40,0x20,0x40,0x20,0x40,0x40,0x40,0x80,0x40},
        };
       
    switch (pill_name)
    {
        case 0:
         //显示药品
        OledDrawChinese16x16(0, 48, fonts[0], OLED_COLOR_WHITE); // "药"
        OledDrawChinese16x16(16, 48, fonts[1], OLED_COLOR_WHITE); // "品"
                // 绘制"999感冒灵"字符
        OledShowString(32, 6, ":999", FONT8x16); 
        OledDrawChinese16x16(64, 48, fonts1[0], OLED_COLOR_WHITE);  // "感"
        OledDrawChinese16x16(80, 48, fonts1[1], OLED_COLOR_WHITE); // "冒"
        OledDrawChinese16x16(96, 48, fonts1[2], OLED_COLOR_WHITE); // "灵"
            break;
        case 1:// 绘制"左氧沙星滴眼液"字符
        OledDrawChinese16x16(0, 48, fonts2[0], OLED_COLOR_WHITE); // "左"
        OledDrawChinese16x16(16, 48, fonts2[1], OLED_COLOR_WHITE); // "氧"
        
        OledDrawChinese16x16(32, 48, fonts2[2], OLED_COLOR_WHITE);  // "氟"
        OledDrawChinese16x16(48, 48, fonts2[3], OLED_COLOR_WHITE); // "沙"
        OledDrawChinese16x16(64, 48, fonts2[4], OLED_COLOR_WHITE); // "星"
        OledDrawChinese16x16(80, 48, fonts2[5], OLED_COLOR_WHITE); // "滴"
        OledDrawChinese16x16(96, 48, fonts2[6], OLED_COLOR_WHITE); // "眼"
        OledDrawChinese16x16(112, 48, fonts2[7], OLED_COLOR_WHITE); // "液"
            break;
        case 2:// 绘制"双黄连口服液"字符 
        OledDrawChinese16x16(16, 48, fonts3[0], OLED_COLOR_WHITE); // "双"
        OledDrawChinese16x16(32, 48, fonts3[1], OLED_COLOR_WHITE); // "黄"
        OledDrawChinese16x16(48, 48, fonts3[2], OLED_COLOR_WHITE); // "连"
        OledDrawChinese16x16(64, 48, fonts3[3], OLED_COLOR_WHITE); // "口"
        OledDrawChinese16x16(80, 48, fonts3[4], OLED_COLOR_WHITE); // "服"
        OledDrawChinese16x16(96, 48, fonts3[5], OLED_COLOR_WHITE); // "液"
            break;
        case 3:// 绘制"浦蓝地消炎片"字符 
        OledDrawChinese16x16(16, 48, fonts4[0], OLED_COLOR_WHITE); // "双"
        OledDrawChinese16x16(32, 48, fonts4[1], OLED_COLOR_WHITE); // "黄"
        OledDrawChinese16x16(48, 48, fonts4[2], OLED_COLOR_WHITE); // "连"
        OledDrawChinese16x16(64, 48, fonts4[3], OLED_COLOR_WHITE); // "口"
        OledDrawChinese16x16(80, 48, fonts4[4], OLED_COLOR_WHITE); // "服"
        OledDrawChinese16x16(96, 48, fonts4[5], OLED_COLOR_WHITE); // "液"
            break;   
    }


}

/**
 * @brief 初始化OLED显示（只调用一次）
 */
static void init_oled_display(void)
{
    // 格式化时间字符串
    format_time_string();
    
    // 清屏
    OledFillScreen(0x00);
    
   
    // 显示静态内容
    OledShowString(5, 0, "Smart pillBox", FONT8x16);   
    OledShowString(25, 2, g_date_str, FONT8x16);  
    OledShowString(30, 4, g_time_str, FONT8x16); 
    
    TempHumChinese();
    //OledShowString(0, 6, "name:", FONT8x16);
}

/**
 * @brief 只更新时间显示部分
 */
static void update_time_display_only(void)
{
    // 格式化时间字符串
    format_time_string();
    
    // 只更新时间显示区域（第4行，位置30）
    // 先清除时间区域 - 用空格覆盖
    OledShowString(30, 4, "        ", FONT8x16);  // 8个空格清除8个字符宽度
    
    // 显示新的时间
    OledShowString(30, 4, g_time_str, FONT8x16);
}

/**
 * @brief 定时器回调函数 - 每秒更新一次时间
 */
void timer_callback(void *arg)
{
    (void)arg;
    
    // 更新秒数
    g_current_time.second++;
    
    // 处理时间进位
    if (g_current_time.second >= 60) {
        g_current_time.second = 0;
        g_current_time.minute++;
        
        if (g_current_time.minute >= 60) {
            g_current_time.minute = 0;
            g_current_time.hour++;
            
            if (g_current_time.hour >= 24) {
                g_current_time.hour = 0;
                g_current_time.day++;
                
                // 简单的月份处理（假设每月30天）
                if (g_current_time.day > 30) {
                    g_current_time.day = 1;
                    g_current_time.month++;
                    
                    if (g_current_time.month > 12) {
                        g_current_time.month = 1;
                        g_current_time.year++;
                    }
                }
            }
        }
    }
    
    // 只更新时间显示
    update_time_display_only();
    
    printf("[Timer] Time updated: %s %s\r\n", g_date_str, g_time_str);
}

static void OledTask(void *arg)
{
    (void)arg;

    printf("[OLED] Starting OLED task...\r\n");

    // 初始化SSD1306
    uint32_t ret = OledInit();
    if (ret != 0) {
        printf("[OLED] OLED init failed: %d\r\n", ret);
        return;
    }
    
    printf("[OLED] OLED initialized successfully\r\n");

    // 显示初始内容
    init_oled_display();
    
    printf("[OLED] Initial display completed\r\n");
    
    // 创建定时器 - 每秒更新一次
    g_timer_id = osTimerNew(timer_callback, osTimerPeriodic, NULL, NULL);
    if (g_timer_id == NULL) {
        printf("[OLED] Failed to create timer!\r\n");
        return;
    }
    
    printf("[OLED] Timer created successfully\r\n");
    
    // 启动定时器 - 每秒触发一次
    osStatus_t status = osTimerStart(g_timer_id, 100);
    if (status != osOK) {
        printf("[OLED] Failed to start timer: %d\r\n", status);
        osTimerDelete(g_timer_id);
        return;
    }
    
    printf("[OLED] Timer started successfully\r\n");
    printf("[OLED] Time will update every second\r\n");
    
    // 保持任务运行
    while (1) {
        osDelay(100); // 每秒检查一次
    }
}

// SLE任务函数 - 暂时注释掉，先编译服务端

static void SleTaskInOled(void *arg)
{
    (void)arg;
    
    printf("[SLE] Starting SLE task in OLED demo...\r\n");
    
    // 设置SLE服务端名称
    sle_set_server_name("sle_server");
    printf("!!![SLE] Server name set to: sle_server\r\n");
    // 初始化SLE客户端
    sle_client_init();
    printf("!!![SLE] Client initialized\r\n");
    // 等待服务发现
    sle_wait_service_found();
    printf("!!![SLE] Service found\r\n");

    printf("[SLE] Service found, starting data transmission...\r\n");
    
    // 数据发送循环
    while (1) {
        uint8_t data[] = {65,66,67,68,69,70,71,72,73,74};  // A-J
        switch (rx_buffer[0])
        {
            case 0xAA:
                data[0]=65;
                pill_name=0;//999感冒灵
            break;
            case 0xBB:
                data[0]=66;
                pill_name=1;//999感冒灵
            break;            
            case 0xCC:
                data[0]=67;
                pill_name=2;//999感冒灵
            break;            
            case 0xDD:
                data[0]=68;
                pill_name=3;//999感冒灵
            break;
        }
        
        char *data2 = "OLED+SLE Data\n";
        
        printf("发送数组数据: ");
        for(int i = 0; i < sizeof(data); i++) {
            printf("%c ", data[i]);
        }
        printf("\n");
        
        int ret1 = sle_client_send_data(data, sizeof(data));
        printf("数组发送结果: %d\n", ret1);
        
        int ret2 = sle_client_send_data((uint8_t *)data2, strlen(data2));
        printf("字符串发送结果: %d\n", ret2);
        
        osDelay(100);  // 每秒发送一次
    }
}


static void OledDemo(void)
{
    osThreadAttr_t attr;
    attr.name = "OledTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 0x1000;   
    attr.priority = osPriorityNormal;

    // SLE线程属性 
    
    osThreadAttr_t sle_attr;
    sle_attr.name = "SleTask";
    sle_attr.attr_bits = 0U;
    sle_attr.cb_mem = NULL;
    sle_attr.cb_size = 0U;
    sle_attr.stack_mem = NULL;
    sle_attr.stack_size = 1024 * 10;  // SLE需要更大的栈
    sle_attr.priority = osPriorityNormal;
    

   //osThreadAttr_t uart_attr;
   //uart_attr.name = "UartTask";
    //uart_attr.attr_bits = 0U;
   // uart_attr.cb_mem = NULL;
   // uart_attr.cb_size = 0U;
   // uart_attr.stack_mem = NULL;
    //uart_attr.stack_size = 1024 * 10;  // 串口需要更大的栈
    //uart_attr.priority = osPriorityNormal;

    // 创建OLED线程
    if (osThreadNew(OledTask, NULL, &attr) == NULL) {
        printf("[OledDemo] Falied to create OledTask!\n");
    }
    // 创建SLE线程 
    if (osThreadNew(SleTaskInOled, NULL, &sle_attr) == NULL) {
         printf("[OledDemo] Failed to create SleTask!\n");
    }
    
    //if (osThreadNew(uart_OledTask, NULL, &uart_attr) == NULL) {
       //  printf("[OledDemo] Failed to create UartTask!\n");
    //}

}


APP_FEATURE_INIT(OledDemo);
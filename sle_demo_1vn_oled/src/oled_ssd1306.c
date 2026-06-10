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
#include <stddef.h>    
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_i2c.h"  
#include "iot_errno.h" 
#include "oled_fonts.h"
#include "oled_ssd1306.h"
// i2c波特率
#define OLED_I2C_BAUDRATE (400000) // 400KHz
// i2c主机id
#define OLED_I2C_IDX 1
// SSD1306驱动芯片从机地址
#define OLED_I2C_ADDR 0x3C 
// 写命令
#define OLED_I2C_CMD 0x00  
// 写数据
#define OLED_I2C_DATA 0x40

// OLED屏幕尺寸定义
#define OLED_WIDTH  128
#define OLED_HEIGHT 64 


typedef struct
{ 
    unsigned char *sendBuf;
    unsigned int sendLen;
    unsigned char *receiveBuf;
    unsigned int receiveLen;
} IotI2cData;

//向OLED写一个字节
static uint32_t I2cWiteByte(uint8_t regAddr, uint8_t byte)
{
   
    uint8_t buffer[] = {regAddr, byte};
    IotI2cData i2cData = {0};
    i2cData.sendBuf = buffer;
    i2cData.sendLen = sizeof(buffer) / sizeof(buffer[0]);
    return IoTI2cWrite(OLED_I2C_IDX, OLED_I2C_ADDR, i2cData.sendBuf, i2cData.sendLen);
}

//向OLED写一个命令字节
static uint32_t WriteCmd(uint8_t cmd)
{
    return I2cWiteByte(OLED_I2C_CMD, cmd);
}

//向OLED写一个数据字节
uint32_t WriteData(uint8_t data)
{
    return I2cWiteByte(OLED_I2C_DATA, data);
}

//初始化SSD1306显示屏驱动芯片
uint32_t OledInit(void)
{

    static const uint8_t initCmds[] = {
        0xAE, // --display off
        0x00, // ---set low column address
        0x10, // ---set high column address
        0x40, // --set start line address
        0xB0, // --set page address
        0x81, // contract control
        0xFF, // --128
        0xA1, // set segment remap
        0xA6, // --normal / reverse
        0xA8, // --set multiplex ratio(1 to 64)
        0x3F, // --1/32 duty
        0xC8, // Com scan direction
        0xD3, // -set display offset
        0x00,
        0xD5, // set osc division
        0x80,
        0xD8, // set area color mode off
        0x05,
        0xD9, // Set Pre-Charge Period
        0xF1,
        0xDA, // set com pin configuration
        0x12,
        0xDB, // set Vcomh
        0x30,
        0x8D, // set charge pump enable
        0x14,
        0xAF, // --turn on oled panel
    };

    // 初始化GPIO
    IoTGpioInit(1);
    // 设置GPIO-15引脚功能为I2C1_SDA
    IoSetFunc(IOT_IO_NAME_GPIO_15, IOT_IO_FUNC_GPIO_15_I2C1_SDA);
    // 设置GPIO-16引脚功能为I2C1_SCL
    IoSetFunc(IOT_IO_NAME_GPIO_16, IOT_IO_FUNC_GPIO_16_I2C1_SCL);
    // 初始化I2C1
    IoTI2cInit(OLED_I2C_IDX, OLED_I2C_BAUDRATE);

    // 初始化SSD1306显示屏驱动芯片
    for (size_t i = 0; i < sizeof(initCmds) / sizeof(initCmds[0]); i++)
    {
        // 发送一个命令字节
        uint32_t status = WriteCmd(initCmds[i]);
        if (status != IOT_SUCCESS)
        {
            return status;
        }
    }

    // OLED初始化完成，返回成功
    return IOT_SUCCESS;
}

/// @brief 设置显示位置
/// @param x x坐标，1像素为单位
/// @param y y坐标，8像素为单位。即页面起始地址
/// @return 无
void OledSetPosition(uint8_t x, uint8_t y)
{
  
    WriteCmd(0xb0 + y);
    WriteCmd(x & 0x0f);
    WriteCmd(((x & 0xf0) >> 4) | 0x10);
}

/// @brief 全屏填充
/// @param fillData 填充的数据，1字节
/// @return 无
void OledFillScreen(uint8_t fillData)
{   
    uint8_t m = 0;
    uint8_t n = 0;
    for (m = 0; m < 8; m++)
    { 
        WriteCmd(0xb0 + m);
        WriteCmd(0x00);
        WriteCmd(0x10); 
        for (n = 0; n < 128; n++)
        {
            WriteData(fillData);
        }
    }
}



/// @brief 显示一个字符
/// @param x: x坐标，1像素为单位
/// @param y: y坐标，8像素为单位
/// @param ch: 要显示的字符
/// @param font: 字库
void OledShowChar(uint8_t x, uint8_t y, uint8_t ch, Font font)
{
    
    uint8_t c = 0;
    uint8_t i = 0;
    c = ch - ' ';
    if (font == FONT8x16) 
    {
        OledSetPosition(x, y);
        for (i = 0; i < 8; i++)
        {
            WriteData(F8X16[c * 16 + i]);
        }

        OledSetPosition(x, y + 1);
       
        for (i = 0; i < 8; i++)
        {
            WriteData(F8X16[c * 16 + 8 + i]);
        }
    }
    else 
    {
        OledSetPosition(x, y);
        for (i = 0; i < 6; i++)
        {
            WriteData(F6x8[c][i]);
        }
    }
}

/// @brief 显示一个字符串
/// @param x: x坐标，1像素为单位
/// @param y: y坐标，8像素为单位
/// @param str: 要显示的字符串
/// @param font: 字库
void OledShowString(uint8_t x, uint8_t y, const char *str, Font font)
{
   
    uint8_t j = 0;

    
    if (str == NULL)
    {
        printf("param is NULL,Please check!!!\r\n");
        return;
    }

    while (str[j])
    {
       
        OledShowChar(x, y, str[j], font);

        x += 8;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }

        j++;
    }
}

/**
 * @brief 绘制单个像素（直接I2C方式）
 * @param x X坐标 (0-127)
 * @param y Y坐标 (0-63)
 * @param color 像素颜色 (OLED_COLOR_BLACK 或 OLED_COLOR_WHITE)
 */
void OledDrawPixel(uint8_t x, uint8_t y, oled_color_t color)
{
    // 边界检查
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }
    
    // 计算页地址和位位置
    uint8_t page = y / 8;        // 页地址 (0-7)
    uint8_t bit_pos = y % 8;     // 页内位位置 (0-7)
    
    // 设置显示位置
    OledSetPosition(x, page);
    
    // 创建单像素数据
    uint8_t pixel_data = 0;
    if (color == OLED_COLOR_WHITE) {
        pixel_data = 0x80 >> bit_pos;  // 设置对应位为1
    }
    // 黑色像素不需要设置，默认为0
    
    // 发送像素数据
    WriteData(pixel_data);
}

/**
 * @brief 绘制区域（直接I2C方式）
 * @param x 起始X坐标
 * @param y 起始Y坐标  
 * @param width 区域宽度
 * @param height 区域高度
 * @param data 位图数据指针（按行存储，每行width/8字节）
 * @param color 绘制颜色
 */
void OledDrawRegion(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                   const uint8_t *data, oled_color_t color)
{
    // 边界检查
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT || 
        x + width > OLED_WIDTH || y + height > OLED_HEIGHT ||
        data == NULL) {
        printf("OledDrawRegion: Invalid parameters!\r\n");
        return;
    }
    
    // 按页绘制（每页8像素）
    for (uint8_t page = 0; page < (height + 7) / 8; page++) {
        uint8_t page_y = y + page * 8;
        
        // 设置页起始位置
        OledSetPosition(x, page_y / 8);
        
        // 逐列绘制该页的数据
        for (uint8_t col = 0; col < width; col++) {
            uint8_t current_x = x + col;
            
            // 计算该列在当前页的像素数据
            uint8_t pixel_data = 0;
            
            // 遍历该页的8个像素
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t pixel_y = page_y + bit;
                
                // 检查是否在有效范围内
                if (pixel_y < y + height) {
                    // 计算位图数据中的位置
                    uint8_t row = pixel_y - y;
                    uint8_t byte_index = row * (width / 8) + col / 8;
                    uint8_t bit_index = col % 8;
                    
                    if (byte_index < (height * width + 7) / 8) {
                        uint8_t byte_data = data[byte_index];
                        uint8_t bit_value = (byte_data >> (7 - bit_index)) & 0x01;
                        
                        if (bit_value) {
                            pixel_data |= (1 << bit);
                        }
                    }
                }
            }
            
            // 发送该列的像素数据
            WriteData(pixel_data);
        }
    }
}

/**
 * @brief 绘制16x16中文字符（专门优化）
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param data 16x16字体数据（32字节）
 * @param color 绘制颜色
 */
void OledDrawChinese16x16(uint8_t x, uint8_t y, const uint8_t *data, oled_color_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT || 
        x + 16 > OLED_WIDTH || y + 16 > OLED_HEIGHT ||
        data == NULL) {
        printf("OledDrawChinese16x16: Invalid parameters!\r\n");
        return;
    }
    
    // 16x16字体分2页显示（每页8像素）
    for (uint8_t page = 0; page < 2; page++) {
        uint8_t page_y = y + page * 8;
        
        // 设置页起始位置
        OledSetPosition(x, page_y / 8);
        
        // 逐列绘制（16列）
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t pixel_data = 0;
            
            // 计算该列的8个像素
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t pixel_y = page_y + bit;
                uint8_t row = pixel_y - y;
                
                // 计算位图数据中的位置
                // 字体数据按行存储，每行2字节（16像素），共16行
                // 每行：字节0存储像素0-7，字节1存储像素8-15
                uint8_t byte_index = row * 2 + (col >= 8 ? 1 : 0);  // 根据列位置选择字节
                uint8_t bit_index = (col >= 8) ? (col - 8) : col;    // 字节内的位索引
                
                if (byte_index < 32) {  // 确保不越界
                    uint8_t byte_data = data[byte_index];
                    uint8_t bit_value = (byte_data >> (7 - bit_index)) & 0x01;
                    
                    if (bit_value) {
                        pixel_data |= (1 << bit);
                    }
                }
            }
            
            // 发送该列的像素数据
            WriteData(pixel_data);
        }
    }
}

/**
 * @brief 绘制矩形边框
 * @param x1 起始X坐标
 * @param y1 起始Y坐标
 * @param x2 结束X坐标
 * @param y2 结束Y坐标
 * @param color 颜色
 */
void OledDrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, oled_color_t color)
{
    // 绘制四条边
    for (uint8_t x = x1; x <= x2; x++) {
        OledDrawPixel(x, y1, color);  // 上边
        OledDrawPixel(x, y2, color);  // 下边
    }
    
    for (uint8_t y = y1; y <= y2; y++) {
        OledDrawPixel(x1, y, color);  // 左边
        OledDrawPixel(x2, y, color);  // 右边
    }
}

/**
 * @brief 填充矩形
 * @param x1 起始X坐标
 * @param y1 起始Y坐标
 * @param x2 结束X坐标
 * @param y2 结束Y坐标
 * @param color 颜色
 */
void OledFillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, oled_color_t color)
{
    for (uint8_t y = y1; y <= y2; y++) {
        for (uint8_t x = x1; x <= x2; x++) {
            OledDrawPixel(x, y, color);
        }
    }
}

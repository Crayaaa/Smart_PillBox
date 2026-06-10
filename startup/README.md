# Hello World 案例

## 版权
Copyright (c) 2024-2025, Qi Yaolong.<br>
dragon@hbu.edu.cn<br>
HeBei University, China.

保留所有权利。除非遵守以下条款，否则您不得使用此文件：

本文件及其内容仅用于个人学习用途，在此范围之外的使用请先获得版权持有人的授权。

本程序由版权持有人和贡献人“按原样”提供，不提供任何明示或暗示担保，包括但不局限于对适销性和特定用途适合性的暗示担保。在任何情况下，版权持有人或贡献人对因使用本程序而导致的任何直接、间接、附带、特殊、典型或后果性损害（包括但不限于购买替代商品或服务；使用损失、数据丢失或利润损失；业务中断）不承担任何责任，无论其如何产生，也不论其责任理论为何，无论是合同、严格责任还是侵权（包括疏忽或其他），即使已告知此类损害的可能性。

## 简介

入门案例。演示如何使用星闪OpenHarmony SDK，实现一个简单的Hello World程序。

## 修复SDK BUG
由于SDK（oh_sdk_20241022）问题，`printf`函数无法实现串口输出。解决方案有两种，推荐第二种。
- 可以将`printf`函数替换为`osal_printk`函数。
- 修改`device\soc\hisilicon\ws63v100\sdk\build\config\target_config\ws63\config.py`文件，找到`'ws63-liteos-app'`部分，在其`'ram_component'`中，添加以下代码：
```python
'printf_adapt',     # 修复printf打印问题
```

## 编译

1. 修改`applications\sample\wifi-iot\app\BUILD.gn`文件，放行以下代码：
```gn
# Application "Hello World"
lite_component("app") {
    # features字段指定业务模块，使目标模块参与编译。
    features = [
        # 在features字段中增加索引，包括路径和目标。
        # 路径"startup"："applications\sample\wifi-iot\app\startup"目录
        # 目标"hello_world"："applications\sample\wifi-iot\app\startup\BUILD.gn"中的静态库名称
        "startup:hello_world",
        # 如果"applications\sample\wifi-iot\app\startup\BUILD.gn"中的静态库名称与所在目录"startup"同名,
        # 即目标与路径同名，可以简写为：
        # "startup",
    ]
}
```

2. 在`device\soc\hisilicon\ws63v100\sdk\build\config\target_config\ws63\config.py`文件中，找到`'ws63-liteos-app'`部分，在其`'ram_component'`中，放行以下代码：
```python
"hello_world"
```

3. 在`device\soc\hisilicon\ws63v100\sdk\libs_url\ws63\cmake\ohos.cmake`文件中，找到`set(COMPONENT_LIST`部分，放行以下代码：
```cmake
"hello_world"
```

4. 在OpenHarmony SDK根目录（例如`~/openharmony/sdk20240628`）下，执行如下命令进行编译：
```shell
rm -rf out && hb set -p nearlink_dk_3863 && hb build -f
```

5. 编译日志中出现如下信息，表示编译成功：
```verilog
[OHOS INFO] nearlink_dk_3863 build success
[OHOS INFO] Cost time:  0:00:18
```
- 编译成功后，生成的固件文件在`out\nearlink_dk_3863\nearlink_dk_3863\ws63-liteos-app`目录下，文件名为`ws63-liteos-app_load_only.fwpkg`，或者`ws63-liteos-app_all.fwpkg`。
- 如果编译失败，根据编译日志中的错误信息，结合`out\nearlink_dk_3863\nearlink_dk_3863`目录中的编译日志文件（*.log）中的错误信息，定位错误，修改代码后重新编译。

## 烧录

使用"BurnTool"或者"快速烧录工具"，将编译生成的`ws63-liteos-app_load_only.fwpkg`或者`ws63-liteos-app_all.fwpkg`固件文件烧录到开发板中。

## 硬件准备
- 核心板

## 运行

运行串口工具（SecureCRT、MobaXterm等），并连接到开发板串口。重启开发板，查看串口输出信息。

## 运行效果

```verilog
boot.
Flash Init Fail! ret = 0x80001341
verify_public_rootkey secure verify disable!
verify_params_key_area secure verify disable!
verify_params_area_info secure verify disable!
verify_image_key_area secure verify disable!
verify_image_code_info secure verify disable!
SSB Uart Init Succ!
Flash Init Succ!
SSB Flash Init Succ!
ETI: 0x3
verify_image_key_area secure verify disable!
verify_image_code_info secure verify disable!
Flashboot Uart Init Succ!
Flashboot Malloc Init Succ!
Flash Init Succ!
[UPG] upgrade init OK!
No need to upgrade...
flash_encrypt disable.
verify_image_key_area secure verify disable!
verify_image_code_info secure verify disable!
APP|Debug uart init succ.
[UPG] upgrade init OK!
APP|init_dev_addr, mac_addr:0x 0,0x16,0x3e,0x2b,0x**,0x**,
[osal_irq_request:57]:LOS_HwiCreate failed! irq[53] ret = 0x2000904.
APP|AT uart init succ.
los_at_plt_cmd_register EXCUTE
los_at_radar_cmd_register EXCUTE
[osal_msg_queue_create:25]:qName:dfx_msg qID=0x0 
APP|=========FS MOUNT=========
APP|=========FS READY=========
APP|WARNING: main_initialise::thread[11] func is null
hilog will init.

hievent will init.

hievent init success.
LFS [E]:lfs_file_open failed, ret = 0xfffffffe, name = tmp_persist_parameters
Please implement the interface according to the platform!
Hello world!
cpu 0 entering scheduler
[osal_msg_queue_create:25]:qName:BthChannel qID=0x4 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x5 
[osal_msg_queue_create:25]:qName:BthChannel qID=0x6 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x7 
[osal_irq_request:57]:LOS_HwiCreate failed! irq[46] ret = 0x2000904.
APP|btc open
hiview init success.
[RADAR_LOG] alg ctrl read from nv[0x2140]:1,0,0,0,1,0,
[osal_kthread_set_priority:59]:parameter invalid!
[osal_kthread_set_priority:59]:parameter invalid!
device_main_init: 0!
===hal_initialize_phy===222===
device_module_init:: succ!
```

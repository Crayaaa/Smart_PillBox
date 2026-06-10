# 星闪1vn标准案例

## 版权
Copyright (c) 2024-2025, Qi Yaolong.<br>
dragon@hbu.edu.cn<br>
HeBei University, China.

保留所有权利。除非遵守以下条款，否则您不得使用此文件：

本文件及其内容仅用于个人学习用途，在此范围之外的使用请先获得版权持有人的授权。

本程序由版权持有人和贡献人“按原样”提供，不提供任何明示或暗示担保，包括但不局限于对适销性和特定用途适合性的暗示担保。在任何情况下，版权持有人或贡献人对因使用本程序而导致的任何直接、间接、附带、特殊、典型或后果性损害（包括但不限于购买替代商品或服务；使用损失、数据丢失或利润损失；业务中断）不承担任何责任，无论其如何产生，也不论其责任理论为何，无论是合同、严格责任还是侵权（包括疏忽或其他），即使已告知此类损害的可能性。

## 简介

星闪1vn标准案例。一个客户端和多个服务端使用星闪SLE能力发送/接收数据。

遵循SparkLink团体标准。对客户端和服务端进行封装，更易用。

可以作为项目模板，适用于各种星闪1vn通讯场景。

包括客户端（client）和服务端（server）。

### 主（Master）模式
主模式是指星闪设备发起连接的模式。在主模式下，星闪设备主动搜索其他设备并建立连接。
在编程时一般称为客户（Client）端。

### 从（Slave）模式
从模式是指星闪设备被连接的模式。在从模式下，星闪设备等待其他设备的连接。
在编程时一般称为服务（Server）端。

## 工作流程

### 客户端
01. 注册回调函数，使能SLE，设置本地设备地址
02. sle_enable_cbk           : 使能SLE成功后，扫描
03. seek_result_info_cbk     : 扫描结果上报后，连接
04. connect_state_changed_cbk: 连接成功后，交换信息（Exchange Info）
05. exchange_info_cbk        : 交换信息成功后，查找服务（Find Structure）
06. find_structure_cbk       : 服务发现成功后，保存service信息，可以发送数据
07. find_structure_cmp_cbk   : 查找服务完成

### 服务端
连接断开后，可以重新广播

## 编译

0. 选择模式。在本案例目录下的`BUILD.gn`文件中，找到`sources = [...]`部分，放行其中任意一行，例如：
```gn
  sources = [
    "sle_client.c", "sle_client_demo.c",
    # "sle_server.c", "sle_server_adv.c", "sle_server_demo.c",
  ]
```

1. 修改`applications\sample\wifi-iot\app\BUILD.gn`文件，放行以下代码：
```gn
# Application "sle_demo_1vn"
lite_component("app") {
    features = [
        "sle_demo_1vn",                                     # 案例程序模块
        "ssd1306_3rd_driver/ssd1306:oled_ssd1306",          # OLED驱动模块
    ]
}
```

2. 在`device\soc\hisilicon\ws63v100\sdk\build\config\target_config\ws63\config.py`文件中，找到`'ws63-liteos-app'`部分，在其`'ram_component'`中，放行以下代码：
```python
"sle_demo_1vn", "oled_ssd1306"
```

3. 在`device\soc\hisilicon\ws63v100\sdk\libs_url\ws63\cmake\ohos.cmake`文件中，找到`set(COMPONENT_LIST`部分，放行以下代码：
```cmake
"sle_demo_1vn" "oled_ssd1306"
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

6. 分别编译客户端和服务端。

## 烧录

使用"BurnTool"或者"快速烧录工具"，将编译生成的`ws63-liteos-app_load_only.fwpkg`或者`ws63-liteos-app_all.fwpkg`固件文件烧录到开发板中。

使用至少3块开发板，分别烧录1个客户端和至少2个服务端。

## 硬件准备
- 核心板
- 0.96英寸OLED显示屏（SSD1306驱动芯片）

## 运行

运行串口工具（SecureCRT、MobaXterm等），并连接到开发板串口。重启开发板，查看串口输出信息。

## 运行效果

### 客户端

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
cpu 0 entering scheduler
[osal_msg_queue_create:25]:qName:BthChannel qID=0x4 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x5 
[osal_msg_queue_create:25]:qName:BthChannel qID=0x6 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x7 
[osal_irq_request:57]:LOS_HwiCreate failed! irq[46] ret = 0x2000904.
APP|btc open
hiview init success.
[SleClientDemo] Create SleTask successfully!
[RADAR_LOG] alg ctrl read from nv[0x2140]:1,0,0,0,1,0,
[osal_kthread_set_priority:59]:parameter invalid!
[osal_kthread_set_priority:59]:parameter invalid!
local_addr: a6:f5:42:01:1f:84
[uuid client] ssapc_register_client 
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 10
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 12
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 8
[ACore] sle enable cbk in, result:0
sle enable
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 10
device_main_init: 0!
===hal_initialize_phy===222===
device_module_init:: succ!
cali_set_cali_mask:old[0x0] -> new[0x1fa2]
cali_set_cali_data_mask:old[0x0] -> new[0x14]

fe_rf_initialize
cali_offline_cali_entry enter
cali_set_cali_done_flag:old[0x0] -> new[0x1]

rf cali OK. time cost:23, ret:0
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[adv_report] event_type: 0x03, addr_type: 0x0000, addr: 36:**:**:**:13:58
[adv_report] data length: 6, data: 0x01 0x02 0x01 0x02 0x02 0x00
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[adv_report] event_type: 0x0b, addr_type: 0x0000, addr: 36:**:**:**:13:58
[adv_report] data length: 20, data: 0x0c 0x02 0x0a 0x10 0x0b 0x73
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 36:a6:c3:40:13:58
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 36:a6:c3:40:13:58
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 36:a6:c3:40:13:58
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 36:a6:c3:40:13:58
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 36:a6:c3:40:13:58
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 36:a6:c3:40:13:58
hci api_c2h_read [HCI][C-->H]: evt_code = 1401 [len] 32
[Connected]
addr:36:**:**:**:13:58, handle:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
[sle client] [sle_client_connect_state_changed_cbk] conn_id:0x00, conn_state:0x1, pair_state:0x1,         disc_reason:0x0
[sle client] [sle_client_connect_state_changed_cbk] addr:36:a6:c3:40:13:58
[sle client] [sle_client_connect_state_changed_cbk] SLE_ACB_STATE_CONNECTED
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1801 [len] 14
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 14
ssapc exchange info, conn_id:0, err_code:0
[sle client] [sle_client_exchange_info_cbk] client id:0 status:0
[sle client] [sle_client_exchange_info_cbk] mtu size: 512, version: 1
hci api_c2h_read [HCI][C-->H]: evt_code = 1802 [len] 10
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 17
discovery character cbk complete in
[sle client] [sle_client_find_structure_cbk] client: 0 conn_id:0 status: 0 
[sle client] [sle_client_find_structure_cbk] start_hdl:[0x01], end_hdl:[0x02], uuid len:2
[sle client] [sle_client_find_structure_cmp_cbk] client id:0 status:0 type:1 uuid len:0 
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 28
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:0, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 28
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:0, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 28
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:0, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 28
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:0, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 1a:70:63:6e:22:0d
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1401 [len] 32
[Connected]
addr:1a:**:**:**:22:0d, handle:01
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
[sle client] [sle_client_connect_state_changed_cbk] conn_id:0x01, conn_state:0x1, pair_state:0x1,         disc_reason:0x0
[sle client] [sle_client_connect_state_changed_cbk] addr:1a:70:63:6e:22:0d
hci api_c2h_read [HCI][C-->H]: evt_code = 1801 [len] 14
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
[sle client] [sle_client_connect_state_changed_cbk] SLE_ACB_STATE_CONNECTED
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1802 [len] 10
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 14
ssapc exchange info, conn_id:1, err_code:0
[sle client] [sle_client_exchange_info_cbk] client id:0 status:0
[sle client] [sle_client_exchange_info_cbk] mtu size: 512, version: 1
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 17
discovery character cbk complete in
[sle client] [sle_client_find_structure_cbk] client: 0 conn_id:1 status: 0 
[sle client] [sle_client_find_structure_cbk] start_hdl:[0x01], end_hdl:[0x02], uuid len:2
[sle client] [sle_client_find_structure_cmp_cbk] client id:0 status:0 type:1 uuid len:0 
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 28
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 28
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

APP|[SYS INFO] mem: used:181612, free:141096; log: drop/all[0/0], at_recv 0.
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 5e:c4:a6:ce:12:e9
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 5e:c4:a6:ce:12:e9
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 5e:c4:a6:ce:12:e9
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
hci api_c2h_read [HCI][C-->H]: evt_code = 180b [len] 278
[sle client] [sle_client_seek_result_info_cbk] target found, addr: 5e:c4:a6:ce:12:e9
hci api_c2h_read [HCI][C-->H]: evt_code = 1401 [len] 32
[Connected]
addr:5e:**:**:**:12:e9, handle:02
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
[sle client] [sle_client_connect_state_changed_cbk] conn_id:0x02, conn_state:0x1, pair_state:0x1,         disc_reason:0x0
[sle client] [sle_client_connect_state_changed_cbk] addr:5e:c4:a6:ce:12:e9
[sle client] [sle_client_connect_state_changed_cbk] SLE_ACB_STATE_CONNECTED
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1801 [len] 14
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 14
ssapc exchange info, conn_id:2, err_code:0
[sle client] [sle_client_exchange_info_cbk] client id:0 status:0
[sle client] [sle_client_exchange_info_cbk] mtu size: 512, version: 1
hci api_c2h_read [HCI][C-->H]: evt_code = 1802 [len] 10
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 17
discovery character cbk complete in
[sle client] [sle_client_find_structure_cbk] client: 0 conn_id:2 status: 0 
[sle client] [sle_client_find_structure_cbk] start_hdl:[0x01], end_hdl:[0x02], uuid len:2
[sle client] [sle_client_find_structure_cmp_cbk] client id:0 status:0 type:1 uuid len:0 
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

APP|[SYS INFO] mem: used:183020, free:139688; log: drop/all[0/0], at_recv 0.
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:0, err_code:0
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
[sle write_req_complete_cbk]conn_id:1, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
ssapc write rsp handle:1
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[sle write_req_complete_cbk]conn_id:0, err_code:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 32
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:1, err_code:0
ssapc write rsp handle:1
[sle write_req_complete_cbk]conn_id:2, err_code:0
[sle client] [sle_client_write_cfm_cbk] conn_id:0 client id:0 status:0 handle:01 type:00
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 13
[sle client] [sle_client_write_cfm_cbk] conn_id:1 client id:0 status:0 handle:01 type:00
[sle client] [sle_client_write_cfm_cbk] conn_id:2 client id:0 status:0 handle:01 type:00
[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!

[ssapc_notification_cbk] server_send_data: Response from SLE server!
```

### 服务端1

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
cpu 0 entering scheduler
[osal_msg_queue_create:25]:qName:BthChannel qID=0x4 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x5 
[osal_msg_queue_create:25]:qName:BthChannel qID=0x6 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x7 
[osal_irq_request:57]:LOS_HwiCreate failed! irq[46] ret = 0x2000904.
APP|btc open
hiview init success.
[SleServerDemo] Create SleTask successfully!
[RADAR_LOG] id:[0x2140] read nv fail!
[osal_kthread_set_priority:59]:parameter invalid!
[osal_kthread_set_priority:59]:parameter invalid!
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 10
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 12
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 8
[ACore] sle enable cbk in, result:0
[sle server] [sle_enable_cbk] status:0
[sle server] sle add service in
[sle server] sle add service, server_id:1, service_handle:1, property_handle:2
[sle server] sle add service out
AnnounceParam.ownAddr: 1a:70:63:6e:22:0d
[ACore] sle set announce param, handle:1, mode:3, min_interval:c8, max_interval:c8, tx_power: 0
[ACore] sle set announce param, own addr:0x1a:**:**:**:22:0d
[ACore] sle set announce param, peer addr:0x00:**:**:**:00:00
[sle server] data.announce_data_len = 6
[sle server] data.announce_data: 0x01 0x02 0x01 0x02 0x02 0x00 
[sle server] local_name_len = 15
[sle server] local_name: 0x73 0x6c 0x65 0x5f 0x73 0x65 0x72 0x76 0x65 0x72 0x00 0x00 0x00 0x00 0x00 
[sle server] data.seek_rsp_data_len = 20
[sle server] data.seek_rsp_data: 0x0c 0x02 0x0a 0x10 0x0b 0x73 0x6c 0x65 0x5f 0x73 0x65 0x72 0x76 0x65 0x72 0x00 0x00 0x00 0x00 0x00 
[sle server] [sle_set_default_announce_data] set announce data success.
[ACore] sle start announce in, adv_id:1
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
[ACore] sle adv cbk in, event:0 status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
[ACore] sle adv cbk in, event:1 status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
[ACore] sle adv cbk in, event:2 status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
[ACore] sle adv cbk in, event:3 status:0
[sle server] [ssaps_start_service_cbk] server_id:1, handle:1, status:0
[sle server] [sle_announce_enable_cbk] id:01, state:0
sle enable
[sle server] [sle_server_init] init ok
hci api_c2h_read [HCI][C-->H]: evt_code = 1401 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 4 [len] 4
[Connected]
addr:a6:**:**:**:1f:84, handle:00
[ACore] sle adv cbk in, event:7 status:0
[sle server] [sle_connect_state_changed_cbk] conn_id:0x00, conn_state:0x1, pair_state:0x1,         disc_reason:0x0
[sle server] [sle_connect_state_changed_cbk] addr:a6:f5:42:01:1f:84
[sle server] [sle_announce_terminal_cbk] id:01
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 14
[sle server] [ssaps_mtu_changed_cbk] server_id:0, conn_id:0, mtu_size:200, status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 14
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 28
[ssaps_write_request_cbk] client_send_data: heart beat: 9

update ssap send report handle: pre handle:ffff, current:0 
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
device_main_init: 0!
===hal_initialize_phy===222===
device_module_init:: succ!
cali_set_cali_mask:old[0x0] -> new[0x1fa2]
cali_set_cali_data_mask:old[0x0] -> new[0x14]

fe_rf_initialize
cali_offline_cali_entry enter
{fe_hal_phy_wait_fft_ready::timeout!}
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
{fe_hal_phy_wait_fft_ready::timeout!}
{fe_hal_phy_wait_fft_ready::timeout!}
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
cali_set_cali_done_flag:old[0x0] -> new[0x1]

rf cali OK. time cost:67, ret:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 10

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 11

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 12

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 13

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 14

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 15

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 16

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 17

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 18

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 19

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
APP|[SYS INFO] mem: used:119092, free:206992; log: drop/all[0/0], at_recv 0.
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 20

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 21

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 22

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 23

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 24

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 25

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 26

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 27

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 28

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
```

### 服务端2

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
cpu 0 entering scheduler
[osal_msg_queue_create:25]:qName:BthChannel qID=0x4 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x5 
[osal_msg_queue_create:25]:qName:BthChannel qID=0x6 
[osal_msg_queue_create:25]:qName:BtcChannel qID=0x7 
[osal_irq_request:57]:LOS_HwiCreate failed! irq[46] ret = 0x2000904.
APP|btc open
hiview init success.
[SleServerDemo] Create SleTask successfully!
[RADAR_LOG] alg ctrl read from nv[0x2140]:1,0,0,0,1,0,
[osal_kthread_set_priority:59]:parameter invalid!
[osal_kthread_set_priority:59]:parameter invalid!
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 10
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 12
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 8
[ACore] sle enable cbk in, result:0
[sle server] [sle_enable_cbk] status:0
[sle server] sle add service in
[sle server] sle add service, server_id:1, service_handle:1, property_handle:2
[sle server] sle add service out
AnnounceParam.ownAddr: 5e:c4:a6:ce:12:e9
[ACore] sle set announce param, handle:1, mode:3, min_interval:c8, max_interval:c8, tx_power: 0
[ACore] sle set announce param, own addr:0x5e:**:**:**:12:e9
[ACore] sle set announce param, peer addr:0x00:**:**:**:00:00
[sle server] data.announce_data_len = 6
[sle server] data.announce_data: 0x01 0x02 0x01 0x02 0x02 0x00 
[sle server] local_name_len = 15
[sle server] local_name: 0x73 0x6c 0x65 0x5f 0x73 0x65 0x72 0x76 0x65 0x72 0x00 0x00 0x00 0x00 0x00 
[sle server] data.seek_rsp_data_len = 20
[sle server] data.seek_rsp_data: 0x0c 0x02 0x0a 0x10 0x0b 0x73 0x6c 0x65 0x5f 0x73 0x65 0x72 0x76 0x65 0x72 0x00 0x00 0x00 0x00 0x00 
[sle server] [sle_set_default_announce_data] set announce data success.
[ACore] sle start announce in, adv_id:1
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 6
[ACore] sle adv cbk in, event:0 status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
[ACore] sle adv cbk in, event:1 status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
[ACore] sle adv cbk in, event:2 status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 2 [len] 4
[ACore] sle adv cbk in, event:3 status:0
[sle server] [ssaps_start_service_cbk] server_id:1, handle:1, status:0
[sle server] [sle_announce_enable_cbk] id:01, state:0
sle enable
[sle server] [sle_server_init] init ok
hci api_c2h_read [HCI][C-->H]: evt_code = 1401 [len] 32
hci api_c2h_read [HCI][C-->H]: evt_code = 4 [len] 4
[Connected]
addr:a6:**:**:**:1f:84, handle:00
[ACore] sle adv cbk in, event:7 status:0
[sle server] [sle_connect_state_changed_cbk] conn_id:0x00, conn_state:0x1, pair_state:0x1,         disc_reason:0x0
[sle server] [sle_connect_state_changed_cbk] addr:a6:f5:42:01:1f:84
[sle server] [sle_announce_terminal_cbk] id:01
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 14
[sle server] [ssaps_mtu_changed_cbk] server_id:0, conn_id:0, mtu_size:200, status:0
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 14
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
device_main_init: 0!
===hal_initialize_phy===222===
device_module_init:: succ!
cali_set_cali_mask:old[0x0] -> new[0x1fa2]
cali_set_cali_data_mask:old[0x0] -> new[0x14]

fe_rf_initialize
cali_offline_cali_entry enter
tx_pwr cali not enough: real_pow:-138 target_pwr:180
tx_pwr cali not enough: real_pow:-72 target_pwr:110
tx_pwr cali not enough: real_pow:-138 target_pwr:180
tx_pwr cali not enough: real_pow:-72 target_pwr:110
tx_pwr cali not enough: real_pow:-138 target_pwr:180
tx_pwr cali not enough: real_pow:-72 target_pwr:110
{fe_hal_phy_wait_fft_ready::timeout!}
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
{fe_hal_phy_wait_fft_ready::timeout!}
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
        hal_cali_complex_div: l_power is zero!
cali_set_cali_done_flag:old[0x0] -> new[0x1]

rf cali OK. time cost:75, ret:0
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 17

update ssap send report handle: pre handle:ffff, current:0 
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 18

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 19

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 20

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 21

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 22

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 23

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 24

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 25

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 26

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 27

APP|[SYS INFO] mem: used:119208, free:206876; log: drop/all[0/0], at_recv 0.
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 28

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
```

### 服务端3

```verilog
......
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 11

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 12

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 13

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 14

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
APP|[SYS INFO] mem: used:119076, free:207008; log: drop/all[0/0], at_recv 0.
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 15

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 16

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 17

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 18

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 19

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 20

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 21

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 22

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 23

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 24

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 25

APP|[SYS INFO] mem: used:119184, free:206900; log: drop/all[0/0], at_recv 0.
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 26

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 27

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 1 [len] 29
[ssaps_write_request_cbk] client_send_data: heart beat: 28

hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
hci api_c2h_read [HCI][C-->H]: evt_code = 9 [len] 4
```

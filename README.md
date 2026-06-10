# Smart_PillBox
项目是一套面向居家场景的智能药盒系统，旨在帮助老年人规范、安全用药。系统以 K230 视觉模块为核心，通过端侧 OCR 模型识别药盒文字，自动区分感冒灵、左氧氟沙星滴眼液、蒲地蓝消炎片、双黄连口服液等常见药品；识别结果经星闪（SLE）无线通信同步至 OLED 显示屏与 51 单片机语音主控。语音模块集成 SYN6288 中文合成芯片，可在 8:30、14:30、20:30 等时段定时播报用药提醒，并实时统计库存、在余量不足时提示补药；DS18B20 监测盒内温度，超过 28℃ 时语音预警，保障药品存放安全。系统融合视觉识别、无线互联与语音交互，实现「看得见、听得懂、记得住」的全流程用药辅助。
# 智能药盒 · 星闪（SLE）无线通信开发

本仓库开源智能药盒项目中的**星闪（SLE）无线通信**部分，基于 OpenHarmony 与 WS63 星闪开发板，遵循 SparkLink 团体标准。提供 1 对多（1vn）星闪通信方案，支持 UART 与星闪之间的数据透传，可作为星闪多设备互联场景的开发参考与项目模板。

## 功能特性

- **1vn 星闪通信**：一个客户端同时连接多个服务端，实现一对多无线数据分发
- **UART 透传桥接**：客户端接收上位机串口指令，经星闪转发至各服务端
- **OLED 屏显联动**：服务端接收指令后在 SSD1306 屏上显示药品名称与时间
- **语音模块联动**：服务端将星闪指令转为串口字符，驱动 51 单片机语音播报
- **模块化封装**：SLE Client / Server 独立封装，接口清晰，便于二次开发
- **多案例可选**：提供 1v1、1vn 标准案例及智能药盒业务定制案例

## 硬件要求

| 设备 | 说明 |
|------|------|
| WS63 星闪开发板（NearLink DK3863） | 核心通信模块，需多块（客户端 1 块 + 服务端若干块） |
| 0.96 英寸 OLED（SSD1306） | 用于 `sle_demo_1vn_oled` 显示端 |
| 上位机 / K230 等 | 通过 UART 向客户端发送识别结果（`sle_demo_1vn_k230`） |
| 51 单片机语音模块 | 通过 UART 接收语音服务端指令（`sle_demo_1vn_voice`） |

## 系统架构

```
┌─────────────┐   UART(115200)   ┌──────────────────┐
│  K230 /     │ ───────────────► │ sle_demo_1vn_k230│
│  上位机     │   0xAA/BB/CC/DD  │  (SLE Client)     │
└─────────────┘                  └────────┬─────────┘
                                          │ 星闪 SLE 无线
                    ┌─────────────────────┼─────────────────────┐
                    ▼                     ▼                     ▼
         ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
         │ sle_demo_1vn_oled│  │ sle_demo_1vn_voice│  │  其他 SLE Server │
         │ (SLE Server)     │  │ (SLE Server)      │  │                  │
         │ + OLED 显示      │  │ + UART 转发       │  │                  │
         └──────────────────┘  └────────┬─────────┘  └──────────────────┘
                                          │ UART(9600)
                                          ▼
                               ┌──────────────────┐
                               │ 51 单片机语音模块 │
                               └──────────────────┘
```

## 目录结构

```
sle_app/
├── BUILD.gn                    # 应用入口，选择编译的 demo 模块
├── sle_demo_1vn_k230/          # 【客户端】UART 接收 + SLE 转发
├── sle_demo_1vn_oled/          # 【服务端】SLE 接收 + OLED 显示
├── sle_demo_1vn_voice/         # 【服务端】SLE 接收 + UART 转发至语音模块
├── sle_demo_1vn/               # 星闪 1vn 标准案例（通用模板）
├── sle_demo_1v1/               # 星闪 1v1 标准案例
├── uart_test/                  # UART 通信测试
├── uart_test_ready/            # UART 就绪测试
├── new_app_mix/                # OLED + SLE 混合应用
├── gn_practice/                # GN 构建练习
├── startup/                    # Hello World 入门
└── 01_timer/                   # 定时器示例
```

## 核心模块说明

### sle_demo_1vn_k230（SLE 客户端）

- 通过 UART 中断接收上位机数据
- 将十六进制帧头映射为字符，经星闪发送至已连接的服务端
- 支持数据包去重，避免重复转发
- 波特率：**115200**

### sle_demo_1vn_oled（SLE + OLED 服务端）

- 作为星闪服务端广播并等待连接
- 接收客户端字符指令，在 OLED 上显示对应药品名称
- 双线程架构：SLE 任务 + OLED 显示任务（含时钟刷新）

### sle_demo_1vn_voice（SLE + UART 服务端）

- 作为星闪服务端接收客户端指令
- 将字符 `A/B/C/D` 转为串口数据转发至 51 语音主控
- 双线程架构：SLE 任务 + UART 通信任务
- 波特率：**9600**

## 通信协议

### UART → SLE 客户端（上位机发送）

| UART 字节 | 星闪转发字符 | 含义 |
|-----------|-------------|------|
| `0xAA` | `A` | 999 感冒灵 |
| `0xBB` | `B` | 左氧氟沙星滴眼液 |
| `0xCC` | `C` | 蒲地蓝消炎片 |
| `0xDD` | `D` | 双黄连口服液 |

### SLE → OLED 服务端（显示映射）

| 接收字符 | pill_name | 显示内容 |
|----------|-----------|----------|
| `A` | 0 | 感冒灵 |
| `B` | 1 | 左氧氟沙星滴眼液 |
| `C` | 2 | 浦蓝地消炎片 |
| `D` | 3 | 双黄连口服液 |

### SLE → 语音服务端 → 51 单片机

| 接收字符 | UART 发送 |
|----------|-----------|
| `A` | `0x41` |
| `B` | `0x42` |
| `C` | `0x43` |
| `D` | `0x44` |

## 编译说明

本代码需在 **OpenHarmony SDK** 环境中编译，作为 `applications/sample/wifi-iot/app` 的子模块使用。

### 1. 选择编译模块

编辑 `sle_app/BUILD.gn`，在 `features` 中启用目标模块（每次只编译一个角色）：

```gn
lite_component("app") {
    features = [
        "sle_demo_1vn_k230",      # UART + SLE 客户端
        # "sle_demo_1vn_oled",     # SLE + OLED 服务端
        # "sle_demo_1vn_voice",    # SLE + 语音 UART 服务端
    ]
}
```

同时编辑对应子目录下的 `BUILD.gn`，在 `sources` 中选择 Client 或 Server 源码。

### 2. 注册到 SDK 工程

在 OpenHarmony SDK 中修改以下文件（以 `sle_demo_1vn_oled` 为例）：

**`applications/sample/wifi-iot/app/BUILD.gn`**

```gn
lite_component("app") {
    features = [
        "sle_demo_1vn_oled",
    ]
}
```

**`device/soc/hisilicon/ws63v100/sdk/build/config/target_config/ws63/config.py`**

在 `'ws63-liteos-app'` 的 `'ram_component'` 中添加：

```python
"sle_demo_1vn_oled"
```

**`device/soc/hisilicon/ws63v100/sdk/libs_url/ws63/cmake/ohos.cmake`**

在 `set(COMPONENT_LIST` 中添加：

```cmake
"sle_demo_1vn_oled"
```

### 3. 执行编译

在 OpenHarmony SDK 根目录执行：

```shell
rm -rf out && hb set -p nearlink_dk_3863 && hb build -f
```

编译成功后，固件位于：

```
out/nearlink_dk_3863/nearlink_dk_3863/ws63-liteos-app/
├── ws63-liteos-app_load_only.fwpkg
└── ws63-liteos-app_all.fwpkg
```

日志出现 `nearlink_dk_3863 build success` 即表示编译成功。

> **注意**：客户端与服务端需分别修改 `BUILD.gn` 后各自编译、烧录。

## 烧录与运行

1. 使用 BurnTool 或快速烧录工具，将 `.fwpkg` 固件烧录至 WS63 开发板
2. 智能药盒完整方案至少需要：
   - 1 块板烧录 `sle_demo_1vn_k230`（客户端）
   - 1 块板烧录 `sle_demo_1vn_oled`（OLED 显示端）
   - 1 块板烧录 `sle_demo_1vn_voice`（语音转发端）
3. 连接串口工具（SecureCRT、MobaXterm 等），波特率 115200，查看运行日志
4. 客户端上电后自动扫描并连接所有 `sle_server` 广播设备

各子模块的详细编译步骤与运行日志示例，请参阅对应目录下的 README：

- [sle_demo_1vn_k230/README.md](sle_demo_1vn_k230/README.md)
- [sle_demo_1vn_oled/README.md](sle_demo_1vn_oled/README.md)
- [sle_demo_1vn_voice/README.md](sle_demo_1vn_voice/README.md)
- [sle_demo_1vn/README.md](sle_demo_1vn/README.md)
- [sle_demo_1v1/README.md](sle_demo_1v1/README.md)

## SLE 工作流程

### 客户端（Client）

1. 使能 SLE → 扫描周边设备
2. 发现 `sle_server` → 建立连接
3. 交换信息（Exchange Info）→ 发现服务（Find Structure）
4. 服务发现完成 → 可发送 / 接收数据

### 服务端（Server）

1. 使能 SLE → 添加服务 → 开始广播
2. 等待客户端连接 → 接收写入数据
3. 连接断开后可重新广播

## 常见问题

**Q：编译报错找不到头文件？**  
确认已将目标模块加入 `config.py` 和 `ohos.cmake`，并执行了 `hb build -f` 全量编译。

**Q：客户端连不上服务端？**  
检查多块板是否均已上电，服务端广播名是否为 `sle_server`，天线与距离是否正常。

**Q：UART 无数据？**  
K230 客户端波特率为 115200，语音服务端为 9600，请与对接设备保持一致。

**Q：OLED 不显示药品名？**  
确认客户端已成功连接 OLED 服务端，且发送的字符为 `A`/`B`/`C`/`D`。


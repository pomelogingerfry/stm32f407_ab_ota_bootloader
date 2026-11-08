# STM32F407 OTA Bootloader（Boot + 双 App 分区 + 回滚 + 故障快照）

基于 STM32F407ZGT6 + HAL + MDK-AC6 的串口/CAN 双通道 OTA 升级工程，带 A/B 双分区、
CRC 校验、失败自动回滚、HardFault 现场快照与故障注入测试。

## 1. 功能一览

- Bootloader 独立分区（128KB），上电先跑，负责升级、校验、跳转
- A/B 双 App 分区（各 384KB），升级永远写“另一个”槽，不覆盖正在运行的固件
- 元数据双备份（槽0/槽1），任意时刻至少一份有效
- 帧级 CRC32 + 整片 CRC32 双重校验
- 校验失败自动回滚到旧槽
- HardFault / BusFault / 看门狗现场存备份 SRAM，复位后 App 直接打印
- 故障注入：模拟断电、非法指令、非法地址，用于验证回滚链路
- 上位机 Python 工具：下发固件、构造坏包、解析日志

## 2. 目录结构

```
.
├── bootloader/              # Bootloader 工程（CubeMX 生成 MDK-ARM 工程）
│   ├── Core/Inc/            # 头文件：hal_flash / ota_protocol / state_machine ...
│   ├── Core/Src/            # 实现（main.c 为模板，合进 CubeMX 生成的 main.c）
│   └── LD/                  # bootloader.sct + app_partition.h
├── app/                     # 应用程序工程
│   ├── Core/Inc/            # app_fault_read.h
│   ├── Core/Src/            # app_fault_read.c / main.c(模板)
│   └── LD/                  # app.sct + app_partition.h（与 bootloader 保持同步）
├── docs/                    # 接线、分区、协议、测试用例、踩坑
├── tools/                   # ota_host.py / log_parser.py
├── firmware/                # 编译输出 bin/hex（已 gitignore）
├── .gitignore
├── README.md
└── CHANGELOG.md
```

## 3. 硬件

| 功能 | 引脚 |
|---|---|
| CAN1 | PB8 (RX) / PB9 (TX) → SN65HVD230 |
| USART1 | PA9 (TX) / PA10 (RX) → CH340 |
| SWD | PA13 / PA14 → ST-Link |
| LED | PC13 |

详细接线见 `docs/hardware/hardware_connect.md`。

## 4. 编译与烧录

1. **两个独立 CubeMX 工程**，不是一个工程两个文件夹：
   - `bootloader`：选 STM32F407ZGT6，Toolchain 选 `MDK-ARM V5`，开 CAN1 + USART1，生成到 `bootloader/`
   - `app`：同样芯片，同样外设，生成到 `app/`
2. 把本仓库的 `Core/Inc`、`Core/Src` 文件加入对应工程 Group（CubeMX 不会自动加）
3. 两个工程的 Include Paths 都要加：
   - `../bootloader/Core/Inc`（App 需要 `fault_capture.h` 结构体）
   - `../LD`（`app_partition.h`）
4. Linker 里指定 scatter file：
   - bootloader → `..\LD\bootloader.sct`（起始 0x08000000）
   - app → `..\LD\app.sct`（起始 0x08040000）
5. 输出 bin：Options → User → After Build 填
   ```
   fromelf --bin --output ..\..\firmware\bootloader\bootloader.bin .\Objects\bootloader.axf
   ```
   app 同理输出到 `firmware\app\app.bin`
6. 烧录顺序：先 `bootloader.bin` @0x08000000，再 `app.bin` @0x08040000

## 5. 演示步骤

1. 只烧 bootloader，上电 → 串口应打印 `STATE=FAULT`（没有 App）
2. 烧 App（首次用 Keil 直接下载到 0x08040000），上电 → 进 App，LED 闪烁
3. 打开 XCOM 观察，另开终端执行：
   ```
   python tools/ota_host.py flash firmware/app/app.bin --port COM3
   ```
4. 每帧应答 OK，收完自动复位，串口打印新版本号
5. 测试回滚：`--corrupt 100` 再执行一次，应看到 CRC 失败、ROLLBACK、跳回旧固件
6. 测试崩溃快照：App 里调 `FaultInject_Run(INJ_HARDFAULT)`，复位后打印 PC/LR/CFSR

## 6. 关键常量（改分区只改这些）

| 宏 | 值 |
|---|---|
| `BOOT_ADDR` | 0x08000000 (128KB) |
| `META_SLOT0_ADDR` / `META_SLOT1_ADDR` | 0x08020000 / 0x08027000 |
| `APP_A_ADDR` | 0x08040000 (384KB) |
| `APP_B_ADDR` | 0x080A0000 (384KB) |
| `VECT_TAB_OFFSET_A` | 0x00040000 |
| `FAULT_RECORD_ADDR` | 0x40024000（备份 SRAM） |

改完必须同步 `.sct`，否则链接地址和代码判断会打架。

## 7. 已知限制

- `aes_rsa_crypto.c` 是 fail-closed 的桩实现，要真验签需接 micro-ECC 或 mbedTLS
- CAN 分支上位机是简化分片，接收侧拼包逻辑需按实际帧长调整
- 需在 CubeMX 里自行开启 IWDG 才能测看门狗复位用例

## 8. 后续迭代

见 `CHANGELOG.md` 的 tag 规划，按 v1.0 → v1.5 逐步加串口 OTA、CAN OTA、安全校验、
可靠性增强、上位机工具、收尾文档。

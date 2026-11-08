# 版本变更记录

## v1.0-base_serial_ota
- 建立 bootloader + app 双工程骨架，MDK-AC6，`bootloader.sct` / `app.sct` 分区
- 串口 OTA 全链路：BEGIN / DATA / END 三阶段握手，逐帧 CRC32 校验
- Flash 驱动：按扇区擦除、按字写入、边写边验
- A/B 双分区跳转，VTOR 重定位

## v1.1-can_ota
- 接入 SN65HVD230，CAN1 500kbps 收固件
- CAN 过滤器只放行 0x601，帧 ID 0x602 回传应答
- 上位机 `ota_host.py` 增加 `--can` 分支
- 修复：CAN 收帧后未清 FIFO 导致重复处理

## v1.2-security_enhance
- 引入 `aes_rsa_crypto`：RSA 验签（fail-closed）+ AES-128-CBC 传输解密
- SHA-256 固件摘要，元数据增加 `fw_version` 比对，防降级攻击
- 未验签的固件一律拒绝，不回写 Flash

## v1.3-reliability_enhance
- 元数据双槽备份 + 版本号择优读取
- 校验失败自动 ROLLBACK 到另一个有效槽
- 故障快照：HardFault / BusFault / UsageFault 现场存备份 SRAM
- 故障注入模块：`INJ_POWER_LOST` 模拟升级中途断电
- 修复：`UPGRADING` 状态断电后卡死，补 ROLLBACK 兜底

## v1.4-python_tool
- `ota_host.py` 支持 `--corrupt` 构造坏包、`--version` 指定版本
- `log_parser.py` 统计升级/回滚/崩溃次数与应答分布
- 帧序号断号自动重发，最多 3 次

## v1.5-intern_final
- 文档补齐：接线、分区、状态机、协议、测试用例、踩坑
- 测试用例 A1~A4 / B1~B5 / C1~C4 / D1~D4 全部跑通
- 已知限制写进 README，桩实现位置明确标注
- 简历用：项目描述 + 难点 + 量化指标（升级耗时、50 次擦写无损、30 次掉电无变砖）

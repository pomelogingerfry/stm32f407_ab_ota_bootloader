## OTA Protocol

# 帧格式：帧类型 + 包序号 + 长度 + 数据 + 整包 CRC32

首包START:8字节大端(4字节固件总长+4字节全局CRC)
分包DATA带SEQ序号,停等ACK重传
末包END结束接收

每包必须回ACK;上位机超时200ms重发;同一序号连续3次失败则整体中止

fw_size>SLOT_SIZE必须在VERIFY阶段判REJECT,不能写到一半越界

REJECT是拒绝本次升级不动active槽,不改标志;ROLLBACK是新固件试运行失败后回旧槽

# 状态机迁移:
BOOT->IDLE->RECEIVING->VERIFY->SWITCH->TRIAL->CONFIRMED / REJECT / ROLLBACK
IDLE默认3s无升级指令就跳active

# 重点理解:
1.双层CRC:
帧CRC:单帧传输校验,防串口出错
全局CRC:整片固件写入Flash后校验,放丢包,写坏

2.SEQ序号容错:
序号偏小:重传包,只写ACK,不写Flash
序号不符:直接丢弃

3.自愈回滚机制:
升级断电:无Trial标记,上电跑旧固件
新固件崩溃:启动次数超限(3次),自动回滚旧固件
新固件自检成功->自动转正常驻
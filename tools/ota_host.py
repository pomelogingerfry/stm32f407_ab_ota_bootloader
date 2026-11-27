import serial
import struct
import zlib
import argparse
import time
import os

OTA_HEAD_0=0xAA
OTA_HEAD_1=0xBB
OTA_MAX_DATA_LEN=1024

OTA_CMD_START=0x01
OTA_CMD_DATA=0x02
OTA_CMD_END=0x03
OTA_CMD_ACK=0x04

ACK_TIMEOUT=3.0
MAX_RETRY=3
ACK_LEN=6

def clac_zlib_crc32(body_bytes:bytes)->int:
    crc=zlib.crc32(body_bytes)
    return crc&0xFFFFFFFF

def clac_whole_firmware_crc32(fw_bytes:bytes)->int:
    return zlib.crc32(fw_bytes)&0xFFFFFFFF

def build_frame(cmd:int,seq:int,payload:bytes)->bytes:
    payload_len=len(payload)
    body=struct.pack(">BHH",cmd,seq,payload_len)+payload
    crc_body=clac_zlib_crc32(body)
    crc_for_send=(~crc_body)&0xFFFFFFFF
    frame=bytes([OTA_HEAD_0,OTA_HEAD_1])+body+struct.pack(">I",crc_for_send)
    return frame

def drain_input(ser:serial.Serial,duration:float):
    t0=time.time()
    dumped=bytearray()
    while time.time()-t0<duration:
        n=getattr(ser,"in_waiting",0)
        if n<=0:
            time.sleep(0.01)
            continue
        chunk=ser.read(n)
        if chunk:
            dumped+=chunk
    return bytes(dumped)

def send_frame_wait_ack(ser:serial.Serial,cmd:int,seq:int,payload:bytes)->bool:
    tx_frame=build_frame(cmd,seq,payload)
    for retry in range(MAX_RETRY):
        ser.reset_input_buffer()
        ser.write(tx_frame)
        ser.flush()
        print(f"发送帧: cmd=0x{cmd:02X} seq={seq} len={len(payload)} 共{len(tx_frame)}字节")
        buf=bytearray()
        t0=time.time()
        got_ack=False
        while time.time()-t0<ACK_TIMEOUT:
            n=ser.in_waiting
            if n>0:
                buf+=ser.read(n)
            while True:
                idx=buf.find(bytes([OTA_HEAD_0,OTA_HEAD_1]))
                if idx<0:
                    if len(buf)>1:
                        buf=buf[-1:]
                    break
                if idx>0:
                    buf=buf[idx:]
                if len(buf)<ACK_LEN:
                    break
                h0=buf[0]
                h1=buf[1]
                ack_cmd=buf[2]
                ack_status=buf[3]
                ack_seq=(buf[4]<<8)|buf[5]
                if h0==OTA_HEAD_0 and h1==OTA_HEAD_1 and ack_cmd==OTA_CMD_ACK:
                    del buf[0:ACK_LEN]
                    if ack_status==0 and ack_seq==seq:
                        got_ack=True
                        break
                    if ack_status!=0:
                        print(f"[WARN] seq={seq} NACK status={ack_status}")
                    continue
                del buf[0:1]
            if got_ack:
                break
            time.sleep(0.005)
        if got_ack:
            return True
        print(f"[WARN] seq={seq} 第{retry+1}次等待ACK超时({ACK_TIMEOUT}s)")
    return False

def send_start(ser:serial.Serial,seq:int,payload:bytes)->bool:
    return send_frame_wait_ack(ser,OTA_CMD_START,seq,payload)

def main():
    global ACK_TIMEOUT
    parser=argparse.ArgumentParser(description="OTA上位机,带START启动包&END包")
    parser.add_argument("--port",required=True,help="UART COM NUM")
    parser.add_argument("--file",required=True,help="BIN FILE PATH")
    parser.add_argument("--baud",type=int,default=115200,help="UART BAUD,默认115200")
    parser.add_argument("--ack-timeout",type=float,default=ACK_TIMEOUT,help="等待ACK超时秒数,默认3.0")
    parser.add_argument("--start-delay",type=float,default=0.3,help="复位后等待bootloader就绪的秒数,默认0.3")

    args=parser.parse_args()
    ACK_TIMEOUT=args.ack_timeout

    if not os.path.exists(args.file):
        print(f"[ERROR] FILE NOT EXISTS:{args.file}")
        return

    with open(args.file,"rb") as f:
        fw_data=f.read()
    fw_total_len=len(fw_data)
    fw_total_crc=clac_whole_firmware_crc32(fw_data)
    if fw_total_len==0:
        print("[ERROR] 固件为空")
        return
    print(f"固件总大小:{fw_total_len} bytes")
    print(f"固件整体CRC32:0x{fw_total_crc:08X}")

    ser=serial.Serial(port=args.port,baudrate=args.baud,timeout=0.01)
    if not ser.is_open:
        print("[ERROR] 串口打开失败,请检测串口是否被占用")
        return

    try:
        ser.dtr=False
        ser.rts=False
        time.sleep(args.start_delay)
        tail=drain_input(ser,0.2)
        if tail:
            print(f"[INFO] 丢弃启动前的残留字节 {len(tail)} 个")

        seq_num=0
        start_payload=struct.pack(">II",fw_total_len,fw_total_crc)
        print("\n=====发送OTA_CMD_START启动包=====")
        print(f"OTA_CMD_START payload: fw_total_len={fw_total_len}, fw_total_crc=0x{fw_total_crc:08X}")
        print("[INFO] bootloader擦除Flash需要数秒,START的ACK可能明显变慢,请耐心等待")
        if not send_start(ser,seq_num,start_payload):
            print("[ERROR] START包应答失败,升级中止")
            return
        print(f"[OK] seq={seq_num} START已应答")
        seq_num+=1

        print("\n=====开始发送OTA_CMD_DATA固件数据包=====")
        offset=0
        total_chunks=(fw_total_len+OTA_MAX_DATA_LEN-1)//OTA_MAX_DATA_LEN
        done_chunks=0
        last_print=0.0
        while offset<fw_total_len:
            chunk=fw_data[offset:offset+OTA_MAX_DATA_LEN]
            if not send_frame_wait_ack(ser,OTA_CMD_DATA,seq_num,chunk):
                print(f"[ERROR] seq={seq_num} DATA包应答失败,升级中止")
                return
            offset+=len(chunk)
            done_chunks+=1
            now=time.time()
            if done_chunks==total_chunks or now-last_print>1.0:
                last_print=now
                print(f"[OK] seq={seq_num:4d} offset={offset:6d}/{fw_total_len} chunk={done_chunks}/{total_chunks}")
            seq_num+=1

        print("\n=====发送OTA_CMD_END结束包=====")
        print("[INFO] bootloader校验整体CRC并写标志,END的应答可能很慢")
        if not send_frame_wait_ack(ser,OTA_CMD_END,seq_num,b""):
            print("[ERROR] END包应答失败,升级中止")
            return
        print("\n[INFO] 全部帧发送完成!")
        print("[INFO] Bootloader:校验整体CRC,写boot_flag,Trial试运行标记")
    finally:
        ser.close()

if __name__=="__main__":
    main()
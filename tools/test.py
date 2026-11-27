import serial
ser=serial.Serial("COM4",115200,timeout=0.1)
print("Sucess!")
ser.close()
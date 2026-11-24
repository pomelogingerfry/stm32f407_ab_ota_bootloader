## Bootloader OTA 升级流程理解笔记
1.初始化环形缓冲区

2.调用statemachine init，如果有试运行标记，进行判断是否进行下一次试运行.没有或者非法将pendingslot置none，flag state置idle.最后将sm state置dile.

3.状态机轮询 statemachine run启动，根据状态机的状态做出相应处理
idle：判断3秒超时，跳转app
verify：从flash读数据计算crc，如果匹配写trial标志，并切换到switch状态
switch：调用nvic systemreset重启

4.收到数据先由dma自动把数据搬到rx buffer暂存，一帧数据发完后，总线idle，触发UART3中断，中断层调用ringbuffer write 把数据写进环形缓冲

5.查询ringbuffer available，发现缓冲区有数据，调用ringbuffer read，将数据放入parse buffer

6.将取出的数据交给ota protocol parse解析（寻找帧头，提取cmd，seq，len，接收data，crc32，存入全局变量ota pack）
crc校验通过后，把标志位ota rx flag置1，并调用ota protocol sendack 回一个ack

7.main中while1检测到ota rx flag为1时，调用statemachine oneframereceived ，读取ota packet中的内容，并根据packet内容和当前statemachine状态做出响应
start包：解析固件大小和总crc，擦除目标扇区，切换statemachine到receiving状态
data包：检查seq是否正确，调用flash write 写入，累加total received
end包：切换statemachine到verify状态
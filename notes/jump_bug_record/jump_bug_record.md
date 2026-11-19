# Jump程序调试过程中遇到的问题
1.bootloader侧没有hal rcc deinit

2.使用c语言函数指针进行跳转，压栈动作破坏栈顶数据

3.栈顶合法范围设置过小，合法栈顶可能被当做非法处理

4.清理中断先后顺序错误，清理nvic寄存器时如果有中断触发可能打断，应该先关总闸disable irq再清理

5.app侧没有开启总中断enable irq，因为hal init会重新配置systick，不开总中断，systick进不去，无法计时，如果调用hal delay就会死等

6.app侧没有设置vtor，由于bootloader侧在跳转之前调用hal rcc deinit或执行其他底层操作，可能导致部分芯片内核状态被重置

7.app侧设置vtor，boot跳转前没有关systick，或者关了但没清挂起标志，跳过去后还没执行scb vtor，systick中断爆发，跑去boot旧向量表找函数，导致死机重启

8.boot侧没有关systick，app中main函数执行hal init时会调用hal initsystick重新配置systick，如果这时候还有旧的systick状态残留，会直接配死

9.system clock config配置问题，bootloader侧没有rcc deinit



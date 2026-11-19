# HardFault—Capture 调试过程中的问题

## 最重要问题:

1.hardfault快照复位后丢失(原本一直以为没进capture，没捕获到信息)
原因:__attribute__((section(".noinit")))生成的是PROGBITS段，armlink的UINIT只对ZI段生效，
__main会生成__scatterload_copy每次复位都用 ROM 里的 0覆盖快照
解决方法:改成.bss.noinit(ZI/NOBITS)+scatter里*(.bss.noinit)放进UNINIT区

2.进入hardfault_capture后死锁
原因:当发生故障时，硬件会自动把 R0-R3 等寄存器压入栈中，如果是因为栈溢出导致的崩溃，此时的 sp 指针可能已经越界到了非法地址，
如果你执行glo_buf.r0 = sp[0]，CPU读取非法内存，会瞬间引发二次 HardFault，进入Lockup死锁状态
解决方法:在提取sp之前，做边界合法性检查

3.反复复位，串口刷屏
原因:捕获完直接NVIC_SystemReset()，下次上电仍会走同一条故障路径，进了捕获函数然后反复复位
解决方法:保存到flash后应当停机或置标志，不能立刻又跳 App

## 其他问题

1.Boot和App的快照区必须是同一固定地址(0x2001E000)，否则App里发生的 HardFault，
Bootloader永远读不到那个RAM地址

2.用绝对地址法#define global_buf (*(volatile struct*)0x2001F000),可以绕过编译器，编译阶段链接器不会给它分配段属性，
而且系统复位时，C库启动代码执行的分散加载(scatter load)会直接绕过他，从而实现数据的保留
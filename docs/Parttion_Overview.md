###   区域	 起始	 结束	 大小	 扇区	                        用途/宏名
Bootloader	0x08000000	0x08020000	128KB	  s0~s4	                启动、OTA、跳转；BOOT_BASE/BOOT_SIZE


Meta 升级标志	0x08020000	0x08040000	128KB	s5	             boot flag 双备份；META_BASE / META_SIZE


Slot A（AppA）	0x08040000	0x08080000	256KB	s6~s7	            运行固件槽 A；APP_A_BASE


Slot B（AppB）	0x08080000	0x080C0000	256KB	s8~s9	            待升级固件槽 B；APP_B_BASE


故障快照	0x080C0000	0x080E0000	128KB	s10	                   HardFault快照；FAULT_BASE


预留	0x080E0000	0x08100000	128KB	s11	                         Meta 镜像 + 日志；META_MIRROR_BASE
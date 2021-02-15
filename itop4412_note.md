https://www.cnblogs.com/topeet/p/10839409.html

# 1.使用```/home/w/workspace/itop4412/支持设备树的镜像文件```

1. 使用4412默认的uboot更新支持设备树的uboot
```fastboot flash bootloader u-boot-iTOP-4412.bin```

2. 进入支持设备树的uboot控制台，使用命令```fastboot 0```

3. 更新内核、设备树、文件系统使用一下命令
```fastboot flash kernel uImage```
```flastboot flash dtb exuons4412-itop-elite.dtb```
```fastboot flash system system.img```

使用系统编程中的文件系统源码目录下的system.img
使用fastboot flash bootloader u-boot-iTOP-4412.bin

4. 使用```fastboot reboot```重启开发板 

# 2.使用过程中遇到的问题总结
## 1. 更新完镜像文件后重启开发板发现文件系统挂载不了，不能进入控制台，提示找不到root device
需要修改配置文件 arch/arm/configs/iTop-4412_scp_defconfig将
```CONFIG_CMDLINE="root=/dev/mmcblk0p2 console=ttySAC2,115200 init=/linuxrc rootwait"```
改为
```CONFIG_CMDLINE="root=/dev/mmcblk1p2 console=ttySAC2,115200 init=/linuxrc rootwait"```
重新编译烧写内核。

## 2. 控制台一直有pid报错，需要处理最小linux系统
修改system文件夹内```etc/init.d/rcS```屏蔽```/dev/tty2~4```相关的项。
使用```make_ext4fs -s -l 314572800 -a root -L Linux system.img system```命令生成system文件重新烧写

## 3. 文件系统无法新建文件夹
在root=/dev/mmcblk1p2后面加上rw更改为```CONFIG_CMDLINE="root=/dev/mmcblk1p2 rw console=ttySAC2,115200 init=/linuxrc rootwait"```表示文件系统可读可写，然后编译uImage和system.img文件系统，更新系统镜像

## 4.开发板进入linux系统后挂载tf卡
使用```mkdir /mnt/udisk```命令在mnt目录下创建udisk目录

使用```mount /dev/mmcblk0p1 /mnt/udisk```命令挂载tf卡到/mnt/udisk目录，mmcblk0p1根据系统情况选择

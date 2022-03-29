# mcs

#### 介绍

该模块用于提供OpenAMP样例的内核态与用户态支持。

#### 软件架构

kernel_cpu_handler:  提供OpenAMP所需内核模块，支持Client OS启动、专用中断收发等功能。

openamp_demo: 提供OpenAMP用户态程序样例，支持与指定Client OS进行通信。


#### 安装教程

1. 根据openEuler Embedded使用手册安装SDK并设置SDK环境变量。

2. 编译内核模块cpu_handler_dev.ko，编译方式如下:

````
    cd kernel_cpu_handler
    make
````

3. 编译用户态程序rpmsg_main，编译方式如下:

````
    cd openamp_demo
    make
````

注意：此处定义OpenAMP通信设备共享内存起始地址为0x70000000，可根据实际内存分配进行修改。

4. 将编译好的KO模块、用户态程序，以及zephyr.bin镜像拷贝到OpenEuler Embedded 系统的目录下。如何拷贝可以参考使用手册中共享文件系统场景。

5. 将OpenAMP的依赖库libmetal.so，libopen_amp.so，libsysfs.so拷贝至文件系统/lib64目录。如何拷贝可以参考使用手册中共享文件系统场景。

#### 使用说明

1.  在openEuler Embedded系统上插入内核KO模块cpu_handler_dev.ko。

````
    insmod cpu_handler_dev.ko
````

2.  运行rpmsg_main程序，使用方式如下:

````
    ./rpmsg_main -c [cpu_id] -b [boot_address] -t [target_binfile] -a [target_binaddress]
    eg:
    ./rpmsg_main -c 3 -b 0x7a000ffc -t /tmp/zephyr.bin -a 0x7a000000
````

此处定义Client OS起始地址为0x7a000000，启动地址为0x7a000ffc。

注意：以上述demo为例，需要预留出地址0x70000000为起始的内存用于OpenAMP demo和Client OS启动。通过QEMU启动时，当指定-m 1G时默认使用0x40000000-0x80000000的系统内存。添加内核启动参数`mem=768M`，可预留地址为0x70000000-0x80000000的256M内存，可根据实际情况进行调整。

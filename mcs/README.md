# mcs

#### 介绍

该模块用于提供OpenAMP样例的内核态与用户态支持。

#### 软件架构

kernel_cpu_handler:  提供OpenAMP所需内核模块，支持Client OS启动、专用中断收发等功能。

openamp_demo: 提供OpenAMP用户态程序Linux端样例，支持与指定Client OS进行通信。

zephyr: 提供样例zephyr.bin镜像文件，该文件需要被加载至0x7a000000的起始地址，并在0x7a00ffc的地址进行启动。启动后会运行OpenAMP Client端的样例程序，并与Linux端进行交互。

#### 原理简介

OpenAMP旨在通过非对称多处理器的开源解决方案来标准化异构嵌入式系统中操作环境之间的交互。

OpenAMP是一个软件框架，提供了为AMP系统开发软件应用程序所需的软件组件，允许操作系统在复杂的同构和异构体系结构中交互。

OpenAMP包括如下三大重要组件：

-virtio：该组件是rpmsg组件的实现基础。

-rpmsg：实现多核处理器通信的通道，基于virtio组件实现。

-remoteproc：该组件用于主机上实现对远程处理器及相关软件环境的生命周期管理、及virtio和rpmsg设备的注册等。

样例Demo通过提供cpu_handler_dev内核KO模块实现Linux内核启动从核的功能，并预留了OpenAMP通信所需的专用中断及其收发机制。用户可在用户态通过dev设备实现Client OS的启动，并通过rpmsg组件实现与Client OS的简单通信。

#### 安装教程

1.  根据openEuler Embedded使用手册安装SDK并设置SDK环境变量。

2.  编译内核模块cpu_handler_dev.ko，编译方式如下:

````
    cd kernel_cpu_handler
    make
````

3.  编译用户态程序rpmsg_main，编译方式如下:

````
    cd openamp_demo
    make
````

注意：此处定义OpenAMP通信设备共享内存起始地址为0x70000000，可根据实际内存分配进行修改。

4.  将编译好的KO模块、用户态程序，以及zephyr.bin镜像拷贝到openEuler Embedded系统的目录下。如何拷贝可以参考使用手册中共享文件系统场景。

5.  将OpenAMP的依赖库libmetal.so*，libopen_amp.so*，libsysfs.so*拷贝至文件系统/lib64目录。对应so可在sdk目录中找到，如何拷贝可以参考使用手册中共享文件系统场景。

#### 使用说明

1.  通过QEMU启动openEuler Embedded镜像，如何启动可参考使用手册中QEMU使用与调试章节。

-以上述demo为例，需要预留出地址0x70000000为起始的内存用于OpenAMP demo和Client OS启动。通过QEMU启动时，当指定-m 1G时默认使用0x40000000-0x80000000的系统内存。添加内核启动参数mem=768M，可预留地址为0x70000000-0x80000000的256M内存。
-在样例中在cpu 3启动Client OS，需要预留出3号cpu。
-样例中zephyr镜像默认gic版本为3，需要在QEMU中设置。

可参考如下命令进行启动：

````
    qemu-system-aarch64 -M virt,gic_version=3 -m 1G -cpu cortex-a57 -nographic -kernel zImage -initrd initrd -append 'mem=768M maxcpus=3' -smp 4 
````

2.  在openEuler Embedded系统上插入内核KO模块cpu_handler_dev.ko。

````
    insmod cpu_handler_dev.ko
````

3.  运行rpmsg_main程序，使用方式如下:

````
    ./rpmsg_main -c [cpu_id] -b [boot_address] -t [target_binfile] -a [target_binaddress]
    eg:
    ./rpmsg_main -c 3 -b 0x7a000ffc -t /tmp/zephyr.bin -a 0x7a000000
````

此处定义Client OS起始地址为0x7a000000，启动地址为0x7a000ffc，Client OS镜像路径为/tmp/zephyr.bin。


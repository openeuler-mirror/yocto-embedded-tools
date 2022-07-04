# cross_tools

#### 介绍

该模块用于制作openEuler嵌入式的交叉编译器

#### 软件架构和配置说明

configs:  依赖工具及其crosstool-ng的各架构构建配置

prepare.sh: 用于下载构建所需的依赖仓库，并按照下载的路径，刷新config

对于64位编译器，脚本中(update_feature)通过修改GCC源码，默认从lib64目录下寻找链接器，并在libstdc++.so中添加默认安全选项（relro、now、noexecstack）

可通过ct-ng show-config查看配置基础情况（例如cp config_aarch64 .config && ct-ng show-config）

最终配置可参见输出件*gcc -v

例（arm64）：

````
COLLECT_GCC=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu/bin/aarch64-openeuler-linux-gnu-gcc
COLLECT_LTO_WRAPPER=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu/libexec/gcc/aarch64-openeuler-linux-gnu/10.3.1/lto-wrapper
Target: aarch64-openeuler-linux-gnu
Configured with: /usr1/cross-ng_openeuler/.build/aarch64-openeuler-linux-gnu/src/gcc/configure --build=x86_64-build_pc-linux-gnu --host=x86_64-build_pc-linux-gnu --target=aarch64-openeuler-linux-gnu --prefix=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu --exec_prefix=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu --with-sysroot=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu/aarch64-openeuler-linux-gnu/sysroot --enable-languages=c,c++,fortran --with-pkgversion='crosstool-NG 1.25.0' --enable-__cxa_atexit --disable-libmudflap --enable-libgomp --disable-libssp --disable-libquadmath --disable-libquadmath-support --disable-libsanitizer --disable-libmpx --disable-libstdcxx-verbose --with-gmp=/usr1/cross-ng_openeuler/.build/aarch64-openeuler-linux-gnu/buildtools --with-mpfr=/usr1/cross-ng_openeuler/.build/aarch64-openeuler-linux-gnu/buildtools --with-mpc=/usr1/cross-ng_openeuler/.build/aarch64-openeuler-linux-gnu/buildtools --with-isl=/usr1/cross-ng_openeuler/.build/aarch64-openeuler-linux-gnu/buildtools --enable-lto --enable-threads=posix --enable-target-optspace --enable-plugin --enable-gold --disable-nls --enable-multiarch --with-multilib-list=lp64 --with-local-prefix=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu/aarch64-openeuler-linux-gnu/sysroot --enable-long-long --with-arch=armv8-a --with-gnu-as --with-gnu-ld --enable-c99 --enable-shared --enable-poison-system-directories --enable-symvers=gnu --disable-bootstrap --disable-libstdcxx-dual-abi --enable-default-pie --libdir=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu/lib64 --with-build-time-tools=/home/openeuler/x-tools/aarch64-openeuler-linux-gnu/aarch64-openeuler-linux-gnu/bin
Thread model: posix
Supported LTO compression algorithms: zlib
gcc version 10.3.1 (crosstool-NG 1.25.0)
````

#### 使用教程

1.  准备交叉编译器构建所需的容器镜像并进入（以下仅为例子，实际镜像所用分支，以openEuler Embedded在线文档描述为准）

````
    sudo docker pull swr.cn-north-4.myhuaweicloud.com/openeuler-embedded/openeuler-compile-tool:21.03-1
    sudo docker run -idt --network host swr.cn-north-4.myhuaweicloud.com/openeuler-embedded/openeuler-compile-tool:21.03-1 bash
````

2.  下载本仓库的代码，并通过脚本一键准备构建所需的代码和配置:

````
    cd /usr1 && git clone -b openEuler-22.03-LTS https://gitee.com/openeuler/yocto-embedded-tools.git
    cd yocto-embedded-tools/cross_tools
    ./prepare.sh
````

4.  使用普通用户，通过ct-ng工具（本容器构建环境已集成）进行构建。

````
    chown -R openeuler:users /usr1
    su openeuler
    #aarch64:
    cp config_aarch64 .config && ct-ng build
    #arm32
    cp config_arm32 .config && ct-ng build
````

5.  等待构建完成后，在对应工作目录的./build中有构建中间件，在/home/openeuler/x-tools/下有构建的输出件

以arm64为例，重命名目录后打包即可使用。/home/openeuler/x-tools/aarch64-openeuler-linux-gnu下的内容和yocto构建容器的/usr1/openeuler/gcc/openeuler_gcc_arm64le下的内容一致

````
    cd /home/openeuler/x-tools/
    mv aarch64-openeuler-linux-gnu openeuler_gcc_arm64le
    tar czf openeuler_gcc_arm64le.tar.gz openeuler_gcc_arm64le
````


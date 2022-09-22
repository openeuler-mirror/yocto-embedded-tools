## gcc-tools

### 介绍

该python脚本用于gcc全自动化构建任务，任务流程如下：

1. 构建选定的gcc编译链包，可选的gcc编译链有openeuler_gcc_arm64le.tar.gz、openeuler_gcc_arm32le.tar.gz、openeuler_gcc_x86_64.tar.gz、openeuler_gcc_riscv64.tar.gz。

2. 上传已构建好的gcc tar包到远程服务器。

### 使用教程：

1. 准备交叉编译器构建所需的CI容器镜像并进入

```
sudo docker pull swr.cn-north-4.myhuaweicloud.com/openeuler-embedded/openeuler-ci-gcc:latest
sudo docker run -itd --network host swr.cn-north-4.myhuaweicloud.com/openeuler-embedded/openeuler-ci-gcc:latest bash
```

2. 安装gcc-tools脚本所需的库

```
pip install six
```

3. 将本仓库代码下载到/usr1目录，并更改本仓库目录的所属用户组，然后进入到本仓库目录执行main.py脚本

```
sudo git clone --depth=1 https://gitee.com/openeuler/yocto-embedded-tools.git -v /usr1/yocto-embedded-tools
sudo chown -R jenkins:jenkins /usr1/yocto-embedded-tools
cd /usr1/yocto-embedded-tools
python3 build_tools/gcc-tools/main.py \
	-u remoteName \
	-p remotePasswd \
	-skey remotePkey \
	-ip remoteIp \
	-dst remoteDstDir \
	-archs "aarch64 arm32"
```

### 参数解析：

-u：远程服务器登录用户名

-p：远程服务器登录密码

-skey：远程服务器登陆密钥，如果同时传入-p和-skey，则选用-skey参数

-ip：远程服务器IP地址

-dst：gcc tar包在远程服务器存放地址

-archs：选定的需要编译的gcc版本



执行完成看到`all task finishd successful`输出则表示所有构建全部完成，登录远程主机，查看remoteDesDir目录，该目录下会出现gcc目录，gcc目录下存放着gcc最新编译链。
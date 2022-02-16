# base image
FROM openeuler/openeuler:21.03

# MAINTAINER
MAINTAINER zhangweigang3@huawei.com

# yum install
RUN set -eux; \
    yum -y install git java tar cmake gperf sqlite-devel libffi-devel xz-devel zlib zlib-devel openssl-devel bzip2-devel ncurses-devel readline-devel libpcap-devel parted
RUN set -eux; \
    yum -y install chrpath gcc-c++ patch rpm-build flex autoconf automake m4 bison bc libtool gettext-devel createrepo_c git net-tools wget sudo hostname rpcgen texinfo python meson dosfstools mtools

# gcc install
ARG tools_dir=/usr1/tools
ARG gcc_install_dir=/usr1/openeuler/gcc

# change dir to workdir and star install
WORKDIR ${gcc_install_dir}
RUN wget https://gitee.com/openeuler/yocto-embedded-tools/attach_files/911964/download/openeuler_gcc_arm32le.tar.xz -P ${tools_dir}
RUN wget https://gitee.com/openeuler/yocto-embedded-tools/attach_files/911963/download/openeuler_gcc_arm64le.tar.xz -P ${tools_dir}

# change dir to workdir and unpack
WORKDIR ${gcc_install_dir}
RUN tar -xf ${tools_dir}/openeuler_gcc_arm32le.tar.xz
RUN find ./openeuler_gcc_arm32le -type d | xargs chmod go+x
RUN chmod go+r ./openeuler_gcc_arm32le -R

RUN tar -xf ${tools_dir}/openeuler_gcc_arm64le.tar.xz
RUN find ./openeuler_gcc_arm64le -type d | xargs chmod go+x
RUN chmod go+r ./openeuler_gcc_arm64le -R

# python install
ARG python_install_dir=/opt/buildtools/python-3.9.2

# change dir to workdir and start install
WORKDIR /usr1/tools
RUN wget https://www.python.org/ftp/python/3.9.2/Python-3.9.2.tgz
RUN tar -xf Python-3.9.2.tgz
RUN rm -rf /usr/local/bin/python3 /usr/local/bin/python
WORKDIR /usr1/tools/Python-3.9.2
RUN ./configure --prefix=/opt/buildtools/python-3.9.2 --enable-loadable-sqlite-extensions
RUN make -j 8 && make install
RUN ln -s ${python_install_dir}/bin/python3 /usr/local/bin/python3
RUN ln -s ${python_install_dir}/bin/python3 /usr/local/bin/python
RUN export PYTHONPATH=${python_install_dir}/lib64/python3.9/lib-dynload/
RUN export PYTHONPATH="${python_install_dir}/lib/python3.9/site-packages/:${python_install_dir}:${python_install_dir}/lib64/python3.9/lib-dynload/"

# ninja install
ARG ninja_install_dir="/opt/buildtools/ninja-1.10.1"

# change dir to workdir and start install
WORKDIR /usr1/tools
RUN wget https://distfiles.macports.org/ninja/ninja-1.10.1.tar.gz
RUN tar -xf ninja-1.10.1.tar.gz
WORKDIR /usr1/tools/ninja-1.10.1
RUN sed -ie '1c#!/usr/bin/env python3' *.py
RUN ./configure.py --bootstrap
RUN mkdir -p ${ninja_install_dir}/bin
RUN install -m 0755 ./ninja ${ninja_install_dir}/bin

# change access permission
WORKDIR /opt/buildtools
RUN find ./ -type d | xargs chmod 755

# clean install package
WORKDIR /usr1
RUN rm -rf tools

ARG VERSION=4.3
ARG user=huawei
ARG group=huawei
ARG uid=1000
ARG gid=1000
ARG AGENT_WORKDIR=/home/agent

# add build user
RUN groupadd -g ${gid} ${group}
RUN useradd -c "huawei" -d /home/${user} -u ${uid} -g ${gid} -m ${user}
RUN echo "${user} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers


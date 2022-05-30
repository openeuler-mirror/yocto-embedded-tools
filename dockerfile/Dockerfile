# base image
FROM openeuler/openeuler:22.03-lts

# MAINTAINER
MAINTAINER harvey

# yum install
RUN set -eux; \
    yum -y install git java tar cmake gperf sqlite-devel libffi-devel xz-devel zlib zlib-devel openssl-devel bzip2-devel ncurses-devel readline-devel libpcap-devel parted autoconf-archive chrpath gcc-c++ patch rpm-build flex autoconf automake m4 bison bc libtool gettext-devel createrepo_c git net-tools wget sudo hostname rpcgen texinfo python meson dosfstools mtools libmpc-devel gmp-devel ninja-build numactl-devel make

# gcc install
ARG tools_dir=/usr1/tools
ARG gcc_install_dir=/usr1/openeuler/gcc

# change dir to workdir and star install
WORKDIR ${tools_dir}
RUN wget https://repo.openeuler.org/openEuler-22.03-LTS/EPOL/main/x86_64/Packages/gcc-cross-1.0-0.oe2203.x86_64.rpm -P ${tools_dir}
RUN rpm2cpio ${tools_dir}/gcc-cross-1.0-0.oe2203.x86_64.rpm | cpio -id

# change dir to workdir and unpack
WORKDIR ${gcc_install_dir}
RUN tar -xf ${tools_dir}/tmp/openeuler_gcc_arm32le.tar.gz
RUN find ./openeuler_gcc_arm32le -type d | xargs chmod go+x
RUN chmod go+r ./openeuler_gcc_arm32le -R
RUN chmod -R 755 /usr1/openeuler/gcc/openeuler_gcc_arm32le/bin

RUN tar -xf ${tools_dir}/tmp/openeuler_gcc_arm64le.tar.gz
RUN find ./openeuler_gcc_arm64le -type d | xargs chmod go+x
RUN chmod go+r ./openeuler_gcc_arm64le -R
RUN chmod -R 755 /usr1/openeuler/gcc/openeuler_gcc_arm64le/bin
RUN find ./ | xargs chmod go+x


# clean install package
WORKDIR /usr1
RUN rm -rf tools

ARG VERSION=4.3
ARG user=openeuler
ARG group=openeuler
ARG uid=1000
ARG gid=1000
ARG AGENT_WORKDIR=/home/agent

# add build user
RUN groupadd -g ${gid} ${group}
RUN useradd -c "openeuler" -d /home/${user} -u ${uid} -g ${gid} -m ${user}
RUN echo "${user} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# modify /usr1 owner
RUN chown -R openeuler:users /usr1

# modify default configs
RUN sed -i 's/TMOUT=300/TMOUT=/g' /etc/bashrc

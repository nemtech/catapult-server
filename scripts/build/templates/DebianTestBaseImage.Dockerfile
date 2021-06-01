FROM {{BASE_IMAGE}}
ENV DEBIAN_FRONTEND=noninteractive
MAINTAINER Catapult Development Team
RUN apt-get -y update && apt-get install -y \
    bison \
    gdb \
    git \
    flex \
    python3 \
    python3-ply \
    python3-pip \
    shellcheck \
    inotify-tools \
    libdw-dev \
    libelf-dev \
    libiberty-dev \
    libslang2-dev \
    rustc \
    libssl-dev \
    && \
    rm -rf /var/lib/apt/lists/* && \
    pip3 install -U colorama cryptography gitpython pycodestyle pylint pylint-quotes PyYAML && \
    git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linux.git && \
    cd linux.git/tools/perf && \
    make && \
    cp perf /usr/bin && \
    cd ../../.. && \
    rm -rf linux.git

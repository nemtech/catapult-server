FROM debian:10.9

RUN apt-get update >/dev/null && \
    apt-get install -y \
    apt-utils \
    build-essential \
    gnupg2 \
    curl \
    xz-utils \
    wget \
    >/dev/null && \
    apt-get update >/dev/null && \
    apt-get install -y \
    gcc g++ \
    >/dev/null && \
    rm -rf /var/lib/apt/lists/*

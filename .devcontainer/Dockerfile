FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

RUN apt update && export DEBIAN_FRONTEND=noninteractive \
    && apt -y install --no-install-recommends \
        build-essential \
        autoconf \
        autoconf-archive \
        automake \
        libtool \
        gdb \
        ninja-build \
        pkg-config \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        xorg-dev \
        libx11-dev \
        libxrandr-dev \
        libxinerama-dev \
        libxcursor-dev \
        libxi-dev \
        libicu-dev \
        libsystemd-dev \
        libltdl-dev \
        libxft-dev \
        libxext-dev \
        python3-jinja2 \
        lsb-release wget software-properties-common gnupg

# Clang 설치
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 21
RUN sudo apt install libc++-21-dev libc++abi-21-dev

# 정리
RUN rm llvm.sh && apt clean && rm -rf /var/lib/apt/lists/*

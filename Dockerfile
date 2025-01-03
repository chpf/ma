# TODO: multi stage build for toolchain and seal5, maybe even antlr4

FROM ubuntu:24.04 AS gcc_toolchain_builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install --no-install-recommends -y \
    autoconf automake autotools-dev curl python3 python3-pip \
    python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex \
    texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git \
    cmake libglib2.0-dev libslirp-dev

RUN git clone https://github.com/riscv/riscv-gnu-toolchain
WORKDIR /riscv-gnu-toolchain
RUN git checkout 43536acae8791de5fc93acad51d0c03dda9f903e
RUN ./configure --prefix=/opt/riscv --with-arch=rv32i --with-abi=ilp32 && make -j8



FROM ubuntu:24.04

COPY --from=gcc_toolchain_builder /opt/riscv /opt/riscv

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install --no-install-recommends -y \
    git \
    bash \
    clang \
    patch \
    make \
    cmake \
    sccache \
    xz-utils \
    ninja-build \
    default-jre \
    python3 \
    python3-pip \
    python3-venv \
    wget \
    curl \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean


# get seal5
COPY --from=ghcr.io/astral-sh/uv:0.5.14 /uv /uvx /bin/
WORKDIR /app
RUN git clone --branch main https://github.com/tum-ei-eda/seal5.git && \
    cd seal5 && \
    git checkout 3d4b9746cd21a4cd76564fdf5b88d5c82e9f75b4 && \
    git submodule update --init --remote

# TODO: make sure to keep this layer it's expensive to rebuild  (20 mins)
RUN cd seal5 && \
    uv venv && \
    . .venv/bin/activate && \
    uv pip install -r requirements.txt && \
    uv pip install setuptools && \
    uv pip install -e . &&\
    bash ./examples/demo.sh
ENV SEAL5_CLANG_DIR=/tmp/seal5_llvm_cli_demo/.seal5/build/release/bin/


# fetch prebuild gnu-riscv toolchain
# RUN curl -sL https://github.com/PhilippvK/riscv-tools/releases/download/gnu_2024.09.03/riscv32-unknown-elf-ubuntu-24.04-rv32gcv_ilp32d.tar.xz \
#     -o riscv32-unknown-elf-ubuntu-24.04-rv32gcv_ilp32d.tar.xz && \
#     mkdir /rv32gcv_ilp32d && \
#     tar -xvf riscv32-unknown-elf-ubuntu-24.04-rv32gcv_ilp32d.tar.xz -C /opt/riscv

# install ANTLR4 cli tool as packaged version is too old
RUN cd /usr/local/lib && \
    wget https://www.antlr.org/download/antlr-4.13.2-complete.jar && \
    echo "#!/bin/bash\njava -jar /usr/local/lib/antlr-4.13.2-complete.jar \$@" > /usr/local/bin/antlr4 && \
    chmod +x /usr/local/bin/antlr4
ENV CLASSPATH=".:/usr/local/lib/antlr-4.13.2-complete.jar"
ENV PATH="/usr/local/bin:${PATH}"
RUN antlr4


WORKDIR /app
RUN git clone --branch master https://github.com/flame/blis.git && \
    cd blis && \
    git checkout 827c50b23402ee61da40fb789432213ded6af5b2
ENV BLIS_DIR=/app/blis

COPY ./bg ./bg/

RUN mkdir -p /app/bg/build
WORKDIR /app/bg

# TODO this also rebuilds the cpp antlr4-runtime on every change in bg. try compiling lib in another build stage and installing it
RUN cmake -G Ninja -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release
RUN mv build/compile_commands.json .

# build and run generator executable
RUN cmake --build build
RUN ./build/bg

# Apply generated patches
# docker cache will lie to us
WORKDIR ${BLIS_DIR}
RUN git clean -d -f && \
    git reset --hard
ENV KERNEL_DIR=$BLIS_DIR/kernels
ENV GENERATED_KERNELS_DIR=/app/bg/build/testing_output

RUN cp -r $GENERATED_KERNELS_DIR/* $BLIS_DIR/

RUN patch -p1 frame/base/bli_arch.c < $GENERATED_KERNELS_DIR/patches/bli_arch.c.patch && \
    patch -p1 frame/base/bli_cpuid.c < $GENERATED_KERNELS_DIR/patches/bli_cpuid.c.patch && \
    patch -p1 frame/include/bli_arch_config.h < $GENERATED_KERNELS_DIR/patches/bli_arch_config.h.patch && \
    patch -p1 frame/include/bli_gentconf_macro_defs.h < $GENERATED_KERNELS_DIR/patches/bli_gentconf_macro_defs.h.patch && \
    patch -p1 frame/include/bli_type_defs.h < $GENERATED_KERNELS_DIR/patches/bli_type_defs.h.patch && \
    patch -p1 config_registry < $GENERATED_KERNELS_DIR/patches/config_registry.patch

# Congigure and build blis
RUN ./configure --disable-system --disable-threading --enable-static --disable-shared --enable-verbose-make  test_kernel && \
    make V=1 -j8


# at this point libblis.a is build in /app/blis/lib/test_kernel
RUN test -e /app/blis/lib/test_kernel/libblis.a
RUN test -e /app/blis/include/test_kernel/blis.h

WORKDIR /app
COPY test ./test
RUN cd test && \
    cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Debug

WORKDIR /app/test
# CMD  [  "cmake" ,"--build" ,"build"]
CMD [ "/bin/bash"]






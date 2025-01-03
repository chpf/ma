cmake_minimum_required(VERSION 3.20)
project(test)

set(CMAKE_C_COMPILER /tmp/seal5_llvm_cli_demo/.seal5/build/release/bin/clang)

add_executable(test main.c)

target_compile_options(test PRIVATE
    -march=rv32ixexample
    -mabi=ilp32d
    --gcc-toolchain=/rv32gcv_ilp32d
    --sysroot=/rv32gcv_ilp32d/riscv32-unknown-elf
)

target_link_options(test PRIVATE
    -march=rv32ixexample
    -mabi=ilp32d
    --gcc-toolchain=/rv32gcv_ilp32d
    --sysroot=/rv32gcv_ilp32d/riscv32-unknown-elf
)

target_include_directories(test PRIVATE
    /app/blis/include/test_kernel
)
target_link_directories(test PRIVATE
    /app/blis/lib/test_kernel
)
target_link_libraries(test PRIVATE
    blis
)


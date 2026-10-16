cmake_minimum_required(VERSION 4.2)
project(FPU_en_C C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -Wextra -Werror -O2)

add_executable(FPU_en_C
        main.c
        ieee754_rounding.c
        ieee754_alu.c)
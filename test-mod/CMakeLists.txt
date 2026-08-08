cmake_minimum_required(VERSION 3.28.3)
project(TestMod)

add_library(TestMod SHARED
        mod.c
)

target_include_directories(TestMod PRIVATE ../mod-api ../includes/tefkernel)
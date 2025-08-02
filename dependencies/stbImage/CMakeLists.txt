cmake_minimum_required(VERSION 3.16)
project(stbImage VERSION 1.0.0 LANGUAGES CXX)

add_library(stbImage STATIC)

target_sources(stbImage PRIVATE
    src/stb_image.c
    include/stb_image.h
)

target_include_directories(stbImage PUBLIC
    include
)
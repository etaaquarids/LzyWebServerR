﻿# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("LzyServer")

# 包含子项目。
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory ("LzyServer")


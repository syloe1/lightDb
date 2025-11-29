cmake_minimum_required(VERSION 3.10)
project(lightdb)  # 项目名称

# 设置 C++ 标准（你的代码用了 C++17，必须指定）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 配置头文件搜索路径（核心！告诉编译器去哪里找头文件）
include_directories(include)  # 表示 "include/" 是头文件根目录

# 收集源文件（src 下的所有 .cpp 文件，包括子目录）
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# 生成可执行文件（lightdb 是可执行文件名，SRC_FILES 是所有源文件）
add_executable(lightdb ${SRC_FILES})
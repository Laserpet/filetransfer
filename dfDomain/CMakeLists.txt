cmake_minimum_required(VERSION 2.6)
project(df_domain_simulator)

# 设置 C++ 编译器
# set(CMAKE_CXX_COMPILER "/usr/bin/gcc")
# set(CMAKE_CXX_COMPILER "/usr/bin/g++")

# 包含当前目录
include_directories(./)

# 搜索当前目录下的所有源文件
aux_source_directory(./ SRC_LIST)

# 打印源文件列表（用于调试）
message("app src:${SRC_LIST}")

# 设置编译标志
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath=./lib/")

# 定义预处理宏
#add_definitions(-DMINGW32)
#add_definitions(-g)
# add_definitions("-Wall -Werror 
                    # -Wno-format  
                    # -Wno-unused-but-set-variable 
                    # -Wno-unused-variable
                    # -Wno-unused-function
                    # -Wno-reorder")

# 定义可执行文件目标
add_executable(${PROJECT_NAME} ${SRC_LIST})

# 链接必要的库
# target_link_libraries(${PROJECT_NAME} 
                        # ws2_32
                        # winmm
                        # pthread )

target_link_libraries(${PROJECT_NAME} 
                        pthread )

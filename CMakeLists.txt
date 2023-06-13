cmake_minimum_required(VERSION 3.25)
project(EpollWebServer)

set(CMAKE_CXX_STANDARD 17)

# 指定 POCO 库的路径
#link_directories(/usr/local/lib)
link_directories(/home/hh/Downloads/poco-1.12.4-all/lib/Linux/x86_64)

## 添加Poco evnet库的链接路径
#link_directories(${POCO_PREFIX}/lib)

# 添加可执行文件
add_executable(EpollWebServer main.cpp)

## 链接 POCO 库
#target_link_libraries(EpollWebServer PocoFoundation PocoNet PocoUtil)
# 链接Poco evnet库
target_link_libraries(EpollWebServer PocoFoundation PocoNet PocoUtil PocoXML PocoJSON)



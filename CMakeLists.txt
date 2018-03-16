cmake_minimum_required(VERSION 3.9)
project(seawsgi)

set(CMAKE_CXX_COMPILER /usr/bin/g++-7)

set(CMAKE_CXX_STANDARD 17)

find_package(PythonLibs REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Protobuf 2.6.1 REQUIRED)

set(ENV{PKG_CONFIG_PATH}  "/home/zlfeng/CLionProjects/seastar/build/release")

pkg_check_modules(SEASTAR seastar)

add_compile_options(${SEASTAR_CFLAGS})

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${SEASTAR_INCLUDE_DIRS})

link_directories(${SEASTAR_LIBRARY_DIRS})
add_executable(seawsgi main.cpp wsgi.cc wsgi.hh)
target_link_libraries(seawsgi ${PYTHON_LIBRARIES} ${SEASTAR_LDFLAGS} ${PROTOBUF_LIBRARIES})


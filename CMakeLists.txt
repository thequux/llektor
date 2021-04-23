cmake_minimum_required(VERSION 3.19)
project(llektor)

set(CMAKE_CXX_STANDARD 14)

find_package(LLVM 11 REQUIRED CONFIG)
list(APPEND  CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

add_subdirectory(llektor)
add_subdirectory(llektor_stub)

#add_llvm_loadable_module(LLektor library.cpp)

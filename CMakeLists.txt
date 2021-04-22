cmake_minimum_required(VERSION 3.19)
project(llektor)

set(CMAKE_CXX_STANDARD 14)


#set(LLVM_ROOT "/usr/lib/llvm-11" CACHE PATH "Root of LLVM install")
#if (NOT EXISTS ${LLVM_ROOT}/include/llvm)
#    message(FATAL_ERROR "LLVM_ROOT (${LLVM_ROOT}) is not valid")
#endif()
#
#list(APPEND CMAKE_PREFIX_PATH "${LLVM_ROOT}/share/llvm/cmake")
#find_package(LLVM REQUIRED CONFIG)
#
#list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
#include(HandleLLVMOptions)
#include(AddLLVM)
#
#add_definitions(${LLVM_DEFINITIONS})
#include_directories(${LLVM_INCLUDE_DIRS})

#set(LLVM_RUNTIME_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INT_DIR}" )
#set(LLVM_LIBRARY_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/lib/${CMAKE_CFG_INT_DIR}" )


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
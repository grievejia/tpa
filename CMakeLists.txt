cmake_minimum_required(VERSION 2.8.8)
project(tpa)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++14 support
# for your compiler.
# Check for C++14 support and set the compilation flag
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
if(COMPILER_SUPPORTS_CXX14)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-rtti -Wall -fPIC")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
endif()

configure_file(config/source_sink.conf source_sink.conf COPYONLY)
configure_file(config/ptr_effect.conf ptr_effect.conf COPYONLY)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Specify library and binary output dir
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory (lib)
add_subdirectory (unittest)
add_subdirectory (tool)

enable_testing ()
add_test (MemoryUnitTest ${PROJECT_BINARY_DIR}/unittest/MemoryTest)
add_test (InterpreterUnitTest ${PROJECT_BINARY_DIR}/unittest/InterpreterTest)
add_test (GlobalAnalysisUnitTest ${PROJECT_BINARY_DIR}/unittest/GlobalAnalysisTest)
add_test (ControlFlowTest ${PROJECT_BINARY_DIR}/unittest/ControlFlowTest)
add_test (TaintnessTest ${PROJECT_BINARY_DIR}/unittest/TaintnessTest)
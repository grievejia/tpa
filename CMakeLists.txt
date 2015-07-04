cmake_minimum_required(VERSION 2.8.8)
project(tpa)

# Check for LLVM installation
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Check for Python3 installation
find_package(PythonInterp 3 REQUIRED)
message(STATUS "Found Python ${PYTHON_VERSION_STRING}")
message(STATUS "Using python interpreter in: ${PYTHON_EXECUTABLE}")

# Check for C++14 support and set the compilation flag
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
if(COMPILER_SUPPORTS_CXX14)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-rtti -Wall -fPIC")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
endif()

configure_file(config/ptr.config ptr.config COPYONLY)
configure_file(config/modref.config modref.config COPYONLY)
configure_file(config/taint.config taint.config COPYONLY)

# Specify library and binary output dir
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory (lib)
#add_subdirectory (unittest)
add_subdirectory (tool)

enable_testing ()
add_test (MemoryUnitTest ${PROJECT_BINARY_DIR}/unittest/MemoryTest)
add_test (InterpreterUnitTest ${PROJECT_BINARY_DIR}/unittest/InterpreterTest)
add_test (GlobalAnalysisUnitTest ${PROJECT_BINARY_DIR}/unittest/GlobalAnalysisTest)
add_test (ControlFlowTest ${PROJECT_BINARY_DIR}/unittest/ControlFlowTest)
add_test (TaintnessTest ${PROJECT_BINARY_DIR}/unittest/TaintnessTest)
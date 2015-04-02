include_directories(
  ${tpa_SOURCE_DIR}/util/googletest/include
  ${tpa_SOURCE_DIR}/include)

link_directories (${tpa_BINARY_DIR}/lib)

set (EXECUTABLE_OUTPUT_PATH ${tpa_BINARY_DIR}/unittest)
add_definitions(-DGTEST_HAS_RTTI=0)

add_executable(MemoryTest MemoryUnitTest/PtsSetTest.cpp MemoryUnitTest/MemoryTest.cpp)
target_link_libraries(MemoryTest MemoryModel LLVMAsmParser LLVMSupport LLVMCore gtest_main)

add_executable(ControlFlowTest ControlFlowUnitTest/ControlFlowTest.cpp)
target_link_libraries(ControlFlowTest PointerAnalysis LLVMAnalysis LLVMAsmParser LLVMSupport LLVMCore gtest_main)

add_executable(InterpreterTest InterpreterUnitTest/InterpreterTest.cpp)
target_link_libraries(InterpreterTest TPA LLVMAnalysis LLVMAsmParser LLVMSupport LLVMCore gtest_main)

add_executable(GlobalAnalysisTest GlobalAnalysisUnitTest/GlobalAnalysisTest.cpp)
target_link_libraries(GlobalAnalysisTest PointerAnalysis LLVMAnalysis LLVMAsmParser LLVMSupport LLVMCore gtest_main)

add_executable(TaintnessTest TaintnessUnitTest/TaintnessTest.cpp)
target_link_libraries(TaintnessTest Clients TPA LLVMAnalysis LLVMAsmParser LLVMSupport LLVMCore gtest_main)
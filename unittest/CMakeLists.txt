include_directories(
  ${tpa_SOURCE_DIR}/util/googletest/include
  ${tpa_SOURCE_DIR}/include)

link_directories (${tpa_BINARY_DIR}/lib)

set (EXECUTABLE_OUTPUT_PATH ${tpa_BINARY_DIR}/unittest)
add_definitions(-DGTEST_HAS_RTTI=0)

add_executable(MemoryTest MemoryUnitTest/ContextTest.cpp MemoryUnitTest/PointerTest.cpp MemoryUnitTest/MemoryTest.cpp MemoryUnitTest/PtsSetTest.cpp)
target_link_libraries(MemoryTest MemoryModelStatic LLVMAsmParser gtest_main)

add_executable(ControlFlowTest ControlFlowUnitTest/ControlFlowTest.cpp)
target_link_libraries(ControlFlowTest PointerAnalysisStatic LLVMAsmParser gtest_main)

add_executable(InterpreterTest InterpreterUnitTest/InterpreterTest.cpp InterpreterUnitTest/InterProcTest.cpp)
target_link_libraries(InterpreterTest TPAStatic LLVMAsmParser gtest_main)

add_executable(GlobalAnalysisTest GlobalAnalysisUnitTest/GlobalAnalysisTest.cpp)
target_link_libraries(GlobalAnalysisTest PointerAnalysisStatic LLVMAsmParser gtest_main)

add_executable(TaintnessTest TaintnessUnitTest/TaintnessTest.cpp TaintnessUnitTest/PrecisionTest.cpp)
target_link_libraries(TaintnessTest ClientsStatic LLVMAsmParser gtest_main)
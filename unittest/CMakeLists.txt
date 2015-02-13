include_directories(
  ${gtest_SOURCE_DIR}/include
  ${gtest_SOURCE_DIR}
  ${tpa_SOURCE_DIR}/include)

link_directories (${tpa_BINARY_DIR}/lib)

set (EXECUTABLE_OUTPUT_PATH ${tpa_BINARY_DIR}/unittest)

cxx_executable(MemoryTest MemoryUnitTest "MemoryModel;LLVMAsmParser;LLVMAnalysis;LLVMSupport;LLVMCore;gtest_main;")
cxx_executable(ControlFlowTest ControlFlowUnitTest "PointerAnalysis;LLVMAsmParser;LLVMAnalysis;LLVMSupport;LLVMCore;gtest_main;")
cxx_executable(InterpreterTest InterpreterUnitTest "TPA;LLVMAsmParser;LLVMAnalysis;LLVMSupport;LLVMCore;gtest_main;")
cxx_executable(GlobalAnalysisTest GlobalAnalysisUnitTest "MemoryModel;LLVMAsmParser;LLVMAnalysis;LLVMSupport;LLVMCore;gtest_main;")
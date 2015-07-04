include_directories (${PROJECT_SOURCE_DIR}/tool/pts-verify)

set (ptsVerifySourceCode
	pts-verify.cpp
	CommandLineOptions.cpp
	RunAnalysis.cpp
)

add_executable (pts-verify ${ptsVerifySourceCode})
target_link_libraries (pts-verify DynamicAnalysis Instrument Util PointerAnalysis)
include_directories (${PROJECT_SOURCE_DIR}/tool/vkcfa-taint)

set (vkcfaSourceCode
	vkcfa-taint.cpp
	CommandLineOptions.cpp
	RunAnalysis.cpp
)

add_executable (vkcfa-taint ${vkcfaSourceCode})
target_link_libraries (vkcfa-taint Util Transforms TaintAnalysis)
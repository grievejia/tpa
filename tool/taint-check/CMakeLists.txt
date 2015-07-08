include_directories (${PROJECT_SOURCE_DIR}/tool/taint-check)

set (taintCheckSourceCode
	taint-check.cpp
	CommandLineOptions.cpp
	RunAnalysis.cpp
)

add_executable (taint-check ${taintCheckSourceCode})
target_link_libraries (taint-check Util Transforms TaintAnalysis)
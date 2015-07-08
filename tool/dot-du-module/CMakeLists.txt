include_directories (${PROJECT_SOURCE_DIR}/tool/dot-du-module)

set (dotDefUseModuleCode
	CommandLineOptions.cpp
	dot-du-module.cpp
	RunAnalysis.cpp
)

add_executable (dot-du-module ${dotDefUseModuleCode})
target_link_libraries (dot-du-module Util Transforms TaintAnalysis)
include_directories (${PROJECT_SOURCE_DIR}/tool/dot-pointer-cfg)

set (dotPointerCFGCode
	CommandLineOptions.cpp
	dot-pointer-cfg.cpp
	RunAnalysis.cpp
)

add_executable (dot-pointer-cfg ${dotPointerCFGCode})
target_link_libraries (dot-pointer-cfg Util Transforms PointerAnalysis)
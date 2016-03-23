include_directories (${PROJECT_SOURCE_DIR}/tool/pts-dump)

set (ptsDumpSourceCode
	pts-dump.cpp
	CommandLineOptions.cpp
	RunAnalysis.cpp
)

add_executable (pts-dump ${ptsDumpSourceCode})
target_link_libraries (pts-dump Util Transforms PointerAnalysis)
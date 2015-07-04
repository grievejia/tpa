include_directories (${PROJECT_SOURCE_DIR}/tool/global-pts)

set (llvmGlobalPtsSourceCode
	CommandLineOptions.cpp
	global-pts.cpp
	RunAnalysis.cpp
)

add_executable (global-pts ${llvmGlobalPtsSourceCode})
target_link_libraries (global-pts Util Transforms PointerAnalysis)
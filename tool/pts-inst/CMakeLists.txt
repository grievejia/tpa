include_directories (${PROJECT_SOURCE_DIR}/tool/pts-inst)

set (ptsInstSourceCode
	pts-inst.cpp
	CommandLineOptions.cpp
)

add_executable (pts-inst ${ptsInstSourceCode})
target_link_libraries (pts-inst Instrument Util Transforms)
include_directories (${PROJECT_SOURCE_DIR}/tool/pts-log-dump)

set (ptsLogDumpSourceCode
	pts-log-dump.cpp
)

add_executable (pts-log-dump ${ptsLogDumpSourceCode})
target_link_libraries (pts-log-dump DynamicLog Util)
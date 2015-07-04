include_directories (${tpa_SOURCE_DIR}/tool/table-check)

set (tableCheckSourceCode
	table-check.cpp
	TableChecker.cpp
)

add_executable (table-check ${tableCheckSourceCode})
target_link_libraries (table-check Util Annotation)
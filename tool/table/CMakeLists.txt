include_directories (${tpa_SOURCE_DIR}/tool/table)

set (tableSourceCode
	table.cpp
	TableManager/TableManager.cpp
)

add_executable (table ${tableSourceCode})
target_link_libraries (table ClientsStatic)
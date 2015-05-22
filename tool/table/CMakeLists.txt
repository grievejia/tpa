include_directories (${tpa_SOURCE_DIR}/tool/table)

set (tableSourceCode
	table.cpp
	TableManager/TableManager.cpp
	TableManager/Validator/ModRefValidator.cpp
	TableManager/Validator/PointerValidator.cpp
	TableManager/Validator/TaintValidator.cpp
)

add_executable (table ${tableSourceCode})
target_link_libraries (table ClientsStatic)
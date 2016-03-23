include_directories (${tpa_SOURCE_DIR}/tool/table)

set (tableSourceCode
	CommandLineOptions.cpp
	ExternalModRefTablePrinter.cpp
	ExternalPointerTablePrinter.cpp
	ExternalTaintTablePrinter.cpp
	table.cpp
)

add_executable (table ${tableSourceCode})
target_link_libraries (table Util Annotation)
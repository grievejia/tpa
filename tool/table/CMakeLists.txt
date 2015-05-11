include_directories (${tpa_SOURCE_DIR}/tool/table)

set (tableSourceCode
	table.cpp
	Pointer/PointerEffectLister.cpp
)

add_executable (table ${tableSourceCode})
target_link_libraries (table PointerAnalysisStatic)

set (PointerControlFlowSourceCodes
	ExternalPointerEffectTable.cpp
	PointerCFG.cpp
	PointerProgramBuilder.cpp
	SemiSparseProgramBuilder.cpp
	WriteDotFile.cpp
)
add_library (PointerControlFlow SHARED ${PointerControlFlowSourceCodes})
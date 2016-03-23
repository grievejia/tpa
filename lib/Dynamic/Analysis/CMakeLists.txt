set (DynamicAnalysisSourceCodes
	DynamicContext.cpp
	DynamicMemory.cpp
	DynamicPointerAnalysis.cpp
)
add_library (DynamicAnalysis STATIC ${DynamicAnalysisSourceCodes})
target_link_libraries (DynamicAnalysis DynamicLog)
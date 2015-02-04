
set (AndersenSourceCodes
	Analysis/AndersenAnalysis.cpp
)
add_library (Andersen SHARED ${AndersenSourceCodes})
target_link_libraries (Andersen MemoryModel)
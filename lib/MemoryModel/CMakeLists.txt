
set (MemoryModelSourceCodes
	Analysis/GlobalPointerAnalysis.cpp
	Manager/MemoryManager.cpp
	Manager/PointerManager.cpp
	Precision/Context.cpp
	Precision/KLimitContext.cpp
	PrettyPrint.cpp
)
add_library (MemoryModel SHARED ${MemoryModelSourceCodes})
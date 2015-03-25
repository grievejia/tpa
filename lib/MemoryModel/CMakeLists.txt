
set (MemoryModelSourceCodes
	Manager/MemoryManager.cpp
	Manager/PointerManager.cpp
	Precision/AdaptiveContext.cpp
	Precision/Context.cpp
	Precision/KLimitContext.cpp
	PrettyPrint.cpp
)
add_library (MemoryModel SHARED ${MemoryModelSourceCodes})
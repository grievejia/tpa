
set (MemoryModelSourceCodes
	Manager/MemoryManager.cpp
	Manager/PointerManager.cpp
	Precision/AdaptiveContext.cpp
	Precision/Context.cpp
	Precision/KLimitContext.cpp
	PtsSet/PtsSet.cpp
	PrettyPrint.cpp
)
add_library (MemoryModelObj OBJECT ${MemoryModelSourceCodes})
add_library (MemoryModel SHARED $<TARGET_OBJECTS:MemoryModelObj>)
add_library (MemoryModelStatic STATIC $<TARGET_OBJECTS:MemoryModelObj>)

target_link_libraries(MemoryModelStatic LLVMSupport LLVMCore)
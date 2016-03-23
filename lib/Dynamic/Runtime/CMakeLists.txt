set (DynamicRuntimeSourceCodes
	MemoryHooks.c
)

add_library (DynamicRuntime STATIC ${DynamicRuntimeSourceCodes})
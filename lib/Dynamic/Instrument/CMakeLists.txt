set (InstrumentersSourceCodes
	DynamicHooks.cpp
	FeatureCheck.cpp
	IDAssigner.cpp
	MemoryInstrument.cpp
)
add_library (Instrument STATIC ${InstrumentersSourceCodes})
target_link_libraries (Instrument LLVMCore Annotation)
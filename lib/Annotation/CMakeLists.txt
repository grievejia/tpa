set (AnnotationSourceCodes
	ExternalModRefTable.cpp
	ExternalPointerTable.cpp
	ExternalTaintTable.cpp
)
add_library (Annotation STATIC ${AnnotationSourceCodes})

target_link_libraries (Annotation LLVMSupport)
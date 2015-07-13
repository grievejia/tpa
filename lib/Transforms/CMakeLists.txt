include_directories (${tpa_SOURCE_DIR}/include/Transforms)

set (TransformSourceCodes
	ExpandByVal.cpp
	ExpandConstantExpr.cpp
	ExpandGetElementPtr.cpp
	ExpandIndirectBr.cpp
	ExpandUtils.cpp
	FoldIntToPtr.cpp
	GlobalCleanup.cpp
	LowerGlobalCtor.cpp
	RunPrepass.cpp
)

add_library (Transforms STATIC ${TransformSourceCodes})
target_link_libraries (Transforms LLVMTransformUtils LLVMipo LLVMSupport LLVMCore)

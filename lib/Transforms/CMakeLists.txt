include_directories (${tpa_SOURCE_DIR}/include/Transforms)

set (TransformSourceCodes
	ExpandByVal.cpp
	ExpandConstantExpr.cpp
	ExpandGetElementPtr.cpp
	ExpandIndirectBr.cpp
	ExpandUtils.cpp
	GlobalCleanup.cpp
)

add_library (TransformObj OBJECT ${TransformSourceCodes})
add_library (Transform SHARED $<TARGET_OBJECTS:TransformObj>)
add_library (TransformStatic STATIC $<TARGET_OBJECTS:TransformObj>)


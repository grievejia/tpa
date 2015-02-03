include_directories (${tpa_SOURCE_DIR}/include/Transforms)

set (TransformSourceCodes ExpandByVal.cpp ExpandConstantExpr.cpp ExpandGetElementPtr.cpp ExpandIndirectBr.cpp ExpandUtils.cpp GlobalCleanup.cpp)
add_library (Transform SHARED ${TransformSourceCodes})


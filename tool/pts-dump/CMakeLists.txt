include_directories (${tpa_SOURCE_DIR}/tool/pts-dump)

set (ptsdumpSourceCode
	pts-dump.cpp
	PtsDumpPass.cpp
)

add_executable (pts-dump ${ptsdumpSourceCode})
target_link_libraries (pts-dump TPAStatic TransformStatic LLVMIRReader LLVMAsmParser LLVMTransformUtils LLVMipo LLVMCore)
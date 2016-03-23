set (UtilCodes
	CommandLine/CommandLineParser.cpp
	CommandLine/TypedCommandLineParser.cpp
	IO/ReadIR.cpp
	IO/WriteIR.cpp
)

add_library (Util STATIC ${UtilCodes})
target_link_libraries (Util LLVMIRReader LLVMAsmParser LLVMBitWriter LLVMSupport LLVMCore)

set (ClientSourceCodes
	Taintness/SourceSink.cpp
	Taintness/TaintAnalysis.cpp
	Taintness/TaintAnalysisPass.cpp
	Taintness/TaintTransferFunction.cpp
	Util/ClientWorkList.cpp
)
add_library (Clients SHARED ${ClientSourceCodes})
target_link_libraries (Clients TPA)
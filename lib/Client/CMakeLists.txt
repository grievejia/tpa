
set (ClientSourceCodes
	Taintness/AdaptiveTaintAnalysisPass.cpp
	Taintness/SourceSink.cpp
	Taintness/TaintAnalysis.cpp
	Taintness/TaintAnalysisPass.cpp
	Taintness/TaintPrecisionMonitor.cpp
	Taintness/TaintTransferFunction.cpp
)
add_library (Clients SHARED ${ClientSourceCodes})
target_link_libraries (Clients TPA)
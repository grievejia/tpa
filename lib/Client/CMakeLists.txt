
set (ClientSourceCodes
	Taintness/Analysis/TaintAnalysis.cpp
	Taintness/DataFlow/SourceSink.cpp
	Taintness/DataFlow/TaintPrecisionMonitor.cpp
	Taintness/DataFlow/TaintTransferFunction.cpp
	Taintness/Pass/TaintAnalysisPass.cpp
)
add_library (Clients SHARED ${ClientSourceCodes})
target_link_libraries (Clients TPA)
#ifndef TPA_DATAFLOW_ANALYSIS_ENGINE_H
#define TPA_DATAFLOW_ANALYSIS_ENGINE_H

namespace tpa
{

template <typename WorkListType>
class DataFlowAnalysisEngine
{
private:
	using MemoType = Memo<WorkListType::ElemType>;
protected:
	WorkListType globalWorkList;
	MemoType& memo;

	virtual void initalizeWorkList() = 0;
public:
	DataFlowAnalysisEngine(MemoType& m): memo(m) {}
	virtual ~DataFlowAnalysisEngine() {}
};

}

#endif

#pragma once

#include "Dynamic/Analysis/DynamicMemory.h"
#include "Dynamic/Log/LogProcessor.h"

namespace dynamic
{

class DynamicContext;

class DynamicPointerAnalysis: public LogProcessor<DynamicPointerAnalysis>
{
private:
	using MapType = std::unordered_map<DynamicPointer, DynamicMemoryObject>;
	MapType ptsMap;

	DynamicMemory memory;
	const DynamicContext* currCtx;
	unsigned callerId;

	void setPointsTo(const DynamicPointer&, const DynamicMemoryObject&);
public:
	using const_iterator = MapType::const_iterator;

	DynamicPointerAnalysis(const char* fileName);

	void visitAllocRecord(const AllocRecord&);
	void visitPointerRecord(const PointerRecord&);
	void visitEnterRecord(const EnterRecord&);
	void visitExitRecord(const ExitRecord&);
	void visitCallRecord(const CallRecord&);

	const_iterator begin() const { return ptsMap.begin(); }
	const_iterator end() const { return ptsMap.end(); }
};

}

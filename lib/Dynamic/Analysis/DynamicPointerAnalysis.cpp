#include "Dynamic/Analysis/DynamicContext.h"
#include "Dynamic/Analysis/DynamicPointerAnalysis.h"

#include <cassert>

namespace dynamic
{

DynamicPointerAnalysis::DynamicPointerAnalysis(const char* fileName): LogProcessor(fileName), currCtx(DynamicContext::getGlobalContext()), callerId(0) {}

void DynamicPointerAnalysis::setPointsTo(const DynamicPointer& ptr, const DynamicMemoryObject& obj)
{
	auto itr = ptsMap.find(ptr);
	if (itr == ptsMap.end())
		ptsMap.insert(std::make_pair(ptr, obj));
	else
		itr->second = obj;
}

void DynamicPointerAnalysis::visitAllocRecord(const AllocRecord& allocRecord)
{
	auto ptr = DynamicPointer(currCtx, allocRecord.id);
	auto obj = memory.allocate(static_cast<AllocType>(allocRecord.type), ptr, allocRecord.address);
	setPointsTo(ptr, obj);
}

void DynamicPointerAnalysis::visitPointerRecord(const PointerRecord& ptrRecord)
{
	auto ptr = DynamicPointer(currCtx, ptrRecord.id);
	auto obj = memory.getMemoryObject(ptrRecord.address);
	setPointsTo(ptr, obj);
}

void DynamicPointerAnalysis::visitEnterRecord(const EnterRecord& enterRecord)
{
	assert(callerId != 0);
	currCtx = DynamicContext::pushContext(currCtx, callerId);
	callerId = 0;
}

void DynamicPointerAnalysis::visitExitRecord(const ExitRecord& exitRecord)
{
	memory.deallocateStack(currCtx);
	currCtx = DynamicContext::popContext(currCtx);
}

void DynamicPointerAnalysis::visitCallRecord(const CallRecord& callRecord)
{
	callerId = callRecord.id;
}


}
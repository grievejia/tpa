#include "Context/Context.h"
#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/Initializer.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"
#include "PointerAnalysis/Program/SemiSparseProgram.h"
#include "PointerAnalysis/Support/Memo.h"

namespace tpa
{

WorkList Initializer::runOnInitState(Store&& initStore)
{
	WorkList workList;

	auto entryCtx = context::Context::getGlobalContext();
	auto entryCFG = globalState.getSemiSparseProgram().getEntryCFG();
	assert(entryCFG != nullptr);
	auto entryNode = entryCFG->getEntryNode();

	// Set up argv
	auto& entryFunc = entryCFG->getFunction();
	if (entryFunc.arg_size() > 0)
	{
		auto argvValue = ++entryFunc.arg_begin();
		auto argvPtr = globalState.getPointerManager().getOrCreatePointer(entryCtx, argvValue);
		auto argvObj = globalState.getMemoryManager().allocateArgv(argvValue);
		globalState.getEnv().insert(argvPtr, argvObj);
		initStore.insert(argvObj, argvObj);
	}

	auto pp = ProgramPoint(entryCtx, entryNode);
	memo.update(pp, std::move(initStore));
	workList.enqueue(pp);

	return workList;
}

}

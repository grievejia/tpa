#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/ControlFlow/StaticCallGraph.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/SemiSparseGlobalState.h"
#include "TPA/DataFlow/TransferFunction.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

class InterProcTest: public ::testing::Test
{
protected:
	std::unique_ptr<Module> testModule = parseAssembly(
		"@g1 = common global i32 1, align 8\n"
		"@g2 = common global i32* @g1, align 8\n"
		"@g3 = common global {i32*, i32**} {i32* @g1, i32** @g2}, align 8\n"
		"@g4 = common global {i32*, i32**} {i32* @g1, i32** @g2}, align 8\n"
		"define i32 @f() {\n"
		"bb:\n"
		"  %x = alloca i64, align 8\n"
		"  %y = alloca i64*, align 8\n"
		"  %z = alloca i64**, align 8\n"
		"  %w = call i64*** @g(i64* %x, i64** %y, i64*** %z)\n"
		"  %g3cast = bitcast {i32*, i32**}* @g3 to i8*\n"
		"  %r = call i8* @memset(i8* %g3cast, i32 0, i64 16)\n"
		"  ret i32 0\n"
		"}\n"
		"define i64*** @g(i64* %p0, i64** %p1, i64*** %p2) {\n"
		"bb:\n"
		"  ret i64*** %p2\n"
		"}\n"
		"declare i32 @getchar()\n"
		"declare i8* @malloc(i64)\n"
		"declare i8* @gets(i8*)\n"
		"declare double @strtod(i8*, i8**)\n"
		"declare i8* @memcpy(i8*, i8*, i64)\n"
		"declare i8* @memset(i8*, i32, i64)\n"
	);
	DataLayout dataLayout = DataLayout(testModule.get());

	PointerManager ptrManager;
	MemoryManager memManager = MemoryManager(dataLayout);
	const Context* globalCtx = Context::getGlobalContext();

	PointerProgram prog;
	const Function* f = testModule->getFunction("f");
	const Function* g = testModule->getFunction("g");
	PointerCFG* fCfg = prog.createPointerCFGForFunction(f);
	PointerCFG* gCfg = prog.createPointerCFGForFunction(g);

	StaticCallGraph callGraph;
	ExternalPointerTable extTable;
	Env env;
	SemiSparseGlobalState globalState = SemiSparseGlobalState(ptrManager, memManager, prog, callGraph, env, extTable);

	Store store;
	TransferFunction transferFunction = TransferFunction(globalCtx, store, globalState);

	const Instruction *x, *y, *z, *w, *gret, *rret;
	const MemoryLocation *xLoc, *yLoc, *zLoc;

	void SetUp() override
	{
		auto itr = testModule->getFunction("f")->begin()->begin();
		x = itr++;
		y = itr++;
		z = itr++;
		w = itr++;
		rret = ++itr;

		gret = testModule->getFunction("g")->begin()->begin();

		auto xObj = memManager.allocateMemory(ProgramLocation(globalCtx, x), x->getType()->getPointerElementType());
		auto yObj = memManager.allocateMemory(ProgramLocation(globalCtx, y), y->getType()->getPointerElementType());
		auto zObj = memManager.allocateMemory(ProgramLocation(globalCtx, z), z->getType()->getPointerElementType());
		xLoc = memManager.offsetMemory(xObj, 0);
		yLoc = memManager.offsetMemory(yObj, 0);
		zLoc = memManager.offsetMemory(zObj, 0);

		extTable = ExternalPointerTable::loadFromFile("ptr_effect.conf");
	}
};

TEST_F(InterProcTest, CallTest)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto ptrZ = ptrManager.getOrCreatePointer(globalCtx, z);
	env.insert(ptrX, xLoc);
	env.insert(ptrY, yLoc);
	env.insert(ptrZ, zLoc);
	store.insert(xLoc, yLoc);
	store.insert(xLoc, zLoc);

	auto paramItr = g->arg_begin();
	auto param0 = ptrManager.getOrCreatePointer(globalCtx, paramItr++);
	auto param1 = ptrManager.getOrCreatePointer(globalCtx, paramItr++);
	auto param2 = ptrManager.getOrCreatePointer(globalCtx, paramItr++);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	callNode->addArgument(x);
	callNode->addArgument(y);
	callNode->addArgument(z);
	auto status = transferFunction.evalCallArguments(callNode, globalCtx, g);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());

	EXPECT_EQ(env.lookup(param0).getSize(), 1u);
	EXPECT_TRUE(env.lookup(param0).has(xLoc));
	EXPECT_EQ(env.lookup(param1).getSize(), 1u);
	EXPECT_TRUE(env.lookup(param1).has(yLoc));
	EXPECT_EQ(env.lookup(param2).getSize(), 1u);
	EXPECT_TRUE(env.lookup(param2).has(zLoc));
}

TEST_F(InterProcTest, ReturnTest)
{
	auto retVal = gret->getOperand(0);
	auto ptrRet = ptrManager.getOrCreatePointer(globalCtx, retVal);
	auto ptrW = ptrManager.getOrCreatePointer(globalCtx, w);
	env.insert(ptrRet, xLoc);
	env.insert(ptrRet, yLoc);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	auto retNode = gCfg->create<ReturnNode>(gret, retVal);
	auto status = transferFunction.evalReturnValue(retNode, globalCtx, callNode);

	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());

	EXPECT_EQ(env.lookup(ptrW).getSize(), 2u);
	EXPECT_TRUE(env.lookup(ptrW).has(xLoc));
	EXPECT_TRUE(env.lookup(ptrW).has(yLoc));
}

TEST_F(InterProcTest, ExternalNoEffectTest)
{
	EXPECT_EQ(env.getSize(), 0u);
	EXPECT_EQ(store.getSize(), 0u);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	auto status = transferFunction.evalExternalCall(callNode, testModule->getFunction("getchar"));

	EXPECT_TRUE(status.isValid());
	EXPECT_FALSE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());

	EXPECT_EQ(env.getSize(), 0u);
	EXPECT_EQ(store.getSize(), 0u);	
}

TEST_F(InterProcTest, ExternalMallocTest)
{
	auto ptrW = ptrManager.getOrCreatePointer(globalCtx, w);
	env.insert(ptrW, xLoc);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	auto status = transferFunction.evalExternalCall(callNode, testModule->getFunction("malloc"));

	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());

	EXPECT_EQ(env.lookup(ptrW).getSize(), 1u);
	const MemoryLocation* loc = *env.begin()->second.begin();
	EXPECT_EQ(loc->getOffset(), 0u);
	EXPECT_TRUE(loc->isSummaryLocation());
	EXPECT_TRUE(loc->getMemoryObject()->isHeapObject());
}

TEST_F(InterProcTest, ExternalRetArg0Test)
{
	auto ptrW = ptrManager.getOrCreatePointer(globalCtx, w);
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	env.insert(ptrX, xLoc);
	env.insert(ptrX, yLoc);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	callNode->addArgument(x);
	auto status = transferFunction.evalExternalCall(callNode, testModule->getFunction("gets"));

	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());

	EXPECT_EQ(env.lookup(ptrW), env.lookup(ptrX));
}

TEST_F(InterProcTest, ExternalStoreTest)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrX, xLoc);
	env.insert(ptrY, yLoc);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	callNode->addArgument(x);
	callNode->addArgument(y);
	auto status = transferFunction.evalExternalCall(callNode, testModule->getFunction("strtod"));

	EXPECT_TRUE(status.isValid());
	EXPECT_FALSE(status.hasEnvChanged());
	EXPECT_TRUE(status.hasStoreChanged());

	EXPECT_EQ(store.lookup(yLoc).getSize(), 1u);
	EXPECT_TRUE(store.lookup(yLoc).has(xLoc));
}

TEST_F(InterProcTest, ExternalMemcpyTest)
{
	auto g3 = testModule->getGlobalVariable("g3");
	auto g3Obj = memManager.allocateMemory(ProgramLocation(globalCtx, g3), g3->getType()->getPointerElementType());
	auto g3Loc0 = memManager.offsetMemory(g3Obj, 0);
	auto g3Loc1 = memManager.offsetMemory(g3Obj, 8);

	auto g4 = testModule->getGlobalVariable("g4");
	auto g4Obj = memManager.allocateMemory(ProgramLocation(globalCtx, g4), g4->getType()->getPointerElementType());
	auto g4Loc0 = memManager.offsetMemory(g4Obj, 0);
	auto g4Loc1 = memManager.offsetMemory(g4Obj, 8);

	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto ptrW = ptrManager.getOrCreatePointer(globalCtx, w);

	store.insert(g3Loc0, xLoc);
	store.insert(g3Loc1, yLoc);
	store.insert(g3Loc1, zLoc);
	env.insert(ptrY, g3Loc0);
	env.insert(ptrX, g4Loc0);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	callNode->addArgument(y);
	callNode->addArgument(x);
	auto status = transferFunction.evalExternalCall(callNode, testModule->getFunction("memcpy"));

	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_TRUE(status.hasStoreChanged());

	EXPECT_EQ(store.lookup(g4Loc0), store.lookup(g3Loc0));
	EXPECT_EQ(store.lookup(g4Loc1), store.lookup(g3Loc1));
	EXPECT_EQ(env.lookup(ptrW), env.lookup(ptrX));
}

TEST_F(InterProcTest, ExternalMemcpyTest2)
{
	auto g3 = testModule->getGlobalVariable("g3");
	auto g3Obj = memManager.allocateMemory(ProgramLocation(globalCtx, g3), g3->getType()->getPointerElementType());
	auto g3Loc0 = memManager.offsetMemory(g3Obj, 0);
	auto g3Loc1 = memManager.offsetMemory(g3Obj, 8);

	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto ptrW = ptrManager.getOrCreatePointer(globalCtx, w);

	store.insert(g3Loc0, xLoc);
	store.insert(g3Loc1, yLoc);
	store.insert(g3Loc1, zLoc);
	env.insert(ptrY, g3Loc0);
	env.insert(ptrX, yLoc);

	auto callNode = fCfg->create<CallNode>(w, g, w);
	callNode->addArgument(y);
	callNode->addArgument(x);
	auto status = transferFunction.evalExternalCall(callNode, testModule->getFunction("memcpy"));

	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_TRUE(status.hasStoreChanged());

	EXPECT_EQ(store.lookup(yLoc), store.lookup(g3Loc0));
	EXPECT_TRUE(store.lookup(memManager.getUniversalLocation()).isEmpty());
	EXPECT_EQ(env.lookup(ptrW), env.lookup(ptrX));
}

TEST_F(InterProcTest, ExternalMemsetTest)
{
	auto g3 = testModule->getGlobalVariable("g3");
	auto ptrG3 = ptrManager.getOrCreatePointer(globalCtx, g3);
	auto retPtr = ptrManager.getOrCreatePointer(globalCtx, g3);
	auto g3Obj = memManager.allocateMemory(ProgramLocation(globalCtx, g3), g3->getType()->getPointerElementType());
	auto g3Loc0 = memManager.offsetMemory(g3Obj, 0);

	env.insert(ptrG3, g3Loc0);

	auto callNode = fCfg->create<CallNode>(rret, g, rret);
	callNode->addArgument(g3);
	auto status = transferFunction.evalExternalCall(callNode, testModule->getFunction("memset"));

	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_TRUE(status.hasStoreChanged());

	EXPECT_EQ(env.lookup(retPtr), env.lookup(ptrG3));
	auto g3Loc1 = memManager.offsetMemory(g3Obj, 8);
	EXPECT_EQ(store.lookup(g3Loc0).getSize(), 1u);
	EXPECT_TRUE(store.lookup(g3Loc0).has(memManager.getNullLocation()));
	EXPECT_EQ(store.lookup(g3Loc1).getSize(), 1u);
	EXPECT_TRUE(store.lookup(g3Loc1).has(memManager.getNullLocation()));
}

}
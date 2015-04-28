#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/ControlFlow/StaticCallGraph.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/SemiSparseGlobalState.h"
#include "TPA/DataFlow/TransferFunction.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

class InterpreterTest: public ::testing::Test
{
protected:
	std::unique_ptr<Module> testModule = parseAssembly(
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca { i32, i32 }, align 4\n"
		"  %y = alloca { i32*, i32*, i32* }, align 4\n"
		"  %z = alloca [10 x i32], align 4\n"
		"  ret i32 0\n"
		"}\n"
	);
	DataLayout dataLayout = DataLayout(testModule.get());

	PointerManager ptrManager;
	MemoryManager memManager = MemoryManager(dataLayout);
	const Context* globalCtx = Context::getGlobalContext();

	PointerProgram prog;
	PointerCFG* mainCfg = prog.createPointerCFGForFunction(testModule->getFunction("main"));

	StaticCallGraph callGraph;
	ExternalPointerEffectTable extTable;
	Env env;
	SemiSparseGlobalState globalState = SemiSparseGlobalState(ptrManager, memManager, prog, callGraph, env, extTable);

	Store store;
	TransferFunction transferFunction = TransferFunction(globalCtx, store, globalState);

	const Instruction *x, *y, *z;
	const MemoryLocation *xLoc, *yLoc, *zLoc;

	void SetUp() override
	{
		auto itr = testModule->begin()->begin()->begin();
		x = itr++;
		y = itr++;
		z = itr++;

		auto xObj = memManager.allocateMemory(ProgramLocation(globalCtx, x), x->getType()->getPointerElementType());
		auto yObj = memManager.allocateMemory(ProgramLocation(globalCtx, y), y->getType()->getPointerElementType());
		auto zObj = memManager.allocateMemory(ProgramLocation(globalCtx, z), z->getType()->getPointerElementType());
		xLoc = memManager.offsetMemory(xObj, 0);
		yLoc = memManager.offsetMemory(yObj, 0);
		zLoc = memManager.offsetMemory(zObj, 0);
	}
};

TEST_F(InterpreterTest, MemoTest)
{
	ASSERT_TRUE(testModule.get() != nullptr);

	unsigned n0 = 0, n1 = 1;
	Memo<unsigned> testMemo;

	EXPECT_FALSE(testMemo.hasMemoState(globalCtx, &n0));

	auto set0 = PtsSet::getSingletonSet(xLoc);
	testMemo.updateMemo(globalCtx, &n0, memManager.getUniversalLocation(), set0);
	ASSERT_TRUE(testMemo.hasMemoState(globalCtx, &n0));
	EXPECT_EQ(testMemo.lookup(globalCtx, &n0)->getSize(), 1u);
	EXPECT_EQ(testMemo.lookup(globalCtx, &n0)->lookup(memManager.getUniversalLocation()), set0);

	auto store = Store();
	store.insert(memManager.getUniversalLocation(), yLoc);
	testMemo.updateMemo(globalCtx, &n0, store);
	EXPECT_EQ(testMemo.lookup(globalCtx, &n0)->getSize(), 1u);
	auto set1 = set0.insert(yLoc);
	EXPECT_EQ(testMemo.lookup(globalCtx, &n0)->lookup(memManager.getUniversalLocation()), set1);

	auto set2 = PtsSet::getSingletonSet(yLoc);
	testMemo.updateMemo(globalCtx, &n1, xLoc, set2);
	ASSERT_TRUE(testMemo.hasMemoState(globalCtx, &n1));
	EXPECT_EQ(testMemo.lookup(globalCtx, &n1)->getSize(), 1u);
	EXPECT_EQ(testMemo.lookup(globalCtx, &n1)->lookup(xLoc), set2);
}

TEST_F(InterpreterTest, TransferAllocTest)
{
	// Test alloca
	auto allocNode = mainCfg->create<AllocNode>(x);
	auto status = transferFunction.evalAllocNode(allocNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_TRUE(env.lookup(ptrManager.getPointer(globalCtx, x)).has(xLoc));
}

TEST_F(InterpreterTest, TransferCopyTest1)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrX, xLoc);

	// Test copy - assignment
	auto copyNode = mainCfg->create<CopyNode>(y, x, 0, false);
	auto status = transferFunction.evalCopyNode(copyNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_EQ(env.lookup(ptrX), env.lookup(ptrY));
}

TEST_F(InterpreterTest, TransferCopyTest2)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrX, xLoc);
	env.insert(ptrY, yLoc);

	// Test copy - strong update
	auto copyNode = mainCfg->create<CopyNode>(y, x, 0, false);
	auto status = transferFunction.evalCopyNode(copyNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_EQ(env.lookup(ptrX), env.lookup(ptrY));
}

TEST_F(InterpreterTest, TransferCopyTest3)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrX, xLoc);
	auto offset = 4u;
	auto xLoc2 = memManager.offsetMemory(xLoc, offset);

	// Test copy - offset inbound
	auto copyNode = mainCfg->create<CopyNode>(y, x, offset, false);
	auto status = transferFunction.evalCopyNode(copyNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_EQ(env.lookup(ptrY).getSize(), 1u);
	EXPECT_TRUE(env.lookup(ptrY).has(xLoc2));
}

TEST_F(InterpreterTest, TransferCopyTest4)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrX, xLoc);

	// Test copy - offset out of bound
	auto copyNode = mainCfg->create<CopyNode>(y, x, 8, false);
	auto status = transferFunction.evalCopyNode(copyNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_TRUE(env.lookup(ptrY).has(memManager.getUniversalLocation()));
}

TEST_F(InterpreterTest, TransferCopyTest5)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrY, yLoc);

	// Test copy - array copy
	auto copyNode = mainCfg->create<CopyNode>(x, y, 4, true);
	auto status = transferFunction.evalCopyNode(copyNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_EQ(env.lookup(ptrX).getSize(), 6u);
	EXPECT_TRUE(env.lookup(ptrX).has(yLoc));
	EXPECT_TRUE(env.lookup(ptrX).has(memManager.offsetMemory(yLoc, 4)));
	EXPECT_TRUE(env.lookup(ptrX).has(memManager.offsetMemory(yLoc, 8)));
	EXPECT_TRUE(env.lookup(ptrX).has(memManager.offsetMemory(yLoc, 12)));
	EXPECT_TRUE(env.lookup(ptrX).has(memManager.offsetMemory(yLoc, 16)));
	EXPECT_TRUE(env.lookup(ptrX).has(memManager.offsetMemory(yLoc, 20)));
}

TEST_F(InterpreterTest, TransferCopyTest6)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto ptrZ = ptrManager.getOrCreatePointer(globalCtx, z);
	auto yLoc2 = memManager.offsetMemory(yLoc, 8);
	env.insert(ptrX, xLoc);
	env.insert(ptrY, yLoc);
	env.insert(ptrY, yLoc2);

	// Test copy - multiple sources
	auto copyNode = mainCfg->create<CopyNode>(z);
	copyNode->addSrc(x);
	copyNode->addSrc(y);
	auto status = transferFunction.evalCopyNode(copyNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_EQ(env.lookup(ptrZ).getSize(), 3u);
	EXPECT_TRUE(env.lookup(ptrZ).has(xLoc));
	EXPECT_TRUE(env.lookup(ptrZ).has(yLoc));
	EXPECT_TRUE(env.lookup(ptrZ).has(yLoc2));
}

TEST_F(InterpreterTest, TransferCopyTest7)
{
	ptrManager.getOrCreatePointer(globalCtx, x);

	// Test copy - failure
	auto copyNode = mainCfg->create<CopyNode>(y, x, 0, false);
	auto status = transferFunction.evalCopyNode(copyNode);
	EXPECT_FALSE(status.isValid());
}

TEST_F(InterpreterTest, TransferLoadTest1)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrX, xLoc);
	env.insert(ptrY, yLoc);
	store.insert(xLoc, zLoc);

	// Test load - single src, strong update
	auto loadNode = mainCfg->create<LoadNode>(y, x);
	auto status = transferFunction.evalLoadNode(loadNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_EQ(env.lookup(ptrY).getSize(), 1u);
	EXPECT_TRUE(env.lookup(ptrY).has(zLoc));
}

TEST_F(InterpreterTest, TransferLoadTest2)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto xLoc2 = memManager.offsetMemory(xLoc, 4);
	auto yLoc2 = memManager.offsetMemory(yLoc, 8);
	auto yLoc3 = memManager.offsetMemory(yLoc2, 8);
	env.insert(ptrX, xLoc);
	env.insert(ptrX, xLoc2);
	store.insert(xLoc, yLoc2);
	store.insert(xLoc2, yLoc3);
	store.insert(xLoc2, zLoc);

	// Test load - multiple srcs
	auto loadNode = mainCfg->create<LoadNode>(y, x);
	auto status = transferFunction.evalLoadNode(loadNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_TRUE(status.hasEnvChanged());
	EXPECT_FALSE(status.hasStoreChanged());
	EXPECT_EQ(env.lookup(ptrY).getSize(), 3u);
	EXPECT_TRUE(env.lookup(ptrY).has(yLoc2));
	EXPECT_TRUE(env.lookup(ptrY).has(yLoc3));
	EXPECT_TRUE(env.lookup(ptrY).has(zLoc));
}

TEST_F(InterpreterTest, TransferLoadTest3)
{
	ptrManager.getOrCreatePointer(globalCtx, x);

	// Test load - failure
	auto loadNode = mainCfg->create<LoadNode>(y, x);
	auto status = transferFunction.evalLoadNode(loadNode);
	EXPECT_FALSE(status.isValid());
}

TEST_F(InterpreterTest, TransferStoreTest1)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	env.insert(ptrX, xLoc);
	env.insert(ptrY, yLoc);
	store.insert(yLoc, zLoc);

	// Test store - strong update
	auto storeNode = mainCfg->create<StoreNode>(z, y, x);
	auto status = transferFunction.evalStoreNode(storeNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_FALSE(status.hasEnvChanged());
	EXPECT_TRUE(status.hasStoreChanged());
	EXPECT_EQ(store.lookup(yLoc).getSize(), 1u);
	EXPECT_TRUE(store.lookup(yLoc).has(xLoc));
}

TEST_F(InterpreterTest, TransferStoreTest2)
{
	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto yLoc2 = memManager.offsetMemory(yLoc, 8);
	auto yLoc3 = memManager.offsetMemory(yLoc2, 8);
	env.insert(ptrX, xLoc);
	env.insert(ptrY, yLoc);
	env.insert(ptrY, yLoc2);
	store.insert(yLoc, zLoc);
	store.insert(yLoc2, yLoc3);

	// Test store - weak update (imprecise dst)
	auto storeNode = mainCfg->create<StoreNode>(z, y, x);
	auto status = transferFunction.evalStoreNode(storeNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_FALSE(status.hasEnvChanged());
	EXPECT_TRUE(status.hasStoreChanged());

	EXPECT_EQ(store.lookup(yLoc).getSize(), 2u);
	EXPECT_TRUE(store.lookup(yLoc).has(xLoc));
	EXPECT_TRUE(store.lookup(yLoc).has(zLoc));
	EXPECT_EQ(store.lookup(yLoc2).getSize(), 2u);
	EXPECT_TRUE(store.lookup(yLoc2).has(yLoc3));
	EXPECT_TRUE(store.lookup(yLoc2).has(xLoc));
}

TEST_F(InterpreterTest, TransferStoreTest3)
{
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto ptrZ = ptrManager.getOrCreatePointer(globalCtx, z);
	env.insert(ptrY, yLoc);
	env.insert(ptrZ, zLoc);
	store.insert(zLoc, xLoc);

	// Test store - weak update (summary dst)
	auto storeNode = mainCfg->create<StoreNode>(x, z, y);
	auto status = transferFunction.evalStoreNode(storeNode);
	EXPECT_TRUE(status.isValid());
	EXPECT_FALSE(status.hasEnvChanged());
	EXPECT_TRUE(status.hasStoreChanged());

	EXPECT_EQ(store.lookup(zLoc).getSize(), 2u);
	EXPECT_TRUE(store.lookup(zLoc).has(xLoc));
	EXPECT_TRUE(store.lookup(zLoc).has(yLoc));
}

}

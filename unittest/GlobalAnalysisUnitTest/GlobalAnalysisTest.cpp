#include "TPA/Analysis/GlobalPointerAnalysis.h"
#include "TPA/DataFlow/Env.h"
#include "TPA/DataFlow/StoreManager.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "Utils/ParseLLVMAssembly.h"

#include <llvm/Support/raw_ostream.h>

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

TEST(GlobalAnalysisTest, BasicTest)
{
	auto testModule = parseAssembly(
		"@g1 = common global i32 1, align 8\n"
		"@g2 = common global i32* @g1, align 8\n"
		"@g3 = common global {i32*, i32**} {i32* @g1, i32** @g2}, align 8\n"
		"define i32 @main() {\n"
		"bb:\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto g1 = testModule->getGlobalVariable("g1");
	auto g2 = testModule->getGlobalVariable("g2");
	auto g3 = testModule->getGlobalVariable("g3");

	auto ptrManager = PointerManager();
	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	VectorSetManager<const MemoryLocation*> pSetManager;
	auto storeManager = StoreManager(pSetManager);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, memManager, storeManager);
	auto envStore = globalAnalysis.runOnModule(*testModule);
	auto env = std::move(envStore.first);
	auto store = std::move(envStore.second);

	auto globalCtx = Context::getGlobalContext();
	auto ptr1 = ptrManager.getPointer(globalCtx, g1);
	EXPECT_NE(ptr1, nullptr);
	auto ptr2 = ptrManager.getPointer(globalCtx, g2);
	EXPECT_NE(ptr2, nullptr);
	auto ptr3 = ptrManager.getPointer(globalCtx, g3);
	EXPECT_NE(ptr3, nullptr);

	auto locSet1 = env.lookup(ptr1);
	ASSERT_NE(locSet1, nullptr);
	ASSERT_EQ(locSet1->getSize(), 1u);
	auto loc1 = *locSet1->begin();
	ASSERT_NE(loc1, memManager.getUniversalLocation());
	auto locSet2 = env.lookup(ptr2);
	ASSERT_NE(locSet2, nullptr);
	ASSERT_EQ(locSet2->getSize(), 1u);
	auto loc2 = *locSet2->begin();
	ASSERT_NE(loc2, memManager.getUniversalLocation());
	auto locSet3 = env.lookup(ptr3);
	ASSERT_NE(locSet3, nullptr);
	ASSERT_EQ(locSet3->getSize(), 1u);
	auto loc3 = *locSet3->begin();
	ASSERT_NE(loc3, memManager.getUniversalLocation());
	auto loc4 = memManager.offsetMemory(loc3, dataLayout.getPointerSize());
	ASSERT_NE(loc4, memManager.getUniversalLocation());

	auto memSet2 = store.lookup(loc2);
	ASSERT_NE(memSet2, nullptr);
	ASSERT_EQ(memSet2->getSize(), 1u);
	ASSERT_EQ(memSet2, locSet1);

	auto memSet3 = store.lookup(loc3);
	ASSERT_NE(memSet3, nullptr);
	ASSERT_EQ(memSet3->getSize(), 1u);
	ASSERT_EQ(memSet3, locSet1);

	auto memSet4 = store.lookup(loc4);
	ASSERT_NE(memSet4, nullptr);
	ASSERT_EQ(memSet4->getSize(), 1u);
	ASSERT_EQ(memSet4, locSet2);
}

TEST(GlobalAnalysisTest, BasicTest2)
{
	auto testModule = parseAssembly(
		"@g1 = global {i32, float} zeroinitializer, align 8\n"
		"@g2 = global float* getelementptr inbounds ({i32, float}* @g1, i32 0, i32 1), align 8\n"
		"@g3 = global float* null, align 8\n"
		"@g4 = global [2 x float**] [float** @g2, float** @g3], align 8\n"
		"define i32 @main() {\n"
		"bb:\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto g1 = testModule->getGlobalVariable("g1");
	auto g2 = testModule->getGlobalVariable("g2");
	auto g3 = testModule->getGlobalVariable("g3");
	auto g4 = testModule->getGlobalVariable("g4");

	auto ptrManager = PointerManager();
	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	VectorSetManager<const MemoryLocation*> pSetManager;
	auto storeManager = StoreManager(pSetManager);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, memManager, storeManager);
	auto envStore = globalAnalysis.runOnModule(*testModule);
	auto env = std::move(envStore.first);
	auto store = std::move(envStore.second);

	auto globalCtx = Context::getGlobalContext();
	auto ptr1 = ptrManager.getPointer(globalCtx, g1);
	EXPECT_NE(ptr1, nullptr);
	auto ptr2 = ptrManager.getPointer(globalCtx, g2);
	EXPECT_NE(ptr2, nullptr);
	auto ptr3 = ptrManager.getPointer(globalCtx, g3);
	EXPECT_NE(ptr3, nullptr);
	auto ptr4 = ptrManager.getPointer(globalCtx, g4);
	EXPECT_NE(ptr4, nullptr);

	auto locSet1 = env.lookup(ptr1);
	ASSERT_NE(locSet1, nullptr);
	ASSERT_EQ(locSet1->getSize(), 1u);
	auto loc1 = *locSet1->begin();
	ASSERT_NE(loc1, memManager.getUniversalLocation());
	auto locSet2 = env.lookup(ptr2);
	ASSERT_NE(locSet2, nullptr);
	ASSERT_EQ(locSet2->getSize(), 1u);
	auto loc2 = *locSet2->begin();
	ASSERT_NE(loc2, memManager.getUniversalLocation());
	auto locSet3 = env.lookup(ptr3);
	ASSERT_NE(locSet3, nullptr);
	ASSERT_EQ(locSet3->getSize(), 1u);
	auto loc3 = *locSet3->begin();
	ASSERT_NE(loc3, memManager.getUniversalLocation());
	auto locSet4 = env.lookup(ptr4);
	ASSERT_NE(locSet4, nullptr);
	ASSERT_EQ(locSet4->getSize(), 1u);
	auto loc4 = *locSet4->begin();
	ASSERT_NE(loc4, memManager.getUniversalLocation());

	auto memSet2 = store.lookup(loc2);
	ASSERT_NE(memSet2, nullptr);
	ASSERT_EQ(memSet2->getSize(), 1u);
	auto memSet2Loc = *memSet2->begin();
	ASSERT_EQ(memSet2Loc, memManager.offsetMemory(loc1, 4));

	auto memSet3 = store.lookup(loc3);
	ASSERT_NE(memSet3, nullptr);
	ASSERT_EQ(memSet3->getSize(), 1u);
	auto memSet3Loc = *memSet3->begin();
	ASSERT_EQ(memSet3Loc, memManager.getNullLocation());

	auto memSet4 = store.lookup(loc4);
	ASSERT_NE(memSet4, nullptr);
	ASSERT_EQ(memSet4->getSize(), 2u);
	ASSERT_TRUE(memSet4->has(loc2));
	ASSERT_TRUE(memSet4->has(loc3));
}

TEST(GlobalAnalysisTest, BasicTest3)
{
	auto testModule = parseAssembly(
		"@g1 = global [2 x {i32, i32}] [{i32, i32} {i32 0, i32 1}, {i32,  i32} {i32 2, i32 3}]\n"
		"@g2 = global i32* bitcast (i8* getelementptr (i8* bitcast ([2 x {i32, i32}]* @g1 to i8*), i64 12) to i32*), align 8\n"
		"@g3 = global i32* bitcast (i8* getelementptr (i8* bitcast ([2 x {i32, i32}]* @g1 to i8*), i64 4) to i32*), align 8\n"
		"@g4 = global i32* bitcast (i8* getelementptr (i8* bitcast ([2 x {i32, i32}]* @g1 to i8*), i64 8) to i32*), align 8\n"
		"@g5 = global <{ { i32**, i32, [4 x i8] }, { i32**, i32, [4 x i8] } }> <{ { i32**, i32, [4 x i8] } { i32** @g3, i32 2, [4 x i8] undef }, { i32**, i32, [4 x i8] } { i32** @g4, i32 3, [4 x i8] undef } }>, align 16\n"
		"@g6 = global { i32**, i32 }* bitcast (i8* getelementptr (i8* bitcast (<{ { i32**, i32, [4 x i8] }, { i32**, i32, [4 x i8] } }>* @g5 to i8*), i64 16) to { i32**, i32 }*), align 8\n"
		"define i32 @main() {\n"
		"bb:\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto g1 = testModule->getGlobalVariable("g1");
	auto g2 = testModule->getGlobalVariable("g2");
	auto g3 = testModule->getGlobalVariable("g3");
	auto g4 = testModule->getGlobalVariable("g4");
	auto g5 = testModule->getGlobalVariable("g5");
	auto g6 = testModule->getGlobalVariable("g6");

	auto ptrManager = PointerManager();
	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	VectorSetManager<const MemoryLocation*> pSetManager;
	auto storeManager = StoreManager(pSetManager);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, memManager, storeManager);
	auto envStore = globalAnalysis.runOnModule(*testModule);
	auto env = std::move(envStore.first);
	auto store = std::move(envStore.second);

	auto globalCtx = Context::getGlobalContext();
	auto ptr1 = ptrManager.getPointer(globalCtx, g1);
	EXPECT_NE(ptr1, nullptr);
	auto ptr2 = ptrManager.getPointer(globalCtx, g2);
	EXPECT_NE(ptr2, nullptr);
	auto ptr3 = ptrManager.getPointer(globalCtx, g3);
	EXPECT_NE(ptr3, nullptr);
	auto ptr4 = ptrManager.getPointer(globalCtx, g4);
	EXPECT_NE(ptr4, nullptr);
	auto ptr5 = ptrManager.getPointer(globalCtx, g5);
	EXPECT_NE(ptr5, nullptr);
	auto ptr6 = ptrManager.getPointer(globalCtx, g6);
	EXPECT_NE(ptr6, nullptr);

	auto locSet1 = env.lookup(ptr1);
	ASSERT_NE(locSet1, nullptr);
	ASSERT_EQ(locSet1->getSize(), 1u);
	auto loc1 = *locSet1->begin();
	ASSERT_NE(loc1, memManager.getUniversalLocation());
	auto locSet2 = env.lookup(ptr2);
	ASSERT_NE(locSet2, nullptr);
	ASSERT_EQ(locSet2->getSize(), 1u);
	auto loc2 = *locSet2->begin();
	ASSERT_NE(loc2, memManager.getUniversalLocation());
	auto locSet3 = env.lookup(ptr3);
	ASSERT_NE(locSet3, nullptr);
	ASSERT_EQ(locSet3->getSize(), 1u);
	auto loc3 = *locSet3->begin();
	ASSERT_NE(loc3, memManager.getUniversalLocation());
	auto locSet4 = env.lookup(ptr4);
	ASSERT_NE(locSet4, nullptr);
	ASSERT_EQ(locSet4->getSize(), 1u);
	auto loc4 = *locSet4->begin();
	ASSERT_NE(loc4, memManager.getUniversalLocation());
	auto locSet5 = env.lookup(ptr5);
	ASSERT_NE(locSet5, nullptr);
	ASSERT_EQ(locSet5->getSize(), 1u);
	auto loc5 = *locSet5->begin();
	ASSERT_NE(loc5, memManager.getUniversalLocation());
	auto locSet6 = env.lookup(ptr6);
	ASSERT_NE(locSet6, nullptr);
	ASSERT_EQ(locSet6->getSize(), 1u);
	auto loc6 = *locSet6->begin();
	ASSERT_NE(loc6, memManager.getUniversalLocation());

	auto memSet2 = store.lookup(loc2);
	ASSERT_NE(memSet2, nullptr);
	ASSERT_EQ(memSet2->getSize(), 1u);
	auto memSet2Loc = *memSet2->begin();
	ASSERT_EQ(memSet2Loc, memManager.offsetMemory(loc1, 4));

	auto memSet3 = store.lookup(loc3);
	ASSERT_NE(memSet3, nullptr);
	ASSERT_EQ(memSet3->getSize(), 1u);
	ASSERT_EQ(memSet3, memSet2);

	auto memSet4 = store.lookup(loc4);
	ASSERT_NE(memSet4, nullptr);
	ASSERT_EQ(memSet4->getSize(), 1u);
	ASSERT_EQ(memSet4, locSet1);

	auto memSet5 = store.lookup(loc5);
	ASSERT_NE(memSet5, nullptr);
	ASSERT_EQ(memSet5->getSize(), 1u);
	ASSERT_TRUE(memSet5->has(loc3));

	auto memSet5o = store.lookup(memManager.offsetMemory(loc5, 16));
	ASSERT_NE(memSet5o, nullptr);
	ASSERT_EQ(memSet5o->getSize(), 1u);
	ASSERT_TRUE(memSet5o->has(loc4));

	auto memSet6 = store.lookup(loc6);
	ASSERT_NE(memSet6, nullptr);
	ASSERT_EQ(memSet6->getSize(), 1u);
	ASSERT_TRUE(memSet6->has(memManager.offsetMemory(loc5, 16)));
}

}
#include "Memory/MemoryManager.h"
#include "Memory/PointerManager.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace {

TEST(MemoryTest, PointerManagerTest)
{
	auto ptrManager = PointerManager();
	
	EXPECT_TRUE(ptrManager.getUniversalPtr() != nullptr);
	EXPECT_TRUE(ptrManager.getNullPtr() != nullptr);

	auto testModule = parseAssembly(
		"@g = common global i32* null, align 8"
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32*, align 4\n"
		"  %y = alloca i32*, align 4\n"
		"  %z = alloca i32*, align 8\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto itr = testModule->begin()->begin()->begin();
	auto x = itr;
	auto y = ++itr;
	auto z = ++itr;
	auto g = testModule->global_begin();

	auto globalCtx = Context::getGlobalContext();
	auto fakeCtx = Context::pushContext(globalCtx, itr);

	auto ptrX = ptrManager.getOrCreatePointer(globalCtx, x);
	auto ptrY = ptrManager.getOrCreatePointer(globalCtx, y);
	auto ptrZ = ptrManager.getOrCreatePointer(fakeCtx, z);

	EXPECT_EQ(ptrX->getValue(), x);
	EXPECT_EQ(ptrX->getContext(), globalCtx);
	EXPECT_EQ(ptrY->getValue(), y);
	EXPECT_EQ(ptrZ->getValue(), z);
	EXPECT_EQ(ptrZ->getContext(), fakeCtx);
	EXPECT_EQ(ptrManager.getPointer(globalCtx, x), ptrX);
	EXPECT_EQ(ptrManager.getPointer(fakeCtx, x), nullptr);
	EXPECT_EQ(ptrManager.getPointer(globalCtx, y), ptrY);
	EXPECT_EQ(ptrManager.getPointer(globalCtx, z), nullptr);
	EXPECT_EQ(ptrManager.getPointer(fakeCtx, z), ptrZ);

	auto ptrZ2 = ptrManager.getOrCreatePointer(globalCtx, z);
	auto ptrVec = ptrManager.getPointersWithValue(z);
	EXPECT_EQ(ptrVec.size(), 2u);
	EXPECT_NE(std::find(ptrVec.begin(), ptrVec.end(), ptrZ), ptrVec.end());
	EXPECT_NE(std::find(ptrVec.begin(), ptrVec.end(), ptrZ2), ptrVec.end());
	auto ptrVec2 = ptrManager.getPointersWithValue(g);
	EXPECT_TRUE(ptrVec2.empty());
}

TEST(MemoryTest, BasicMemoryObjectTest)
{
	auto testModule = parseAssembly(
		"@g1 = global i32 1, align 4\n"
		"@g2 = global i32 2, align 4\n"
		"@arr = global [2 x i32*] [i32* @g1, i32* @g2], align 16\n"
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32, align 4\n"
		"  %y = alloca i32*, i32 4, align 4\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto f = testModule->begin();
	auto x = testModule->begin()->begin()->begin();
	auto y = ++x;

	GlobalValue* g1 = nullptr;
	GlobalValue* g2 = nullptr;
	GlobalValue* arr = nullptr;
	for (auto& glb: testModule->globals())
	{
		if (glb.getName() == std::string("g1"))
			g1 = &glb;
		else if (glb.getName() == std::string("g2"))
			g2 = &glb;
		else if (glb.getName() == std::string("arr"))
			arr = &glb;
	}
	ASSERT_TRUE(g1 != nullptr);
	ASSERT_TRUE(g2 != nullptr);
	ASSERT_TRUE(arr != nullptr);

	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	auto globalCtx = Context::getGlobalContext();
	auto mysterySite = ProgramLocation(globalCtx, nullptr);
	auto uObj = memManager.getUniversalObject();
	auto nObj = memManager.getNullObject();
	EXPECT_EQ(uObj->getAllocationSite(), mysterySite);
	EXPECT_EQ(uObj->getSize(), 1u);
	EXPECT_TRUE(uObj->isGlobalObject());
	EXPECT_EQ(nObj->getAllocationSite(), mysterySite);
	EXPECT_EQ(nObj->getSize(), 0u);
	EXPECT_TRUE(nObj->isGlobalObject());

	auto objG1 = memManager.allocateMemory(ProgramLocation(globalCtx, g1), g1->getType()->getElementType());
	EXPECT_TRUE(objG1 != nullptr);
	auto objG2 = memManager.allocateMemory(ProgramLocation(globalCtx, g2), g2->getType()->getElementType());
	EXPECT_TRUE(objG2 != nullptr);
	auto objArr = memManager.allocateMemory(ProgramLocation(globalCtx, arr), arr->getType()->getElementType());
	EXPECT_TRUE(objArr != nullptr);
	auto objX = memManager.allocateMemory(ProgramLocation(globalCtx, x), x->getType());
	EXPECT_TRUE(objX != nullptr);
	auto objY = memManager.allocateMemory(ProgramLocation(globalCtx, y), y->getType());
	EXPECT_TRUE(objY != nullptr);

	EXPECT_EQ(objG1->getSize(), dataLayout.getTypeAllocSize(g1->getType()->getElementType()));
	EXPECT_EQ(objG2->getSize(), dataLayout.getTypeAllocSize(g2->getType()->getElementType()));
	EXPECT_EQ(objArr->getSize(), dataLayout.getPointerSize());
	EXPECT_EQ(objX->getSize(), dataLayout.getTypeAllocSize(x->getType()));
	EXPECT_EQ(objY->getSize(), dataLayout.getTypeAllocSize(y->getType()));

	EXPECT_TRUE(objG1->isGlobalObject());
	EXPECT_TRUE(objG2->isGlobalObject());
	EXPECT_TRUE(objArr->isGlobalObject());
	EXPECT_TRUE(objX->isStackObject());
	EXPECT_TRUE(objY->isStackObject());

	auto objMain = memManager.createMemoryObjectForFunction(f);
	EXPECT_TRUE(objMain != nullptr);
	EXPECT_EQ(memManager.getFunctionForObject(objMain), f);
}

TEST(MemoryTest, OffsetMemoryObjectTest1)
{
	auto testModule = parseAssembly(
		"%struct.foo = type { i32, i64, i32, i32 }\n"
		"@g1 = global %struct.foo { i32 1, i64 2, i32 3, i32 4 }\n"
	);

	auto g1 = testModule->global_begin();

	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	auto globalCtx = Context::getGlobalContext();
	auto objG1 = memManager.allocateMemory(ProgramLocation(globalCtx, g1), g1->getType()->getElementType());
	auto locG12 = memManager.offsetMemory(objG1, 12);
	auto locG11 = memManager.offsetMemory(objG1, 4);

	EXPECT_EQ(locG11->getOffset(), 4u);
	EXPECT_EQ(locG11->getMemoryObject(), objG1);
	EXPECT_EQ(locG12->getOffset(), 12u);
	EXPECT_EQ(locG12->getMemoryObject(), objG1);

	auto locG1o = memManager.offsetMemory(locG11, 8u);
	EXPECT_EQ(locG1o->getMemoryObject(), objG1);
	EXPECT_EQ(locG1o->getOffset(), 12u);
	EXPECT_EQ(locG1o, locG12);

	auto locG13 = memManager.offsetMemory(locG11, 12u);
	EXPECT_EQ(locG13->getOffset(), 16u);
	EXPECT_EQ(locG13->getMemoryObject(), objG1);
}

TEST(MemoryTest, OffsetMemoryObjectTest2)
{
	auto testModule = parseAssembly(
		"%struct.foo = type { i32*, i32 }\n"
		"@arr = global <{ { i32*, i32, [4 x i8] }, { i32*, i32, [4 x i8] } }> <{ { i32*, i32, [4 x i8] } { i32* null, i32 2, [4 x i8] undef }, { i32*, i32, [4 x i8] } { i32* null, i32 3, [4 x i8] undef } }>, align 16\n"
	);

	auto arr = testModule->global_begin();

	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	auto objArr = memManager.allocateMemory(ProgramLocation(Context::getGlobalContext(), arr), arr->getType()->getElementType());
	auto loc = memManager.offsetMemory(objArr, 16);

	EXPECT_EQ(loc->getOffset(), 16u);
	EXPECT_EQ(loc->getMemoryObject(), objArr);

	auto loc2 = memManager.offsetMemory(loc, 8u);
	EXPECT_EQ(loc2->getOffset(), 24u);
	EXPECT_EQ(loc->getMemoryObject(), objArr);
}

TEST(MemoryTest, OffsetMemoryObjectTest3)
{
	auto testModule = parseAssembly(
		"%struct.bar = type { i16, i16 }\n"
		"%struct.foo = type { i32, i32, [3 x %struct.bar], i8* }\n"
		"@arr = global [1 x %struct.foo] [%struct.foo { i32 1, i32 0, [3 x %struct.bar] [%struct.bar { i16 1, i16 2} , %struct.bar { i16 3, i16 4}, %struct.bar { i16 5, i16 6}], i8* null }]\n"
	);

	auto arr = testModule->global_begin();

	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	auto objArr = memManager.allocateMemory(ProgramLocation(Context::getGlobalContext(), arr), arr->getType()->getElementType());
	auto loc1 = memManager.offsetMemory(objArr, 8);
	auto loc2 = memManager.offsetMemory(objArr, 12);
	auto loc3 = memManager.offsetMemory(objArr, 16);
	auto loc4 = memManager.offsetMemory(objArr, 24);

	EXPECT_EQ(loc1, loc2);
	EXPECT_EQ(loc2, loc3);
	EXPECT_NE(loc3, loc4);
	EXPECT_TRUE(loc2->isSummaryLocation());
	EXPECT_TRUE(loc3->isSummaryLocation());
	EXPECT_FALSE(loc4->isSummaryLocation());

	auto obj5 = memManager.offsetMemory(objArr, 10);
	auto obj6 = memManager.offsetMemory(objArr, 14);
	EXPECT_EQ(obj5, obj6);
	EXPECT_NE(obj5, loc1);
	EXPECT_NE(obj6, loc2);
}

}	// end of anonymous namespace
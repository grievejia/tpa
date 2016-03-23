#include "PointerAnalysis/MemoryModel/Pointer/PointerManager.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

TEST(PointerTest, PointerManagerTest)
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

}
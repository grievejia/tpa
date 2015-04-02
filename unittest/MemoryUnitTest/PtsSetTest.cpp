#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/PtsSet/PtsSet.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

TEST(MemoryTest, PtsSetTest)
{
	auto testModule = parseAssembly(
		"@g1 = common global i32* null, align 8"
		"@g2 = common global i32* null, align 8"
		"@g3 = common global i32* null, align 8"
	);

	auto itr = testModule->global_begin();
	auto g1 = itr++;
	auto g2 = itr++;
	auto g3 = itr++;

	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	auto globalCtx = Context::getGlobalContext();

	auto obj1 = memManager.allocateMemory(ProgramLocation(globalCtx, g1), g1->getType()->getElementType());
	auto loc1 = memManager.offsetMemory(obj1, 0);
	auto obj2 = memManager.allocateMemory(ProgramLocation(globalCtx, g2), g2->getType()->getElementType());
	auto loc2 = memManager.offsetMemory(obj2, 0);
	auto obj3 = memManager.allocateMemory(ProgramLocation(globalCtx, g3), g3->getType()->getElementType());
	auto loc3 = memManager.offsetMemory(obj3, 0);

	auto mtSet = PtsSet::getEmptySet();
	EXPECT_TRUE(mtSet.isEmpty());

	// set1 -> { loc1 }
	auto set1 = mtSet.insert(loc1);
	EXPECT_FALSE(set1.isEmpty());
	EXPECT_TRUE(set1.has(loc1));
	EXPECT_FALSE(set1.has(loc2));

	// set2 -> { loc1 }
	auto set2 = mtSet.insert(loc1);
	EXPECT_EQ(set1, set2);

	// set1 -> { loc1, loc2 }
	set1 = set1.insert(loc2);
	EXPECT_TRUE(set2.has(loc1));
	EXPECT_FALSE(set2.has(loc2));
	EXPECT_TRUE(set1.has(loc1));
	EXPECT_TRUE(set1.has(loc2));
	EXPECT_NE(set1, set2);

	// set3 -> { loc3 }
	auto set3 = PtsSet::getSingletonSet(loc3);
	EXPECT_FALSE(!PtsSet::intersects(set1, set3).empty());
	EXPECT_TRUE(!PtsSet::intersects(set1, set2).empty());

	// set4 -> { loc1, loc2, loc3 }
	auto set4 = set1.merge(set3);
	EXPECT_EQ(set4.getSize(), 3u);
	EXPECT_TRUE(set4.has(loc1));
	EXPECT_TRUE(set4.has(loc2));
	EXPECT_TRUE(set4.has(loc3));
	EXPECT_TRUE(set1.has(loc1));
	EXPECT_TRUE(set1.has(loc2));
	EXPECT_FALSE(set1.has(loc3));
}

}
#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "MemoryModel/PtsSet/Env.h"
#include "MemoryModel/PtsSet/PtsSet.h"
#include "MemoryModel/PtsSet/Store.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

TEST(PtsSetTest, PtsSetTest)
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

	// set5 -> { loc1, loc2 }
	auto set5 = set2.merge(set1);
	EXPECT_EQ(set5.getSize(), 2u);
	EXPECT_TRUE(set5.has(loc1));
	EXPECT_TRUE(set5.has(loc2));
}

TEST(PtsSetTest, EnvTest)
{
	auto ptrManager = PointerManager();

	auto testModule = parseAssembly(
		"@g1 = common global i32* null, align 8"
		"@g2 = common global i32* null, align 8"
		"@g3 = common global i32* null, align 8"
		"@g4 = common global i32* null, align 8"
	);

	auto itr = testModule->global_begin();
	auto g1 = itr++;
	auto g2 = itr++;
	auto g3 = itr++;
	auto g4 = itr++;

	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	auto globalCtx = Context::getGlobalContext();

	auto obj1 = memManager.offsetMemory(memManager.allocateMemory(ProgramLocation(globalCtx, g1), g1->getType()->getElementType()), 0);
	auto obj2 = memManager.offsetMemory(memManager.allocateMemory(ProgramLocation(globalCtx, g2), g2->getType()->getElementType()), 0);
	auto obj3 = memManager.offsetMemory(memManager.allocateMemory(ProgramLocation(globalCtx, g3), g3->getType()->getElementType()), 0);
	auto obj4 = memManager.offsetMemory(memManager.allocateMemory(ProgramLocation(globalCtx, g4), g4->getType()->getElementType()), 0);

	auto ptr1 = ptrManager.getOrCreatePointer(globalCtx, g1);
	auto ptr2 = ptrManager.getOrCreatePointer(globalCtx, g2);
	auto ptr3 = ptrManager.getOrCreatePointer(globalCtx, g3);
	auto ptr4 = ptrManager.getOrCreatePointer(globalCtx, g4);

	auto env = Env();
	EXPECT_EQ(env.getSize(), 0u);
	EXPECT_TRUE(env.lookup(ptr1).isEmpty());

	// env = [ var1 -> {addr1} ]
	EXPECT_TRUE(env.insert(ptr1, obj1));
	EXPECT_EQ(env.getSize(), 1u);
	ASSERT_FALSE(env.lookup(ptr1).isEmpty());
	EXPECT_TRUE(env.lookup(ptr1).has(obj1));

	// env = [ var1 -> {addr1}, var2 -> {addr2}]
	EXPECT_TRUE(env.insert(ptr2, obj2));
	EXPECT_EQ(env.getSize(), 2u);
	ASSERT_FALSE(env.lookup(ptr2).isEmpty());
	EXPECT_TRUE(env.lookup(ptr2).has(obj2));
	EXPECT_FALSE(env.insert(ptr1, obj1));
	EXPECT_FALSE(env.insert(ptr2, obj2));

	// env = [ var1 -> {addr1}, var2 -> {addr2}, var3 -> {addr3} ]
	EXPECT_TRUE(env.insert(ptr3, obj3));
	EXPECT_EQ(env.getSize(), 3u);
	ASSERT_FALSE(env.lookup(ptr3).isEmpty());
	EXPECT_TRUE(env.lookup(ptr3).has(obj3));

	EXPECT_TRUE(env.lookup(ptr4).isEmpty());

	auto pSet3 = PtsSet::getSingletonSet(obj3);
	auto pSet4 = PtsSet::getSingletonSet(obj4);

	// env = [ var1 -> {addr1, addr4}, var2 -> {addr2}, var3 -> {addr3} ]
	EXPECT_TRUE(env.weakUpdate(ptr1, pSet4));
	EXPECT_EQ(env.getSize(), 3u);
	ASSERT_FALSE(env.lookup(ptr1).isEmpty());
	EXPECT_TRUE(env.lookup(ptr1).has(obj1));
	EXPECT_TRUE(env.lookup(ptr1).has(obj4));
	
	// env = [ var1 -> {addr1, addr4}, var2 -> {addr2, addr3}, var3 -> {addr3} ]
	EXPECT_FALSE(env.weakUpdate(ptr3, pSet3));
	EXPECT_TRUE(env.weakUpdate(ptr2, pSet3));
	ASSERT_FALSE(env.lookup(ptr2).isEmpty());
	EXPECT_TRUE(env.lookup(ptr2).has(obj2));
	EXPECT_TRUE(env.lookup(ptr2).has(obj3));

	// env = [ var1 -> {addr1, addr4}, var2 -> {addr2, addr3}, var3 -> {addr1, addr4} ]
	ASSERT_FALSE(env.lookup(ptr1).isEmpty());
	EXPECT_TRUE(env.strongUpdate(ptr3, env.lookup(ptr1)));
	ASSERT_FALSE(env.lookup(ptr3).isEmpty());
	EXPECT_TRUE(env.lookup(ptr3).has(obj1));
	EXPECT_TRUE(env.lookup(ptr3).has(obj4));
	EXPECT_FALSE(env.lookup(ptr3).has(obj3));
}

bool isStoreEqual(const Store& s0, const Store& s1)
{
	if (s0.getSize() != s1.getSize())
		return false;
	for (auto const& mapping: s0)
		if (s1.lookup(mapping.first) != mapping.second)
			return false;
	return true;
}

TEST(PtsSetTest, StoreTest)
{
	auto testModule = parseAssembly(
		"declare noalias i8* @malloc(i64) \n"
		"@g1 = global {i32*, i32*, i32*, i32*} {i32* null, i32* null, i32* null, i32* null}, align 16\n"
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca {i32*, i32*}, align 4\n"
		"  %y = call noalias i8* @malloc(i64 8)\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto g1 = testModule->global_begin();
	auto itr = testModule->begin()->begin()->begin();
	auto g2 = itr++;
	auto g3 = itr++;

	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);

	auto globalCtx = Context::getGlobalContext();

	auto o1 = memManager.allocateMemory(ProgramLocation(globalCtx, g1), g1->getType()->getElementType());
	auto obj1 = memManager.offsetMemory(o1, 0);
	auto o2 = memManager.allocateMemory(ProgramLocation(globalCtx, g2), g2->getType());
	auto obj2 = memManager.offsetMemory(o2, 0);
	auto o3 = memManager.allocateMemory(ProgramLocation(globalCtx, g3), g3->getType());
	auto obj3 = memManager.offsetMemory(o3, 0);
	auto obj10 = memManager.offsetMemory(o1, 8);
	auto obj11 = memManager.offsetMemory(o1, 16);
	auto obj12 = memManager.offsetMemory(o1, 24);
	auto obj13 = memManager.offsetMemory(o1, 32);
	auto obj20 = memManager.offsetMemory(o2, 8);
	auto obj21 = memManager.offsetMemory(o1, 16);;

	auto mtStore = Store();

	// st1 = [ 1->{10, 11} ]
	auto st1 = mtStore;
	EXPECT_TRUE(st1.insert( obj1, obj10));
	EXPECT_TRUE(st1.insert( obj1, obj11));
	EXPECT_EQ(st1.getSize(), 1u);

	auto pSet1 = st1.lookup(obj1);
	EXPECT_FALSE(pSet1.isEmpty());
	EXPECT_EQ(pSet1.getSize(), 2u);
	EXPECT_TRUE(pSet1.has(obj10));
	EXPECT_TRUE(pSet1.has(obj11));

	// st2 = [ 1->{10, 12}]
	auto st2 = mtStore;
	EXPECT_TRUE(st2.insert( obj1, obj10));
	EXPECT_TRUE(st2.insert( obj1, obj12));
	EXPECT_EQ(st2.getSize(), 1u);

	// st3  = [ 1->{10, 11, 12}]
	auto st3 = st1;
	EXPECT_TRUE(st3.weakUpdate(obj1, st2.lookup(obj1)));
	EXPECT_EQ(st3.getSize(), 1u);

	auto pSet3 = st3.lookup(obj1);
	EXPECT_NE(pSet3, pSet1);
	EXPECT_FALSE(pSet3.isEmpty());
	EXPECT_EQ(pSet3.getSize(), 3u);
	EXPECT_TRUE(pSet3.has(obj10));
	EXPECT_TRUE(pSet3.has(obj11));
	EXPECT_TRUE(pSet3.has(obj12));

	auto pSet2 = st2.lookup(obj1);
	EXPECT_FALSE(pSet2.isEmpty());
	EXPECT_EQ(pSet2.getSize(), 2u);
	EXPECT_TRUE(pSet2.has(obj10));
	EXPECT_FALSE(pSet2.has(obj11));
	EXPECT_TRUE(pSet2.has(obj12));

	// st4 = [ 1->{10, 12} ]
	auto st4 = st1;
	EXPECT_TRUE(st4.strongUpdate(obj1, st2.lookup(obj1)));
	EXPECT_EQ(st4.getSize(), 1u);

	auto pSet4 = st4.lookup(obj1);
	EXPECT_FALSE(pSet4.isEmpty());
	EXPECT_EQ(pSet4.getSize(), 2u);
	EXPECT_TRUE(pSet4.has(obj10));
	EXPECT_FALSE(pSet4.has(obj11));
	EXPECT_TRUE(pSet4.has(obj12));
	EXPECT_TRUE(isStoreEqual(st4, st2));

	// st5 = [ 2->{20, 21} ]
	auto st5 = mtStore;
	EXPECT_TRUE(st5.insert(obj2, obj20));
	EXPECT_TRUE(st5.insert(obj2, obj21));
	EXPECT_EQ(st5.getSize(), 1u);

	// st6 = [ 1->{10, 12}, 2->{20, 21} ]
	auto st6 = st4;
	EXPECT_TRUE(st6.mergeWith(st5));
	EXPECT_EQ(st6.getSize(), 2u);

	EXPECT_TRUE(st6.contains(obj1));
	EXPECT_TRUE(st6.contains(obj2));
	auto pSet61 = st6.lookup(obj1);
	auto pSet62 = st6.lookup(obj2);
	EXPECT_FALSE(pSet61.isEmpty());
	EXPECT_EQ(pSet61.getSize(), 2u);
	EXPECT_TRUE(pSet61.has(obj10));
	EXPECT_TRUE(pSet61.has(obj12));
	EXPECT_FALSE(pSet62.isEmpty());
	EXPECT_EQ(pSet62.getSize(), 2u);
	EXPECT_TRUE(pSet62.has(obj20));
	EXPECT_TRUE(pSet62.has(obj21));

	// st7 = [ 1->{10, 12}, 2->{20, 21}, 3->{10, 11} ]
	auto st7 = st6;
	EXPECT_TRUE(st7.weakUpdate(obj3, st1.lookup(obj1)));
	EXPECT_EQ(st7.getSize(), 3u);

	EXPECT_TRUE(st7.contains(obj3));
	EXPECT_EQ(st7.lookup(obj3), st1.lookup(obj1));

	// st8 = st7
	auto st8 = st7;
	EXPECT_TRUE(st8.contains(obj1));
	EXPECT_TRUE(st8.contains(obj2));
	EXPECT_TRUE(st8.contains(obj3));

	// st9 = [ 1->{10, 12}, 2->{20, 21}, 3->{10, 11} ]
	auto st9 = st7;
	EXPECT_FALSE(st9.insert(obj1, obj10));
	EXPECT_EQ(st9.getSize(), 3u);

	EXPECT_TRUE(isStoreEqual(st7, st9));

	// st10 = [ 1->{10, 12, 13}, 2->{20, 21}, 3->{10, 11} ]
	auto st10 = st7;
	EXPECT_TRUE(st9.insert(obj1, obj13));
	EXPECT_TRUE(isStoreEqual(st7, st10));
	EXPECT_EQ(st10.getSize(), 3u);

	// st11 = [ 1->{10, 12}, 2->{20, 21}, 3->{10, 11} ]
	auto st11 = st7;
	EXPECT_FALSE(st11.strongUpdate(obj1, st4.lookup(obj1)));
	EXPECT_TRUE(isStoreEqual(st7, st11));
	EXPECT_EQ(st11.getSize(), 3u);
}

}
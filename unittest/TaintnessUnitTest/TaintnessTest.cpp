#include "Client/Taintness/DataFlow/TaintEnv.h"
#include "Client/Taintness/SourceSink/Checker/SinkSignature.h"
#include "Client/Taintness/SourceSink/Checker/SinkViolationChecker.h"
#include "Client/Taintness/SourceSink/Table/ExternalTaintTable.h"
#include "PointerAnalysis/MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/Pointer/PointerManager.h"
#include "Annotation/Pointer/ExternalPointerTable.h"
#include "Utils/DummyPointerAnalysis.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace llvm;
using namespace tpa;
using namespace client;
using namespace client::taint;

namespace
{

TEST(TaintnessTest, SourceSinkTest)
{
	auto ssTable = ExternalTaintTable();
	EXPECT_EQ(ssTable.lookup("read"), nullptr);
	EXPECT_EQ(ssTable.lookup("printf"), nullptr);

	ssTable = ExternalTaintTable::loadFromFile();
	ASSERT_TRUE(ssTable.size() > 0);
	EXPECT_NE(ssTable.lookup("read"), nullptr);
	EXPECT_NE(ssTable.lookup("printf"), nullptr);
}

TEST(TaintnessTest, EnvTest)
{
	auto testModule = parseAssembly(
		"@g = common global i32* null, align 8"
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32*, align 4\n"
		"  %y = alloca i32*, align 4\n"
		"  %z = alloca i32*, align 8\n"
		"  %g = alloca i32*, align 8\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto globalCtx = Context::getGlobalContext();
	auto itr = testModule->begin()->begin()->begin();
	auto x = ProgramLocation(globalCtx, itr);
	auto y = ProgramLocation(globalCtx, ++itr);
	auto z = ProgramLocation(globalCtx, ++itr);
	auto g = ProgramLocation(globalCtx, ++itr);

	auto env = TaintEnv();
	EXPECT_TRUE(env.weakUpdate(x, TaintLattice::Untainted));
	EXPECT_TRUE(env.weakUpdate(y, TaintLattice::Tainted));
	EXPECT_EQ(env.size(), 2u);
	EXPECT_EQ(env.lookup(x), TaintLattice::Untainted);
	EXPECT_EQ(env.lookup(y), TaintLattice::Tainted);
	EXPECT_EQ(env.lookup(z), TaintLattice::Unknown);

	auto env2 = env;
	EXPECT_FALSE(env2.weakUpdate(y, TaintLattice::Tainted));
	EXPECT_FALSE(env2.weakUpdate(x, TaintLattice::Untainted));
	EXPECT_TRUE(env2.weakUpdate(x, TaintLattice::Tainted));
	EXPECT_EQ(env2.lookup(x), TaintLattice::Either);
	EXPECT_EQ(env.lookup(x), TaintLattice::Untainted);

	auto env3 = TaintEnv();
	EXPECT_TRUE(env3.weakUpdate(z, TaintLattice::Untainted));
	EXPECT_TRUE(env3.weakUpdate(g, TaintLattice::Tainted));
	auto env4 = TaintEnv();
	EXPECT_TRUE(env4.weakUpdate(g, TaintLattice::Tainted));
	EXPECT_FALSE(env3.mergeWith(env4));
	EXPECT_TRUE(env4.strongUpdate(g, TaintLattice::Untainted));
	EXPECT_TRUE(env3.mergeWith(env4));
	EXPECT_EQ(env3.lookup(g), TaintLattice::Either);

	EXPECT_TRUE(env.mergeWith(env4));
	EXPECT_EQ(env.lookup(g), TaintLattice::Untainted);
	EXPECT_EQ(env.lookup(z), TaintLattice::Unknown);
	EXPECT_TRUE(env.mergeWith(env3));
	EXPECT_EQ(env3.lookup(g), TaintLattice::Either);
	EXPECT_EQ(env3.lookup(z), TaintLattice::Untainted);

	EXPECT_TRUE(env3.strongUpdate(g, TaintLattice::Untainted));
	EXPECT_EQ(env3.lookup(g), TaintLattice::Untainted);
	EXPECT_FALSE(env3.strongUpdate(z, TaintLattice::Untainted));
	EXPECT_EQ(env3.lookup(z), TaintLattice::Untainted);
}

TEST(TaintnessTest, ViolationCheckerTest)
{
	auto testModule = parseAssembly(
		"@g0 = common global i64 1, align 8"
		"@g1 = common global i64 2, align 8"
		"@g2 = common global i64 3, align 8"
		"@g3 = common global i8 4, align 8"
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = load i64* @g0, align 8\n"
		"  %y = load i64* @g1, align 8\n"
		"  %z = load i64* @g2, align 8\n"
		"  call noalias i8* @malloc(i64 %x)\n"
		"  call noalias i8* @malloc(i64 %y)\n"
		"  call i64 @read(i64 %x, i8* null, i64 %z)\n"
		"  call i32 (i8*, ...)* @printf(i8* @g3, i64 %x, i64 %y, i64 %z)"
		"  ret i32 0\n"
		"}\n"
		"declare noalias i8* @malloc(i64)\n"
		"declare i64 @read(i64, i8*, i64)\n"
		"declare i32 @printf(i8*, ...)"
	);

	// Grab a pointer to each value and each function
	auto globalCtx = Context::getGlobalContext();
	auto itr = testModule->getFunction("main")->begin()->begin();
	auto x = ProgramLocation(globalCtx, itr);
	auto y = ProgramLocation(globalCtx, ++itr);
	auto z = ProgramLocation(globalCtx, ++itr);
	auto du0 = DefUseInstruction(*++itr);
	auto du1 = DefUseInstruction(*++itr);
	auto du2 = DefUseInstruction(*++itr);
	auto du3 = DefUseInstruction(*++itr);
	auto sink0 = DefUseProgramLocation(globalCtx, &du0);
	auto sink1 = DefUseProgramLocation(globalCtx, &du1);
	auto sink2 = DefUseProgramLocation(globalCtx, &du2);
	auto sink3 = DefUseProgramLocation(globalCtx, &du3);
	auto g3 = testModule->getGlobalVariable("g3");
	ASSERT_NE(g3, nullptr);

	auto mallocFunc = testModule->getFunction("malloc");
	auto printfFunc = testModule->getFunction("printf");
	auto readFunc = testModule->getFunction("read");

	// Create and initialize PointerAnalysis
	auto ptrManager = PointerManager();
	auto dataLayout = DataLayout(testModule.get());
	auto memManager = MemoryManager(dataLayout);
	auto ptrG = ptrManager.getOrCreatePointer(globalCtx, g3);
	auto objG = memManager.allocateMemory(ProgramLocation(globalCtx, g3), g3->getType()->getElementType());
	auto locG = memManager.offsetMemory(objG, 0);
	auto extTable = ExternalPointerTable::loadFromFile("ptr_effect.conf");

	DummyPointerAnalysis ptrAnalysis(ptrManager, memManager, extTable);
	ptrAnalysis.injectEnv(ptrG, locG);

	// Create and initialize taint env and store
	TaintEnv env;
	TaintStore store;
	env.strongUpdate(x, TaintLattice::Tainted);
	env.strongUpdate(y, TaintLattice::Untainted);
	env.strongUpdate(z, TaintLattice::Either);
	store.strongUpdate(locG, TaintLattice::Either);

	auto ssLookupTable = ExternalTaintTable::loadFromFile();

	// The real test actually starts here...
	SinkViolationChecker checker(env, store, ssLookupTable, ptrAnalysis);
	auto checkRes0 = checker.checkSinkViolation(SinkSignature(sink0, mallocFunc));
	ASSERT_EQ(checkRes0.size(), 1u);
	EXPECT_EQ(checkRes0.front().argPos, 0u);
	EXPECT_EQ(checkRes0.front().what, TClass::ValueOnly);
	EXPECT_EQ(checkRes0.front().expectVal, TaintLattice::Untainted);
	EXPECT_EQ(checkRes0.front().actualVal, TaintLattice::Tainted);

	auto checkRes1 = checker.checkSinkViolation(SinkSignature(sink1, mallocFunc));
	ASSERT_TRUE(checkRes1.empty());

	auto checkRes2 = checker.checkSinkViolation(SinkSignature(sink2, readFunc));
	ASSERT_EQ(checkRes2.size(), 2u);
	EXPECT_EQ(checkRes2[0].argPos, 0u);
	EXPECT_EQ(checkRes2[0].what, TClass::ValueOnly);
	EXPECT_EQ(checkRes2[0].expectVal, TaintLattice::Untainted);
	EXPECT_EQ(checkRes2[0].actualVal, TaintLattice::Tainted);
	EXPECT_EQ(checkRes2[1].argPos, 2u);
	EXPECT_EQ(checkRes2[1].what, TClass::ValueOnly);
	EXPECT_EQ(checkRes2[1].expectVal, TaintLattice::Untainted);
	EXPECT_EQ(checkRes2[1].actualVal, TaintLattice::Either);

	auto checkRes3 = checker.checkSinkViolation(SinkSignature(sink3, printfFunc));
	ASSERT_EQ(checkRes3.size(), 1u);
	EXPECT_EQ(checkRes3.front().argPos, 0u);
	EXPECT_EQ(checkRes3.front().what, TClass::DirectMemory);
	EXPECT_EQ(checkRes3.front().expectVal, TaintLattice::Untainted);
	EXPECT_EQ(checkRes3.front().actualVal, TaintLattice::Either);
}

}

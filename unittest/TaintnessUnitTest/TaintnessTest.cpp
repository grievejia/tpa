#include "Client/Taintness/DataFlow/SourceSink.h"
#include "Client/Taintness/DataFlow/TaintEnvStore.h"
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
	auto ssManager = SourceSinkLookupTable();
	EXPECT_EQ(ssManager.getSummary("read"), nullptr);
	EXPECT_EQ(ssManager.getSummary("printf"), nullptr);

	ssManager.readSummaryFromFile("source_sink.conf");
	EXPECT_NE(ssManager.getSummary("read"), nullptr);
	EXPECT_NE(ssManager.getSummary("printf"), nullptr);
	EXPECT_EQ(ssManager.getSummary("scanf")->getSize(), 2u);
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
		"  ret i32 0\n"
		"}\n"
	);

	auto globalCtx = Context::getGlobalContext();
	auto itr = testModule->begin()->begin()->begin();
	auto x = ProgramLocation(globalCtx, itr);
	auto y = ProgramLocation(globalCtx, ++itr);
	auto z = ProgramLocation(globalCtx, ++itr);
	auto g = ProgramLocation(globalCtx, testModule->global_begin());

	auto env = TaintEnv();
	EXPECT_TRUE(env.weakUpdate(x, TaintLattice::Untainted));
	EXPECT_TRUE(env.weakUpdate(y, TaintLattice::Tainted));
	EXPECT_EQ(env.getSize(), 2u);
	EXPECT_TRUE(bool(env.lookup(x)));
	EXPECT_EQ(*env.lookup(x), TaintLattice::Untainted);
	EXPECT_TRUE(bool(env.lookup(y)));
	EXPECT_EQ(*env.lookup(y), TaintLattice::Tainted);
	EXPECT_FALSE(env.lookup(z));

	auto env2 = env;
	EXPECT_FALSE(env2.weakUpdate(y, TaintLattice::Tainted));
	EXPECT_FALSE(env2.weakUpdate(x, TaintLattice::Untainted));
	EXPECT_TRUE(env2.weakUpdate(x, TaintLattice::Tainted));
	EXPECT_TRUE(bool(env2.lookup(x)));
	EXPECT_EQ(*env2.lookup(x), TaintLattice::Either);
	EXPECT_TRUE(bool(env.lookup(x)));
	EXPECT_EQ(*env.lookup(x), TaintLattice::Untainted);

	auto env3 = TaintEnv();
	EXPECT_TRUE(env3.weakUpdate(z, TaintLattice::Untainted));
	EXPECT_TRUE(env3.weakUpdate(g, TaintLattice::Tainted));
	auto env4 = TaintEnv();
	EXPECT_TRUE(env4.weakUpdate(g, TaintLattice::Tainted));
	EXPECT_FALSE(env3.mergeWith(env4));
	EXPECT_TRUE(env4.strongUpdate(g, TaintLattice::Untainted));
	EXPECT_TRUE(env3.mergeWith(env4));
	EXPECT_TRUE(bool(env3.lookup(g)));
	EXPECT_EQ(*env3.lookup(g), TaintLattice::Either);

	EXPECT_TRUE(env.mergeWith(env4));
	EXPECT_TRUE(bool(env.lookup(g)));
	EXPECT_EQ(*env.lookup(g), TaintLattice::Untainted);
	EXPECT_FALSE(bool(env.lookup(z)));
	EXPECT_TRUE(env.mergeWith(env3));
	EXPECT_TRUE(bool(env3.lookup(g)));
	EXPECT_EQ(*env3.lookup(g), TaintLattice::Either);
	EXPECT_TRUE(bool(env3.lookup(z)));
	EXPECT_EQ(*env3.lookup(z), TaintLattice::Untainted);

	EXPECT_TRUE(env3.strongUpdate(g, TaintLattice::Untainted));
	EXPECT_TRUE(bool(env3.lookup(g)));
	EXPECT_EQ(*env3.lookup(g), TaintLattice::Untainted);
	EXPECT_FALSE(env3.strongUpdate(z, TaintLattice::Untainted));
	EXPECT_TRUE(bool(env3.lookup(z)));
	EXPECT_EQ(*env3.lookup(z), TaintLattice::Untainted);
}

}

#include "Client/Taintness/Precision/TaintPrecisionMonitor.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace llvm;
using namespace tpa;
using namespace client;
using namespace client::taint;

namespace
{

template <size_t n>
bool isArgEmpty(const PrecisionLossPositionSet<n>& posSet)
{
	bool ret = true;
	for (auto i = size_t(0), e = posSet.size(); i < e; ++i)
		ret = ret && !posSet.isArgumentLossPosition(i);
	return ret;
}

template <size_t n>
bool isEmpty(const PrecisionLossPositionSet<n>& posSet)
{
	bool ret = !posSet.isReturnLossPosition();
	return ret && isArgEmpty(posSet);
}

template <size_t n, typename Filter>
void setSelectedArgNotEmpty(PrecisionLossPositionSet<n>& posSet, Filter&& f)
{
	for (auto i = size_t(0), e = posSet.size(); i < e; ++i)
		if (f(i))
			posSet.setArgumentLossPosition(i);
}

template <size_t n, typename Filter>
bool isSelectedArgNotEmpty(const PrecisionLossPositionSet<n>& posSet, Filter&& f)
{
	bool ret = true;
	for (auto i = size_t(0), e = posSet.size(); i < e; ++i)
		if (f(i))
			ret = ret && posSet.isArgumentLossPosition(i);
		else
			ret = ret && !posSet.isArgumentLossPosition(i);
	return ret;
}

TEST(TaintnessTest, PrecisionLossPositionSetTest)
{
	auto posSet = PrecisionLossPositionSet<8>();

	EXPECT_TRUE(isEmpty(posSet));

	posSet.setReturnLossPosition();
	EXPECT_TRUE(posSet.isReturnLossPosition());
	EXPECT_TRUE(isArgEmpty(posSet));

	auto filter = [] (size_t idx)
	{
		return (idx % 3 == 0);
	};

	setSelectedArgNotEmpty(posSet, filter);
	EXPECT_TRUE(isSelectedArgNotEmpty(posSet, filter));
	EXPECT_TRUE(posSet.isReturnLossPosition());
}

TEST(TaintnessTest, PrecisionLossRecordTest)
{
	auto testModule = parseAssembly(
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

	auto record = PrecisionLossRecord();
	
	EXPECT_EQ(record.lookupRecord(x), nullptr);
	EXPECT_EQ(record.lookupRecord(y), nullptr);
	EXPECT_EQ(record.lookupRecord(z), nullptr);

	auto& posSetX = record.getRecordAt(x);
	EXPECT_TRUE(isEmpty(posSetX));
	posSetX.setReturnLossPosition();

	auto posSetX2 = record.lookupRecord(x);
	ASSERT_NE(posSetX2, nullptr);
	EXPECT_TRUE(posSetX2->isReturnLossPosition());

	auto predY = [] (size_t idx)
	{
		return idx > 4;
	};
	auto predZ = [] (size_t idx)
	{
		return idx < 4;
	};

	setSelectedArgNotEmpty(record.getRecordAt(y), predY);
	setSelectedArgNotEmpty(record.getRecordAt(z), predZ);

	auto posSetY2 = record.lookupRecord(y);
	ASSERT_NE(posSetY2, nullptr);
	EXPECT_TRUE(isSelectedArgNotEmpty(*posSetY2, predY));

	auto posSetZ2 = record.lookupRecord(z);
	ASSERT_NE(posSetZ2, nullptr);
	EXPECT_TRUE(isSelectedArgNotEmpty(*posSetZ2, predZ));
}

TEST(TaintnessTest, PrecisionMonitorTest)
{
	auto testModule = parseAssembly(
		"define i32 @main() {\n"
		"bb:\n"
		"  %a = alloca i32, align 4\n"
		"  %b = alloca i32, align 4\n"
		"  %c = alloca i32, align 4\n"
		"  %d = alloca i32, align 4\n"
		"  %e = alloca i32, align 4\n"
		"  %r = call i32* @foo(i32* %a, i32* %b, i32* %c, i32* %d, i32* %e)\n"
		"  ret i32 0\n"
		"}\n"
		"declare i32* @foo(i32*, i32*, i32*, i32*, i32*)\n"
	);

	auto globalCtx = Context::getGlobalContext();
	auto itr = testModule->getFunction("main")->begin()->begin();
	for (auto i = 0u; i < 4; ++i)
		++itr;
	auto callsite = ProgramLocation(globalCtx, ++itr);

	auto env = TaintEnv();
	auto monitor = TaintPrecisionMonitor(env);

	auto callee = testModule->getFunction("foo");
	auto paramItr = callee->arg_begin();
	env.strongUpdate(ProgramLocation(globalCtx, paramItr), TaintLattice::Tainted);
	env.strongUpdate(ProgramLocation(globalCtx, ++paramItr), TaintLattice::Untainted);
	env.strongUpdate(ProgramLocation(globalCtx, ++paramItr), TaintLattice::Untainted);
	env.strongUpdate(ProgramLocation(globalCtx, ++paramItr), TaintLattice::Either);
	env.strongUpdate(ProgramLocation(globalCtx, ++paramItr), TaintLattice::Either);

	std::vector<TaintLattice> taintVec = { TaintLattice::Tainted, TaintLattice::Tainted, TaintLattice::Either, TaintLattice::Untainted, TaintLattice::Either };
	monitor.trackPrecisionLoss(callsite, globalCtx, callee, taintVec);

	auto posSet = monitor.getPrecisionLossRecord().lookupRecord(callsite);
	ASSERT_NE(posSet, nullptr);
	EXPECT_FALSE(posSet->isReturnLossPosition());
	EXPECT_FALSE(posSet->isArgumentLossPosition(0));
	EXPECT_TRUE(posSet->isArgumentLossPosition(1));
	EXPECT_TRUE(posSet->isArgumentLossPosition(2));
	EXPECT_TRUE(posSet->isArgumentLossPosition(3));
	EXPECT_FALSE(posSet->isArgumentLossPosition(4));
}

}
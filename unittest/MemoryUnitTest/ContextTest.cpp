#include "MemoryModel/Precision/KLimitContext.h"
#include "MemoryModel/Precision/AdaptiveContext.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

TEST(ContextTest, ContextTest)
{
	auto testModule = parseAssembly(
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32*, align 4\n"
		"  %y = alloca i32*, align 4\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto itr = testModule->begin()->begin()->begin();
	auto x = itr;
	auto y = ++itr;

	auto globalCtx = Context::getGlobalContext();
	EXPECT_TRUE(globalCtx->isGlobalContext());
	EXPECT_EQ(globalCtx->size(), 0u);

	auto ctxX = Context::pushContext(globalCtx, x);
	auto ctxY = Context::pushContext(globalCtx, y);
	auto ctxXY = Context::pushContext(ctxX, y);
	EXPECT_FALSE(ctxX->isGlobalContext());
	EXPECT_EQ(ctxX->size(), 1u);
	EXPECT_EQ(ctxY->size(), 1u);
	EXPECT_EQ(ctxXY->size(), 2u);

	EXPECT_NE(ctxX, globalCtx);
	EXPECT_NE(ctxY, ctxX);
	EXPECT_NE(ctxX, ctxXY);
	EXPECT_NE(ctxY, ctxXY);
	EXPECT_EQ(ctxXY, Context::pushContext(ctxX, y));
	
	EXPECT_EQ(x, ctxX->getCallSite());
	EXPECT_EQ(y, ctxY->getCallSite());
	EXPECT_EQ(y, ctxXY->getCallSite());
	EXPECT_EQ(ctxX, Context::popContext(ctxXY));
	EXPECT_EQ(globalCtx, Context::popContext(ctxX));
	EXPECT_EQ(globalCtx, Context::popContext(ctxY));

	auto allCtx = Context::getAllContexts();
	EXPECT_NE(std::find(allCtx.begin(), allCtx.end(), ctxX), allCtx.end());
	EXPECT_NE(std::find(allCtx.begin(), allCtx.end(), ctxY), allCtx.end());
	EXPECT_NE(std::find(allCtx.begin(), allCtx.end(), ctxXY), allCtx.end());
	EXPECT_NE(std::find(allCtx.begin(), allCtx.end(), globalCtx), allCtx.end());
}

TEST(ContextTest, KLimitTest)
{
	auto testModule = parseAssembly(
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32*, align 4\n"
		"  %y = alloca i32*, align 4\n"
		"  %z = alloca i32*, align 4\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto itr = testModule->begin()->begin()->begin();
	auto x = itr;
	auto y = ++itr;
	auto z = ++itr;

	KLimitContext::defaultLimit = 2;

	auto ctx0 = Context::getGlobalContext();
	auto ctx1 = KLimitContext::pushContext(ctx0, x, nullptr);
	auto ctx2 = KLimitContext::pushContext(ctx1, y, nullptr);
	EXPECT_NE(ctx2, ctx1);

	auto ctx3 = KLimitContext::pushContext(ctx2, z, nullptr);
	EXPECT_EQ(ctx3, ctx2);

	auto ctx4 = KLimitContext::pushContext(ctx0, y, nullptr);
	EXPECT_NE(ctx4, ctx2);
}

TEST(ContextTest, AdaptiveContextTest)
{
	auto testModule = parseAssembly(
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32*, align 4\n"
		"  %y = alloca i32*, align 4\n"
		"  %z = alloca i32*, align 4\n"
		"  ret i32 0\n"
		"}\n"
	);

	auto itr = testModule->begin()->begin()->begin();
	auto x = itr;
	auto y = ++itr;
	auto z = ++itr;

	auto ctx0 = Context::getGlobalContext();
	EXPECT_EQ(ctx0, AdaptiveContext::pushContext(ctx0, x, nullptr));
	EXPECT_EQ(ctx0, AdaptiveContext::pushContext(ctx0, y, nullptr));
	EXPECT_EQ(ctx0, AdaptiveContext::pushContext(ctx0, z, nullptr));

	AdaptiveContext::trackCallSite(ProgramLocation(ctx0, x));
	auto ctx1 = AdaptiveContext::pushContext(ctx0, x, nullptr);
	EXPECT_NE(ctx1, ctx0);
	EXPECT_EQ(ctx0, AdaptiveContext::pushContext(ctx0, y, nullptr));
	EXPECT_EQ(ctx0, AdaptiveContext::pushContext(ctx0, z, nullptr));
	EXPECT_EQ(ctx1, AdaptiveContext::pushContext(ctx1, x, nullptr));
	EXPECT_EQ(ctx1, AdaptiveContext::pushContext(ctx1, y, nullptr));
	EXPECT_EQ(ctx1, AdaptiveContext::pushContext(ctx1, z, nullptr));

	AdaptiveContext::trackCallSite(ProgramLocation(ctx1, y));
	auto ctx2 = AdaptiveContext::pushContext(ctx1, y, nullptr);
	EXPECT_NE(ctx2, ctx0);
	EXPECT_NE(ctx2, ctx1);
	EXPECT_EQ(ctx1, AdaptiveContext::pushContext(ctx1, x, nullptr));
	EXPECT_EQ(ctx1, AdaptiveContext::pushContext(ctx1, z, nullptr));	
	EXPECT_EQ(ctx2, AdaptiveContext::pushContext(ctx2, x, nullptr));
	EXPECT_EQ(ctx2, AdaptiveContext::pushContext(ctx2, y, nullptr));
	EXPECT_EQ(ctx2, AdaptiveContext::pushContext(ctx2, z, nullptr));
}

}
#include "PointerAnalysis/ControlFlow/PointerProgramBuilder.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "Utils/ParseLLVMAssembly.h"

#include "gtest/gtest.h"

using namespace tpa;
using namespace llvm;

namespace
{

TEST(ControlFlowTest, PointerCFGTest)
{
	auto testModule = parseAssembly(
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32, align 4"
		"  %y = alloca i32, align 4"
		"  %z = alloca i32*, align 8"
		"  store i32* %y, i32** %z"
		"  %w = load i32** %z"
		"  ret i32 0\n"
		"}\n"
	);

	auto mainFunc = testModule->begin();
	auto itr = mainFunc->begin()->begin();
	auto v1 = itr++;
	auto v2 = itr++;
	auto v3 = itr++;
	auto v4 = itr++;
	auto v5 = itr++;
	auto v6 = itr++;

	auto cfg = PointerCFG(mainFunc);

	auto node1 = cfg.create<AllocNode>(v1);
	auto node2 = cfg.create<AllocNode>(v2);
	auto node3 = cfg.create<CopyNode>(v3, v1, 0, false);
	auto node4 = cfg.create<StoreNode>(v4, v3, v2);
	auto node5 = cfg.create<LoadNode>(v5, v3);
	auto node6 = cfg.create<ReturnNode>(v6, v5);

	cfg.setExitNode(node6);

	cfg.getEntryNode()->insertEdge(node1);
	node1->insertEdge(node2);
	node2->insertEdge(node3);
	node2->insertEdge(node4);
	node3->insertEdge(node5);
	node4->insertEdge(node5);
	node5->insertEdge(node6);

	EXPECT_EQ(node1->pred_size(), 1u);
	EXPECT_EQ(node1->succ_size(), 1u);
	EXPECT_EQ(node2->pred_size(), 1u);
	EXPECT_EQ(node2->succ_size(), 2u);
	EXPECT_EQ(node3->pred_size(), 1u);
	EXPECT_EQ(node3->succ_size(), 1u);
	EXPECT_EQ(node4->pred_size(), 1u);
	EXPECT_EQ(node4->succ_size(), 1u);
	EXPECT_EQ(node5->pred_size(), 2u);
	EXPECT_EQ(node5->succ_size(), 1u);
	EXPECT_EQ(node6->pred_size(), 1u);
	EXPECT_EQ(node6->succ_size(), 0u);

	node4->removeEdge(node5);
	EXPECT_EQ(node5->pred_size(), 1u);
	EXPECT_EQ(node4->succ_size(), 0u);
}

TEST(ControlFlowTest, PointerCFGBuilderTest)
{
	auto testModule = parseAssembly(
		"define i32 @main() {\n"
		"bb:\n"
		"  %x = alloca i32, align 4\n"
		"  %y = alloca i32, align 4\n"
		"  %a = alloca i32*, align 8\n"
		"  %b = alloca i32*, align 8\n"
		"  %tmp1 = icmp ne i32 1, 0\n"
		"  br i1 %tmp1, label %bb1, label %bb2\n"
		"bb1:\n"
		"  store i32* %x, i32** %a, align 8\n"
		"  br label %bb3\n"
		"bb2:\n"
		"  store i32* %y, i32** %b, align 8\n"
		"  br label %bb3\n"
		"bb3:\n"
		"  %c = phi i32** [%a, %bb1], [%b, %bb2]"
		"  %tmp3 = load i32** %c, align 8\n"
		"  %tmp4 = load i32* %tmp3, align 4\n"
		"  ret i32 %tmp4\n"
		"}\n"
	);

	auto builder = PointerProgramBuilder();
	auto prog = builder.buildPointerProgram(*testModule);
	auto cfg = prog.getPointerCFG(testModule->begin());
	EXPECT_EQ(cfg, prog.getEntryCFG());

	EXPECT_EQ(cfg->getNumNodes(), 10u);

	auto entry = cfg->getEntryNode();
	EXPECT_TRUE(isa<EntryNode>(entry));
	
	auto node1 = *entry->succ_begin();
	EXPECT_TRUE(node1 != nullptr);
	EXPECT_TRUE(isa<AllocNode>(node1));
	EXPECT_EQ(cast<AllocNode>(node1)->getAllocType(), Type::getInt32Ty(testModule->getContext()));
	EXPECT_EQ(node1->succ_size(), 1u);

	auto node2 = *node1->succ_begin();
	EXPECT_TRUE(isa<AllocNode>(node2));
	EXPECT_EQ(node2->succ_size(), 1u);

	auto node3 = *node2->succ_begin();
	EXPECT_TRUE(isa<AllocNode>(node3));
	EXPECT_EQ(node3->succ_size(), 1u);

	auto node4 = *node3->succ_begin();
	EXPECT_TRUE(isa<AllocNode>(node4));
	EXPECT_EQ(node4->succ_size(), 2u);

	auto node5 = *node4->succ_begin();
	EXPECT_TRUE(isa<StoreNode>(node5));
	EXPECT_EQ(node5->succ_size(), 1u);
	auto node6 = *(node4->succ_begin() + 1);
	EXPECT_TRUE(isa<StoreNode>(node6));
	EXPECT_EQ(node6->succ_size(), 1u);

	auto node7 = *node5->succ_begin();
	EXPECT_TRUE(isa<CopyNode>(node7));
	EXPECT_EQ(node7->succ_size(), 1u);
	EXPECT_EQ(node7->pred_size(), 2u);
	EXPECT_EQ(*node6->succ_begin(), node7);

	auto node8 = *node7->succ_begin();
	EXPECT_TRUE(isa<LoadNode>(node8));
	EXPECT_EQ(node8->succ_size(), 1u);

	auto node9 = *node8->succ_begin();
	EXPECT_TRUE(isa<ReturnNode>(node9));
	EXPECT_EQ(node9->succ_size(), 0u);
	EXPECT_EQ(cfg->getExitNode(), node9);
}

}

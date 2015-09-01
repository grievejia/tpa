#include "Context/Context.h"
#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/TypeMap.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static bool isScalarNonPointerType(const Type* type)
{
	assert(type != nullptr);
	return type->isSingleValueType() && !type->isPointerTy();
}

GlobalPointerAnalysis::GlobalPointerAnalysis(PointerManager& p, MemoryManager& m, const TypeMap& t): ptrManager(p), memManager(m), typeMap(t), globalCtx(context::Context::getGlobalContext()) {}

void GlobalPointerAnalysis::createGlobalVariables(const Module& module, Env& env)
{
	for (auto const& gVar: module.globals())
	{
		// Create the pointer first
		auto gPtr = ptrManager.getOrCreatePointer(globalCtx, &gVar);

		// Create the memory object
		auto gType = gVar.getType()->getPointerElementType();
		auto typeLayout = typeMap.lookup(gType);
		assert(typeLayout != nullptr);
		auto gObj = memManager.allocateGlobalMemory(&gVar, typeLayout);

		// Now add the top-level mapping
		env.insert(gPtr, gObj);
	}
}

void GlobalPointerAnalysis::createFunctions(const llvm::Module& module, Env& env)
{
	for (auto const& f: module)
	{
		// For each function, regardless of whether it is internal or external, and regardless of whether it has its address taken or not, we are going to create a function pointer and a function object for it
		auto fPtr = ptrManager.getOrCreatePointer(globalCtx, &f);
		auto fObj = memManager.allocateMemoryForFunction(&f);

		// Add the top-level mapping
		env.insert(fPtr, fObj);
	}
}

const MemoryObject* GlobalPointerAnalysis::getGlobalObject(const GlobalValue* gv, const Env& env)
{
	auto iPtr = ptrManager.getPointer(globalCtx, gv);
	assert(iPtr != nullptr && "gv ptr not found");
	auto iSet = env.lookup(iPtr);
	assert(iSet.size() == 1 && "Cannot find pSet of gv ptr!");
	auto obj = *iSet.begin();
	assert(obj != nullptr);
	return obj;
}

void GlobalPointerAnalysis::initializeGlobalValues(const llvm::Module& module, EnvStore& envStore)
{
	DataLayout dataLayout(&module);
	for (auto const& gVar: module.globals())
	{
		auto gObj = getGlobalObject(&gVar, envStore.first);

		if (gVar.hasInitializer())
		{
			processGlobalInitializer(gObj, gVar.getInitializer(), envStore, dataLayout);
		}
		else
		{
			// If gVar doesn't have an initializer, since we are assuming a whole-program analysis, the value must be external (e.g. struct FILE* stdin)
			// To be conservative, assume that those "external" globals can points to anything
			envStore.second.strongUpdate(gObj, PtsSet::getSingletonSet(MemoryManager::getUniversalObject()));
		}
	}
}

std::pair<const llvm::GlobalVariable*, size_t> GlobalPointerAnalysis::processConstantGEP(const llvm::ConstantExpr* cexpr, const DataLayout& dataLayout)
{
	assert(cexpr->getOpcode() == llvm::Instruction::GetElementPtr);
	
	auto baseVal = cexpr->getOperand(0);
	auto indexes = llvm::SmallVector<llvm::Value*, 4>(cexpr->op_begin() + 1, cexpr->op_end());
	unsigned offset = dataLayout.getIndexedOffset(baseVal->getType(), indexes);

	// The loop is written for bitcast handling
	while (true)
	{
		if (auto gVar = dyn_cast<GlobalVariable>(baseVal))
		{
			return std::make_pair(gVar, offset);
		}
		else if (auto ce = dyn_cast<ConstantExpr>(baseVal))
		{
			switch (ce->getOpcode())
			{
				case Instruction::GetElementPtr:
				{
					// Accumulate offset
					auto baseOffsetPair = processConstantGEP(ce, dataLayout);
					return std::make_pair(baseOffsetPair.first, baseOffsetPair.second + offset);
				}
				case Instruction::IntToPtr:
					return std::make_pair(nullptr, 0);
				case Instruction::BitCast:
					baseVal = ce->getOperand(0);
					// Don't return. Keep looping on baseVal
					break;
				default:
					errs() << "Constant expr not yet handled in global intializer: " << *ce << "\n";
					llvm_unreachable("Unknown constantexpr!");
			}
		}
		else
		{
			llvm::errs() << *baseVal << "\n";
			llvm_unreachable("Unknown constant gep base!");
		}
	}
}

void GlobalPointerAnalysis::processGlobalScalarInitializer(const MemoryObject* gObj, const llvm::Constant* initializer, EnvStore& envStore, const DataLayout& dataLayout)
{
	if (!initializer->getType()->isPointerTy())
		return;

	if (initializer->isNullValue())
		envStore.second.insert(gObj, memManager.getNullObject());
	else if (isa<UndefValue>(initializer))
		envStore.second.strongUpdate(gObj, PtsSet::getSingletonSet(MemoryManager::getUniversalObject()));
	else if (isa<GlobalVariable>(initializer) || isa<Function>(initializer))
	{
		auto gv = cast<GlobalValue>(initializer);
		auto tgtObj = getGlobalObject(gv, envStore.first);
		envStore.second.insert(gObj, tgtObj);
	}
	else if (auto ce = dyn_cast<ConstantExpr>(initializer))
	{
		switch (ce->getOpcode())
		{
			case Instruction::GetElementPtr:
			{
				// Offset calculation
				auto baseOffsetPair = processConstantGEP(ce, dataLayout);
				if (baseOffsetPair.first == nullptr)
					envStore.second.strongUpdate(gObj, PtsSet::getSingletonSet(MemoryManager::getUniversalObject()));
				else
				{
					auto tgtObj = getGlobalObject(baseOffsetPair.first, envStore.first);
					auto offsetObj = memManager.offsetMemory(tgtObj, baseOffsetPair.second);
					envStore.second.insert(gObj, offsetObj);
				}
				break;
			}
			case Instruction::IntToPtr:
			{
				// By default, clang won't generate global pointer arithmetic as ptrtoint+inttoptr, so we will do the simplest thing here
				envStore.second.insert(gObj, MemoryManager::getUniversalObject());

				break;
			}
			case Instruction::BitCast:
			{
				processGlobalInitializer(gObj, ce->getOperand(0), envStore, dataLayout);
				break;
			}
			default:
				break;
		}
	}
	else
	{
		llvm::errs() << *initializer << "\n";
		llvm_unreachable("Unsupported constant pointer!");
	}
}

void GlobalPointerAnalysis::processGlobalStructInitializer(const MemoryObject* gObj, const llvm::Constant* initializer, EnvStore& envStore, const DataLayout& dataLayout)
{
	auto stType = cast<StructType>(initializer->getType());

	// Structs are treated field-sensitively
	auto stLayout = dataLayout.getStructLayout(stType);
	for (unsigned i = 0, e = initializer->getNumOperands(); i != e; ++i)
	{
		unsigned offset = stLayout->getElementOffset(i);

		const Constant* subInitializer = nullptr;
		if (auto caz = dyn_cast<ConstantAggregateZero>(initializer))
			subInitializer = caz->getStructElement(i);
		else if (auto undef = dyn_cast<UndefValue>(initializer))
			subInitializer = undef->getStructElement(i);
		else
			subInitializer = cast<Constant>(initializer->getOperand(i));
		
		auto subInitType = subInitializer->getType();
		if (isScalarNonPointerType(subInitType))
			// Not an interesting field. Skip it.
			continue;
		
		auto offsetObj = memManager.offsetMemory(gObj, offset);
		processGlobalInitializer(offsetObj, subInitializer, envStore, dataLayout);
	}
}

void GlobalPointerAnalysis::processGlobalArrayInitializer(const MemoryObject* gObj, const llvm::Constant* initializer, EnvStore& envStore, const DataLayout& dataLayout)
{
	auto arrayType = cast<ArrayType>(initializer->getType());
	auto elemType = arrayType->getElementType();

	if (!isScalarNonPointerType(elemType))
	{
		// Arrays/vectors are collapsed into a single element
		for (unsigned i = 0, e = initializer->getNumOperands(); i < e; ++i)
		{
			const Constant* elem = nullptr;
			if (auto caz = dyn_cast<ConstantAggregateZero>(initializer))
				elem = caz->getSequentialElement();
			else if (auto undef = dyn_cast<UndefValue>(initializer))
				elem = undef->getSequentialElement();
			else
				elem = cast<Constant>(initializer->getOperand(i));

			processGlobalInitializer(gObj, elem, envStore, dataLayout);
		}
	}
}

void GlobalPointerAnalysis::processGlobalInitializer(const MemoryObject* gObj, const Constant* initializer, EnvStore& envStore, const DataLayout& dataLayout)
{
	if (initializer->getType()->isSingleValueType())
		processGlobalScalarInitializer(gObj, initializer, envStore, dataLayout);
	else if (initializer->getType()->isStructTy())
		processGlobalStructInitializer(gObj, initializer, envStore, dataLayout);
	else if (initializer->getType()->isArrayTy())
		processGlobalArrayInitializer(gObj, initializer, envStore, dataLayout);
	else
	{
		errs() << "initializer = " << *initializer << "\n";
		llvm_unreachable("Unknown initializer type");
	}
}

void GlobalPointerAnalysis::initializeSpecialPointerObject(const Module& module, EnvStore& envStore)
{
	auto uPtr = ptrManager.setUniversalPointer(UndefValue::get(Type::getInt8PtrTy(module.getContext())));
	auto uLoc = MemoryManager::getUniversalObject();
	envStore.first.insert(uPtr, uLoc);
	envStore.second.insert(uLoc, uLoc);
	auto nPtr = ptrManager.setNullPointer(ConstantPointerNull::get(Type::getInt8PtrTy(module.getContext())));
	auto nLoc = memManager.getNullObject();
	envStore.first.insert(nPtr, nLoc);
}

std::pair<Env, Store> GlobalPointerAnalysis::runOnModule(const Module& module)
{
	EnvStore envStore;

	// Set up the points-to relations of uPtr, uObj and nullPtr
	initializeSpecialPointerObject(module, envStore);

	// TODO: Fix this file after finishing typeMap

	// First, scan through all the global values and register them in ptrManager. This scan should precede varaible initialization because the initialization may refer to another global value defined "below" it
	createGlobalVariables(module, envStore.first);
	createFunctions(module, envStore.first);

	// After all the global values are defined, go ahead and process the initializers
	initializeGlobalValues(module, envStore);

	// I'm not sure whether RVO will trigger in this case. So to be safe I'll just use move construction.
	return envStore;
}

}
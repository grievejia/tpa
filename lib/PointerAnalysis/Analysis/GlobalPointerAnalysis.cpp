#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace tpa;
using namespace llvm;

GlobalPointerAnalysis::GlobalPointerAnalysis(PointerManager& p, MemoryManager& m): ptrManager(p), memManager(m), dataLayout(memManager.getDataLayout()), globalCtx(Context::getGlobalContext()) {}

void GlobalPointerAnalysis::registerGlobalValues(Env& env, const Module& module)
{
	// Scan the global variables first
	for (auto const& gVal: module.globals())
	{
		// Create the pointer first
		auto gPtr = ptrManager.getOrCreatePointer(globalCtx, &gVal);

		// Create the memory object
		auto gType = gVal.getType()->getPointerElementType();
		auto gObj = memManager.allocateMemory(ProgramLocation(globalCtx, &gVal), gType);
		auto gLoc = memManager.offsetMemory(gObj, 0);

		// Now add the top-level mapping
		env.insert(gPtr, gLoc);
	}

	// Next, scan the functions
	for (auto const& f: module)
	{
		// For each function, regardless of whether it is internal or external, and regardless of whether it has its address taken or not, we are going to create a function pointer and a function object for it
		auto fPtr = ptrManager.getOrCreatePointer(globalCtx, &f);
		auto fObj = memManager.createMemoryObjectForFunction(&f);
		auto fLoc = memManager.offsetMemory(fObj, 0);

		// Add the top-level mapping
		env.insert(fPtr, fLoc);
	}
}

void GlobalPointerAnalysis::initializeGlobalValues(Env& env, Store& store, const llvm::Module& module)
{
	for (auto const& gVal: module.globals())
	{
		auto gPtr = ptrManager.getPointer(globalCtx, &gVal);
		assert(gPtr != nullptr && "Cannot find global ptr!");
		auto gSet = env.lookup(gPtr);
		assert(gSet.getSize() == 1 && "Cannot find pSet of global ptr!");
		auto gLoc = *gSet.begin();
		assert(gLoc != nullptr);

		if (gVal.hasInitializer())
		{
			processGlobalInitializer(gLoc, gVal.getInitializer(), env, store);
		}
		else
		{
			// If gVal doesn't have an initializer, since we are assuming a whole-program analysis, the value must be external (e.g. struct FILE* stdin)
			// To be conservative, assume that those "external" globals can points to anything
			store.insert(gLoc, memManager.getUniversalLocation());
		}
	}
}

void GlobalPointerAnalysis::processGlobalInitializer(const MemoryLocation* gLoc, const Constant* initializer, const Env& env, Store& store)
{
	if (initializer->isNullValue())
	{
		// Make gLoc points to null Loc
		// This is how we differentiate undefined values and null pointers: null-pointers' pts-to sets are not empty, yet undefined values are
		if (initializer->getType()->isPointerTy())
			store.insert(gLoc, memManager.getNullLocation());
		else if (auto caz = dyn_cast<ConstantAggregateZero>(initializer))
		{
			// Examine the type of initializer and expand it accordingly
			auto aggType = initializer->getType();
			if (auto stType = dyn_cast<StructType>(aggType))
			{
				auto stLayout = dataLayout.getStructLayout(stType);
				for (unsigned i = 0, e = stType->getNumElements(); i != e; ++i)
				{
					unsigned offset = stLayout->getElementOffset(i);
					auto elemLoc = memManager.offsetMemory(gLoc, offset);
					processGlobalInitializer(elemLoc, caz->getStructElement(i), env, store);
				}
			}
			else if (aggType->isArrayTy() || aggType->isVectorTy())
			{
				// Array elements are treated as one
				auto seqElem = caz->getSequentialElement();
				processGlobalInitializer(gLoc, seqElem, env, store);
			}
		}
	}
	else if (initializer->getType()->isSingleValueType())
	{
		if (initializer->getType()->isPointerTy())
		{
			if (isa<GlobalValue>(initializer))
			{
				auto iPtr = ptrManager.getPointer(globalCtx, initializer);
				assert(iPtr != nullptr && "rhs ptr not found");
				auto iSet = env.lookup(iPtr);
				assert(!iSet.isEmpty() && "Cannot find pSet of global ptr!");

				store.weakUpdate(gLoc, iSet);
			}
			else if (auto ce = dyn_cast<ConstantExpr>(initializer))
			{
				switch (ce->getOpcode())
				{
					case Instruction::GetElementPtr:
					{
						// Offset calculation
						auto baseVal = ce->getOperand(0);
						auto indexes = SmallVector<Value*, 4>(ce->op_begin() + 1, ce->op_end());
						unsigned offset = dataLayout.getIndexedOffset(baseVal->getType(), indexes);

						auto offsetLoc = processConstantGEP(cast<Constant>(baseVal), offset, env, store);
						assert(offsetLoc != memManager.getUniversalLocation());

						store.insert(gLoc, offsetLoc);
						break;
					}
					case Instruction::IntToPtr:
					{
						// By default, clang won't generate global pointer arithmetic as ptrtoint+inttoptr, so we will do the simplest thing here
						store.insert(gLoc, memManager.getUniversalLocation());

						break;
					}
					case Instruction::BitCast:
					{
						processGlobalInitializer(gLoc, ce->getOperand(0), env, store);
						break;
					}
					default:
						errs() << "Constant expr not yet handled in global intializer: " << *ce << "\n";
						llvm_unreachable(0);
				}
			}
			else
			{
				errs() << *initializer << "\n";
				llvm_unreachable("Unknown constant pointer!");
			}
		}
	}
	else if (isa<ConstantStruct>(initializer))
	{
		// Structs are treated field-sensitively
		auto stType = cast<StructType>(initializer->getType());
		auto stLayout = dataLayout.getStructLayout(stType);
		for (unsigned i = 0, e = initializer->getNumOperands(); i != e; ++i)
		{
			unsigned offset = stLayout->getElementOffset(i);
			auto subInitializer = cast<Constant>(initializer->getOperand(i));
			auto subInitType = subInitializer->getType();
			if (subInitType->isSingleValueType() && !subInitType->isPointerTy())
				// Not an interesting field. Skip it.
				// Even with this skip here, its is still possible that we might create redundant memory objects of struct/array type.
				continue;
			auto elemLoc = memManager.offsetMemory(gLoc, offset);
			processGlobalInitializer(elemLoc, subInitializer, env, store);
		}
	}
	else if (isa<ConstantArray>(initializer) || isa<ConstantDataSequential>(initializer) || isa<ConstantVector>(initializer))
	{
		// Arrays/vectors are collapsed into a single element
		for (unsigned i = 0, e = initializer->getNumOperands(); i != e; ++i)
		{
			auto elem = cast<Constant>(initializer->getOperand(i));
			processGlobalInitializer(gLoc, elem, env, store);
		}
	}
	else if (!isa<UndefValue>(initializer))
	{
		errs() << "Unknown initializer: " << *initializer << "\n";
		assert(false && "Not supported yet");
	}
}

const MemoryLocation* GlobalPointerAnalysis::processConstantGEP(const llvm::Constant* base, size_t offset, const Env& env, Store& store)
{
	assert(base->getType()->isPointerTy());

	if (isa<GlobalValue>(base))
	{
		auto iPtr = ptrManager.getPointer(globalCtx, base);
		assert(iPtr != nullptr && "rhs ptr not found");
		auto iSet = env.lookup(iPtr);
		assert(iSet.getSize() == 1 && "rhs obj not found");
		auto iLoc = *iSet.begin();

		return memManager.offsetMemory(iLoc, offset);
	}
	else if (auto ce = dyn_cast<ConstantExpr>(base))
	{
		switch (ce->getOpcode())
		{
			case Instruction::GetElementPtr:
			{
				auto baseVal = ce->getOperand(0);
				auto indexes = SmallVector<Value*, 4>(ce->op_begin() + 1, ce->op_end());
				unsigned newOffset = dataLayout.getIndexedOffset(baseVal->getType(), indexes);

				auto iLoc = processConstantGEP(cast<Constant>(baseVal), newOffset, env, store);
				assert(iLoc != nullptr && "rhs obj not found");

				return memManager.offsetMemory(iLoc, offset);
			}
			case Instruction::IntToPtr:
				return memManager.getUniversalLocation();
			case Instruction::BitCast:
				return processConstantGEP(ce->getOperand(0), offset, env, store);
			default:
				errs() << "Constant expr not yet handled in global intializer: " << *ce << "\n";
				llvm_unreachable(0);
		}
	}
	else
	{
		errs() << *base << "\n";
		llvm_unreachable("Unknown constant pointer!");
	}
}

void GlobalPointerAnalysis::initializeMainArgs(const Module& module, Env& env, Store& store)
{
	auto entryFunc = module.getFunction("main");
	if (entryFunc == nullptr)
		llvm_unreachable("Cannot find main funciton in module");
	// Check if the main function contains any parameters (argc, argv). If so, initialize them
	if (entryFunc->arg_size() > 1)
	{
		auto argv = std::next(entryFunc->arg_begin());
		// We cannot initialize the type of argvObj and argvObjObj with the type of argvPtr. This is because if you write the type of argv as char** then argvObj and argvObjObj won't be initialized with an array type. We have to manually construct the array type char[] and char[][] at this point.
		auto charTy = Type::getInt8Ty(argv->getType()->getContext());
		auto charArrayTy = ArrayType::get(charTy, 1);
		auto charArrayArrayTy = ArrayType::get(charTy, 1);

		auto globalCtx = Context::getGlobalContext();
		auto argvObj = memManager.allocateMemory(ProgramLocation(globalCtx, entryFunc), charArrayArrayTy);
		auto argvObjObj = memManager.allocateMemory(ProgramLocation(globalCtx, entryFunc), charArrayTy);
		auto argvLoc = memManager.offsetMemory(argvObj, 0);
		auto argvObjLoc = memManager.offsetMemory(argvObjObj, 0);
		memManager.setArgv(argvLoc, argvObjLoc);

		auto argvPtr = ptrManager.getOrCreatePointer(globalCtx, argv);
		env.insert(argvPtr, argvLoc);
		store.insert(argvLoc, argvObjLoc);
	}
}

std::pair<Env, Store> GlobalPointerAnalysis::runOnModule(const Module& module)
{
	Env env;
	auto store = Store();

	// Set up the points-to relations of uPtr, uObj and nullPtr
	auto uPtr = ptrManager.getUniversalPtr();
	auto uLoc = memManager.getUniversalLocation();
	env.insert(uPtr, uLoc);
	store.insert(uLoc, uLoc);
	auto nPtr = ptrManager.getNullPtr();
	auto nLoc = memManager.getNullLocation();
	env.insert(nPtr, nLoc);

	// First, scan through all the global values and register them in ptrManager. This scan should precede varaible initialization because the initialization may refer to another global value defined "below" it
	registerGlobalValues(env, module);

	// After all the global values are defined, go ahead and process the initializers
	initializeGlobalValues(env, store, module);

	initializeMainArgs(module, env, store);

	// I'm not sure whether RVO will trigger in this case. So to be safe I'll just use move construction.
	return std::make_pair(std::move(env), std::move(store));
}

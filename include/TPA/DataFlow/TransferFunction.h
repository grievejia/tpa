#ifndef TPA_TRANSFER_FUNCTION_H
#define TPA_TRANSFER_FUNCTION_H

#include "PointerAnalysis/ControlFlow/NodeMixins.h"

#include <llvm/ADT/ArrayRef.h>

#include <utility>

namespace tpa
{

class PointerManager;
class MemoryManager;

class Env;
class Store;
class StoreManager;
class ExternalPointerEffectTable;

class PointerProgram;

class Context;

template <typename GraphType>
class TransferFunction
{
private:
	using NodeType = typename GraphType::NodeType;

	PointerManager& ptrManager;
	MemoryManager& memManager;
	StoreManager& storeManager;

	const ExternalPointerEffectTable& extTable;

	const Pointer* getPointer(const Context*, const llvm::Value*);
	bool evalMalloc(const Pointer* dst, const llvm::Instruction*, const llvm::Value* size, Env&, Store&);
	std::pair<bool, bool> evalStore(const Pointer* dst, const Pointer* src, const Env&, Store&);
	std::pair<bool, bool> evalCopy(const Pointer* dst, const Pointer* src, Env&);
public:
	TransferFunction(PointerManager& p, MemoryManager& m, StoreManager& s, const ExternalPointerEffectTable& t): ptrManager(p), memManager(m), storeManager(s), extTable(t) {}

	// Return true if env changes
	bool evalAlloc(const Context*, const AllocNodeMixin<NodeType>*, Env&);
	// The first returned boolean indicates whether the evaluation succeeded or not. The second (and third, if exists) returned boolean indicates wheter the env/store changes or not
	std::pair<bool, bool> evalCopy(const Context*, const CopyNodeMixin<NodeType>*, Env&);
	std::pair<bool, bool> evalLoad(const Context*, const LoadNodeMixin<NodeType>*, Env&, const Store&);
	std::pair<bool, bool> evalStore(const Context*, const StoreNodeMixin<NodeType>*, const Env&, Store&);
	std::vector<const llvm::Function*> resolveCallTarget(const Context*, const CallNodeMixin<NodeType>*, const Env&, const llvm::ArrayRef<const llvm::Function*>);
	std::pair<bool, bool> evalCall(const Context*, const CallNodeMixin<NodeType>*, const Context*, const GraphType&, Env&);
	std::pair<bool, bool> evalReturn(const Context*, const ReturnNodeMixin<NodeType>*, const Context*, const CallNodeMixin<NodeType>*, Env&);
	std::tuple<bool, bool, bool> applyExternal(const Context*, const CallNodeMixin<NodeType>*, Env&, Store&, const llvm::Function* callee);
};

}

#endif

#ifndef TPA_TRANSFER_FUNCTION_H
#define TPA_TRANSFER_FUNCTION_H

#include <utility>

namespace tpa
{

class PointerManager;
class MemoryManager;

class Env;
class Store;
class StoreManager;
class ExternalPointerEffectTable;

class AllocNode;
class CopyNode;
class LoadNode;
class StoreNode;
class CallNode;
class ReturnNode;
class PointerProgram;

class Context;

class TransferFunction
{
private:
	PointerManager& ptrManager;
	MemoryManager& memManager;
	StoreManager& storeManager;

	const ExternalPointerEffectTable& extTable;

	bool evalMalloc(const Pointer* dst, const llvm::Instruction*, const llvm::Value* size, Env&, Store&);
	std::pair<bool, bool> evalStore(const Pointer* dst, const Pointer* src, const Env&, Store&);
	std::pair<bool, bool> evalCopy(const Pointer* dst, const Pointer* src, Env&);
public:
	TransferFunction(PointerManager& p, MemoryManager& m, StoreManager& s, const ExternalPointerEffectTable& t): ptrManager(p), memManager(m), storeManager(s), extTable(t) {}

	// Return true if env changes
	bool evalAlloc(const Context*, const AllocNode*, Env&);
	// The first returned boolean indicates whether the evaluation succeeded or not. The second (and third, if exists) returned boolean indicates wheter the env/store changes or not
	std::pair<bool, bool> evalCopy(const Context*, const CopyNode*, Env&);
	std::pair<bool, bool> evalLoad(const Context*, const LoadNode*, Env&, const Store&);
	std::pair<bool, bool> evalStore(const Context*, const StoreNode*, const Env&, Store&);
	std::vector<const llvm::Function*> resolveCallTarget(const Context*, const CallNode*, const Env&, const PointerProgram&);
	std::pair<bool, bool> evalCall(const Context*, const CallNode*, const Context*, const PointerCFG&, Env&);
	std::pair<bool, bool> evalReturn(const Context*, const ReturnNode*, const Context*, const CallNode*, Env&);
	std::tuple<bool, bool, bool> applyExternal(const Context*, const CallNode*, Env&, Store&, const llvm::Function* callee);
};

}

#endif

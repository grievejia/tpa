#pragma once

#include <llvm/ADT/DenseMap.h>

namespace llvm
{
	class Module;
	class Value;
	class User;
}

namespace dynamic
{

class IDAssigner
{
private:
	using IDType = unsigned;
	IDType nextID;

	using MapType = llvm::DenseMap<const llvm::Value*, IDType>;
	MapType idMap;
	using RevMapType = std::vector<const llvm::Value*>;
	RevMapType revIdMap;

	bool assignValueID(const llvm::Value*);
	bool assignUserID(const llvm::User*);
public:
	IDAssigner(const llvm::Module&);

	const IDType* getID(const llvm::Value* v) const;
	const llvm::Value* getValue(IDType id) const;

	void dump() const;
};

}

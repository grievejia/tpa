#ifndef TPA_TAINT_ENTRY_H
#define TPA_TAINT_ENTRY_H

#include "Client/Taintness/SourceSink/Table/TaintDescriptor.h"

#include <cassert>

namespace client
{
namespace taint
{

class TaintEntry
{
private:
	TEnd type;
public:
	TaintEntry(TEnd t): type(t) {}
	virtual ~TaintEntry() = default;

	TEnd getEntryEnd() const { return type; }
};

class SourceTaintEntry: public TaintEntry
{
private:
	TPosition pos;
public:
	SourceTaintEntry(TPosition p): TaintEntry(TEnd::Source), pos(p) {}

	TPosition getTaintPosition() const { return pos; }

	static bool classof(const TaintEntry* e)
	{
		return e->getEntryEnd() == TEnd::Source;
	}
};

class PipeTaintEntry: public TaintEntry
{
private:
	TPosition dstPos, srcPos;
	TClass dstClass, srcClass;
public:
	PipeTaintEntry(TPosition dp, TClass dc, TPosition sp, TClass sc): TaintEntry(TEnd::Pipe), dstPos(dp), srcPos(sp), dstClass(dc), srcClass(sc) {}

	TPosition getDstPosition() const { return dstPos; }
	TClass getDstClass() const { return dstClass; }
	TPosition getSrcPosition() const { return srcPos; }
	TClass getSrcClass() const { return srcClass; }

	static bool classof(const TaintEntry* e)
	{
		return e->getEntryEnd() == TEnd::Pipe;
	}
};

class SinkTaintEntry: public TaintEntry
{
private:
	TPosition argPos;
	TClass argClass;
public:
	SinkTaintEntry(TPosition p, TClass c): TaintEntry(TEnd::Sink), argPos(p), argClass(c)
	{
		assert(!argPos.isReturnPosition());
	}

	TPosition getArgPosition() const { return argPos; }
	TClass getTaintClass() const { return argClass; }

	static bool classof(const TaintEntry* e)
	{
		return e->getEntryEnd() == TEnd::Sink;
	}
};

}
}

#endif

#ifndef TPA_TAINT_ENTRY_H
#define TPA_TAINT_ENTRY_H

#include "Client/Taintness/SourceSink/Table/TaintDescriptor.h"

#include <cassert>

namespace client
{
namespace taint
{

class SourceTaintEntry
{
private:
	TPosition pos;
public:
	SourceTaintEntry(TPosition p): pos(p) {}

	TPosition getTaintPosition() const { return pos; }
};

class PipeTaintEntry
{
private:
	TPosition dstPos, srcPos;
	TClass dstClass, srcClass;
public:
	PipeTaintEntry(TPosition dp, TClass dc, TPosition sp, TClass sc): dstPos(dp), srcPos(sp), dstClass(dc), srcClass(sc) {}

	TPosition getDstPosition() const { return dstPos; }
	TClass getDstClass() const { return dstClass; }
	TPosition getSrcPosition() const { return srcPos; }
	TClass getSrcClass() const { return srcClass; }
};

class SinkTaintEntry
{
private:
	TPosition argPos;
	TClass argClass;
public:
	SinkTaintEntry(TPosition p, TClass c): argPos(p), argClass(c)
	{
		assert(!argPos.isReturnPosition());
	}

	TPosition getArgPosition() const { return argPos; }
	TClass getTaintClass() const { return argClass; }
};

class TaintEntry
{
private:
	TEnd type;

	union
	{
		SourceTaintEntry source;
		PipeTaintEntry pipe;
		SinkTaintEntry sink;
	};

	TaintEntry(TPosition p): type(TEnd::Source)
	{
		new (&source) SourceTaintEntry(p);
	}
	TaintEntry(TPosition dp, TClass dc, TPosition sp, TClass sc): type(TEnd::Pipe)
	{
		new (&pipe) PipeTaintEntry(dp, dc, sp, sc);
	}
	TaintEntry(TPosition p, TClass c): type(TEnd::Sink)
	{
		new (&sink) SinkTaintEntry(p, c);
	}
public:

	static TaintEntry getSourceEntry(TPosition p)
	{
		return TaintEntry(p);
	}
	static TaintEntry getPipeEntry(TPosition dp, TClass dc, TPosition sp, TClass sc)
	{
		return TaintEntry(dp, dc, sp, sc);
	}
	static TaintEntry getSinkEntry(TPosition p, TClass c)
	{
		return TaintEntry(p, c);
	}

	~TaintEntry()
	{
		switch (type)
		{
			case TEnd::Source:
				source.~SourceTaintEntry();
				break;
			case TEnd::Pipe:
				pipe.~PipeTaintEntry();
			case TEnd::Sink:
				sink.~SinkTaintEntry();
		}
	}

	TEnd getEntryEnd() const { return type; }

	const SourceTaintEntry& getAsSourceEntry() const
	{
		assert(type == TEnd::Source);
		return source;
	}
	const PipeTaintEntry& getAsPipeEntry() const
	{
		assert(type == TEnd::Pipe);
		return pipe;
	}
	const SinkTaintEntry& getAsSinkEntry() const
	{
		assert(type == TEnd::Sink);
		return sink;
	}
};

}
}

#endif

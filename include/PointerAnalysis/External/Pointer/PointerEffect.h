#ifndef TPA_POINTER_EFFECT_H
#define TPA_POINTER_EFFECT_H

#include "PointerAnalysis/External/Descriptors.h"

namespace tpa
{

enum class PointerEffectType
{
	Alloc,
	Copy,
};

class PointerAllocEffect
{
private:
	APosition sizePos;
	bool hasSize;
public:
	PointerAllocEffect(): sizePos(APosition::getReturnPosition()), hasSize(false) {}
	PointerAllocEffect(const APosition& p): sizePos(p), hasSize(true) {}

	bool hasArgPosition() const { return hasSize; }
	APosition getSizePosition() const
	{
		assert(hasSize);
		return sizePos;
	}
};

class CopySource
{
public:
	enum class SourceType: std::uint8_t
	{
		ArgValue,
		ArgDirectMemory,
		ArgReachableMemory,
		Null,
		Universal,
		Static,
	};
private:
	SourceType type;
	APosition pos;

	CopySource(SourceType t, const APosition& p): type(t), pos(p) {}
public:

	SourceType getType() const { return type; }
	APosition getPosition() const
	{
		assert(pos.isArgPosition());
		return pos;
	}

	static CopySource getArgValue(const APosition& p)
	{
		assert(p.isArgPosition());
		return CopySource(SourceType::ArgValue, p);
	}
	static CopySource getArgDirectMemory(const APosition& p)
	{
		assert(p.isArgPosition());
		return CopySource(SourceType::ArgDirectMemory, p);
	}
	static CopySource getArgRechableMemory(const APosition& p)
	{
		assert(p.isArgPosition());
		return CopySource(SourceType::ArgReachableMemory, p);
	}
	static CopySource getNullPointer()
	{
		return CopySource(SourceType::Null, APosition::getReturnPosition());
	}
	static CopySource getUniversalPointer()
	{
		return CopySource(SourceType::Universal, APosition::getReturnPosition());
	}
	static CopySource getStaticPointer()
	{
		return CopySource(SourceType::Static, APosition::getReturnPosition());
	}
};

class PointerCopyEffect
{
private:
	APosition dstPos;
	CopySource srcPos;
public:
	PointerCopyEffect(APosition d, CopySource s): dstPos(d), srcPos(s) {}

	APosition getDest() const { return dstPos; }
	CopySource getSource() const { return srcPos; }
};

class PointerEffect
{
private:
	PointerEffectType type;

	union
	{
		PointerAllocEffect alloc;
		PointerCopyEffect copy;
	};

	PointerEffect(): type(PointerEffectType::Alloc)
	{
		new (&alloc) PointerAllocEffect();
	}
	PointerEffect(const APosition& a): type(PointerEffectType::Alloc)
	{
		new (&alloc) PointerAllocEffect(a);
	}
	PointerEffect(const APosition& d, const CopySource& s): type(PointerEffectType::Copy)
	{
		new (&copy) PointerCopyEffect(d, s);
	}
public:
	PointerEffectType getType() const { return type; }

	const PointerAllocEffect& getAsAllocEffect() const
	{
		assert(type == PointerEffectType::Alloc);
		return alloc;
	}

	const PointerCopyEffect& getAsCopyEffect() const
	{
		assert(type == PointerEffectType::Copy);
		return copy;
	}

	static PointerEffect getAllocEffect()
	{
		return PointerEffect();
	}

	static PointerEffect getAllocEffect(const APosition& sizePos)
	{
		return PointerEffect(sizePos);
	}

	static PointerEffect getCopyEffect(const APosition& d, const CopySource& s)
	{
		return PointerEffect(d, s);
	}

	~PointerEffect()
	{
		switch (type)
		{
			case PointerEffectType::Alloc:
				alloc.~PointerAllocEffect();
				break;
			case PointerEffectType::Copy:
				copy.~PointerCopyEffect();
				break;
		}
	}
};

}

#endif

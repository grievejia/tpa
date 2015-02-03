#ifndef TPA_STL_UTILS_H
#define TPA_STL_UTILS_H

#include <functional>
#include <type_traits>

namespace tpa
{

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// This is a generalize hasher for all STL containers (and all custom containers that exports value_type typedef). For some reasons, C++11 doesn't specialize std::hash for key types that are containers themselves. This hasher tries to mimic what boost::hash_combine and boost::hash_range does.
template <class ContainerType>
class ContainerHasher
{
public:
	using value_type = typename ContainerType::value_type;

	std::size_t operator()(const ContainerType& c) const
	{
		std::size_t seed = 0;
		for (auto elem: c)
			hash_combine(seed, elem);
		return seed;
	}
};

template <class T, class U>
struct PairHasher
{
	using PairType = std::pair<T, U>;

	std::size_t operator()(const PairType& p) const
	{
		std::size_t seed = 0;
		hash_combine(seed, p.first);
		hash_combine(seed, p.second);
		return seed;
	}
};

template <typename EnumClassType>
struct EnumClassHasher
{
	using RealType = std::underlying_type_t<EnumClassType>;
	std::size_t operator()(const EnumClassType& e) const
	{
		return std::hash<RealType>()(static_cast<RealType>(e));
	}
};

}

#endif

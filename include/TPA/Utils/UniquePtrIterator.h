#ifndef TPA_UNIQUE_PTR_ITERATOR_H
#define TPA_UNIQUE_PTR_ITERATOR_H

#include <iterator>

namespace tpa
{

// An iterator adapter that takes an iterator over a container of unique_ptrs and returns the corresponding iterator that automatically invoke the get() method of the unique_ptr
// Such an adapter is useful when we want to hide the ownership detail of a container when exposing its iterators 
template <class Iterator>
class UniquePtrIterator: public std::iterator<typename std::iterator_traits<Iterator>::iterator_category, typename std::iterator_traits<Iterator>::value_type::pointer>
{
private:
	Iterator itr;

	using pointer = typename std::iterator_traits<UniquePtrIterator>::pointer;
	using value_type = typename std::iterator_traits<Iterator>::value_type::pointer;
public:
	UniquePtrIterator() {}
	explicit UniquePtrIterator(const Iterator& i): itr(i) {}

	bool operator== (const UniquePtrIterator& other) { return itr == other.itr; }
	bool operator!= (const UniquePtrIterator& other) { return !(*this == other); }

	// Returning a value type here means that we cannot use bind the iterator to auto&. Be aware of the problem here.
	value_type operator* () { return itr->get(); }
	const value_type operator* () const { return itr->get(); }

	pointer operator->() { return &itr->get(); }
	const pointer operator->() const { return &itr->get(); }

	// Pre-increment
	UniquePtrIterator& operator++() { ++itr; return *this; }
	// Post-increment
	UniquePtrIterator operator++(int)
	{
		auto ret = *this;
		++itr;
		return ret;
	}

	// Pre-decrement
	UniquePtrIterator& operator--() { --itr; return *this; }
	// Post-decrement
	UniquePtrIterator operator--(int)
	{
		auto ret = *this;
		--itr;
		return ret;
	}
};

}

#endif

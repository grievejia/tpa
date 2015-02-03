#ifndef TPA_WORKLIST_H
#define TPA_WORKLIST_H

#include <cassert>
#include <queue>
#include <unordered_set>

// This worklist implementation is a simple FIFO queue with an additional set to help remove duplicate entries
template <typename ElemType, typename Hasher = std::hash<ElemType>>
class WorkList
{
private:
	// The FIFO queue
	std::queue<ElemType> list;
	// Avoid duplicate entries in FIFO queue
	std::unordered_set<ElemType, Hasher> set;
public:
	WorkList() {}
	bool enqueue(ElemType elem)
	{
		if (!set.count(elem))
		{
			list.push(elem);
			set.insert(elem);
			return true;
		}
		else
			return false;
	}
	ElemType dequeue()
	{
		assert(!list.empty() && "Trying to dequeue an empty queue!");
		ElemType ret = list.front();
		list.pop();
		set.erase(ret);
		return ret;
	}
	bool isEmpty() const { return list.empty(); }
};

// This worklist implementation is a priority queue with an additional set to help remove duplicate entries
template <typename ElemType, typename Compare = std::less<ElemType>>
class PrioWorkList
{
private:
	// The FIFO queue
	std::priority_queue<ElemType, std::vector<ElemType>, Compare> list;
	// Avoid duplicate entries in FIFO queue
	std::unordered_set<ElemType> set;
public:
	PrioWorkList() {}

	bool enqueue(ElemType elem)
	{
		if (!set.count(elem))
		{
			list.push(elem);
			set.insert(elem);
			return true;
		}
		else
			return false;
	}
	ElemType dequeue()
	{
		assert(!list.empty() && "Trying to dequeue an empty queue!");
		ElemType ret = list.top();
		list.pop();
		set.erase(ret);
		return ret;
	}
	bool isEmpty() const { return list.empty(); }
};

#endif

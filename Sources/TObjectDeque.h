/*!
	@file TObjectDeque.h

	@brief STL implementation of a double-ended object array

	Object containers extend the basic behaviors of STL classes
	by making sure that the stored instances are deleted when items
	are "erased" from the container.

	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef TOBJECTDEQUE_H
#define TOBJECTDEQUE_H

#include <deque>
using namespace std;

#pragma mark -
#pragma mark class TObjectDeque

/*! Double-ended array for object pointers.
 *
 * A fast random-access container with fast front/back push and pop.
 *
 */
template<class T>
class TObjectDeque : public deque<T*> {

//! Iterator type for the object deque
typedef typename deque<T*>::iterator d_iterator;
//! Reverse iterator type for the object deque
typedef typename deque<T*>::reverse_iterator d_reverse_iterator;

public:

	TObjectDeque() { }

	TObjectDeque(unsigned count) {
		while (count--)
			this->push_back(new T);
	}

	//! Dispose all members during destruct
	~TObjectDeque() {
		dispose_all();
	}

	//! Clear the deque in a thread-safe manner
	void clear() {
		TObjectDeque deleteUs(*this);
		deque<T*>::clear();
	}

	//! Resize the container disposing or adding null entries as needed
	void resize(size_t newSize) {
		if (newSize < this->size()) {
			TObjectDeque deleteUs;
			deleteUs.insert(deleteUs.begin(), this->begin() + newSize, this->end());
			deque<T*>::resize(newSize);
		} else
			deque<T*>::resize(newSize);
	}

	//! Delete all the objects and set the pointers to NULL
	void dispose_all() {
		for (d_iterator itr = this->begin(); itr != this->end(); itr++) {
			delete *itr;
			*itr = NULL;
		}
	}

	//! Delete a range of items specified by indexes
	inline void dispose_range(size_t startIndex, size_t endIndex) {
		dispose_range(this->begin() + startIndex, this->begin() + endIndex + 1);
	}

	//! Delete a range of items specified by iterators
	void dispose_range(d_iterator itr_start, d_iterator itr_end) {
		for (; itr_start != itr_end; itr_start++) {
			T* item = *itr_start;
			*itr_start = NULL;
			delete item;
		}
	}

	//! Erase the item at the given index
	inline void erase(size_t index) {
		(void)this->erase(this->begin() + index);
	}

	//! Erase the items in the given range
	inline void erase(size_t startIndex, size_t endIndex) {
		(void)this->erase(this->begin() + startIndex, this->begin() + (endIndex + 1));
	}

	//! Erase one item specified by an iterator (thread-safe)
	d_iterator erase(d_iterator itr) {
		T* item = *itr;
		d_iterator itr_return = deque<T*>::erase(itr);
		delete item;
		return itr_return;
	}

	//! Erase a range of items specified by iterators (thread-safe)
	d_iterator erase(d_iterator itr1, d_iterator itr2) {
		TObjectDeque deleteUs;
		deleteUs.insert(deleteUs.begin(), itr1, itr2);
		return deque<T*>::erase(itr1, itr2);
	}

	//! Insert a set of new default constructed objects
	void insert_new(size_t startIndex, size_t count) {
		TObjectDeque newdeque(count);
		this->insert_copies(newdeque, startIndex);
	}

	//! Insert a single object pointer
	inline void insert_copy(size_t dstIndex, const T *src) {
		insert_copy(dstIndex, *src);
	}

	//! Insert a single object pointer
	inline void insert_copy(size_t dstIndex, T *src) {
		insert_copy(dstIndex, *src);
	}

	//! Insert a single object
	void insert_copy(size_t dstIndex, const T &src) {
		if (dstIndex >= this->size())
			this->push_back(new T(src));
		else if (dstIndex == 0)
			this->push_front(new T(src));
		else
			this->insert(this->begin() + dstIndex, 1, new T(src));
	}

	//! Insert a set of object pointers contained in a deque
	inline void insert_copies(const TObjectDeque &src, size_t dstIndex) {
		insert_copies(src, 0, dstIndex, src.size());
	}

	//! Insert a set of object pointers contained in a deque
	inline void insert_copies(TObjectDeque &src, size_t dstIndex) {
		insert_copies(src, 0, dstIndex, src.size());
	}

	//! Insert a set of object pointers contained in a deque
	void insert_copies(const TObjectDeque &src, size_t srcIndex, size_t dstIndex, size_t count=1) {
		this->insert(this->begin() + dstIndex, count, NULL);

		d_iterator	itr_src = ((TObjectDeque&)src).begin() + srcIndex,
					itr_dst = this->begin() + dstIndex;
		for (; count-- && itr_src != src.end(); itr_src++, itr_dst++)
			*itr_dst = new T(**itr_src);
	}

	//! Insert a set of object pointers contained in a deque
	void insert_copies(TObjectDeque &src, size_t srcIndex, size_t dstIndex, size_t count=1) {
		this->insert(this->begin() + dstIndex, count, NULL);

		d_iterator	itr_src = src.begin() + srcIndex,
					itr_dst = this->begin() + dstIndex;
		for (; count-- && itr_src != src.end(); itr_src++, itr_dst++)
			*itr_dst = new T(**itr_src);
	}

	//! Append a number of default constructed objects
	inline void append_new(size_t count) {
		while (count--)
			this->push_back(new T);
	}

	//! Append a copy constructed object
	inline void append_copy(T &src) {
		this->push_back(new T(src));
	}

	//! Append a set of objects contained in a deque
	void append_copies(TObjectDeque &src) {
		for (d_iterator itr = src.begin(); itr != src.end(); itr++)
			this->push_back( new T(**itr) );
	}

	//! Append a set of objects contained in a deque slice
	void append_copies(TObjectDeque &src, size_t srcIndex, size_t endIndex) {
		d_iterator	itr_src = src.begin() + srcIndex,
					itr_end = src.begin() + endIndex + 1;

		for (; itr_src != itr_end; itr_src++)
			this->push_back(new T(**itr_src));
	}

	//! Prepend one or more new default constructed objects
	inline void prepend_new(size_t count) {
		this->insert_new(0, count);
	}

	//! Prepend one new default constructed object
	inline void prepend_copy(T &src) {
		this->push_front(new T(src));
	}

	//! Prepend a set of objects contained in a deque
	inline void prepend_copies(TObjectDeque &src) {
		insert_copies(src, 0);
	}

	//! Prepend a set of objects contained in a deque slice
	inline void prepend_copies(TObjectDeque &src, size_t srcIndex, size_t endIndex) {
		insert_copies(src, srcIndex, 0, endIndex - srcIndex + 1);
	}

	//! Remove and dispose the given object
	void remove(T *p_data) {
		d_iterator itr = this->begin();

		while (itr != this->end()) {
			if (*itr == p_data) {
				itr = this->erase(itr);
				delete p_data;
				break;
			}
			else
				itr++;
		}
	}

	//! Print the a simple summary of the deque to stdout
	void print_contents() {
		printf("Deque %08x has %d members:\r", (unsigned long)this, this->size());

		d_iterator itr = this->begin();

		int i=1;
		while (itr != this->end()) {
			printf("Item %d = %08lx\r", i++, (unsigned long)*itr);
			itr++;
		}

		printf("----------------------------------------\r");
	}

};

#endif

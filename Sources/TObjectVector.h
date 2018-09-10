/*!
	@file TObjectVector.h

	@brief STL implementation of a double-ended object array

	Object containers extend the basic behaviors of STL classes
	by making sure that the stored instances are deleted when items
	are "erased" from the container.

	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef TOBJECTVECTOR_H
#define TOBJECTVECTOR_H

#include <vector>
using namespace std;

#pragma mark -
#pragma mark class TObjectVector

/*! Simple array for object pointers.
 *
 * A simple random-access container, fast push and pop_back.
 *
 */
template<class T>
class TObjectVector : public vector<T*> {

typedef typename vector<T*>::iterator v_iterator;

public:

	~TObjectVector() {
		dispose_all();
	}

	//! Clear the container then dispose all its original members
	void clear() {
		TObjectVector deleteUs(*this);
		vector<T*>::clear();
	}

	//! Resize the container disposing or adding null entries as needed
	void resize(size_t newSize) {
		if (newSize < this->size()) {
			TObjectVector deleteUs;
			deleteUs.insert(deleteUs.begin(), this->begin() + newSize, this->end());
			vector<T*>::resize(newSize);
		} else
			vector<T*>::resize(newSize);
	}

	//! Dispose all members and set them to null
	void dispose_all() {
		for (v_iterator itr = this->begin(); itr != this->end(); itr++) {
			T* item = *itr;
			*itr = NULL;
			delete *item;
		}
	}

	//! Dispose a range of items specified by indexes
	inline void dispose_range(size_t startIndex, size_t endIndex) {
		dispose_range(this->begin() + startIndex, this->begin() + endIndex + 1);
	}

	//! Dispose a range of items specified by iterators
	void dispose_range(v_iterator itr_start, v_iterator itr_end) {
		while (; itr_start != itr_end; itr_start++) {
			T* item = *itr_start;
			*itr_start = NULL;
			delete item;
		}
	}

	//! Erase a single item, compressing the vector
	inline void erase(size_t index) {
		(void)this->erase(this->begin() + index);
	}

	//! Erase a range of items, specified by indexes
	inline void erase(size_t startIndex, size_t endIndex) {
		(void)this->erase(this->begin() + startIndex, this->begin() + (endIndex + 1));
	}

	//! Erase one item specified by an iterator
	v_iterator erase(v_iterator itr) {
		T* item = *itr;
		v_iterator itr_return = vector<T*>::erase(itr);
		delete item;
		return itr_return;
	}

	//! Erase a range of items specified by iterators, then dispose the original members
	v_iterator erase(v_iterator itr1, v_iterator itr2) {
		TObjectVector deleteUs;
		deleteUs.insert(deleteUs.begin(), itr1, itr2);
		return vector<T*>::erase(itr1, itr2);
	}

	//! Insert a number of default constructed objects
	void insert_new(size_t startIndex, size_t count) {
		vector<T*>::insert(this->begin() + startIndex, count, (T*)NULL);

		v_iterator itr = this->begin() + startIndex;
		while (count--)
			*(itr++) = new T;
	}

	//! Insert a copy of the given const item
	inline void insert_copy(size_t dstIndex, const T *src) {
		insert_copy(dstIndex, *src);
	}

	//! Insert a copy of the given item
	inline void insert_copy(size_t dstIndex, T *src) {
		insert_copy(dstIndex, *src);
	}

	//! Insert a copy of the given item reference
	void insert_copy(size_t dstIndex, const T &src) {
		if (dstIndex >= this->size())
			push_back(new T(src));
		else if (dstIndex == 0)
			push_front(new T(src));
		else
			this->insert(this->begin() + dstIndex, 1, new T(src));
	}

	//! Insert copies of all items in the given const vector
	inline void insert_copies(const TObjectVector &src, size_t dstIndex) {
		insert_copies(src, 0, dstIndex, src.size());
	}

	//! Insert copies of all items in the given vector
	inline void insert_copies(TObjectVector &src, size_t dstIndex) {
		insert_copies(src, 0, dstIndex, src.size());
	}

	//! Insert copies of some items
	void insert_copies(const TObjectVector &src, size_t srcIndex, size_t dstIndex, size_t count=1) {
		v_iterator	itr_src = ((TObjectVector&)src).begin() + srcIndex;

		this->insert(this->begin() + dstIndex, count, NULL);

		v_iterator itr_dst = this->begin() + dstIndex;
		for (;count-- && itr_src != src.end(); itr_src++, itr_dst++)
			*itr_dst = new T(**itr_src);
	}

	//! Insert copies of some items
	void insert_copies(TObjectVector &src, size_t srcIndex, size_t dstIndex, size_t count=1) {
		v_iterator
			itr_dst = this->begin() + dstIndex,
			itr_src = src.begin() + srcIndex;

		// insert a bunch of nulls all at once
		this->insert(itr_dst, count, NULL);

		while (count--)
			*(itr_dst++) = new T(**(itr_src++));
	}

	//! Append a number of new default constructed objects
	inline void append_new(size_t count) {
		resize(this->size() + count);

		for (v_iterator itr = this->end() - count; itr != this->end(); itr++)
			*itr = new T;
	}

	//! Append a copy of the given object
	inline void append_copy(T &src) {
		push_back(new T(src));
	}

	//! Append copies of all items in the given vector
	void append_copies(TObjectVector &src) {
		for (v_iterator itr = src.begin(); itr != src.end(); itr++)
			push_back( new T(**itr) );
	}

	//! Append copies of some items in the given vector
	void append_copies(TObjectVector &src, size_t srcIndex, size_t endIndex) {
		v_iterator itr_src = src.begin() + srcIndex;

		for (;srcIndex++ <= endIndex && itr_src != src.end(); itr_src++)
			push_back(new T(**itr_src));
	}

	//! Remove and dispose the given object
	void remove(T *p_data) {
		v_iterator itr = this->begin();

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
 
	//! Print a simple contents summary to stdout
	void print_contents() {
		printf("Vector %08x has %d members:\r", (unsigned long)this, this->size());

		v_iterator itr = this->begin();

		int i=1;
		while (itr != this->end()) {
			printf("Item %d = %08x\r", i++, (unsigned long)*itr);
			itr++;
		}

		printf("----------------------------------------\r");
	}

};

#endif

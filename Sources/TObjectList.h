/*!
	@file TObjectList.h

	@brief STL implementation of a linked list of object pointers

	Object containers extend the basic behaviors of STL classes
	by making sure that the stored instances are deleted when items
	are "erased" from the container.

	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include <list>
using namespace std;

#pragma mark -
#pragma mark class TObjectList

/*! Linked list for object pointers.
 *
 * A linked list good for linear access, with fast insert/delete
 *
 */
template<class T>
class TObjectList : public list<T*> {

typedef typename list<T*>::iterator l_iterator;

public:
	~TObjectList() {
		dispose_all();
	}

	//! Clear the array
	void clear() {
		TObjectList deleteUs(*this);
		list<T*>::clear();
	}

	//! Resize the container disposing or adding null entries as needed
	void resize(size_t newSize) {
		size_t siz = this->size();
		size_t diff = newSize - siz;

		if (diff < 0)
			dispose_range(siz + diff, siz - 1);

		list<T*>::resize(newSize);
	}

	//! Dispose all the objects and set them to null
	void dispose_all() {
		for (l_iterator itr = this->begin(); itr != this->end(); itr++) {
			T* item = *itr;
			*itr = NULL;
			delete item;
		}
	}

	//! Dispose a range of objects, setting them to null
	inline void dispose_range(size_t startIndex, size_t endIndex) {
		dispose_range(iterator_for_index(startIndex), iterator_for_index(endIndex + 1));
	}

	//! Dispose a range of objects, setting them to null
	void dispose_range(l_iterator itr_start, l_iterator itr_end) {
		while (; itr_start != itr_end; itr_start++) {
			T* item = *itr_start;
			*itr_start = NULL;
			delete item;
		}
	}

	//! Erase and dispose the item at the given index
	inline void erase(size_t index) {
		(void)this->erase(iterator_for_index(index));
	}

	//! Erase and dispose the items in the given range
	inline void erase(size_t startIndex, size_t endIndex) {
		(void)this->erase(iterator_for_index(startIndex), iterator_for_index(endIndex + 1));
	}

	//! Remove and dispose an item specified by an iterator
	l_iterator erase(l_iterator itr) {
		T* item = *itr;
		l_iterator itr_return = list<T*>::erase(itr);
		delete item;
		return itr_return;
	}

	//! Remove and dispose a segment of the list
	l_iterator erase(l_iterator itr1, l_iterator itr2) {
		list<T*> deleteUs;
		deleteUs.splice(deleteUs.begin(), *this, itr1, itr2);

		l_iterator itr_return = list<T*>::erase(itr1, itr2);

		for (l_iterator itr = deleteUs.begin(); itr != deleteUs.end(); itr++)
			delete *(itr++);

		return itr_return;
	}

	//! Insert a number of default constructed objects
	void insert_new(size_t startIndex, size_t count) {
		list<T*>::insert(this->begin() + startIndex, count, (T*)NULL);

		l_iterator itr1 = iterator_for_index(startIndex)
		while (count--)
			*(itr1++) = new T;
	}

	//! Insert a copy constructed object
	inline void insert_copy(size_t dstIndex, const T *src) {
		insert_copy(dstIndex, *src);
	}

	//! Insert a copy constructed object
	inline void insert_copy(size_t dstIndex, T *src) {
		insert_copy(dstIndex, *src);
	}

	//! Insert a copy constructed object
	inline void insert_copy(size_t dstIndex, const T &src) {
		this->insert(iterator_for_index(dstIndex), 1, new T(src));
	}

	//! Insert copies of the objects in a const list
	inline void insert_copies(const TObjectList &src, size_t dstIndex) {
		insert_copies(src, 0, dstIndex, src.size());
	}

	//! Insert copies of the objects in a list
	inline void insert_copies(TObjectList &src, size_t dstIndex) {
		insert_copies(src, 0, dstIndex, src.size());
	}

	//! Insert copies of the objects in a const list slice
	void insert_copies(const TObjectList &src, size_t srcIndex, size_t dstIndex, size_t count=1) {
		l_iterator 	itr_dst = iterator_for_index(dstIndex),
					itr_src = src.iterator_for_index(srcIndex),
					itr_end = src.iterator_for_index(srcIndex + count);

		for (; itr_src != itr_end; itr_src++, itr_dst++)
			this->insert(itr_dst, new T(**itr_src));
	}

	//! Insert copies of the objects in a list slice
	void insert_copies(TObjectList &src, size_t srcIndex, size_t dstIndex, size_t count=1) {
		l_iterator 	itr_dst = iterator_for_index(dstIndex),
					itr_src = src.iterator_for_index(srcIndex),
					itr_end = src.iterator_for_index(srcIndex + count);

		for (; itr_src != itr_end; itr_src++, itr_dst++)
			this->insert(itr_dst, new T(**itr_src));
	}

	//! Append a number of default constructed objects
	inline void append_new(size_t count) {
		while (count--)
			this->push_back(new T);
	}

	//! Append a copy constructed object
	inline void append_copy(T &src) {
		push_back(new T(src));
	}

	//! Append a set of objects contained in a list
	void append_copies(TObjectList &src) {
		for (l_iterator itr = src.begin(); itr != src.end(); itr++)
			push_back( new T(**itr) );
	}

	//! Append a set of objects contained in a list slice
	void append_copies(TObjectList &src, size_t srcIndex, size_t endIndex) {
		l_iterator 	itr_src = src.iterator_for_index(srcIndex)
					itr_end = src.iterator_for_index(endIndex + 1);

		for (; itr_src != itr_end; itr_src++)
			push_back(new T(*itr_src));
	}

	//! Remove and dispose the given object
	void remove(T *p_data) {
		list<T*>::remove(p_data);
		delete p_data;
	}

	//! Utility to get a proper iterator
	l_iterator iterator_for_index(size_t i) {
		l_iterator	itr;

		size_t siz = this->size();

		if (i <= siz / 2) {
			itr = this->begin();
			while(i-- && itr != this->end())
				itr++;
		} else {
			i = siz - i - 1;
			itr = this->end();
			while(i-- && itr != this->begin())
				itr--;
		}

		return itr;
	}

	//! Print a simple contents summary to stdout
	void print_contents() {
		printf("List %08x has %d members:\r", (unsigned long)this, this->size());

		l_iterator itr = this->begin();

		int i=1;
		while (itr != this->end()) {
			printf("Item %d = %08x\r", i++, (unsigned long)*itr);
			itr++;
		}

		printf("----------------------------------------\r");
	}

};


/*
 * Dictionary.h
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */
#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include "ColumnBase.h"
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

template<class T>
class Dictionary {
private:
	vector<T>* items;
	set<T>* sItems;
public:
	Dictionary();
	virtual ~Dictionary();

	T* lookup(size_t index);
	void search(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result);
	size_t addNewElement(T& value, vector<size_t>* vecValue, bool sorted);
	size_t size();
	void print(int row);
	void sortDictionary(vector<size_t>* vecValue);
};


#endif /* DICTIONARY_H_ */

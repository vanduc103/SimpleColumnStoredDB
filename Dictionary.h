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
#include <algorithm>

using namespace std;

template<class T>
class Dictionary {
private:
	vector<T>* items;
public:
	Dictionary();
	virtual ~Dictionary();

	T* lookup(size_t index);
	void search(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result);
	size_t addNewElement(T& value, vector<size_t>& vecValue);
	size_t size();
	void print(int row);
};


#endif /* DICTIONARY_H_ */

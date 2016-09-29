/*
 * Dictionary.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */
#ifndef _Dictionary_
#define _Dictionary_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "Dictionary.h"

using namespace std;

template<class T>
Dictionary<T>::Dictionary() {
	items = new vector<T>();
}

template<class T>
T* Dictionary<T>::lookup(size_t index) {
	if (items->empty() || index < 0 || index >= items->size()) {
		return NULL;
	} else {
		return &items->at(index);
	}
}

template<class T>
bool compFunc(T value1, T value2) {
	//cout << "compare " << value1 << " , " << value2 << ". result: " << (value1 < value2) << "\n";
	return value1 < value2;
}

template<class T>
bool equalFunc(T value1, T value2) {
	return value1 == value2;
}

template<class T>
void Dictionary<T>::search(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result) {
	if (items->empty()) {
		// return -1 to show no result
		result.push_back(-1);
	} else {
		// find the lower bound for value in vector
		typename vector<T>::iterator lower;
		lower = std::lower_bound(items->begin(), items->end(), value,
				compFunc<T>);

		// based on operator to find exact position in dictionary
		switch (opType) {
		case ColumnBase::equalOp: {
			if (lower != items->end() && equalFunc(*lower, value)) {
				result.push_back(lower - items->begin());
			} else {
				// return -1 to show no result
				result.push_back(-1);
			}
			break;
		}
		case ColumnBase::neOp: {
			int exclusivePosition = -1;
			if (lower != items->end() && equalFunc(*lower, value)) {
				exclusivePosition = lower - items->begin();
			}
			// return all dictionary positions except exclusiveValue
			for (size_t i = 0; i < items->size(); i++) {
				if (i != exclusivePosition) {
					result.push_back(i);
				}
			}
			break;
		}
		case ColumnBase::ltOp: {
			// return positions from 0 to lower
			for (size_t i = 0;
					(lower == items->end()) ?
							i < items->size() : i < (lower - items->begin());
					i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::leOp: {
			unsigned int position = -1;
			if (lower == items->end()) {
				position = items->size();
			} else if (equalFunc(*lower, value)) {
				position = (lower - items->begin()) + 1;
			} else {
				position = lower - items->begin();
			}
			// return from 0 to position
			for (size_t i = 0; i < position; i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::gtOp: {
			unsigned int position = items->size();
			if (lower == items->end()) {
				// all items are less than value
				position = items->size();
			} else if (equalFunc(*lower, value)) {
				position = (lower - items->begin()) + 1;
			} else {
				position = lower - items->begin();
			}
			// return from postion to items.size()
			for (size_t i = position; i < items->size(); i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::geOp: {
			// return from lower to items.size()
			unsigned int i =
					(lower == items->end()) ?
							items->size() : (lower - items->begin());
			for (; i < items->size(); i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::likeOp: {
			break;
		}
		}
	}
}

template<class T>
size_t Dictionary<T>::addNewElement(T& value, vector<size_t>& vecValue) {
	if (items->empty()) {
		items->push_back(value);
		vecValue.push_back(0);
		return 0;
	} else {
		// find the lower bound for value in vector
		typename vector<T>::iterator lower;
		lower = std::lower_bound(items->begin(), items->end(), value,
				compFunc<T>);
		//cout << "compFunc(" << value <<", " << *lower <<") is "<< compFunc(value, *lower) <<"\n";
		// value existed
		if (lower != items->end() && equalFunc(value, *lower)) {
			// return the position of lower
			long elementPos = lower - items->begin();
			vecValue.push_back(elementPos);
			return elementPos;
		} else {
			// The position of new element in dictionary
			size_t newElementPos = 0L;
			if (lower == items->end()) {
				// insert to the end of dictionary
				newElementPos = items->size();
				items->push_back(value);
				vecValue.push_back(newElementPos);
			} else {
				newElementPos = lower - items->begin();
				// insert into dictionary
				items->insert(lower, value);
				// update (+1) to all elements in vecValue have value >= newElementPos
				for (int i = 0; i < vecValue.size(); i++) {
					if (vecValue[i] >= newElementPos) {
						++vecValue[i];
					}
				}
				vecValue.push_back(newElementPos);
			}

			// return the position of new element
			return newElementPos;
		}
	}
}

template<class T>
size_t Dictionary<T>::size() {
	return items->size();
}

template<class T>
void Dictionary<T>::print(int row) {
	for (int i = 0; i < items->size() && i < row; i++) {
		cout << "Dictionary[" << i << "] = " << items->at(i) << "\n";
	}
}

template<class T>
Dictionary<T>::~Dictionary() {
	delete items;
}

template class Dictionary<string> ;
template class Dictionary<int> ;

#endif

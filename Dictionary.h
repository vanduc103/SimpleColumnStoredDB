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
#include <map>
#include <algorithm>

using namespace std;

template<class T>
class Dictionary {
private:
	struct classcomp {
	  bool operator() (const T lhs, const T rhs) const
	  {return lhs<rhs;}
	};

	struct invertedIndex {
		string word;
		char position;	 // position on text
		vector<size_t> location; // position on dictionary
		// functions
		bool operator<(const invertedIndex& a) const
		{
			return word < a.word;
		}
		bool operator==(const invertedIndex& a) const
		{
			return word == a.word;
		}
	};

	vector<T>* items;
	std::map<T, size_t, classcomp>* sMap;
	vector<T>* bulkVecValue;
	vector<invertedIndex>* vecIndexLevel0;
public:
	Dictionary() {
		items = new vector<T>();
		sMap = new map<T, size_t, classcomp>();
		bulkVecValue = new vector<T>();
		vecIndexLevel0 = new vector<invertedIndex>();
	}
	virtual ~Dictionary() {
		delete items;
		delete vecIndexLevel0;
		delete sMap;
		delete bulkVecValue;
	}

	T* lookup(size_t index);
	void search(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result);
	size_t addNewElement(T& value, vector<size_t>* vecValue, bool sorted, bool bulkInsert);
	size_t size();
	void print(int row);
	void buildInvertedIndex();

	vector<T>* getBulkVecValue() {
		return bulkVecValue;
	}

	void clearTemp() {
		sMap->clear();
		bulkVecValue->resize(0);
	}

};


#endif /* DICTIONARY_H_ */

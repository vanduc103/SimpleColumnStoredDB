/*
 * Column.h
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */
#include <vector>

#ifndef COLUMN_H_
#define COLUMN_H_

#include <math.h>
#include <iostream>
#include <boost/dynamic_bitset.hpp>
#include "ColumnBase.h"
#include "Dictionary.h"

namespace std {

template<typename T>
class Column : public ColumnBase {
private:
	// value vector for column
	vector<size_t>* vecValue;
	// encoded value vector
	vector<boost::dynamic_bitset<>>* encodedVecValue;
	// dictionary vector for column
	Dictionary<T>* dictionary;
public:
	Column() {
		encodedVecValue = new vector<boost::dynamic_bitset<>>();
		dictionary = new Dictionary<T>();
		vecValue = new vector<size_t>();
	}
	virtual ~Column() {
		delete vecValue;
		delete encodedVecValue;
		delete dictionary;
	}

	vector<size_t>* getVecValue() {
		if (vecValue == NULL) {
			vecValue = new vector<size_t>();
		}
		return vecValue;
	}
	void printVecValue(int row) {
		for (size_t i = 0; i < (*vecValue).size() && i < row; i++) {
			cout << "vecValue[" << i << "] = " << (*vecValue)[i] << "\n";
		}
	}
	void updateEncodedVecValue(vector<size_t>* vecValue, size_t sizeOfDict) {
		size_t numOfBit = (size_t) ceil(log2((double) sizeOfDict));
		for (size_t i = 0; i < vecValue->size(); i++) {
			encodedVecValue->push_back(boost::dynamic_bitset<>(numOfBit, vecValue->at(i)));
		}
		// delete vecValue
		delete vecValue;
	}
	vector<boost::dynamic_bitset<>>* getEncodedVecValue() {
		return encodedVecValue;
	}
	void printEncodedVecValue(int row) {
		for (size_t i = 0; i < (*encodedVecValue).size() && i < row; i++) {
			cout << "encodedVecValue[" << i << "] = " << ((*encodedVecValue)[i]).to_ulong() << "\n";
		}
	}

	Dictionary<T>* getDictionary() {
		if (dictionary == NULL) {
			dictionary = new Dictionary<T>();
		}
		return dictionary;
	}
};

} /* namespace std */

#endif /* COLUMN_H_ */

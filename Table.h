/*
 * Table.h
 *
 *  Created on: Sep 26, 2016
 *      Author: duclv
 */

#ifndef TABLE_H_
#define TABLE_H_

#include <string>
#include <tuple>
#include <vector>
#include <iostream>
#include "Column.h"
#include "ColumnBase.h"

namespace std {

class Table {
private:
	vector<ColumnBase*>* m_columns;
	string name;
public:
	virtual ~Table() {
		//delete &m_columns;
	}

	Table(vector<ColumnBase*>& columns) {
		m_columns = &columns;
	}

	string getName() {
		return name;
	}
	void setName(string tableName) {
		name = tableName;
	}

	ColumnBase* getColumnByName(string colName) {
		//int tupleSize = tuple_size<decltype(m_columns)>::value;;
		for (size_t i = 0; i < m_columns->size(); i++) {
			ColumnBase* column = m_columns->at(i);
			if (column->getName() == colName)
				return column;
		}
		return NULL;
	}
};

} /* namespace std */

#endif /* TABLE_H_ */

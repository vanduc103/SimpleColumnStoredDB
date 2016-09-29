/*
 * ColumnBase.h
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */

#ifndef COLUMNBASE_H_
#define COLUMNBASE_H_

#include <string>

namespace std {

class ColumnBase {
public:
	enum COLUMN_TYPE {intType, charType, varcharType};
	enum OP_TYPE {equalOp, neOp, ltOp, leOp, gtOp, geOp, likeOp};
private:
	string name;
	COLUMN_TYPE type;
	int size;
public:
	ColumnBase();
	virtual ~ColumnBase();

	string getName();
	void setName(string nameValue);

	COLUMN_TYPE getType();
	void setType(COLUMN_TYPE typeValue);

	int getSize();
	void setSize(int sizeValue);
};

} /* namespace std */

#endif /* COLUMNBASE_H_ */

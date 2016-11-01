//============================================================================
// Name        : App.cpp
// Author      : Le Van Duc
// Version     :
// Copyright   : 
// Description : Main program to run functions of simple column-stored DB
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include "Dictionary.h"
#include "Column.h"
#include "ColumnBase.h"
#include "Table.h"
#include "SQLParser.h"
#include "Util.h"

using namespace std;

// print operator type string value
std::ostream& operator<<(std::ostream& out, const ColumnBase::OP_TYPE value){
    const char* s = 0;
#define PROCESS_VAL(p, str) case(p): s = str; break;
    switch(value){
        PROCESS_VAL(ColumnBase::OP_TYPE::equalOp, "=");
        PROCESS_VAL(ColumnBase::OP_TYPE::neOp, "<>");
        PROCESS_VAL(ColumnBase::OP_TYPE::gtOp, ">");
        PROCESS_VAL(ColumnBase::OP_TYPE::geOp, ">=");
        PROCESS_VAL(ColumnBase::OP_TYPE::ltOp, "<");
        PROCESS_VAL(ColumnBase::OP_TYPE::leOp, "<=");
        PROCESS_VAL(ColumnBase::OP_TYPE::containOp, "CONTAIN");
    }
#undef PROCESS_VAL

    return out << s;
}

// print column type
std::ostream& operator<<(std::ostream& out, const ColumnBase::COLUMN_TYPE value){
    const char* s = 0;
#define PROCESS_VAL(p, str) case(p): s = str; break;
    switch(value){
        PROCESS_VAL(ColumnBase::intType, "INTEGER");
        PROCESS_VAL(ColumnBase::charType, "TEXT");
        PROCESS_VAL(ColumnBase::varcharType, "TEXT");
    }
#undef PROCESS_VAL
    return out << s;
}

int main(void) {
	puts("***** Simple Column-Store Database start ******");

	string createQuery = "create table orders (o_orderkey integer, o_orderstatus text, o_totalprice integer, o_comment text)";
	//cout << "Enter create table statement: ";
	//getline(cin, createQuery);
	hsql::SQLParserResult* pCreateQuery = hsql::SQLParser::parseSQLString(createQuery);
	string tableName;
	vector<string> cColumnName;
	vector<ColumnBase::COLUMN_TYPE> cColumnType;
	if (pCreateQuery->isValid) {
		hsql::SQLStatement* stmt = pCreateQuery->getStatement(0);
		if (stmt->type() == hsql::StatementType::kStmtCreate) {
			hsql::CreateStatement* createStmt = (hsql::CreateStatement*) stmt;
			tableName = createStmt->tableName;
			vector<hsql::ColumnDefinition*>* cols = createStmt->columns;
			for (hsql::ColumnDefinition* colDef : *cols) {
				cColumnName.push_back(colDef->name);
				switch (colDef->type) {
					case hsql::ColumnDefinition::INT:
					case hsql::ColumnDefinition::DOUBLE:
						cColumnType.push_back(ColumnBase::intType);
						break;
					case hsql::ColumnDefinition::TEXT:
						cColumnType.push_back(ColumnBase::charType);
						break;
				}
			}
		}
	}
	else {
		cout << "Create table statement is Invalid !" << endl;
		return -1;
	}
	cout << "tablename: " << tableName << endl;
	// init table
	vector<ColumnBase*> columns;
	Table* table = new Table(columns);
	table->setName(tableName);
	// init column
	for (size_t i = 0; i < cColumnName.size(); i++) {
		string name = cColumnName[i];
		ColumnBase::COLUMN_TYPE type = cColumnType[i];
		//cout << "colname: " << name << endl;
		//cout << "col type: " << type << endl;
		// create new column
		ColumnBase* colBase = new ColumnBase();
		if (type == ColumnBase::intType) {
			Column<int>* col = new Column<int>();
			colBase = col;
		}
		else {
			Column<string>* col = new Column<string>();
			colBase = col;
		}
		colBase->setName(name);
		colBase->setType(type);
		if (name == "o_orderkey") colBase->setPrimaryKey(true);
		if (name == "o_comment") colBase->setCreateInvertedIndex(true);
		columns.push_back(colBase);
	}

	// calculate time execution
	clock_t begin_time = clock();

	// read file
	cout << "Enter file path for orders table: ";
	string filePath;
	getline(cin, filePath);
	ifstream infile(filePath);
	if (!infile) {
		cout << "Cannot open file path: " << filePath << endl;
		return -1;
	}
	// process file
	size_t row = 0;
	string line;
	string delim = ",";
	while(getline(infile, line)) {
		size_t last = 0; size_t next = 0;
		char idx = 0;
		vector<string> items;	// split line into items
		string lastItem;
		string token;
		// read each splited token and add to items
		while ((next = line.find(delim, last)) != string::npos) {
			token = line.substr(last, next - last);
			last = next + delim.length();
			if (idx >= table->numOfColumns() - 1)
				lastItem += token + delim;
			else
				items.push_back(token);
			++idx;
		}
		// get the last token and add to last item
		token = line.substr(last);
		lastItem += token;
		items.push_back(lastItem);

		// process input data based on column type
		for (int i = 0; i < table->numOfColumns(); i++) {
			string item = items[i];
			ColumnBase* colBase = columns[i];
			if (colBase->getType() == ColumnBase::intType) {
				int intValue = stoi(item);
				bool sorted = true;
				// update dictionary
				Column<int>* col = (Column<int>*) colBase;
				col->updateDictionary(intValue, sorted);
			}
			else {
				// char or varchar type
				boost::replace_all(item, "\"", "");
				bool sorted = false;
				// update dictionary
				Column<string>* col = (Column<string>*) colBase;
				col->updateDictionary(item, sorted);
			}
		}
		++row;
		Util::printLoading(row);
	}
	infile.close();

	// print distinct numbers
	cout << endl;
	for (ColumnBase* colBase : columns) {
		if (colBase->getType() == ColumnBase::intType) {
			Column<int>* col = (Column<int>*) colBase;
			cout << col->getName() << " #distinct values = " << col->getDictionary()->size()<<"/"<<row << endl;
		}
		else {
			Column<string>* col = (Column<string>*) colBase;
			cout << col->getName() << " #distinct values = " << col->getDictionary()->size()<<"/"<<row << endl;
		}
	}

	// process columns of table
	table->processColumn();

	// loaded time
	std::cout << "Table Load time: " << float( clock () - begin_time ) /  CLOCKS_PER_SEC << " seconds " << endl;

	/*
	 * Load table 2: lineitem
	 */
	begin_time = clock();
	// init table 2
	vector<ColumnBase*> columns2;
	Table* table2 = new Table(columns2);
	table2->setName("lineitem");
	cout << "table name: " << table2->getName() << endl;

	// Column 0
	Column<int>* col0 = new Column<int>();
	col0->setName("l_orderkey");
	col0->setType(ColumnBase::intType);
	col0->setSize(4);

	// Column 1
	Column<int>* col1 = new Column<int>();
	col1->setName("l_quantity");
	col1->setType(ColumnBase::intType);
	col1->setSize(4);

	// Column 2
	Column<string>* col2 = new Column<string>();
	col2->setName("l_returnflag");
	col2->setType(ColumnBase::charType);
	col2->setSize(1);

	// read data into memory
	/*filePath = "/home/duclv/homework/lineitem1M.tbl";
	ifstream infile2(filePath);
	if (!infile2) {
		cout << "Cannot open file path: " << filePath << endl;
		return -1;
	}*/
	cout << "Enter file path for lineitem table: ";
	getline(cin, filePath);
	ifstream infile2(filePath);
	if (!infile2) {
		cout << "Cannot open file path: " << filePath << endl;
		return -1;
	}
	// process file
	row = 0;
	delim = "|";
	while(getline(infile2, line)) {
		size_t last = 0; size_t next = 0;
		char i = 0;
		string token;
		// read each token and add into column store
		while ((next = line.find(delim, last)) != string::npos) {
			token = line.substr(last, next - last);
			last = next + delim.length();
			i++;
			// orderkey is 1st column
			if (i == 1) {
				int orderkey = stoi(token);
				col0->updateDictionary(orderkey, true);
			}
			// quantity is 2nd column
			else if (i == 2) {
				int quantity = stoi(token);
				col1->updateDictionary(quantity, true);
			}
		}
		// get the last token
		token = line.substr(last);
		boost::replace_all(token, "\"", "");
		col2->updateDictionary(token, false);
		++row;
		Util::printLoading(row);
	}
	infile.close();

	// print distinct values
	cout << endl;
	cout << col0->getName() << " #distinct values = " << col0->getDictionary()->size()<<"/"<<row << endl;
	cout << col1->getName() << " #distinct values = " << col1->getDictionary()->size()<<"/"<<row << endl;
	cout << col2->getName() << " #distinct values = " << col2->getDictionary()->size()<<"/"<<row << endl;

	// add columns to table
	columns2.push_back(col0);
	columns2.push_back(col1);
	columns2.push_back(col2);
	table2->processColumn();

	cout << "Table 2 Load time: " << float( clock () - begin_time ) /  CLOCKS_PER_SEC << " seconds " << endl;

	// query
	while (true) {
		string query = "";
		cout << "Enter a query (enter 'quit' to quit) or select join query example (join1/join2/join3): ";
		getline(cin, query);
		if ("quit" == query)
			break;

		// Join example
		if (query.find("join") != string::npos) {
			begin_time = clock();
			// join l_orderkey with o_orderkey
			Column<int>* l_orderkey = (Column<int>*) table2->getColumnByName("l_orderkey");
			Column<int>* o_orderkey = (Column<int>*) table->getColumnByName("o_orderkey");

			// initialize matching row ids
			vector<bool>* l_rowIds = new vector<bool>();
			for (size_t i = 0; i < l_orderkey->vecValueSize(); i++) {
				l_rowIds->push_back(false);
			}
			vector<bool>* o_rowIds = new vector<bool>();
			for (size_t i = 0;i < o_orderkey->vecValueSize(); i++) {
				o_rowIds->push_back(false);
			}

			// join example 2: orders.o_totalprice < 56789 AND l_quantity > 40
			if (query.find("2") != string::npos) {
				Column<int>* o_totalprice = (Column<int>*) table->getColumnByName("o_totalprice");
				Column<int>* l_quantity = (Column<int>*) table2->getColumnByName("l_quantity");
				// execute where query
				int value = 56789;
				o_totalprice->selection(value, ColumnBase::ltOp, o_rowIds);
				cout << "Orders rowIds count = " << Util::rowSelectedSize(o_rowIds) << endl;
				value = 40;
				l_quantity->selection(value, ColumnBase::gtOp, l_rowIds);
				cout << "Lineitem rowIds count = " << Util::rowSelectedSize(l_rowIds) << endl;
			}
			// join example 3: orders.o_comment contains ‘gift’
			else if (query.find("3") != string::npos) {
				Column<string>* o_comment = (Column<string>*) table->getColumnByName("o_comment");
				// execute where query
				string value = "gift";
				o_comment->selection(value, ColumnBase::containOp, o_rowIds);
				cout << "Orders rowIds count = " << Util::rowSelectedSize(o_rowIds) << endl;
				// all rows of lineitem is selected
				for (size_t i = 0; i < l_rowIds->size(); i++) {
					l_rowIds->at(i) = true;
				}
			}
			// join example 1: no where selection
			else {
				// all rows of 2 joining tables are selected
				for (size_t i = 0; i < l_rowIds->size(); i++) {
					l_rowIds->at(i) = true;
				}
				for (size_t i = 0;i < o_rowIds->size(); i++) {
					o_rowIds->at(i) = true;
				}
			}
			// initialize join result pairs of row ids
			vector<tuple<int, int>>* join_result_pairs = new vector<tuple<int, int>>();

			// process hash and probe
			if (Util::rowSelectedSize(l_rowIds) >= Util::rowSelectedSize(o_rowIds)) {
				// create mapping between vecValue of 2 join columns
				map<size_t, size_t> mappingValueId;
				for (size_t i = 0; i < l_orderkey->getDictionary()->size(); i++) {
					size_t valueId1 = i;
					int* dictValue1 = l_orderkey->getDictionary()->lookup(valueId1);
					if (dictValue1 != NULL) {
						// search dictionary value on 2nd column
						vector<size_t> result;
						o_orderkey->getDictionary()->search(*dictValue1, ColumnBase::equalOp, result);
						// if not existed valueId2 then return -1
						size_t valueId2 = result[0];
						mappingValueId[valueId1] = valueId2;
					}
				}

				// build hashmap for smaller column
				map<size_t, vector<size_t>> hashmap;
				o_orderkey->buildHashmap(hashmap, o_rowIds);

				// probe (join) to find matching row ids
				for (size_t l_rowId = 0; l_rowId < l_orderkey->vecValueSize(); l_rowId++) {
					// by pass if row id not in previous selection result
					if (!l_rowIds->at(l_rowId)) continue;
					size_t valueId1 = l_orderkey->vecValueAt(l_rowId);
					// get valueId2 from mapping
					size_t valueId2 = mappingValueId[valueId1];
					// found on hashmap
					vector<size_t> rowIds = hashmap[valueId2];
					if (rowIds.size() > 0) {
						// make join result pairs
						for (size_t o_rowId : rowIds) {
							join_result_pairs->push_back(make_tuple(l_rowId, o_rowId));
						}
					}
				}
			}
			else {
				// create mapping between vecValue of 2 join columns
				map<size_t, size_t> mappingValueId;
				for (size_t i = 0; i < o_orderkey->getDictionary()->size(); i++) {
					size_t valueId1 = i;
					int* dictValue1 = o_orderkey->getDictionary()->lookup(valueId1);
					if (dictValue1 != NULL) {
						// search dictionary value on 2nd column
						vector<size_t> result;
						l_orderkey->getDictionary()->search(*dictValue1, ColumnBase::equalOp, result);
						// if not existed valueId2 then return -1
						size_t valueId2 = result[0];
						mappingValueId[valueId1] = valueId2;
					}
				}

				// build hashmap for smaller column
				map<size_t, vector<size_t>> hashmap;
				l_orderkey->buildHashmap(hashmap, l_rowIds);

				// probe (join) to find matching row ids
				for (size_t o_rowId = 0; o_rowId < o_orderkey->vecValueSize(); o_rowId++) {
					// by pass if row id not in previous selection result
					if (!o_rowIds->at(o_rowId)) continue;
					size_t valueId1 = o_orderkey->vecValueAt(o_rowId);
					// get valueId2 from mapping
					size_t valueId2 = mappingValueId[valueId1];
					// found on hashmap
					vector<size_t> rowIds = hashmap[valueId2];
					if (rowIds.size() > 0) {
						// make join pair
						for (size_t l_rowId : rowIds)
							join_result_pairs->push_back(make_tuple(l_rowId, o_rowId));
					}
				}
			}

			// print the result based on matching row ids
			cout << "********* Print join result ************" << endl;
			if (query == "join1") {
				cout << "   SELECT * from orders JOIN lineitem ON orders.o_orderkey = lineitem.l_orderkey   " << endl;
			}
			else if (query == "join2") {
				cout << "   SELECT * from orders JOIN lineitem ON orders.o_orderkey = lineitem.l_orderkey WHERE " << endl
					 << "   orders.o_totalprice < 56789 AND l_quantity > 40" << endl;
			}
			else if (query == "join3") {
				cout << "   SELECT * from orders JOIN lineitem ON orders.o_orderkey = lineitem.l_orderkey WHERE " << endl
					 << "   orders.o_comment contains ‘gift’" << endl;
			}
			size_t join_totalresult = join_result_pairs->size();
			size_t limit = 10;
			size_t limitCount = 0;
			// get orders table's row ids
			vector<int>* joinResult_o_rowIds = new vector<int>();
			for (size_t i = 0; i < join_result_pairs->size(); i++) {
				// get from tuple
				joinResult_o_rowIds->push_back(std::get<1>(join_result_pairs->at(i)));
			}
			// get lineitem table's row ids
			vector<int>* joinResult_l_rowIds = new vector<int>();
			for (size_t i = 0; i < join_result_pairs->size(); i++) {
				// get from tuple
				joinResult_l_rowIds->push_back(std::get<0>(join_result_pairs->at(i)));
			}

			vector<string> outputs (limit + 1);
			vector<string> q_select_fields;
			// lineitem's field name
			q_select_fields.resize(0);
			for (ColumnBase* colBase : (*table2->columns())) {
				q_select_fields.push_back(colBase->getName());
			}
			for (size_t idx = 0; idx < q_select_fields.size(); idx++) {
				string select_field_name = q_select_fields[idx];
				outputs[0] += select_field_name + ", ";
				ColumnBase* colBase = (ColumnBase*) table2->getColumnByName(select_field_name);
				if (colBase == NULL) continue;
				if (colBase->getType() == ColumnBase::intType) {
					Column<int>* t = (Column<int>*) colBase;
					vector<int> tmpOut = t->projection(joinResult_l_rowIds, limit, limitCount);
					for (size_t i = 0; i < tmpOut.size(); i++) {
						outputs[i+1] += to_string(tmpOut[i]) + ",   ";
						// padding whitespace
						for (int j = 11 - (outputs[i+1].length()); j > 0; j--) {
							outputs[i+1] += " ";
						}
					}
				}
				else {
					Column<string>* t = (Column<string>*) colBase;
					vector<string> tmpOut = t->projection(joinResult_l_rowIds, limit, limitCount);
					for (size_t i = 0; i < tmpOut.size(); i++) {
						outputs[i+1] += "\"" + tmpOut[i] + "\"" + ",   ";
					}
				}
			}
			for (int i = 1; i < outputs.size(); i++) {
				outputs[i] += "                 ";
			}
			// orders's field name
			q_select_fields.resize(0);
			for (ColumnBase* colBase : (*table->columns())) {
				q_select_fields.push_back(colBase->getName());
			}
			for (size_t idx = 0; idx < q_select_fields.size(); idx++) {
				string select_field_name = q_select_fields[idx];
				outputs[0] += select_field_name + ", ";
				ColumnBase* colBase = (ColumnBase*) table->getColumnByName(select_field_name);
				if (colBase == NULL) continue;
				if (colBase->getType() == ColumnBase::intType) {
					Column<int>* t = (Column<int>*) colBase;
					vector<int> tmpOut = t->projection(joinResult_o_rowIds, limit, limitCount);
					for (size_t i = 0; i < tmpOut.size(); i++) {
						outputs[i+1] += to_string(tmpOut[i]) + ",   ";
						// padding whitespace
						for (int j = 11 - (outputs[i+1].length()); j > 0; j--) {
							outputs[i+1] += " ";
						}
					}
				}
				else {
					Column<string>* t = (Column<string>*) colBase;
					vector<string> tmpOut = t->projection(joinResult_o_rowIds, limit, limitCount);
					for (size_t i = 0; i < tmpOut.size(); i++) {
						outputs[i+1] += "\"" + tmpOut[i] + "\"" + ",   ";
					}
				}
			}
			// print output
			for (string output : outputs) {
				if (!output.empty())
					cout << output << endl;
			}
			if (limitCount >= limit)
				cout << "Showing "<<limit<<"/"<<join_totalresult<<" results !" << endl;
			else if (limitCount == 0)
				cout << "No result found !" << endl;
			else
				cout << "Showing "<<limitCount<<"/"<<join_totalresult<<" results !" << endl;

			// delete temporary variables
			delete o_rowIds;
			delete l_rowIds;
			delete joinResult_o_rowIds;
			delete join_result_pairs;

			std::cout << "Query time: " << float(clock() - begin_time)/CLOCKS_PER_SEC << " seconds " << endl;
			continue;
		}

		// other queries
		begin_time = clock();
		// parse a given query
		hsql::SQLParserResult* pResult = hsql::SQLParser::parseSQLString(query);
		// check whether the parsing was successfull
		bool queryValid = pResult->isValid;
		while (queryValid) {
			printf("Parsed successfully!\n");
			// process the statements...
			string q_table;
			vector<string> q_select_fields;
			vector<string> q_where_fields;
			vector<ColumnBase::OP_TYPE> q_where_ops;
			vector<string> q_where_values;

			hsql::SQLStatement* stmt = pResult->getStatement(0);
			if (stmt->type() == hsql::StatementType::kStmtSelect) {
				hsql::SelectStatement* select = (hsql::SelectStatement*) stmt;
				q_table = select->fromTable->getName();
				cout << "Table name: " << q_table << endl;
				if (q_table != table->getName()) {queryValid = false; break;}

				for (hsql::Expr* expr : *select->selectList) {
					if (expr->type == hsql::ExprType::kExprStar) {
						for (ColumnBase* colBase : (*table->columns())) {
							q_select_fields.push_back(colBase->getName());
						}
					}
					else if (expr->type == hsql::ExprType::kExprColumnRef)
						q_select_fields.push_back(expr->name);
					else if (expr->type == hsql::ExprType::kExprFunctionRef)
						q_select_fields.push_back(string(expr->name) + string("#") + string(expr->expr->name));
				}
				for (size_t i = 0; i < q_select_fields.size(); i++) {
					cout << "select fields[" << i << "] = " << q_select_fields[i] << endl;
				}

				if (select->whereClause != NULL) {
					hsql::Expr* expr = select->whereClause;
					if (expr->type == hsql::ExprType::kExprOperator) {
						if (expr->op_type == hsql::Expr::OperatorType::SIMPLE_OP) {
							q_where_fields.push_back(expr->expr->name);

							if (expr->op_char == '>')
								q_where_ops.push_back(ColumnBase::OP_TYPE::gtOp);
							else if (expr->op_char == '<')
								q_where_ops.push_back(ColumnBase::OP_TYPE::ltOp);
							else if (expr->op_char == '=')
								q_where_ops.push_back(ColumnBase::OP_TYPE::equalOp);

							hsql::ExprType literalType = expr->expr2->type;
							if (literalType == hsql::ExprType::kExprLiteralInt)
								q_where_values.push_back(to_string(expr->expr2->ival));
							else if (literalType == hsql::ExprType::kExprColumnRef)
								q_where_values.push_back(expr->expr2->name);
						}
						else if (expr->op_type == hsql::Expr::OperatorType::LIKE) {
							q_where_fields.push_back(expr->expr->name);

							q_where_ops.push_back(ColumnBase::OP_TYPE::containOp);

							hsql::ExprType literalType = expr->expr2->type;
							if (literalType == hsql::ExprType::kExprLiteralInt)
								q_where_values.push_back(to_string(expr->expr2->ival));
							else if (literalType == hsql::ExprType::kExprColumnRef)
								q_where_values.push_back(expr->expr2->name);
						}
						else if (expr->op_type == hsql::Expr::OperatorType::AND) {
							hsql::Expr* expr1 = expr->expr;
							hsql::Expr* expr2 = expr->expr2;
							if (expr1->op_type == hsql::Expr::OperatorType::SIMPLE_OP) {
								q_where_fields.push_back(expr1->expr->name);

								if (expr1->op_char == '>')
									q_where_ops.push_back(ColumnBase::OP_TYPE::gtOp);
								else if (expr1->op_char == '<')
									q_where_ops.push_back(ColumnBase::OP_TYPE::ltOp);
								else if (expr1->op_char == '=')
									q_where_ops.push_back(ColumnBase::OP_TYPE::equalOp);

								if (expr1->expr2->type == hsql::ExprType::kExprLiteralInt)
									q_where_values.push_back(to_string(expr1->expr2->ival));
								else if (expr1->expr2->type == hsql::ExprType::kExprColumnRef)
									q_where_values.push_back(expr1->expr2->name);
							}
							if (expr2->op_type == hsql::Expr::OperatorType::SIMPLE_OP) {
								q_where_fields.push_back(expr2->expr->name);

								if (expr2->op_char == '>')
									q_where_ops.push_back(ColumnBase::OP_TYPE::gtOp);
								else if (expr2->op_char == '<')
									q_where_ops.push_back(ColumnBase::OP_TYPE::ltOp);
								else if (expr2->op_char == '=')
									q_where_ops.push_back(ColumnBase::OP_TYPE::equalOp);

								if (expr2->expr2->type == hsql::ExprType::kExprLiteralInt)
									q_where_values.push_back(to_string(expr2->expr2->ival));
								else if (expr2->expr2->type == hsql::ExprType::kExprColumnRef)
									q_where_values.push_back(expr2->expr2->name);
							}
						}
						else {
							// not support
							queryValid = false;
							break;
						}
					}
					for (size_t i = 0; i < q_where_fields.size(); i++) {
						cout << "where fields[" << i << "] = " << q_where_fields[i] << endl;
					}
					for (size_t i = 0; i < q_where_ops.size(); i++) {
						cout << "where operators[" << i << "] = " << q_where_ops[i] << endl;
					}
					for (size_t i = 0; i < q_where_values.size(); i++) {
						cout << "value values[" << i << "] = " << q_where_values[i] << endl;
					}
				}
				/*
				 * execute query
				 */
				// this vector contain query result: row id = true, otherwise row id = false
				vector<bool>* q_resultRid = new vector<bool>();
				vector<vector<size_t>> q_results;
				for (size_t fidx = 0; fidx < q_where_fields.size(); fidx++) {
					string q_where_field = q_where_fields[fidx];
					ColumnBase::OP_TYPE q_where_op = q_where_ops[fidx];
					string q_where_value = q_where_values[fidx];

					// get column by name then cast to appropriate column based on column type
					ColumnBase* colBase = table->getColumnByName(q_where_field);
					if (colBase == NULL) continue;
					switch (colBase->getType()) {
						case ColumnBase::COLUMN_TYPE::intType:
						{
							Column<int>* t = (Column<int>*) colBase;
							int searchValue = 0;
							try {
								searchValue = stoi(q_where_value);
							} catch (exception& e) {
								cerr << "Exception: " << e.what() << endl;
								break;
							}
							// execute query
							bool initQueryResult = (fidx == 0);
							t->selection(searchValue, q_where_op, q_resultRid, initQueryResult);
							break;
						}
						case ColumnBase::charType:
						case ColumnBase::varcharType:
						{
							Column<string>* t = (Column<string>*) colBase;
							string searchValue = q_where_value;
							// execute query
							bool initQueryResult = (fidx == 0);
							t->selection(searchValue, q_where_op, q_resultRid, initQueryResult);
							break;
						}
					}
				}
				size_t totalResult = 0;
				for (size_t rid = 0; rid < q_resultRid->size(); rid++) {
					if (q_resultRid->at(rid))
						++totalResult;
				}
				// print result
				cout << "*** Query result ***" << endl;
				size_t limit = 10; // the limit result to print
				size_t limitCount = 0;
				vector<string> outputs (limit + 1);
				for (size_t idx = 0; idx < q_select_fields.size(); idx++) {
					string select_field_name = q_select_fields[idx];
					outputs[0] += select_field_name + ", ";
					ColumnBase* colBase = (ColumnBase*) table->getColumnByName(select_field_name);
					if (colBase == NULL)
						continue;
					limitCount = 0;
					if (colBase->getType() == ColumnBase::intType) {
						Column<int>* t = (Column<int>*) colBase;
						// get result
						vector<int> tmpOut = t->projection(q_resultRid, limit, limitCount);
						for (size_t i = 0; i < tmpOut.size(); i++) {
							outputs[i+1] += to_string(tmpOut[i]) + ",   ";
							// padding whitespace
							for (int j = 11 - (outputs[i+1].length()); j > 0; j--) {
								outputs[i+1] += " ";
							}
						}
					}
					else {
						Column<string>* t = (Column<string>*) colBase;
						// get result
						vector<string> tmpOut = t->projection(q_resultRid, limit, limitCount);
						for (size_t i = 0; i < tmpOut.size(); i++) {
							outputs[i+1] += "\"" + tmpOut[i] + "\"" + ",   ";
						}
					}
				}
				delete q_resultRid;
				// print result
				for (string output : outputs) {
					if (!output.empty())
						cout << output << endl;
				}
				if (limitCount >= limit)
					cout << "Showing "<<limit<<"/"<<totalResult<<" results !" << endl;
				else if (limitCount == 0)
					cout << "No result found !" << endl;
				else
					cout << "Showing "<<limitCount<<"/"<<totalResult<<" results !" << endl;
				// query time
				std::cout << "Table Selection time: " << float(clock() - begin_time)/CLOCKS_PER_SEC << " seconds " << endl;
				// Processe done !
				break;
			}
			else
				cout << "Please enter a SELECT query !!!" << endl;
		}
		if (!queryValid) {
			printf("The SQL query is invalid or not supported !!!\n");
		}
	}

	return EXIT_SUCCESS;
}


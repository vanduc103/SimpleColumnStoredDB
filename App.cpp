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
#include <boost/dynamic_bitset.hpp>

#include "Dictionary.h"
#include "Column.h"
#include "ColumnBase.h"
#include "Table.h"
#include "SQLParser.h"

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
        PROCESS_VAL(ColumnBase::OP_TYPE::likeOp, "LIKE");
    }
#undef PROCESS_VAL

    return out << s;
}

int main(void) {
	puts("***** Simple Column-Store Database start ******");

	// Column 0
	Column<int>* col0 = new Column<int>();
	col0->setName("o_orderkey");
	col0->setType(ColumnBase::intType);
	col0->setSize(4);

	// Column 1
	Column<string>* col1 = new Column<string>();
	col1->setName("o_orderstatus");
	col1->setType(ColumnBase::charType);
	col1->setSize(1);

	// Column 2
	Column<int>* col2 = new Column<int>();
	col2->setName("o_totalprice");
	col2->setType(ColumnBase::intType);
	col2->setSize(4);

	// Column 3
	Column<string>* col3 = new Column<string>();
	col3->setName("o_comment");
	col3->setType(ColumnBase::charType);
	col3->setSize(30);

	// calculate time execution
	clock_t begin_time = clock();

	// read file
	cout << "Enter file path: ";
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
		char i = 0;
		string comment;
		string token;
		// read each token and add into column store
		while ((next = line.find(delim, last)) != string::npos) {
			token = line.substr(last, next - last);
			last = next + delim.length();
		    i++;
		    // key is 1st column
		    if (i == 1) {
		    	int key = stoi(token);
				col0->updateDictionary(key, false);
		    }
		    // status is 2nd column
		    else if (i == 2) {
				boost::replace_all(token, "\"", "");
		    	col1->updateDictionary(token, true);
		    }
		    // totalprice is 3rd column
		    else if (i == 3) {
		    	int totalprice = stoi(token);
		    	col2->updateDictionary(totalprice, true);
		    }
		    // comment is from 4th column
			if (i >= 4) comment += token + delim;
		}
		// get the last token and add to comment
		token = line.substr(last);
		comment += token;
		boost::replace_all(comment, "\"", "");
		boost::trim(comment);
		col3->updateDictionary(comment, false);
		++row;
	}
	infile.close();

	// print distinct values
	cout << col1->getName() << " #distinct values = " << col1->getDictionary()->size()<<"/"<<row << endl;
	cout << col2->getName() << " #distinct values = " << col2->getDictionary()->size()<<"/"<<row << endl;
	cout << col3->getName() << " #distinct values = " << col3->getDictionary()->size()<<"/"<<row << endl;
	// bit packing
	col0->bitPackingVecValue();
	col1->bitPackingVecValue();
	col2->bitPackingVecValue();
	col3->bitPackingVecValue();

	// init Table
	vector<ColumnBase*> columns;
	columns.push_back(col0);
	columns.push_back(col1);
	columns.push_back(col2);
	columns.push_back(col3);
	Table* table = new Table(columns);
	table->setName("orders");

	// loaded time
	std::cout << "Table Load time: " << float( clock () - begin_time ) /  CLOCKS_PER_SEC << " seconds " << endl;

	// print result for testing
	//col3->getDictionary()->print(10);
	//col3->printVecValue(10);

	// query
	while (true) {
		string query = "";
		cout << "Enter a query (enter 'quit' to quit): ";
		getline(cin, query);
		if ("quit" == query)
			break;

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
						q_select_fields.push_back("o_orderkey");
						q_select_fields.push_back("o_orderstatus");
						q_select_fields.push_back("o_totalprice");
						q_select_fields.push_back("o_comment");
					}
					else if (expr->type == hsql::ExprType::kExprColumnRef)
						q_select_fields.push_back(expr->name);
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
							vector<size_t> result;
							int searchValue = 0;
							try {
								searchValue = stoi(q_where_value);
							} catch (exception& e) {
								cerr << "Exception: " << e.what() << endl;
								break;
							}
							t->getDictionary()->search(searchValue, q_where_op, result);

							// find rowId with appropriate encodeValue
							for (size_t rowId = 0; !result.empty() && rowId < t->vecValueSize(); rowId++) {
								size_t encodeValue = t->vecValueAt(rowId);
								if (encodeValue >= result.front() && encodeValue <= result.back()) {
									// first where expr
									if (fidx == 0)
										q_resultRid->push_back(1); //rowId is in query result. set to true
									else {
										// only keep rid that existed on previous rid vector
										if (q_resultRid->at(rowId)) {
											// keep this row id in result - do nothing
										}
									}
								}
								else {
									// rowId is not in query result, set to false
									if(fidx == 0)
										q_resultRid->push_back(0);
									else
										q_resultRid->at(rowId) = 0;
								}
							}
							break;
						}
						case ColumnBase::charType:
						case ColumnBase::varcharType:
						{
							Column<string>* t = (Column<string>*) colBase;
							vector<size_t> result;
							string searchValue = q_where_value;
							t->getDictionary()->search(searchValue, q_where_op, result);

							// find rowId with appropriate encodeValue
							for (size_t rowId = 0; !result.empty() && rowId < t->vecValueSize(); rowId++) {
								size_t encodeValue = t->vecValueAt(rowId);
								if (encodeValue >= result.front() && encodeValue <= result.back()) {
									// first where expr
									if (fidx == 0)
										q_resultRid->push_back(1); //rowId is in query result
									else {
										// only keep rid that existed on previous rid vector
										if (q_resultRid->at(rowId)) {
											// keep this row id in result - do nothing
										}
									}
								}
								else {
									// rowId is not in query result
									if(fidx == 0)
										q_resultRid->push_back(0);
									else
										q_resultRid->at(rowId) = 0;
								}
							}
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
						for (size_t rid = 0; rid < q_resultRid->size(); rid++) {
							if (q_resultRid->at(rid)) {
								size_t encodeValue = t->vecValueAt(rid);
								int* a = t->getDictionary()->lookup(encodeValue);
								outputs[limitCount+1] += to_string(*a) + ", ";
								if (++limitCount >= limit) break;
							}
						}
					}
					else {
						Column<string>* t = (Column<string>*) colBase;
						for (size_t rid = 0; rid < q_resultRid->size(); rid++) {
							if (q_resultRid->at(rid)) {
								size_t encodeValue = t->vecValueAt(rid);
								string* a = t->getDictionary()->lookup(encodeValue);
								outputs[limitCount+1] += "\"" + *a + "\", ";
								if (++limitCount >= limit) break;
							}
						}
					}
				}
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


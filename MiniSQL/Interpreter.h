#pragma once
#ifndef _INTERPRET_H_
#define _INTERPRET_H_

#include <string>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include "CatalogManager.h"
#include "RecordManager.h"
#include "IndexManager.h"
#include "Macro.h"
#include "SQL.h"
using namespace std;

bool isChar(char c);
bool isValid(char c);
bool getWord(string & src, string & des);
bool getStr(string & src, string & des);
bool same(string src, string des);
bool isInt(const char *input);
bool isFloat(const char *input);

class Interpret {
public:
							//解析树,所有的命令均可以反映到这样一棵树上
	int m_operation;		//要执行的操作或错误代码,用宏表示
	string m_tabname;		//要操作的表格名
	string m_indname;		//要操作的索引名
	string m_colname;
	string m_filename;		//要操作的文件名
	vector<Attribute> column;
	vector<Condition> condition;			//要比较的where字句的链表
	Row row;		//要插入的值链表
	Table getTableInfo;
	Index getIndexInfo;
	int PrimaryKeyPosition;
	int UniquePostion;
	vector<Condition> UniqueCondition;
	//static CCatalogManager Catalog;

	Interpret() {
		m_operation = UNKNOW;
		m_tabname = "";
		m_indname = "";
		m_colname = "";
		m_filename = "";
		PrimaryKeyPosition = -1;
		UniquePostion = -1;
	}

	~Interpret(){}
	
	Condition getCon(string &cmd);
	Attribute getCol(string &cmd);
	string getRow(string &cmd);
	void Parse(char* command);

	//This function is used to initiate the 'column' structure
	void initCol()
	{
		if(column.size()>0){
			column.clear();
		}
	}
	
	//This function is used to initiate the 'condition' structure
	void initCond()
	{
		if(condition.size()>0){
			condition.clear();
		}
	}
	
	//This function is used to initiate the 'insertvalue' structure
	void initValue()
	{/*
		if(row.columns.size()>0){
			row.columns.clear();
		}*/
	}
	
	void initTable(){
		getTableInfo.tableName = "";
		getTableInfo.attriNum = getTableInfo.blockNum = getTableInfo.totalLength = 0;
		getTableInfo.attribute.clear();
	}
	
	void initIndex(){
		getIndexInfo.blockNum = getIndexInfo.column = -1;
		getIndexInfo.indexName = "";
		getIndexInfo.tableName = "";
	}
	
	void makeInitilate(){
		m_operation =UNKNOW;
		m_tabname = "";
		m_indname = "";
		initCol();
		initCond();
		initValue();
		initTable();
		initIndex();
		row.columns.clear();
	}
};

#endif

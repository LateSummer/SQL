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
							//������,���е���������Է�ӳ������һ������
	int m_operation;		//Ҫִ�еĲ�����������,�ú��ʾ
	string m_tabname;		//Ҫ�����ı����
	string m_indname;		//Ҫ������������
	string m_colname;
	vector<Attribute> column;
	vector<Condition> condition;			//Ҫ�Ƚϵ�where�־������
	Row row;		//Ҫ�����ֵ����
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
	}
};

#endif

#pragma once
#ifndef _CatalogManager_h_
#define _CatalogManager_h_

#include <string>
#include <vector>
#include <iostream>
#include "Macro.h"
#include "SQL.h"
using namespace std;

/*实现的功能： 
 *在程序运行之前把catalog信息读入内存 
 *在程序结束之前把catalog信息读入硬盘
 */

class CatalogManager{
private:
	vector<Table> tables;
	int tableNum;          //the number of tables in the database, should be equels to tables.size()
	vector<Index> indexes;                	//索引列表
	int indexNum;                   	//索引数目
private:
	void InitialTalbeCatalog(){ //打错字？
		const string filename = "table.catlog";
		fstream fin(filename.c_str(), ios::in);
		fin >> tableNum;
		for (int i = 0; i < tableNum; i++)
		{//fill in the vector of tables
			Table temp_table;
			fin >> temp_table.tableName;
			fin >> temp_table.attriNum;
			fin >> temp_table.blockNum;
			for (int j = 0; j < temp_table.attriNum; j++)
			{//fill in the vector of temp_table.attribute
				Attribute temp_attri;
				fin >> temp_attri.name;
				fin >> temp_attri.type;
				fin >> temp_attri.length;
				fin >> temp_attri.isPrimeryKey;
				fin >> temp_attri.isUnique;
				temp_table.attribute.push_back(temp_attri);
				temp_table.totalLength += temp_attri.length;
			}
			tables.push_back(temp_table);
		}
		fin.close();
	}
	
	void InitialIndexCatalog(){
		const string filename = "index.catlog";
		fstream fin(filename.c_str(), ios::in);
		fin >> indexNum;
		for (int i = 0; i < indexNum; i++)
		{//fill in the vector of indexes
			Index temp_index;
			fin >> temp_index.indexName;
			fin >> temp_index.tableName;
			fin >> temp_index.column;
			fin >> temp_index.columnLength;
			fin >> temp_index.blockNum;
			indexes.push_back(temp_index);
		}
		fin.close();
	}
	
	void StoreTalbeCatalog(){
		string filename = "table.catlog";
		fstream fout(filename.c_str(), ios::out);
		
		fout << tableNum << endl;
		for(int i = 0; i < tableNum; i++)
		{
			fout << tables[i].tableName << " ";
			fout << tables[i].attriNum << " ";
			fout << tables[i].blockNum << endl;
			
			for(int j = 0; j < tables[i].attriNum; j++)
			{
				fout << tables[i].attribute[j].name << " ";
				fout << tables[i].attribute[j].type << " ";
				fout << tables[i].attribute[j].length << " ";
				fout << tables[i].attribute[j].isUnique  << " ";
				fout << tables[i].attribute[j].isPrimeryKey  << endl;
		 	}
		}
		fout.close(); 
	}
	
	void StoreIndexCatalog(){
		string filename = "index.catlog";
		fstream fout(filename.c_str(), ios::out);
		fout << indexNum << endl;
		for(int i = 0; i < indexNum; i++)
		{
			fout << indexes[i].indexName << " ";
			fout << indexes[i].tableName << " ";
			fout << indexes[i].column << " ";
			fout << indexes[i].columnLength << " ";
			fout << indexes[i].blockNum << endl;
		}
		fout.close(); 
	}
public: 
	CatalogManager(){
		InitialTalbeCatalog();
		InitialIndexCatalog();
	}
	
	~CatalogManager(){
		StoreTalbeCatalog();
		StoreIndexCatalog();
	}

	void createTable(Table& table){
		tableNum++;
		for(int i = 0; i < table.attribute.size(); i++){
			table.totalLength += table.attribute[i].length;
		}
		tables.push_back(table);
		StoreTalbeCatalog(); 
	}

	void createIndex(Index index){
		indexNum++;
		indexes.push_back(index);
		StoreIndexCatalog(); 
	}

	void dropTable(Table table){
		dropTable(table.tableName);
	}
	
	void dropIndex(Index index){
		dropIndex(index.indexName);
	}
	
	void dropTable(string tablename){
		for(int i = tableNum -1; i >= 0; i--){
			if(tables[i].tableName == tablename){
				tables.erase (tables.begin()+i);
				tableNum--;
			}
		}
		StoreTalbeCatalog(); 
		for(int i = indexNum - 1; i >= 0; i--){//表被删除了，建在这个表上的所有的index也要跟着删除
			if(indexes[i].tableName == tablename){
				indexes.erase (indexes.begin()+i);
				indexNum--;
			}
		}
		StoreIndexCatalog(); 
	}
	
	void dropIndex(string indexName){
		for(int i = indexNum - 1; i >= 0; i--){
			if(indexes[i].indexName == indexName){
				indexes.erase (indexes.begin()+i);
				indexNum--;
			}
		}
		StoreIndexCatalog(); 
	}
	
	void update(Table& tableinfor){
		for(int i = 0; i < tableNum; i++){
			if(tables[i].tableName == tableinfor.tableName){
				tables[i].attriNum = tableinfor.attriNum;
				tables[i].blockNum = tableinfor.blockNum;
				tables[i].totalLength = tableinfor.totalLength;
				tables[i].attribute = tableinfor.attribute;
			}
		}
		StoreTalbeCatalog(); 
	}
	
	void update(Index& index){
		for(int i = 0; i< indexNum; i++){
			if(indexes[i].indexName == index.indexName){
				indexes[i].tableName = index.tableName;
				indexes[i].column = index.column;
				indexes[i].blockNum = index.blockNum;
			}
		}
		StoreIndexCatalog();
	}
	
	bool ExistTable(string tablename){
		int i;
		for(i=0;i<tables.size();i++){
			if(tables[i].tableName==tablename)
				return true;
		}
		return false;
	}
	
	bool ExistIndex(string tablename, int column){
		int i;
		for(i = 0; i < indexes.size(); i++){
			if(indexes[i].tableName == tablename && indexes[i].column==column)
				break;//found it
		}
		if(i >= indexes.size()) return 0;
		else return 1;
	}
	
	bool ExistIndex(string indexname){
		int i;
		for(i = 0; i <indexes.size(); i++){
			if(indexes[i].indexName == indexname)
				break;//found it
		}
		if(i >= indexes.size()) return 0;
		else return 1;
	}
	
	Table getTableInformation(string tablename){
		int i;
		Table temp;
		for(i=0;i<tableNum;i++){
			if((tables[i].tableName)==tablename){
				
				return tables[i];
			}
		}		
		return temp; //待议 
	}
	
	Index getIndexInformation(string tablename, int column){
		int i;
		for(i = 0; i < indexes.size(); i++){
			if(indexes[i].tableName == tablename && indexes[i].column==column)
				break;//found it
		}
		if(i >= indexNum){
			Index tmpt;
			return tmpt;//indicate that table information not found
		}
		return indexes[i];
	}
	
	Index getIndexInformation(string indexName){
		int i;
		for(i = 0; i < tableNum; i++){
			if(indexes[i].indexName == indexName)
				break;//found it
		}
		if(i >= indexNum){
			Index tmpt;
			return tmpt;//indicate that table information not found
		}
		return indexes[i];
	}

	void ShowCatalog(){
		ShowTableCatalog();
		ShowIndexCatalog();
	}
	
	void ShowTableCatalog(){//this method is for debug only
		cout << "##    Number of tables:" << tableNum << endl;
		for(int i = 0; i < tableNum; i++)
		{
			cout << "TABLE " << i << endl;
			cout << "Table Name: " << tables[i].tableName << endl;
			cout << "Number of attribute: "<< tables[i].attriNum << endl;
			cout << "Number of blocks occupied in disk: " << tables[i].blockNum << endl;
			for(int j = 0; j < tables[i].attriNum; j++)
			{
				cout << tables[i].attribute[j].name << "\t";
				switch(tables[i].attribute[j].type)
				{
					case STRING:	cout << "CHAR("<<tables[i].attribute[j].length << ")\t";	break;
					case INT:	cout << "INT\t";		break;
					case FLOAT:	cout << "FLOAT\t";	break;
					default:	cout << "UNKNOW TYPE\t";	break;
				}
				if(tables[i].attribute[j].isUnique)	cout << "Unique\t";
				else cout << "NotUnique ";
				if(tables[i].attribute[j].isPrimeryKey) cout << "PrimeryKey\t" << endl;
				else cout << endl;
		 	}
		}
	}
	
	void ShowIndexCatalog(){//this method is for debug also
		cout << "##    Number of indexes:" <<indexNum << endl;
		for(int i = 0; i < indexNum; i++)
		{
			cout << "INDEX " << i << endl;
			cout << "Index Name: " << indexes[i].indexName << endl;
			cout << "Table Name: "<< indexes[i].tableName << endl;
			cout << "Column Number: " << indexes[i].column << endl;
			cout << "Column Number of blocks occupied in disk: " << indexes[i].blockNum << endl;
		}
	}
	
	int GetColumnNumber(Table& tableinfo, string columnname)
	{
		for(int i=0;i<tableinfo.attribute.size();i++){
			if (tableinfo.attribute[i].name == columnname){
				return i;
			}
		}
		return -1;
	}

	int GetColumnAmount(Table& tableinfo){
		return tableinfo.attribute.size();
	}
};

#endif

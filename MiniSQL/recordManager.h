#pragma once
#ifndef _RECORD_H_
#define _RECORD_H_
#include "BufferManager.h"
#include "SQL.h"
#include "Macro.h"
#include <stdio.h>
#include <vector>

extern BufferManager buf;

class RecordManager
{
public:
	bool Comparator(Table tableinfor, Row row, vector<Condition> conditions)
	{
		for (int i = 0; i < conditions.size(); i++) {
			int colNum = conditions[i].columnNum;
			string value1 = "";
			string value2 = conditions[i].value;
			int length1 = 0;
			int length2 = value2.length();
			for (int k = 0; k < row.columns[colNum].length(); k++) {
				if (row.columns[colNum].c_str()[k] == EMPTY) {
					length1 = k;
					break;
				}
				value1 += row.columns[colNum].c_str()[k];
			}
			float fvalue1 = atof(value1.c_str());
			float fvalue2 = atof(value2.c_str());
			switch (tableinfor.attribute[colNum].type)
			{
			case STRING:
				switch (conditions[i].op)
				{
				case Lt:
					if (value1 >= value2) return 0;
					break;
				case Le:
					if (value1 > value2) return 0;
					break;
				case Gt:
					if (value1 <= value2) return 0;
					break;
				case Ge:
					if (value1 < value2) return 0;
					break;
				case Eq:
					if (value1 != value2) return 0;
					break;
				case Ne:
					if (value1 == value2) return 0;
					break;
				}
				break;
			case INT: //整型比较，先比较长度，比如说123坑定就比99大，长度相同345与543的比较同字符串
				switch (conditions[i].op)
				{
				case Lt:
					if (length1 > length2) return 0;
					else if (length1 < length2) break;
					else if (value1 >= value2) return 0;
					break;
				case Le:
					if (length1 > length2) return 0;
					else if (length1 < length2) break;
					else if (value1 > value2) return 0;
					break;
				case Gt:
					if (length1 < length2) return 0;
					else if (length1 > length2) break;
					else if (value1 <= value2) return 0;
					break;
				case Ge:
					if (length1 < length2) return 0;
					else if (length1 > length2) break;
					else if (value1 < value2) return 0;
					break;
				case Eq:
					if (length1 != length2) return 0;
					else if (value1 != value2) return 0;
					break;
				case Ne:
					if (length1 != length2) break;
					else if (value1 == value2) return 0;
					break;
				}
				break;
			case FLOAT:// return 0;//浮点数比较大小，不知道怎么方便 
				switch (conditions[i].op)
				{
				case Lt:
					if (fvalue1 >= fvalue2) return 0;
					break;
				case Le:
					if (fvalue1 > fvalue2) return 0;
					break;
				case Gt:
					if (fvalue1 <= fvalue2) return 0;
					break;
				case Ge:
					if (fvalue1 < fvalue2) return 0;
					break;
				case Eq:
					if (fvalue1 != fvalue2) return 0;
					break;
				case Ne:
					if (fvalue1 == fvalue2) return 0;
					break;
				}
			}
		}
		return 1;
	}

	Data select(Table& tableinfor) {
		string filename = tableinfor.tableName + ".table";
		string stringrow;
		Row splitedRow;
		Data datas;
		int length = tableinfor.totalLength + 1;//加多一位来判断这条记录是否被删除了
		const int recordNum = BLOCKSIZE / length;

		for (int blockOffset = 0; blockOffset < tableinfor.blockNum; blockOffset++) {
			int bufferNum = buf.getIfIsInBuffer(filename, blockOffset);
			if (bufferNum == -1) {
				bufferNum = buf.getEmptyBuffer();
				buf.readBlock(filename, blockOffset, bufferNum);
			}
			for (int offset = 0; offset < recordNum; offset++) {
				int position = offset * length;
				stringrow = buf.bufferBlock[bufferNum].getvalue(position, position + length);
				if (stringrow.c_str()[0] == EMPTY) continue;//inticate that this row of record have been deleted
				stringrow.erase(stringrow.begin());//把第一位去掉
				splitedRow = splitRow(tableinfor, stringrow);
				datas.rows.push_back(splitedRow);
			}
		}
		return datas;
	}

	Data select(Table tableinfor, vector<Condition> conditions) {
		Data datas;
		if (conditions.size() == 0) {
			datas = select(tableinfor);
			return datas;
		}
		string filename = tableinfor.tableName + ".table";
		//buf.scanIn(tableinfor);
		string stringrow;
		Row splitedRow;
		int length = tableinfor.totalLength + 1;//加多一位来判断这条记录是否被删除了
		const int recordNum = BLOCKSIZE / length;//加多一位来判断这条记录是否被删除了

		for (int blockOffset = 0; blockOffset < tableinfor.blockNum; blockOffset++) {
			int bufferNum = buf.getIfIsInBuffer(filename, blockOffset);
			if (bufferNum == -1) {
				bufferNum = buf.getEmptyBuffer();
				buf.readBlock(filename, blockOffset, bufferNum);
			}
			for (int offset = 0; offset < recordNum; offset++) {
				int position = offset * length;
				stringrow = buf.bufferBlock[bufferNum].getvalue(position, position + length);
				if (stringrow.c_str()[0] != EMPTY) {
					stringrow.erase(stringrow.begin());//把第一位去掉
					splitedRow = splitRow(tableinfor, stringrow);
					if (Comparator(tableinfor, splitedRow, conditions)) {//如果满足条件，就把结果push到datas里面去
						datas.rows.push_back(splitedRow);
					}
				}
			}
		}
		return datas;
	}

	void insertValue(Table& tableinfor, Row& splitedRow) {
		string stringrow = connectRow(tableinfor, splitedRow);
		insertPos iPos = buf.getInsertPosition(tableinfor);
		buf.bufferBlock[iPos.BLOCKNUM].value[iPos.position] = NOTEMPTY;
		for (int i = 0; i < tableinfor.totalLength; i++) {
			buf.bufferBlock[iPos.BLOCKNUM].value[iPos.position + i + 1] = stringrow.c_str()[i];
		}
		buf.bufferBlock[iPos.BLOCKNUM].Written = 1;
	}

	int deleteValue(Table tableinfor) {
		string filename = tableinfor.tableName + ".table";
		int count = 0;
		const int recordNum = BLOCKSIZE / (tableinfor.totalLength + 1);	//加多一位来判断这条记录是否被删除了
		for (int blockOffset = 0; blockOffset < tableinfor.blockNum; blockOffset++) {
			int bufferNum = buf.getIfIsInBuffer(filename, blockOffset);
			if (bufferNum == -1) {
				bufferNum = buf.getEmptyBuffer();
				buf.readBlock(filename, blockOffset, bufferNum);
			}
			for (int offset = 0; offset < recordNum; offset++) {
				int position = offset * (tableinfor.totalLength + 1);
				if (buf.bufferBlock[bufferNum].value[position] != EMPTY) {
					buf.bufferBlock[bufferNum].value[position] = EMPTY;
					count++;
				}
			}
		}
		return count;
	}

	int deleteValue(Table tableinfor, vector<Condition> conditions) {
		string filename = tableinfor.tableName + ".table";
		string stringrow;
		Row splitedRow;
		int count = 0;
		const int recordNum = BLOCKSIZE / (tableinfor.totalLength + 1);
		for (int blockOffset = 0; blockOffset < tableinfor.blockNum; blockOffset++) {
			int bufferNum = buf.getIfIsInBuffer(filename, blockOffset);
			if (bufferNum == -1) {
				bufferNum = buf.getEmptyBuffer();
				buf.readBlock(filename, blockOffset, bufferNum);
			}
			for (int offset = 0; offset < recordNum; offset++) {
				int position = offset * (tableinfor.totalLength + 1);
				stringrow = buf.bufferBlock[bufferNum].getvalue(position, position + tableinfor.totalLength + 1);
				if (stringrow.c_str()[0] != EMPTY) {
					stringrow.erase(stringrow.begin());//把第一位去掉
					splitedRow = splitRow(tableinfor, stringrow);
					if (Comparator(tableinfor, splitedRow, conditions)) {//如果满足条件，就把记录delete掉 
						buf.bufferBlock[bufferNum].value[position] = DELETED;
						count++;
					}
				}
			}
			buf.bufferBlock[bufferNum].Written = 1;
		}
		return count;
	}

	void dropTable(Table tableinfo) {
		string filename = tableinfo.tableName + ".table";
		if (remove(filename.c_str()) != 0)
			perror("Error deleting file");
		else
			buf.setInvalid(filename);//when a file is deleted, a table or an index, all the value in buffer should be set invalid
	}

	void createTable(Table tableinfo) {
		string filename = tableinfo.tableName + ".table";
		fstream  fout(filename.c_str(), ios::out);
		fout.close();
	}

private:
	Row splitRow(Table tableinfor, string row) {
		Row splitedRow;
		int s_pos = 0, f_pos = 0;//start position & finish position
		for (int i = 0; i < tableinfor.attriNum; i++) {
			s_pos = f_pos;
			f_pos += tableinfor.attribute[i].length;
			string col;
			for (int j = s_pos; j < f_pos; j++) {
				col += row[j];
			}
			splitedRow.columns.push_back(col);
		}
		return splitedRow;
	}

	string connectRow(Table tableinfor, Row splitedRow) {
		string tmptRow;
		string stringrow;
		for (int i = 0; i < splitedRow.columns.size(); i++) {
			tmptRow = splitedRow.columns[i];
			for (; tmptRow.length() < tableinfor.attribute[i].length; tmptRow += EMPTY);//补'@'以达到要求的长度
			stringrow += tmptRow;
		}
		return stringrow;
	}
public:
	void showDatas(const Data& datas) const {
		if (datas.rows.size() == 0)
		{
			cout << "No Datas" << endl;
			return;
		}
		for (int i = 0; i < datas.rows.size(); i++) {
			cout << (i + 1) << " ";//序号 
			for (int j = 0; j < datas.rows[i].columns.size(); j++) {
				for (int k = 0; k < datas.rows[i].columns[j].length(); k++)
					if (datas.rows[i].columns[j].c_str()[k] == EMPTY) break;
					else cout << datas.rows[i].columns[j].c_str()[k];
					cout << '\t';
			}
			cout << endl;
		}
	}
};
#endif







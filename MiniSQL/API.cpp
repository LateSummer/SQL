#include "API.h"

extern CatalogManager Catalog;
extern BufferManager buf;
extern Interpret ParseTree;
extern RecordManager record;
extern int fp;
extern fstream fin;

void Execute()
{
	Table tableinfor;
	Index indexinfor;
	string tempKeyValue;
	int tempPrimaryPosition = -1;
	int rowCount = 0;
	Data data;
	switch (ParseTree.m_operation)
	{
	case CRETAB:
		ParseTree.getTableInfo.attriNum = ParseTree.getTableInfo.attribute.size();
		Catalog.createTable(ParseTree.getTableInfo);
		record.createTable(ParseTree.getTableInfo);
		for (int i = 0; i < ParseTree.getTableInfo.attriNum; i++)
			if (ParseTree.getTableInfo.attribute[i].isPrimeryKey) {
				TYPE type = ParseTree.getTableInfo.attribute[i].type;
				if (type == INT) {
					IndexManager<int> indexint(indexinfor, tableinfor);
				}
				else if (type == FLOAT) {
					IndexManager<float> indexfloat(indexinfor, tableinfor);
				}
				else {
					IndexManager<string> indexstring(indexinfor, tableinfor);
				}
			}
		cout << "Table " << ParseTree.getTableInfo.tableName << " has been created successfully" << endl;
		break;
	case TABLEEXISTED:
		cout << "The table has been created, please check the database" << endl;
		break;
	case DRPTAB:
		record.dropTable(ParseTree.getTableInfo);
		for (int i = 0; i < ParseTree.getTableInfo.attriNum; i++) {	//把这各表所有的index都删掉
			indexinfor = Catalog.getIndexInformation(ParseTree.getTableInfo.tableName, i);
			if (indexinfor.indexName != "") dropIndex(indexinfor);
		}
		Catalog.dropTable(ParseTree.getTableInfo);
		cout << "Table " << ParseTree.getTableInfo.tableName << " has been dropped successfully" << endl;
		break;
	case INSERT:
		tableinfor = ParseTree.getTableInfo;
		if (ParseTree.PrimaryKeyPosition == -1 && ParseTree.UniquePostion == -1) {
			record.insertValue(tableinfor, ParseTree.row);
			Catalog.update(tableinfor);
			cout << "One record has been inserted successfully" << endl;
			break;
		}
		if (ParseTree.PrimaryKeyPosition != -1)
		{
			data = record.select(tableinfor, ParseTree.condition);
			if (data.rows.size() > 0) {
				cout << "Primary Key Redundancy occurs, thus insertion failed" << endl;
				break;
			}
		}
		if (ParseTree.UniquePostion != -1) {

			data = record.select(tableinfor, ParseTree.UniqueCondition);
			if (data.rows.size() > 0) {
				cout << "Unique Value Redundancy occurs, thus insertion failed" << endl;
				break;
			}
		}
		record.insertValue(tableinfor, ParseTree.row);
		Catalog.update(tableinfor);
		cout << "One record has been inserted successfully" << endl;
		break;
	case INSERTERR:
		cout << "Incorrect usage of \"insert\" query! Please check your input!" << endl;
		break;
	case SELECT_NOWHERE_CAULSE:
		tableinfor = ParseTree.getTableInfo;
		data = record.select(tableinfor);
		if (data.rows.size() != 0)
			ShowResult(data, tableinfor, ParseTree.column);
		else {
			cout << "No data is found!!!" << endl;
		}
		break;
	case SELECT_WHERE_CAULSE:
		tableinfor = ParseTree.getTableInfo;
		if (ParseTree.condition.size() == 1) {
			for (int i = 0; i < ParseTree.getTableInfo.attribute.size(); i++) {
				/*修改*/if ((ParseTree.getTableInfo.attribute[i].isPrimeryKey == true || ParseTree.getTableInfo.attribute[i].isUnique == true) && ParseTree.m_colname == ParseTree.getTableInfo.attribute[i].name) {
					tempPrimaryPosition = i;
					indexinfor = Catalog.getIndexInformation(tableinfor.tableName, i);
					break;
				}
			}
			if (tempPrimaryPosition == ParseTree.condition[0].columnNum && ParseTree.condition[0].op == Eq && indexinfor.tableName != "") {
				tempKeyValue = ParseTree.condition[0].value;
				stringstream ss;
				ss << tempKeyValue;
				if (indexinfor.keytype == INT) {
					IndexManager<int> indexint;
					int KeyValue;
					ss >> KeyValue;
					//indexint.selectEqual(KeyValue);
				}
				else if (indexinfor.keytype == FLOAT) {
					IndexManager<float> indexfloat;
					float KeyValue;
					ss >> KeyValue;
					//indexfloat.selectEqual(KeyValue);
				}
				else {
					IndexManager<string> indexstring;
					//indexstring.selectEqual(tempKeyValue);
				}
			}
			else {
				data = record.select(tableinfor, ParseTree.condition);
			}
		}
		else {
			data = record.select(tableinfor, ParseTree.condition);
		}
		if (data.rows.size() != 0)
			ShowResult(data, tableinfor, ParseTree.column);
		else {
			cout << "No data is found!!!" << endl;
		}
		break;
	case DELETE:
		rowCount = record.deleteValue(ParseTree.getTableInfo, ParseTree.condition);
		cout << rowCount << "  rows have been deleted." << endl;
		break;
	case CREIND:
		tableinfor = ParseTree.getTableInfo;
		indexinfor = ParseTree.getIndexInfo;
		if (!tableinfor.attribute[indexinfor.column].isPrimeryKey && !tableinfor.attribute[indexinfor.column].isUnique) {//不是primary key，不可以建index
			cout << "Column " << tableinfor.attribute[indexinfor.column].name << "  is not unique." << endl;
			break;
		}
		Catalog.createIndex(indexinfor);
		if (indexinfor.keytype == INT) {
			IndexManager<int> indexint(indexinfor, tableinfor);
		}
		else if (indexinfor.keytype == FLOAT) {
			IndexManager<float> indexfloat(indexinfor, tableinfor);
		}
		else {
			IndexManager<string> indexstring(indexinfor, tableinfor);
		}
		Catalog.update(indexinfor);
		cout << "The index " << indexinfor.indexName << " has been created successfully" << endl;
		break;
	case INDEXERROR:
		cout << "The index on primary key of table has been existed" << endl;
		break;
	case DRPIND:
		indexinfor = Catalog.getIndexInformation(ParseTree.m_indname);
		if (indexinfor.indexName == "") {
			cout << "Index" << ParseTree.m_indname << "does not exist!" << endl;
		}
		if (indexinfor.keytype == INT) {
			IndexManager<int> indexint(indexinfor, tableinfor);
		}
		else if (indexinfor.keytype == FLOAT) {
			IndexManager<float> indexfloat(indexinfor, tableinfor);
		}
		else {
			IndexManager<string> indexfloat(indexinfor, tableinfor);
		}
		dropIndex(indexinfor);
		Catalog.dropIndex(ParseTree.m_indname);
		cout << "The index has been dropped successfully" << endl;
		break;
	case CREINDERR:
		cout << "Incorrect usage of \"create index\" query! Please check your input!" << endl;
		break;
	case QUIT:
		cout << "Have a good day! Press any key to close this window." << endl;
		getchar();
		exit(0);
		break;
	case EXEFILE:
		fp = 1;
		fin.open(ParseTree.m_filename.c_str(), ios::in);
		if (fin.fail()) cout << "Open file fail!" << endl;
		break;
	case EXEFILERR:
		cout << "Incorrect usage of \"execfile\" query! Please check your input!" << endl;
		break;
	case EMPTY:
		cout << "Empty query! Please enter your command!" << endl;
		break;
	case UNKNOW:
		cout << "UNKNOW query! Please check your input!" << endl;
		break;
	case SELERR:
		cout << "Incorrect usage of \"select\" query! Please check your input!" << endl;
		break;
	case CRETABERR:
		cout << "Incorrect usage of \"create table\" query! Please check your input!" << endl;
		break;
	case DELETEERR:
		cout << "Incorrect usage of \"delete from\" query! Please check your input!" << endl;
		break;
	case DRPTABERR:
		cout << "Incorrect usage of \"drop table\" query! Please check your input!" << endl;
		break;
	case DRPINDERR:
		cout << "Incorrect usage of \"drop index\" query! Please check your input!" << endl;
		break;
	case VOIDPRI:
		cout << "Error: invalid primary key! Please check your input!" << endl;
		break;
	case VOIDUNI:
		cout << "Error: invalid unique key! Please check your input!" << endl;
		break;
	case CHARBOUD:
		cout << "Error: only 1~255 charactors is allowed! Please check your input!" << endl;
		break;
	case NOPRIKEY:
		cout << "No primary key is defined! Please check your input!" << endl;
		break;
	case TABLEERROR:
		cout << "Table is not existed,please check the database" << endl;
		break;
	case INDEXEROR:
		cout << "Index is not existed,please check the database" << endl;
		break;
	case COLUMNERROR:
		cout << "One column is not existed" << endl;
		break;
	case INSERTNUMBERERROR:
		cout << "The column number is not according to the columns in our database" << endl;
		break;
	}
}

void ShowResult(Data data, Table tableinfor, vector<Attribute> column) {
	if (column[0].name == "*") {
		cout << endl << "+";
		for (int i = 0; i < tableinfor.attriNum; i++) {
			for (int j = 0; j < tableinfor.attribute[i].length + 1; j++) {
				cout << "-";
			}
			cout << "+";
		}
		cout << endl;
		cout << "| ";
		for (int i = 0; i < tableinfor.attriNum; i++) {
			cout << tableinfor.attribute[i].name;
			int lengthLeft = tableinfor.attribute[i].length - tableinfor.attribute[i].name.length();
			for (int j = 0; j < lengthLeft; j++) {
				cout << ' ';
			}
			cout << "| ";
		}
		cout << endl;
		cout << "+";
		for (int i = 0; i < tableinfor.attriNum; i++) {
			for (int j = 0; j < tableinfor.attribute[i].length + 1; j++) {
				cout << "-";
			}
			cout << "+";
		}
		cout << endl;

		//内容
		for (int i = 0; i < data.rows.size(); i++) {
			cout << "| ";
			for (int j = 0; j < tableinfor.attriNum; j++) {
				int lengthLeft = tableinfor.attribute[j].length;
				for (int k = 0; k < data.rows[i].columns[j].length(); k++) {
					if (data.rows[i].columns[j].c_str()[k] == EMPTY) break;
					else {
						cout << data.rows[i].columns[j].c_str()[k];
						lengthLeft--;
					}
				}
				for (int k = 0; k < lengthLeft; k++) cout << " ";
				cout << "| ";
			}
			cout << endl;
		}

		cout << "+";
		for (int i = 0; i < tableinfor.attriNum; i++) {
			for (int j = 0; j < tableinfor.attribute[i].length + 1; j++) {
				cout << "-";
			}
			cout << "+";
		}
		cout << endl;
	}
	else {
		cout << endl << "+";
		for (int i = 0; i < column.size(); i++) {
			int col;
			for (col = 0; col < tableinfor.attribute.size(); col++) {
				if (tableinfor.attribute[col].name == column[i].name) break;
			}
			for (int j = 0; j < tableinfor.attribute[col].length + 1; j++) {
				cout << "-";
			}
			cout << "+";
		}
		cout << endl;
		cout << "| ";
		for (int i = 0; i < column.size(); i++) {
			int col;
			for (col = 0; col < tableinfor.attribute.size(); col++) {
				if (tableinfor.attribute[col].name == column[i].name) break;
			}
			cout << tableinfor.attribute[col].name;
			int lengthLeft = tableinfor.attribute[col].length - tableinfor.attribute[col].name.length();
			for (int j = 0; j < lengthLeft; j++) {
				cout << ' ';
			}
			cout << "| ";
		}
		cout << endl;
		cout << "+";
		for (int i = 0; i < column.size(); i++) {
			int col;
			for (col = 0; col < tableinfor.attribute.size(); col++) {
				if (tableinfor.attribute[col].name == column[i].name) break;
			}
			for (int j = 0; j < tableinfor.attribute[col].length + 1; j++) {
				cout << "-";
			}
			cout << "+";
		}
		cout << endl;

		//内容
		for (int i = 0; i < data.rows.size(); i++) {
			cout << "| ";
			for (int j = 0; j < column.size(); j++) {
				int col;
				for (col = 0; col < tableinfor.attribute.size(); col++) {
					if (tableinfor.attribute[col].name == column[j].name) break;
				}
				int lengthLeft = tableinfor.attribute[col].length;
				for (int k = 0; k < data.rows[i].columns[col].length(); k++) {
					if (data.rows[i].columns[col].c_str()[k] == EMPTY) break;
					else {
						cout << data.rows[i].columns[col].c_str()[k];
						lengthLeft--;
					}
				}
				for (int k = 0; k < lengthLeft; k++) cout << " ";
				cout << "| ";
			}
			cout << endl;
		}

		cout << "+";
		for (int i = 0; i < column.size(); i++) {
			int col;
			for (col = 0; col < tableinfor.attribute.size(); col++) {
				if (tableinfor.attribute[col].name == column[i].name) break;
			}
			for (int j = 0; j < tableinfor.attribute[col].length + 1; j++) {
				cout << "-";
			}
			cout << "+";
		}
		cout << endl;
	}
	cout << data.rows.size() << " rows have been found." << endl;
}
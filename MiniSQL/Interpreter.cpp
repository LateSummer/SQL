#include "Interpreter.h"

extern CatalogManager Catalog;

bool isChar(char c)
{
	if (c != '*' && c != ',' && c != '(' && c != ')' && c != ' ' && c != '\t' && c != '=' && c != '>'
		&& c != '<' && c != '\'' && c != 10 && c != 13) return true;
	else return false;
}

bool isValid(char c)
{
	if (c != ' ' && c != '\t' && c != 10 && c != 13) return true;
	else return false;
}

//This function is used to get a word ( maybe a 'token' is more appropriate )
//from the 'src' string and copy it into the 'des' string
bool getWord(string & src, string & des)
{
	des.clear();
	int cnt = 0;
	int len = src.length();

	for (; cnt < len; cnt++) {
		if (isValid(src[cnt])) break;
	}
	if (cnt == len) return false;

	if (src[cnt] == ',' || src[cnt] == '(' || src[cnt] == ')' ||
		src[cnt] == '*' || src[cnt] == '=' || src[cnt] == '\'') {
		des += src[cnt];
		src.erase(0, cnt + 1);
	}
	else
		if (src[cnt] == '<' || src[cnt] == '>')
		{
			if (src[cnt + 1] == '=' || (src[cnt] == '<' && src[cnt + 1] == '>')) {
				des += src[cnt];
				des += src[cnt + 1];
				src.erase(0, cnt + 2);
			}
			else {
				des += src[cnt];
				src.erase(0, cnt + 1);
			}
		}
		else {
			for (; cnt < len; cnt++)
				if (isChar(src[cnt]) && src[cnt] != ';') des += src[cnt];
				else break;
				src.erase(0, cnt);
		}
		return true;
}

//This function is used to get a string end with '
bool getStr(string& src, string& des)
{
	des.clear();
	int len = src.length();
	for (int i = 0; i < len; i++)
		if (src[i] != '\'') des += src[i];
		else {
			src.erase(0, i);
			return true;
		}
		return false;
}

bool same(string src, string des)
{
	if (src == des) return true;
	else return false;
}

Condition Interpret::getCon(string &cmd)
{
	string word;
	Condition tempcon;
	if (!getWord(cmd, word)) {
		tempcon.columnNum = -1;
		return tempcon;
	}

	tempcon.columnNum = Catalog.GetColumnNumber(getTableInfo, word);
	m_colname = word;
	if (tempcon.columnNum == -1) {
		m_operation = COLUMNERROR;
		return tempcon;
	}

	if (!getWord(cmd, word)) {
		tempcon.columnNum = -1;
		return tempcon;
	}

	if (word == "<") tempcon.op = Lt;
	else if (word == "<=") tempcon.op = Le;
	else if (word == ">") tempcon.op = Gt;
	else if (word == ">=") tempcon.op = Ge;
	else if (word == "=") tempcon.op = Eq;
	else if (word == "<>") tempcon.op = Ne;
	else {
		tempcon.columnNum = -1;
		return tempcon;
	}

	if (!getWord(cmd, word)) {
		tempcon.columnNum = -1;
		return tempcon;
	}
	if (word == "\'")
	{
		if (!getWord(cmd, word)) {
			tempcon.columnNum = -1;
			return tempcon;
		}
		tempcon.value = word;
		if (!getWord(cmd, word)) {
			tempcon.columnNum = -1;
			return tempcon;
		}
		if (!(word == "\'")) {
			tempcon.columnNum = -1;
			return tempcon;
		}
	}
	else
	{
		tempcon.value = word;
	}

	return tempcon;
}

Attribute Interpret::getCol(string &cmd)
{
	int FindPriKey = 0, FindUnique = 0;
	string word;
	Attribute tempcol;
	tempcol.type = FALSE;
	if (!getWord(cmd, word)) return tempcol;
	if (word == "primary")
	{
		if (!getWord(cmd, word)) return tempcol;
		if (!(word == "key")) return tempcol;
		if (!getWord(cmd, word)) return tempcol;
		if (strcmp(word.c_str(), "(") != 0) return tempcol;
		if (!getWord(cmd, word)) return tempcol;
		for (int i = 0; i < getTableInfo.attribute.size(); i++) {
			if (getTableInfo.attribute[i].name == word) {
				FindPriKey = 1;
				getTableInfo.attribute[i].isPrimeryKey = 1;
				getTableInfo.attribute[i].isUnique = 1;
			}
		}
		if (!FindPriKey)
		{
			m_operation = VOIDPRI;
			return tempcol;
		}
		FindPriKey = 0;
		if (!getWord(cmd, word)) return tempcol;
		return tempcol;
	}
	//如果是unique命令
	if (word == "unique")
	{
		if (!getWord(cmd, word)) return tempcol;
		if (word == "(") return tempcol;
		if (!getWord(cmd, word)) return tempcol;
		for (int i = 0; i < getTableInfo.attribute.size(); i++) {
			if (getTableInfo.attribute[i].name == word) {
				FindUnique = 1;
				getTableInfo.attribute[i].isUnique = 1;
			}
		}
		if (!FindUnique)
		{
			m_operation = VOIDUNI;
			return tempcol;
		}
		FindUnique = 0;
		if (!getWord(cmd, word)) return tempcol;
		return tempcol;
	}

	//如果不是unique与primary key命令
	tempcol.name = word;
	if (!getWord(cmd, word)) return tempcol;
	if (word == "int")
	{
		tempcol.type = INT;
		tempcol.length = INTLEN;
		if (!getWord(cmd, word)) return tempcol;
		if (word == "unique")
		{
			tempcol.isUnique = 1;
			if (!getWord(cmd, word)) return tempcol;
		}
		cmd = word + cmd;
		return tempcol;
	}
	if (word == "float")
	{
		tempcol.type = FLOAT;
		tempcol.length = FLOATLEN;
		if (!getWord(cmd, word)) return tempcol;
		if (word == "unique")
		{
			tempcol.isUnique = 1;
			if (!getWord(cmd, word)) return tempcol;
		}
		cmd = word + cmd;
		return tempcol;
	}
	if (word == "char")
	{
		tempcol.type = STRING;
		if (!getWord(cmd, word)) return tempcol;
		if (!(word == "("))	return tempcol;
		if (!getWord(cmd, word)) return tempcol;
		if (!isInt(word.c_str())) return tempcol;
		tempcol.length = atoi(word.c_str()) + 1;
		if (tempcol.length > 255 || tempcol.length < 1)
		{
			m_operation = CHARBOUD;
			return tempcol;
		}
		if (!getWord(cmd, word)) return tempcol;
		if (!(word == ")")) return tempcol;
		if (!getWord(cmd, word)) return tempcol;
		if (word == "unique")
		{
			tempcol.isUnique = 1;
			if (!getWord(cmd, word)) return tempcol;
		}
		cmd = word + cmd;
		return tempcol;
	}
	else return tempcol;
}

string Interpret::getRow(string &cmd)
{
	int flag;
	string word, temprow = "";
	if (!getWord(cmd, word)) return "";
	if (word == "\'")
	{
		if (!getStr(cmd, word)) return "";
		temprow = word;
		if (!getWord(cmd, word)) return "";
		if (!(word == "\'")) return "";
	}
	else
	{
		temprow = word;
	}
	return temprow;
}

//This function is used to parse the 'command' string which comes from user's 
//input, and return a corresponding parse parsetree
void Interpret::Parse(char* command)
{
	string word, temp = command;
	Attribute tempcol;
	Condition tempcon;
	string temprow;
	bool flag;

	makeInitilate();			//将解释树中的变量全部初始化为空值或NULL

	//把temp字符串中第一个空格前的单词去掉，并将该单词赋予WORD作为字符串,flag返回1
	if (!getWord(temp, word))					//若是空查询
	{
		m_operation = EMPTY;
		return;
	}

	if (word == "quit")			//若是退出命令
	{
		if (!getWord(temp, word)) m_operation = QUIT;
		return;
	}

	//若是select命令
	if (word == "select")
	{
		m_operation = SELERR;

		//解析select子句
		if (!getWord(temp, word)) return;

		tempcol.name = word;
		column.push_back(tempcol);		//在这里开始push要返回的列
		if (!getWord(temp, word)) return;

		while (word == ",")
		{
			if (!getWord(temp, word)) return;
			tempcol.name = word;
			column.push_back(tempcol);	//循环push要返回的列
			if (!getWord(temp, word)) return;
		}

		if (!(word == "from")) return;

		//解析from子句
		if (!getWord(temp, word)) return;
		m_tabname = word;

		if (!Catalog.ExistTable(word)) {
			m_operation = TABLEERROR;
			return;
		}

		getTableInfo = Catalog.getTableInformation(m_tabname);//在表存在的前提下获取表的信息

		//如果没有where子句，正确返回
		flag = getWord(temp, word);
		if (!flag)
		{
			m_operation = SELECT_NOWHERE_CAULSE;
			return;
		}
		if (!(word == "where")) return;

		//解析where子句
		tempcon = getCon(temp);
		if (tempcon.columnNum == -1) return;
		condition.push_back(tempcon);//开始打入条件容器
		flag = getWord(temp, word);
		if (!flag)
		{
			m_operation = SELECT_WHERE_CAULSE;
			return;
		}
		while (word == "and")
		{
			tempcon = getCon(temp);
			if (tempcon.columnNum == -1) return;
			condition.push_back(tempcon);
			if (!getWord(temp, word))
			{
				m_operation = SELECT_WHERE_CAULSE;
				return;
			}
		}
		return;
	}

	//若是create table/create index命令
	if (word == "create")
	{
		if (!getWord(temp, word)) return;
		//解析create table命令
		if (word == "table")
		{
			m_operation = CRETABERR;
			if (!getWord(temp, word)) return;
			m_tabname = word;
			if (Catalog.ExistTable(m_tabname)) {
				m_operation = TABLEEXISTED;
				return;
			}
			getTableInfo.tableName = word;
			if (!getWord(temp, word)) return;
			if (!(word == "("))	return;
			tempcol = getCol(temp);
			if (tempcol.type != FALSE) {
				getTableInfo.attribute.push_back(tempcol);//开始加入信息表中
			}
			if (!getWord(temp, word)) return;
			while (word == ",")
			{
				tempcol = getCol(temp);
				if (tempcol.type != FALSE) {
					getTableInfo.attribute.push_back(tempcol);
				}
				if (!getWord(temp, word)) return;
			};
			if (!(word == ")"))	return;
			if (!getWord(temp, word)) {
				m_operation = CRETAB;
			}
			return;
		}

		//解析create index命令
		if (word == "index")
		{
			m_operation = CREINDERR;
			if (!getWord(temp, word)) return;
			m_indname = word;
			getIndexInfo.indexName = word;
			if (Catalog.ExistIndex(m_indname)) {
				m_operation = INDEXERROR;
				return;
			}
			if (!getWord(temp, word)) return;
			if (!(word == "on")) return;
			if (!getWord(temp, word)) return;
			m_tabname = word;
			if (!Catalog.ExistTable(word)) {
				m_operation = TABLEERROR;
				return;
			}
			getTableInfo = Catalog.getTableInformation(m_tabname);
			getIndexInfo.tableName = word;
			if (!getWord(temp, word)) return;
			if (!(word == "(")) return;
			if (!getWord(temp, word)) return;
			int tempint;
			tempint = Catalog.GetColumnNumber(getTableInfo, word);
			if (tempint == -1) {
				m_operation = COLUMNERROR;
				return;
			}
			if (Catalog.ExistIndex(m_tabname, tempint)) {
				m_operation = INDEXERROR;
				return;
			}
			getIndexInfo.column = tempint;
			getIndexInfo.keytype = getTableInfo.attribute[tempint].type;
			getIndexInfo.blockNum = 0;
			getIndexInfo.columnLength = getTableInfo.attribute[tempint].length;
			if (!getWord(temp, word)) return;
			if (!(word == ")"))	return;
			if (!getWord(temp, word)) m_operation = CREIND;
			return;
		}
	}

	//若是delete from命令
	if (word == "delete")
	{
		if (!getWord(temp, word)) return;
		if (word == "from")
		{
			m_operation = DELETEERR;
			if (!getWord(temp, word)) return;
			m_tabname = word;
			if (!Catalog.ExistTable(word)) {
				m_operation = TABLEERROR;
				return;
			}
			//若没有where子句，正确返回
			getTableInfo = Catalog.getTableInformation(m_tabname);
			if (!getWord(temp, word))
			{
				m_operation = DELETE;
				return;
			}
			if (!(word == "where")) return;
			//开始解析where子句
			tempcon = getCon(temp);
			if (tempcon.columnNum == -1) return;
			condition.push_back(tempcon);
			if (!getWord(temp, word))
			{
				m_operation = DELETE;
				return;
			}
			while (word == "and") {
				tempcon = getCon(temp);
				if (tempcon.columnNum == -1) return;
				condition.push_back(tempcon);
				if (!getWord(temp, word))
				{
					m_operation = DELETE;
					return;
				}
			}
		}
		return;
	}

	//若是insert into命令
	if (word == "insert")
	{
		if (!getWord(temp, word)) return;
		if (word == "into")
		{
			m_operation = INSERTERR;
			if (!getWord(temp, word)) return;
			m_tabname = word;
			if (!Catalog.ExistTable(word)) {
				m_operation = TABLEERROR;
				return;
			}
			getTableInfo = Catalog.getTableInformation(m_tabname);

			if (!getWord(temp, word)) return;
			if (!(word == "values")) return;
			if (!getWord(temp, word)) return;
			if (!(word == "(")) return;
			temprow = getRow(temp);
			if (temprow == "") return;
			if (!getWord(temp, word)) return;
			row.columns.push_back(temprow);
			while (word == ",")
			{
				temprow = getRow(temp);
				if (temprow == "") return;
				row.columns.push_back(temprow);
				if (!getWord(temp, word)) return;
			}
			if (!(word == ")")) return;
			if (row.columns.size() != Catalog.GetColumnAmount(getTableInfo)) {
				m_operation = INSERTNUMBERERROR;
				return;
			}

			for (int i = 0; i < getTableInfo.attribute.size(); i++) {
				if (getTableInfo.attribute[i].isPrimeryKey) {
					PrimaryKeyPosition = i;
					tempcon.columnNum = i;
					tempcon.op = Eq;
					tempcon.value = row.columns[i];
					condition.push_back(tempcon);
				}
				if (getTableInfo.attribute[i].isPrimeryKey != 1 && getTableInfo.attribute[i].isUnique) {
					UniquePostion = i;
					tempcon.columnNum = i;
					tempcon.op = Eq;
					tempcon.value = row.columns[i];
					UniqueCondition.push_back(tempcon);
				}
			}
			if (!getWord(temp, word)) {
				m_operation = INSERT;
				return;
			}
			return;
		}
		return;
	}

	//若是drop table/drop index命令
	if (word == "drop")
	{
		if (!getWord(temp, word)) return;
		if (word == "table")
		{
			m_operation = DRPTABERR;
			if (!getWord(temp, word)) return;
			m_tabname = word;
			if (!Catalog.ExistTable(m_tabname)) {
				m_operation = TABLEERROR;
				return;
			}
			getTableInfo = Catalog.getTableInformation(m_tabname);
			if (!getWord(temp, word)) m_operation = DRPTAB;
			return;
		}
		if (word == "index")
		{
			m_operation = DRPINDERR;
			if (!getWord(temp, word)) return;
			m_indname = word;
			if (!Catalog.ExistIndex(m_indname)) {
				m_operation = INDEXEROR;
				return;
			}
			if (!getWord(temp, word)) m_operation = DRPIND;
		}
		return;
	}

	if (word == "execfile")
	{
		m_operation = EXEFILERR;
		if (!getWord(temp, word)) return;
		m_filename = word;
		if (!getWord(temp, word)) m_operation = EXEFILE;
	}

	return;
}

bool isInt(const char *input)
{
	int length = strlen(input);
	if (input[0] < '0' && input[0] > '9' && input[0] != '-') return 0;
	for (int i = 1; i < length; i++)
		if (input[i] < '0' && input[i] > '9') return 0;
	return 1;
}

bool isFloat(const char *input)
{
	int dot = 0;
	int length = strlen(input);
	if (input[0] < '0' && input[0] > '9' && input[0] != '-') return 0;
	for (int i = 1; i < length; i++) {
		if (input[i] == '.')
			if (dot) return 0;
			else dot = 1;
		else
			if (input[i] < '0' && input[i] > '9') return 0;
	}
	return 1;
}
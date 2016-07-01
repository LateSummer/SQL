#include "IndexManager.h"
#include <iterator>


int OutGetPointer(int bufferNum,int pos)//一个pointer最大5位数,有5个字节存储,从位置11开始储存
{
	int ptr = 0;
	for (int i = pos; i<pos + POINTERLENGTH; i++){
		ptr = 10 * ptr + buf.bufferBlock[bufferNum].value[i] - '0';
	}
	return ptr;
}
void OutSetPointer(int bufferNum, int position, int pointer)
{
	char tmpf[6];
	_itoa_s(pointer, tmpf, 10);
	string str = tmpf;
	while (str.length() < 5)
		str = '0' + str;
	memcpy(buf.bufferBlock[bufferNum].value + position, str.c_str(), 5);
	return;
}


/*
template <typename KeyType>
class IndexManager
{
public:
	Index treeIndex;
	Table treeTable;
	Branch<KeyType> rootB;
	Leaf<KeyType> rootL;
	string indexname;
	bool isLeaf;
public:
	IndexManager(const Index& _treeIndex, const Table& _treeTable);
	void creatIndex();
	void InitializeRoot();
	int getKeyPosition();
	~IndexManager(){};

public:
	bool search(const KeyType& mykey,int& offset,int bufferNum);
	bool findToLeaf(const KeyType& mykey, int& offset, int& bufferNum);

	recordPosition selectEqual(KeyType mykey);
	vector<recordPosition>& selectBetween(KeyType mykey1, KeyType mykey2);


	bool insertIndex(KeyType mykey, int blockNum, int blockOffset);//需要维护block的writeen
	bool insertBranch(KeyType mykey, int child,int bufferNum, int offset);
	bool insertBranch(KeyType mykey, int child, int branchBlock);
	bool insertLeaf(KeyType mykey, int blockNum, int blockOffset, int bufferNum,int offset);//将一个值插入到buffer中的一个叶子中的第offset的前一个位置
	bool insertLeaf(KeyType mykey, int blockNum, int blockOffset, int leafBlock);//将一个值插入到index文件中的第leafBlock中去

	void adjustafterinsert(int bufferNum);

	bool deleteIndex(KeyType mykey);
	bool deleteBranch(KeyType mykey, int branchBlock);
	bool deleteBranch(KeyType mykey, int bufferNum, int offset);
	bool deleteLeaf(KeyType mykey, int leafBlock);
	bool deleteLeaf(KeyType mykey, int bufferNum, int offset);

	void adjustafterdelete(int bufferNum);
	void deleteNode(int bufferNum);
};*/

void dropIndex(Index& indexinfor)
{
	string filename = indexinfor.indexName + ".index";
	buf.setInvalid(filename);
	remove(filename.c_str());
}

int getRecordNum(int bufferNum)
{
	int recordNum = 0;
	for (int i = 2; i<6; i++) {
		if (buf.bufferBlock[bufferNum].value[i] == EMPTY) break;
		recordNum = 10 * recordNum + buf.bufferBlock[bufferNum].value[i] - '0';
	}
	return recordNum;

}

void creatIndex(const Index& _treeIndex, const Table& _treeTable)
{
	if (_treeIndex.keytype==1)
		IndexManager<int>(_treeIndex, _treeTable);
	else if (_treeIndex.keytype==2)
		IndexManager<string>(_treeIndex, _treeTable);
	else if (_treeIndex.keytype == 3)
		IndexManager<float>(_treeIndex, _treeTable);
	
}

Row splitRow(Table tableinfor, string row)
{
	Row splitedRow;
	int s_pos = 0, f_pos = 0;//start position & finish position
	for (int i = 0; i < tableinfor.attriNum; i++){
		s_pos = f_pos;
		f_pos += tableinfor.attribute[i].length;
		string col;
		for (int j = s_pos; j < f_pos; j++){
			col += row[j];
		}
		splitedRow.columns.push_back(col);
	}
	return splitedRow;
}

Data select(Table tableinfor, recordPosition recordp)
{
	Data datas;
	string filename = tableinfor.tableName + ".table";
	string stringrow;
	Row splitedRow;
	int length = tableinfor.totalLength + 1;
	int bufferNum = buf.getIfIsInBuffer(filename, recordp.blockNum);
	if (bufferNum == -1)
	{
		bufferNum = buf.getEmptyBuffer();
		buf.readBlock(filename, recordp.blockNum, bufferNum);
	}
	stringrow = buf.bufferBlock[bufferNum].getvalue(recordp.blockPosition, recordp.blockPosition + length);
	if (stringrow.c_str()[0] != EMPTY)
	{
		stringrow.erase(stringrow.begin());
		splitedRow = splitRow(tableinfor, stringrow);
		datas.rows.push_back(splitedRow);
	}
	return datas;
}

Data select(Table tableinfor, vector<recordPosition>& recordps)
{
	Data datas;
	string filename = tableinfor.tableName + ".table";
	string stringrow;
	Row splitedRow;
	int length = tableinfor.totalLength + 1;
	int bufferNum;
	for (int i = 0; i<recordps.size(); i++)
	{
		bufferNum = buf.getIfIsInBuffer(filename, recordps[i].blockNum);
		if (bufferNum == -1)
		{
			bufferNum = buf.getEmptyBuffer();
			buf.readBlock(filename, recordps[i].blockNum, bufferNum);
		}
		stringrow = buf.bufferBlock[bufferNum].getvalue(recordps[i].blockPosition, recordps[i].blockPosition + length);
		if (stringrow.c_str()[0] != EMPTY)
		{
			stringrow.erase(stringrow.begin());
			splitedRow = splitRow(tableinfor, stringrow);
			datas.rows.push_back(splitedRow);
		}
	}
	return datas;
}

#pragma once
#ifndef _INDEX_H
#define _INDEX_H
#include "BufferManager.h"
#include "recordManager.h"
#include "SQL.h"
#include <list>
#include <typeinfo>
#include <iostream>
#include <iterator>
#include <sstream>
#define POINTERLENGTH 5
extern BufferManager buf;

void OutSetPointer(int bufferNum, int position, int pointer);
int OutGetPointer(int bufferNum, int pos);

typedef int POINTER;

class recordPosition
{
public:
	int blockNum;
	int blockPosition;
	recordPosition() :blockNum(-1), blockPosition(0) {}
	recordPosition(int blockNum, int blockPosition) :blockNum(blockNum), blockPosition(blockPosition) {}
};

void dropIndex(Index& indexinfor);
template <typename KeyType> KeyType getValue(int bufferNum, int position, int length);
template <typename KeyType> KeyType getTableValue(int bufferNum, int position, int length);
template <typename KeyType> void writeValue(int bufferNum, int position, int length, KeyType K);
int getRecordNum(int bufferNum);
Data select(Table tableinfor, vector<recordPosition>& recordps);
Data select(Table tableinfor, recordPosition recordp);


template <typename KeyType>
class leafUnit{//a part of a leaf,多个leafUnit构成一个B+数leaf节点,一个B+树的节点为一个block
public:
	KeyType key;
	recordPosition POS;
	template <typename KeyType> leafUnit() :key(KeyType()),POS(0,0){}
	template <typename KeyType> leafUnit(KeyType k, int oif, int oib) : key(k), POS(oif, oib){}
};

template <typename KeyType>
class branchUnit{//not a leaf, normal node,多个indexbranch构成一个B+数的节点,一个B+树的节点为一个block
public:
	KeyType key;
	int ptrChild;	//block pointer,胭脂斋批：这里所谓的指针其实就是block在文件中的偏移,索引储存在index文件中,由叶子节点组成,每个leaf就是一个block,这里指block的编号
	template <typename KeyType> branchUnit() :key(KeyType()), ptrChild(0){}
	template <typename KeyType> branchUnit(KeyType k, int ptrC) : key(k), ptrChild(ptrC){}
};

template <typename KeyType>
class fatherNode{
public:
	bool isRoot;
	int bufferNum;      //记录了现在这个节点在buf中的哪一个节点
	int father;		//block pointer, if is root, this pointer is useless,记录了它的父节点在index文件中所在block的位置
	int recordNum;
	int columnLength;  //index的key的数据长

	fatherNode()
	{
		bufferNum = -1;
		recordNum = -1;
	}
	fatherNode(int bufferNum) : bufferNum(bufferNum), recordNum(0){}
	int getPointer(int pos)//一个pointer最大5位数,有5个字节存储,从位置11开始储存
	{
		int ptr = 0;
		for (int i = pos; i<pos + POINTERLENGTH; i++){
			ptr = 10 * ptr + buf.bufferBlock[bufferNum].value[i] - '0';
		}
		if (ptr == -29)
			ptr = -1;
		return ptr;
	}
	void writePointer(int position, POINTER P)//一个pointer最大5位数,有5个字节存储,从位置11开始储存
	{
		char tmpf[6];
		_itoa(P, tmpf, 10);
		string str = tmpf;
		while (str.length() < 5)
			str = '0' + str;
		memcpy(buf.bufferBlock[bufferNum].value + position, str.c_str(), 5);
	}
	int getRecordNum(){//这个block中的record的个数,最大4位数,由4个字节存储,为这个block的2~5位
		int recordNum = 0;
		for (int i = 2; i<6; i++){
			if (buf.bufferBlock[bufferNum].value[i] == EMPTY) break;
			recordNum = 10 * recordNum + buf.bufferBlock[bufferNum].value[i] - '0';
		}
		return recordNum;
	}
	
};

//在branch节点这个block中,第0位是否为'R'代表这是否是个root,第1位是否为'L'代表这是否是个leaf,这个block中的record的个数,最大4位数,由4个字节存储,为这个block的2~5位,6~10位为parent指针,
//11~15位为这个节点的第一个指针,从16位开始由一个有一个的key+child_pointer组成,每一个占据columnLength+5位(这里的位指最小单位：字节)
//unleaf node
template <typename KeyType>
class Branch : public fatherNode<KeyType>
{
public:
	vector<KeyType> key;
	vector<POINTER> child;
	int degree;
public:
	Branch(){}
	Branch(int _bufferNum) : fatherNode<KeyType>(_bufferNum)
	{
		this->bufferNum = _bufferNum;
		this->child.clear();
		this->key.clear();
		this->columnLength = 0;
		this->isRoot = 0;
		this->recordNum = 0;
		this->father = -1;
		this->degree = 0;
	}//this is for new added brach
	Branch(int _bufferNum, const Index& indexinfor);
	~Branch()
	{
		this->child.clear();
		this->key.clear();
	}
	
public:
	bool search(KeyType mykey, int& offset);//在这个节点中搜索key，若存在则返回这个的key的编号，若不存在则返回这个key肯能存在的子节点的指针编号，返回通过index实现
	void writeBack();//将这个节点的数据存入内存buffer中的一个block

};

template <typename KeyType>
bool Branch<KeyType>::search(KeyType mykey, int& offset)
{
	offset = 0;
	int cou;
	int count = this->key.size();
	if (count == 0)
	{
		offset = 0;
		return 0;
	}
	else if (count <= 20)// sequential search
	{
		for (cou = 0; cou < count; cou++)
		{
			if (mykey < key[cou])
			{
				offset = cou;
				return 0;
			}
			else if (mykey == key[cou])
			{
				offset = cou;
				return 1;
			}
			else
				continue;
		}
		offset = count;
		return 0;
	}
	else// too many key, binary search. 2* log(n,2) < (1+n)/2
	{
		int start = 0;
		int tail = count - 1;
		int mid;
		if (mykey < key[0])
		{
			offset = 0;
			return 0;
		}
		else if (mykey>key[count - 1])
		{
			offset = count;
			return 0;
		}
		while ((start + 1) < tail)
		{
			mid = (start + tail) / 2;
			if (mykey < key[mid])
			{
				tail = mid;
			}
			else if (mykey == key[mid])
			{
				offset = mid;
				return 1;
			}
			else
			{
				start = mid;
			}
		}
		if (mykey == key[start])
		{
			offset = start;
			return 1;
		}
		else if (mykey == key[tail])
		{
			offset = tail;
			return 1;
		}
		else
		{
			offset = tail;
			return 0;
		}


	}

	return 0;
}

template <typename KeyType> 
Branch<KeyType>::Branch(int _bufferNum, const Index& indexinfor) : fatherNode<KeyType>(_bufferNum)//从这个block中读出数据构成branch节点,一个节点即一个nodelist
{	
	POINTER ptr = 0;
	this->bufferNum = _bufferNum;
	this->isRoot = (buf.bufferBlock[this->bufferNum].value[0] == 'R');//第一位是否为'R'代表这是否是个root
	int recordCount = getRecordNum();
	this->recordNum = 0;//recordNum will increase when function insert is called, and finally as large as recordCount
	this->father = getPointer(6);//6~10位为parent指针
	this->columnLength = indexinfor.columnLength;
	this->degree = indexinfor.degree;
	int position = 6 + POINTERLENGTH;
	for (int i = position; i<position + POINTERLENGTH; i++)
	{
		ptr = 10 * ptr + buf.bufferBlock[bufferNum].value[i] - '0';
	}
	this->child.push_back(ptr);
	position += POINTERLENGTH;
	for (int i = 0; i < recordCount; i++)
	{
		KeyType K;
		K = getValue<KeyType>(this->bufferNum,position,this->columnLength);
		this->key.push_back(K);
		position += this->columnLength;

		int ptrChild = getPointer(position);
		position += POINTERLENGTH;
		this->child.push_back(ptrChild);
		this->recordNum++;
	}
	buf.bufferBlock[this->bufferNum].Lock = 1;
}

template <typename KeyType>
KeyType getValue(int bufferNum, int position, int length)
{
	KeyType K=KeyType();
	string str = "";
	if (typeid(KeyType) == typeid(int))
	{
		char ch[4];
		for (int i = position; i < position + 4; i++)
		{			
			ch[i-position]= buf.bufferBlock[bufferNum].value[i];			
		}
		int res=ch[0];
		res = (res << 8) + ch[1];
		res = (res << 8) + ch[2];
		res = (res << 8) + ch[3];
		K = res;
	}
	else if (typeid(KeyType) == typeid(string))
	{
		for (int i = position; i < position + length; i++)
		{
			K += buf.bufferBlock[bufferNum].value[i];
		}		
	}
	else if (typeid(KeyType) == typeid(float))
	{
		char ch[4];
		for (int i = position; i < position + 4; i++)
		{
			ch[i - position] = buf.bufferBlock[bufferNum].value[i];
		}
		int res = ch[0];
		res = (res << 8) + ch[1];
		res = (res << 8) + ch[2];
		res = (res << 8) + ch[3];
		int* resP = &res;
		float* FLP=reinterpret_cast<float*>(resP);
		K = *FLP;
		/*unsigned int num=0;
		for (int i = position; i < position + length; i++)
		{
			num += buf.bufferBlock[bufferNum].value[i];
		}
		unsigned int* Pnum = &num;
		float* PF;
		PF = reinterpret_cast<float*>(Pnum);
		K = *PF;*/
	}
	else
	{
		cout << "invaild type  impossible in get value!!!" << endl;
		exit(1);
	}
	return K;
}
template <typename KeyType>
KeyType getTableValue(int bufferNum, int position, int length)
{
	KeyType K = KeyType();
	string str = "";
	if (typeid(KeyType) == typeid(int))
	{
		for (int i = position; i < position + length; i++)
		{
			str += buf.bufferBlock[bufferNum].value[i];
		}
		while (str[0] == '0')
			str.erase(0, 1);
		stringstream ss;
		ss << str;
		int res;
		ss >> res;
		K = res;
	}
	else if (typeid(KeyType) == typeid(string))
	{
		for (int i = position; i < position + length; i++)
		{
			K += buf.bufferBlock[bufferNum].value[i];
		}
	}
	else if (typeid(KeyType) == typeid(float))
	{
		for (int i = position; i < position + length; i++)
		{
			str += buf.bufferBlock[bufferNum].value[i];
		}
		while (str[0] == '0')
			str.erase(0, 1);
		if (str[0] == '.')
			str.insert(str.begin(), '0');
		stringstream ss;
		ss << str;
		float res;
		ss >> res;
		K = res;
		/*unsigned int num=0;
		for (int i = position; i < position + length; i++)
		{
		num += buf.bufferBlock[bufferNum].value[i];
		}
		unsigned int* Pnum = &num;
		float* PF;
		PF = reinterpret_cast<float*>(Pnum);
		K = *PF;*/
	}
	else
	{
		cout << "invaild type  impossible in get value!!!" << endl;
		exit(1);
	}
	return K;
}
template <typename KeyType>
void writeValue(int bufferNum, int position, int length, KeyType K)
{
	if (typeid(KeyType) == typeid(int))
	{
		char ch[4];
		KeyType* KP = &K;
		int *resP = reinterpret_cast<int*>(KP);
		int res = *resP;
		ch[3] = res;
		ch[2] = res >> 8;
		ch[1] = res >> 16;
		ch[0] = res >> 24;
		memcpy(buf.bufferBlock[bufferNum].value + position, ch, 4);
	}
	else if (typeid(KeyType) == typeid(string))
	{
		KeyType* KP = &K;
		string *resP = reinterpret_cast<string*>(KP);
		for (int i = position; i < position + length; i++)
		{
			char ch = (*resP)[i - position];
			memcpy(buf.bufferBlock[bufferNum].value + i, &ch, 1);
		}
	}
	else if (typeid(KeyType) == typeid(float))
	{
		char ch[4];
		KeyType* KP = &K;
		int *resP = reinterpret_cast<int*>(KP);
		int res = *resP;
		ch[3] = res;
		ch[2] = res >> 8;
		ch[1] = res >> 16;
		ch[0] = res >> 24;
		memcpy(buf.bufferBlock[bufferNum].value + position, ch, 4);
	}
	else
	{
		cout << "invaild type  impossible in write value!!!" << endl;
		exit(1);
	}
	return;
}



template <typename KeyType>
void Branch<KeyType>::writeBack()//将这个节点的数据存入内存buffer中的一个block
{
	//isRoot
	if (this->isRoot) buf.bufferBlock[bufferNum].value[0] = 'R';
	else buf.bufferBlock[bufferNum].value[0] = '_';
	//is not a Leaf
	buf.bufferBlock[bufferNum].value[1] = '_';
	//recordNum
	char tmpt[5];
	_itoa(recordNum, tmpt, 10);
	string strRecordNum = tmpt;
	while (strRecordNum.length() < 4)
		strRecordNum = '0' + strRecordNum;
	memcpy(buf.bufferBlock[bufferNum].value + 2, strRecordNum.c_str(), 4);
	//father
	int position = 6;
	writePointer(position, this->father);
	
	//nodelist
	position = 6 + POINTERLENGTH;	//前面的几位用来存储index的相关信息了
	writePointer(position, this->child[0]);
	for (int cou = 0; cou < this->recordNum;cou++)
	{
		writeValue<KeyType>(this->bufferNum, position, this->columnLength, this->key[cou]);
		position += columnLength;
		writePointer(position, this->child[cou + 1]);
		position += POINTERLENGTH;
	}
	buf.writeBlock(this->bufferNum);
	buf.bufferBlock[this->bufferNum].Lock = 0;
	buf.flashBack(this->bufferNum);
}

//在leaf节点这个block中,第0位是否为'R'代表这是否是个root,第1位是否为'L'代表这是否是个leaf,这个block中的record的个数,最大4位数,由4个字节存储,为这个block的2~5位,6~10位为parent指针,
//11~15位为上一个叶子指针,16~20为下一个叶子指针,21位开始由一个有一个的key(columnLength)+offsetInFile(5)+offsetInBlock(5)(指这条记录在文件中的那一个block,这个block中的第几个位置)组成,每一个占据columnLength+5位+5位(这里的位指最小单位：字节)

template <typename KeyType> 
class Leaf : public fatherNode<KeyType>//将存储在buffer[bufferNum]中
{
public:
	POINTER nextleaf;	//block pointer
	POINTER lastleaf;	//block pointer
	vector<KeyType> key;
	vector<recordPosition> POS;
	int degree;
public:
	Leaf(){}
	Leaf(int _bufferNum)//this kind of leaf is added when old leaf is needed to be splited
	{	
		this->bufferNum = _bufferNum;
		this->POS.clear();
		this->key.clear();
		this->columnLength = 0;
		this->isRoot = 0;
		this->recordNum = 0;
		this->father = -1;
		this->nextleaf = -1;
		this->lastleaf = -1;
		this->degree = 0;
	}
	Leaf(int _bufferNum, const Index& indexinfor);
	~Leaf()
	{
		this->POS.clear();
		this->key.clear();
	}

public:
	bool search(KeyType mykey, int& offset);
	void writeBack();
};



template <typename KeyType>
Leaf<KeyType>::Leaf(int _bufferNum, const Index& indexinfor) :fatherNode<KeyType>(_bufferNum)//从BM的这个这个block中读出数据构成leaf节点,一个节点即一个nodelist
{
	POINTER ptr = 0;
	this->bufferNum = _bufferNum;
	this->isRoot = (buf.bufferBlock[this->bufferNum].value[0] == 'R');
	int recordCount = getRecordNum();
	this->recordNum = 0;
	this->father = getPointer(6);//6~10位为parent指针
	this->lastleaf = getPointer(11);
	this->nextleaf = getPointer(16);
	this->columnLength = indexinfor.columnLength;	//保存起来以后析构函数还要用到的
	this->degree = indexinfor.degree;
	//cout << "recordCount = "<< recordCount << endl;
	int position = 6 + POINTERLENGTH * 3;	//前面的几位用来存储index的相关信息了
	for (int i = 0; i < recordCount; i++)
	{
		KeyType K;
		K = getValue<KeyType>(this->bufferNum, position, this->columnLength);
		this->key.push_back(K);
		this->columnLength = 4;
		position += this->columnLength;

		recordPosition R;
		R.blockNum = getPointer(position);
		position += POINTERLENGTH;
		R.blockPosition = getPointer(position);
		position += POINTERLENGTH;
		this->POS.push_back(R);
		this->recordNum++;

	}
	buf.bufferBlock[this->bufferNum].Lock = 1;
}



template <typename KeyType>
void Leaf<KeyType>::writeBack()//将这个节点的数据存入内存buffer中的一个block
{
	//isRoot
	if (this->isRoot) buf.bufferBlock[bufferNum].value[0] = 'R';
	else buf.bufferBlock[bufferNum].value[0] = '_';
	//isLeaf
	buf.bufferBlock[bufferNum].value[1] = 'L';
	//recordNum
	char tmpt[5];
	_itoa(recordNum, tmpt, 10);
	string str = tmpt;
	while (str.length() < 4)
		str = '0' + str;
	int position = 2;
	memcpy(buf.bufferBlock[bufferNum].value + position, str.c_str(), 4);
	position += 4;

	writePointer(position, this->father);
	position += POINTERLENGTH;
	writePointer(position, this->lastleaf);
	position += POINTERLENGTH;
	writePointer(position, this->nextleaf);
	position += POINTERLENGTH;
	//nodelist

	for (int cou = 0; cou < this->recordNum; cou++)
	{
		this->columnLength = 4;
		writeValue<KeyType>(this->bufferNum, position, this->columnLength, this->key[cou]);
		position += columnLength;
		writePointer(position, this->POS[cou].blockNum);
		position += POINTERLENGTH;
		writePointer(position, this->POS[cou].blockPosition);
		position += POINTERLENGTH;
	}
	buf.bufferBlock[this->bufferNum].Lock = 0;
	buf.writeBlock(this->bufferNum);
	buf.flashBack_no_initial(this->bufferNum);
}

template <typename KeyType>
bool Leaf<KeyType>::search(KeyType mykey, int& offset)
{
	offset = 0;
	int cou;
	int count = this->key.size();
	if (count == 0)
	{
		offset = 0;
		return 0;
	}
	else if (count <= 20)// sequential search
	{
		for (cou = 0; cou < count; cou++)
		{
			if (mykey < key[cou])
			{
				offset = cou;
				return 0;
			}
			else if (mykey == key[cou])
			{
				offset = cou;
				return 1;
			}
			else
				continue;
		}
		offset = count;
		return 0;
	}
	else// too many key, binary search. 2* log(n,2) < (1+n)/2
	{
		int start = 0;
		int tail = count - 1;
		int mid;
		if (mykey < key[0])
		{
			offset = 0;
			return 0;
		}
		else if (mykey>key[count - 1])
		{
			offset = count;
			return 0;
		}
		while ((start + 1) < tail)
		{
			mid = (start + tail) / 2;
			if (mykey < key[mid])
			{
				tail = mid;
			}
			else if (mykey == key[mid])
			{
				offset = mid;
				return 1;
			}
			else
			{
				start = mid;
			}
		}
		if (mykey == key[start])
		{
			offset = start;
			return 1;
		}
		else if (mykey == key[tail])
		{
			offset = tail;
			return 1;
		}
		else
		{
			offset = tail;
			return 0;
		}


	}

	return 0;
}
//**********************BplusTree***************************//


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
	IndexManager(){};
	void creatIndex();
	void InitializeRoot();
	int getKeyPosition();
	~IndexManager(){};

public:
	bool search(const KeyType& mykey, int& offset, int bufferNum);
	bool findToLeaf(const KeyType& mykey, int& offset, int& bufferNum);

	Data selectEqual(KeyType mykey);
	Data selectBetween(KeyType mykey1, KeyType mykey2);


	bool insertIndex(KeyType mykey, int blockNum, int blockOffset);//需要维护block的writeen
	bool insertBranch(KeyType mykey, int child, int bufferNum, int offset);
	bool insertBranch(KeyType mykey, int child, int branchBlock);
	bool insertLeaf(KeyType mykey, int blockNum, int blockOffset, int bufferNum, int offset);//将一个值插入到buffer中的一个叶子中的第offset的前一个位置
	bool insertLeaf(KeyType mykey, int blockNum, int blockOffset, int leafBlock);//将一个值插入到index文件中的第leafBlock中去

	void adjustafterinsert(int bufferNum);

	bool deleteIndex(KeyType mykey);
	bool deleteBranch(KeyType mykey, int branchBlock);
	bool deleteBranch(KeyType mykey, int bufferNum, int offset);
	bool deleteLeaf(KeyType mykey, int leafBlock);
	bool deleteLeaf(KeyType mykey, int bufferNum, int offset);

	void adjustafterdelete(int bufferNum);
	void deleteNode(int bufferNum);
};






template <typename KeyType>
IndexManager<KeyType>::IndexManager(const Index& _treeIndex, const Table& _treeTable)//需要维护：blockNum;degree;leafhead;
{
	treeIndex = _treeIndex;
	treeTable = _treeTable;
	char ch;
	indexname = treeIndex.indexName + ".index";
	treeIndex.degree = (BLOCKSIZE - 30) / (this->treeIndex.columnLength + 2 * POINTERLENGTH);//recordNum最大degree-1个
	fstream  fp(indexname.c_str(), ios::out | ios::binary);
	fp.get();
	if (fp.eof())
	{
		fp.close();
		creatIndex();
		return;
	}

	int bufferNum = buf.getEmptyBufferExceptFilename(indexname);
	buf.readBlock(indexname, 0, bufferNum);

	if (buf.bufferBlock[bufferNum].value[0] != 'R')
	{
		cout << "indexfile " << indexname << " exist error!!!" << endl;
		exit(1);
	}
	else
	{
		buf.bufferBlock[bufferNum].Lock = 1;
		if (buf.bufferBlock[bufferNum].value[1] == 'L')
		{
			Leaf<KeyType> NEW(bufferNum, this->treeIndex);
			this->rootL = NEW;
			this->isLeaf = 1;
		}
		else
		{
			Branch<KeyType> NEW(bufferNum, this->treeIndex);
			this->rootB = NEW;
			this->isLeaf = 0;
		}
	}
}

//在这个节点中搜索key，若存在则返回这个的key的编号，若不存在则返回这个key肯能存在的子节点的指针编号，返回通过offset实现
template <typename KeyType>
bool IndexManager<KeyType>::search(const KeyType& mykey, int& offset, int bufferNum)
{
	bool res;
	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		Leaf<KeyType> node(bufferNum, this->treeIndex);
		res = node.search(mykey, offset);
	}
	else
	{
		Branch<KeyType> node(bufferNum, this->treeIndex);
		res = node.search(mykey, offset);
	}

	return res;
}

//在这棵树中搜索key，若存在则返回这个叶子节点在内存中的bufferNum和这个key在这个节点中是第几个
template <class KeyType>
bool IndexManager<KeyType>::findToLeaf(const KeyType& mykey, int& offset, int& bufferNum)//在这棵树中搜索key，若存在则返回这个叶子节点在内存中的bufferNum和这个key在这个节点中是第几个，若不存在则返回这个key肯能存在的子节点的指针编号，返回通过index实现
{
	POINTER myChild;
	if (this->search(mykey, offset, bufferNum))
	{
		if (buf.bufferBlock[bufferNum].value[1] == 'L')
		{
			bufferNum = bufferNum;
			return 1;
		}
		else
		{
			myChild = OutGetPointer(bufferNum, 16 + offset*(treeIndex.columnLength + 5) + treeIndex.columnLength);
			bufferNum = buf.getBlockNum(this->indexname, myChild);
			while (buf.bufferBlock[bufferNum].value[1] != 'L')
			{
				myChild = OutGetPointer(bufferNum, 11);
				bufferNum = buf.getBlockNum(this->indexname, myChild);
			}
			offset = 0;
			return 1;

		}
	}
	else
	{
		if (buf.bufferBlock[bufferNum].value[1] == 'L')
		{
			bufferNum = bufferNum;
			return 0;
		}
		else
		{
			myChild = OutGetPointer(bufferNum, 16 + (offset - 1)*(treeIndex.columnLength + 5) + treeIndex.columnLength);

			bufferNum = buf.getBlockNum(this->indexname, myChild);
			return IndexManager<KeyType>::findToLeaf(mykey, offset, bufferNum);
		}
	}
}

template <class KeyType>
void IndexManager<KeyType>::InitializeRoot()
{
	int bufferNum = buf.addBlockInFile(this->treeIndex);
	this->isLeaf = 1;
	Leaf<KeyType> NEW(bufferNum, this->treeIndex);
	this->rootL = NEW;
	this->rootL.father = -1;
	this->rootL.lastleaf = -1;
	this->rootL.nextleaf = -1;
	this->rootL.columnLength = treeIndex.columnLength;
	this->rootL.isRoot = 1;
	this->rootL.degree = treeIndex.degree;
	this->rootL.writeBack();
	buf.bufferBlock[bufferNum].Lock = 1;
	return;
}
//把一个节点从buffer读到节点中只需要node(bufferNum)即可，写回去只要node.writeBack()即可，将一个buffer写到文件只要处理Lock和buf.flashBack(bufferNum)即可
//把文件中的block写到buffer中只要getBlockNum(string filename, int blockOffset)即可，若要在文件中新加block只要addBlockInFile()在处理新加index节点的isRoot和columnLength即可

//每次把节点写回buffer中之后要考虑是否还要用这个节点来决定是否锁住这个buffer
template <typename KeyType>
bool IndexManager<KeyType>::insertIndex(KeyType mykey, int blockNum, int blockOffset)
{
	if (this->rootL.key.empty() && this->rootB.key.empty())
		this->InitializeRoot();

	int offset;
	int bufferNum = buf.getBlockNum(this->indexname, 0);
	if (!this->findToLeaf(mykey, offset, bufferNum))
	{
		this->insertLeaf(mykey, blockNum, blockOffset, bufferNum, offset);
		int recordNum = getRecordNum(bufferNum);
		if (recordNum == treeIndex.degree)
		{
			this->adjustafterinsert(bufferNum);
		}
		return 1;
	}
	else
	{
		cout << "Error:in insert key to index: the duplicated key!" << endl;
		return 0;
	}
}

template <class KeyType>
bool IndexManager<KeyType>::insertLeaf(KeyType mykey, int blockNum, int blockOffset, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] != 'L')
	{
		cout << "Error:insertLeaf(const KeyType &mykey,const offset val) is a function for leaf nodes" << endl;
		return 0;
	}
	Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
	if (myleaf.recordNum == 0)
	{
		myleaf.key.clear();
		myleaf.key.push_back(mykey);
		myleaf.POS.clear();
		myleaf.POS.push_back(recordPosition(blockNum, blockOffset));
		myleaf.recordNum++;
		if (myleaf.isRoot)
			this->rootL = myleaf;
		myleaf.writeBack();
	}
	else
	{
		typename vector<KeyType>::iterator kit = myleaf.key.begin();
		vector<recordPosition>::iterator pit = myleaf.POS.begin();
		kit += offset;
		pit += offset;
		myleaf.key.insert(kit, mykey);
		myleaf.POS.insert(pit, recordPosition(blockNum, blockOffset));
		myleaf.recordNum++;
		if (myleaf.isRoot)
			this->rootL = myleaf;
		myleaf.writeBack();

		if (offset == 0)
		{
			int NUM;
			KeyType oldkey = myleaf.key[1];
			int parent = myleaf.father;
			while (parent >= 0 && parent <= (this->treeIndex.degree - 1) && buf.bufferBlock[bufferNum].blockOffset == OutGetPointer(NUM = buf.getBlockNum(this->indexname, parent), 11))
			{
				parent = OutGetPointer(NUM, 6);
				bufferNum = NUM;
			}
			if (parent >= 0 && parent <= (this->treeIndex.degree - 1))
			{
				int set;
				if (this->search(mykey, set, bufferNum))
					writeValue<KeyType>(bufferNum, 16 + set*(5 + this->treeIndex.columnLength), this->treeIndex.columnLength, mykey);
				else
					cout << "Error:in insert key to leaf: fail to update the ancestor node!" << endl;
			}

		}
		buf.writeBlock(bufferNum);
		if (getRecordNum(myleaf.bufferNum) >= (this->treeIndex.degree - 1))
			this->adjustafterinsert(myleaf.bufferNum);

	}

	return 1;
}

template <class KeyType>
bool IndexManager<KeyType>::insertLeaf(KeyType mykey, int blockNum, int blockOffset, int leafBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, leafBlock);
	int offset;
	if (search(mykey, offset, bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->insertLeaf<KeyType>(mykey, blockNum, blockOffset, bufferNum, offset);
	}
	return 1;
}

template <class KeyType>
bool IndexManager<KeyType>::insertBranch(KeyType mykey, int child, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] != '_')
	{
		cout << "Error:insertBranch(const KeyType &mykey,const offset val) is a function for Branch nodes" << endl;
		return 0;
	}

	Branch<KeyType> mybranch(bufferNum, this->treeIndex);
	if (mybranch.recordNum == 0)
	{
		mybranch.key.clear();
		mybranch.key.push_back(mykey);
		mybranch.child.push_back(child);
		mybranch.recordNum++;
		if (mybranch.isRoot)
			this->rootB = mybranch;
		mybranch.writeBack();
		return 1;
	}
	else
	{
		typename vector<KeyType>::iterator kit = mybranch.key.begin();
		vector<POINTER>::iterator cit = mybranch.child.begin();
		kit += offset;
		cit += offset;
		mybranch.key.insert(kit, mykey);
		mybranch.child.insert(cit, child);
		mybranch.recordNum++;
		if (mybranch.isRoot)
			this->rootB = mybranch;
		mybranch.writeBack();

		if (getRecordNum(mybranch.bufferNum) >= (this->treeIndex.degree - 1))
			this->adjustafterinsert(mybranch.bufferNum);
		return 1;
	}
}

template <class KeyType>
bool IndexManager<KeyType>::insertBranch(KeyType mykey, int child, int branchBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, branchBlock);
	int offset;
	if (search(mykey, offset, bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->insertBranch(mykey, child, bufferNum, offset);

	}
	return 1;
}

template <class KeyType>
void IndexManager<KeyType>::adjustafterinsert(int bufferNum)
{
	KeyType mykey;
	if (getRecordNum(bufferNum) <= (this->treeIndex.degree - 1))//record number is too great, need to split
		return;

	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		Leaf<KeyType> leaf(bufferNum, this->treeIndex);
		if (leaf.isRoot)
		{//this leaf is a root
			int fbufferNum = buf.addBlockInFile(this->treeIndex);	//find a new place for old leaf
			int sbufferNum = buf.addBlockInFile(this->treeIndex);	// buffer number for sibling 

			Branch<KeyType> branchRoot(fbufferNum);	//new root, which is branch indeed
			Leaf<KeyType> leafadd(sbufferNum);	//sibling

												//is root
			branchRoot.isRoot = 1;
			leafadd.isRoot = 0;
			leaf.isRoot = 0;

			branchRoot.father = -1;
			leafadd.father = leaf.father = buf.bufferBlock[fbufferNum].blockOffset;
			branchRoot.columnLength = leafadd.columnLength = leaf.columnLength;
			branchRoot.degree = leafadd.degree = leaf.degree;

			//link the newly added leaf block in the link list of leaf
			leafadd.lastleaf = buf.bufferBlock[leaf.bufferNum].blockOffset;
			leaf.lastleaf = -1;
			leafadd.nextleaf = -1;
			leaf.nextleaf = buf.bufferBlock[leafadd.bufferNum].blockOffset;
			branchRoot.child.push_back(buf.bufferBlock[leaf.bufferNum].blockOffset);
			branchRoot.child.push_back(buf.bufferBlock[leafadd.bufferNum].blockOffset);
			branchRoot.recordNum++;
			KeyType ktmp;
			recordPosition rtmp;
			while (leafadd.key.size() < leaf.key.size())
			{
				ktmp = leaf.key.back();
				rtmp = leaf.POS.back();
				leaf.key.pop_back();
				leaf.POS.pop_back();
				leafadd.key.insert(leafadd.key.begin(), ktmp);
				leafadd.POS.insert(leafadd.POS.begin(), rtmp);
				leaf.recordNum--;
				leafadd.recordNum++;
			}
			branchRoot.key.push_back(ktmp);

			this->rootB = branchRoot;
			this->isLeaf = 0;
			buf.bufferBlock[leaf.bufferNum].blockOffset = buf.bufferBlock[this->rootB.bufferNum].blockOffset;
			buf.bufferBlock[this->rootB.bufferNum].blockOffset = 0;
			this->rootB.writeBack();
			leaf.writeBack();
			leafadd.writeBack();
			this->treeIndex.blockNum += 2;
		}
		else
		{//this leaf is not a root, we have to cascade up
			int sbufferNum = buf.addBlockInFile(this->treeIndex);
			Leaf<KeyType> leafadd(sbufferNum);

			leafadd.isRoot = 0;

			leafadd.father = leaf.father;
			leafadd.columnLength = leaf.columnLength;
			leafadd.degree = leaf.degree;

			//link the newly added leaf block in the link list of leaf
			leafadd.lastleaf = buf.bufferBlock[leaf.bufferNum].blockOffset;
			leafadd.nextleaf = leaf.nextleaf;
			leaf.nextleaf = buf.bufferBlock[leafadd.bufferNum].blockOffset;

			KeyType ktmp;
			recordPosition rtmp;
			while (leafadd.key.size() < leaf.key.size())
			{
				ktmp = leaf.key.back();
				rtmp = leaf.POS.back();
				leaf.key.pop_back();
				leaf.POS.pop_back();
				leafadd.key.insert(leafadd.key.begin(), ktmp);
				leafadd.POS.insert(leafadd.POS.begin(), rtmp);
				leaf.recordNum--;
				leafadd.recordNum++;
			}
			leaf.writeBack();
			leafadd.writeBack();

			this->insertBranch(ktmp, buf.bufferBlock[leafadd.bufferNum].blockOffset, leaf.father);
			this->treeIndex.blockNum++;
		}
	}
	else//需要调整的节点是个branch
	{
		Branch<KeyType> branch(bufferNum, this->treeIndex);
		if (branch.isRoot)//this branch is a root
		{
			int fbufferNum = buf.addBlockInFile(this->treeIndex);	//find a new place for old branch
			int sbufferNum = buf.addBlockInFile(this->treeIndex);	// buffer number for sibling 

			Branch<KeyType> branchRoot(fbufferNum);	//new root, which is branch indeed
			Branch<KeyType> branchadd(sbufferNum);	//sibling

													//is root
			branchRoot.isRoot = 1;
			branchadd.isRoot = 0;
			branch.isRoot = 0;

			branchRoot.father = -1;
			branchadd.father = branch.father = buf.bufferBlock[fbufferNum].blockOffset;
			branchRoot.columnLength = branchadd.columnLength = branch.columnLength;
			branchRoot.degree = branchadd.degree = branch.degree;

			//link the newly added branch block in the link list of branch
			branchRoot.child.push_back(buf.bufferBlock[branch.bufferNum].blockOffset);
			branchRoot.child.push_back(buf.bufferBlock[branchadd.bufferNum].blockOffset);
			branchRoot.recordNum++;
			KeyType ktmp;
			POINTER ctmp;
			while (branchadd.key.size() < branch.key.size())
			{
				ktmp = branch.key.back();
				ctmp = branch.child.back();
				branch.key.pop_back();
				branch.child.pop_back();
				branchadd.key.insert(branchadd.key.begin(), ktmp);
				branchadd.child.insert(branchadd.child.begin(), ctmp);
				branch.recordNum--;
				branchadd.recordNum++;
			}
			branchadd.recordNum--;
			branchadd.key.erase(branchadd.key.begin());
			branchRoot.key.push_back(ktmp);

			this->rootB = branchRoot;
			this->isLeaf = 0;
			buf.bufferBlock[branch.bufferNum].blockOffset = buf.bufferBlock[this->rootB.bufferNum].blockOffset;
			buf.bufferBlock[this->rootB.bufferNum].blockOffset = 0;
			this->rootB.writeBack();
			branch.writeBack();
			branchadd.writeBack();
			this->treeIndex.blockNum += 2;
		}
		else//this branch is not a root, we have to cascade up
		{
			int sbufferNum = buf.addBlockInFile(this->treeIndex);
			Branch<KeyType> branchadd(sbufferNum);

			branchadd.isRoot = 0;

			branchadd.father = branch.father;
			branchadd.columnLength = branch.columnLength;
			branchadd.degree = branch.degree;

			//link the newly added branch block in the link list of branch

			KeyType ktmp;
			POINTER ctmp;
			while (branchadd.key.size() < branch.key.size())
			{
				ktmp = branch.key.back();
				ctmp = branch.child.back();
				branch.key.pop_back();
				branch.child.pop_back();
				branchadd.key.insert(branchadd.key.begin(), ktmp);
				branchadd.child.insert(branchadd.child.begin(), ctmp);
				branch.recordNum--;
				branchadd.recordNum++;
			}
			branchadd.recordNum--;
			branchadd.key.erase(branchadd.key.begin());
			branch.writeBack();
			branchadd.writeBack();

			this->insertBranch(ktmp, buf.bufferBlock[branchadd.bufferNum].blockOffset, branch.father);
			this->treeIndex.blockNum++;
		}
	}

}

template <class KeyType>
void IndexManager<KeyType>::creatIndex()
{
	InitializeRoot();

	//retrieve datas of the table and form a B+ Tree
	string tablename = this->treeTable.tableName + ".table";
	string stringrow;
	KeyType mykey;

	int length = this->treeTable.totalLength + 1;//table中的record会加多一位来判断这条记录是否被删除了,而index中的record不会
	const int recordNum = BLOCKSIZE / length;

	//read datas from the record and sort it into a B+ Tree and store it
	for (int blockOffset = 0; blockOffset < this->treeTable.blockNum; blockOffset++) {
		int bufferNum = buf.getBlockNum(tablename, blockOffset);
		for (int offset = 0; offset < recordNum; offset++) {
			int position = offset * length;
			stringrow = buf.bufferBlock[bufferNum].getvalue(position, position + length);
			if (stringrow.c_str()[0] == EMPTY) continue;//inticate that this row of record have been deleted
			stringrow.erase(stringrow.begin());	//把第一位去掉//现在stringrow代表一条记录
			if (this->treeIndex.column == 0)
				mykey = getTableValue<KeyType>(bufferNum, 1 + position + getKeyPosition(), this->treeIndex.columnLength);
			else
				mykey = getTableValue<KeyType>(bufferNum, position + getKeyPosition(), this->treeIndex.columnLength);
			insertIndex(mykey, blockOffset, offset);
		}
	}
}

template <class KeyType>
int IndexManager<KeyType>::getKeyPosition()//从记录row中提取key
{
	int s_pos = 0, f_pos = 0;	//start position & finish position
	for (int i = 0; i <= this->treeIndex.column; i++) {
		s_pos = f_pos;
		f_pos += this->treeTable.attribute[i].length;
	}
	return s_pos;
}



template <class KeyType>
bool IndexManager<KeyType>::deleteIndex(KeyType mykey)
{
	if (this->rootB.key.empty() && this->rootL.key.empty())
	{
		cout << "ERROR: In deleteKey, no nodes in the tree " << this->indexname << "!" << endl;
		return 0;
	}
	int offset;
	int bufferNum;
	if (this->isLeaf)
		bufferNum = this->rootL.bufferNum;
	else
		bufferNum = this->rootB.bufferNum;

	if (!findToLeaf(mykey, int& offset, int& bufferNum))
	{
		cout << "ERROR: In deleteKey, no key in the tree " << this->indexname << "!" << endl;
		return 0;
	}

	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		this->deleteLeaf(mykey, bufferNum, offset);
		return 1;
	}
	else
	{
		this->deleteBranch(mykey, bufferNum, offset);
		return 1;
	}
}

template <class KeyType>
bool IndexManager<KeyType>::deleteBranch(KeyType mykey, int branchBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, branchBlock);
	int offset;
	if (search(mykey, offset, bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->deleteBranch<KeyType>(mykey, bufferNum, offset);
	}
	return 1;
}

template <class KeyType>
bool IndexManager<KeyType>::deleteBranch(KeyType mykey, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		cout << "Error:insertBranch(const KeyType &mykey,const offset val) is a function for branch nodes" << endl;
		return 0;
	}

	Branch<KeyType> mybranch(bufferNum, this->treeIndex);
	if (mybranch.key.size()<(offset + 1) || mybranch.key[offset] != mykey)
	{
		cout << "Error:In remove(size_t index), can not find the key!" << endl;
		return 0;
	}

	typename vector<KeyType>::iterator kit = mybranch.key.begin();
	vector<POINTER>::iterator cit = mybranch.child.begin();
	kit += offset;
	cit += offset;
	mybranch.key.erase(kit);
	mybranch.child.erase(cit);
	mybranch.recordNum--;
	mybranch.writeBack();
	if (getRecordNum(mybranch.bufferNum) >= (this->treeIndex.degree - 1))
		this->adjustafterdelete(mybranch.bufferNum);
}

template <class KeyType>
bool IndexManager<KeyType>::deleteLeaf(KeyType mykey, int leafBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, leafBlock);
	int offset;
	if (search(mykey, offset, bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->deleteLeaf<KeyType>(mykey, bufferNum, offset);
	}
	return 1;
}

template <class KeyType>
void IndexManager<KeyType>::deleteNode(int bufferNum)
{
	int lastblock = buf.getBlockNum(this->indexname, this->treeIndex.blockNum - 1);
	if (lastblock == bufferNum)
	{
		this->treeIndex.blockNum--;
		return;
	}

	if (buf.bufferBlock[bufferNum].value[1] == 'L')//这是个叶子
	{

		if (buf.bufferBlock[lastblock].value[1] == 'L')
		{
			if (buf.bufferBlock[lastblock].value[0] == 'R')
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			else
			{
				int offset;
				int index;
				Leaf<KeyType> lastleaf(lastblock, this->treeIndex);
				int fbufferNum = buf.getBlockNum(this->indexname, lastleaf.father);
				Branch<KeyType> parent(fbufferNum, this->treeIndex);
				parent.search(lastleaf.key[0], offset);
				if (offset == 0 && parent.key[0] > lastleaf.key[0])
					index = 0;
				else
					index = offset + 1;
				parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
				parent.writeBack();
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
				lastleaf.writeBack();
				buf.bufferBlock[bufferNum].initialize();
				this->treeIndex.blockNum--;
			}
		}
		else
		{
			int offset;
			int index;
			Branch<KeyType> lastbranch(lastblock, this->treeIndex);
			int fbufferNum = buf.getBlockNum(this->indexname, lastbranch.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			parent.search(lastbranch.key[0], offset);
			index = offset;
			parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
			parent.writeBack();
			vector<int>::iterator it = lastbranch.child.begin();
			for (; it != lastbranch.child.end(); it++)
			{
				int cbufferNum = buf.getBlockNum(this->indexname, *it);
				OutSetPointer(cbufferNum, 6, buf.bufferBlock[bufferNum].blockOffset);
			}

			buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			lastbranch.writeBack();
			buf.bufferBlock[bufferNum].initialize();
			this->treeIndex.blockNum--;
		}

	}
	else//it's a branch
	{

		if (buf.bufferBlock[lastblock].value[1] == 'L')
		{
			if (buf.bufferBlock[lastblock].value[0] == 'R')
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			else
			{
				int offset;
				int index;
				Leaf<KeyType> lastleaf(lastblock, this->treeIndex);
				int fbufferNum = buf.getBlockNum(this->indexname, lastleaf.father);
				Branch<KeyType> parent(fbufferNum, this->treeIndex);
				parent.search(lastleaf.key[0], offset);
				if (offset == 0 && parent.key[0] > lastleaf.key[0])
					index = 0;
				else
					index = offset + 1;
				parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
				parent.writeBack();
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
				lastleaf.writeBack();
				buf.bufferBlock[bufferNum].initialize();
				this->treeIndex.blockNum--;
			}
		}
		else
		{
			int offset;
			int index;
			Branch<KeyType> lastbranch(lastblock, this->treeIndex);
			int fbufferNum = buf.getBlockNum(this->indexname, lastbranch.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			parent.search(lastbranch.key[0], offset);
			index = offset;
			parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
			parent.writeBack();
			vector<int>::iterator it = lastbranch.child.begin();
			for (; it != lastbranch.child.end(); it++)
			{
				int cbufferNum = buf.getBlockNum(this->indexname, *it);
				OutSetPointer(cbufferNum, 6, buf.bufferBlock[bufferNum].blockOffset);
			}

			buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			lastbranch.writeBack();
			buf.bufferBlock[bufferNum].initialize();
			this->treeIndex.blockNum--;
		}
	}
}

template <class KeyType>
bool IndexManager<KeyType>::deleteLeaf(KeyType mykey, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] != 'L')
	{
		cout << "Error:insertLeaf(const KeyType &mykey,const offset val) is a function for leaf nodes" << endl;
		return 0;
	}

	Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
	if (myleaf.key.size()<(offset + 1) || myleaf.key[offset] != mykey)
	{
		cout << "Error:In remove(size_t index), can not find the key!" << endl;
		return 0;
	}

	typename vector<KeyType>::iterator kit = myleaf.key.begin();
	vector<recordPosition>::iterator rit = myleaf.POS.begin();
	kit += offset;
	rit += offset;
	myleaf.key.erase(kit);
	myleaf.POS.erase(rit);
	myleaf.recordNum--;
	myleaf.writeBack();
	if (offset == 0)
	{
		int NUM;
		KeyType newkey = myleaf.key[0];
		int parent = myleaf.father;
		while (parent >= 0 && parent <= (this->treeIndex.degree - 1) && buf.bufferBlock[bufferNum].blockOffset == OutGetPointer(NUM = buf.getBlockNum(this->indexname, parent), 11))
		{
			parent = OutGetPointer(NUM, 6);
			bufferNum = NUM;
		}
		if (parent >= 0 && parent <= (this->treeIndex.degree - 1))
		{
			int set;
			if (this->search(mykey, set, bufferNum))
				writeValue<KeyType>(bufferNum, 16 + set*(5 + this->treeIndex.columnLength), this->treeIndex.columnLength, mykey);
			else
				cout << "Error:in insert key to leaf: fail to update the ancestor node!" << endl;
		}
	}
	buf.bufferBlock[bufferNum].Written();
	if (getRecordNum(myleaf.bufferNum) >= (this->treeIndex.degree - 1))
		this->adjustafterdelete(myleaf.bufferNum);
}

template <class KeyType>
void IndexManager<KeyType>::adjustafterdelete(int bufferNum)//需要维护：blockNum;leafhead;
{
	int min_leaf = this->treeIndex.degree / 2;
	int min_branch = (this->treeIndex.degree - 1) / 2;
	if ((buf.bufferBlock[bufferNum].value[1] == 'L' && getRecordNum(bufferNum) >= min_leaf) || (buf.bufferBlock[bufferNum].value[1] != 'L' && getRecordNum(bufferNum) >= min_branch))
		return 1;

	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
		if (myleaf.isRoot)
		{
			if (myleaf.recordNum > 0)
				return 1;
			else
			{
				this->treeIndex.blockNum = 0;
				this->InitializeRoot();
				return 1;
			}
		}
		else
		{
			int offset;
			int fbufferNum = buf.getBlockNum(this->indexname, myleaf.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			parent.search(myleaf.key[0], offset);
			if (offset != 0 && (offset == parent.recordNum - 1))//choose the left brother to merge or replace
			{
				int sbufferNum = buf.getBlockNum(this->indexname, parent.child[offset]);
				Leaf<KeyType> brother(sbufferNum, this->treeIndex);
				if (brother.recordNum > min_leaf)// choose the most right key of brother to add to the left hand of the pnode
				{
					myleaf.key.insert(myleaf.key.begin(), brother.key[brother.recordNum - 1]);

					myleaf.POS.insert(myleaf.POS.begin(), brother.POS[brother.recordNum - 1]);
					myleaf.recordNum++;
					brother.recordNum--;
					parent.key[offset] = myleaf.key[0];
					brother.key.pop_back();
					brother.POS.pop_back();
					myleaf.writeBack();
					brother.writeBack();
					parent.writeBack();
				}
				else// merge the node with its left brother
				{
					parent.writeBack();
					this->deleteBranch(myleaf.key[0], parent.bufferNum, parent.recordNum - 1);

					while (!myleaf.key.empty())
					{
						brother.key.push_back(myleaf.key[0]);
						brother.POS.push_back(myleaf.POS[0]);
						myleaf.key.erase(myleaf.key.begin());
						myleaf.POS.erase(myleaf.POS.begin());
						brother.recordNum++;
						myleaf.recordNum--;

					}
					brother.nextleaf = myleaf.nextleaf;

					brother.writeBack();
					myleaf.writeBack();

					this->deleteNode(myleaf.bufferNum);
					this->adjustafterdelete(parent.bufferNum);
					return 1;
				}

			}
			else//choose the right brother to merge or replace
			{
				int sbufferNum;
				int index;
				if (parent.key[0] > myleaf.key[0])
				{
					index = 1;
				}
				else
				{
					index = offset + 2;
				}
				sbufferNum = buf.getBlockNum(this->indexname, parent.child[index]);
				Leaf<KeyType> brother(sbufferNum, this->treeIndex);

				if (brother.recordNum > min_leaf)//// choose the most left key of right brother to add to the right hand of the node
				{
					KeyType mykey = brother.key[0];
					recordPosition myPOS = brother.POS[0];
					int RECORDNUM = myleaf.recordNum;
					this->deleteLeaf(mykey, sbufferNum, 0);
					this->insertLeaf(mykey, myPOS.blockNum, myPOS.blockPosition, sbufferNum, RECORDNUM);

				}// end add
				else // merge the node with its right brother
				{
					parent.writeBack();
					this->deleteBranch(brother.key[0], parent.bufferNum, index - 1);
					while (!brother.key.empty())
					{
						myleaf.key.push_back(brother.key[0]);
						myleaf.POS.push_back(brother.POS[0]);
						brother.key.erase(brother.key.begin());
						brother.POS.erase(brother.POS.begin());
						brother.recordNum--;
						myleaf.recordNum++;

					}
					myleaf.nextleaf = brother.nextleaf;

					brother.writeBack();
					myleaf.writeBack();
					this->deleteNode(brother.bufferNum);
					this->adjustafterdelete(parent.bufferNum);
					return 1;
				}
			}

		}
	}
	else
	{
		Branch<KeyType> mybranch(bufferNum, this->treeIndex);
		if (mybranch.isRoot)
		{
			if (mybranch.recordNum > 0)
				return 1;
			else
			{
				int cbufferNum = buf.getBlockNum(this->indexname, mybranch.child[0]);
				if (buf.bufferBlock[cbufferNum].value[1] == 'L')
				{
					Leaf<KeyType> childleaf(cbufferNum, this->treeIndex);
					buf.bufferBlock[mybranch.bufferNum].blockOffset = buf.bufferBlock[cbufferNum].blockOffset;
					buf.bufferBlock[cbufferNum].blockOffset = 0;
					Branch<KeyType> childbranch(cbufferNum, this->treeIndex);
					childbranch.father = -1;
					childbranch.isRoot = 1;
					this->rootB = childbranch;
					this->isLeaf = 1;

					childbranch.writeBack();
					mybranch.isRoot = 0;
					mybranch.writeBack();
					this->deleteNode(mybranch.bufferNum);
				}
				else
				{
					buf.bufferBlock[mybranch.bufferNum].blockOffset = buf.bufferBlock[cbufferNum].blockOffset;
					buf.bufferBlock[cbufferNum].blockOffset = 0;
					Branch<KeyType> childbranch(cbufferNum, this->treeIndex);
					childbranch.father = -1;
					childbranch.isRoot = 1;
					vector<int>::iterator it = childbranch.child.begin();
					for (; it != childbranch.child.end(); it++)
					{
						int ccbufferNum = buf.getBlockNum(this->indexname, *it);
						OutSetPointer(ccbufferNum, 6, buf.bufferBlock[cbufferNum].blockOffset);
					}
					this->rootB = childbranch;
					childbranch.writeBack();
					mybranch.isRoot = 0;
					mybranch.writeBack();
					this->deleteNode(mybranch.bufferNum);
				}
			}
		}
		else//调整非根节点的branch节点
		{
			size_t index = 0;
			if (pnode->count == 0)
			{
				for (index = 0; index <= pnode->count; index++)
				{
					if (pnode->parent->child[index] == pnode)
						break;
				}
			}
			else
			{
				par->search(pnode->key[0], index);
			}

			int offset = 0;
			int fbufferNum = buf.getBlockNum(this->indexname, mybranch.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			if (mybranch.recordNum == 0)
			{
				for (offset = 0; offset <= parent.recordNum; offset++)
				{
					if (parent.child[offset] == buf.bufferBlock[mybranch.bufferNum].blockOffset)
						break;
				}
			}
			else
			{
				parent.search(mybranch.key[0], offset);
			}

			if (offset != 0 && (offset == parent.recordNum))//choose the left brother to merge or replace
			{
				int sbufferNum = buf.getBlockNum(this->indexname, parent.child[offset - 1]);
				Branch<KeyType> brother(sbufferNum, this->treeIndex);
				if (brother.recordNum > min_branch)// choose the most right key of brother to add to the left hand of the pnode
				{
					mybranch.key.insert(mybranch.key.begin(), parent.key[offset - 1]);
					int childblock = brother.child[brother.recordNum];
					mybranch.child.insert(mybranch.child.begin(), brother.child[brother.recordNum]);
					mybranch.recordNum++;
					brother.recordNum--;
					parent.key[offset - 1] = brother.key[brother.recordNum - 1];
					brother.key.pop_back();
					brother.child.pop_back();
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);

					mybranch.writeBack();
					brother.writeBack();
					parent.writeBack();
				}
				else// merge the node with its left brother
				{
					parent.writeBack();
					KeyType parentKey = parent.key[parent.recordNum - 1];
					this->deleteBranch(parentKey, parent.bufferNum, parent.recordNum - 1);

					brother.key.push_back(parentKey);
					int childblock = mybranch.child[0];
					brother.child.push_back(mybranch.child[0]);
					mybranch.child.erase(mybranch.child.begin());
					brother.recordNum++;
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[brother.bufferNum].blockOffset);
					while (!mybranch.key.empty())
					{
						brother.key.push_back(mybranch.key[0]);
						brother.child.push_back(mybranch.child[0]);
						childblock = mybranch.child[0];
						mybranch.key.erase(mybranch.key.begin());
						mybranch.child.erase(mybranch.child.begin());

						cbufferNum = buf.getBlockNum(this->indexname, childblock);
						OutSetPointer(cbufferNum, 6, buf.bufferBlock[brother.bufferNum].blockOffset);
						brother.recordNum++;
						mybranch.recordNum--;

					}

					brother.writeBack();
					mybranch.writeBack();

					this->deleteNode(mybranch.bufferNum);
					return 1;
				}

			}
			else//choose the right brother to merge or replace
			{

				int sbufferNum = buf.getBlockNum(this->indexname, parent.child[offset + 1]);
				Branch<KeyType> brother(sbufferNum, this->treeIndex);

				if (brother.recordNum > min_branch)//// choose the most left key of right brother to add to the right hand of the node
				{
					mybranch.key.push_back(parent.key[offset]);
					int childblock = brother.child[0];
					mybranch.child.push_back(brother.child[0]);
					mybranch.recordNum++;
					brother.recordNum--;
					parent.key[offset] = brother.key[0];
					brother.key.erase(brother.key.begin());
					brother.child.erase(brother.key.begin());
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);

					mybranch.writeBack();
					brother.writeBack();
					parent.writeBack();

				}// end add
				else // merge the node with its right brother
				{
					parent.writeBack();
					KeyType parentKey = parent.key[offset];
					this->deleteBranch(parentKey, parent.bufferNum, offset);

					mybranch.key.push_back(parentKey);
					int childblock = brother.child[0];
					mybranch.child.push_back(brother.child[0]);
					brother.child.erase(brother.child.begin());
					mybranch.recordNum++;
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);

					while (!brother.key.empty())
					{
						mybranch.key.push_back(brother.key[0]);
						mybranch.child.push_back(brother.child[0]);
						childblock = brother.child[0];
						brother.key.erase(brother.key.begin());
						brother.child.erase(brother.child.begin());

						cbufferNum = buf.getBlockNum(this->indexname, childblock);
						OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);
						brother.recordNum--;
						mybranch.recordNum++;

					}

					brother.writeBack();
					mybranch.writeBack();

					this->deleteNode(brother.bufferNum);
					return 1;
				}
			}
		}
	}


}


template <class KeyType>
Data IndexManager<KeyType>::selectEqual(KeyType mykey)
{
	int offset;
	int bufferNum = 0;
	recordPosition res(-1, 0);
	if (this->findToLeaf(mykey, offset, bufferNum))
	{
		Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
		res.blockNum = myleaf.POS[offset].blockNum;
		res.blockPosition = myleaf.POS[offset].blockPosition;
		//return res;
	}
	else
		//return res;

		return select(this->treeTable, res);
}

template <class KeyType>
Data IndexManager<KeyType>::selectBetween(KeyType mykey1, KeyType mykey2)//大于等于mykey1,小于mykey2
{
	int offset;
	int bufferNum = 0;
	vector<recordPosition> RESULT;

	this->findToLeaf<KeyType>(mykey1, offset, bufferNum);
	Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
	while ((offset + 1) <= myleaf.recordNum && myleaf.key[offset]<mykey2)
	{
		recordPosition res;
		res.blockNum = myleaf.POS[offset].blockNum;
		res.blockPosition = myleaf.POS[offset].blockPosition;
		RESULT.push_back(res);
		offset++;
	}
	int nextL = myleaf.nextleaf;
	while (nextL != -1)
	{
		bufferNum = buf.getBlockNum(this->indexname, nextL);
		offset = 0;
		Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
		while ((offset + 1) <= myleaf.recordNum && myleaf.key[offset]<mykey2)
		{
			recordPosition res;
			res.blockNum = myleaf.POS[offset].blockNum;
			res.blockPosition = myleaf.POS[offset].blockPosition;
			RESULT.push_back(res);
			offset++;
		}
		if (myleaf.key[offset] < mykey2)
			nextL = myleaf.nextleaf;
		else
			nextL = -1;


	}
	return select(this->treeTable, RESULT);
}









#endif
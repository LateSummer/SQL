#include"BufferManager.h"
#include"IndexManager.h"
#include"recordManager.h"
#include"Interpreter.h"
#include"API.h"
#include"Macro.h"
#include"SQL.h"
using namespace std;

CatalogManager Catalog;
BufferManager buf;
Interpret ParseTree;
RecordManager record;

bool cmdEnd(char *s)
{
	int len = strlen(s);
	for (int i = 0; i < len; i++)
		if (s[i] == ';') {
			s[i] = 0;
			return true;
		}
	return false;
}

int main()
{
	int isQuit = 0, isCmdEnd, tot_len;
	char input[1024], cmd[1024];
	//freopen("input.txt", "r", stdin);
	cout << "Welcome to our miniSQL!" << endl;
	while (!isQuit)
	{
		cout << "miniSQL>>";
		isCmdEnd = 0;
		cmd[0] = 0;
		tot_len = 0;
		while (!isCmdEnd){
			cin.getline(input, 1023, '\n');
			tot_len += strlen(input);
			if (tot_len >= 1024){
				cout << "ERROR! The max input length is 1023!" << endl;
				continue;
			}
			if (cmdEnd(input)) isCmdEnd = 1;
			if (same(input, "quit")) isCmdEnd = 1, isQuit = 1;
			strcat(cmd, input);
		}
		ParseTree.Parse(cmd);
		Execute();
		ParseTree.makeInitilate();
	}
	return 0;
}
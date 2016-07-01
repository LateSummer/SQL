#include"API.h"
#include"BufferManager.h"
#include"IndexManager.h"
#include"recordManager.h"
#include"Interpreter.h"
#include"Macro.h"
#include"SQL.h"
using namespace std;

CatalogManager Catalog;
BufferManager buf;
Interpret ParseTree;
RecordManager record;
int fp;
fstream fin;

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
		if (!fp) {
			cout << "miniSQL>>";
			isCmdEnd = 0;
			cmd[0] = 0;
			tot_len = 0;
			while (!isCmdEnd) {
				cin.getline(input, 1023, '\n');
				tot_len += strlen(input);
				if (tot_len >= 1024) {
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
		else {
			isCmdEnd = 0;
			cmd[0] = 0;
			tot_len = 0;
			if (fin.eof()) {
				fp = 0;
				freopen("CON", "w", stdout);
				cout << "OK!" << endl;
				continue;
			}
			while (!isCmdEnd) {
				if (fin.eof()) {
					freopen("CON", "w", stdout);
					fp = 0;
					cout << "OK!" << endl;
					break;
				}
				fin.getline(input, 1023, '\n');
				tot_len += strlen(input);
				if (tot_len >= 1024) {
					cout << "ERROR! The max input length is 1023!" << endl;
					continue;
				}
				if (cmdEnd(input)) isCmdEnd = 1;
				if (same(input, "quit")) isCmdEnd = 1, isQuit = 1;
				strcat(cmd, input);
			}
			if (!fp) continue;
			ParseTree.Parse(cmd);
			Execute();
			ParseTree.makeInitilate();
		}
	}
	return 0;
}
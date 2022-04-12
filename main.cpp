#include "stdinc.h"

using namespace std;

SimpleTimer Timer;
LoggerSrv Logger;
DebugState DbgSt;
Consola* pCons;
#include <windows.h>

bool current_working_directory(string & wd)
{
	char working_directory[MAX_PATH + 1];
	GetCurrentDirectoryA(sizeof(working_directory), working_directory); // **** win32 specific ****
	wd = working_directory;
	return true;
}


void ConsoleRun ()
{
	string wd;
	current_working_directory(wd);
	cout << "CWD " << wd << endl;

	pCons = new Consola();
	pCons->pushCons();

	int cnt = 0;
	while (pCons->getCmd()) {
		cnt++;
	}
}

void TestBSet()
{
	BitSet* pBSet = new BitSet(10000);
	pBSet->Test(4);
	pBSet->Test(15);
	pBSet->Test(40);
	pBSet->Test(60);
}

int main(int argc, char** argv)
{
	//TestBSet();
	ConsoleRun();
}


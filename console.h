#pragma once

#include <chrono>
#include <assert.h>

#include <iostream>
#include <stack>
#include <conio.h>
#include <sstream>      // std::stringstream
#include <fstream>
#include "logger.h"
#include <unordered_map>

class SymType {
public:
	typedef enum {
		TYPEMPTY = 0,
		TYPOBJ,
		TYPSPM,
		TYPDEVMUL,
		TYPDEVSPM,
		TYPVAL, 
		TYPSENV,
		TYPNENV
	} kind;

	SymType() : t(TYPEMPTY) {}
	SymType(kind _t) : t(_t) {}

	kind t;

	const char* Name() {
		switch (t) {
		case TYPEMPTY: return "EMPTY";
		case TYPOBJ: return "OBJ";
		case TYPSPM: return "SPM";
		case TYPDEVMUL: return "DEVMUL";
		case TYPDEVSPM: return "DEVSPM";
		case TYPVAL: return "VAL";
		case TYPSENV: return "SENV";
		}
		return "INVALID TYPE";
	}
};

class Symbol {
public:
	Symbol() : typ(SymType()), pObj(0), iVal(0) {}
	Symbol(SymType _typ, void* _pObj, int _iVal) : 
		typ(_typ), pObj(_pObj), iVal(_iVal) {}

	SymType typ;
	void* pObj;
	int iVal;

	void Dump(ostream& out) {
		out << typ.Name() << ", 0x" << hex << pObj << ", " << dec << iVal;
	}
};

class SymbolTable : public LogClient {
public:
	SymbolTable() : pSym(0) {}

	std::unordered_map<std::string, Symbol*> Map;
	std::unordered_map<std::string, Symbol*>::const_iterator itEnt;

	Symbol* pSym;

	bool Add(string id, SymType typ, void* pObj, int iVal) {
		pSym = new Symbol(typ, pObj, iVal);
		Map[id] = pSym;
		return true;
	}

	Symbol* Find(string id, bool mustExist = false) {
		itEnt = Map.find(id);
		if (itEnt == Map.end()) {
			if (mustExist)
				cerr << "Object " << id << " not found" << endl;
			return 0;
		}
		else
			return itEnt->second;
	}

	void* FetchObj(string id, SymType typ) {
		pSym = Find(id, true);
		if (pSym != 0) {
			if (pSym->typ.t != typ.t) {
				cerr << "Object " << id << " is not " << typ.Name() << endl;
				return 0;
			}
			return pSym->pObj;
		}
		else {
			return 0;
		}
	}

	virtual void Dump(ostream& out, int dumpType) {
		out << endl << "Symbol Table" << endl;

		for (auto& ent : Map) {
			out << "[" << ent.first << "] ";
			ent.second->Dump(out);
			out << endl;
		}
		out << endl;
	}
};

using namespace std;

// Input into consola
class InputState {
public:
	InputState() {}
	InputState(istream* _pIn, string& _prompt, string& _name)
		: pIn(_pIn), prompt(_prompt), name(_name)
	{}

	istream* pIn;
	string prompt;
	string name;
	bool isBottom;

	void set(istream* _pIn, const char* _prompt,
		const char* _name, bool _isBottom);

	void set(istream* _pIn, string _prompt,
		string _name, bool _isBottom);

	// Detect user interruption
	bool request();
};

class Consola : public LogClient {
public:
	Consola() 
	{
		setAtBottom();
		workdir = "../common/";
		scriptdir = workdir + "scripts/";
		datadir = workdir + "data/";
		scriptDefault = "exec.txt";
	}

	string workdir;
	string scriptdir;
	string datadir;
	string scriptDefault;

	void setup(const char* setConsFile)
	{
		if (setConsFile != nullptr) {
			pushInput(setConsFile);
		}
	}

	// Input management
	InputState inputSt;
	stack<class InputState> InStk;
	ifstream *pFS;

	void setAtBottom();
	void popInput();
	void pushInput(const char* fName);
	// Push in a new console
	void pushCons();

	// Process file execution
	void procExec();
	// Process end of execution from a command stream
	bool procEnd();

	void outPrompt() { cout << inputSt.prompt;  }

	// Symbols
	SymbolTable SymTab;
	Symbol* pSym;

	// Input and processing of a command line
	static const int lineMax = 100;
	char lineBuf[lineMax];
	bool lineEof;
	static const int tokenMax = 10;
	string Tokens[tokenMax];
	int tokenCnt;

	int tokToInt(string& tok);
	float tokToFloat(string& tok);

	// Input line and tokenize
	bool getCmdLine();

	// Echo line on console output and logger
	void showLine();
	void logLine();

	// Keep track of command nesting
	class NestState {
	public:
		bool exec;
		enum Mode : u8 { NORMAL, TOELSE, TOFI } mode;

		NestState() : exec(true), mode(NORMAL) {}
		NestState(bool ex, Mode m) : exec(ex), mode(m) {}
	};
	NestState dpst;
	stack <NestState> DSt;

	// Keep track of IF / FI / ELSE depth
	bool processDepth();

	// Top level dispatcher of user commands
	const char commentChar = '#';
	bool getCmd();
	bool processCmd();

	// Command Processing
	bool ProcessRead(string& id, string& fname, string &dim, string& t);
	bool ProcessDump(string& id, string& kind, 
		string& p0, string& p1, string& p2, string& p3);
	bool ProcessFill(string& id);
	bool ProcessRegen(string& id);
	bool ProcessSort(string& id, string& par);
	bool ProcessLink(string& id);
	bool ProcessCount(string& id, string& lfrom, string& lto, string& ifrom, string& ito);
	bool ProcessSearch(string& id);
	bool ProcessDebug(string& par0, string &par1, string& par2);
	bool ProcessReadRaw(string& id, string& fname, string &sortMode);
	bool ProcessWriteRaw(string& id, string& fname);
	bool ProcessUniqueRmv(string& id, string& fname);

	// Timing
	long long ms;
	void ReportTime();

	void* FindObj(string& id) {
		Symbol* pSym = SymTab.Find(id);
		if (pSym != 0)
			return pSym->pObj;
		else
			return 0;
	}

};

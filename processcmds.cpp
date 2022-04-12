//
//  Command console: command dispatching
//

#include "stdinc.h"
#include "senv.h"
#include "nenv.h"
using namespace std;

bool Consola::processCmd()
{
	bool retVal = true;

	Timer.Start();

	if (Tokens[0] == "set") {
		SymTab.Add(Tokens[1], SymType(SymType::TYPVAL), 0, tokToInt(Tokens[2]));
	}
	else if (Tokens[0] == "get") {
		pSym = SymTab.Find(Tokens[1]);
		if (pSym) {
			cout << "GET " << Tokens[1] << " ";
			pSym->Dump(cout);
			cout << endl;
		}
	}
	else if (Tokens[0] == "logopen") {
		Logger.StrOpen(Tokens[1]);
	}
	else if (Tokens[0] == "logclose") {
		Logger.StrClose(Tokens[1]);
	}
	else if (Tokens[0] == "dump") {
		if (tokenCnt <= 1)
			Tokens[1] = "";
		if (tokenCnt <= 2)
			Tokens[2] = "";
		ProcessDump(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5], Tokens[6]);
	}
	else if (Tokens[0] == "read") {
		ProcessRead(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "readraw") {
		ProcessReadRaw(Tokens[1], Tokens[2], Tokens[3]);
	}
	else if (Tokens[0] == "writeraw") {
		ProcessWriteRaw(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "uniquermv") {
		ProcessUniqueRmv(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "count") {
		ProcessCount(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5]);
	}
	else if (Tokens[0] == "fill") {
		ProcessFill(Tokens[1]);
	}
	else if (Tokens[0] == "sort") {
		ProcessSort(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "link") {
		ProcessLink(Tokens[1]);
	}
	else if (Tokens[0] == "search") {
		ProcessSearch(Tokens[1], Tokens[2], Tokens[3]);
	}
	else if (Tokens[0] == "regen") {
		ProcessRegen(Tokens[1]);
	}
	else if (Tokens[0] == "debug") {
		ProcessDebug(Tokens[1], Tokens[2], Tokens[3]);
	}
	else {
		cerr << Tokens[0] << "???" << endl;
	}

	ms = Timer.LapsedMSecs();
	ReportTime();

	// Continue running console, according to retVal
	return retVal;

}

bool Consola::ProcessDump(string& id, string& par, 
	string& p0, string& p1, string& p2, string &p3)
{
	//SolveEnv* pS;
	NEnv* pN;
	if (id == "")
		SymTab.LogClient::Dump();
	else {
		Symbol* pSym = SymTab.Find(id);
		int kind;
		if (pSym != 0) {
			switch (pSym->typ.t) {
			case SymType::TYPSPM:
			case SymType::TYPDEVSPM:
				break;

			case SymType::TYPNENV:
				pN = (NEnv*)(pSym->pObj);
				pN->DumpSet(0, p0 == "col");
				pN->DumpSet(1, tokToInt(p1));
				pN->DumpSet(2, tokToInt(p2));
				pN->DumpSet(3, tokToInt(p3));
				if (par == "mat")
					kind = DUMPMAT;
				else if (par == "lane")
					kind = DUMPLANES;
				else if (par == "back")
					kind = DUMPBACK;
				else if (par == "pset")
					kind = DUMPPSET;
				else if (par == "dist")
					kind = DUMPDIST;
				else
					return false;
				pN->LogClient::Dump(kind);

				break;

			case SymType::TYPOBJ:
			case SymType::TYPVAL:
			case SymType::TYPEMPTY:
			default:
				break;
			}
		}
	}

	return true;
}


bool Consola::ProcessRead(string& id, string& fname, string& dim, string& t)
{
	int n = tokToInt(dim);

	if (t == "") {
		// NEnv
		NEnv* pNEnv = new NEnv(n);

		if (!pNEnv->Read(fname)) {
			cerr << "Cannot load " << datadir + fname << endl;
			return false;
		}
		else {
			pNEnv->ProcessMat();
			SymTab.Add(id, SymType(SymType::TYPNENV), pNEnv, 0);
			return true;
		}
	}
	else {
		return true;
	}
}

bool Consola::ProcessReadRaw(string& id, string& fname, string& sortMode)
{
	NEnv* pNEnv = new NEnv();

	if (!pNEnv->ReadRaw(fname, sortMode)) {
		cerr << "Cannot load " << datadir + fname << endl;
		delete pNEnv;
		return false;
	}
	else {
		SymTab.Add(id, SymType(SymType::TYPNENV), pNEnv, 0);
		return true;
	}
}

bool Consola::ProcessWriteRaw(string& id, string& fname)
{
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;

	NEnv* pNEnv = (NEnv*)pSym->pObj;
	if (!pNEnv->WriteRaw(fname)) {
		cerr << "Cannot open " << fname << endl;
		return false;
	}

	return true;
}

bool Consola::ProcessUniqueRmv(string& id, string& fname)
{
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;

	NEnv* pNEnv = (NEnv*)pSym->pObj;
	pNEnv->UniqueRemove();
	if (!pNEnv->WriteRaw(fname)) {
		cerr << "Cannot open " << fname << endl;
		return false;
	}

	return true;
}

bool Consola::ProcessFill(string& id)
{
	//SolveEnv* pSEnv;
	NEnv* pN;
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;

	if (pSym->typ.t == SymType::TYPNENV) {
		pN = (NEnv*)pSym->pObj;
		while (pN->NHisto.Fill() > 0)
			;
		pN->FillBlanks();
		pN->RegenPSets();
		return true;
	}

	// Wrong object type
	return false;
}

bool Consola::ProcessSearch(string& id, string& kind, string& level)
{
	NEnv* pN;
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;

	if (pSym->typ.t == SymType::TYPNENV) {
		pN = (NEnv*)pSym->pObj;
		if (kind == "dist")
			pN->SearchD();
		else if (kind == "rinit") {
			pN->SearchRedInit(tokToInt(level));
		}
		else if (kind == "red") {
			pN->SearchRed();
		}
		else
			return false;
	}
	else
		return false;
	return true;
}

bool Consola::ProcessRegen(string& id)
{
	NEnv* pN;
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;

	if (pSym->typ.t == SymType::TYPNENV) {
		pN = (NEnv*)pSym->pObj;
		for (;;) {
			if (pN->RegenPSets() == 0)
				break;;
		}
		return true;
	}

	// Wrong object type
	return false;
}

bool Consola::ProcessSort(string& id, string& par)
{
	return true;
}

bool Consola::ProcessDebug(string& par0, string& par1, string& par2)
{
	if (par0 == "out") {
		if (!DbgSt.SetOut(par1)) {
			cerr << " Debug file " << par1 << " not open" << endl;
			return false;
		}
		else
			return true;
	}

	int cnt = tokToInt(par1);

	if (par0 == "exp") {
		DbgSt.dumpExpand = cnt;
	}
	else if (par0 == "range") {
		DbgSt.dumpRange = cnt;
	}
	else if (par0 == "comp") {
		DbgSt.dumpExpand = cnt;
	}
	else if (par0 == "term") {
		DbgSt.dumpTerm = cnt;
	}
	else if (par0 == "single") {
		DbgSt.dumpMakeSingle = cnt;
	}
	else
		cerr << " DEBUG bad param " << par0 << endl;
	return true;
}

bool Consola::ProcessLink(string& id)
{
	return true;

}

bool Consola::ProcessCount(string& id, string& lfrom, string& lto,
	string& ifrom, string& ito)
{
	return true;
}

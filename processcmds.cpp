//
//  Command console: command dispatching
//

#include "stdinc.h"
#include "senv.h"
#include "nenv.h"
#include "growenv.h"

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
		ProcessReadRaw(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "writeraw") {
		ProcessWriteRaw(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "randgen") {
		ProcessRandGen(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "uniquermv") {
		ProcessUniqueRmv(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "count") {
		ProcessCount(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5]);
	}
	else if (Tokens[0] == "perms") {
		ProcessPerms(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5], Tokens[6]);
	}
	else if (Tokens[0] == "fill") {
		ProcessFill(Tokens[1]);
	}
	else if (Tokens[0] == "fillents") {
		ProcessFillEnts(Tokens[1]);
	}
	else if (Tokens[0] == "envpar") {
		ProcessEnvPar(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "sort") {
		ProcessSort(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "link") {
		ProcessLink(Tokens[1]);
	}
	else if (Tokens[0] == "search") {
		ProcessSearch(Tokens[1], Tokens[2], Tokens[3]);
	}
	else if (Tokens[0] == "testchg") {
		ProcessTestChg(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
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
	EnvBase* pB;

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
			case SymType::TYPGENV:
				pB = (EnvBase*)(pSym->pObj);
				pB->DParsSet(0, p0 == "col");
				pB->DParsSet(1, tokToInt(p1));
				pB->DParsSet(2, tokToInt(p2));
				pB->DParsSet(3, tokToInt(p3));
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
				pB->LogClient::Dump(kind);
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
	else if (t == "grow") {
		// GEnv
		GEnv* pG = new GEnv(n);
		if (!pG->ReadSrc(fname)) {
			cerr << "Cannot load " << datadir + fname << endl;
			return false;
		}
		else {
			SymTab.Add(id, SymType(SymType::TYPGENV), pG, 0);
			pG->SortNone();
			return true;
		}
	}

	return false;
}

bool Consola::ProcessReadRaw(string& id, string& fname, string& kind, string& sortMode)
{
	if (kind == "grow") {
		GEnv* pGEnv = new GEnv();

		if (!pGEnv->ReadRaw(fname, sortMode)) {
			cerr << "Cannot load " << datadir + fname << endl;
			delete pGEnv;
			return false;
		}
		else {
			SymTab.Add(id, SymType(SymType::TYPGENV), pGEnv, 0);
			return true;
		}
	}
	else {
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
}

bool Consola::ProcessRandGen(string& id, string& dim, string& blkPerLane, string& seed)
{
	int n = tokToInt(dim);
	GEnv* pGEnv = new GEnv(n);
	SymTab.Add(id, SymType(SymType::TYPGENV), pGEnv, 0);

	return pGEnv->GenRaw(n, tokToInt(blkPerLane), tokToInt(seed));
}

bool Consola::ProcessTestChg(string& dim, string& blkPerLane, string& seedPar, string & cntPar)
{
	int n = tokToInt(dim);
	int blankPerLane = tokToInt(blkPerLane);
	int cnt = tokToInt(cntPar);

	srand(tokToInt(seedPar));

	int seed;
	int cntNO, cntCHG;
	for (int i = 0; i < cnt; i++) {
		GEnv* pG = new GEnv(n);
		seed = rand();
		srand(seed);

		pG->GenRaw(n, blankPerLane, seed);
		((EnvBase*)pG)->HistoScan();
		pG->FillBlanks();

		DbgSt.doChange = false;
		cntNO = pG->Search();

		DbgSt.doChange = true;
		cntCHG = pG->Search();

		if (cntNO != cntCHG) {
			Report(" SEARCH DIFF seed cntNO cntCHG ", seed, cntNO, cntCHG);
		}
		else
			Report(" SEARCH SAME seed cnt ", seed, cntNO);
	}
	return true;
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

bool Consola::ProcessEnvPar(string& par, string& val)
{
	return EPars.SetPar(par, val);
}

bool Consola::ProcessPerms(string& id, string& kind, string& doRow, string& lfrom, string& lto, string& doCheck)
{
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;
	EnvBase* pB = (EnvBase*)pSym->pObj;
	int k;
	if (kind == "ref")
		k = EnvBase::GENREF;
	else if (kind == "exp")
			k = EnvBase::GENEXP;
	else if (kind == "lvl")
		k = EnvBase::GENLVL;
	else if (kind == "all")
		k = EnvBase::GENALL;
	else {
		cerr << "BAD GEN KIND" << endl;
		return false;
	}
	
	pB->GenPerms(k, doRow == "row", tokToInt(lfrom), tokToInt(lto), doCheck == "check");
	return true;
}

bool Consola::ProcessFill(string& id)
{
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;

	if (pSym->typ.t == SymType::TYPNENV) {
		NEnv* pN = (NEnv*)pSym->pObj;
		while (pN->NHisto.Fill() > 0)
			;

		pN->FillBlanks();
		//pN->RegenPSets();
		return true;
	}
	else if (pSym->typ.t == SymType::TYPGENV) {
		GEnv* pG = (GEnv*)pSym->pObj;
		((EnvBase*)pG)->HistoScan();
		((EnvBase *)pG)->LogClient::Dump(DUMPMAT);
		pG->FillBlanks();
		return true;
	}

	// Wrong object type
	return false;
}

bool Consola::ProcessFillEnts(string& id)
{
	Symbol* pSym = SymTab.Find(id);
	if (pSym == 0)
		return false;

	if (pSym->typ.t == SymType::TYPNENV) {
		NEnv* pN = (NEnv*)pSym->pObj;
		pN->FillEnts();
		return true;
	}
	else if (pSym->typ.t == SymType::TYPGENV) {
		GEnv* pG = (GEnv*)pSym->pObj;
		pG->FillEnts();
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
	else if (pSym->typ.t == SymType::TYPGENV) {
		GEnv* pG = (GEnv*)pSym->pObj;
		pG->Search();
		return true;
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

bool Consola::ProcessSort(string& id, string& srcId, string& rowOrder, string& colOrder)
{
	GEnv* pG = new GEnv();
	Symbol* pSym = SymTab.Find(srcId);
	if (pSym == 0)
		return false;

	GEnv* pSrc = (GEnv*)pSym->pObj;
	pG->CopyFrom(*pSrc, rowOrder, colOrder);

	SymTab.Add(id, SymType(SymType::TYPGENV), pG, 0);
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
	else if (par0 == "change") {
		DbgSt.doChange = (cnt > 0);
	}
	else if (par0 == "term") {
		DbgSt.dumpTerm = cnt;
	}
	else if (par0 == "single") {
		DbgSt.dumpMakeSingle = cnt;
	}
	else if (par0 == "searchlim") {
		DbgSt.searchLim = cnt;
	}
	else if (par0 == "dumpres") {
		DbgSt.dumpResult = cnt;
	}
	else if (par0 == "reportcreate") {
		DbgSt.reportCreate = true;
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

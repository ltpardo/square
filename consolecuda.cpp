//
//  Command console
//

#include <stdinc.h>

extern bool testcuda();
extern int TestMark(DevSpM* pDev);
extern int MMarkCuda(DevSpM* pDev);

//////////////////////////////////////////////////////////
//
//	Input handling
//
//////////////////////////////////////////////////////////

void InputState::set(istream* _pIn, const char* _prompt,
	const char* _name, bool _isBottom)
{
	pIn = _pIn;
	prompt = _prompt;
	name = _name;
	isBottom = _isBottom;
}

void InputState::set(istream* _pIn, string _prompt,
	string _name, bool _isBottom)
{
	pIn = _pIn;
	prompt = _prompt;
	name = _name;
	isBottom = _isBottom;
}


//////////////////////////////////////////////////////////
//
//	Consola
//
//////////////////////////////////////////////////////////

int Consola::tokToInt(string& tok)
{
	int v;
	std::istringstream strm(tok);
	strm >> v;
	return v;
}

float Consola::tokToFloat(string& tok)
{
	float v;
	std::istringstream strm(tok);
	strm >> v;
	return v;
}

// Input line and tokenize
bool Consola::getCmdLine()
{
	tokenCnt = 0;
	inputSt.pIn->getline(lineBuf, sizeof(lineBuf));
	if (lineEof = inputSt.pIn->eof())
		return false;

	// Tokenize
	std::istringstream strm(lineBuf);
	string tok;
	for (; tokenCnt < tokenMax; ) {
		tok = "";
		strm >> tok;
		if (tok.size() > 0) {
			if (tok[0] == commentChar)
				// Rest of line is a comment
				break;
			Tokens[tokenCnt++] = tok;
		}
		if (strm.eof())
			break;
	}
	return true;
}

// Echo line on console output
void Consola::showLine()
{
	if (!inputSt.isBottom)
		std::cout << lineBuf << endl;
}

// Echo line on Logger
void Consola::logLine()
{
	string msg = "\n>>>>  ";
	msg += lineBuf;
	msg += "   >>>>\n";
	Report(msg);
}

// Keep track of IF / FI / ELSE depth
bool Consola::processDepth()
{
	if (Tokens[0] == "if") {
		DSt.push(dpst);
		if (!dpst.exec) {
			dpst = NestState(false, NestState::TOFI);
		}
		else {
			bool cond = (Tokens[1] == "t");
			dpst = NestState(cond, NestState::TOELSE);
		}
	}
	else if (Tokens[0] == "else") {
		if (!dpst.mode == NestState::TOELSE)
			cerr << "Invalid else" << endl;
		dpst = NestState(!dpst.exec, NestState::TOFI);
	}
	else if (Tokens[0] == "fi") {
		dpst = DSt.top();
		DSt.pop();
	}
	else
		return false;
	return true;
}

#if 0
// Parse token: dst.var.comp.fld
void Consola::parseFields(string& tok)
{
	std::istringstream strm(tok);

	for (fldCnt = 0; fldCnt < FLDCNTMAX; fldCnt++) {
		if (strm.eof())
			break;
		std::getline(strm, fields[fldCnt], '.');
		if (fields[fldCnt].size() == 0) {
			fldCnt--;
			break;
		}
	}
}
#endif // 0

void Consola::setAtBottom()
{
	inputSt.set(&cin, ">> ", "console", true);
	InStk.push(inputSt);
}

void Consola::popInput()
{
	if (inputSt.pIn != &cin) {
		pFS = (ifstream*)inputSt.pIn;
		pFS->close();
		delete pFS;
	}
	inputSt = InStk.top();
	InStk.pop();
}

void Consola::pushInput(const char *fName)
{
	string openName = scriptdir + fName;
	pFS = new std::ifstream(openName);
	if (!pFS->is_open()) {
		cerr << "Failed exec " << openName << endl;
	}
	else {
		InStk.push(inputSt);
		inputSt.set(pFS, (string("[") + fName + "] "),
			string(fName), false);
	}
}

// Process file execution
void  Consola::procExec()
{
	if (tokenCnt <= 1) {
		pushInput(scriptDefault.c_str());
	} else
		pushInput(Tokens[1].c_str());
}

// Process end of execution from a command stream
bool Consola::procEnd()
{
	if (Tokens[1] == "run") {
		// Pop all levels and end session
		while (!inputSt.isBottom) {
			popInput();
		}
	}

	if (!inputSt.isBottom) {
		popInput();
		return true;
	}

	// Already at bottom, end of console session
	//sendCmd(CmdCarrier::DstCode::DST_STUB, (u8)StubOps::START, 0, 0);
	return false;
}

// Push in a new console
void Consola::pushCons()
{
	InStk.push(inputSt);
	inputSt.set(&cin, string(">>"), "", false);
}

// Top level dispatcher of user commands
//	Returns false when user access should stop:
//		either "end" command was issued at bottom level
//		or a response is needed
//
bool Consola::getCmd()
{
	bool retVal = true;

	outPrompt();
	getCmdLine();
	showLine();
	logLine();
	if (tokenCnt <= 0)
		// Empty line
		return true;

	// Process structure commands first
	if (processDepth())
		// Nesting command
		return true;

	if (!dpst.exec)
		// Nested Skipping
		return true;

	if (Tokens[0] == "exec") {
		procExec();
	}
	else if (Tokens[0] == "end") {
		return procEnd();
	}
	else if (Tokens[0] == "cons") {
		pushCons();
	}
	else if (Tokens[0] == "quit") {
		// Brute force exit
		exit(0);
	}
	else 
		return processCmd();

	return retVal;
}

bool Consola::processCmd()
{
	bool retVal = true;

	Timer.Start();

	if (Tokens[0] == "set") {
		SymTab.Add(Tokens[1], SymType(SymType::TYPVAL), 0, tokToInt(Tokens[2]));
	}
	else if(Tokens[0] == "get") {
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
		ProcessDump(Tokens[1]);
	}
	else if (Tokens[0] == "read") {
		ProcessRead(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "histo") {
		ProcessHisto(Tokens[1], tokToInt(Tokens[2]), Tokens[3]);
	}
	else if (Tokens[0] == "mult") {
		ProcessMult(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5]);
	}
	else if (Tokens[0] == "equals") {
		ProcessEquals(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "count") {
		ProcessCount(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "stats") {
		ProcessStats(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "slicemap") {
		ProcessSliceMap(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "devcopy") {
		ProcessDevCopy(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "devset") {
		ProcessDevSet(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "devplan") {
		ProcessDevPlan(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5], Tokens[6]);
	}
	else if (Tokens[0] == "dumpplan") {
		ProcessDumpPlan(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "dumplines") {
		ProcessDumpLines(Tokens[1], Tokens[2], Tokens[3]);
	}
	else if (Tokens[0] == "devrun") {
		ProcessDevRun(Tokens[1], Tokens[2]);
	}
	else if (Tokens[0] == "devmultone") {
		ProcessDevMultOne(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5]);
	}
	else if (Tokens[0] == "devmult") {
		ProcessDevMultMany(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5], Tokens[6], Tokens[7]);
	}
	else if (Tokens[0] == "system") {
		ProcessSystem(Tokens[1], Tokens[2], Tokens[3], Tokens[4]);
	}
	else if (Tokens[0] == "cuda") {
		testcuda();
	}
	else if (Tokens[0] == "cudamult") {
		ProcessCudaMult(Tokens[1], Tokens[2], Tokens[3], Tokens[4], Tokens[5], Tokens[6], Tokens[7], Tokens[8]);
	}
	else if (Tokens[0] == "cudamark") {
		ProcessCudaMark(Tokens[1], Tokens[2]);
	}
	else {
		cerr << Tokens[0] << "???" << endl;
	}

	ms = Timer.LapsedMSecs();
	ReportTime();

	// Continue running console, according to retVal
	return retVal;

}

void Consola::ReportTime()
{
	ostringstream msStr;
	msStr << ms;
	string msg = "  [[[[[  Command Time ";
	msg += msStr.str();
	msg += " msecs  ]]]]]\n";
	Report(msg);
}

bool Consola::ProcessDump(string &id)
{
	SpM* pSpM;
	if (id == "")
		SymTab.LogClient::Dump();
	else {
		Symbol* pSym = SymTab.Find(id);
		if (pSym != 0) {
			switch (pSym->typ.t) {
			case SymType::TYPSPM:
			case SymType::TYPDEVSPM:
				pSpM = (SpM *) pSym->pObj;
				pSpM->LogClient::Dump(SpM::DUMPLVL1);
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

bool Consola::ProcessDumpLines(string& id, string& fromIStr, string& toIStr)
{
	if (GetSpM(pSpMA, id)) {
		int frI = tokToInt(fromIStr);
		int toI = tokToInt(toIStr);
		pSpMA->LinesDump(frI, toI);

		return true;
	}
	return false;
}

bool Consola::ProcessSystem(string& blockCnttok, string& qSzLogtok, string& bufCntLogTok, string& arg)
{
	int blockCnt  = tokToInt(blockCnttok);
	int qSzLog    = tokToInt(qSzLogtok);
	int bufCntLog = tokToInt(bufCntLogTok);
	pSys = new ParSystem(blockCnt, qSzLog, bufCntLog);
	pSys->Create();

	return true;
}

bool Consola::ProcessDevSet(string& idRes, string& idA, string& idB, string& warpCnt)
{
	if (!GetSpMAB(idA, idB))
		return false;

	DevMult* pDevM = new DevMult();

	pDevM->Prepare(pSys, pSpMA, pSpMB);
	if (warpCnt != "")
		pDevM->SetWarpCnt(tokToInt(warpCnt));
	pSys->ServiceSet(pDevM);

	SymTab.Add(idRes, SymType::TYPDEVMUL, pDevM, 0);

	return true;
}

bool Consola::ProcessDevMultOne(string& idMul, string& idRes, string& idA, string& idB, string& partStr)
{
	if (!GetSpMAB(idA, idB))
		return false;

	DevMult* pDevM = new DevMult();
	SymTab.Add(idMul, SymType::TYPDEVMUL, pDevM, 0);
	float part = tokToFloat(partStr);

	SpM* pSpM = pDevM->MultOneBlock(pSys, pSpMA, pSpMB, part);

	SymTab.Add(idRes, SymType::TYPDEVSPM, pSpM, 0);

	return true;
}

bool Consola::ProcessDevMultMany(string& idMul, string& idRes, string& idA, string& idB, string& partStr, string& bCnt, string& lineMax)
{
	if (!GetSpMAB(idA, idB))
		return false;

	pSys = new ParSystem();
	pSys->AllocParams(tokToInt(bCnt), pSpMA->frame.nRows);
	pSys->Create();

	DevMult* pDevM = new DevMult();
	SymTab.Add(idMul, SymType::TYPDEVMUL, pDevM, 0);
	float part = tokToFloat(partStr);
	pDevM->blockCntTarget = tokToInt(bCnt);

	SpM* pSpM = pDevM->MultAllBlocks(pSys, pSpMA, pSpMB, tokToInt(lineMax), part);

	SymTab.Add(idRes, SymType::TYPDEVSPM, pSpM, 0);

	return true;
}

extern SpM* CudaMult(DevMult* pMult);
bool Consola::ProcessCudaMult(string& idMul, string& idRes, string& idA, string& idB, 
	string& partStr, string& bCnt, string& lineMax, string& warpsPerBlock)
{
	if (!GetSpMAB(idA, idB))
		return false;

	DevMult* pDevM = new DevMult();
	pDevM->SetCudaMult(tokToInt(warpsPerBlock));
	pDevM->blockCntTarget = tokToInt(bCnt);

	SymTab.Add(idMul, SymType::TYPDEVMUL, pDevM, 0);
	float part = tokToFloat(partStr);

	pDevM->MultAllInit(NULL, pSpMA, pSpMB, tokToInt(lineMax), part);

	SpM* pSpM = CudaMult(pDevM);

	SymTab.Add(idRes, SymType::TYPDEVSPM, pSpM, 0);

	return true;
}

bool Consola::ProcessDevPlan(string& idDev, string& blkId, string& fromIStr, string& toIStr, string& partStr, string& dump)
{
	DevMult* pDevM;
	if ((pDevM = (DevMult*) (SymTab.FetchObj(idDev, SymType::TYPDEVMUL))) == 0)
		return false;

	int frI = tokToInt(fromIStr);
	int toI = tokToInt(toIStr);
	float part = tokToFloat(partStr);
	if (frI < 0)
		frI = 0;
	if (toI < 0)
		toI = pDevM->pDA->nRows - 1;
	if (part < 0)
		part = -1.5f;

	pDevM->PrepCmds(tokToInt(blkId), frI, toI, part);

	if (dump == "dump") {
		pDevM->Dumper.SetRange(frI, toI);
		pDevM->LogClient::Dump(pDevM->DUMP);
	}
	else if (dump == "dumpwarps") {
		pDevM->Dumper.SetRange(frI, toI);
		pDevM->LogClient::Dump(pDevM->DUMPWARPS);
	}

	return true;
}

bool Consola::ProcessDumpPlan(string& idDev, string& fromIStr, string& toIStr, string& doWarps)
{
	DevMult* pDevM;
	if ((pDevM = (DevMult*)(SymTab.FetchObj(idDev, SymType::TYPDEVMUL))) == 0)
		return false;

	int frI = tokToInt(fromIStr);
	int toI = tokToInt(toIStr);
	if (frI < 0)
		frI = 0;
	if (toI < 0)
		toI = pDevM->pDA->nRows - 1;
	pDevM->Dumper.SetRange(frI, toI, doWarps == "warps");
	int typ = doWarps == "warps" ? pDevM->DUMPWARPS : pDevM->DUMP;
	pDevM->LogClient::Dump(typ);

	return true;
}

bool Consola::ProcessDevRun(string& idDev, string& blkId)
{
	DevMult* pDevM;
	if ((pDevM = (DevMult*)(SymTab.FetchObj(idDev, SymType::TYPDEVMUL))) == 0)
		return false;

	int blk = tokToInt(blkId);
	
	if (! pDevM->Run(blk))
		cerr << "BlkId " << blk << " is not current" << endl;

	return true;
}

bool Consola::ProcessDevCopy(string& idRes, string& idA, string& op, string& dmp)
{
	if (!GetSpM(pSpMA, idA))
		return false;

	DevSpM* pDev;
	if (op == "dir") {
		pDev = pSpMA->pRowM->MakeDevCopy(false);
	}
	else if (op == "slice") {
		pDev = pSpMA->pRowM->MakeDevCopy(true);
	}
	else {
		cerr << "Unsupported DevCopy kind: " << op << endl;
		return false;
	}

	SymTab.Add(idRes, SymType::TYPDEVSPM, pDev, 0);

	return true;
}

bool Consola::ProcessHisto(string &id, int levels, string &dump)
{
	if (GetSpM(pSpMA, id)) {
		pSpMA->HistoMake(levels, dump == "dump");
		return true;
	}
	return false;
}

bool Consola::ProcessStats(string& id, string& dump)
{
	if (GetSpM(pSpMA, id)) {
		pSpMA->StatsProcess();
		if (dump == "dump")
			pSpMA->Stats.LogClient::Dump();
		return true;
	}
	return false;
}

bool Consola::ProcessSliceMap(string& id, string& dump)
{
	if (GetSpM(pSpMA, id)) {
		pSpMA->SliceMapProcess();
		if (dump == "dump")
			pSpMA->SlcMap.LogClient::Dump();
		return true;
	}
	return false;
}

bool Consola::ProcessRead(string& id, string& fname)
{
	SpM* pSpM = new SpM();

	if (!pSpM->Process(fname, datadir)) {
		cerr << "Cannot load " << datadir + fname << endl;
		return false;
	}
	else {
		SymTab.Add(id, SymType(SymType::TYPSPM), pSpM, 0);
		return true;
	}
}

bool Consola::ProcessMult(string& idRes, string& idA, string& idB, string &op, string &dmp)
{
	if (!GetSpMAB(idA, idB))
		return false;

	SpMMultDesc* pMD;
	if (op == "dir") {
		pMD = new SpMMultDesc(pSpMA, pSpMB, "Dir");
		pMD->MultDirect();
	}
	else if (op == "row") {
		pMD = new SpMMultDesc(pSpMA, pSpMB, "Row*Row");
		pMD->MultRowRow();
	}
	else {
		cerr << "Unsupported mult kind: " << op << endl;
		return false;
	}

	SymTab.Add(idRes, SymType::TYPSPM, pMD->pResult, 0);

	if (dmp == "dump")
		pMD->LogClient::Dump(SpMMultDesc::DUMPMULT);

	return true;
}

bool Consola::ProcessCount(string& idA, string& idB, string& op, string& dmp)
{
	if (!GetSpMAB(idA, idB))
		return false;

	SpMMultDesc* pMD;
	int dmpKind;
	if (op == "dir") {
		pMD = new SpMMultDesc(pSpMA, pSpMB, "Dir");
		pMD->MultFlopsCnt();
		dmpKind = SpMMultDesc::DUMPMULT;
	}
	else if (op == "row") {
		pMD = new SpMMultDesc(pSpMA, pSpMB, "Row*Row");
		pMD->MultByBandsCnt();
		dmpKind = SpMMultDesc::DUMPBANDS;
	}
	else if (op == "rowflops") {
		pMD = new SpMMultDesc(pSpMA, pSpMB, "Row*Row");
		pMD->MultRowRow(true);
		dmpKind = SpMMultDesc::DUMPMULT;
	}
	else {
		cerr << "Unsupported mult kind: " << op << endl;
		return false;
	}

	if (dmp == "dump")
		pMD->LogClient::Dump(dmpKind);
	return true;
}

bool Consola::ProcessEquals(string &idA, string &idB)
{
	string res;
	string msg;
	if (!GetSpMAB(idA, idB))
		return false;

	int lineErr, errCnt;

	bool eq = pSpMA->Equals(pSpMB, lineErr, errCnt);
	res = eq ? " EQUALS " : " DIFFERS ";
	msg = pSpMA->title + res + pSpMB->title + "\n";
	Report(msg);
	return eq;
}

bool Consola::ProcessCudaMark(string& idA, string& cuda)
{
	if (!GetSpM(pSpMA, idA))
		return false;

	DevSpM * pDA = pSpMA->pRowM->MakeDevCopy(false);

	int errCnt;

	if (cuda == "cuda")
		errCnt = MMarkCuda(pDA);
	else
		errCnt = TestMark(pDA);

	if (errCnt > 0)
		return false;
	return true;
}

bool Consola::GetSpM(SpM * &pSpM, string& id)
{
	string msg;
	pSpM = SymTab.FetchSpM(id);
	if (pSpM == 0)
		return false;
	if (!pSpM->isIntact) {
		msg = pSpM->title + " NOT INTACT\n";
		Report(msg);
		return false;
	}
	return true;
}

bool Consola::GetSpMAB(string& idA, string& idB)
{
	string msg;
	if (!GetSpM(pSpMA, idA))
		return false;
	if (!GetSpM(pSpMB, idB))
		return false;
	return true;
}



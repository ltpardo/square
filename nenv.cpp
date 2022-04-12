#include "nlane.h"
#include "nenv.h"

using namespace std;
SolveStack SStk;

bool NEnv::Read(string& fname)
{
	IO.ReadOpen(fname);
	for (int i = 0; i < n; i++) {
		IO.ReadLine(Mat[i], 2, n);
	}

	return true;
}

bool NEnv::Read(char* fname)
{
	IO.ReadOpen(fname);
	for (int i = 0; i < n; i++) {
		IO.ReadLine(Mat[i], 2, n);
	}

	return true;
}

void NEnv::ReadLineRaw(ifstream &in, u8* pLine, int cnt)
{
	int i;
	int v;
	in >> i;
	for (int p = 0; p < cnt; p++) {
		in >> v;
		pLine[p] = v;
	}
}

void NEnv::OutLineRaw(ostream& out, u8* pLine, int i, int cnt)
{
	out << setw(2) << i << " ";
	for (int j = 0; j < cnt; j++) {
		out << setw(3) << (int)pLine[j];
	}
	out << endl;
}

bool NEnv::ReadRaw(string& fname, string sortMode)
{
	ifstream in(fname);
	if (!in.is_open())
		return false;
	in >> n;
	Init();

	for (int i = 0; i < n; i++) {
		ReadLineRaw(in, Mat[i], n);
	}

	for (int j = 0; j < n; j++)
		in >> ColsPermCnt[j];

	in.close();

	if (sortMode == "up") {
		SortRaw(true);
	}
	else if (sortMode == "down") {
		SortRaw(false);
	}

	ProcessMat();
	return true;
}

bool SortCompUp(u32 a, u32 b) { return a <= b; }
bool SortCompDn(u32 a, u32 b) { return a >= b; }

bool NEnv::SortRaw(bool ascending)
{
	u32 SortTab[DIMMAX];
	FastMatrix<FASTMATDIMLOG, u8> MatAux;
	MatAux = Mat;

	for (int j = 0; j < n; j++)
		SortTab[j] = (ColsPermCnt[j] << 8) | j;

	if (ascending)
		std::sort(SortTab, SortTab + n, SortCompUp);
	else
		std::sort(SortTab, SortTab + n, SortCompDn);

	for (int j = 0; j < n; j++) {
		int jSrc = SortTab[j] & 0xFF;
		for (int i = 0; i < n; i++)
			Mat[i][j] = MatAux[i][jSrc];
	}

	return true;
}

bool NEnv::WriteRaw(string& fname)
{
	ofstream *pOut;
	pOut = new std::ofstream(fname);
	if (!pOut->is_open())
		return false;
	*pOut << n << endl;
	for (int i = 0; i < n; i++) {
		OutLineRaw(*pOut, Mat[i], i, n);
	}
	*pOut << endl << endl;

	for (int j = 0; j < n; j++)
		*pOut << setw(6) << ColsPermCnt[j];
	*pOut << endl;
	pOut->close();

	return true;
}


void Histo<NLane>::Start()
{
	cnt = 0;
	for (int v = 0; v < n; v++) {
		VCnt[v] = 0;
		LastCol[v] = 0;
	}
}

void Histo<NLane>::Add(Set s, int idx)
{
	Mask m = 1;
	for (int v = 0; v < n; v++) {
		if (s.Contains(m)) {
			VCnt[v]++;
			LastCol[v] = idx;
			cnt++;
		}
		m <<= 1;
	}
}

int Histo<NLane>::Scan(int j)
{
	int scanCnt = 0;
	Mask vMask;
	int fixCnt = 0;
	for (int v = 0; v < n; v++) {
		scanCnt += VCnt[v];
		vMask = Set::BitMask(v);
		if (Cols[j].absMiss.Contains(vMask)) {
			assert(VCnt[v] > 0);
			if (VCnt[v] <= 1) {
				// Forced value
				int i = LastCol[v];
				if (fixCnt == 0) {
					Report("    Scan col ", j);
				}
				fixCnt++;
				Report(" row/v ", i, v);

				// Change Mat and SLanes
				(* pMat)[i][j] = v;
				Cols[j].absMiss -= vMask;
				Cols[j].missCnt--;
				if (Rows[i].absMiss.Contains(vMask)) {
					Rows[i].absMiss -= vMask;
					Rows[i].missCnt--;
				}
			}
		}
	}
	if (scanCnt != cnt) {
		Report(" CNT ERROR  histo/scan ", cnt, scanCnt);
		fixCnt = -1;
	}

	if (fixCnt != 0)
		Report("\n");

	return fixCnt;
}

int Histo<NLane>::Fill()
{
	Report("\n FILL \n");

	int fixCnt = 0;
	for (int j = 0; j < n; j++) {
		Start();
		for (int i = 0; i < n; i++) {
			if ((*pMat)[i][j] > n) {
				// Blank entry
				Add(Rows[i].absMiss & Cols[j].absMiss, i);
			}
		}
		fixCnt += Scan(j);
	}
	return fixCnt;
}

void NEnv::ProcessMat()
{
	u8 v;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			v = Mat(i, j);
			if (v < n) {
				Rows[i].Present(v);
				Cols[j].Present(v);
			}
		}
	}

	for (int l = 0; l < n; l++) {
		Rows[l].Check();
		Cols[l].Check();
	}

	// Copy Mat for future verification
	MatCpy = Mat;
}

void NEnv::FillBlanks()
{
	NEntry* pBlank;
	// Create BStacks
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (Mat(i, j) > n) {
				pBlank = new NEntry(i, j);
				Rows[i].AddBlank(pBlank);
				Cols[j].AddBlank(pBlank);
			}
		}
	}

	// Fill Cols (will assume thru direction)
	blankCnt = 0;
	for (int j = 0; j < n; j++) {
		Cols[j].FillBlanks(Rows);
		blankCnt += Cols[j].blankCnt;
		ColsPermCnt[j] = Cols[j].PSet.permCnt;
	}

	// Fill Rows (cross direction)
	for (int i = 0; i < n; i++) {
		Rows[i].FillBlanks(Cols);
	}

	LinkBlanks();
	// SearchDistComp();
}

int NEnv::RegenPSets()
{
	int regCnt = 0;
	for (int j = 0; j < n; j++) {
		regCnt += Cols[j].RegenPSet();
	}

	for (int i = 0; i < n; i++) {
		Rows[i].RegenPSet();
	}

	Report(" REGEN cnt ", regCnt);
	return regCnt;
}

int NEnv::UniqueRemove()
{
	int rmvCnt = 0;
	NLane* pLane;
	NEntry* pBlank;
	for (int j = 0; j < n; j++) {
		pLane = Cols + j;
		for (int level = 0; level < pLane->blankCnt; level++) {
			pBlank = pLane->NStack[level].pEnt;
			if (pBlank->missCnt == 1) {
				Mat[pBlank->i][pBlank->j] = pBlank->missV[0];
				rmvCnt++;
			}
		}
	}

	Report("   UNIQUE removed ", rmvCnt);
	return rmvCnt;
}

// Link Cols
void NEnv::LinkBlanks()
{
	for (int l = 0; l < n; l++) {
		Cols[l].LinkThru(Rows, Cols);
	}

	// Mark end of chain
	for (int l = 0; l < n; l++) {
		Cols[l].ringHd.upThru->thLast = true;
		Rows[l].ringHd.upCross->crLast = true;
	}

	pFirst = Cols[0].NStack[0].pEnt;
}

void NEnv::SearchPublish()
{
	solveCnt++;
}

void NEnv::SearchThruInitCol()
{
	pColCur = Cols + colLevel;
	pColCur->NextPermInit();
}

void NEnv::SearchThruDump(bool isDown)
{
	if (DbgSt.dumpExpand) {
		*DbgSt.pOut << (isDown ? "vvvv Down to " : "^^^^ Up to ")
			<< colLevel << endl;
	}
}

void NEnv::SearchThruInit()
{
	colLevel = 0;
	SearchThruInitCol();

	// Init PSets first access
	for (int l = 0; l < n; l++) {
		Cols[l].ringHd.dnThru->thUpBr = 0;
		Rows[l].ringHd.dnCross->crUpBr = 0;
	}
}

void NEnv::SearchDnCol()
{
	colLevel++;
	SearchThruInitCol();
	SearchThruDump(true);
}

void NEnv::SearchUpCol()
{
	NEntry* pBack;
	if (!pColCur->SearchBackDst(pColCur - 1)) {
		colLevel--;
		pColCur = Cols + colLevel;
		pColCur->Backtrack(NULL);
	}
	else {
		colLevel = pColCur->backJ;
		pBack = pColCur->pBackDst;
		pColCur = Cols + colLevel;
		pColCur->Backtrack(pBack);
	}

	SearchThruDump(false);
}

int NEnv::SearchThru()
{
	SearchThruInit();
	RngTrack.Init();

	while (colLevel >= 0) {
		RngTrack.Collect(colLevel);
		if (colLevel >= DbgSt.dumpLevel)
			DbgSt.dumpExpand = 0x2000;
		else
			DbgSt.dumpExpand = 0;

		if (pColCur->NextPermInCol()) {
			// Process perm in current column
			if (SearchThruSuccess()) {
				// Found Solution
				SearchPublish();
			}
			else {
				// Proceed down
				SearchDnCol();
			}
		}
		else {
			// Current column is done
			SearchUpCol();
		}
	}

	cout << endl << " SEARCH CNT " << solveCnt << endl;
	return solveCnt;
}

void NEnv::DumpMat(ostream& out)
{
	u8 buf[DIMMAX];
	IO.OutSet(out);
	IO.OutHeader(n);
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			buf[j] = Mat[i][j];
		}
		IO.OutLineV(buf, i, n);
	}
}

void NEnv::DumpBack(ostream& out)
{
	const int fldWd = 3;
	int d;
	int bd;
	NEntry* pEnt, *pTo;
	NLane* pLane;
	IO.OutSet(out);
	IO.OutHeaderDirect(n, fldWd);

	for (int i = 0; i < n; i++) {
		NLane* pRow = Rows + i;
		d = 0;
		int v;
		IO.OutRowNum(-1);
		for (int j = 0; j < n; j++) {
			IO.OutSep(j);
			v = Mat[i][j];
			if (v >= n) {
				pEnt = pRow->NStack[d++].pEnt;
				if (pEnt->IsRedTop()) {
					IO.OutStr(" # ");
				}
				else {
					pLane = NLane::LaneFromLevel(pEnt->redBackLvl, Rows, Cols);
					bd = pEnt->redBackDepth;
					pTo = pLane->NStack[bd].pEnt;
					if (pLane->isRow)
						IO.OutPair('I', pTo->i, fldWd);
					else
						IO.OutPair('J', pTo->j, fldWd);
				}
			}
			else {
				IO.OutPad(fldWd);
			}
		}
		IO.EndLine();

		d = 0;
		IO.OutRowNum(i);
		for (int j = 0; j < n; j++) {
			IO.OutSep(j);
			v = Mat[i][j];
			if (v >= n) {
				pEnt = pRow->NStack[d++].pEnt;
				if (pEnt->IsRedTop()) {
					IO.OutStr(" # ");
				}
				else {
					pLane = NLane::LaneFromLevel(pEnt->redBackLvl, Rows, Cols);
					bd = pEnt->redBackDepth;
					pTo = pLane->NStack[bd].pEnt;
					if (pLane->isRow)
						IO.OutPair('J', pTo->j, fldWd);
					else
						IO.OutPair('I', pTo->i, fldWd);
				}
			}
			else {
				IO.OutVal(v, fldWd);
			}
		}
		IO.EndLine();
	}
}

void NEnv::DumpDist(ostream& out)
{
	out << endl << " DISTANCES  max: " << distMax << endl;
	for (int d = 0; d < distMax; d++) {
		out << setw(3) << d << setw(4) << DistLvlCnt[d] << endl;
	}
	out << endl;
}

// DPars[0]: isCol
// DPars[1]: idx
// DPars[2]: dump PSet.NodeCnt[]
//
bool NEnv::DumpLane(ostream& out)
{
	NLane* pLane;
	string kind;
	if (DPars[1] >= n)
		return false;
	if (DPars[0]) {
		pLane = Cols;
		kind = "COL";
	}
	else {
		pLane = Rows;
		kind = "ROW";
	}

	if (DPars[1] < 0) {
		for (int idx = 0; idx < n; idx++) {
			out << "PERMS " << kind << setw(3) << idx;
			pLane[idx].PSet.Dump(out, DPars[2] != 0);
		}
	}
	else {
		out << "PERMS " << kind << setw(3) << DPars[1];
		pLane[DPars[1]].PSet.Dump(out, DPars[2] != 0);
	}
	return true;
}

// DPars[0]: isCol
// DPars[1]: idx
// DPars[2]: lvlFr
// DPars[3]: lvlTo
//
bool NEnv::DumpPSet(ostream& out)
{
	NLane* pLane;
	if (DPars[1] >= n || DPars[1] < 0)
		return false;
	pLane = DPars[0] ? Cols : Rows;
	pLane += DPars[1];
	pLane->PSet.DumpLvls(out, DPars[2], DPars[3]);
	return true;
}

bool NEnv::DumpSet(int par, int parVal)
{
	if (par < 0 || par >= DPARMAX)
		return false;
	DPars[par] = parVal;
	return true;
}

void NEnv::Dump(ostream& out, int dumpType)
{
	switch (dumpType) {
	case DUMPALL:
		//Dump(out, false, false);
		break;

	case DUMPMAT:
		DumpMat(out);
		break;

	case DUMPBACK:
		DumpBack(out);
		break;

	case DUMPLANES:
		DumpLane(out);
		break;

	case DUMPRINGS:
		//DumpRings(out);
		break;

	case DUMPPSET:
		DumpPSet(out);
		break;

	case DUMPDIST:
		DumpDist(out);
		break;

	default:
		break;
	}
}

bool NEnv::SearchDistThru(int j)
{
	NEntry* pEnt;
	int dist;

	dist = 0;
	NLane* pColCur = Cols + j;
	for (pEnt = Cols[j].NStack[0].pEnt; ; pEnt = pEnt->dnThru) {
		if (dist < pEnt->upCross->dLevel)
			dist = pEnt->upCross->dLevel;
		dist++;
		pEnt->dLevel = dist;
		SearchDistInsert(pEnt);
		if (pEnt->thLast)
			break;
	}
	return true;
}

void NEnv::SearchDistLink()
{
	NEntry* pEnt;

	DistLvlBase[0] = 0;
	for (int d = 1; d <= distMax; d++) {
		DistLvlBase[d] = DistLvlBase[d - 1] + DistLvlCnt[d - 1];
	}
	assert(blankCnt ==
		DistLvlBase[distMax] + DistLvlCnt[distMax]);

	for (int j = 0; j < n; j++) {
		for (pEnt = Cols[j].NStack[0].pEnt; ; pEnt = pEnt->dnThru) {
			pEnt->dLvlPos += DistLvlBase[pEnt->dLevel];
			DEntIndex[pEnt->dLvlPos] = pEnt;
			if (pEnt->thLast)
				break;
		}
	}

	int b;
	for (b = 1; b < DistLvlBase[2]; b++) {
		pEnt = DEntIndex[b];
		pEnt->backPos = b - 1;
	}

	for ( ; b < blankCnt; b++) {
		pEnt = DEntIndex[b];
		assert(pEnt != NULL);
		if (pEnt->upThru->dLvlPos > pEnt->upCross->dLvlPos)
			pEnt->backPos = pEnt->upThru->dLvlPos;
		else
			pEnt->backPos = pEnt->upCross->dLvlPos;
	}
}

bool NEnv::SearchDistComp()
{
	for (int j = 0; j < n; j++) {
		SearchDistThru(j);
	}
	SearchDistLink();
	return true;
}

void NEnv::SearchDistInit()
{
	for (int i = 0; i < INDEXMAX; i++)
		DEntIndex[i] = NULL;

	for (int d = 0; d < DISTMAX; d++) {
		DistLvl[d] = 0;
		DistLvlCnt[d] = 0;
	}
	distMax = 0;
}

bool NEnv::SearchDistInsert(NEntry* pEnt)
{
	int d = pEnt->dLevel;
	if (distMax < d)
		distMax = d;

	pEnt->pDist = DistLvl[d];
	DistLvl[d] = pEnt;
	pEnt->dLvlPos = DistLvlCnt[d];
	DistLvlCnt[d]++;

	return true;
}


bool NEnv::SearchDSuccess()
{
	return dPos >= blankCnt;
}

void NEnv::SearchDInit()
{
	pBSet = new BitSet(blankCnt + 1);
	// Init PSets first access
	for (int l = 0; l < n; l++) {
		Cols[l].ringHd.dnThru->thUpBr = 0;
		Rows[l].ringHd.dnCross->crUpBr = 0;
	}

	for (int e = 0; ; e++) {
		pEnt = DEntIndex[e];
		if (pEnt->dLevel > 1)
			break;
		pEnt->thUpBr = 0;
		pEnt->crUpBr = 0;
		pEnt->ESt.BrNewSet();
	}
	dLevel = 0;
	dPos = 0;
}

int NEnv::SearchD()
{
	SearchDInit();
	RngTrack.Init();

	while (1) {
		dPos = pBSet->ExtractFirst();

		pEnt = DEntIndex[dPos];
		RngTrack.Collect(pEnt->dLevel, dPos);

		if (! pEnt->DProcess(pBSet)) {
			// Failure: backtrack
			if (pEnt->backPos < 0)
				break;
			pBSet->Set(pEnt->backPos);
			pBSet->Set(dPos);
		}
		else if (SearchDSuccess()) {
			// Found Solution
			SearchPublish();
			continue;
		}
	}

	std::cout << endl << " SEARCH D CNT " << solveCnt << endl;
	return solveCnt;
}

#ifdef DBGEXPAND
#endif

int NEnv::SearchRed()
{
	level = 0;
	fromAbove = true;
	mode = NLane::MODERESTART;
	RngTrack.Init();

	while (level >= 0) {
		RngTrack.Collect(level);

		if (level >= DbgSt.dumpLevel)
			DbgSt.dumpExpand = 0x2000;
		else
			DbgSt.dumpExpand = 0;

		if (fromAbove) {
			RStk[level].NewVisit();
		} 

		pLaneCur = (NLane::RLevelIsCol(level) ? Cols : Rows) + (level >> 1);
		if (pLaneCur->NextPermRed(mode, fDepth)) {
			// Reduction at level done
			if (level >= redLevel) {
				// Found reduccion: account and climb up
				RStk[level].Success();
				RedPublish();
				SearchUpRedSuccess();
			}
			else {
				// Proceed down: fromAbove = true, level++, MODERESTART
				SearchDnRed();
			}
		}
		else {
			// Current lane is done: fromAbove = false
			if (RStk[level].visits == 0)
				// Lane failed: {level, fDepth} from pLaneCr->pCur, MODEFAILED
				SearchUpRedDone();
			else
				// Lane had success: level--, MODECONT
				SearchUpRedSuccess();
		}
	}

	std::cout << endl << " SEARCH RED CNT " << solveCnt << endl;
	return solveCnt;
}

void NEnv::SearchRedInit(int _redLevel)
{
	redLevel = _redLevel;

	// Init PSets first access
	for (int l = 0; l < n; l++) {
		Cols[l].ringHd.thDnBr = 0;
		Rows[l].ringHd.crDnBr = 0;
	}

	// Init Reduction start and backtrack info
	for (int l = 0; l < n; l++) {
		Cols[l].RedStartFind();
		Rows[l].RedStartFind();
		// Backtrack info depends only on lower level lanes
		Cols[l].RedBackSet(Rows, Cols);
		Rows[l].RedBackSet(Rows, Cols);
	}

	for (int l = 0; l < n * 2; l++)
		RStk[l].Reset();

	// Init PSets first access
	for (int l = 0; l < n; l++) {
		Cols[l].ringHd.dnThru->thUpBr = 0;
		Rows[l].ringHd.dnCross->crUpBr = 0;
	}
}

void NEnv::SearchUpRedSuccess()
{
	// Go back to previous level, continue
	mode = NLane::MODECONT;
	level--;
	fDepth = NLane::FDEPTHNULL;
	fromAbove = false;
	SearchRedDump();
}

void NEnv::SearchUpRedDone()
{
	mode = NLane::MODEFAILED;
	level = pLaneCur->pCur->redBackLvl;
	fDepth = pLaneCur->redDepth;
	pLaneCur->redDepth = 0;
	fromAbove = false;
	SearchRedDump();
}

void NEnv::SearchDnRed()
{
	RStk[level].Success();
	mode = NLane::MODERESTART;
	level++;
	fDepth = NLane::FDEPTHNULL;
	fromAbove = true;
	SearchRedDump();
}

void NEnv::RedPublish()
{
	solveCnt++;
}

void NEnv::SearchRedDump()
{
	if (!DbgSt.dumpExpand)
		return;

	if (fromAbove)
		*DbgSt.pOut << "vvvv DOWN to " << level << endl;
	else if (mode == NLane::MODEFAILED)
		*DbgSt.pOut << "^^^^ UP FAIL " << level << " depth " << fDepth << endl;
	else if(mode == NLane::MODECONT)
		* DbgSt.pOut << "^^^^ UP DONE " << level << endl;
}


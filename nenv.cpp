#include "nlane.h"
#include "nenv.h"

using namespace std;
SolveStack SStk;

// Auxiliary modules
LatSqGen LSGen;

bool EnvBase::Read(string& fname)
{
	IO.ReadOpen(fname);
	for (int i = 0; i < n; i++) {
		IO.ReadLine(Mat[i], 2, n);
	}

	return true;
}

bool EnvBase::Read(char* fname)
{
	IO.ReadOpen(fname);
	for (int i = 0; i < n; i++) {
		IO.ReadLine(Mat[i], 2, n);
	}

	return true;
}

void EnvBase::ReadLineRaw(ifstream &in, u8* pLine, int cnt)
{
	int i;
	int v;
	in >> i;
	for (int p = 0; p < cnt; p++) {
		in >> v;
		pLine[p] = v;
	}
}

void EnvBase::OutLineRaw(ostream& out, u8* pLine, int i, int cnt)
{
	out << setw(2) << i << " ";
	for (int j = 0; j < cnt; j++) {
		out << setw(3) << (int)pLine[j];
	}
	out << endl;
}

void EnvBase::SortMat(string& sortRows, string& sortCols)
{
	if (colsSorted = (sortCols=="up" || sortCols=="down"))
		SortRaw(false, ColsPermCnt, ColUnsort, sortCols == "up");

	if (rowsSorted = (sortRows == "up" || sortRows == "down"))
		SortRaw(true, RowsPermCnt, RowUnsort, sortRows == "up");
}

bool EnvBase::CopyRaw(EnvBase& Src, string& sortRows, string& sortCols)
{
	n = Src.n;
	Mat = Src.Mat;

	for (int l = 0; l < n; l++) {
		ColsPermCnt[l] = Src.ColsPermCnt[l];
		RowsPermCnt[l] = Src.RowsPermCnt[l];
	}

	SortMat(sortRows, sortCols);
	return true;
}

bool EnvBase::InputRaw(string& fname, string sortCols)
{
	ifstream in(fname);
	if (!in.is_open())
		return false;
	in >> n;

	for (int i = 0; i < n; i++) {
		ReadLineRaw(in, Mat[i], n);
	}

	for (int j = 0; j < n; j++)
		in >> ColsPermCnt[j];

	in.close();

	string noSort("");
	SortMat(noSort, sortCols);

	return true;
}

void EnvBase::RandGenRaw(int n, int blkCnt, int seed)
{
	if (seed > 0)
		LSGen.Seed(seed);
	LSGen.Gen(n);
	LSGen.Blank(blkCnt);
	ImportRaw(LSGen.blk);
	SortNone();
}

void EnvBase::ImportRaw(u8 Src[FASTMATDIM][FASTMATDIM])
{
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			Mat[i][j] = Src[i][j];
}

bool SortCompUp(u32 a, u32 b) { return a <= b; }
bool SortCompDn(u32 a, u32 b) { return a >= b; }

bool EnvBase::ShuffleMats(FastMatrix<FASTMATDIMLOG, u8>& M, bool shuffleRows, s8 shufTab[])
{
	FastMatrix<FASTMATDIMLOG, u8> MatAux;
	MatAux = M;

	// Recopy lanes in sort order
	if (shuffleRows) {
		for (int i = 0; i < n; i++) {
			int iSrc = shufTab[i];
			for (int j = 0; j < n; j++)
				M[i][j] = MatAux[iSrc][j];
		}
	}
	else {
		for (int j = 0; j < n; j++) {
			int jSrc = shufTab[j];
			for (int i = 0; i < n; i++)
				M[i][j] = MatAux[i][jSrc];
		}
	}

	return true;
}

bool EnvBase::SortRaw(bool sortRows, int srcCnt[], s8 unsort[], bool ascending)
{
	u32 SortTab[DIMMAX];
	s8 shufTab[DIMMAX];
	FastMatrix<FASTMATDIMLOG, u8> MatAux;
	MatAux = Mat;

	// Create sorting keys
	for (int l = 0; l < n; l++)
		SortTab[l] = (srcCnt[l] << 8) | l;

	// Sort keys
	if (ascending)
		std::sort(SortTab, SortTab + n, SortCompUp);
	else
		std::sort(SortTab, SortTab + n, SortCompDn);

	for (int l = 0; l < n; l++)
		shufTab[l] = SortTab[l] & 0xFF;

	// Shuffle lanes
	ShuffleMats(Mat, sortRows, shufTab);

	for (int l = 0; l < n; l++) {
		unsort[shufTab[l]] = l;
	}

	return true;
}

bool EnvBase::WriteRaw(string& fname)
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

// Static Histo scan
//  Eliminates single value blanks
//  Recomputes lanes absMiss and missCnt
//  Refills lanes VDict
//
void EnvBase::HistoScan()
{
	while (NHisto.Fill() > 0)
		;

	// Fill VDict for all full lanes
    // Compute total blank count
    blankCnt = 0;
	for (int l = 0; l < n; l++) {
		RowFull[l]->AbsToLocal();
		ColFull[l]->AbsToLocal();
		blankCnt += ColFull[l]->missCnt;
	}
}

// Create all lane fulls
//  Fill *Full indices
//
void EnvBase::CreateFull()
{
	LaneFull* pRowFull = new LaneFull[n];
	LaneFull* pColFull = new LaneFull[n];

	for (int l = 0; l < n; l++) {
		RowFull[l] = pRowFull + l;
		ColFull[l] = pColFull + l;
	}
}

// Compute absMiss and missCnt for all full lanes
// Compute blankCnt (# of blank entries in square
//
void EnvBase::BuildFull()
{
	blankCnt = 0;

	Mask all = Set::BitMask(n) - 1;
	for (int l = 0; l < n; l++) {
		RowFull[l]->Init(n, l, true,  all, n);
		ColFull[l]->Init(n, l, false, all, n);
	}
	u8 v;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			v = Mat(i, j);
			if (v < n) {
				RowFull[i]->Present(v);
				ColFull[j]->Present(v);
			}
			else
				blankCnt++;
		}
	}

	for (int l = 0; l < n; l++) {
		RowFull[l]->Check();
		ColFull[l]->Check();
	}
}

bool EnvBase::VerifyLatSq(FastMatrix<FASTMATDIMLOG, u8>& M)
{
	Set lane;
	Mask all = (1LL << n) - 1;
	bool success = true;

	for (s8 i = 0; i < n; i++) {
		// Verify Row i
		lane = 0;
		for (s8 j = 0; j < n; j++)
			lane += M[i][j];
		if (lane != all) {
			Report(" BAD ROW i ", i);
			success = false;
		}
	}
		
	for (s8 j = 0; j < n; j++) {
		// Verify Col j
		lane = 0;
		for (s8 i = 0; i < n; i++) 
			lane += M[i][j];
		if (lane != all) {
			Report(" BAD COL j ", j);
			success = false;
		}
	}
	return success;
}

bool EnvBase::DParsSet(int par, int parVal)
{
	if (par < 0 || par >= DPARMAX)
		return false;
	DPars[par] = parVal;
	return true;
}


void EnvBase::Dump(ostream& out, int dumpType)
{
	switch (dumpType) {
	case DUMPALL:
		//Dump(out, false, false);
		break;

	case DUMPRSLT:
		DumpMat(out, RMat);
		break;

	case DUMPMAT:
		DumpMat(out, Mat);
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


void Histo<LaneFull>::Start()
{
	cnt = 0;
	for (int v = 0; v < n; v++) {
		VCnt[v] = 0;
		LastCol[v] = 0;
	}
}

void Histo<LaneFull>::Add(Set s, int idx)
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

int Histo<LaneFull>::Scan(int j)
{
	int scanCnt = 0;
	Mask vMask;
	int fixCnt = 0;
	for (int v = 0; v < n; v++) {
		scanCnt += VCnt[v];
		vMask = Set::BitMask(v);
		if (ColTab[j]->absMiss.Contains(vMask)) {
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
				ColTab[j]->absMiss -= vMask;
				ColTab[j]->missCnt--;
				if (RowTab[i]->absMiss.Contains(vMask)) {
					RowTab[i]->absMiss -= vMask;
					RowTab[i]->missCnt--;
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

template<class LaneFull> int Histo<LaneFull>::Fill()
{
	if (DbgSt.reportCreate)
		Report("\n HISTO FILL \n");

	int fixCnt = 0;
	for (int j = 0; j < n; j++) {
		Start();
		for (int i = 0; i < n; i++) {
			if ((*pMat)[i][j] > n) {
				// Blank entry
				Add(RowTab[i]->absMiss & ColTab[j]->absMiss, i);
			}
		}
		fixCnt += Scan(j);
	}
	return fixCnt;
}

bool NEnv::ReadRaw(string& fname, string sortMode)
{
	InputRaw(fname, sortMode);
	Init();
	ProcessMat();
	return true;
}

void NEnv::ProcessMat()
{
	// Fill Full Indices
	for (int l = 0; l < n; l++) {
		RowFull[l] = (LaneFull*)(Rows + l);
		ColFull[l] = (LaneFull*)(Cols + l);
	}

	BuildFull();
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
		ColsPermCnt[j] = Cols[j].PSet.Counts.permCnt;
	}

	// Fill Rows (cross direction)
	for (int i = 0; i < n; i++) {
		Rows[i].FillBlanks(Cols);
	}

	LinkBlanks();
}

void NEnv::FillEnts()
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
		Cols[j].FillBlanks(Rows, false);
		blankCnt += Cols[j].blankCnt;
		ColsPermCnt[j] = Cols[j].PSet.Counts.permCnt;
	}

	// Fill Rows (cross direction)
	for (int i = 0; i < n; i++) {
		Rows[i].FillBlanks(Cols, false);
	}

	LinkBlanks();
}

void EnvBase::GenPerms(int kind, bool doRow, int idxFrom, int idxTo, bool doCheck)
{
	LaneFull* pLane;

	for (int idx = idxFrom; idx < idxTo; idx++) {
		pLane = (doRow ? RowFull : ColFull)[idx];
		switch (kind) {
		case GENREF:
			pLane->PermRefCreate();
			break;

		case GENEXP:
			pLane->GenExpSet();
			if (doCheck)
				pLane->CheckExpSet();
			break;

		case GENLVL:
			pLane->GenPSet();
			if (doCheck)
				pLane->CheckPSet();
			break;

		case GENALL:
			pLane->PermRefCreate();
			pLane->GenExpSet();
			pLane->GenPSet();
			if (doCheck) {
				pLane->CheckExpSet();
				pLane->CheckPSet();
			}
			break;

		default:
			assert(0);
		}
	}
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

void EnvBase::DumpMat(ostream& out, FastMatrix<FASTMATDIMLOG, u8>& DMat)
{
	u8 buf[DIMMAX];
	IO.OutSet(out);
	IO.OutHeader(n);
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			buf[j] = DMat[i][j];
		}
		IO.OutLineV(buf, i, n);
	}
}

// DPars[0]: isCol
// DPars[1]: idx
//		if (DPars[1] < 0) Print full set from 0 to n
// DPars[2]: lvlFr
// DPars[3]: lvlTo
//
void EnvBase::DumpPSet(ostream& out)
{
	LaneFull* pLane;
	if (DPars[1] >= n)
		return;

	int l0, l1;
	if (DPars[1] < 0) {
		l0 = 0;
		l1 = n - 1;
	}
	else {
		l0 = DPars[1];
		l1 = DPars[1];
	}

	for (int l = l0; l <= l1; l++) {
		pLane = DPars[0] ? ColFull[l] : RowFull[l];
		pLane->PSet.DumpLvls(out, DPars[2], DPars[3]);
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
void NEnv::DumpLane(ostream& out)
{
	NLane* pLane;
	string kind;
	if (DPars[1] >= n)
		return;
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
}

// DPars[0]: isCol
// DPars[1]: idx
// DPars[2]: lvlFr
// DPars[3]: lvlTo
//
void NEnv::DumpPSet(ostream& out)
{
	NLane* pLane;
	if (DPars[1] >= n || DPars[1] < 0)
		return;
	pLane = DPars[0] ? Cols : Rows;
	pLane += DPars[1];
	pLane->PSet.DumpLvls(out, DPars[2], DPars[3]);
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


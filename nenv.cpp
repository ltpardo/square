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
	for (int j = 0; j < n; j++) {
		Cols[j].FillBlanks(Rows);
		ColsPermCnt[j] = Cols[j].PSet.permCnt;
		if (Cols[j].PSet.OutOflo())
			Report(" OOOOFFFFLLLLOOO  COL ", j);
		if (Cols[j].PSet.badCounts)
			Report(" BAD COUNTS COL ", j);
	}

	// Fill Rows (cross direction)
	for (int i = 0; i < n; i++) {
		Rows[i].FillBlanks(Cols);
		if (Rows[i].PSet.OutOflo())
			Report(" OOOOFFFFLLLLOOO  ROW ", i);
		if (Rows[i].PSet.badCounts)
			Report(" BAD COUNTS ROW ", i);
	}

	LinkBlanks();
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
	RangeInit();

	while (colLevel >= 0) {
		RangeColl();
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

	IO.OutHeader(out, n);
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			buf[j] = Mat[i][j];
		}
		IO.OutLineV(out, buf, i, n);
	}
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

	case DUMPLANES:
		DumpLane(out);
		break;

	case DUMPRINGS:
		//DumpRings(out);
		break;

	case DUMPPSET:
		DumpPSet(out);
		break;

	default:
		break;
	}
}

#if 0
void NEnv::SolveInit(int lvlFr)
{
	pColCur = LaneTab[lvlFr].pLane;
	pColCur->PermuteStart();
	for (int s = 0; s < n; s++) {
		LaneTab[s].pLane->St.Reset();
	}

	// Init laneMiss for all rows
	BEntry* pEnt;
	for (int i = 0; i < n; i++) {
		pEnt = Rows[i].ringHd.rtLink;
		if (!pEnt->IsHead())
			pEnt->laneMiss = Rows[i].missing;
	}

	for (colLevel = lvlFr; colLevel <= n - 1; colLevel++) {
		pColCur = LaneTab[colLevel].pLane;
		pColCur->counting = false;
	}

	solveCnt = 0;
}

int NEnv::Solve()
{
	int depth;
	// SolveInit();

	for (colLevel = 0; colLevel >= 0; ) {
		if (pColCur->PermuteStep() > 0) {
			// Successful step
			if (colLevel >= n - 1) {
				// At bottom level: a solution has been found
				// PublishSolution();
			}
			else {
				// Continue search down next level
				if ((depth = RestrictRows(pColCur)) < 0) {
					pColCur = Cols + (++colLevel);
					pColCur->PermuteStart();
				}
				else {
					// Restart permute at failure depth
					SolveFailed(depth);
				}
			}
		}
		else {
			// Level has been exhausted
			if (colLevel > 0) {
				// Not on top: no more permutations at this level
				pColCur = Cols + (--colLevel);
			}
			else
				// At level 0: solve is done
				break;
		}
	}

	return solveCnt;
}

void NEnv::SolveFailed(int depth)
{
	//pColCur->PermuteContinueAt(depth);
}


// Restricts next level rows starting at pCol->firstPermLvl
//       Returns failure level or -1 if success
//
int NEnv::RestrictRows(NLane* pCol)
{
	NEntry* pBlank;
	NEntry* pNext;

	// Restriction begins at first permutation level
	int d = pCol->firstPermLvl;
	for (; d < pCol->missCnt; d++) {
		pBlank = pCol->NStack[d].pEnt;
		pNext = pBlank->dnCross;
#if 0
		if (!pNext->IsHead()) {
			// Update Downstream
			pNext->laneMiss = pBlank->laneMiss & (~pBlank->chosenBit);
			pNext->miss = pNext->laneMiss & Cols[pNext->j].absMiss;

			if (pNext->miss == 0) {
				// This won't do: pNext is unviable
				return d;
			}

			Set laneMask = pNext->laneMiss;
			if (pNext->miss.CountIsOne())
				laneMask -= pNext->miss;
		}
#endif
	}

	// Restriction was succesful
	return -1;
}
#endif


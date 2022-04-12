#include "senv.h"

using namespace std;
#if 0
void SLane::Init(int _n, int _idx) {
	n = _n;
	idx = _idx;
	assert(n < DIMMAX - 2);
	all = Set::BitMask(n) - 1;
	missing = all;
	present = 0;
	presCnt = 0;
	missCnt = n;
	stops = 0;
	blankCnt = 0;
	ringHd.SelfLink();

	St.InitPass();
	St.starts = 0;
}

void SLane::Present(u8 v) {
	if (v >= n)
		return;
	presCnt++;
	present |= Set::BitMask(v);

	missCnt--;
	missing &= ~Set::BitMask(v);
}

void SLane::Missing(u8 v) {
	if (v < n)
		return;
	missCnt++;
	missing &= ~(Set::BitMask(v));
}

bool SLane::Check() {
	if (presCnt + missCnt != n)
		return false;
	if ((present | missing) != all)
		return false;
	return true;
}

void SLane::AddBlank(int i, int j)
{
	pBlank = new BEntry();
	BStack[blankCnt] = pBlank;
	pBlank->SetCoords(i, j);
	pBlank->SetLevel(blankCnt);
	blankCnt++;
}


bool SLane::PermuteStartStk()
{
	foundCnt = 0;

	// First level has 
	level = 0;
	pBlank = BStack[0];
	pBlank->selectedSet = 0;			// No selected set
	pBlank->unSelected = missing;	// All missing vals are needed
	pBlank->choiceSet = pBlank->miss;	// Choose from all available candidates

	St.InitPass();

	return true;
}

bool SLane::PermuteStepStk()
{
	St.Step();

	while (level >= 0) {
		// Entering level
		if (pBlank->choiceSet == 0) {				// Exhausted branch, or
			// Climb up
			pBlank = BStack[--level];
			St.Exhaust();
		}
		else if ((pBlank->unSelected.mask
			!= (pBlank->availForSelect
				& pBlank->unSelected))) {	// Rest of permutation unviable
			// Climb up
			pBlank = BStack[--level];
			St.Select();
		}
		else {
			// Extract candidate from choiceSet
			pBlank->chosenBit = pBlank->choiceSet.ExtractFirst();

			// Branch has a candidate
			if (level == missCnt - 1) {
				// Bottom branch: found one
				assert(pBlank->choiceSet == 0);		//   Only one candidate should remain...
				foundCnt++;

				// Climb up
				pBlank = BStack[--level];
				return true;
			}
			else {
				// Not bottom branch: add chosenBit to selected set for next level
				Mask chosen = pBlank->chosenBit;
				Mask nextselSet = pBlank->selectedSet | chosen;
				// ... and proceed down
				pBlank = BStack[++level];
				pBlank->selectedSet = nextselSet;
				pBlank->unSelected = missing - pBlank->selectedSet;
				pBlank->choiceSet = pBlank->miss & pBlank->unSelected;
			}
		}
	}

	// Iteration finished
	St.End();
	return false;
}

bool SLane::PermuteScrub(SLane* pClog, SLane* Rows)
{
	int i;
	// Start at Bottom of BStack
	for (level = missCnt - 1; level >= 0;) {
		pBlank = BStack[level];
		i = pBlank->i;

		if (Rows[i].Reaches(pClog->idx)) {
			if ((pBlank->laneMiss & pClog->needSet) != 0) {
				// Found a possible clog: should be the chosenBit
				if (pClog->needSet.Contains(pBlank->chosenBit)) {
					pClog->needSet -= pBlank->chosenBit;
					// Scrub will end for needSet exhausted
					if (pClog->needSet == 0)
						return true;
				}
			}
		}

		// No reach: move up
		level--;
	}

	// Level exhausted
	return false;
}

u64 SLane::PermuteCount()
{
	PermuteStart();

	while (PermuteStep() > 0)
		;

	permuteCnt = (int)foundCnt;
	return foundCnt;
}

bool SLane::VerifyPerm()
{
	bool success = true;
	Set perm = 0;
	for (int d = 0; d < missCnt; d++) {
		perm += BStack[d]->chosenBit;
		if ((BStack[d]->miss & BStack[d]->chosenBit) == 0)
			success = false;
	}
	if (perm != missing)
		success = false;

	return success;
}
#endif

#if 0
#if 0
u8 Histo[DIMMAX];
u8 HistoLastCol[DIMMAX];
int histoCnt;
void HistoInit();
void HistoAdd(Set s, int idx);
void HistoScan(SLane*& Rows, FastMatrix<6, u8>& Mat);
#endif

void SLane::HistoInit()
{
	histoCnt = 0;
	for (int v = 0; v < n; v++) {
		Histo[v] = 0;
		HistoLastCol[v] = 0;
	}
}

void SLane::HistoAdd(Set s, int idx)
{
	Mask m = 1;
	for (int v = 0; v < n; v++) {
		if (s.Contains(m)) {
			Histo[v]++;
			HistoLastCol[v] = idx;
			histoCnt++;
		}
		m <<= 1;
	}
}

void SLane::HistoScan(SLane*& Rows, FastMatrix<6, u8>& Mat)
{
	int scanCnt = 0;
	Mask vMask;
	cout << "    Scan col [" << setw(2) << idx << "] ";
	for (int v = 0; v < n; v++) {
		scanCnt += Histo[v];
		vMask = Set::BitMask(v);
		if (missing.Contains(vMask)) {
			assert(Histo[v] > 0);
			if (Histo[v] <= 1) {
				// Forced value
				cout << setw(3) << v;

				// Change Mat and SLanes
				Mat[HistoLastCol[v]][idx] = v;
				missing -= vMask;
				for (level = 0; level < blankCnt; level++) {
					pBlank = BStack[level];
					if (Rows[pBlank->i].missing.Contains(vMask)) {
						Rows[pBlank->i].missing -= vMask;
					}
				}
			}
		}
	}
	if (scanCnt != histoCnt)
		cout << " CNT ERROR  histo " << histoCnt << " != scan " << scanCnt;
	cout << endl;
}

//   Fill initial values:
//		pBlank->laneMiss	with row missing set
//		pBlank->miss		with intersection row and column missing sets
void SLane::FillBlanks(SLane* Rows)
{
	for (level = 0; level < blankCnt; level++) {
		pBlank = BStack[level];
		pBlank->miss = missing & Rows[pBlank->i].missing;
		pBlank->laneMiss = Rows[pBlank->i].missing;
	}

	ComputeAvail();
}

// Insert blanks into corresponding row ring
void SLane::LinkBlanks(SLane* Rows)
{
	for (int d = 0; d < blankCnt; d++) {
		pBlank = BStack[d];
		pBlank->LinkLeftOf(&Rows[pBlank->i].ringHd);
		Rows[pBlank->i].stops += Set::BitMask(pBlank->j);
	}
}

bool SolveEnv::Read(string& fname)
{
	IO.ReadOpen(fname);
	for (int i = 0; i < n; i++) {
		IO.ReadLine(Mat[i], 2, n);
	}

	return true;
}

bool SolveEnv::Read(char* fname)
{
	IO.ReadOpen(fname);
	for (int i = 0; i < n; i++) {
		IO.ReadLine(Mat[i], 2, n);
	}

	return true;
}

void SolveEnv::DumpMat(ostream& out, bool doSort)
{
	u8 buf[DIMMAX];
	for (int j = 0; j < n; j++)
		buf[j] = LaneTab[j].idx;
	IO.OutHeader(out, n, buf);

	IO.OutHeader(out, n);
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			buf[j] = doSort ? Mat[i][LaneTab[j].idx] : Mat[i][j];
		}
		IO.OutLineV(out, buf, i, n);
	}
}

void SolveEnv::DumpRings(ostream& out)
{
	out << endl << "  Rings" << endl;
	BEntry* pEnt;
	for (int i = 0; i < n; i++) {
		out << "[" << setw(2) << i << "] ";
		for (pEnt = Rows[i].ringHd.rtLink; !pEnt->IsHead(); pEnt = pEnt->rtLink) {
			out << setw(3) << pEnt->j;
			if (pEnt->i != i) {
				out << endl << " RINGS NOT SET" << endl;
				return;
			}
		}
		out << endl;
	}
}

void SolveEnv::DumpLanes(ostream& out)
{
	out << endl << "  Cols / count" << endl;
	for (int j = 0; j < n; j++) {
		out << " Col " << setw(2) << LaneTab[j].idx
			<< " PermCnt  " << setw(5) << LaneTab[j].permCnt << endl;
	}
}


bool SolveEnv::Dump(ostream& out, bool doSort, bool doRings)
{
	DumpMat(out, doSort);
	if (doRings)
		DumpRings(out);
	return true;
}

void SolveEnv::ProcessMat()
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

void SolveEnv::FillBlanks()
{
	// Create BStacks
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (Mat(i, j) > n) {
				Rows[i].AddBlank(i, j);
				Cols[j].AddBlank(i, j);
			}
		}
	}

	// Init ringHd for Rows
	for (int i = 0; i < n; i++) {
		Rows[i].ringHd.laneMiss = Rows[i].missing;
		Rows[i].ringHd.SetAsHead();
	}

	for (int j = 0; j < n; j++) {
		Cols[j].FillBlanks(Rows);
	}

	for (int j = 0; j < n; j++) {
		LaneTab[j].permCnt = Cols[j].PermuteCount();
		LaneTab[j].pLane = &Cols[j];
		LaneTab[j].idx = j;
	}
}

// Link Cols based on sorted LaneTab
void SolveEnv::LinkBlanks()
{
	for (int l = 0; l < n; l++) {
		 LaneTab[l].pLane->LinkBlanks(Rows);
	}
}

bool LaneCompUp(LaneRank<SLane> a, LaneRank<SLane> b)
{
	return a.permCnt < b.permCnt;
}
bool LaneCompDown(LaneRank<SLane> a, LaneRank<SLane> b)
{
	return a.permCnt > b.permCnt;
}

void SolveEnv::SortTab(int order)
{
	switch (order) {
	case SORTUP:
		std::sort(LaneTab, LaneTab + n, LaneCompUp);
		break;

	case SORTDOWN:
		std::sort(LaneTab, LaneTab + n, LaneCompDown);
		break;

	default:
		break;
	}
}

void SolveEnv::SortCols(ostream& out, bool doSort)
{
	for (int j = 0; j < n; j++) {
		LaneTab[j].permCnt = Cols[j].PermuteCount();
		LaneTab[j].pLane = &Cols[j];
		LaneTab[j].idx = j;
		out << " Col " << j << " PermCnt  " << LaneTab[j].permCnt << endl;
	}

	if (doSort) {
		std::sort(LaneTab, LaneTab + n, LaneCompUp);

		out << endl;
		for (int j = 0; j < n; j++) {
			out << " Sorted " << setw(2) << LaneTab[j].idx
				<< " PermCnt  " << setw(5) << LaneTab[j].permCnt << endl;
		}
	}
	else {
		out << endl << " NO SORT" << endl;
	}
}

void SolveEnv::HistoInit()
{
	histoCnt = 0;
	for (int v = 0; v < n; v++) {
		Histo[v] = 0;
		HistoLastCol[v] = 0;
	}
}

void SolveEnv::HistoAdd(Set s, int idx)
{
	Mask m = 1;
	for (int v = 0; v < n; v++) {
		if (s.Contains(m)) {
			Histo[v]++;
			HistoLastCol[v] = idx;
			histoCnt++;
		}
		m <<= 1;
	}
}

int SolveEnv::HistoScan(ostream& out, int j)
{
	return HistoScan(j);
}

int SolveEnv::HistoScan(int j)
{
	int scanCnt = 0;
	Mask vMask;
	int fixCnt = 0;
	for (int v = 0; v < n; v++) {
		scanCnt += Histo[v];
		vMask = Set::BitMask(v);
		if (Cols[j].missing.Contains(vMask)) {
			assert(Histo[v] > 0);
			if (Histo[v] <= 1) {
				// Forced value
				int i = HistoLastCol[v];
				if (fixCnt == 0) {
					Report("    Scan col ", j);
				}
				fixCnt++;
				Report(" row/v ", i, v);

				// Change Mat and SLanes
				Mat[i][j] = v;
				Cols[j].missing -= vMask;
				Cols[j].missCnt--;
				if (Rows[i].missing.Contains(vMask)) {
					Rows[i].missing -= vMask;
					Rows[i].missCnt--;
				}
			}
		}
	}
	if (scanCnt != histoCnt) {
		Report(" CNT ERROR  histo/scan ", histoCnt, scanCnt);
		fixCnt = -1;
	}

	if (fixCnt != 0)
		Report("\n");

	return fixCnt;
}

int SolveEnv::HistoFill(ostream& out)
{
	return HistoFill();
}

int SolveEnv::HistoFill()
{
	Report("\n FILL \n");

	int fixCnt = 0;
	for (int j = 0; j < n; j++) {
		HistoInit();
		for (int i = 0; i < n; i++) {
			if (Mat[i][j] > n) {
				// Blank entry
				HistoAdd(Rows[i].missing & Cols[j].missing, i);
			}
		}
		fixCnt += HistoScan(j);
	}
	return fixCnt;
}

bool SolveEnv::UpdateEntries(SLane* pCol)
{
	int zzz = 0;
	int badCnt = 0;
	Set allMask = 0;
	pCol->needCnt = 0;
	pCol->needSet = 0;

	// Update blanks for next column
	BEntry* pBlank;
	for (int d = 0; d < pCol->missCnt; d++) {
		pBlank = pCol->BStack[d];
		pBlank->miss =
			pBlank->laneMiss & pCol->missing;
		if (pBlank->miss == 0) {
			// Will take any of the original missings
			pCol->needSet |=
				(Rows[pBlank->i].missing & pCol->missing);
			pCol->needLevels[pCol->needCnt++] = d;
			badCnt++;
		}
		allMask |= pBlank->laneMiss;
	}

	if (pCol->needSet == 0) {
		// Look for Col wide needs
		pCol->needSet = allMask & pCol->missing;
		if (pCol->needSet != pCol->missing) {
			pCol->needSet ^= pCol->missing;
			badCnt += pCol->needSet.Count();
		}
		else {
			pCol->needSet = 0;
			pCol->needCnt = 0;
		}
	}

	if (badCnt > 1) {
		zzz += 1;
	}

	return pCol->needCnt <= 0 && pCol->needSet == 0;
}

void SolveEnv::RestoreRows(SLane* pCol)
{
	BEntry* pBlank;
	for (int d = 0; d < pCol->missCnt; d++, pBlank++) {
		pBlank = pCol->BStack[d];
		Rows[pBlank->i].missing |= pBlank->chosenBit;
	}
}

int SolveEnv::BackRow(int d)
{
	BEntry* pBlank;
	for (pBlank = pClog->BStack[d]; !pBlank->IsHead(); pBlank = pBlank->lfLink) {
		if (pClog->needSet.Contains(pBlank->chosenBit)) {
			pClog->needSet -= pBlank->chosenBit;
			return pBlank->level;
		}
	}
	return -1;
}

void SolveEnv::BacktrackRows()
{
	int d;
	BEntry* pEnt;
	BEntry* pPrev;
	for (int l = 0; l < pClog->needCnt; l ++) {
		d = pClog->needLevels[l];
		pEnt = pClog->BStack[d];
		for (;;) {
		}
		pPrev = pEnt;
	}
}

void SolveEnv::Backtrack()
{
	pColCur = LaneTab[--colLevel].pLane;
	while (colLevel >= 0) {
		if ((pColCur->missing & pClog->needSet) == 0) {
			if (colLevel > 0) {
				// This level is not involved in the clogging: skip it
				pColCur = LaneTab[--colLevel].pLane;
			}
			else
				return;
		}
		else {
			// Involved level: scrub Permute stack
			if (pColCur->PermuteScrub(pClog, Rows)) {
				// Scrub found a needed value
				return;
			}
			else {
				if (colLevel > 0) {
					// Level scrubbed, nothing found, so climb up
					pColCur = LaneTab[--colLevel].pLane;
				}
				else
					// At level 0: cannot backtrack any more
					return;
			}
		}
	}
}
#endif

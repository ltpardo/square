#include "stdinc.h"
#include "latsq.h"
#include "senv.h"

using namespace std;

bool SolveEnv::PublishSolution()
{
	bool success = true;
	for (int j = 0; j < n; j++) {
		if (!Cols[j].VerifyPerm())
			success = false;
	}
	solveCnt++;
	cout << " SOLUTION # " << solveCnt << endl;
	return success;
}

bool SLane::PermuteStart()
{
	foundCnt = 0;

	// First level has 
	level = rngFrom;
	pBlank = BStack[level];
	pBlank->selectedSet = preSelected;			// CountOne selected set
	pBlank->unSelected = missing - preSelected;	// All missing vals are needed
	pBlank->choiceSet = pBlank->miss - preSelected;	// Choose from all available candidates

	St.InitPass();

	return true;
}

// Set stack range
//  Returns false if range has no entries
bool SLane::SetCntRange(int iFrom, int iTo)
{
	counting = true;
	preSelected = 0;

	BEntry* pEnt;
	int d;
	int up = BEntry::TOP;
	rngFrom = -1;
	rngTo = -2;

	pEnt = NULL;
	for (d = 0; d < missCnt; d++) {
		pEnt = BStack[d];
		pEnt->changeStkPos = ChangeStack::UNSTACKED;
		if (pEnt->i >= iFrom && pEnt->i <= iTo) {
			if (rngFrom < 0)
				rngFrom = d;
			pEnt->upTrav = up;
			pEnt->dnTrav = d + 1;
			pEnt->availForSelect = missing;
			up = d;
		}
		if (pEnt->i >= iTo) {
			break;
		}
	}

	if (rngFrom < 0) {
		return false;
	}
	else {
		rngTo = d;
		pEnt->dnTrav = BEntry::BOT;
		return true;
	}
}

bool SLane::CheckAvail()
{
	BEntry* pEnt;
	Set avail = 0;
	preSelected = 0;

	for (int d = missCnt - 1; d >= 0; d--) {
		pEnt = BStack[d];
		if (pEnt->miss == 0)
			return false;
		if (pEnt->miss.CountIsOne()) {
			if (preSelected.Contains(pEnt->miss.mask)) {
				// Multiple forced single selections of same value: unviable
				return false;
			}
			preSelected += pEnt->miss;
		}
		else
			avail += pEnt->miss;
	}
	needSet = missing - preSelected;
	if (!avail.Contains(needSet))
		return false;
	else
		return true;
}

int SLane::ComputeAvail()
{
	BEntry* pEnt, * pDn;
	Set avail = 0;
	int dn = BEntry::BOT;
	pDn = NULL;
	preSelected = 0;

	for (int d = missCnt - 1; d >= 0; d--) {
		pEnt = BStack[d];
		pEnt->dnTrav = dn;

		if (pEnt->miss.CountIsOne()) {
			if (preSelected.Contains(pEnt->miss.mask)) {
				// Multiple forced single selections of same value: unviable
				return d;
			}

			pEnt->chosenBit = pEnt->miss.mask;
			preSelected += pEnt->chosenBit;
			pEnt->upTrav = d - 1;
		}
		else {
			if (pDn != NULL)
				pDn->upTrav = d;
			avail += pEnt->miss;
			pEnt->availForSelect = avail;
			pDn = pEnt;
			dn = d;
		}
	}
	if (pDn != NULL)
		pDn->upTrav = BEntry::TOP;
	else
		assert(0);
	rngFrom = dn;
	return -1;
}

void SLane::PermuteContinueAt(int depth)
{
	level = depth;
	pBlank = BStack[level];
}

// Computes next permutation in column
//   Returns false if column has exhausted all permutations
//   firstPerLvl is level of first change from previous permutation (-1 if exhausted)
//
int SLane::PermuteStep()
{
	int found;
	firstPermLvl = level;
	St.Step();

	while (level >= 0) {
		// Entering level
		if (pBlank->choiceSet == 0) {				// Exhausted branch, or
			// Climb up
			level = pBlank->upTrav;
			pBlank = BStack[level];
			St.Exhaust();
		}
		else if ((pBlank->unSelected.mask
			!= (pBlank->availForSelect
				& pBlank->unSelected))) {	// Rest of permutation unviable
			// Climb up
			level = pBlank->upTrav;
			pBlank = BStack[level];
			St.Select();
		}
		else {
			// Extract candidate from choiceSet
			pBlank->chosenBit = pBlank->choiceSet.ExtractFirst();

			// Update firstLvl
			if (firstPermLvl > level)
				firstPermLvl = level;

			// Branch has a candidate
			if (pBlank->dnTrav == BEntry::BOT) {
				// Bottom branch: found a permutation
				assert(counting || pBlank->choiceSet == 0);		//   Only one candidate should remain...
				found = pBlank->choiceSet.Count() + 1;
				foundCnt += found;

				// Climb up
				level = pBlank->upTrav;
				pBlank = BStack[level];
				return found;
			}
			else {
				// Not bottom branch: add chosenBit to selected set for next level
				Mask chosen = pBlank->chosenBit;
				Mask nextselSet = pBlank->selectedSet | chosen;
				// ... and proceed down
				level = pBlank->dnTrav;
				pBlank = BStack[level];
				pBlank->selectedSet = nextselSet;
				pBlank->unSelected = missing - pBlank->selectedSet;
				pBlank->choiceSet = pBlank->miss & pBlank->unSelected;
			}
		}
	}

	// No more permutations for this pass
	St.End();
	firstPermLvl = -1;
	return 0;
}

void SolveEnv::SolveInit(int lvlFr)
{
	pColCur = LaneTab[lvlFr].pLane;
	pColCur->PermuteStart();
	for (int s = 0; s < n; s++) {
		FailCnt[s] = 0;
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

void SolveEnv::SolveCntInit(int lvlFrom, int lvlTo, int iFrom, int iTo)
{
	for (int s = 0; s < n; s++) {
		FailCnt[s] = 0;
		LaneTab[s].pLane->St.Reset();
	}

	// Init laneMiss for all rows in [iFrom, iTo]
	BEntry* pEnt;
	for (int i = iFrom; i < iTo; i++) {
		for (pEnt = Rows[i].ringHd.rtLink; !pEnt->IsHead(); pEnt = pEnt->rtLink)
			pEnt->laneMiss = Rows[i].missing;
	}

	for (colLevel = lvlFrom; colLevel <= lvlTo; colLevel++) {
		pColCur = LaneTab[colLevel].pLane;
		pColCur->SetCntRange(iFrom, iTo);
	}

	pColCur = LaneTab[lvlFrom].pLane;
	pColCur->PermuteStart();

	solveCnt = 0;
}

bool SolveEnv::SpyPropagate()
{
	int j;
	SLane* pCol;
	while (Spy.Touched() != 0) {
		j = Spy.ExtractColIdx();
		pCol = & Cols[j];
		if (! pCol->CheckAvail()) {
			// pCol is unviable
			return false;
		}
	}
	return true;
}

bool SolveEnv::SpyDownStream(BEntry* pEnt, Set laneMask)
{
	SLane* pCol;

	for (; !pEnt->IsHead(); pEnt = pEnt->rtLink) {
		pCol = &Cols[pEnt->j];
		Spy.Touch(pEnt->j);

		pChgStk->PushAndChange(pEnt, laneMask.mask, pCol->missing & laneMask);

		if (pEnt->miss == 0)
			// Unviable
			return false;

		if (pEnt->miss.CountIsOne())
			// Only one choice: restrict laneMask to downstream entries
			laneMask -= pEnt->miss;
	}

	return true;
}

// Restricts next level rows starting at pCol->firstPermLvl
//       Returns failure level or -1 if success
//
int SolveEnv::RestrictRows(SLane* pCol)
{
	BEntry* pBlank;
	BEntry* pNext;
	Spy.Init();

	// Restriction begins at first permutation level
	int d = pCol->firstPermLvl;
	// Restore things after first step
	if (pCol->foundCnt > 1)
		pChgStk->RestoreFrom(pCol->BStack[d]->changeStkPos);

	for (; d < pCol->missCnt; d++) {
		pBlank = pCol->BStack[d];
		pBlank->changeStkPos = pChgStk->CurFree();
		pNext = pBlank->rtLink;
		if (!pNext->IsHead()) {
			// Update Downstream
			pChgStk->PushAndChange(pNext,
				pBlank->laneMiss & (~pBlank->chosenBit),
				pNext->laneMiss & Cols[pNext->j].missing);

			if (pNext->miss == 0) {
				// This won't do: pNext is unviable
				return d;
			}

			Set laneMask = pNext->laneMiss;
			if (pNext->miss.CountIsOne())
				laneMask -= pNext->miss;
			if (!SpyDownStream(pNext->rtLink, laneMask)) {
				// This won't do: downstream chain unviable
				return d;
			}
		}
	}

	// Restriction was succesful
	return -1;
}

// Restricts next level rows for counting
// starting at pCol->firstPermLvl
void SolveEnv::RestrictRowsCount(SLane* pCol)
{
	BEntry* pBlank;
	BEntry* pNext;

	// Restriction begins at first permutation level
	int d = pCol->firstPermLvl;
	//if (pCol->BStack[d]->changeStkPos != ChangeStack::UNSTACKED)
	pChgStk->RestoreFrom(pCol->BStack[d]->changeStkPos);

	for (; d < pCol->missCnt; d++) {
		pBlank = pCol->BStack[d];
		pBlank->changeStkPos = pChgStk->CurFree();
		pNext = pBlank->rtLink;
		if (!pNext->IsHead()) {
			// Update Downstream
			pChgStk->PushAndChange(pNext,
				pBlank->laneMiss & (~pBlank->chosenBit),
				pNext->laneMiss & Cols[pNext->j].missing);
		}
		if (pBlank->dnTrav == BEntry::BOT)
			break;
	}
}

void SolveEnv::SolveFailed(int depth)
{
	// Restore downstream state to the first change in the permutation
	pChgStk->RestoreFrom(pColCur->BStack[depth]->changeStkPos);
	pColCur->PermuteContinueAt(depth);
}

void SolveEnv::CountSolution(int found)
{
	solveCnt+= found;
}

//////////////////////////////////////////////////// LIMIT PERMUTE 
//  permLvl
//	Last level could be lastI rather than using downTo link
//
int SolveEnv::SolveCnt(int lvlFrom, int lvlTo, int iFrom, int iTo)
{
	int found;
	SLane* pDown;

	SolveCntInit(lvlFrom, lvlTo, iFrom, iTo);

	for (colLevel = lvlFrom; colLevel >= lvlFrom; ) {
		if ((found = pColCur->PermuteStep()) > 0) {
			// Successful step
			if (colLevel >= lvlTo) {
				// At end level: a solution has been found
				CountSolution(found);
			}
			else {
				// Continue search down next level
				RestrictRowsCount(pColCur);
				pDown = LaneTab[++colLevel].pLane;
				pColCur->St.Down();
				pColCur = pDown;
				pColCur->PermuteStart();
			}
		}
		else {
			// Level has been exhausted
			if (colLevel > lvlFrom) {
				// Not on top: no more permutations at this level
				FailCnt[colLevel]++;
				pColCur->St.End();

				// Permute completed
				if (pColCur->foundCnt > 0)
					pChgStk->RestoreFrom(pColCur->BStack[0]->changeStkPos);
				pColCur = LaneTab[--colLevel].pLane;
			}
			else
				// At level 0: solve is done
				break;
		}
	}

	Report(" SolveCnt from lvl/j ", lvlFrom, LaneTab[lvlFrom].pLane->idx);
	Report("            to lvl/j ", lvlTo, LaneTab[lvlTo].pLane->idx);
	Report("                                                = ", solveCnt);

	return solveCnt;
}

int SolveEnv::Solve()
{
	int depth;
	SLane* pDown;

	SolveInit();

	for (colLevel = 0; colLevel >= 0; ) {
		if (pColCur->PermuteStep() > 0) {
			// Successful step
			if (colLevel >= n - 1) {
				// At bottom level: a solution has been found
				PublishSolution();
			}
			else {
				// Continue search down next level
				if ((depth = RestrictRows(pColCur)) < 0) {
					// Restriction was successful: check for further spying
					if (Spy.Touched()) {
						if (!SpyPropagate()) {
							// Failed Propagation: continue at level
							pColCur->St.Prop();
							continue;
						}
					}

					// Spy succeeded: try Down
					pDown = LaneTab[++colLevel].pLane;
					if ((depth = pDown->ComputeAvail()) >= 0) {
						// Down failed in ComputeAvail: restart at failure depth
						SolveFailed(depth);
					}
					else {
						// ComputeAvail succeeded: going down!
						pColCur->St.Down();
						pColCur = pDown;
						pColCur->PermuteStart();
					}
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
				FailCnt[colLevel]++;
				pColCur->St.End();

				// Permute completed
				if (pColCur->foundCnt > 0)
					pChgStk->RestoreFrom(pColCur->BStack[0]->changeStkPos);
				pColCur = LaneTab[--colLevel].pLane;
			}
			else
				// At level 0: solve is done
				break;
		}
	}

	return solveCnt;
}

void SolveEnv::Dump(ostream& out, int dumpType)
{
	switch (dumpType) {
	case DUMPALL:
		Dump(out, false, false);
		break;

	case DUMPMAT:
		DumpMat(out, false);
		break;

	case DUMPSORTED:
		DumpMat(out, true);
		break;

	case DUMPRINGS:
		DumpRings(out);
		break;

	case DUMPLANES:
		DumpLanes(out);
		break;

	default:
		break;
	}
}

SimpleTimer Timer;
LoggerSrv Logger;
DebugState DbgSt;

/////////////////////////////////////////////////////////////

SolveEnv SEnv(35);

int mainold(int argc, char** argv)
{
	SEnv.Read(argv[1]); ;
	SEnv.ProcessMat();

	ofstream dout("matdmp.txt");
	dout << "ORIGINAL MAT" << endl;
	SEnv.Dump(dout, false, false);
	while (SEnv.HistoFill(dout) > 0)
		;
	dout << endl << "FINAL MAT" << endl;
	SEnv.Dump(dout, false, false);

	SEnv.FillBlanks();

#if 0
	for (int j = 0; j < DIM; j++) {
		u64 cnt = SEnv.Cols[j].PermuteCount();
		cout << " Col " << j << " PermCnt  " << cnt << endl;
	}
	SEnv.SortCols(dout, true);
	SEnv.LinkBlanks();
	SEnv.Dump(cout, true, true);

	SEnv.Dump(dout, true, true);
	dout.close();

	SEnv.SolveCnt(12, 15);
	SEnv.SolveCnt(0, 0);
	SEnv.SolveCnt(1, 1);
	SEnv.SolveCnt(0, 1);
	SEnv.SolveCnt(16,17);
	SEnv.SolveCnt(33,34);
#endif


	SEnv.Solve();

	return 0;
}

Consola* pCons;
#include <windows.h>

std::string current_working_directory()
{
	char working_directory[MAX_PATH + 1];
	GetCurrentDirectoryA(sizeof(working_directory), working_directory); // **** win32 specific ****
	return working_directory;
}


void ConsoleRun ()
{
	string wd = current_working_directory();
	cout << "CWD " << wd << endl;

	pCons = new Consola();

	pCons->pushCons();

	int cnt = 0;
	while (pCons->getCmd()) {
		cnt++;
	}
}

void TestSort()
{
	Sorter TSort;
	for (int sz = 2; sz <= 12; sz++) {
		cout << "Testing sz " << sz << endl;
		TSort.Test(2000, sz);
	}
}

int main(int argc, char** argv)
{
	TestSort();
	ConsoleRun();
}

#if 0
void SolveEnv::UnRestrictRows(SLane* pCol, int depth)
{
	BEntry* pBlank;
	BEntry* pNext;
	for (int d = 0; d <= depth; d++) {
		pBlank = pCol->BStack[d];
		pNext = pBlank->rtLink;
		if (RBuf[d].blankM != 0) {
			// Restore downstream
			pNext->laneMiss = RBuf[d].laneM;
			pNext->miss = RBuf[d].blankM;
		}
	}
}
#endif


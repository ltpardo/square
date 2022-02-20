#include "nlane.h"

void NLane::Init(int _n, int _idx, bool _isRow)
{
	n = _n;
	idx = _idx;
	assert(n < DIMMAX - 2);
	all = Set::BitMask(n) - 1;
	absMiss = all;
	present = 0;
	presCnt = 0;
	missCnt = n;
	stops = 0;
	blankCnt = 0;
	ringHd.SelfLink();
	isRow = _isRow;
}

void NLane::Present(u8 v) {
	if (v >= n)
		return;
	presCnt++;
	present |= Set::BitMask(v);

	missCnt--;
	absMiss &= ~Set::BitMask(v);
}

void NLane::Missing(u8 v) {
	if (v < n)
		return;
	missCnt++;
	absMiss &= ~(Set::BitMask(v));
}

bool NLane::Check() {
	if (presCnt + missCnt != n)
		return false;
	if ((present | absMiss) != all)
		return false;
	return true;
}

void NLane::AddBlank(NEntry* pEnt)
{
	NStack[blankCnt].pEnt = pEnt;
	blankCnt++;
}

//   Fill initial values:
//		Create VDict
//		For Cols: fill pBlank
//			pBlank->pThru and pCross
//			pBlank->miss		with intersection thru and cross missing sets
//		Fill in NStack
//
void NLane::FillBlanks(NLane* CrossLanes)
{
	NLane* pCross;

	// Create VDict
	AbsToLocal();

	for (level = 0; level < blankCnt; level++) {
		pBlank = NStack[level].pEnt;
		if (!isRow) {
			pCross = &CrossLanes[pBlank->i];
			// Fill in pBlank
			pBlank->pThru = this;
			pBlank->pCross = pCross;
			pBlank->miss = absMiss & pCross->absMiss.mask;
			pBlank->missCnt = pBlank->miss.ExtractValues(pBlank->missV, sizeof(pBlank->missV));
		}
		else {
			pCross = &CrossLanes[pBlank->j];
			// Init unselected
			NStack[0].pEnt->crUnSel = absMiss;
		}

		NStack[level].lMiss = MapToLocal(pBlank->missV, pBlank->missCnt);
	}


	// Generate PSet
	u16 cnt;
	PSet.Init(blankCnt, 1, true);
	PSet.VDictSet(absMiss);
	PSet.ExpAlloc();
	PermSetGenExpanded(0, 0, cnt);
	PSet.ExpVerifyCounts();
	if (PSet.badCounts)
		assert(0);
	PSet.LvlFromExp(lMissing);
	PSet.SetId (isRow, idx);

	PSet.LvlSetMiss(this);

//#define REFCREATE
#ifdef REFCREATE
	PermRefCreate();
	string failedExp = "FAILED EXP vs REF";
	if (!PSet.ExpCheckRef(pRef)) {
		Report(failedExp, pRef->fname, pRef->errCnt);
	}
	string failedLvl = "FAILED LVLPSET vs REF";
	if (!PSet.LvlCheckRef(pRef)) {
		Report(failedLvl, pRef->fname, pRef->errCnt);
	}
#endif
}

// Insert blanks into corresponding row ring
void NLane::LinkThru(NLane* Rows, NLane* Cols)
{
	for (int d = 0; d < blankCnt; d++) {
		pBlank = NStack[d].pEnt;
		pBlank->UpThruOf(&ringHd);
		pBlank->thLvl = pBlank->upThru->thLvl + 1;
		pBlank->UpCrossOf(&(Rows[pBlank->i].ringHd));
		pBlank->crLvl = pBlank->upCross->crLvl + 1;
		Rows[pBlank->i].stops += Set::BitMask(pBlank->j);
		pBlank->InitSearchInfo(& PSet, & (Rows[pBlank->i].PSet));
	}
	pBlank->thLast = true;

}

// Insert blanks into corresponding col ring
void NLane::LinkCross(NLane* Cols)
{
	for (int d = 0; d < blankCnt; d++) {
		pBlank = NStack[d].pEnt;
		pBlank->UpCrossOf(&Cols[pBlank->j].ringHd);
		Cols[pBlank->j].stops += Set::BitMask(pBlank->i);
	}
	pBlank->crLast = true;
}

int NLane::RegenPSet()
{
	PSet.ExpDealloc();
	PSet.LvlDealloc();

	for (level = 0; level < blankCnt; level++) {
		pBlank = NStack[level].pEnt;
		NStack[level].lMiss = MapToLocal(pBlank->missV, pBlank->missCnt);
	}

	// Generate PSet
	u16 cnt;
	PSet.Init(blankCnt, 1, true);
	PSet.VDictSet(absMiss);
	PSet.ExpAlloc();
	PermSetGenExpanded(0, 0, cnt);
	PSet.ExpVerifyCounts();
	if (PSet.badCounts)
		assert(0);
	PSet.LvlFromExp(lMissing);
	PSet.SetId(isRow, idx);

	return PSet.LvlSetMiss(this);
}

void NLane::NextPermInit()
{
	pCur = NStack[0].pEnt;
	pCur->ThSetExhausted();
	pCur->thUpBr = 0;
	deadEndDepth = 0;
	isDeadEnd = true;
	pTermReturn = NULL;
}

bool NLane::SearchBackDst(NLane* pPrevLane)
{
	if (! isDeadEnd)
		return false;

	backJ = -1;
	NEntry* pPrev;
	NEntry* pEnt;
	pBackDst = NULL;
	for (int d = 0; d <= deadEndDepth; d++) {
		pEnt = NStack[d].pEnt;
		pPrev = pEnt->upCross;
		if (pPrev->CrIsValid() && pPrev->iCnt > 0
			/* && pPrev->StkCheckSet(pEnt->ThSet) */
			) {
			if (backJ <= pPrev->j) {
				backJ = pPrev->j;
				pBackDst = pPrev;
			}
		}
	}

	// Back destination found if deeper than normal back return
	return pBackDst != NULL
		&& ((pBackDst->j < (idx - 1)
			|| (pBackDst->i < pPrevLane->pCur->i)));
}

void NLane::Backtrack(NEntry* pBack)
{
	if (pBack != NULL) {
		pCur = pBack;
		deadEndDepth = pBack->thLvl;
	}
	SStk.SetBackTop(pCur->stkPos);
}

bool NLane::NextPermAdvCur()
{
	if (pCur->thDnBr != PermSet::LVLNULL)
		// Work has been done by previous expand
		return true;
	if (pCur->iCnt > 0) {
		// Previous expand stored results in stack
		pCur->StkPull();
		if (DbgSt.DmpExpand()) {
			*DbgSt.pOut << pCur->pThPSet->searchId << " PULL " << pCur->thLvl << endl;
		}
		return true;
	}
	if (pCur->thUpBr != PermSet::LVLNULL) {
		// Expand 
		return pCur->ThExpand(pTermReturn);
	}
	// No possible advance
	return false;
}


bool NLane::NextPermInCol()
{
	while (pCur->ThIsValid()) {
		if (deadEndDepth < pCur->thLvl)
			deadEndDepth = pCur->thLvl;

		if (! NextPermAdvCur()) {
			pCur = pCur->ThClimbUp(pTermReturn);
			continue;
		}

		if (!pCur->ThAdvCross())
			continue;

		if (pCur->thLast) {
			// Success: prepare for next iteration and return true
			pCur = pCur->ThClimbUp(pTermReturn);
			isDeadEnd = false;
			return true;
		}
		else {
			pCur = pCur->ThClimbDown();
		}
	}

	// Column has been exhausted
	return false;
}

void NLane::AbsToLocal()
{
	Set abs = absMiss;
	abs.ExtractValues(VDict, BLANKCNTMAX);
	lMissing = (1 << missCnt) - 1;
}

LMask NLane::MapToLocal(Set abs)
{
	LMask m = 0;

	u8 v;
	int d;
	for (d = 0; abs != 0; ) {
		v = (u8)abs.GetFirstIdx();
		while (VDict[d] < v) {
			d++;
			if (d >= missCnt) {
				assert(0);
			}
		}
		assert(VDict[d] == v);
		m |= (u16)(1 << v);
	}
	return m;
}

LMask NLane::MapToLocal(u8* pV, int cnt)
{
	LMask m = 0;

	u8 v;
	int d, l;
	for (d = 0, l = 0; l < cnt; l++) {
		v = pV[l];
		while (VDict[d] < v) {
			d++;
			if (d >= missCnt) {
				assert(0);
			}
		}
		assert(VDict[d] == v);
		m |= (u16)(1 << d);
	}
	return m;
}

// Returns lnk to node and node cnt
//	Input: lane lMissing and stack[level].lMiss
//
LongLink NLane::PermSetGenExpanded(int level, LMask sel, ShortCnt& cnt)
{
	LongLink branch;

	LMask chosenBit;
	LMask unsel = lMissing & (~sel);
	LMask choice = NStack[level].lMiss & unsel;
	LMask nextSel;

	if (choice == 0)
		// Exhausted branch
		return PermSet::LONGLNKNULL;

	if (level == missCnt - 1) {
		// At Bottom
		assert(choice == (choice & (~choice + 1)));
		// Returns mask
		cnt = 1;
		PSet.CountTerm();
		return choice;
	}

	// Internal branch: iterate through all descendants
	ShortLink nodeCnt = 0;
	LongLink* pTemp = PSet.NodeExpStart(level);
	ShortCnt vMask = 0;
	ShortCnt branchCnt;

	do {
		// Extract candidate from choiceSet
		chosenBit = choice & (~choice + 1);
		choice &= ~chosenBit;
		nextSel = sel | chosenBit;

		if ((branch = PermSetGenExpanded(level + 1, nextSel, branchCnt)) == PermSet::LONGLNKNULL)
			// Branch is not viable
			continue;

		if (level >= missCnt - 2) {
			// Bottom branch: 
			pTemp = PSet.NodeExpBotBranch(pTemp, branch);
		}
		else {
			// Intermediate level - tree zone
			pTemp = PSet.NodeExpBranch(pTemp, branchCnt, branch);
		}
		nodeCnt += branchCnt;
		vMask |= chosenBit;
	} while (choice != 0);

	// Create tree node and return
	if (nodeCnt > 0) {
		cnt = nodeCnt;
		return PSet.NodeExpEnd(level, pTemp, nodeCnt, vMask);
	}
	else
		return PermSet::LONGLNKNULL;
}

int NLane::PermRefCreate()
{
	pRef = new PermSetRef(isRow, idx, missCnt);
	pRef->Create();
	PermRefGen(0, 0, refCnt);
	refCnt = pRef->Close();
	return refCnt;
}


bool NLane::PermRefGen(int level, Mask sel, ShortCnt& cnt)
{
	Mask chosenBit;
	Mask unsel = absMiss & (~sel);
	Mask choice = NStack[level].pEnt->miss.mask & unsel;

	if (choice == 0)
		// Exhausted branch
		return false;

	Mask nextSel;
	ShortCnt brCnt;
	bool notEmpty = false;
	do {
		// Extract level value from choiceSet
		chosenBit = choice & (~choice + 1);
		pRef->V[level] = Set::GetIdx(chosenBit);
		choice &= ~chosenBit;
		if (level == missCnt - 1) {
			// At Bottom
			assert(choice == 0);
			pRef->Out();
			return true;
		}

		nextSel = sel | chosenBit;
		if (! PermRefGen(level + 1, nextSel, brCnt)) {
			// Branch is not viable
			continue;
		}
		cnt += brCnt;
		notEmpty = true;
	} while (choice != 0);

	return notEmpty;
}

#if 0
// Returns lnk to node and node cnt
//	Input: lane lMissing and stack[level].lMiss
//
u16 NLane::PermSetGen(int level, LMask sel, u16& cnt)
{
	if (level < 0)
		// Done
		return level;

	u16 branch;
	if (level == PSet.depthTerm) {
		// Generate terminals
		branch = PSet.TermStart(lMissing & (~sel));
		PermSetGenTerminals(level, sel);
		if ((cnt = PSet.TermEnd()) > 0)
			return branch;
		else
			return PermSet::LNKNULL;
	}

	LMask chosenBit;
	LMask unsel = lMissing & (~sel);
	LMask choice = NStack[level].lMiss & unsel;
	LMask nextSel;

	if (choice == 0)
		// Exhausted branch
		return PermSet::LNKNULL;

	// Internal branch: iterate through all descendants
	u16 nodeCnt = 0;
	u16* pTemp = PSet.NodeStart(level);
	u16 vMask = 0;

	do {
		// Extract candidate from choiceSet
		chosenBit = choice & (~choice + 1);
		choice &= ~chosenBit;
		nextSel = sel | chosenBit;

		// Intermediate level - tree zone
		u16 branchCnt;
		if ((branch = PermSetGen(level + 1, nextSel, branchCnt)) == PermSet::LNKNULL)
			// Branch is not viable
			continue;
		nodeCnt += branchCnt;
		vMask |= chosenBit;
		pTemp = PSet.NodeBranch(pTemp, branchCnt, branch);
	} while (choice != 0);

	// Create tree node and return
	cnt = nodeCnt;
	return PSet.NodeEnd(level, pTemp, nodeCnt, vMask);
}

// Returns lnk to node and node cnt
//
bool NLane::PermSetGenTerminals(int level, LMask sel)
{
	LMask unsel = lMissing & (~sel);
	LMask choice = NStack[level].lMiss & unsel;
	if (choice == 0)
		// Exhausted branch
		return false;

	LMask chosenBit;
	LMask nextSel;
	LMask botUnsel;

	bool success = false;
	do {
		// Extract candidate from choiceSet
		chosenBit = choice & (~choice + 1);
		choice &= ~chosenBit;
		nextSel = sel | chosenBit;
		PSet.TermEncode(level, chosenBit);

		if (level >= missCnt - 2) {
			// Next level is bottom: create terminal if viable
			botUnsel = lMissing & (~nextSel);
			assert(__popcnt16(botUnsel) == 1);    // Only one candidate should remain

			if ((NStack[level + 1].lMiss & botUnsel) == botUnsel) {
				// Bottom choice is OK
				PSet.TermOut();
				success = true;
			}
			else
				// Bottom choice is not viable
				continue;
		}
		else {
			// Intermediate level - term zone
			if (!PermSetGenTerminals(level + 1, nextSel))
				// Branch is not viable
				continue;
			success = true;
		}
	} while (choice != 0);

	return success;
}



// Insert blanks into corresponding row ring
void NLane::LinkBlanks(NLane* Rows)
{
	for (int d = 0; d < blankCnt; d++) {
		pBlank = NStack[d].pEnt;
		pBlank->UpThruOf(&Rows[pBlank->i].ringHd);
		Rows[pBlank->i].stops += Set::BitMask(pBlank->j);
	}
}

bool NLane::PermuteStart()
{
	// First level has 
	level = 0;
	pBlank = NStack[level].pEnt;
	pBlank->selectedSet = 0;			// CountOne selected set
	pBlank->unSelected = absMiss;		// All missing vals are needed
	pBlank->choiceSet = pBlank->miss;	// Choose from all available candidates

	return true;
}

void NLane::PermuteContinueAt(int depth)
{
	level = depth;
	pBlank = NStack[level].pEnt;
}

// Computes next permutation in column
//   Returns false if column has exhausted all permutations
//   firstPermLvl is level of first change from previous permutation (-1 if exhausted)
//
int NLane::PermuteStep()
{
	firstPermLvl = level;

	while (level >= 0) {
		// Entering level
		if (pBlank->choiceSet == 0) {				// Exhausted branch
			// Climb up
			pBlank = NStack[--level].pEnt;
		}
		else {
			// Extract candidate from choiceSet
			pBlank->ExtractBestChoice();

			// Update firstLvl
			if (firstPermLvl > level)
				firstPermLvl = level;

			// Branch has a candidate
			if (level > missCnt) {
				// Bottom branch: found a permutation
				assert(pBlank->choiceSet == 0);		//   Only one candidate should remain...

				// Climb up
				pBlank = NStack[--level].pEnt;
				return 1;
			}
			else {
				// Not bottom branch: add chosenBit to selected set for next level
				Mask nextselSet = pBlank->selectedSet | pBlank->chosenBit;
				// ... and proceed down
				pBlank = NStack[++level].pEnt;
				pBlank->selectedSet = nextselSet;
				pBlank->unSelected = absMiss - pBlank->selectedSet;
				pBlank->choiceSet = pBlank->miss & pBlank->unSelected;
			}
		}
	}

	// No more permutations for this pass
	firstPermLvl = -1;
	return 0;
}

bool NLane::LvlContains(int lvl, u8 val)
{
	if (IsSingleRange())
		return val == SingleV[lvl];
	else {
		return false;
		//return PSet.LvlGetBranch(lvl, NStack[lvl].nodeLnk, val);
	}

}

void NLane::PermThruEnterSingle()
{
}

bool NLane::PermThruEnterLvl()
{
	pBlank = NStack[level].pEnt;
	NLane* pCross = pBlank->pCross;
	int crossLvl = pBlank->crLvl;
	LvlBranch branch = NStack[level].enterBranch;

	if (PSet.LvlIsTerm(branch)) {
		NStack[level].selV = PSet.LvlExtractV(branch);
		if (!pCross->LvlContains(crossLvl, NStack[level].selV))
			return false;
		NStack[level].branch = branch;
		return true;
	}
	else if (pCross->IsSingleRange()) {
		u8 v = pCross->SingleV[crossLvl];
		if (LvlContains(level, v))
			return false;
		PermThruEnterSingle();
		return true;
	}
	else {
		// Both lanes have multiple branches: intersect
		LvlBranch* pThNode = PSet.BranchPtr(level, branch);
		LvlBranch* pCrNode =
			pCross->PSet.BranchPtr(crossLvl, pCross->NStack[crossLvl].enterBranch);
		lCnt = PSet.LvlIntersect(pThNode, pCrNode);
		if (lCnt == 0)
			return false;
		if (lCnt == 1) {
			NStack[level].selV = PSet.LvlGetV(PSet.Branches[0]);
			NStack[level].branch = PSet.Branches[0];
		}
		else {
			PermThruSort();
		}
		return true;
	}

	// Enter level
	return true;
}

void NLane::PermThruSort()
{
}

void NLane::PermThruUp()
{
	PermThruEnterLvl();

}

void NLane::PermThruDown()
{

}

bool NLane::PermThruBranch()
{

	return true;
}

void NLane::PermThruSuccess()
{

}

void PermThruStart()
{

}

bool NLane::PermThruAdv()
{
	while (level >= 0) {
		if (NStack[level].IsEmpty()) {
			if (!PermThruEnterLvl()) {
				level--;
				continue;
			}
		}

		if (!PermThruBranch())
			// Unsuccessful branch: continue at this level
			continue;

		// Successful branch
		if (level >= missCnt - 1) {
			PermThruSuccess();
			return true;
		}
		else {
			level++;
			NStack[level].SetEmpty();
			//NStack[level].nodeLnk = ?;
		}
	}

	// No more permutations for this pass
	firstPermLvl = -1;
	return false;
}
#endif


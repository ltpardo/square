#include "nlane.h"
#include "nenv.h"

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
	ringHd.dLevel = 0;
	ringHd.rLevel = NEntry::RLEVELNULL;
	isRow = _isRow;
}

void LaneFull::Present(u8 v)
{
	if (v == BLKENT)
		return;
	presCnt++;
	present |= Set::BitMask(v);

	missCnt--;
	absMiss &= ~Set::BitMask(v);
}

void LaneFull::Missing(u8 v)
{
	if (v == BLKENT)
		return;
	missCnt++;
	absMiss &= ~(Set::BitMask(v));
}

bool LaneFull::Check()
{
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
//		Create ExpSet and PSet
//
void NLane::FillBlanks(NLane* CrossLanes, bool doPerms)
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

	if (doPerms) {
		// Generate ExpSet and PSet
		GenExpSet();
		GenPSet();

		if (EPars.makeRef) {
			// Create Ref file and verify against it
			PermRefCreate();
		}

		if (EPars.checkRef) {
			CheckExpSet();
			CheckPSet();
		}
	}
}

int LaneFull::GenPSet()
{
	PSet.Init(blankCnt, 1, true);
	PSet.VDictSet(absMiss);
	PSet.LvlFromExp(pExpSet, lMissing);
	PSet.SetId(isRow, idx);
#ifdef RESTRICT
	int chgCnt = PSetRestrictMiss();
	return chgCnt;
#endif
	return 0;
}

bool LaneFull::CheckExpSet()
{
	string msg = "FAILED EXP vs REF";
	if (!pExpSet->CheckRef(pRef)) {
		Report(msg, pRef->fname, pRef->errCnt);
		return false;
	}
	return true;
}

bool LaneFull::CheckPSet()
{
	string msg = "FAILED LVLPSET vs REF";
	if (!PSet.LvlCheckRef(pRef)) {
		Report(msg, pRef->fname, pRef->errCnt);
		return false;
	}
	return true;
}


// Add an entry to NStack
//	 Sets pPrev to NStack[blankCnt].pEnt
//   Returns stack level
//
s8 LaneFull::AddEnt(EntryBase* pEnt, u8 EntDict[], EntryBase*& pPrev)
{
	pPrev = (blankCnt == 0) ? nullptr : (EntryBase*)(NStack[blankCnt - 1].pEnt);

	NStack[blankCnt].pEnt = (NEntry *) pEnt;		///// FIX type
	NStack[blankCnt].lMiss = MapToLocal(EntDict, pEnt->missCnt);
	
	return blankCnt++;
}

bool LaneFull::PermsGenAndCheck()
{
	if (EPars.makeRef)
		PermRefCreate();

	if (GenExpSet() == PermSet::LONGLNKNULL) {
		PSet.SetEmpty();
		return false;
	}

	bool checkSuccess = true;
	if (EPars.checkRef)
		checkSuccess = CheckExpSet();

	GenPSet();
	if (EPars.checkRef)
		checkSuccess = checkSuccess && CheckPSet();

	return checkSuccess;
}

LongLink LaneFull::GenExpSet()
{
	if (missCnt <= 0) {
		// Empty lane
		pExpSet = nullptr;
		return PermSet::LONGLNKNULL;
	}

	LMask MissTab[BLANKCNTMAX];
	for (int d = 0; d < missCnt; d++) {
		MissTab[d] = NStack[d].lMiss;
	}
	pExpSet = new PermExp();
	pExpSet->Alloc();
	pExpSet->GenInit(lMissing, MissTab, missCnt);

	ShortCnt cnt;
	pExpSet->baseLnk = pExpSet->Gen(0, 0, cnt);
	pExpSet->VDictSet(absMiss);
	if (!pExpSet->VerifyCounts())
		assert(0);
	return pExpSet->baseLnk;

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
	pExpSet->Dealloc();
	delete pExpSet;
	PSet.LvlDealloc();

	for (level = 0; level < blankCnt; level++) {
		pBlank = NStack[level].pEnt;
		NStack[level].lMiss = MapToLocal(pBlank->missV, pBlank->missCnt);
	}

	// Generate ExpSet and PSet
	if (EPars.makeRef) {
		// Create Ref file and verify against it
		PermRefCreate();
	}

	GenExpSet();
	if (EPars.checkRef)
		CheckExpSet();

	int chg = GenPSet();
	if (EPars.checkRef)
		CheckPSet();
	return chg;
}

////////////////////////////////////////////////////////////////////////////
//
//	SEARCHES
// 
////////////////////////////////////////////////////////////////////////////

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

bool NLane::RedStartFind()
{
	redStartDepth = DIMMAX;
	rLevel = RLevelCalc(idx, isRow);
	if (isRow) {		// Rows
		// Skip entries in previous lanes
		for (pCurRedStart = NStack[0].pEnt;
			pCurRedStart->j < idx;
			pCurRedStart = pCurRedStart->dnCross)
		{
			if (pCurRedStart->crLast) {
				// Tail-empty row
				pCurRedStart = NULL;
				prevOverlap = false;
				return false;
			}
		}
		// Overlap when pCurRedStart is on diagonal
		prevOverlap = pCurRedStart->j == idx;
		// Skip overlap entry for rows
		if (prevOverlap) {
			// Skip overlap with col, if not last
			if (!pCurRedStart->crLast)
				pCurRedStart = pCurRedStart->dnCross;
		}
		if (pCurRedStart->crLast) {
			// Tail-empty row
			pCurRedStart = NULL;
			return false;
		}
		redStartDepth = pCurRedStart->crLvl;
		pCurRedStart->crStart = true;
		assert(pCurRedStart->j > pCurRedStart->i);
	}
	else {			// Cols
		// Skip entries in previous lanes
		for (pCurRedStart = NStack[0].pEnt;
			pCurRedStart->i < idx;
			pCurRedStart = pCurRedStart->dnThru)
		{
			if (pCurRedStart->thLast) {
				prevOverlap = false;
				pCurRedStart = NULL;
				return false;
			}
		}
		// Overlap when previous row starts at col
		prevOverlap = pCurRedStart->upThru->i == idx;
		redStartDepth = pCurRedStart->thLvl;
		pCurRedStart->thStart = true;
		assert(pCurRedStart->i >= pCurRedStart->j);
	}

	return true;
}

void NLane::RedBackSet(NLane* Rows, NLane* Cols)
{
	NEntry* pEnt;
	NLane* pPrevLane;
	NEntry* pPrevLink = NULL;
	int utl;
	int ucl;

	for (int d = redStartDepth; d < blankCnt; d++) {
		pEnt = NStack[d].pEnt;
		pEnt->rLevel = rLevel;
		if (idx == 0) {
			// Top level lane: no backtrack possible
			pEnt->SetRedTop();
			continue;
		}

		pPrevLane = LaneFromLevel(rLevel - 1, Rows, Cols);
		if (prevOverlap) {
			// Backtrack to start of previous level
			pEnt->SetRed(rLevel - 1, pPrevLane->redStartDepth);
			goto BackCheck;
		}

		if (isRow) {
			if (pEnt->upThru->rLevel == rLevel - 2) {
				// Backtrack to entry above
				pEnt->SetRed(rLevel - 2, pEnt->upThru->crLvl);
				pPrevLink = pEnt;   // this is the new prevLink
				goto BackCheck;
			}
		}
		else {
			if (pEnt->upCross->rLevel == rLevel - 2) {
				// Backtrack to entry to the left
				pEnt->SetRed(rLevel - 2, pEnt->upCross->thLvl);
				pPrevLink = pEnt;   // this is the new prevLink
				goto BackCheck;
			}
		}
		
		if (pPrevLink != NULL) {
			// Backtrack to previous link
			pEnt->SetRed(pPrevLink->redBackLvl, pPrevLink->redBackDepth);
			goto BackCheck;
		}

		// No prevLink, no overlap: backtrack to closest precursor
		utl = pEnt->upCross->rLevel;
		ucl = pEnt->upThru->rLevel;
		if (ucl == NEntry::RLEVELNULL && utl == NEntry::RLEVELNULL) {
			// Starting Node: backtrack to previous level and restart node
			//pEnt->SetRed(rLevel - 1, pPrevLane->redStartDepth);
			pEnt->SetRedTop();
			//pPrevLink = pEnt;
			continue;
		}
		else if (ucl > utl) {
			// Row is closest
			pEnt->SetRed(ucl, pEnt->upThru->crLvl);
			pPrevLink = pEnt;
		}
		else {
			// Col is closer
			pEnt->SetRed(utl, pEnt->upCross->thLvl);
			pPrevLink = pEnt;
		}

	BackCheck:
		NLane* pBackLane = LaneFromLevel(pEnt->redBackLvl, Rows, Cols);
		NEntry* pBackEnt = pBackLane->NStack[pEnt->redBackDepth].pEnt;
		assert(pBackLane->EntryInRLevel(pBackEnt));
	}
}

void NLane::NextPermRedRestart()
{
	pCur = pCurRedStart;
	pCur->ESt.CntNullSet();
	pCur->iCnt = 0;
	pTermReturn = NULL;
	if (isRow) {
		pCur->crUpBr = pCur->upCross->crDnBr;
		pCur->crDnBr = PermSet::LVLNULL;
	}
	else {
		pCur->thUpBr = pCur->upThru->thDnBr;
		pCur->thDnBr = PermSet::LVLNULL;
	}

	redDepth = 0;
	redSuccess = false;
}

void NLane::NextPermRedFailed(int fDepth)
{
	int curDepth = isRow ? pCur->crLvl : pCur->thLvl;
	if (fDepth < curDepth)
		pCur = NStack[fDepth].pEnt;
}

bool NLane::NextPermRedRow()
{
	for (; ; ) {
		if (pCur->j <= idx) {
			cerr << "ROW TRESPASS idx " << idx << " j: " << pCur->j << endl;
		}
		if (redDepth < pCur->crLvl)
			redDepth = pCur->crLvl;

		if (!NextPermRedDnRow()) {
			if (pCur == pCurRedStart)
				// Current iteration exhausted
				return false;
			if (pCur->j <= pCurRedStart->j)
				cerr << "ROW SKIP" << endl;

			pCur = pCur->CrClimbUp(pTermReturn);
			continue;
		}
		pCur->RowAdvCross();

		if (pCur->crLast) {
			// Success: prepare for next iteration and return true
			pCur = pCur->CrClimbUp(pTermReturn);
			redSuccess = true;
			return true;
		}
		else {
			pCur = pCur->CrClimbDown();
		}
	}

	// Unreachable: Row has been exhausted
	return false;
}

bool NLane::NextPermRedDnRow()
{
	if (pCur->crDnBr != PermSet::LVLNULL)
		// Work has been done by previous expand
		return true;
	if (!pCur->ESt.IsStkEmpty()) {
		// Pull branches
		if (!pCur->ESt.Pull(pCur->thDnBr, pCur->crDnBr))
			pCur->ESt.CntNullSet();
		return true;
	}
	if (pCur->crUpBr != PermSet::LVLNULL) {
		// Expand 
		return pCur->DExpand(pTermReturn, true);
	}
	// No possible advance
	return false;
}

bool NLane::NextPermRedCol()
{
	for( ; ; ) {
		if (pCur->i < idx) {
			cerr << "COL TRESPASS idx " << idx << " i: " << pCur->i << endl;
		}
		if (redDepth < pCur->thLvl)
			redDepth = pCur->thLvl;

		if (!NextPermRedDnCol()) {
			if (pCur->i < pCurRedStart->i) {
				cerr << "COL SKIP idx " << idx << " i: " << pCur->i << endl;
			}
			if (pCur == pCurRedStart)
				// Current iteration exhausted
				return false;

			pCur = pCur->ThClimbUp(pTermReturn);
			continue;
		}

		pCur->ColAdvCross();

		if (pCur->thLast) {
			// Success: prepare for next iteration and return true
			pCur = pCur->ThClimbUp(pTermReturn);
			redSuccess = true;
			return true;
		}
		else {
			pCur = pCur->ThClimbDown();
		}
	}

	// Unreachable: Column has been exhausted
	return false;
}

bool NLane::NextPermRedDnCol()
{
	if (pCur->thDnBr != PermSet::LVLNULL)
		// Work has been done by previous expand
		return true;
	if (!pCur->ESt.IsStkEmpty()) {
		// Pull branches
		if (!pCur->ESt.Pull(pCur->thDnBr, pCur->crDnBr))
			pCur->ESt.CntNullSet();
		return true;
	}

	if (pCur->thUpBr != PermSet::LVLNULL) {
		// Expand 
		return pCur->DExpand(pTermReturn, false);
	}
	// No possible advance
	return false;
}

void LaneFull::AbsToLocal()
{
	Set abs = absMiss;
	abs.ExtractValues(VDict, BLANKCNTMAX);
	lMissing = (1 << missCnt) - 1;
}

LMask LaneFull::MapToLocal(Set abs)
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

LMask LaneFull::MapToLocal(u8* pV, int cnt)
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

void LaneFull::RemoveV(u8 v)
{
	assert(absMiss.Contains(v));
	absMiss -= v;
	missCnt--;
	AbsToLocal();
}

int LaneFull::PermRefCreate()
{
	Set MissTab[BLANKCNTMAX];

	for (int d = 0; d < missCnt; d++) {
		MissTab[d] = NStack[d].pEnt->miss.mask;
	}

	pRef = new PermSetRef(isRow, idx, missCnt, absMiss, MissTab);
	pRef->Create();
	pRef->Gen(0, 0, refCnt);
	refCnt = pRef->Close();
	return refCnt;
}

int NLane::PSetRestrictMiss()
{
	int MissDel[BLANKCNTMAX];

	PSet.pChkRef = 0;
	for (int d = 0; d < PSet.depth; d++) {
		PSet.VisitMiss[d] = 0;
	}
	PSet.LvlVisit(0, PSet.BranchPtr(0, (ShortLink)0));
	NEntry* pEnt;
	int chgCnt = 0;
	for (int d = 0; d < blankCnt; d++) {
		pEnt = NStack[d].pEnt;
		assert(pEnt->miss >= PSet.VisitMiss[d]);
		int mCnt = pEnt->miss.Count();
		int vCnt = PSet.VisitMiss[d].Count();
		MissDel[d] = mCnt - vCnt;
		if (MissDel[d] > 0) {
			Report(
				isRow ? "  ROW CHG idx/d/cnt " : "  COL CHG idx/d/cnt ",
				idx, d, vCnt);
		}
		if (vCnt == 1) {
			Report(
				isRow ? "  ROW UNIQUE idx/d/i/j " : "  COL UNIQUE idx/d/i/j ",
				idx, d, pEnt->i, pEnt->j);
		}
		chgCnt += MissDel[d];
		// Restrict entry miss
		pEnt->miss = PSet.VisitMiss[d];
		pEnt->missCnt =
			pEnt->miss.ExtractValues(pEnt->missV, sizeof(pEnt->missV));

		NStack[d].lMiss =
			MapToLocal(pEnt->missV, pEnt->missCnt);
	}

	return chgCnt;
}

int LaneFull::PSetRestrictMiss()
{
	int MissDel[BLANKCNTMAX];

	PSet.pChkRef = 0;
	for (int d = 0; d < PSet.depth; d++) {
		PSet.VisitMiss[d] = 0;
	}
	PSet.LvlVisit(0, PSet.BranchPtr(0, (ShortLink)0));
	EntryBase* pEnt;
	u8 VAux[BLANKCNTMAX];
	int chgCnt = 0;
	for (int d = 0; d < blankCnt; d++) {
		pEnt = NStack[d].pEnt;
		if (!(pEnt->miss >= PSet.VisitMiss[d])) {
			Report(isRow ? "  ROW PSET OFLO idx/d " : "  COL PSET OFLO idx/d ", idx, d);
		}
		else {
			int mCnt = pEnt->miss.Count();
			int vCnt = PSet.VisitMiss[d].Count();
			MissDel[d] = mCnt - vCnt;
			if (MissDel[d] > 0) {
				Report(
					isRow ? "  ROW CHG idx/d/cnt " : "  COL CHG idx/d/cnt ",
					idx, d, vCnt);
			}
			if (vCnt == 1) {
				Report(
					isRow ? "  ROW UNIQUE idx/d/i/j " : "  COL UNIQUE idx/d/i/j ",
					idx, d, pEnt->i, pEnt->j);
			}
			chgCnt += MissDel[d];
			// Restrict entry miss
			pEnt->miss = PSet.VisitMiss[d];
		}
		pEnt->missCnt =
			pEnt->miss.ExtractValues(VAux, sizeof(VAux));

		NStack[d].lMiss =
			MapToLocal(VAux, pEnt->missCnt);
	}

	return chgCnt;
}


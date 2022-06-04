#include "nlane.h"

void NEntry::DumpExpand(bool isRow)
{
	if (isRow) {
		*DbgSt.pOut << pCrPSet->searchId << " ";
		pCrPSet->DumpExpanded(*DbgSt.pOut, crLvl, crUpBr);
		*DbgSt.pOut << " <<>> ";

		*DbgSt.pOut << pThPSet->searchId << " ";
		pThPSet->DumpExpanded(*DbgSt.pOut, thLvl, thUpBr);
		*DbgSt.pOut << endl;
	}
	else {
		*DbgSt.pOut << pThPSet->searchId << " ";
		pThPSet->DumpExpanded(*DbgSt.pOut, thLvl, thUpBr);
		*DbgSt.pOut << " <<>> ";

		*DbgSt.pOut << pCrPSet->searchId << " ";
		pCrPSet->DumpExpanded(*DbgSt.pOut, crLvl, crUpBr);
		*DbgSt.pOut << endl;
	}
}

bool NEntry::StkCheckSet(Set& set)
{
	SolveStkEnt* pStkEnt;
	u8 v;
	for (int disp = 1; disp <= iCnt; disp++) {
		pStkEnt = SStk.Peek(stkPos - disp);
		v = pCrPSet->LvlGetV(pStkEnt->crBr);
		if (set.Contains(v))
			return true;
	}
	return false;
}

bool NEntry::ThTermReturn(NEntry*& pReturn)
{
	if (pReturn != NULL)
		// Already in terminal mode
		return false;
	if (ThStarts())
		// At top: case of a single perm (!!!)
		return false;

	// Set return address
	pReturn = upThru;
	return true;
}

bool NEntry::CrTermReturn(NEntry*& pReturn)
{
	if (pReturn != NULL)
		// Already in terminal mode
		return false;
	if (CrStarts())
		// At top: case of a single perm (!!!)
		return false;

	// Set return address
	pReturn = upCross;
	return true;
}

bool NEntry::ThExpand(NEntry*& pReturn)
{
	LvlBranch branch;
	LvlBranch brChk;
	bool success = true;
	ThSet = 0;
	success = false;
	thDnBr = PermSet::LVLNULL;
	crDnBr = PermSet::LVLNULL;
	iCnt = 0;

	if (DbgSt.DmpExpand())
		DumpExpand();

	if (pThPSet->LvlIsTerm(thUpBr)) {
		branch = thUpBr;
		selV = pThPSet->LvlTermAdv(branch);
		if ((brChk = pCrPSet->LvlBranchAdv(crLvl, crUpBr, selV)) != PermSet::LVLNULL) {
			ThTermReturn(pReturn);
			thDnBr = branch;
			crDnBr = brChk;
			if (selV == upCross->selV)
				cerr << "TERM DUP SELV " << selV << endl;
			success = true;
		}
	}
	else if (pCrPSet->LvlIsTerm(crUpBr)) {
		branch = crUpBr;
		selV = pCrPSet->LvlTermAdv(branch);
		if ((brChk = pThPSet->LvlBranchAdv(thLvl, thUpBr, selV)) != PermSet::LVLNULL) {
			thDnBr = brChk;
			crDnBr = branch;
			success = true;
		}
	}
	else {
		// Both lanes have multiple branches: intersect
		LvlBranch* pThNode = pThPSet->BranchPtr(thLvl, thUpBr);
		LvlBranch* pCrNode = pCrPSet->BranchPtr(crLvl, crUpBr);
		iCnt = pThPSet->LvlIntersect(pThNode, pCrNode, ThSet);

		if (iCnt > 0) {
			success = true;
			if (iCnt == 1) {
				thDnBr = pThPSet->Branches[0];
				selV = pThPSet->LvlGetV(thDnBr);
				if (selV == upCross->selV)
					cerr << "SINGLE SELV " << selV << endl;
				crDnBr = pThPSet->CrBranches[0];
				if (pThPSet->LvlCntIsOne(thUpBr))
					ThTermReturn(pReturn);
				ThSet = 0;
				iCnt = 0;
			}
			else {
				if (DbgSt.noPotSort) {
					thDnBr = pThPSet->Branches[0];
					selV = pThPSet->LvlGetV(thDnBr);
					ThSet -= selV;
					crDnBr = pThPSet->CrBranches[0];

					// Push rest of branches in potential order
					for (int pos = 1; pos < iCnt; pos++) {
						SStk.Push(pThPSet->Branches[pos], pThPSet->CrBranches[pos]);
					}

				}
				else {
					// Sort, set highest potential result and push the rest
					pThPSet->LvlSortInter();

					if (DbgSt.lowPotFirst) {
						int pos = pThPSet->SortedIdx(iCnt - 1);
						thDnBr = pThPSet->Branches[pos];
						selV = pThPSet->LvlGetV(thDnBr);
						if (selV == upCross->selV)
							cerr << "EXP SELV " << selV << endl;
						crDnBr = pThPSet->CrBranches[pos];

						// Push rest of branches in potential order
						for (int s = 0; s < iCnt - 1; s++) {
							pos = pThPSet->SortedIdx(s);
							SStk.Push(pThPSet->Branches[pos], pThPSet->CrBranches[pos]);
						}
					}
					else {
						int pos = pThPSet->SortedIdx(0);
						thDnBr = pThPSet->Branches[pos];
						selV = pThPSet->LvlGetV(thDnBr);
						if (selV == upCross->selV)
							cerr << "EXP SELV " << selV << endl;
						ThSet -= selV;
						crDnBr = pThPSet->CrBranches[pos];

						// Push rest of branches in potential order
						for (int s = iCnt - 1; s > 0; s--) {
							pos = pThPSet->SortedIdx(s);
							SStk.Push(pThPSet->Branches[pos], pThPSet->CrBranches[pos]);
						}
					}
				}
				iCnt--;
			}
		}
	}

	// Enter level:
	//		thUpBr has been expanded
	//		thDnBr has the chosen sibling
	//		other siblings are stored in SStk
	//
	StkPos();
	thUpBr = PermSet::LVLNULL;
	return success;
}

#define DBGREL
#ifdef DBGREL
void DumpJumpTrace(string & id, int level) {
	if (DbgSt.DmpExpand()) {
		*DbgSt.pOut << id << " UP JUMP " << level << endl;
	}
}

void DumpUpTrace(bool valid, string& id, int level) {
	if (DbgSt.DmpExpand()) {
		if (valid)
			*DbgSt.pOut << id << " UP " << level << endl;
		else
			*DbgSt.pOut << " UP AND OUT " << endl;
	}
}
#else
#define DumpJumpTrace(a, b)
#define DumpUpTrace(a, b, c)
#endif

NEntry* NEntry::ThClimbUp(NEntry*& pReturn)
{
	if (ThFirstEnt())
		return NULL;
	NEntry* pEnt;
	if (pReturn != NULL) {
		pEnt = pReturn;
		DumpJumpTrace(pEnt->pThPSet->searchId, pEnt->thLvl);
		pReturn = NULL;
	}
	else {
		pEnt = upThru;
		DumpUpTrace(pEnt->ThIsValid(), pEnt->pThPSet->searchId, pEnt->thLvl);
	}
	pEnt->thDnBr = PermSet::LVLNULL;
	return pEnt;
}

NEntry* NEntry::ThClimbDown()
{
	assert(!thLast);

	NEntry* pEnt = dnThru;
	pEnt->thUpBr = thDnBr;
	thDnBr = PermSet::LVLNULL;
	pEnt->thDnBr = PermSet::LVLNULL;
	pEnt->iCnt = 0;
	return pEnt;
}

NEntry* NEntry::CrClimbUp(NEntry*& pReturn)
{
	if (CrFirstEnt())
		return NULL;
	NEntry* pEnt;
	if (pReturn != NULL) {
		pEnt = pReturn;
		DumpJumpTrace(pEnt->pCrPSet->searchId, pEnt->crLvl);
		pReturn = NULL;
	}
	else {
		pEnt = upCross;
		DumpUpTrace(pEnt->CrIsValid(), pEnt->pCrPSet->searchId, pEnt->crLvl);
	}
	pEnt->crDnBr = PermSet::LVLNULL;
	return pEnt;
}

NEntry* NEntry::CrClimbDown()
{
	assert(!crLast);

	NEntry* pEnt = dnCross;
	pEnt->crUpBr = crDnBr;
	crDnBr = PermSet::LVLNULL;
	pEnt->crDnBr = PermSet::LVLNULL;
	pEnt->iCnt = 0;
	return pEnt;
}

bool NEntry::ThAdvCross()
{
	Set uns = crUnSel - selV;
	if (crLast) {
		assert(crUnSel == 0);
		return true;
	}

	dnCross->crUnSel = uns;
	Set inter;
	for (NEntry* pEnt = dnCross; !pEnt->crLast; pEnt = pEnt->dnCross) {
		inter = uns & pEnt->miss;
		if (inter == 0) {
			thDnBr = PermSet::LVLNULL;
			return false;
		}
		if (inter.CountIsOne()) {
			uns -= inter;
		}
	}

	dnCross->crUpBr = crDnBr;
	dnCross->crDnBr = PermSet::LVLNULL;
	return true;
}

bool NEntry::ColAdvCross()
{
	Set uns = crUnSel - selV;
	if (crLast) {
		assert(crUnSel == 0);
		return true;
	}

	dnCross->crUnSel = uns;
	dnCross->crUpBr = crDnBr;
	dnCross->crDnBr = PermSet::LVLNULL;
	dnCross->ESt.CntNullSet();
	return true;
}

bool NEntry::RowAdvCross()
{
	Set uns = thUnSel - selV;
	if (thLast) {
		assert(thUnSel == 0);
		return true;
	}

	dnThru->thUnSel = uns;
	dnThru->thUpBr = thDnBr;
	dnThru->thDnBr = PermSet::LVLNULL;
	dnThru->ESt.CntNullSet();
	return true;
}

bool NEntry::DExpand(NEntry*& pReturn, bool isRow)
{
	LvlBranch branch;
	LvlBranch brChk;

	ESt.Opt = 0;
	ESt.CntNullSet();
	ESt.TabReset();
	iCnt = 0;

	if (DbgSt.DmpExpand())
		DumpExpand(isRow);

	bool success = true;
	if (pThPSet->LvlIsTerm(thUpBr)) {
		branch = thUpBr;
		selV = pThPSet->LvlTermAdv(branch);
		if ((brChk = pCrPSet->LvlBranchAdv(crLvl, crUpBr, selV)) != PermSet::LVLNULL) {
			if (!isRow)
				ThTermReturn(pReturn);
			thDnBr = branch;
			crDnBr = brChk;
		}
		else
			success = false;
	}
	else if (pCrPSet->LvlIsTerm(crUpBr)) {
		branch = crUpBr;
		selV = pCrPSet->LvlTermAdv(branch);
		if ((brChk = pThPSet->LvlBranchAdv(thLvl, thUpBr, selV)) != PermSet::LVLNULL) {
			if (isRow)
				CrTermReturn(pReturn);
			thDnBr = brChk;
			crDnBr = branch;
		}
		else
			success = false;
	}
	else {
		// Both lanes have multiple branches: intersect
		LvlBranch* pThNode = pThPSet->BranchPtr(thLvl, thUpBr);
		LvlBranch* pCrNode = pCrPSet->BranchPtr(crLvl, crUpBr);
		iCnt = pThPSet->LvlIntersect(pThNode, pCrNode, ESt.Opt);

		if (iCnt > 0) {
			if (iCnt == 1) {
				thDnBr = pThPSet->Branches[0];
				selV = pThPSet->LvlGetV(thDnBr);
				crDnBr = pThPSet->CrBranches[0];

				// Check for singles
				if (isRow) {
					if (pCrPSet->LvlCntIsOne(crUpBr))
						CrTermReturn(pReturn);
				}
				else {
					if (pThPSet->LvlCntIsOne(thUpBr))
						ThTermReturn(pReturn);
				}
				ESt.Opt = 0;
				iCnt = 0;
			}
			else {
				// Multiple intersections
				//
				ESt.CntNullReset();
				if (DbgSt.noPotSort) {
					thDnBr = pThPSet->Branches[0];
					selV = pThPSet->LvlGetV(thDnBr);
					ESt.Opt -= selV;
					crDnBr = pThPSet->CrBranches[0];

					// Push rest of branches in potential order
					for (int pos = 1; pos < iCnt; pos++) {
						ESt.Push(pThPSet->Branches[pos], pThPSet->CrBranches[pos]);
					}
				}
				else {
					// Sort, set highest potential result and push the rest
					pThPSet->LvlSortInter();

					if (DbgSt.lowPotFirst) {
						int pos = pThPSet->SortedIdx(iCnt - 1);
						thDnBr = pThPSet->Branches[pos];
						selV = pThPSet->LvlGetV(thDnBr);
						crDnBr = pThPSet->CrBranches[pos];

						// Push rest of branches in potential order
						for (int s = 0; s < iCnt - 1; s++) {
							pos = pThPSet->SortedIdx(s);
							ESt.Push(pThPSet->Branches[pos], pThPSet->CrBranches[pos]);
						}
					}
					else {
						int pos = pThPSet->SortedIdx(0);
						thDnBr = pThPSet->Branches[pos];
						selV = pThPSet->LvlGetV(thDnBr);
						ESt.Opt -= selV;
						crDnBr = pThPSet->CrBranches[pos];

						// Push rest of branches in potential order
						for (int s = iCnt - 1; s > 0; s--) {
							pos = pThPSet->SortedIdx(s);
							ESt.Push(pThPSet->Branches[pos], pThPSet->CrBranches[pos]);
						}
					}
				}
				iCnt--;
			}
		}
		else {
			// No intersection
			success = false;
		}
	}

	//		thUpBr + crUpBr have been expanded
	//		selV has been selected
	//		thDnBr + crDnBr have the siblings for selV
	//		other siblings are stored in SStk
	//
	ESt.ExpandedSet();
	if (isRow)
		crUpBr = PermSet::LVLNULL;
	else
		thUpBr = PermSet::LVLNULL;

	return success;
}

bool NEntry::DProcess(BitSet* pBSet)
{
	assert(ESt.st != ESt.ST_VOID);
	bool success;
	NEntry* pRet;
	if (ESt.IsBrsNew()) {
		// Expand pEnt
		ESt.BrNewReset();
		success = DExpand(pRet);
	}
	else if (! ESt.IsCntNull()) {
		// Pull branches
		if (! ESt.Pull(thDnBr, crDnBr))
			ESt.CntNullSet();
		success = true;
	}
	else {
		// pEnt exhausted
		success = false;
	}

	if (success) {
		// Propagate success
		if (!thLast) {
			dnThru->thUpBr = thDnBr;
			dnThru->ESt.BrNewSet();
			pBSet->Set(dnThru->dLvlPos);
		}
		if (!crLast) {
			dnCross->crUpBr = crDnBr;
			dnCross->ESt.BrNewSet();
			pBSet->Set(dnCross->dLvlPos);
		}
	}
	return success;
}


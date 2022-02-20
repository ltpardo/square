#include "nlane.h"

void NEntry::DumpExpand()
{
	*DbgSt.pOut << pThPSet->searchId << " ";
	pThPSet->DumpExpanded(*DbgSt.pOut, thLvl, thUpBr);
	pCrPSet->DumpExpanded(*DbgSt.pOut, crLvl, crUpBr);
	*DbgSt.pOut << endl;
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
	if (ThFirstEnt())
		// At top: case of a single perm (!!!)
		return false;

	// Set return address
	pReturn = upThru;
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
				if (pThPSet->LvlGetCnt(thUpBr) == 1)
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
void DumpJumpTrace(NEntry* pEnt) {
	if (DbgSt.DmpExpand()) {
		*DbgSt.pOut << pEnt->pThPSet->searchId << " UP JUMP " << pEnt->thLvl << endl;
	}
}

void DumpUpTrace(NEntry * pEnt) {
	if (DbgSt.DmpExpand()) {
		if (pEnt->ThIsValid())
			*DbgSt.pOut << pEnt->pThPSet->searchId
			<< " UP " << pEnt->thLvl << endl;
		else
			*DbgSt.pOut << " UP AND OUT " << endl;
	}
}
#else
#define DumpJumpTrace(a)
#define DumpUpTrace(a)
#endif

NEntry* NEntry::ThClimbUp(NEntry* &pReturn)
{
	if (ThFirstEnt())
		return NULL;
	NEntry* pEnt;
	if (pReturn != NULL) {
		pEnt = pReturn;
		DumpJumpTrace(pEnt);
		pReturn = NULL;
	}
	else {
		pEnt = upThru;
		DumpUpTrace(pEnt);
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


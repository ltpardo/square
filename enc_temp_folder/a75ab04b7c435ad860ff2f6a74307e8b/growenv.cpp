#include "nenv.h"
#include "growenv.h"
#include "matvis.h"

RangeTrack RngTrack;

SolveStkEnt StkPool[STKSIZE];

int GEnv::Search()
{
    int chgCnt = 0;
    SearchInit();

    RngTrack.Init(&cout);

    for ( ; ; ) {
        RngTrack.Collect(srLevel);
        if (srLevel < srLvlFirst)
            // End of search
            break;

        if (pSrLane->AdvPerm()) {
            // Lane has a valid perm
            if (srLevel >= srLvlLast) {
                // Found a solution
                Publish();
                SearchUpSuccess();
                RngTrack.Init(&cout);
                    Report("  Chg cnt ", chgCnt);
                chgCnt = 0;
                if (DbgSt.SearchLim() <= 0)
                    break;
            }
            else {
                // Continue to next level
                SearchDn();
            }
        }
        else {
            if (srLevel < 0) {
                // All settings explored: search done
                break;
            }
            else {
                if (pSrLane->AdvFailed()) {
                    // Lane blocks on this Setting: enter change mode
                    chgCnt++;
                    if (DbgSt.doChange)
                        SearchChange();
                    else
                        SearchUp();
                }
                else {
                    // Lane has processed all Perms
                    SearchUp();
                }
            }
        }
    }

    Report("END OF SEARCH count: ", srCnt);
    return srCnt;
}

void GEnv::SearchInit()
{
    srCnt = 0;
    srLevel = srLvlFirst;
    pSrLane = pLanes + srLvlFirst;
    //growLevel = srLvlFirst;
    growLevel = -1;
    pSrLane->AdvInit();
    SetChgAreaNull();

    // Initialize all up branches in all entries
    GEntry* pLim = pEntries + blankCnt;
    for (GEntry* pEnt = pEntries; pEnt < pLim; pEnt++)
        pEnt->InitBranches();
}

// Expand Change area to include pEnt
//
void GEnv::SetChgArea(GEntry* pEnt)
{
    if (iChg > pEnt->i)
        iChg = pEnt->i;
    if (jChg > pEnt->j)
        jChg = pEnt->j;
}

void GEnv::SearchUpSuccess()
{
    int advCnt = pSrLane->advPerms;
    //growLevel = pSrLane->gUp;
    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;
    if (DbgSt.DmpExpand())
        Report("  ++ SUCCESS to lvl / advPerms", srLevel, advCnt);
}

void GEnv::SearchUp()
{
    //  if (! InChangeMode()) 
    //    growLevel = pSrLane->gUp;
    int advCnt = pSrLane->advPerms;
    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;
    if (DbgSt.DmpExpand())
        Report("  UP to lvl / advPerms", srLevel, advCnt);
}

// Climb up when a pSrLane has not yielded a perm
void GEnv::SearchChange()
{
    s16 idx;
    GEntry* pFail = pSrLane->pEntFirst + pSrLane->failDepth;
    idx = pFail->changeIdx;
    GEntry *pChg = pEntries + idx;

    srLevel = pChg->gLevel;
    pSrLane = pLanes + srLevel;
    pSrLane->pAdv = pChg;
    pSrLane->advDepth = pChg->laneLvl;

    //SetChgArea(pChg);
    if (DbgSt.DmpExpand())
        Report("  CHG UP to ", srLevel);
}

void GEnv::SearchDn()
{
    srLevel = pSrLane->gDn;
    pSrLane = pLanes + srLevel;

    if (InChangeMode()) {
        // Changing
        pSrLane->AdvCont(iChg, jChg);
        if (DbgSt.DmpExpand())
            Report("  DOWN CHG ", srLevel);
    }
    else {
        // Growing: start lane
        // growLevel = srLevel;
        pSrLane->AdvInit();
        //SetChgAreaNull();
        if (DbgSt.DmpExpand())
            Report("  DOWN GRW ", srLevel);
    }
}

void GEnv::Publish()
{
    srCnt++;
    SearchDump();
}

void GEnv::SearchDump()
{
    RMat = Mat;

    GEntry* pEnt = pEntries;
    u32 hash = 0;
    const int hashMask = 0xF;
    for (int e = 0; e < blankCnt; e++, pEnt++) {
        RMat[pEnt->i][pEnt->j] = pEnt->selV;
        hash ^= (((u32)pEnt->selV) << (e & hashMask));
    }

    if (!VerifyLatSq(RMat))
        Report(" BAD RESULT");

    Report(" RESULT # ", srCnt, hash);
    RngTrack.StepsDump();
    if (DbgSt.DmpResult())
        LogClient::Dump(DUMPRSLT);
}

void GEnv::Init() {
    gLevelMax = GEnvGeom::GLevelMax(n);
    pVMap = new MatVis(n);
    NHisto.Init(n, RowFull, ColFull, &Mat);
}

// Inputs array from fname
// Sorts array columns 
// ProcessMat
//
bool GEnv::ReadRaw(string& fname, string sortMode)
{
    InputRaw(fname, sortMode);
    Init();
    ProcessMat();
    return true;
}

// Random generates array (nxn, blkCnt blanks per lane, using seed)
// ProcessMat
//
bool GEnv::GenRaw(int n, int blkCnt, int seed)
{
    RandGenRaw(n, blkCnt, seed);
    Init();
    ProcessMat();
    return true;
}

// Creates and builds *Full arrays
// Fills VDict in *Full lanes
//
void GEnv::ProcessMat()
{
    CreateFull();
    BuildFull();

    // Fill VDict for all full lanes
    for (int l = 0; l < n; l++) {
        RowFull[l]->AbsToLocal();
        ColFull[l]->AbsToLocal();
    }
}

// Sets search boundaries: srLvlFirst, srLvlLast
//  For all GLanes: set gUp, gDn
//
bool GEnv::LinkGLanes()
{
    GLane* pLane, *pLast;
    GLane* pUp;

    pLane = pLanes;
    pLast = pLanes + gLevelMax;

    // Find first non empty lane
    while (pLane <= pLast) {
        if (pLane->entCnt > 0) {
            break;
        }
        pLane++;
    }

    if (pLane > pLast) {
        cerr << "NO BLANK ENTRIES IN SQUARE" << endl;
        return false;
    }

    // pLane is first non empty lane: link up to NULL
    pLane->gUp = GEnvGeom::GLVLNULL;
    srLvlFirst = pLane->gLevel;
    pUp = pLane++;

    while (pLane <= pLast) {
        // pLane is linked up, find next non empty lane
        while (pLane <= pLast) {
            if (pLane->entCnt > 0) {
                // Found next non empty lane: link pUp <-> pLane
                pUp->gDn = pLane->gLevel;
                pLane->gUp = pUp->gLevel;
                pUp = pLane++;
                break;
            } else
                pLane++;
        }
    }
    srLvlLast = pUp->gLevel;
    pUp->gDn = GEnvGeom::GLVLNULL;
    return true;
}

// Scan Mat at lvl
//	Fills gEntries in pLanes[lvl]: 
//      coords, miss*, *Lvl, ent*
//      gIdx, upEdge
//      pPSet*
//      Allocates Stk
//	Fills pLanes[lvl]: 
//		gLevel, idx, isRow
//		entFirst, entLast, entCnt, pEntFirst
//  Fills full lanes NStack
//  Discards unique valued entries and returns discarded count
//
int GEnv::ScanMat(s8 lvl)
{
    GLane* pLane = pLanes + lvl;
    pLane->gLevel = lvl;
    pLane->isRow = GEnvGeom::GLevelIsRow(lvl);
    pLane->idx = GEnvGeom::IdxFromLvl(lvl);
    pLane->entFirst = freeEnt;
    pLane->entCnt = 0;
    int discardCnt = 0;

    s8 i0, iLim;
    s8 j0, jLim;
    GEntry* pEnt;
    EntryBase* pRowPrev = nullptr, * pColPrev = nullptr;
    u8 Dict[BLANKCNTMAX];
    pEnt = pEntries + pLane->entFirst;
    GEnvGeom::SetRange(lvl, i0, iLim, j0, jLim);
    s8 rowLvl, colLvl;

    for (s8 i = i0; i <= iLim; i++) {
        for (s8 j = j0; j <= jLim; j++) {
            if (Mat[i][j] == BLKENT) {
                pEnt->SetMissing(RowFull[i]->absMiss, ColFull[j]->absMiss, Dict, BLANKCNTMAX);
                // Check for unique entries
                if (pEnt->missCnt == 1) {
                    // Entry has unique value: fix Mat and continue
                    Report("UNIQUE REMOVED i/j/v", i, j, Dict[0]);
                    Mat[i][j] = Dict[0];
                    RowFull[i]->RemoveV(Dict[0]);
                    ColFull[j]->RemoveV(Dict[0]);
                    blankCnt--;
                    discardCnt++;
                }
                else {
                    // Entry to be added
                    pEnt->SetCoords(i, j);
                    pEnt->gIdx = freeEnt;
                    rowLvl = RowFull[i]->AddEnt(pEnt, Dict, pRowPrev);
                    colLvl = ColFull[j]->AddEnt(pEnt, Dict, pColPrev);
                    if (pLane->isRow) {
                        pEnt->laneLvl = rowLvl;
                        pEnt->edgeLvl = colLvl;
                        pEnt->upEdge = (pColPrev == nullptr) ? GEntry::GIDXNULL : ((GEntry*)pColPrev)->gIdx;
                        pEnt->pLanePSet = &RowFull[i]->PSet;
                        pEnt->pEdgePSet = &ColFull[j]->PSet;
                    }
                    else {
                        pEnt->laneLvl = colLvl;
                        pEnt->edgeLvl = rowLvl;
                        pEnt->upEdge = (pRowPrev == nullptr) ? GEntry::GIDXNULL : ((GEntry*)pRowPrev)->gIdx;
                        pEnt->pLanePSet = &ColFull[j]->PSet;
                        pEnt->pEdgePSet = &RowFull[i]->PSet;
                    }

                    // Fill in Stk for pEnt
                    pEnt->Stk.Set(stackBase, BLANKCNTMAX);
                    stackBase += BLANKCNTMAX;
                    assert(stackBase <= STKSIZE);

                    pLane->entCnt++;
                    freeEnt++;
                    pEnt++;
                }
            }
        }
    }

    if (pLane->entCnt > 0) {
        // Lane has blank entries
        pLane->entLast = pLane->entFirst + pLane->entCnt - 1;
        pLane->pEntFirst = pEntries + pLane->entFirst;
    }
    else {
        // Empty glane
        // TBD
        pLane->entLast = pLane->entFirst - 1;
        pLane->pEntFirst = NULL;
    }

    return discardCnt;
}

// Compute dnEdge, pDnEdge and changeIdx indices for all entries
//  Set pEntAfter for gLane
//
void GEnv::CompleteLinks(s8 lvl)
{
    GLane* pLane = pLanes + lvl;
    GEntry* pEnt = pLane->pEntFirst;
    GEntry* pDnEnt, *pUpEnt;
    LaneFull* pEdgeLane;
    s8 dnEdLvl, upEdLvl;
    GEntry::GIndex upIdx;
    GEntry::GIndex maxIdx = SHRT_MIN;

    for (int ent = pLane->entFirst; ent <= pLane->entLast; ent++, pEnt++) {
        dnEdLvl = pEnt->edgeLvl + 1;
        pEdgeLane = (pLane->isRow) ? ColFull[pEnt->j] : RowFull[pEnt->i];

        // Down entry
        if (dnEdLvl < pEdgeLane->blankCnt) {
            // Down entry is within square
            pDnEnt = (GEntry *) pEdgeLane->NStack[dnEdLvl].pEnt;
            pEnt->dnEdge = pDnEnt->gIdx;
            pEnt->pDnEdge = pEntries + pEnt->dnEdge;
        }
        else {
            pEnt->dnEdge = GEntry::GIDXNULL;
            pEnt->pDnEdge = nullptr;
        }

        // Change entry
        upEdLvl = pEnt->edgeLvl - 1;
        if (upEdLvl >= 0) {
            // Up entry within square
            pUpEnt = (GEntry*)pEdgeLane->NStack[upEdLvl].pEnt;
            upIdx = pUpEnt->gIdx;
            if (maxIdx < upIdx)
                maxIdx = upIdx;
            pEnt->changeIdx = maxIdx;
        }
        else {
            // Up entry outside of square: take maxIdx or GIDXNULL
            pEnt->changeIdx = maxIdx >= 0 ? maxIdx : GEntry::GIDXNULL;
        }
    }

    // Compute pEntAfter
    if (pLane->entCnt > 0) {
        LaneFull *pThruLane = (pLane->isRow) ? RowFull[pLane->idx] : ColFull[pLane->idx];
        if (pLane->entCnt < pThruLane->blankCnt)
            pLane->pEntAfter = (GEntry *) pThruLane->NStack[pLane->entCnt].pEnt;
        else
            // No further entries after gLane
            pLane->pEntAfter = nullptr;
    }
    else
        // Empty gLane
        pLane->pEntAfter = nullptr;
}

// Creates pEntries, pLanes
//   ScanMat for all lanes
//   CompleteLinks for all lanes
//   LinkGLanes
//   Recomputes blankCnt
//
void GEnv::FillEnts()
{
    // Create blankCnt Entries
    pEntries = new GEntry[blankCnt];

    // Create srLevelMax gLanes
    pLanes = new GLane[gLevelMax + 1];

    // Init StkPool allocation
    stackBase = 0;

    // Fill lanes and NStacks in LaneFull
    int discCnt;
    do {
        discCnt = 0;
        freeEnt = 0;
        // Blank Stacks
        for (int l = 0; l < n; l++) {
            RowFull[l]->BlankStk();
            ColFull[l]->BlankStk();
        }

        for (srLevel = 0; srLevel <= gLevelMax; srLevel++) {
            discCnt += ScanMat(srLevel);
        }
        assert(freeEnt == blankCnt);
        if (DbgSt.reportCreate)
            Report("Discarded count ", discCnt);
    } while (discCnt > 0);

    // Fill entry links
    for (srLevel = 0; srLevel <= gLevelMax; srLevel++) {
        CompleteLinks(srLevel);
    }

    // Link lanes for search traversal
    LinkGLanes();
}

//	FillEnts()
//	Generate PermSets, checks if EPars
//
void GEnv::FillBlanks()
{
    FillEnts();

    GenPermSets();
    if (DbgSt.reportCreate)
        DisplayChangeLinks();
}

//	Generate ExpSets and PSets
//      EPars.makeRef ->    create PermRef
//      EPars.checkRef->    check against PermRef
//
void GEnv::GenPermSets()
{
    // Generate ExpSets and PSets
    // 
    for (int l = 0; l < n; l++) {
        RowFull[l]->PermsGenAndCheck();
        if (DbgSt.reportCreate)
            Report("GEN ROW i/permCnt ", l, RowFull[l]->PSet.Counts.permCnt);
        ColFull[l]->PermsGenAndCheck();
        if (DbgSt.reportCreate)
            Report("GEN COL j/permCnt ", l, ColFull[l]->PSet.Counts.permCnt);
        ColsPermCnt[l] = RowFull[l]->PSet.Counts.permCnt;
    }

    //LinkBlanks();
}

// Display changeLinks
void GEnv::DisplayChangeLinks()
{
    GEntry* pEnt, * pChg;
    s16 chgIdx;
    for (int e = 0; e < blankCnt; e++) {
        pEnt = pEntries + e;
        chgIdx = pEnt->changeIdx;
        if (chgIdx != GEntry::GIDXNULL) {
            // Display link
            pChg = pEntries + chgIdx;
            if (pEnt->i == pChg->i) {
                // Change to left
                pVMap->ArrowLeft(pEnt->i, pChg->j, pEnt->j);
            }
            else if (pEnt->j == pChg->j) {
                // Change up
                pVMap->ArrowUp(pEnt->j, pChg->i, pEnt->i);
            }
            else  if (pEnt->InRow()) {
                // Row entry: left on row, up at pChg
                pVMap->ArrowLeft(pEnt->i, pChg->j, pEnt->j);
                pVMap->ArrowUp(pChg->j, pChg->i, pEnt->i);
            }
            else {
                // Col entry: up on col, left on pChg
                pVMap->ArrowUp(pEnt->j, pChg->i, pEnt->i);
                pVMap->ArrowLeft(pChg->i, pChg->j, pEnt->j);
            }
        }
    }

    for (int e = 0; e < blankCnt; e++) {
        pEnt = pEntries + e;
        pVMap->SetEnt(pEnt->i, pEnt->j);
    }

    pVMap->Out(cout);
}


/////////////////////////////////////////////////////////////////////////
//
//  GLane
//
/////////////////////////////////////////////////////////////////////////

bool GLane::AdvInit()
{
    if (entCnt != 0) {
        pAdv = pEntFirst;
        pAdv->Stk.Reset();
        pAdv->StartPSet();

        advPerms = 0;
        advDepth = 0;
        failDepth = -1;
        retDepth = -1;

        return true;
    }
    else
        return false;
}

// Used instead of AdvInit in change mode
bool GLane::AdvCont(s8 iChg, s8 jChg)
{
    if (entCnt == 0) {
        pAdv = NULL;
        return false;
    }

    advDepth = entCnt - 1;
    pAdv = pEntFirst + advDepth;
    bool lastIn = (isRow) ? pAdv->j >= jChg : pAdv->i >= iChg;
    if (!lastIn || advDepth == 0)
        // Single entry or out of area
        return lastIn;

    // Multi entry within change area
    GEntry* pUp;
    for ( ; advDepth > 0; ) {
        pUp = pAdv - 1;
        if (isRow ? pUp->j < jChg : pUp->i < iChg)
            // pUp is out of area: pAdv is the result
            break;
        // pUp still within change area
        pAdv = pUp;
        advDepth--;
    }

    return true;
}

bool GLane::AdvPerm()
{
    if (entCnt == 0)
        // Empty Lane: always succeeds
        return true;

    if (pAdv == NULL)
        // Top of lane
        //      Only for lanes with entCnt == 1!!!
        return false;

    for (;;) {
        if (!pAdv->Stk.IsEmpty()) {
            pAdv->Pull();
        }
        else {
            if (pAdv->IsExpanded() || !pAdv->Expand(retDepth)) {
                // Failed at pAdv
                if (! AdvClimb())
                    return false;
                else
                    continue;
            }
        }

        // Success for pAdv
        //      Store edge branch
        pAdv->PropagateEdge();
        //      Climb down
        if (advDepth >= entCnt - 1) {
            // Reached end of gLane: success
            AdvSuccess(pAdv->laneDnBr);
            return true;
        }
        else
            // Advance and propagate lane branch
            AdvDown(pAdv->laneDnBr);
    }

    return false;
}

// Move up in generation
//
bool GLane::AdvClimb()
{
    if (failDepth < advDepth)
        failDepth = advDepth;
    if (advDepth <= 0) {
        // At top
        return false;
    }
    else {
        // Inside lane
        pAdv--;
        advDepth--;
        return true;
    }
}

// Move down in generation and pass down lane branch
//
void GLane::AdvDown(LvlBranch dnBr)
{
    advDepth++;
    pAdv++;

    // Propagate branch
    pAdv->laneUpBr = dnBr;
    pAdv->Stk.Reset();
}

// Propagate branch after gLane
//
void GLane::AdvSuccess(LvlBranch afterDnBr)
{
    advPerms++;
    if (pEntAfter != nullptr)
        pEntAfter->edgeUpBr = afterDnBr;
}

/////////////////////////////////////////////////////////////////////////
//
//  GEntry
//
/////////////////////////////////////////////////////////////////////////

bool GEntry::Pull()
{
    if (Stk.IsEmpty())
        return false;

    // Pull branches from stack
    Stk.Pull(laneDnBr, edgeDnBr);
    selV = pLanePSet->LvlGetV(laneDnBr);

    if (DbgSt.DmpExpand())
        EntryBase::DumpPull(selV, pLanePSet, laneDnBr, laneLvl, pEdgePSet, edgeDnBr, edgeLvl);

    return true;
}

void GEntry::ExpandSetReturn(s8& lvl)
{
    if (lvl < 0 && ! AtLaneStart()) {
        // Return not set yet, and entry not at beginning of lane
        lvl = laneLvl;
    }
}

bool GEntry::Expand(s8& retLevel)
{
    Set Opt = 0;
	LvlBranch branch;
	LvlBranch brChk;

	int iCnt = 0;

	if (DbgSt.DmpExpand())
		EntryBase::DumpExpand(pLanePSet, laneUpBr, laneLvl, pEdgePSet, edgeUpBr, edgeLvl);

	bool success = true;
	if (pLanePSet->LvlIsTerm(laneUpBr)) {
		branch = laneUpBr;
		selV = pLanePSet->LvlTermAdv(branch);
		if ((brChk = pEdgePSet->LvlBranchAdv(edgeLvl, edgeUpBr, selV)) != PermSet::LVLNULL) {
            ExpandSetReturn(retLevel);
			laneDnBr = branch;
			edgeDnBr = brChk;
		}
		else
			success = false;
	}
	else if (pEdgePSet->LvlIsTerm(edgeUpBr)) {
		branch = edgeUpBr;
		selV = pEdgePSet->LvlTermAdv(branch);
		if ((brChk = pLanePSet->LvlBranchAdv(laneLvl, laneUpBr, selV)) != PermSet::LVLNULL) {
			laneDnBr = brChk;
			edgeDnBr = branch;
		}
		else
			success = false;
	}
	else {
		// Both lanes have multiple branches: intersect
		LvlBranch* pLnNode = pLanePSet->BranchPtr(laneLvl, laneUpBr);
		LvlBranch* pEdNode = pEdgePSet->BranchPtr(edgeLvl, edgeUpBr);
		iCnt = pLanePSet->LvlIntersect(pLnNode, pEdNode, Opt);

		if (iCnt > 0) {
			if (iCnt == 1) {
				laneDnBr = pLanePSet->Branches[0];
				selV = pLanePSet->LvlGetV(laneDnBr);
				edgeDnBr = pLanePSet->CrBranches[0];

				// Check for singles
				if (pLanePSet->LvlCntIsOne(laneUpBr))
                    ExpandSetReturn(retLevel);
				iCnt = 0;
			}
			else {
				// Multiple intersections
				//
				Stk.Reset();
				laneDnBr = pLanePSet->Branches[0];
				selV = pLanePSet->LvlGetV(laneDnBr);
				//Opt -= selV;
				edgeDnBr = pLanePSet->CrBranches[0];

				// Push rest of branches in potential order
				for (int pos = iCnt - 1; pos >= 1; pos--) {
					Stk.Push(pLanePSet->Branches[pos], pLanePSet->CrBranches[pos]);
				}
				iCnt--;
			}
		}
		else {
			// No intersection
			success = false;
		}
	}

	//		laneUpBr + edgeUpBr have been expanded
	//		selV has been selected
	//		laneDnBr + edgeDnBr have the siblings for selV
	//		other siblings are stored in SStk
    SetExpanded();
	return success;
}

void GEntry::PropagateEdge()
{
    if (! AtEdgeEnd())
        pDnEdge->edgeUpBr = edgeDnBr;
}


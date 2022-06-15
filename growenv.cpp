#include "nenv.h"
#include "growenv.h"
#include "matvis.h"


int GEnv::Search()
{

    int chgCnt = 0;
    SearchInit();

    RngTrack.Init(&cout);

    if (blankCnt <= 0) {
        // No blanks matrix
        Publish();
        return 1;
    }

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
                //Report("  Chg cnt ", chgCnt);
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
            if (DbgSt.doChange) {
                if (pSrLane->AdvDeadEnd()) {
                    // Lane dead ends on this setting: back jump
                    chgCnt++;

                    // Back jump
                    if (SearchBackJump()) {
                        // BJ success: pSrLane already advanced, go down
                        SearchDn();
                    }
                    else {
                        // BJ failed: go up 
                        SearchUp();
                    }
                }
                else {
                    // Go up from current lane
                    SearchUp();
                }

            }
            else {
                // NO debugChange: Lane has processed all Perms, search up
                SearchUp();
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
    pSrLane->AdvInit();

    // Initialize all up branches in all entries
    GEntry* pLim = pEntries + blankCnt;
    for (GEntry* pEnt = pEntries; pEnt < pLim; pEnt++)
        pEnt->InitBranches();
}

void GEnv::SearchUpSuccess()
{
    int advCnt = pSrLane->advPerms;
    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;
    if (DbgSt.DmpExpand())
        Report("  ++ SUCCESS to lvl / advPerms", srLevel, advCnt);
}

void GEnv::SearchUp()
{
    int advCnt = pSrLane->advPerms;
    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;
    if (DbgSt.DmpExpand())
        Report("  UP to lvl / advPerms", srLevel, advCnt);
}

// Back Jump when pSrLane has dead ended
//
bool GEnv::SearchBackJump()
{
    s16 idx;

    // Back Jump
    GEntry* pDEnd = pSrLane->pEntFirst + pSrLane->failDepth;
    idx = pDEnd->bjIdx;
    GEntry *pBJ = pEntries + idx;
    s8 bjLevel = pBJ->gLevel;
    GLane *pBJLane = pLanes + bjLevel;
    pBJLane->pAdv = pBJ;
    pBJLane->advDepth = pBJ->laneLvl;

    if (DbgSt.DmpExpand())
        Report("  BACKJUMP ", bjLevel);

    bool success = false;
    while (pBJLane->AdvPerm()) {
        if (pSrLane->AdvCheckDeadEnd()) {
            success = true;
            if (DbgSt.DmpExpand())
                Report(" CHECKED \n");
            break;
        }
        else
            if (DbgSt.DmpExpand())
                Report(" CHECK FAILED \n");
    }

    srLevel = bjLevel;
    pSrLane = pBJLane;

    if (! success) {
        // Discount the perm that failed
        pBJLane->advPerms--;
    }

    if (DbgSt.DmpExpand()) {
        if (! success)
            if (DbgSt.DmpExpand())
                Report("  BJUMP FAILED ", srLevel);
    }

    return success;
}

// Probe dead ended lane
//
bool GEnv::SearchProbeDeadEnd()
{
    pSrLane->AdvInit();
    return pSrLane->AdvPerm();
}

void GEnv::SearchDn()
{
    srLevel = pSrLane->gDn;
    pSrLane = pLanes + srLevel;
    pSrLane->AdvInit();

    if (DbgSt.DmpExpand())
        Report("  DOWN GRW ", srLevel);
}

void GEnv::Publish()
{
    srCnt++;
    SearchDump();
}

void GEnv::SearchDump()
{
    RMat = Mat;

    // Replace selected values
    GEntry* pEnt = pEntries;
    for (int e = 0; e < blankCnt; e++, pEnt++) {
        RMat[pEnt->i][pEnt->j] = pEnt->selV;
    }

    // Unshuffle result matrix as needed
    if (rowsSorted)
        ShuffleMats(RMat, true, RowUnsort);
    if (colsSorted)
        ShuffleMats(RMat, false, ColUnsort);

    // Generate reult hash
    u32 hash = 0;
    const int hashMask = 0xF;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            hash ^= (((u32)RMat[i][j]) << ((i + j) % 24));
        }
    }

    if (!VerifyLatSq(RMat))
        Report(" BAD RESULT");

    RngTrack.StepsCnt();
    ReportDec();
    Report(" RESULT # hash steps ms", srCnt, hash, RngTrack.totalSteps, (int) RngTrack.ms);
    if (DbgSt.DmpResult())
        LogClient::Dump(DUMPRSLT);
}

void GEnv::Init() {
    gLevelMax = GEnvGeom::GLevelMax(n);
    pVMap = new MatVis(n);
    NHisto.Init(n, RowFull, ColFull, &Mat);
}

// Inputs array (source encoded) from fname
// Inits and ProcessMat
bool GEnv::ReadSrc(string& fname)
{
    if (!Read(fname))
        return false;
    Init();
    ProcessMat();
    return true;
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

// Copy Mat from Src
// Sort according to sortRow and sortCol
// Init and ProcessMat
//
bool GEnv::CopyFrom(EnvBase& Src, string& sortRow, string& sortCol)
{
    CopyRaw(Src, sortRow, sortCol);
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
//      pLanePSet and pEdgePSet
//      Allocates Stk
//	Fills pLanes[lvl]: 
//		gLevel, idx, isRow
//		entFirst, entLast, entCnt, pEntFirst
//  Fills full lanes NStack
//  Discards unique valued entries and returns discarded count
//
int GEnv::ScanMat(s8 lvl, SolveStkEnt* pStkPool)
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
                    pEnt->Stk.Set(pStkPool, stackBase, BLANKCNTMAX);
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
            pEnt->bjIdx = maxIdx;
        }
        else {
            // Up entry outside of square: take maxIdx or GIDXNULL
            pEnt->bjIdx = maxIdx >= 0 ? maxIdx : GEntry::GIDXNULL;
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

// Creates pEntries, pLanes, StkPool
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

    // Create Stack pool
    StkPool = new  SolveStkEnt[STKSIZE];

    // Fill lanes and NStacks in LaneFull
    int discardCnt;
    do {
        // Init StkPool allocation
        stackBase = 0;
        freeEnt = 0;

        discardCnt = 0;
        // Blank Stacks
        for (int l = 0; l < n; l++) {
            RowFull[l]->BlankStk();
            ColFull[l]->BlankStk();
        }

        for (srLevel = 0; srLevel <= gLevelMax; srLevel++) {
            discardCnt += ScanMat(srLevel, StkPool);
        }
        assert(freeEnt == blankCnt);
        if (DbgSt.reportCreate)
            Report("Discarded count ", discardCnt);
    } while (discardCnt > 0);

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
        DisplayBJLinks();
}

//	Generate ExpSets and PSets
//      EPars.makeRef ->    create PermRef
//      EPars.checkRef->    check against PermRef
//      Collect RowsPermCnt[] and ColsPermCnt[]
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

        RowsPermCnt[l] = RowFull[l]->PSet.Counts.permCnt;
        ColsPermCnt[l] = ColFull[l]->PSet.Counts.permCnt;
    }
}

// Display changeLinks
void GEnv::DisplayBJLinks()
{
    GEntry* pEnt, * pBJ;
    s16 chgIdx;
    for (int e = 0; e < blankCnt; e++) {
        pEnt = pEntries + e;
        chgIdx = pEnt->bjIdx;
        if (chgIdx != GEntry::GIDXNULL) {
            // Display link
            pBJ = pEntries + chgIdx;
            if (pEnt->i == pBJ->i) {
                // Change to left
                pVMap->ArrowLeft(pEnt->i, pBJ->j, pEnt->j);
            }
            else if (pEnt->j == pBJ->j) {
                // Change up
                pVMap->ArrowUp(pEnt->j, pBJ->i, pEnt->i);
            }
            else  if (pEnt->InRow()) {
                // Row entry: left on row, up at pBJ
                pVMap->ArrowLeft(pEnt->i, pBJ->j, pEnt->j);
                pVMap->ArrowUp(pBJ->j, pBJ->i, pEnt->i);
            }
            else {
                // Col entry: up on col, left on pBJ
                pVMap->ArrowUp(pEnt->j, pBJ->i, pEnt->i);
                pVMap->ArrowLeft(pBJ->i, pBJ->j, pEnt->j);
            }
        }
    }

    for (int e = 0; e < blankCnt; e++) {
        pEnt = pEntries + e;
        pVMap->SetEnt(pEnt->i, pEnt->j);
    }

    pVMap->Out();
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

bool GLane::AdvPerm()
{
    assert(entCnt > 0);
    assert(pAdv != NULL);

    for (;;) {
        if (!pAdv->Stk.IsEmpty()) {
            pAdv->Pull();
        }
        else {
            if (pAdv->IsExpanded() || !pAdv->Expand(retDepth)) {
                // Failed at pAdv
                if (!AdvClimb())
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

bool GLane::AdvCheckDeadEnd()
{
    pAdv = pEntFirst;
    pAdv->Stk.Reset();
    pAdv->StartPSet();
    advPerms = 0;
    advDepth = 0;
    retDepth = -1;

    for (;;) {
        if (!pAdv->Stk.IsEmpty()) {
            pAdv->Pull();
        }
        else {
            if (pAdv->IsExpanded() || !pAdv->Expand(retDepth)) {
                // Failed at pAdv
                if (advDepth <= 0) {
                    // At top
                    return false;
                }
                else {
                    // Inside lane
                    pAdv--;
                    advDepth--;
                    continue;
                }
            }
        }

        // Success for pAdv
        //      Climb down
        if (advDepth >= failDepth) {
            // Reached previous fail point success
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


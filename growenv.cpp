#include "nenv.h"
#include "growenv.h"
#include "matvis.h"


int GEnv::Search()
{
    int watchCnt = 0;

    if(!SearchInit(SRCHSTD))
        return srCnt;

    for (; ; ) {
        if (DbgSt.DmpTrace())
            DumpTrace();

        RngTrack.Collect(srLevel);
        if (srLevel < srLvlFirst)
            // End of search
            break;

        // Release debugging stop
        if (srLevel >= DbgSt.watchLevel) {
            watchCnt++;
        }

        if (DbgSt.Inspect())
            SearchInspect();

        if (pSrLane->AdvPerm()) {
            // Perm found: go down (or success!)
            if (!SearchDn())
                // Success, with limit reached
                break;
        }
        else {
            // No valid perm left in lane
            if (!SearchUp())
                break;
        }
    }

    SearchEnd();
    return srCnt;
}

//  Search With ladder and DE state checking
//
int GEnv::SearchDEState()
{
    int watchCnt = 0;

    if (!SearchInit(SRCHDEST))
        return srCnt;

    for (; ; ) {
        if (DbgSt.DmpTrace()) {
            DumpTrace();
        }

        RngTrack.Collect(srLevel);

        // Release debugging stop
        if (srLevel >= DbgSt.watchLevel) {
            watchCnt++;
        }

        assert(srLevel <= srLvlLast);

        if (DbgSt.Inspect())
            SearchInspect();

        if (pSrLane->AdvPermDEState(DEActiveOn())) {
            // Found valid perm
            if (DEActiveOn())
                // Process DEState info
                SearchDnDEState();

            // Continue to level down (or success!)
            if (!SearchDn())
                // Success, with limit reached
                break;
        }
        else {
            // No valid perm left in lane
            if (!SearchUpDEState())
                // End of search
                break;
        }

        if (DbgSt.Inspect())
            SearchDEInspDEAct();
    }

    SearchEnd();
    return srCnt;
}

// Search down action when dead end is active
//
void GEnv::SearchDnDEState()
{
    if (pSrLane->DEStateOn()) {
        DEActiveDec();

        // Reset DE related info
        pSrLane->DEStateReset();
        // Erase touched marks above
        pSrLane->XferAbove(0, pSrLane->xferLast, false);
        pSrLane->XferReset();

        if (DbgSt.DmpTrace()) {
            ReportDec();
            Report("DEOFF lvl deActive", srLevel, deActive);
        }
    }
    else if (pSrLane->Touched()) {
        // Erase touched marks above
        pSrLane->XferAbove(0, pSrLane->xferLast, false);
        pSrLane->XferReset();
    }
}

// Lane has a valid perm
bool GEnv::SearchDn()
{
    if (srLevel >= srLvlLast) {
        // Found a solution
        Publish();
        if (!SearchUpSuccess())
            // Search limit reached
            return false;
        else
            // Will continue searching
            return true;
    }

    // Go down
    srLevel = pSrLane->gDn;
    pSrLane = pLanes + srLevel;
    pSrLane->AdvInit();
    if (!DEActiveOn()) {
        pSrLane->DEStateReset();
        pSrLane->TouchReset();
        pSrLane->XferReset();
    }

    if (DbgSt.DmpTrace()) {
        DumpInsCnt();
        ReportDec();
        Report("DOWN TO ", srLevel);
    }
    return true;
}

bool GEnv::SearchUpDEState()
{
    if (srLevel <= srLvlFirst)
        // End of search
        return false;

    if (pSrLane->AdvDeadEnd()) {
        // Lane dead ends (NO success) on this setting
        if (pSrLane->DEStateOn()) {
            // Lane already was dead ended -- correct touches
            s8 xferNew = pSrLane->TouchCount();
            if (xferNew < pSrLane->failDepth)
                xferNew = pSrLane->failDepth;
            pSrLane->XferFixAbove(xferNew);
        }
        else {
            // Lane was not dead ended 
            // Increase dead end count
            DEActiveInc();
            // And transfer all touches above
            pSrLane->XferCountFromMask();
            // Correct for failDepth
            if (pSrLane->xferLast < pSrLane->failDepth)
                pSrLane->xferLast = pSrLane->failDepth;
            pSrLane->XferAbove(0, pSrLane->xferLast, true);
        }

        //  Save Dead End state: sets deLast and deClosest
        pSrLane->DEStateSave();

        if (DbgSt.DmpTrace()) {
            //DumpInsCnt();
            ReportDec();
            Report("DEON lvl deActive fail xfer",
                srLevel, deActive, pSrLane->failDepth, pSrLane->xferLast);
        }
        deCnt++;
    }
    else if (pSrLane->DEStateOn()) {
        // Lane was dead ended but not anymore
        DEActiveDec();
        pSrLane->DEStateReset();
        // Correct touches to ONLY touch mask
        pSrLane->XferFixAbove(pSrLane->TouchCount());
    }
    else if (pSrLane->Touched()) {
        // Lane was touched or dead ended: transfer touches above
        pSrLane->XferCountFromMask();
        pSrLane->XferAbove(0, pSrLane->xferLast, true);
    }

    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;

#define SKIPLADDER
#ifdef SKIPLADDER
    if (DEActiveOn() /* && pSrLane->AdvDeadEnd() */) {
        // Skip untouched lanes
        while (!pSrLane->Touched()) {
            srLevel = pSrLane->gUp;
            pSrLane = pLanes + srLevel;
        }
    }
#endif SKIPLADDER

    if (DbgSt.DmpTrace()) {
        DumpInsCnt();
        ReportDec();
        Report("UPTO  lvl ", srLevel);
    }

    return true;
}

// Propagate branch after gLane
//
void GLane::AdvSuccess(LvlBranch afterDnBr)
{
    advPerms++;
    if (pEntAfter != nullptr && (advDepth == entCnt - 1))
        pEntAfter->edgeUpBr = afterDnBr;
}

// Generate next perm with DE State check
//		Iterates from pAdv until reaching advDepth >= advLast
//
bool GLane::AdvPermDEState(bool deActive)
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
        //      Store edge branch -- check DE State
        if (!AdvPropDEState(deActive, pAdv->pDnEdge, pAdv->edgeDnBr))
            continue;

        //      Climb down
        if (advDepth >= advLast) {
            // Reached end of gLane: success unless DE State fails
            if (!AdvPropDEState(deActive, pEntAfter, pAdv->laneDnBr))
                continue;
            advPerms++;
            return true;
        }
        else
            // Advance and propagate lane branch
            AdvDown(pAdv->laneDnBr);
    }

    return false;
}

// Propagate branch after gLane with DE State Check
bool GLane::AdvPropDEState(bool deActive, GEntry* pDn, LvlBranch br)
{
    if (pDn == NULL)
        return true;

    if (deActive) {
        GLane* pTargLn = this + (pDn->gLevel - gLevel);
        if (pTargLn->DEStateOn()                    /* Target lane is dead end */
            && (pDn->laneLvl <= pTargLn->deLast)) { /* and pDn in dead end area */
            pTargLn->deChgCnt += pDn->EdgeStateChange(br);
#if 0
            if (!pTargLn->AdvPropVerify())
                cerr << " ADV PROP VERIFY FAILED" << endl;
#endif
            if ((pDn->laneLvl == pTargLn->deClosest)    /* Target closed */
                && pTargLn->deChgCnt == 0               /* With no state change */
                && !pTargLn->WasRejected()) {           /* And it is DEnded by state */
                if (DbgSt.dumpTrace)
                    cerr << " >>> ADV PROP REJECTED lvl: " << (int)(pTargLn->gLevel)
                        << " closest: " << (int)(pTargLn->deClosest)
                        << " deLast: " << (int)(pTargLn->deLast)
                        << endl;
                advRejCnt++;
                return false;
            }
            else
                return true;
        }
    }

    // No deActive or target lane is not dead ended
    pDn->edgeUpBr = br;
    return true;
}

bool GLane::AdvPropVerify()
{
    s8 chgCnt = 0;
    for (s8 d = 0; d <= deLast; d++) {
        if (pEntFirst[d].EdgeStateDiff())
            chgCnt++;
    }
    if (chgCnt != deChgCnt)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////
// 
//  INSPECT
// 
////////////////////////////////////////////////////////////////

// Inspect GEnv
//
bool GEnv::SearchInspect()
{
    insCnt++;

    if (DbgSt.InspectPause()) {
        if (insCnt == DbgSt.inspect) {
            cerr << ">>>> INSPECT PAUSE >>>>" << endl;
        }
    }

    if (srType != SearchType::SRCHDEST)
        return true;

    for (insLvl = srLvlFirst; insLvl < srLevel; ) {
        pInsp = pLanes + insLvl;
        SearchDEInspAbove();
        insLvl = pInsp->gDn;
    }
    SearchDEInspSrLevel();
    if (srLevel >= srLvlLast)
        return true;

    for (insLvl = pSrLane->gDn; ; ) {
        pInsp = pLanes + insLvl;
        SearchDEInspBelow();
        if (insLvl >= srLvlLast)
            break;
        insLvl = pInsp->gDn;
    }
    return true;
}

bool GEnv::SearchDEInspDEAct()
{
    if (!DEActiveOn())
        return true;
    insDEActive = 0;

    for (insLvl = srLevel; ; ) {
        pInsp = pLanes + insLvl;
        if (pInsp->DEStateOn())
            insDEActive++;
        if (insLvl >= srLvlLast)
            break;
        insLvl = pInsp->gDn;
    }

    if (insDEActive != deActive) {
        SearchDEInspError();
        return false;
    }
    else
        return true;
}

bool GEnv::SearchDEInspAbove()
{
    s8 d;
    GEntry* pEnt;
    if (pInsp->DEStateOn())
        SearchDEInspError();
    if (pInsp->XferWasDone())
        SearchDEInspError();
    if (DEActiveOn()) {
        for (d = 0; d < pInsp->entCnt; d++) {
            if (pInsp->TouchedEntry(d)) {
                pEnt = pInsp->pEntFirst + d;
                // Transfer has to happen from srLevel or below
                if (pEnt->dnEdge < pSrLane->entFirst)
                    SearchDEInspError();
            }
        }

        // Check pEntAfter touch
        if (pInsp->TouchedEntry(pInsp->entCnt)) {
            pEnt = pInsp->pEntAfter;
            // Transfer has to happen from srLevel or below
            if (pEnt->gIdx < pSrLane->entFirst)
                SearchDEInspError();
        }

    }
    else {
        if (pInsp->Touched())
            SearchDEInspError();
    }
    return true;
}

bool GEnv::SearchDEInspSrLevel()
{
    return true;
}

bool GEnv::SearchDEInspBelow()
{
    if (insLvl <= 0)
        SearchDEInspError();
    if (DEActiveOn() && (insLvl <= deBottom)) {
        if ((pInsp->DEStateOn() || pInsp->Touched()) && !pInsp->XferWasDone())
            SearchDEInspError();
    }
    else {
        if (pInsp->DEStateOn())
            SearchDEInspError();
        if (pInsp->XferWasDone())
            SearchDEInspError();
        if (pInsp->Touched())
            SearchDEInspError();
    }

    return true;
}

bool GEnv::SearchDEInspError()
{
    if (insCntSave < 0) {
        insCntSave = insCnt;
        cerr << "FIRST INSPECT ERROR at " << insCnt << endl;
    }
    return true;
}

// Initialize GEnv
// Returns false if matrix has no blanks
//
bool GEnv::SearchInit(SearchType _srType)
{
    srType = _srType;

    srCnt = 0;
    srHash = 0;
    deCnt = 0;

    // Init lanes state
    for (srLevel = srLvlFirst; ; srLevel = pSrLane->gDn) {
        pSrLane = pLanes + srLevel;
        pSrLane->DEStateReset();
        pSrLane->TouchReset();
        pSrLane->XferReset();
        pSrLane->touchEndBit = Set32::IdxToMask(pSrLane->entCnt);
        if (srLevel >= srLvlLast)
            break;
    }

    srLevel = srLvlFirst;
    pSrLane = pLanes + srLvlFirst;
    pSrLane->AdvInit();

    // Initialize all up branches in all entries
    GEntry* pLim = pEntries + blankCnt;
    for (GEntry* pEnt = pEntries; pEnt < pLim; pEnt++)
        pEnt->InitSearchState();

    // Initialize search lane table
    ChkLdr.Init(gLevelMax, pEntries, pLanes, srType);
    RngTrack.Init(&cout);
    DEActiveReset();
    SearchDEInspInit();

    // Name
    switch (srType) {
    case SRCHSTD:
        srTypeName = "STD SEARCH";
        break;
    case SRCHLADR:
        srTypeName = "LADDER SEARCH";
        break;
    case SRCHDEST:
        srTypeName = "DESTATE SEARCH";
        break;
    default:
        cerr << "Bad search type" << endl;
        exit(1);
    }
    if (DbgSt.reportSearch)
        RepEndl(srTypeName.c_str());

    if (blankCnt <= 0 || (srLevel < srLvlFirst)) {
        // No blanks matrix
        Publish();
        if (srLevel < srLvlFirst) {
            RepEndl("EMPTY MATRIX");
        }
        return false;
    }
    return true;
}

bool GEnv::SearchUp()
{
    if (srLevel <= srLvlFirst)
        // End of search
        return false;

    int advCnt = pSrLane->advPerms;
    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;
    if (DbgSt.DmpTrace()) {
        ReportDec();
        Report("UP to lvl / advPerms", srLevel, advCnt);
    }
    return true;
}

int GEnv::SearchLadder()
{
    int watchCnt = 0;

    if (!SearchInit(SRCHLADR))
        return srCnt;

    for (; ; ) {
        if (DbgSt.DmpTrace())
            DumpTrace();

        RngTrack.Collect(srLevel);

        // Release debugging stop
        if (srLevel >= DbgSt.watchLevel) {
            watchCnt++;
        }

        assert(srLevel <= srLvlLast);

        if (pSrLane->AdvPerm()) {
            // Lane has a perm, valid to advLast
            if (!ChkLdr.IsEmpty() && ChkLdr.LevelIsIn(srLevel)) {
                // Ladder is on and lane is in ladder: Satisfy dead ends
                if (!SearchCheckDeadEnds(srLevel)) {
                    // Check failed, so perm is not valid: continue at this level
                    pSrLane->AdvPermFailed();
                    continue;
                }
            }
            // Continue to level down (or success!)
            if (!SearchDn())
                // Success, with limit reached
                break;
        }
        else {
            // No valid perm left in lane
            if (!SearchUpLdrStep())
                //if (!SearchUpLdr())
                break;
        }
    }

    SearchEnd();
    return srCnt;
}

// Back Jump when pSrLane has dead ended
//
void GEnv::SearchBackJump()
{
    // Move up to first entry in ChkLdr 
    s8 bjLvl = ChkLdr.MoveUp(srLevel);
    assert(bjLvl != GEnvGeom::GLVLNULL);        // We just inserted predecessors!

    if (DbgSt.DmpTrace()) {
        DumpInsCnt();
        ReportDec();
        Report("BACKJUMP from / to", srLevel, bjLvl);
    }

    pSrLane = pLanes + bjLvl;
    srLevel = bjLvl;
}

// After successful AdvPerm at fromLvl, perform a check visit down the ladder
//	If bottom cannot be reached, return false
//
bool GEnv::SearchCheckDeadEnds(s8 fromLvl)
{
    GLane* pChkLane;
 
    s8 chkLvl = ChkLdr.ChkLvlFirst(fromLvl);
    assert(chkLvl != GEnvGeom::GLVLNULL);
    pChkLane = pLanes + chkLvl;

#ifdef DESTATE
    // If next lane Dead Ended: verify Dead End state change
    if (pChkLane->IsDeadEnded()) {
        if (!pSrLane->DeadEndStateDiff())
            // No change in Dead End State, so fail
            return false;
    }
#endif // DESTATE

    pChkLane->AdvInit(false);

    if (DbgSt.DmpTrace()) {
        DumpInsCnt();
        Rep("  >>> CHECK DE at lvl ");
        ReportDec();
        Rep(srLevel);
        RepEndl();
    }

    for ( ; ; ) {
        if (pChkLane->AdvPerm()) {
            // Lane check succeded: go down ladder
            chkLvl = ChkLdr.ChkLvlDown(chkLvl);
            if (chkLvl == GEnvGeom::GLVLNULL) {
                // Reached ChkLdr bottom: successful check
                if (DbgSt.DmpTrace()) {
                    Rep("  >>> CHECK SUCCESS");
                    RepEndl();
                }
                // Check succeeded, complete advance to end of lane
                if (pSrLane->AdvPermComplete()) {
                    // Completion succeeded, so dead ends will not occur
                    // Clean Ladder
                    ChkLdr.Clean();
                    return true;
                }
                else
                    // Completion failed
                    return false;
            }
            pChkLane = pLanes + chkLvl;

#ifdef DESTATE
            // If next lane Dead Ended: verify Dead End state change
            if (pChkLane->IsDeadEnded()) {
                if (!pChkLane->DeadEndStateDiff())
                    // No change in Dead End State, so fail
                    return false;
            }
#endif // DESTATE
            pChkLane->AdvInit(false);
        }
        else {
            // Climb ladder up
            chkLvl = ChkLdr.ChkLvlUp(chkLvl);

            if (chkLvl == GEnvGeom::GLVLNULL) {
                // Back to fromLvl : check failed
                if (DbgSt.DmpTrace()) {
                    Rep("  >>> CHECK FAILED >>>>>>>>>>>>>");
                    RepEndl();
                }
                return false;
            }
            pChkLane = pLanes + chkLvl;
        }
    }
    return true;
}

// Search up within ChkLadder
//  Insert Lane in ladder as needed
//  Find up level
//
bool GEnv::SearchUpLdr()
{
    if (srLevel <= srLvlFirst)
        // End of search
        return false;

    s8 climbLvl;

    if (pSrLane->AdvDeadEnd()) {
        // Lane dead ends (NO success) on this setting: back jump
        //  Insert pSrLane into ladder, restricted by dead end depth
        ChkLdr.InsertLane(pSrLane, pSrLane->failDepth);
        // Move up to first entry in ChkLdr 
        climbLvl = ChkLdr.MoveUp(srLevel);
        if (DbgSt.DmpTrace()) {
            DumpInsCnt();
            ReportDec();
            Report("BACKJUMP from / to", srLevel, climbLvl);
        }

        deCnt++;
    }
    else {
        // Lane has processed all perms (with at least ONE success) on this setting
        if (!ChkLdr.IsEmpty()) {
            //  Insert pSrLane into ladder, restrict according to ladder
            ChkLdr.InsertLane(pSrLane, -1);
            // Move up to first entry in ChkLdr 
            climbLvl = ChkLdr.MoveUp(srLevel);
        }
        else
            climbLvl = pSrLane->gUp;
    }

    if (climbLvl == GEnvGeom::GLVLNULL) {
        // Above ladder: use normal climb structure
        cerr << "ERROR ????" << endl;
        SearchUp();
        // Exit ladder
        ChkLdr.Clean();
        return false;;
    }

    srLevel = climbLvl;
    pSrLane = pLanes + srLevel;
    if (DbgSt.DmpTrace()) {
        ReportDec();
        Report("UPLADDER lvl / advDp", srLevel, (int)pSrLane->advDepth);
    }
    return true;
}

bool GEnv::SearchUpLdrStep()
{
    if (srLevel <= srLvlFirst)
        // End of search
        return false;

    if (pSrLane->AdvDeadEnd()) {
        // Lane dead ends (NO success) on this setting: back jump
        //  Insert pSrLane into ladder, restricted by dead end depth
        ChkLdr.InsertLane(pSrLane, pSrLane->failDepth);

#ifdef DESTATE
        //  Save Dead End state 
        pSrLane->DeadEndStateSave(pSrLane->failDepth);
#endif // DESTATE

        if (DbgSt.DmpTrace()) {
            ReportDec();
            Report("INSERTJUMP lvl ", srLevel);
        }
        deCnt++;
    }
    else {
        // Lane has processed all perms (with at least ONE success) on this setting
        if (!ChkLdr.IsEmpty()) {
            //  Insert pSrLane into ladder, restrict according to ladder
            ChkLdr.InsertLane(pSrLane, -1);
        }
    }

    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;
    //if (ChkLdr.LevelIsIn(srLevel))
    //    pSrLane->AdvPermFailed();
    if (DbgSt.DmpTrace()) {
        ReportDec();
        Report("UPLADDER lvl / advDp", srLevel, (int)pSrLane->advDepth);
    }
    return true;
}

bool GEnv::SearchUpSuccess()
{
    int advCnt = pSrLane->advPerms;
    srLevel = pSrLane->gUp;
    pSrLane = pLanes + srLevel;
    if (DbgSt.DmpTrace()) {
        DumpInsCnt();
        ReportDec();
        Report("  ++ SUCCESS to lvl / advPerms", srLevel, advCnt);
    }

    RngTrack.Init(&cout);
    deCnt = 0;
    return DbgSt.SearchLim() > 0;
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

    // Generate result hash
    u32 hash = 0;
    const int hashMask = 0xF;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            hash ^= (((u32)RMat[i][j]) << ((i + j) % 24));
        }
    }

    srHash ^= (((u64)hash) << (srCnt % 32));

    if (!VerifyLatSq(RMat))
        Report(" BAD RESULT");

    if (DbgSt.reportSearch) {
        DumpInsCnt();
        RngTrack.StepsCnt();
        ReportDec();
        Rep(" RESULT # hash steps ms deCnt ");
        Rep(srCnt);
        ReportHex();
        Rep(hash, 9);
        ReportDec();
        Rep(RngTrack.totalSteps, 17);
        Rep((int)RngTrack.ms);
        Rep(deCnt);
        RepEndl();
        if (DbgSt.DmpResult())
            LogClient::Dump(DUMPRSLT);
    }
}

void GEnv::SearchEnd()
{
    if (DbgSt.reportSearch) {
        DumpInsCnt();
        Rep(srTypeName);
        Rep("  solutions: ");
        ReportDec();
        Rep(srCnt);

        RngTrack.StepsCnt();
        Rep("  final steps:");
        Rep(RngTrack.totalSteps, 17);
        ReportHex();
        Rep(" srHash:");
        Rep(srHash, 17);
        RepEndl();
    }
}

void GEnv::Init()
{
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
                    if (DbgSt.reportCreate)
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
    pLane->targLast = -1;

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

        // Back jump entry
        upEdLvl = pEnt->edgeLvl - 1;
        if (upEdLvl >= 0) {
            // Up entry within square
            pUpEnt = (GEntry*)pEdgeLane->NStack[upEdLvl].pEnt;
            pEnt->pUpEdge = pUpEnt;
            upIdx = pUpEnt->gIdx;
            if (maxIdx < upIdx) {
                pLane->targLast++;
                maxIdx = upIdx;
                pLane->BJTarg[pLane->targLast] = maxIdx;
            }
            pEnt->tgIdx = pLane->targLast;
        }
        else {
            // Up entry outside of square: take maxIdx or GIDXNULL
            pEnt->tgIdx = maxIdx >= 0 ? pLane->targLast : GEntry::NULLTARGET;
            pEnt->pUpEdge = NULL;
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
    s8 tIdx;
    s8 gLvl;
    s16 jmpIdx;
    for (int e = 0; e < blankCnt; e++) {
        pEnt = pEntries + e;
        tIdx = pEnt->tgIdx;
        if (tIdx != GEntry::NULLTARGET) {
            gLvl = GEnvGeom::GLevel(pEnt->i, pEnt->j);
            jmpIdx = pLanes[gLvl].BJTarg[tIdx];
            pBJ = pEntries + jmpIdx;

            // Display link
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

u64 GEnv::Hash()
{
    hash = 0;

    GLane* pLane = pLanes;
    for (s8 l = 0; l < srLevel; l++, pLane++)
        hash ^= pLane->Hash();
    hash ^= pSrLane->HashCurrent();
    return hash;
}

void GEnv::DumpInsCnt()
{
    if (DbgSt.Inspect()) {
        ReportDec();
        Rep("[");
        Rep(insCnt, 6);
        Rep("] ");
    }
}

void GEnv::DumpTrace()
{
    if (DbgSt.DmpExpand())
        RepEndl();

    DumpInsCnt();

    Rep(pSrLane->pEntFirst->pLanePSet->searchId);
    Rep(" TRC LVL ");
    ReportDec();
    Rep(srLevel, 2);
    Rep(" dp ");
    Rep(pSrLane->advDepth);
    Rep(" Hash Ln ");
    ReportHex();
    Rep(pSrLane->HashCurrent(), 17);
    Rep(" Hash Env ");
    Rep(Hash(), 17);
    RepEndl();
}

/////////////////////////////////////////////////////////////////////////
//
//  GLane
//
/////////////////////////////////////////////////////////////////////////

u64 GLane::Hash()
{
    hash = 0;
    GEntry* pEnt = pEntFirst;

    for (int d = 0; d < entCnt; d++, pEnt++) {
        hash ^= pEnt->HashInLane();
    }
    return hash;
}

u64 GLane::HashCurrent()
{
    hash = 0;
    GEntry* pEnt = pEntFirst;

    for (int d = 0; d <= advDepth; d++, pEnt++) {
        hash ^= pEnt->HashInLane();
    }
    return hash;
}

bool GLane::AdvInit(bool setLast)
{
    if (entCnt != 0) {
        pAdv = pEntFirst;
        pAdv->Stk.Reset();
        pAdv->StartPSet();

        advPerms = 0;
        advRejCnt = 0;
        advDepth = 0;
        failDepth = -1;
        retDepth = -1;
        
        if (setLast) {
            AdvUnRestrict();
        }
        return true;
    }
    else
        return false;
}

// Generate next perm
//		Iterates from pAdv until reaching advDepth >= advLast
//
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
        if (advDepth >= advLast) {
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

// Complete perm generated by AdvPerm()
//		Iterates from current (pAdv, advDepth) until reaching advDepth >= entCnt - 1
//
bool GLane::AdvPermComplete()
{
    if (advLast == entCnt - 1)
        // Nothing to complete
        return true;

    // Extend to full lane
    s8 prevLast = advLast;
    AdvUnRestrict();
    AdvDown(pAdv->laneDnBr);

    if (AdvPerm()) {
        // Successful completion: do not count advPerms twice!
        advPerms--;
        return true;
    }
    else {
        // Could not extend: restrict again
        advLast = prevLast;
        return false;
    }
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
    selV = PermSet::LvlGetV(laneDnBr);

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
	if (PermSet::LvlIsTerm(laneUpBr)) {
		branch = laneUpBr;
		selV = PermSet::LvlTermAdv(branch);
		if ((brChk = pEdgePSet->LvlBranchAdv(edgeLvl, edgeUpBr, selV)) != PermSet::LVLNULL) {
            ExpandSetReturn(retLevel);
			laneDnBr = branch;
			edgeDnBr = brChk;
		}
		else
			success = false;
	}
	else if (PermSet::LvlIsTerm(edgeUpBr)) {
		branch = edgeUpBr;
		selV = PermSet::LvlTermAdv(branch);
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
				selV = PermSet::LvlGetV(laneDnBr);
				edgeDnBr = pLanePSet->CrBranches[0];

				// Check for singles
				if (PermSet::LvlCntIsOne(laneUpBr))
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

u32 GEntry::Hash()
{
    hash = laneUpBr ^ edgeUpBr ^ (((int)selV) << 24) ^ Stk.Hash();
    return hash;
}

u64 GEntry::HashInLane()
{
    Hash();
    u64 hl = hash;
    return hl << (laneLvl * 3);
}

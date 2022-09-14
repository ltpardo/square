#pragma once
#include "stdinc.h"
#include "nlane.h"
#include <iomanip>
#include <fstream>
#include "nenv.h"
#include "matvis.h"

typedef enum {
	SRCHSTD, SRCHLADR, SRCHDEST
} SearchType;
enum { STKSIZE = DIMMAX * BLANKCNTMAX * BLANKCNTMAX };

class GStk {
public:
	GStk() : base(0), size(0), free(0) {}

	SolveStkEnt* pBase;
	u16 base;
	u8 size;
	u8 free;

	void Set(SolveStkEnt* pStkPool, s16 _base, u8 _size) {
		pBase = pStkPool + _base;
		base = _base;
		size = _size;
		Reset();
	}

	void Reset() {
		free = 0;
	}

	inline bool IsEmpty() { return free == 0; }

	void Push(LvlBranch thBr, LvlBranch crBr) {
		assert(free < size);
		pBase[free].Set(thBr, crBr);
		free++;
	}

	bool Pull(LvlBranch& thBr, LvlBranch& crBr) {
		assert(free > 0);
		free--;
		thBr = pBase[free].thBr;
		crBr = pBase[free].crBr;
		return ! IsEmpty();
	}

	u32 Hash() {
		u32 h = 0;
		for (int p = 0; p < free; p++) {
			h ^= pBase[p].thBr;
			h ^= pBase[p].crBr;
		}
		return h;
	}
};

class GEnvGeom {
public:
	GEnvGeom() {}
	static inline bool GLevelIsRow(s8 lvl) { return (lvl & 1) != 0; }
	static inline s8 GLevelMax(int n) { return 2 * (n - 1); }
	static inline s8 IdxFromLvl(s8 lvl) { return (lvl + 1) >> 1; }
	static s8 GLevel(s8 i, s8 j) {
		if (i > j)
			// Row
			return 2 * i - 1;
		else
			// Col
			return 2 * j;
	}

	static const s8 GLVLNULL = -1;

	// Ranges
	//			  i		  j
	//	Row		 idx 	< idx
	//	Col		<= idx	  idx
	static void SetRange(s8 lvl, s8 &i0, s8& i1, s8& j0, s8& j1) {
		s8 idx = IdxFromLvl(lvl);
		if (GLevelIsRow(lvl)) {
			i0 = i1 = idx;
			j0 = 0;
			j1 = idx - 1;
		}
		else {
			i0 = 0;
			i1 = idx;
			j0 = j1 = idx;
		}
	}

	static inline bool PosIsRow(s8 i, s8 j) { return i > j; }
};


class GEntry : public EntryBase {
public:
	GEntry() : EntryBase(), gIdx(-1), upEdge(0), dnEdge(0), gLevel(0)
	{
		Stk.Reset();
	}

	typedef s16 GIndex;
	static const GIndex GIDXNULL = -1;

	GIndex gIdx;

	// Linking: 
	//		Axis are lane and edge
	//		Directions are up or down
	GIndex upEdge;
	GIndex dnEdge;
	GEntry* pUpEdge;
	GEntry* pDnEdge;

	// Lanes and lane levels
	s8 laneLvl;
	s8 edgeLvl;
	bool AtLaneStart() { return laneLvl <= 0; }
	bool AtEdgeEnd() { return dnEdge == GIDXNULL; }

	// State Hash
	u32 hash;
	u32 Hash();
	u64 HashInLane();

	// Coords: correspond to thru and cross lane indices
	// s8 i, j;
	s8 gLevel;
	inline bool InRow() { return GEnvGeom::PosIsRow(i, j); }

	void SetCoords(s8 _i, s8 _j) {
		i = _i;
		j = _j;
		gLevel = GEnvGeom::GLevel(_i, _j);
	}

	// Expansion stack
	GStk Stk;
	bool Pull();

	// Search data
	PermSet* pLanePSet;
	PermSet* pEdgePSet;
	LvlBranch edgeUpBr, edgeDnBr, laneUpBr, laneDnBr;
	void InitSearchState() {
		edgeUpBr = 0;
		laneUpBr = 0;
		edgeDnBr = PermSet::BRANCHNULL;
		laneDnBr = PermSet::BRANCHNULL;
		EdgeStateSave();

		selV = 0xFF;
		Stk.Reset();
	}

	inline void SetExpanded() { laneUpBr = PermSet::BRANCHNULL; }
	inline bool IsExpanded() { return laneUpBr == PermSet::BRANCHNULL; }

	inline void StartPSet() { laneUpBr = 0; }
	void PropagateEdge();

	// Edge State
	LvlBranch edgeBrSave;
	inline void EdgeStateSave()  { edgeBrSave = edgeUpBr;	}
	inline bool EdgeStateDiff()  { return edgeBrSave != edgeUpBr; }
	inline bool EdgeStateEqual() { return edgeBrSave == edgeUpBr; }

	// Set entry edge state: returns the variation in lane total change count
	inline int EdgeStateChange(LvlBranch newBr) {
		bool prevEq = EdgeStateEqual();
		edgeUpBr = newBr;
		if (prevEq)
			return EdgeStateEqual() ? 0 : 1;
		else
			return EdgeStateEqual() ? -1 : 0;
	}


	void ExpandSetReturn(s8& lvl);
	bool Expand(s8& retLevel);

	// Back Jump data
	static const s8 NULLTARGET = -1;
	s8 tgIdx;

	void DumpExpand(bool isRow = false);
};

class GLane : public LogClient {
public:
	GLane() : gLevel(0), gUp(GEnvGeom::GLVLNULL), gDn(GEnvGeom::GLVLNULL),
		idx(-1), isRow(false),
		pEntFirst(NULL), entFirst(-1), entLast(-1), entCnt(-1),
		pSet(NULL)
	{}

	void Init(int _gLevel, int _entFirst, int _entCnt, GEntry *pEntTab) {
		gLevel = _gLevel;
		isRow = GEnvGeom::GLevelIsRow(_gLevel);
		entFirst = _entFirst;
		entCnt = _entCnt;
		entLast = entFirst + entCnt - 1;
		pEntFirst = pEntTab + entFirst;

		for (int t = 0; t < BLANKCNTMAX; t++) {
			BJTarg[t] = GEntry::GIDXNULL;
		}
		targLast = -1;
		TouchReset();
		touchEndBit = Set32::IdxToMask(entCnt);
		XferReset();
	}

	s8 gLevel;			// Lane level
	s8 gUp;				// Previous lane
	s8 gDn;				// Next lane
	s8 idx;				
	bool isRow;

	inline bool IsLaneParallel(GLane* pLane) {
		return isRow ? pLane->isRow : !pLane->isRow;
	}

	inline bool IsLaneOrthogonal(GLane* pLane) {
		return isRow ? !pLane->isRow : pLane->isRow;
	}

	// State Hash
	u64 hash;
	u64 Hash();
	u64 HashCurrent();

	GEntry* pEntFirst;	// First entry in gLane
	int entFirst;
	int entLast;
	int entCnt;
	GEntry* pEntAfter;	// Entry to propagate last branch of gLane

	// Back Jump targets
	int targLast;
	GEntry::GIndex BJTarg[BLANKCNTMAX];

	PermSet* pSet;

	// Advance info
	GEntry* pAdv;		// Current entry during generation	
	s8 advDepth;		// Current depth during generation
	s8 advLast;			// Max Depth for AdvPerm()
	s8 retDepth;		// Return depth after failure for terminal perm branches
	s8 failDepth;		// Maximum depth reached on a failed generation
	int advPerms;		// Lane advance has generated advPerms already
	int advRejCnt;		// Propagate rejection count during current AdvPerm

	inline bool WasRejected() {	return advRejCnt > 0; }

	/////////////////////////////////////////////
	//
	//		TOUCH STATE
	//
	u32 touchMask;
	u32 touchEndBit;		// pEntAfter mask entry
	s8 touchLast;

	inline s8 TouchCount() {
		if ((touchMask & touchEndBit) != 0)
			// "Cross" propagation happened
			touchLast = entCnt - 1;
		else
			touchLast = Set32::GetLastIdx(touchMask);
		return touchLast;
	}

	void TouchReset() {
		touchMask = 0;
		touchLast = -1;
	}

	bool Touched() { return touchMask != 0; }

	bool TouchedEntry(s8 d) {
		return Set32::IdxBelongs(touchMask, d);
	}

	inline void TouchLvl(s8 lvl) {
		Set32::SetIdx(touchMask, lvl);
	}
	void TouchOn(GEntry* pEnt) {
		assert(pEnt->gIdx >= entFirst && pEnt->gIdx <= entLast);
		Set32::SetIdx(touchMask, pEnt->laneLvl);
	}
	void TouchOff(GEntry* pEnt) {
		assert(pEnt->gIdx >= entFirst && pEnt->gIdx <= entLast);
		Set32::ResetIdx(touchMask, pEnt->laneLvl);
	}

#if 0
	void TouchFix(GEntry* pEnt, bool fix) {
		assert(pEnt->gIdx >= entFirst && pEnt->gIdx <= entLast);
		if (fix)
			Set32::SetIdx(touchMask, pEnt->laneLvl);
		else
			Set32::ResetIdx(touchMask, pEnt->laneLvl);
	}
#endif

	inline void TouchFix(GEntry* pEnt, bool fix, bool isParallel) {
		// For orthogonal touches, use lvl after last ent
		s8 lvl = isParallel ? pEnt->laneLvl : pEnt->laneLvl + 1;
		if (fix)
			Set32::SetIdx(touchMask, lvl);
		else
			Set32::ResetIdx(touchMask, lvl);
	}

	/////////////////////////////////////////////
	//
	//		TRANSFER STATE
	//
	s8 xferLast;

	inline bool XferWasDone() { return xferLast >= 0; }

	inline void XferReset() {
		xferLast = -1;
	}

	inline void XferCountFromMask() {
		xferLast = TouchCount();
	}

#if 0
	void XferOnAbove() {
		GEntry* pPred;
		GLane* pPredLane;
		for (s8 d = 0; d <= xferLast; d++) {
			if ((pPred = pEntFirst[d].pUpEdge) != NULL) {
				pPredLane = this - (gLevel - pPred->gLevel);
				pPredLane->TouchOn(pPred);
			}
		}
	}

	void XferOffAbove() {
		GEntry* pPred;
		GLane* pPredLane;
		for (s8 d = 0; d <= xferLast; d++) {
			if ((pPred = pEntFirst[d].pUpEdge) != NULL) {
				pPredLane = this - (gLevel - pPred->gLevel);
				pPredLane->TouchOff(pPred);
			}
		}
		xferLast = -1;
	}
#endif

	void XferFixAbove(s8 xferNew) {
		if (xferLast < xferNew) {
			// Touch from touchLast + 1 until failDepth
			XferAbove(xferLast + 1, xferNew, true);
		}
		else if (xferLast > xferNew) {
			// Untouch from failDepth + 1 until touchLast
			XferAbove(xferNew + 1, xferLast, false);
		}
		xferLast = xferNew;
	}

	void XferAbove(s8 dFr, s8 dTo, bool setVal) {
		GEntry* pPred;
		GLane* pPredLane;
		for (s8 d = dFr; d <= dTo; d++) {
			if ((pPred = pEntFirst[d].pUpEdge) != NULL) {
				pPredLane = this - (gLevel - pPred->gLevel);
				pPredLane->TouchFix(pPred, setVal, IsLaneParallel(pPredLane));
			}
		}
	}

	///////////////////////////////////////////////////////////
	// 
	//	DE STATE
	//

	s8 deLast;			// Last entry when deadended, -1 otherwise
	s8 deClosest;		// laneLvl of entry with closes predecessor
	s8 deChgCnt;		// number of state changes in lane

	// Reset Dead End state 
	inline void DEStateReset() {
		deLast = -1;
		deChgCnt = 0;
		deClosest = -1;
	}

	// Lane is deadended
	inline bool DEStateOn() {
		return deLast >= 0;
	}

	// Save DE state up to failDepth
	void DEStateSave() {
		deLast = failDepth;
		deClosest = -1;
		deChgCnt = 0;
		GEntry* pEnt = pEntFirst;
		GEntry::GIndex predIdx = -1;
		for (s8 d = 0; d <= deLast; d++, pEnt++) {
			pEnt->EdgeStateSave();
			if (predIdx < pEnt->upEdge) {
				predIdx = pEnt->upEdge;
				deClosest = d;
			}
		}
		assert(deClosest >= 0);
	}

	bool DEStateDiff() {
		for (s8 d = 0; d <= deLast; d++)
			if (pEntFirst[d].EdgeStateDiff())
				return true;
		return false;
	}

	///////////////////////////////////////////////////
	//
	//		ADVANCE
	// 
		
	// Lane has reached dead end state
	bool AdvDeadEnd() {
		if (advPerms > 0)
			return false;
		else {
			assert(failDepth >= 0 && failDepth < entCnt);
			return true;
		}
	}

	// Restrict AdvPerm up to depth
	void AdvRestrict(s8 depth) {
		advLast = depth;
	}

	// AdvPerm up to full lane
	void AdvUnRestrict() {
		advLast = entCnt - 1;
	}

	// Discount advPerms for a failed case
	void AdvPermFailed() { advPerms--; }

	// Start perm generation for a lane
	//		Used while growing
	bool AdvInit(bool setLast = true);

	// Generate next perm
	//		Iterates from pAdv until reaching advDepth >= advLast
	bool AdvPerm();

	// Complete perm generated by AdvPerm()
	//		Iterates from current pAdv, advDepth until reaching advDepth >= entCnt - 1
	bool AdvPermComplete();

	// Extend AdvPerm() to reach advDepth >= entCnt - 1
	bool AdvPermExtend();

	// Verify that lane is viable
	bool AdvCheckDeadEnd();

	// Move up in generation
	bool AdvClimb();
	// Move down in generation and pass down lane branch
	void AdvDown(LvlBranch dnBr);

	// Propagate branch after gLane
	void AdvSuccess(LvlBranch afterDnBr);

	// Generate next perm with DE State check
	//		Iterates from pAdv until reaching advDepth >= advLast
	bool AdvPermDEState(bool deActive);

	// Propagate pAdv edge with DE State Check
	bool AdvPropDEState(bool deActive, GEntry *pDn, LvlBranch br);

	// Verify State change
	bool AdvPropVerify();
};

class CheckLadder {
public:
	CheckLadder() {}

	typedef u64 DpBlock;
	typedef s8 Depth;
	typedef s8 RefCnt;

	s8 lvlMax;
	GEntry* pEntryBase;
	GLane* pLaneBase;
	SearchType srType;

	void Init(s8 _lvlMax, GEntry* _pEntryBase, GLane* _pLaneBase, SearchType _srType) {
		srType = _srType;
		lvlMax = _lvlMax;
		pEntryBase = _pEntryBase;
		pLaneBase = _pLaneBase;
		blockResetVal = srType == SRCHDEST ? BLOCKDEST : BLOCKLADR;
		Reset();
	}

	static const int LVL2BLKLOG = 3;
	static const int LVL2BLKCNT = 1 << LVL2BLKLOG;
	static const int LEVELMAX = DIMMAX * 2;
	static const int LEVELBLOCKMAX = (LEVELMAX + LVL2BLKCNT - 1) >> LVL2BLKLOG;
	static const u64 BLOCKLADR = 0xFFFFFFFFFFFFFFFF;
	static const u64 BLOCKDEST = 0;
	u64 blockResetVal;

	inline int Lvl2Blk(s8 lvl) { return lvl >> LVL2BLKLOG; }

	union {
		DpBlock Blk[LEVELBLOCKMAX];		// Clean
		Depth Dp[LEVELMAX];				// Depth	-- for ladder search
		RefCnt Cnt[LEVELMAX];			// RefCnt	-- for DEState search
	} Tab;

	
	s8 upLvl;		// Covered range: top
	s8 dnLvl;		// Covered range: bot
	s8 visitLvl;	// Check Visit level
	s8 insLvl;		// Last insert level

	inline bool IsEmpty() { return upLvl > dnLvl; }
	inline bool LevelIsIn(s8 lvl) { return Tab.Dp[lvl] >= 0; }

	void Reset() {
		upLvl = 0;
		dnLvl = lvlMax;
		Clean();
	}

	void Clean() {
		int dnBlk = Lvl2Blk(dnLvl);
		for (int l = Lvl2Blk(upLvl); l <= dnBlk; l++)
			Tab.Blk[l] = blockResetVal;
		upLvl = CHAR_MAX;
		insLvl = CHAR_MAX;
		dnLvl = CHAR_MIN;
	}

	// RANGES
	//  INSERTION:			upLvl <=	insLvl		<= dnLvl
	//  CHECK VISIT:		upLvl <=	visitLvl	<= dnLvl
	//

	// Find first level in ladder below lvl, record it as chkLvl
	inline s8 ChkLvlFirst(s8 lvl) {
		visitLvl = ChkLvlDown(lvl);
		return visitLvl;
	}

	// Find first level in ladder below lvl
	inline s8 ChkLvlDown(s8 lvl) {
		for (lvl++; lvl <= dnLvl; lvl++) {
			if (Tab.Dp[lvl] >= 0)
				return lvl;
		}
		// Below bottom
		return GEnvGeom::GLVLNULL;
	}

	inline s8 ChkLvlUp(s8 lvl) {
		for (lvl--; lvl >= visitLvl; lvl--) {
			if (Tab.Dp[lvl] >= 0)
				return lvl;
		}
		// Reached chkLvl
		return GEnvGeom::GLVLNULL;
	}

	bool AddUpEntry(GEntry::GIndex idx) {
		if (idx == GEntry::GIDXNULL)
			return false;

		GEntry *pEnt = pEntryBase + idx;
		s8 entLvl = pEnt->gLevel;
		if (upLvl > entLvl)
			upLvl = entLvl;
		if (Tab.Dp[entLvl] < pEnt->laneLvl)
			Tab.Dp[entLvl] = pEnt->laneLvl;

		return true;
	}

	// Starting at lvl, Go up to first non zero level or above ladder
	s8 MoveUp(s8 lvl) {
		for (lvl--; lvl >= upLvl; lvl--) {
			if (Tab.Dp[lvl] >= 0)
				return lvl;
		}
		// Reached upLvl
		return GEnvGeom::GLVLNULL;
	}

	// Update ladder up to pLane
	void UpdateUp(GLane* pLane) {
		if (Tab.Dp[pLane->gLevel] >= 0)
			InsertLane(pLane, Tab.Dp[pLane->gLevel]);
	}

	// Insert Lane in Ladder: modifies pLane advLast by Tab.Dp[]
	void InsertLane(GLane* pLane, s8 depth) {
		if (dnLvl < pLane->gLevel)
			// First insertion
			dnLvl = pLane->gLevel;

		// Restrictions
		insLvl = pLane->gLevel;
		if (depth >= 0) {
			// Insertion has restriction depth: update Tab.Dp and lane 
			if (Tab.Dp[insLvl] < depth)
				Tab.Dp[insLvl] = depth;
			pLane->AdvRestrict(Tab.Dp[insLvl]);
		}
		else {
			// Insertion has no restrictions: restrict lane as per ladder
			if (Tab.Dp[insLvl] >= 0) {
				pLane->AdvRestrict(Tab.Dp[insLvl]);
			}
			else
				pLane->AdvUnRestrict();
		}

		// Update Tab.Dp with lane predecessors
		depth = Tab.Dp[insLvl];
		GEntry* pEnt = pLane->pEntFirst;
		for (int d = 0; d <= depth; d++, pEnt++) {
			AddUpEntry(pEnt->upEdge);
		}
	}

	/////////////////////////////////////////////////////
	//   DESTATE
	//
	// RANGES
	//  INSERTION:			upLvl <=	insLvl		<= dnLvl
	//  CHECK VISIT:		upLvl <=	visitLvl	<= dnLvl
	//

	// Find first level in ladder below lvl, record it as chkLvl
	inline s8 DELvlFirst(s8 lvl) {
		visitLvl = DELvlDown(lvl);
		return visitLvl;
	}

	// Find first level in ladder below lvl
	inline s8 DELvlDown(s8 lvl) {
		for (lvl++; lvl <= dnLvl; lvl++) {
			if (Tab.Cnt[lvl] > 0)
				return lvl;
		}
		// Below bottom
		return GEnvGeom::GLVLNULL;
	}

	inline s8 DELvlUp(s8 lvl) {
		for (lvl--; lvl >= visitLvl; lvl--) {
			if (Tab.Cnt[lvl] > 0)
				return lvl;
		}
		// Reached chkLvl
		return GEnvGeom::GLVLNULL;
	}

	bool DEAddUpEntry(GEntry::GIndex idx) {
		if (idx == GEntry::GIDXNULL)
			return false;

		GEntry* pEnt = pEntryBase + idx;
		s8 entLvl = pEnt->gLevel;
		if (upLvl > entLvl)
			upLvl = entLvl;
		Tab.Cnt[entLvl]++;

		return true;
	}

	bool DESubEntry(GEntry::GIndex idx) {
		if (idx == GEntry::GIDXNULL)
			return false;

		GEntry* pEnt = pEntryBase + idx;
		s8 entLvl = pEnt->gLevel;
		Tab.Cnt[entLvl]--;

		return true;
	}

	// Starting at lvl, Go up to first non zero level or above ladder
	s8 DEMoveUp(s8 lvl) {
		for (lvl--; lvl >= upLvl; lvl--) {
			if (Tab.Cnt[lvl] > 0)
				return lvl;
		}
		// Reached upLvl
		return GEnvGeom::GLVLNULL;
	}

	// Insert Lane in Ladder
	void DEInsertLane(GLane* pLane, s8 depth) {
		if (dnLvl < pLane->gLevel)
			// First insertion
			dnLvl = pLane->gLevel;

		// Restrictions
		insLvl = pLane->gLevel;
		if (depth >= 0) {
			// Insertion has restriction depth: update Tab.Dp and lane 
			if (Tab.Dp[insLvl] < depth)
				Tab.Dp[insLvl] = depth;
			pLane->AdvRestrict(Tab.Dp[insLvl]);
		}
		else {
			// Insertion has no restrictions: restrict lane as per ladder
			if (Tab.Dp[insLvl] >= 0) {
				pLane->AdvRestrict(Tab.Dp[insLvl]);
			}
			else
				pLane->AdvUnRestrict();
		}

		// Update Tab.Dp with lane predecessors
		depth = Tab.Dp[insLvl];
		GEntry* pEnt = pLane->pEntFirst;
		for (int d = 0; d <= depth; d++, pEnt++) {
			AddUpEntry(pEnt->upEdge);
		}
	}

};

class GEnv : public EnvBase {
public:
	GEnv() {}
	GEnv(s8 _n) {
		n = _n;
		Init();
	}

	int entCnt;

	void Init();

	GLane* pLanes;
	GEntry* pEntries;
	SolveStkEnt* StkPool;
	int freeEnt;
	s16 stackBase;

	CheckLadder ChkLdr;

	RangeTrack RngTrack;
	int deCnt;				// Dead End count for a search

	//////////////////////////////////////////////////////////
	// 
	// Building
	
	// Inputs array (source encoded) from fname
	// Inits and ProcessMat
	bool ReadSrc(string& fname);

	// Inputs array from fname
	// Sorts array columns 
	// ProcessMat
	bool ReadRaw(string& fname, string sortMode);

	// Random generates array (nxn, blkCnt blanks per lane, using seed)
	// ProcessMat
	bool GenRaw(int n, int blkCnt, int seed);

	// Creates and builds *Full arrays
	// Fills VDict in *Full lanes
	void ProcessMat();

	// Copy Mat from Src
	// Sort according to sortRow and sortCol
	// Init and ProcessMat
	bool CopyFrom(EnvBase& Src, string& sortRow, string& sortCol);

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
	int ScanMat(s8 lvl, SolveStkEnt *pStkPool);

	// Compute dnEdge, pDnEdge and changeIdx indices for all entries
	//  Set pEntAfter for gLane
	void CompleteLinks(s8 lvl);

	//	FillEnts()
	//	Generate PermSets, checks if EPars
	void FillBlanks();

	//	Generate ExpSets and PSets
	//      EPars.makeRef ->    create PermRef
	//      EPars.checkRef->    check against PermRef
	//      Collect RowsPermCnt[] and ColsPermCnt[]
	void GenPermSets();

	// Search boundaries
	s8 srLvlFirst, srLvlLast;

	// Sets search boundaries: srLvlFirst, srLvlLast
	//  For all GLanes: set gUp, gDn
	//
	bool LinkGLanes();

	// Creates pEntries, pLanes
	//   ScanMat for all lanes
	//   CompleteLinks for all lanes
	//   LinkGLanes
	virtual void FillEnts();

	// Display changeLinks
	void DisplayBJLinks();

	//////////////////////////////////////////////////////////
	// 
	// SEARCHING
	//
	s8 gLevelMax;
	s8 srLevel;
	int srCnt;
	u64 srHash;
	GLane* pSrLane;

	// Search top level
	SearchType srType;
	string srTypeName;
	bool SearchInspect();

	//	Standard
	int Search();
	//  With ladder
	int SearchLadder();

	//  With ladder and DE state checking
	int SearchDEState();

	// Inspect GEnv
	s8 insLvl;
	s8 insDEActive;
	GLane* pInsp;
	int insCnt, insCntSave;

	void SearchDEInspInit() {
		insCnt = 0;
		insCntSave = -1;
		insDEActive = 0;
	}

	bool SearchDEInspAbove();
	bool SearchDEInspSrLevel();
	bool SearchDEInspBelow();
	bool SearchDEInspError();
	bool SearchDEInspDEAct();

	// deActive counts the number or Dead ended lanes
	s8 deActive;
	s8 deBottom;
	inline void DEActiveReset() {
		deActive = 0;
		deBottom = -1;
	}
	inline void DEActiveInc()   {
		deActive++;
		if (deBottom < srLevel)
			deBottom = srLevel;
	}
	inline void DEActiveDec()   {
		deActive--;
		if (deActive == 0)
			deBottom = -1;
	}
	inline bool DEActiveOn()	{ return deActive > 0; }

	// Initialize GEnv
	// Returns false if matrix has no blanks
	bool SearchInit(SearchType _srType);
	void SearchEnd();

	// Returns false when search limit has been reached
	bool SearchUpSuccess();
	bool SearchUp();

	// Search down
	bool SearchDn();
	// Search down action when dead end is active
	void SearchDnDEState();

	void Publish();
	void SearchDump();

	// Back Jump when pSrLane has dead ended
	void SearchBackJump();

	// Search up within ChkLadder
	//  Insert Lane in ladder as needed
	//  Find up level
	bool SearchUpLdr();
	bool SearchUpLdrStep();
	bool SearchUpDEState();

	// After successful AdvPerm, run down ladder lanes
	//	If any lane fails, return false
	bool SearchCheckDeadEnds(s8 fromLvl);

	MatVis *pVMap;
	void DumpTrace();
	void DumpInsCnt();

	u64 hash;
	u64 Hash();

};


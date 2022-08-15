#pragma once
#include "stdinc.h"
#include "nlane.h"
#include <iomanip>
#include <fstream>
#include "nenv.h"
#include "matvis.h"

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
		selV = 0xFF;
		Stk.Reset();
	}

	inline void SetExpanded() { laneUpBr = PermSet::BRANCHNULL; }
	inline bool IsExpanded() { return laneUpBr == PermSet::BRANCHNULL; }

	inline void StartPSet() { laneUpBr = 0; }
	void PropagateEdge();

	// Edge State
	LvlBranch edgeBrSave;
	void EdgeStateSave() { 	edgeBrSave = edgeUpBr;	}
	bool EdgeStateDiff() { return edgeBrSave != edgeUpBr; }
	bool EdgeStateEqual() { return edgeBrSave == edgeUpBr; }

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
	}

	s8 gLevel;			// Lane level
	s8 gUp;				// Previous lane
	u8 gDn;				// Next lane
	s8 idx;				
	bool isRow;

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
	s8 deLast;			// Last entry when deadended, -1 otherwise
	int advPerms;		// Lane advance has generated advPerms already

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

	// Lane is deadended
	inline bool IsDeadEnded() {
		return deLast >= 0;
	}

	// Dead End state
	inline void DeadEndStateReset() { deLast = -1; }

	void DeadEndStateSave(s8 _deLast) {
		deLast = _deLast;
		for (s8 d = 0; d <= deLast; d++)
			pEntFirst[d].EdgeStateSave();
	}

	bool DeadEndStateDiff() {
		for (s8 d = 0; d <= deLast; d++)
			if (pEntFirst[d].EdgeStateDiff())
				return true;
		return false;
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
};

#if 0
class GLaneTab {
public:
	GLaneTab() {}

	typedef s8 Lvl;

	static const int TABSZ = 2 * DIMMAX;
	Lvl Tab[TABSZ];

	void Reset() {
		for (int l = 0; l < TABSZ; l++)
			Tab[l] = 0;

		loLvl = 0;
		hiLvl = 0;
		empty = true;
	}

	Lvl loLvl, hiLvl;
	bool empty;

	inline bool Empty() { return empty;  }

	void AddDELane(GLane* pLane) {
		if (pLane->predLast < 0)
			return;
		if (loLvl > pLane->Preds[0])
			loLvl = pLane->Preds[0];
		if (hiLvl < pLane->Preds[pLane->predLast])
			hiLvl = pLane->Preds[pLane->predLast];
		for (int p = 0; p <= pLane->predLast; p++) {
			AddLvl(pLane->Preds[p]);
		}

	}

	inline void AddLvl(s8 l) { Tab[l]++; }
	inline void SubLvl(s8 l) { Tab[l]--; }
};

class DEEntry {
public:
	DEEntry() : entIdx(GEntry::GIDXNULL), nextLaneLvl(-1), nextPos(-1) {}

	void Reset() {
		entIdx = GEntry::GIDXNULL;
		nextLaneLvl = -1;
		nextPos = -1;
	}

	void Set(GLane* _pLane, s8 _targPos) {
		pLane = _pLane;
		assert(_targPos <= pLane->targLast);

		nextPos = _targPos;
		nextLaneLvl = pLane->gLevel;
		entIdx = pLane->BJTarg[nextPos];
		nextPos--;
	}

	GEntry::GIndex Get() {
		if (IsEmpty())
			return GEntry::GIDXNULL;

		GEntry::GIndex ret = entIdx;
		entIdx = pLane->BJTarg[nextPos--];
		return ret;
	}

	bool IsEmpty() { return nextPos < 0;  }

	GLane* pLane;
	GEntry::GIndex entIdx;
	s8 nextLaneLvl;
	s8 nextPos;
};

class DeadEndStk {
public:
	DeadEndStk() {}

	static const int TABSZ = 2 * DIMMAX;
	DEEntry Tab[TABSZ];

	void Reset() {
		for (int l = 0; l < TABSZ; l++)
			Tab[l].Reset();

		topEnt = -1;
		topLaneLvl = SCHAR_MAX;
	}

	s8 topLaneLvl;
	s8 topEnt;
	inline bool Empty() { return topEnt < 0; }

	DEEntry* PushDELane(GLane* pLane, s8 targPos) {
		assert(topEnt < TABSZ - 1);
		assert(topLaneLvl > pLane->gLevel);
		topEnt++;
		Tab[topEnt].Set(pLane, targPos);
		topLaneLvl = pLane->gLevel;
		return Tab + topEnt;
	}

	void Pop() {
		assert(topEnt >= 0);
		topEnt--;
	}

	// Move stack to below pLane
	void MoveUp(s8 nextTopLvl) {
		GLane* pNewLane = nullptr;
		DEEntry* pDEEnt = nullptr;
		while (topLaneLvl > nextTopLvl) {
			topLaneLvl--;
			for (int ent = 0; ent <= topEnt; ent++) {
				if (Tab[ent].nextLaneLvl == topLaneLvl) {
					if (pDEEnt == nullptr) {
						// Insert this lane level into DEStk
						//pNewLane = PLanes + topLaneLvl;
						pDEEnt = PushDELane(pNewLane, 0);
					}
					// Modify pNewLane check limits according to Tab[ent]
					// AdvanceEnt(pDEEnt, Tab[ent]);
				}
			}
		}
	}

};
#endif

class CheckLadder {
public:
	CheckLadder() {}

	typedef u64 DpBlock;
	typedef s8 Depth;

	s8 lvlMax;
	GEntry* pEntryBase;
	GLane* pLaneBase;

	void Init(s8 _lvlMax, GEntry* _pEntryBase, GLane* _pLaneBase) {
		lvlMax = _lvlMax;
		pEntryBase = _pEntryBase;
		pLaneBase = _pLaneBase;
		Reset();
	}

	static const int LVL2BLKLOG = 3;
	static const int LVL2BLKCNT = 1 << LVL2BLKLOG;
	static const int LEVELMAX = DIMMAX * 2;
	static const int LEVELBLOCKMAX = (LEVELMAX + LVL2BLKCNT - 1) >> LVL2BLKLOG;
	static const u64 BLOCKINIT = 0xFFFFFFFFFFFFFFFF;

	inline int Lvl2Blk(s8 lvl) { return lvl >> LVL2BLKLOG; }

	union {
		DpBlock Blk[LEVELBLOCKMAX];		// Clean
		Depth Dp[LEVELMAX];
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
			Tab.Blk[l] = BLOCKINIT;
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
	//	Standard
	int Search();
	//  With ladder
	int SearchLadder();

	// Initialize GEnv
	// Returns false if matrix has no blanks
	bool SearchInit();
	void SearchEnd(const char* sname);

	// Returns false when search limit has been reached
	bool SearchUpSuccess();
	bool SearchUp();
	bool SearchDn();
	void Publish();
	void SearchDump();

	// Back Jump when pSrLane has dead ended
	void SearchBackJump();

	// Search up within ChkLadder
	//  Insert Lane in ladder as needed
	//  Find up level
	bool SearchUpLdr();
	bool SearchUpLdrStep();
	// After successful AdvPerm, run down ladder lanes
	//	If any lane fails, return false
	bool SearchCheckDeadEnds(s8 fromLvl);

	MatVis *pVMap;
	void DumpTrace();

	u64 hash;
	u64 Hash();

};


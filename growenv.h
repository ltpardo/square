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
	void InitBranches() {
		edgeUpBr = 0;
		laneUpBr = 0;
		edgeDnBr = PermSet::BRANCHNULL;
		laneDnBr = PermSet::BRANCHNULL;
	}

	inline void SetExpanded() { laneUpBr = PermSet::BRANCHNULL; }
	inline bool IsExpanded() { return laneUpBr == PermSet::BRANCHNULL; }

	inline void StartPSet() { laneUpBr = 0; }
	void PropagateEdge();
	
	void ExpandSetReturn(s8& lvl);
	bool Expand(s8& retLevel);

	// Back Jump data
	s16 bjIdx;

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
	}

	s8 gLevel;			// Lane level
	s8 gUp;				// Previous lane
	u8 gDn;				// Next lane
	s8 idx;				
	bool isRow;

	GEntry* pEntFirst;	// First entry in gLane
	int entFirst;
	int entLast;
	int entCnt;
	GEntry* pEntAfter;	// Entry to propagate last branch of gLane

	PermSet* pSet;

	// Advance info
	GEntry* pAdv;		// Current entry during generation	
	s8 advDepth;		// Current depth during generation
	int advPerms;		// Lane advance has generated advPerms already
	s8 retDepth;		// Return depth after failure for terminal perm branches
	s8 failDepth;		// Maximum depth reached on a failed generation

	bool AdvDeadEnd() {
		if (advPerms > 0)
			return false;
		else {
			assert(failDepth >= 0 && failDepth < entCnt);
			return true;
		}
	}

	// Start perm generation for a lane
	//		Used while growing
	bool AdvInit();

	// Generate next perm
	bool AdvPerm();

	// Verify that lane is viable
	bool AdvCheckDeadEnd();

	// Move up in generation
	bool AdvClimb();
	// Move down in generation and pass down lane branch
	void AdvDown(LvlBranch dnBr);

	// Propagate branch after gLane
	void AdvSuccess(LvlBranch afterDnBr);
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

	RangeTrack RngTrack;

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
	GLane* pSrLane;
	int Search();

	void SearchInit();
	void SearchUpSuccess();
	void SearchUp();
	void SearchDn();
	void Publish();
	void SearchDump();

	// Back Jump when pSrLane has dead ended
	bool SearchBackJump();

	// Probe dead ended lane
	bool SearchProbeDeadEnd();

	MatVis *pVMap;

};


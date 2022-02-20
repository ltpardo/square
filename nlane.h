#pragma once
#include "stdinc.h"
#include "utils.h"

class NLane;

class SolveStkEnt {
public:
	SolveStkEnt() {}
	SolveStkEnt(LvlBranch _thBr, LvlBranch _crBr) : thBr(_thBr), crBr(_crBr) {}

	LvlBranch thBr;
	LvlBranch crBr;

};

class SolveStack {
public:
	SolveStack() {
		Init();
	}

	void Init() {
		freePos = 0;
	}

	enum { SIZE = BLANKCNTMAX * BLANKCNTMAX	* DIMMAX};
	SolveStkEnt Pool[SIZE];
	int freePos;

	inline int GetTopPos() { return freePos; }
	inline void SetBackTop(int backPos) {
		assert(backPos >= 0 && backPos < SIZE);
		freePos = backPos;
	}

	void Push(LvlBranch thBr, LvlBranch crBr) {
		assert((thBr & 0x3F) == (thBr & 0x3F));
		assert(freePos < SIZE);
		Pool[freePos].thBr = thBr;
		Pool[freePos].crBr = crBr;
		freePos++;
	}

	void Pull(LvlBranch &thBr, LvlBranch &crBr) {
		freePos--;
		assert(freePos >= 0);
		thBr = Pool[freePos].thBr;
		crBr = Pool[freePos].crBr;
		assert((thBr & 0x3F) == (thBr & 0x3F));
	}

	inline SolveStkEnt *Peek(int pos) { return Pool + pos; }
};

extern SolveStack SStk;

class NEntry : public LogClient {
public:
	NEntry() : upThru(0), dnThru(0), upCross(0), dnCross(0),
		miss(0), missCnt(0), selV(0xFF), i(-1), j(-1),
		thLast(false), crLast(false),
		stkPos(SHRT_MIN), iCnt(-DIMMAX)
	{}

	const int INVALIDPOS = SHRT_MIN;
	NEntry(int _i, int _j) : upThru(0), dnThru(0), upCross(0), dnCross(0),
		miss(0), missCnt(0), selV(0xFF), i(_i), j(_j),
		thLast(false), crLast(false),
		stkPos(INVALIDPOS), iCnt(INVALIDPOS)
	{}

	// Linking: 
	//		Axis are thru and cross
	//		Directions are up or down
	NEntry* upThru, * dnThru, * upCross, * dnCross;

	void SelfLink() {
		upThru = this;
		dnThru = this;
		upCross = this;
		dnCross = this;
		thLvl = -1;
		crLvl = -1;
	}
	void UpThruOf(NEntry* pEnt) {
		dnThru = pEnt;
		upThru = pEnt->upThru;
		upThru->dnThru = this;
		pEnt->upThru = this;
	}

	void UpCrossOf(NEntry* pEnt) {
		dnCross = pEnt;
		upCross = pEnt->upCross;
		upCross->dnCross = this;
		pEnt->upCross = this;
	}

	// Lanes and lane levels
	int thLvl, crLvl;
	NLane* pThru, * pCross;

	Set miss;				// Possible values for entry
	int missCnt;			// size of miss
	u8 missV[BLANKCNTMAX];	// Expansion of miss
	u8 selV;				// Search value chosen

	bool thLast;
	bool crLast;
	
	// Coords: correspond to thru and cross lane indices
	int i, j;
	void SetCoords(int _i, int _j) { i = _i; j = _j; }

	// Search data
	PermSet* pThPSet;
	PermSet* pCrPSet;
	LvlBranch thUpBr;
	LvlBranch thDnBr;
	LvlBranch crUpBr;
	LvlBranch crDnBr;

	int stkPos;
	u8 iCnt;				// Number of branches in intersection
	Set ThSet;				// Unused selects from thUpBr
	Set crUnSel;			// Previously unselected set (cross)

	void InitSearchInfo(PermSet* _pThPSet, PermSet* _pCrPSet) {
		pThPSet = _pThPSet;
		pCrPSet = _pCrPSet;
		thUpBr = PermSet::LVLNULL;
		thDnBr = PermSet::LVLNULL;
		crUpBr = PermSet::LVLNULL;
		crDnBr = PermSet::LVLNULL;
	}

	bool ThIsValid() { return thLvl >= 0; }
	bool CrIsValid() { return crLvl >= 0; }

	bool ThExhausted() { return thDnBr == PermSet::LVLNULL; }
	void ThSetExhausted() {
		thDnBr = PermSet::LVLNULL;
		iCnt = 0;
	}
	bool CrExhausted() { return crDnBr == PermSet::LVLNULL; }
	void CrSetExhausted() { crDnBr = PermSet::LVLNULL; }

	bool ThFirstEnt() { return thUpBr == 0; }
	bool ThExpand(NEntry* &pReturn);
	bool ThTermReturn(NEntry* &pReturn);

	NEntry* ThClimbUp(NEntry* &pReturn);
	NEntry* ThClimbDown();
	bool ThAdvCross();

	void DumpExpand();

	inline void StkPos() {
		stkPos = SStk.GetTopPos();
	}

	inline void StkPeek(int dispFromTop) {
		stkPos = SStk.GetTopPos();
	}

	inline void StkPull() {
		assert(stkPos == SStk.freePos);
		SStk.Pull(thDnBr, crDnBr);
		selV = pThPSet->LvlGetV(thDnBr);
		if (selV == upCross->selV)
			cerr << "PULL SELV " << selV << endl;
		ThSet -= selV;
		StkPos();
		iCnt--;
	}

	bool StkCheckSet(Set& set);
};

class InterStack {
public:
	InterStack(int _size) : size(_size) {
		Pool = new IntStkEnt[size];
		topFree = 0;
	}

	~InterStack() {
		delete Pool;
	}

	int size;
	int topFree;

	typedef u8 IntStkEnt;

	inline void Push(IntStkEnt v) {
		assert(topFree < size);
		Pool[topFree++] = v;
	}

	inline IntStkEnt Pop() {
		assert(topFree > 0);
		return Pool[--topFree];
	}
	
	inline void Crop(int newFree) {
		assert(newFree <= topFree);
		topFree = newFree;
	}

	IntStkEnt*Pool;
};

class NStkEnt {
public:
	NStkEnt() {}

	NEntry* pEnt;
	LMask lMiss;
};

class NLane : public LogClient {
public:
	NLane() : present(0), presCnt(0), missCnt(0), stops(0), blankCnt(0) {}
	NLane(int _n) : n(_n) {}

	void Init(int _n, int _idx, bool _isRow);

	int n;
	int idx;
	bool isRow;
	Mask all;

	NEntry* pCur;
	NEntry* pTermReturn;		// Return address for terminal nodes
								//    Set to NULL before terminals

	int deadEndDepth;			// Maximum depth reached in lane
	bool isDeadEnd;				// No Permutation possible
	NEntry* pBackDst;			// Backtrack entry
	int backJ;					// Backtrack level

	void NextPermInit();
	bool NextPermInCol();
	bool NextPermAdvCur();
	bool SearchBackDst(NLane *pPrevLane);

	// Backtrack to entry pBack
	void Backtrack(NEntry* pBack = NULL);

	// Lane original openings
	Set absMiss;		// Absolute set
	LMask lMissing;		// Lane (local) set
	int missCnt;

	// Mapping local <-> abs
	u8 VDict[BLANKCNTMAX];
	void AbsToLocal();
	LMask MapToLocal(Set abs);
	LMask MapToLocal(u8* pV, int cnt);

	// Stack structure: 0 <= d < blankCnt
	NStkEnt NStack[BLANKCNTMAX];
	int blankCnt;

	// Aux vars
	NEntry* pBlank;
	int level;

	// LVL PSet is generated from EXPANDED PSet
	PermSet PSet;

	// Reference  PSet
	ShortCnt refCnt;
	PermSetRef* pRef;
	int PermRefCreate();
	bool PermRefGen(int level, Mask sel, ShortCnt& cnt);

	// EXPANDED PSet
	// Returns lnk to node and node cnt
	LongLink PermSetGenExpanded(int level, LMask sel, ShortCnt& cnt);

	// Construction info and methods
	//
	void AddBlank(NEntry* pEnt);
	void FillBlanks(NLane* Rows);
	void LinkThru(NLane* Rows, NLane* Cols);
	void LinkCross(NLane* Cols);
	int RegenPSet();

	Set present;
	int presCnt;
	void Present(u8 v);
	void Missing(u8 v);
	bool Check();

	// Ring info for rows  (accross)
	NEntry ringHd;
	Set stops;
	bool Reaches(int j) { return stops.Contains(j); }
};


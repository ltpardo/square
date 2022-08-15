#pragma once
#include "stdinc.h"
#include "utils.h"

class NLane;

class SolveStkEnt {
public:
	SolveStkEnt() {}
	SolveStkEnt(LvlBranch _thBr, LvlBranch _crBr) : thBr(_thBr), crBr(_crBr) {}

	void Set(LvlBranch _thBr, LvlBranch _crBr) {
		thBr = _thBr;
		crBr = _crBr;
	}

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

class EState {
public:
	EState() : Opt(0), st (ST_VOID), cnt(0), head(0) {}

	void Reset() {
		Opt = 0;
		st = ST_VOID;
		TabReset();
	}

	Set Opt;

	enum {
		BRSNEWBIT = 0,
		BRSNEWMASK = 1 << BRSNEWBIT,
		CNTNULLBIT = 1,
		CNTNULLMASK = 1 << CNTNULLBIT,
		EXPANDEDBIT = 2,
		EXPANDEDMASK = 1 << EXPANDEDBIT,

		CRNEWBIT = 16,
		CRNEWMASK = 1 << CRNEWBIT,
		THNEWBIT = 17,
		THNEWMASK = 1 << THNEWBIT,
	};

	enum {
		ST_VOID = 0,
		ST_EXPAND = BRSNEWMASK | CNTNULLMASK | ~EXPANDEDMASK,
		ST_PULL = ~BRSNEWMASK | ~CNTNULLMASK | EXPANDEDMASK,
		ST_DONE = ~BRSNEWMASK | CNTNULLMASK | EXPANDEDMASK,
	};
	
	int st;

	bool IsBrsNew() { return (st & BRSNEWMASK) != 0; }
	bool IsCntNull() { return (st & CNTNULLMASK) != 0; }
	bool IsExpanded() { return (st & EXPANDEDMASK) != 0; }

	void BrNewSet() { st |= BRSNEWMASK; }
	void CntNullSet() {
		st |= CNTNULLMASK;
		TabReset();
	}
	void ExpandedSet() { st |= EXPANDEDMASK; }

	void BrNewReset() { st &= ~BRSNEWMASK; }
	void CntNullReset() { st &= ~CNTNULLMASK; }
	void ExpandedReset() { st &= ~EXPANDEDMASK; }

	bool IsCrNew() { return (st & CRNEWMASK) != 0; }
	bool IsThNew() { return (st & THNEWMASK) != 0; }
	void CrNewSet() { st |= CRNEWMASK; }
	void ThNewSet() { st |= THNEWMASK; }
	void CrNewReset() { st &= ~CRNEWMASK; }
	void ThNewReset() { st &= ~THNEWMASK; }

	SolveStkEnt Tab[BLANKCNTMAX];
	int cnt;
	int head;

	void TabReset() {
		cnt = 0;
		head = 0;
	}

	bool IsStkEmpty() { return head >= cnt; }

	bool Push(LvlBranch thBr, LvlBranch crBr) {
		assert(cnt < BLANKCNTMAX);
		Tab[cnt].Set(thBr, crBr);
		cnt++;
		return true;
	}

	bool Pull(LvlBranch &thBr, LvlBranch &crBr) {
		assert(head < cnt);
		thBr = Tab[head].thBr;
		crBr = Tab[head].crBr;
		head++;
		return head < cnt;
	}
};

class EntryBase : public LogClient {
public:
	EntryBase() : miss(0), missCnt(0), selV(0xFF), i(-1), j(-1)
	{}

	EntryBase(int _i, int _j) : miss(0), missCnt(0), selV(0xFF), i(_i), j(_j)
	{}

	Set miss;				// Possible values for entry
	s8 missCnt;				// size of miss
	u8 selV;				// Search value chosen

	// Coords
	s8 i, j;
	void SetCoords(s8 _i, s8 _j) { i = _i; j = _j; }

	// Set missing data
	//	Extracts value set into Dict
	void SetMissing(Set& crMiss, Set& thMiss, u8 Dict[], int dictSz) {
		miss = crMiss & thMiss.mask;
		missCnt = miss.ExtractValues(Dict, dictSz);
	}

	void DumpExpand(PermSet* pLnSet, LvlBranch lnUpBr, s8 lnLvl,
		PermSet* pEdSet, LvlBranch edUpBr, s8 edLvl)
	{
		*DbgSt.pOut << "   " << pLnSet->searchId << " ";
		pLnSet->DumpExpanded(*DbgSt.pOut, lnLvl, lnUpBr);
		*DbgSt.pOut << " <<>> ";

		*DbgSt.pOut << pEdSet->searchId << " ";
		pEdSet->DumpExpanded(*DbgSt.pOut, edLvl, edUpBr);
		*DbgSt.pOut << endl;
	}

	void DumpPull(u8 selV, PermSet* pLnSet, LvlBranch lnDnBr, s8 lnLvl,
		PermSet* pEdSet, LvlBranch edDnBr, s8 edLvl)
	{
		*DbgSt.pOut << "   " << pLnSet->searchId << " ";
		*DbgSt.pOut << "PULL v: " << (int) selV << " depth " << (int)lnLvl << " | ";
		*DbgSt.pOut << pLnSet->searchId << " ";
		pLnSet->DumpBranch(*DbgSt.pOut, lnLvl, lnDnBr);
		*DbgSt.pOut << " <<>> ";

		*DbgSt.pOut << pEdSet->searchId << " ";
		pEdSet->DumpBranch(*DbgSt.pOut, edLvl, edDnBr);
		*DbgSt.pOut << endl;
	}
};

class NEntry : public EntryBase {
public:
	NEntry() : upThru(0), dnThru(0), upCross(0), dnCross(0),
		thLast(false), crLast(false),
		thStart(false), crStart(false),
		stkPos(SHRT_MIN), iCnt(-DIMMAX),
		dLevel(INT_MIN), dLvlPos(INT_MIN), backPos(INT_MIN)
	{
		SetCoords(0, 0);
		miss = 0;
		missCnt = 0;
		selV = 0xFF;
	}

	const int INVALIDPOS = SHRT_MIN;
	NEntry(int _i, int _j) : upThru(0), dnThru(0), upCross(0), dnCross(0),
		thLast(false), crLast(false),
		thStart(false), crStart(false),
		stkPos(INVALIDPOS), iCnt(INVALIDPOS),
		dLevel(INT_MIN), dLvlPos(INT_MIN), backPos(INT_MIN)
	{
		SetCoords(_i, _j);
		miss = 0;
		missCnt = 0;
		selV = 0xFF;
	}

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

	//Set miss;				// Possible values for entry
	//int missCnt;			// size of miss
	//u8 selV;				// Search value chosen
	u8 missV[BLANKCNTMAX];	// Expansion of miss

	bool thStart, crStart;	// (Reduction) Lanes start here
	bool thLast, crLast;	// Lanes end here

	// Coords: correspond to thru and cross lane indices
	//s8 i, j;
	//void SetCoords(int _i, int _j) { i = _i; j = _j; }

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
	Set thUnSel;			// Previously unselected set (thru)

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
	bool CrFirstEnt() { return crUpBr == 0; }
	bool ThStarts() { return thStart; }
	bool CrStarts() { return crStart; }
	bool ThExpand(NEntry* &pReturn);
	bool ThTermReturn(NEntry*& pReturn);
	bool CrTermReturn(NEntry*& pReturn);

	NEntry* ThClimbUp(NEntry*& pReturn);
	NEntry* ThClimbDown();
	bool ThAdvCross();

	NEntry* CrClimbUp(NEntry*& pReturn);
	NEntry* CrClimbDown();

	bool ColAdvCross();
	bool RowAdvCross();

	void DumpExpand(bool isRow = false);

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

	// Search distance ring
	int dLevel;
	int dLvlPos;
	int backPos;		// Backtrack entry position
	NEntry* pDist;
	EState ESt;

	bool DExpand(NEntry*& pReturn, bool isRow = false);
	bool DProcess(BitSet* pBSet);

	// Reduction
	int rLevel;

	// Reduction backtrack
	int redBackLvl;
	int redBackDepth;
	static const int RLEVELNULL = -1;
	static const int RDEPTHRESTART = -1;

	inline void SetRed(int lvl, int dp) {
		redBackLvl = lvl;
		redBackDepth = dp;
	}

	inline void SetRedRestart(int lvl) {
		redBackLvl = lvl;
		redBackDepth = RDEPTHRESTART;
	}

	inline void SetRedTop() {
		redBackLvl = RLEVELNULL;
		redBackDepth = RDEPTHRESTART;
	}

	inline bool IsRedTop() {
		return redBackLvl == RLEVELNULL && redBackDepth == RDEPTHRESTART;
	}
	inline bool IsRedRestart() { return redBackDepth == RDEPTHRESTART;  }
};

class NStkEnt {
public:
	NStkEnt() {}

	NEntry* pEnt;
	LMask lMiss;
};

class LaneFull : public LogClient {
public:
	LaneFull() : present(0), presCnt(0), absMiss(0), missCnt(0), blankCnt(0)
	{}

	int n;
	int idx;
	bool isRow;
	Mask all;

	void Init(int _n, int _idx, bool _isRow, Mask _absMiss, s8 cnt) {
		n = _n;
		idx = _idx;
		isRow = _isRow;
		all = (1LL << n) - 1;
		absMiss = _absMiss;
		missCnt = cnt;
		blankCnt = 0;
	}

	Set present;
	int presCnt;
	Set absMiss;
	int missCnt;
	LMask lMissing;		// Lane (local) set

	void Present(u8 v);
	void Missing(u8 v);
	bool Check();

	// Mapping local <-> abs
	u8 VDict[BLANKCNTMAX];
	void AbsToLocal();
	LMask MapToLocal(Set abs);
	LMask MapToLocal(u8* pV, int cnt);
	void RemoveV(u8 v);

	// Stack structure: 0 <= d < blankCnt
	NStkEnt NStack[BLANKCNTMAX];
	int blankCnt;
	void BlankStk() { blankCnt = 0; }

	// Add an entry to NStack
	//	 Sets pPrev to NStack[blankCnt].pEnt
	//   Returns stack level
	s8 AddEnt(EntryBase *pEnt, u8 EntDict[], EntryBase * &pPrev);

	// EXPANDED PSet
	// Returns lnk to node and node cnt
	PermExp* pExpSet;
	LongLink GenExpSet();
	bool CheckExpSet();

	// LVL PSet is generated from EXPANDED PSet
	PermSet PSet;
	int GenPSet();
	bool CheckPSet();

	// Reference  PSet
	ShortCnt refCnt;
	PermSetRef* pRef;
	int PermRefCreate();

	// All sets generation
	bool PermsGenAndCheck();

	int PSetRestrictMiss();
};

class NLane : public LaneFull {
public:
	NLane() : stops(0) {}
	NLane(int _n) : stops(0) { n = _n; }

	void Init(int _n, int _idx, bool _isRow);

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

	// Reduction search
	NEntry* pCurRedStart;		// Reduced Lane starts here
	int redStartDepth;			//   and at this depth in NStack
	bool prevOverlap;			// Lane overlaps with prev Lane

	// Reduction levels
	int rLevel;
	
	inline bool IsRLevelNull() { return rLevel == NEntry::RLEVELNULL; }

	static inline int RLevelCalc(int idx, bool isRow) {
		return (idx << 1) + (isRow ? 1 : 0);
	}
	static inline int IdxFromRLevel(int level) {
		return (level >> 1);
	}
	static inline bool RLevelIsCol(int level) {
		return (level & 1) == 0;
	}
	static inline NLane* LaneFromLevel(int level, NLane *Rows, NLane *Cols) {
		if (RLevelIsCol(level))
			return Cols + IdxFromRLevel(level);
		else
			return Rows + IdxFromRLevel(level);
	}

	inline bool EntryInRLevel(NEntry* pEnt) {
		if (isRow)
			return pEnt->i == idx && pEnt->j > pEnt->i;
		else
			return pEnt->j == idx && pEnt->i >= pEnt->j;
	}

	bool RedStartFind();
	void RedBackSet(NLane* Rows, NLane* Cols);
	void NextPermRedRestart();
	void NextPermRedFailed(int fDepth);

	enum {
		MODECONT = 0, 
		MODERESTART = 1,
		MODETOTOP = 2,
		MODEFAILED = 3,
		FDEPTHNULL = -1
	};

	int redDepth;
	bool redSuccess;

	inline bool NextPermRed(int mode, int fDepth = 0) {
		if (mode == MODERESTART)
			NextPermRedRestart();
		else if (mode == MODEFAILED)
			NextPermRedFailed(fDepth);
		else if (mode == MODETOTOP)
			pCur = pCurRedStart;
		else
			assert(mode == MODECONT);

		if (isRow)
			return NextPermRedRow();
		else
			return NextPermRedCol();
	}

	bool NextPermRedRow();
	bool NextPermRedCol();

	bool NextPermRedDnRow();
	bool NextPermRedDnCol();

	// Stack structure: 0 <= d < blankCnt
	//NStkEnt NStack[BLANKCNTMAX];
	//int blankCnt;

	// Aux vars
	NEntry* pBlank;
	int level;

	// Construction info and methods
	//
	void AddBlank(NEntry* pEnt);
	void FillBlanks(NLane* Rows, bool doPerms = true);
	void LinkThru(NLane* Rows, NLane* Cols);
	void LinkCross(NLane* Cols);
	int RegenPSet();
	int PSetRestrictMiss();

	// Ring info for rows  (accross)
	NEntry ringHd;
	Set stops;
	bool Reaches(int j) { return stops.Contains(j); }
};


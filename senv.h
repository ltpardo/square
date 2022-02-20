#pragma once
#include "latsq.h"


class Stats {
public:
	Stats() {}

	void Init() {
		starts = 0;
		InitPass();
	}

	void Reset() {
		InitPass();
		starts = 0;
	}

	void InitPass() {
		starts++;
		steps = 0;
		ends = 0;
		exhausts = 0;
		props = 0;
		select = 0;
		downs = 0;
		ups = 0;
	}

	void Step() { steps++; }
	void End() { ends++; }
	void Exhaust() { exhausts++; }
	void Prop() { props++; }
	void Select() { select++; }
	void Up() { ups++; }
	void Down() { downs++; }
	void UnDown() { downs--; }

	int starts;
	int steps;
	int ends;
	int exhausts;
	int props;
	int select;
	int ups;
	int downs;
};


class BEntry {
public:
	BEntry() : lfLink(0), rtLink(0), miss(0),
		chosenBit(0), i(-1), j(-1), level(0)
	{}

	// Linking
	BEntry* lfLink, * rtLink;
	void SelfLink() {
		lfLink = this;
		rtLink = this;
	}
	void LinkLeftOf(BEntry* pEnt) {
		rtLink = pEnt;
		lfLink = pEnt->lfLink;
		lfLink->rtLink = this;
		pEnt->lfLink = this;
	}

	Set miss;				// Possible candidates for entry
	Set laneMiss;			// Missing candidates for cross lane

	// Permutation process status
	Set choiceSet;			// Candidates that may be chosen for entry
	Mask chosenBit;			// Chosen candidate
	Set selectedSet;		// Candidates selected in thru lane
	Set unSelected;			// Candidates not yer selected in thru lane
	Set availForSelect;		// Candidates available to select in thru lane

	bool IsHead() { return level < 0; }
	void SetAsHead() { SetLevel(-1); }

	int i, j;
	int level;
	void SetLevel(int lvl) { level = lvl; }
	void SetCoords(int _i, int _j) { i = _i; j = _j; }

	// BStack traversing
	enum {	TOP = -1, BOT = DIMMAX, INVALID = -DIMMAX };
	int dnTrav;
	int upTrav;

	// Restore position for row starting at this entry
	int changeStkPos;
};

class SLane {
public:
	SLane() : present(0), presCnt(0), missCnt(0), stops(0), blankCnt(0) {}
	SLane(int _n) : n(_n) {}

	void Init(int _n, int _idx);

	int n;
	int idx;
	Mask all;

	// Lane original openings
	Set missing;
	int missCnt;

	// Stack structure for columns (thru)
	BEntry* BStack[BLANKCNTMAX];
	int blankCnt;
	void AddBlank(int i, int j);
	void FillBlanks(SLane* Rows);
	// Aux vars
	BEntry* pBlank;
	int level;

	// Ring info for rows  (accross)
	BEntry ringHd;
	Set stops;
	bool Reaches(int j) { return stops.Contains(j); }
	// Link BStack entries to correspong row rings.
	//		Done after sort (hence using LaneTab)
	void LinkBlanks(SLane* Rows);

	// Set stack range: [rngFrom, rngTo]
	bool SetCntRange(int iFrom, int iTo);
	int rngFrom, rngTo;
	bool counting;

	bool CheckAvail();
	int ComputeAvail();

	// Permutation process (link based)
	Set preSelected;			// Set of forced entries
	u64 foundCnt;				// Number of successes during a permutation pass

	bool PermuteStart();
	int PermuteStep();
	void PermuteContinueAt(int depth);
	int firstPermLvl;

	bool VerifyPerm();

	// Lane initial permutation count
	int permuteCnt;
	u64 PermuteCount();

	Stats St;

	// Stack based permutation
	bool PermuteStartStk();
	bool PermuteStepStk();
	bool PermuteScrub(SLane* pClog, SLane* Rows);

	// Clog analysis
	Set needSet;
	int needCnt;
	u8 needLevels[BLANKCNTMAX];

	// Construction info
	Set present;
	int presCnt;
	void Present(u8 v);
	void Missing(u8 v);
	bool Check();
};

class RestrictBuf {
public:
	RestrictBuf() {}

	void Load(Mask _laneM, Mask _blankM) {
		laneM = _laneM;
		blankM = _blankM;
	}

	Mask laneM;
	Mask blankM;
};

class ChangeBuf {
public:
	ChangeBuf() {}

	BEntry* pEnt;
	Set laneMiss;
	Set miss;

	void Load(BEntry* _pEnt) {
		pEnt = _pEnt;
		laneMiss = pEnt->laneMiss;
		miss = pEnt->miss;
	}

	void Restore() {
		pEnt->laneMiss = laneMiss;
		pEnt->miss = miss;
	}

};

class ChangeStack {
public:
	ChangeStack(int _size) {
		size = _size;
		Stack = new ChangeBuf[size];
		freeStk = 0;
		pFree = Stack;
	}

#if 0
	int LvlDir[DIM];
	int freeLvl;

	void InitLvl() {
		freeLvl = 0;
		for (int l = 0; l < DIM; l++)
			LvlDir[l] = -1;
	}
	int MarkLevel() {
		assert(freeLvl < DIM);
		LvlDir[freeLvl] = freeStk;
		return freeLvl++;
	}

	void RestoreLevel() {
		assert(freeLvl > 0);
		freeLvl--;
		RestoreFrom(LvlDir[freeLvl]);
		LvlDir[freeLvl] = -1;
	}

#endif

	int CurFree() { return freeStk; }

	static const int UNSTACKED = -1;

	void Push(BEntry * pEnt) {
		assert(freeStk < size);
		Stack[freeStk++].Load(pEnt);
	}

	void PushAndChange(BEntry* pEnt, Mask laneMiss, Mask miss) {
		assert(freeStk < size);
		Stack[freeStk++].Load(pEnt);
		pEnt->laneMiss = laneMiss;
		pEnt->miss = miss;
	}

	void RestoreFrom(int from) {
		if (from == UNSTACKED)
			return;
		assert(freeStk >= from);
		for (int s = from; s < freeStk; s++)
			Stack[s].Restore();
		freeStk = from;
	}

	int size;
	int freeStk;
	ChangeBuf* Stack;
	ChangeBuf* pFree;
};

class SpyState {
public:
	SpyState() : touched(0) {}
	Set touched;

	void Init() { touched = 0; }
	void Touch(int j) { touched += Set::BitMask(j); }
	bool Touched () { return touched != 0; }
	int ExtractColIdx() { return touched.ExtractIdx();  }
};

enum { DUMPALL, DUMPMAT, DUMPSORTED, DUMPRINGS, DUMPLANES, DUMPPSET };

class SolveEnv : public LogClient {
public:
	SolveEnv(int _n) : n(_n)
	{
		Rows = new SLane[n];
		Cols = new SLane[n];

		for (int l = 0; l < n; l++) {
			Rows[l].Init(n, l);
			Cols[l].Init(n, l);
		}

		pChgStk = new ChangeStack(n * n * BLANKCNTMAX);
	}

	int n;
	InOut<u8> IO;

	FastMatrix<6, u8> Mat;
	FastMatrix<6, u8> MatCpy;
	SLane* Rows;
	SLane* Cols;

	bool Read(string& fname);
	bool Read(char* fname);
	bool Dump(ostream& out, bool doSort, bool doRings);
	void DumpMat(ostream& out, bool doSort);
	void DumpRings(ostream& out);
	void DumpLanes(ostream& out);
	void ProcessMat();
	void FillBlanks();

	void LinkBlanks();

	// Column sorting
	LaneRank<SLane> LaneTab[DIMMAX];
	void SortCols(ostream& out, bool doSort = true);
	enum { SORTUP, SORTDOWN};
	void SortTab(int order);

	// Solve auxiliary vars
	int solveCnt;
	int colLevel;
	SLane* pColCur;

	int Solve();
	void SolveInit(int lvlFr = 0);
	void SolveFailed(int depth);
	bool PublishSolution();
	int FailCnt[DIMMAX];

	int SolveCnt(int lvlFrom, int lvlTo, int iFrom, int iTo);
	void SolveCntInit(int lvlFrom, int lvlTo, int iFrom, int iTo);
	void CountSolution(int found);

	// Change state saving
	ChangeStack* pChgStk;
	//RestrictBuf RBuf[BLANKCNTMAX];

	// Restricts next level rows starting at pCol->firstPermLvl
	//       Returns failure level or -1 if success
	int RestrictRows(SLane* pCol);
	void UnRestrictRows(SLane* pCol, int depth);
	void RestoreRows(SLane* pCol);
	bool UpdateEntries(SLane* pCol);

	// Restricts next level rows for counting
	// starting at pCol->firstPermLvl
	void RestrictRowsCount(SLane* pCol);

	SpyState Spy;
	bool SpyDownStream(BEntry* pEnt, Set laneMask);
	bool SpyPropagate();

	// Stack based processing
	SLane* pClog;
	void Backtrack();
	void BacktrackRows();
	int BackRow(int d);

	// Initial histo processing
	u8 Histo[DIMMAX];
	u8 HistoLastCol[DIMMAX];
	int histoCnt;
	void HistoInit();
	void HistoAdd(Set s, int idx);
	int HistoScan(ostream& out, int j);
	int HistoFill(ostream& out);
	int HistoScan(int j);
	int HistoFill();

	void Dump(ostream& out, int dumpType);

};


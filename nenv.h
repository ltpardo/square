#pragma once
#include "latsq.h"
#include "senv.h"
#include "nlane.h"

class  RStkEnt {
public:
	RStkEnt() : hits(0), visits(0) {}

	u64 hits;
	int visits;

	inline void Reset() {
		hits = 0;
		visits = 0;
	}

	inline void NewVisit() {
		hits++;
		visits = 0;
	}
	inline void Success() {
		visits++;
	}
};

class NEnv : public LogClient {
public:
	NEnv() {}

	NEnv(int _n) : n(_n)
	{
		Init();
	}

	void Init() {
		Rows = new NLane[n];
		Cols = new NLane[n];

		for (int l = 0; l < n; l++) {
			Rows[l].Init(n, l, true);
			Cols[l].Init(n, l, false);
		}

		NHisto.Init(n, Rows, Cols, &Mat);
		RngTrack.pOutRep = &cout;
	}

	int n;
	InOut<u8> IO;

	FastMatrix<FASTMATDIMLOG, u8> Mat;
	FastMatrix<FASTMATDIMLOG, u8> MatCpy;
	NLane* Rows;
	NLane* Cols;
	int ColsPermCnt[DIMMAX];

	bool Read(string& fname);
	bool Read(char* fname);

	void ReadLineRaw(ifstream& in, u8* pLine, int cnt);
	bool ReadRaw(string& fname, string sortMode);
	bool SortRaw(bool ascending);

	void OutLineRaw(ostream& out, u8* pLine, int i, int cnt);
	bool WriteRaw(string& fname);

	enum { DPARMAX = 8 };
	int DPars[DPARMAX];
	bool DumpSet(int par, int parVal);
	bool DumpLane(ostream& out);
	bool DumpPSet(ostream& out);
	void DumpMat(ostream& out);
	void DumpBack(ostream& out);
	void DumpDist(ostream& out);
	void ProcessMat();

	int blankCnt;			// Total blank Count
	void FillBlanks();
	int RegenPSets();
	int UniqueRemove();

	NEntry* pFirst;
	NEntry* pLast;
	void LinkBlanks();

	// Entry based search
	int solveCnt;
	int colLevel;
	NLane* pColCur;

	inline bool SearchThruSuccess() { return pColCur->idx >= n - 1; }
	inline void SearchThruInit();
	inline void SearchThruInitCol();
	int SearchThru();
	inline void SearchUpCol();
	inline void SearchDnCol();
	void SearchPublish();
	void SearchThruDump(bool isDown);

	RangeTrack RngTrack;

	// Initial histo processing
	Histo<NLane> NHisto;

	void Dump(ostream& out, int dumpType);

	// Compute Search Distances
	int distMax;
	static const int DISTMAX = DIMMAX * 2;
	NEntry* DistLvl[DISTMAX];
	NEntry* DistLvlLast[DISTMAX];
	int DistLvlCnt[DISTMAX];
	int DistLvlBase[DISTMAX];

	static const int INDEXMAX = BLANKCNTMAX * DIMMAX;
	NEntry* DEntIndex[INDEXMAX];

	void SearchDistInit();
	bool SearchDistInsert(NEntry *pEnt);
	bool SearchDistThru(int j);
	void SearchDistLink();
	bool SearchDistComp();
	bool SearchDSuccess();

	// Distance Search
	int dLevel;
	int dPos;
	BitSet* pBSet;
	NEntry* pEnt;

	void SearchDInit();
	int SearchD();

	// Reduction Search
	int level;
	bool fromAbove;
	int mode;
	int fDepth;

	RStkEnt RStk[DIMMAX * 2];
	inline void RLevellDown() {
		level++;
		RStk[level].hits++;
		RStk[level].visits = 0;
	}

	int redLevel;
	NLane* pLaneCur;
	int SearchRed();
	void SearchRedInit(int redLevel);
	void SearchUpRedSuccess();
	void SearchUpRedDone();
	void SearchDnRed();
	void RedPublish();
	void SearchRedDump();

	NLane* pRowCur;
};


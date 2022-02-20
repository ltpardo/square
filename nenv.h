#pragma once
#include "latsq.h"
#include "senv.h"
#include "nlane.h"

template<class Lane> class Histo : public LogClient {
public:
	Histo() {}

	void Init(int _n, Lane* _Rows, Lane* _Cols,
		FastMatrix<FASTMATDIMLOG, u8>* _pMat)
	{
		n = _n;
		Rows = _Rows;
		Cols = _Cols;
		pMat = _pMat;
	}

	int n;
	Lane* Rows;
	Lane* Cols;
	FastMatrix<FASTMATDIMLOG, u8> * pMat;

	// Initial histo processing
	u8 VCnt[DIMMAX];
	u8 LastCol[DIMMAX];
	int cnt;
	void Start();
	void Add(Set s, int idx);
	int Scan(int j);
	int Fill();


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
		pOutRep = &cout;
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
	void ProcessMat();

	void FillBlanks();
	int RegenPSets();
	int UniqueRemove();

	NEntry* pFirst;
	NEntry* pLast;
	void LinkBlanks();
	void LinkPSets();

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

	char searchDir;
	ostream* pOutRep;
	void SearchReport(char dir) {
		if (dir != searchDir) {
			if (dir == 'D')
				*pOutRep << endl;
			else
				*pOutRep << "  /// ";
			searchDir = dir;
		}
		*pOutRep << searchDir << colLevel << " ";
	}

	int lvlMin, lvlMax;
	int lvlTotal;
	int period;
	long long searchIdx;
	const int searchIdxPeriod = 1000000;
	void RangeInit() {
		searchIdx = 0;
		lvlTotal = 0;
		Tim.Start();
		PeriodInit();
	}

	void PeriodInit() {
		lvlMin = 0xff;
		lvlMax = 0;
		period = searchIdxPeriod;
	}

	void RangeColl() {
		period--;
		if (lvlMin > colLevel)
			lvlMin = colLevel;
		if (lvlMax < colLevel)
			lvlMax = colLevel;
		if (period == 0) {
			RangeDump();
			if (lvlTotal < lvlMax)
				lvlTotal = lvlMax;
			PeriodInit();
			searchIdx++;
		}
	}

	SimpleTimer Tim;
	long long ms;

	void RangeDump() {
		ms = Tim.LapsedMSecs();
		*pOutRep << "[" << dec << setw(9) << ms/1000 << " s] ";
		*pOutRep << "ITER " << dec << setw(8) << searchIdx
			<< " MAX " << setw(2) << lvlTotal << " ";
		for (int i = 0; i < lvlMin; i++)
			*pOutRep << "  -";
		for (int i = lvlMin; i <= lvlMax; i++)
			*pOutRep << setw(3) << i;
		*pOutRep << endl;
	}

#if 0
	// Solve auxiliary vars

	void SolveInit(int lvlFr = 0);
	int Solve();
	bool PublishSolution();
	int RestrictRows(NLane* pCol);
	void SolveFailed(int depth);
	// Column sorting
	LaneRank<SLane> LaneTab[DIMMAX];
	void SortCols(ostream& out, bool doSort = true);
	enum { SORTUP, SORTDOWN };
	void SortTab(int order);


	int FailCnt[DIMMAX];

	int SolveCnt(int lvlFrom, int lvlTo, int iFrom, int iTo);
	void SolveCntInit(int lvlFrom, int lvlTo, int iFrom, int iTo);
	void CountSolution(int found);

	// Change state saving
	ChangeStack* pChgStk;

	// Restricts next level rows starting at pCol->firstPermLvl
	//       Returns failure level or -1 if success
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
#endif

	// Initial histo processing
	Histo<NLane> NHisto;

	void Dump(ostream& out, int dumpType);

};


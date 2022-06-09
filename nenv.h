#pragma once
#include "latsq.h"
#include "senv.h"
#include "nlane.h"

class EnvPars {
public:
	EnvPars() : refDir(""), makeRef(false), checkRef(false) {}

	bool SetPar(string& name, string& val) {
		bool valB = (val == "true");
		if (name == "makeref")
			makeRef = valB;
		else if (name == "checkref")
			checkRef = valB;
		else if (name == "refdir")
			refDir = val;
		else
			return false;
		return true;
	}

	string refDir;
	bool makeRef;
	bool checkRef;
};

extern EnvPars EPars;

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

class EnvBase : public LogClient {
public:
	EnvBase() {}

	int n;
	FastMatrix<FASTMATDIMLOG, u8> Mat;
	int blankCnt;			// Total blank Count

	enum { DPARMAX = 8 };
	int DPars[DPARMAX];
	bool DParsSet(int par, int parVal);

	InOut<u8> IO;

	// Result Mat
	FastMatrix<FASTMATDIMLOG, u8> RMat;
	bool VerifyLatSq(FastMatrix<FASTMATDIMLOG, u8> &M);

	int ColsPermCnt[DIMMAX];
	int RowsPermCnt[DIMMAX];

	bool Read(string& fname);
	bool Read(char* fname);

	void ReadLineRaw(ifstream& in, u8* pLine, int cnt);
	bool InputRaw(string& fname, string sortCols);
	bool InputSource(string& fname, string sortRows, string sortCols);
	bool CopyRaw(EnvBase &Src, string& sortRow, string& sortCol);

	// Sorting 
	bool rowsSorted;
	s8 RowUnsort[DIMMAX];
	bool colsSorted;
	s8 ColUnsort[DIMMAX];

	void SortMat(string &sortRows, string& sortCols);

	void SortNone() {
		rowsSorted = false;
		colsSorted = false;
	}

	// Sorts lanes and sets corresponding unsort table
	bool SortRaw(bool sortRows, int srcCnt[], s8 unsort[], bool ascending);
	bool ShuffleMats(FastMatrix<FASTMATDIMLOG, u8> &M, bool shuffleRows, s8 shufTab[]);

	void OutLineRaw(ostream& out, u8* pLine, int i, int cnt);
	bool WriteRaw(string& fname);

	void RandGenRaw(int n, int blkCnt, int seed);
	void ImportRaw(u8 Src[FASTMATDIM][FASTMATDIM]);

	// Building
	LaneFull* RowFull[DIMMAX];
	LaneFull* ColFull[DIMMAX];

	// Create all lane fulls
	//  Fills *Full indices
	void CreateFull();

	// Compute absMiss and missCnt for all full lanes
	// Compute blankCnt (# of blank entries in square
	void BuildFull();

	// Initial histo processing
	Histo<LaneFull> NHisto;
	void HistoScan();

	virtual void FillEnts() = 0;
	enum {
		GENREF = 1,
		GENEXP = 2,
		GENLVL = 3,
		GENALL = 4
	};
	void GenPerms(int kind, bool doRow, int idxFrom, int idxTo, bool doCheck);

	void Dump(ostream& out, int dumpType);

	void DumpMat(ostream& out, FastMatrix<FASTMATDIMLOG, u8> &DMat);
	void DumpMissing(ostream& out, const char *name) {
		out << "MISSING " << name << endl;
	}

	virtual void DumpBack(ostream& out) { DumpMissing(out, "DumpBack"); }
	virtual void DumpLane(ostream& out) { DumpMissing(out, "DumpLane"); }
	//virtual void DumpPSet(ostream& out) { DumpMissing(out, "DumpPSet"); }
	void DumpPSet(ostream& out);
	virtual void DumpDist(ostream& out) { DumpMissing(out, "DumpDist"); }
};

class NEnv : public EnvBase {
public:
	NEnv() {}

	NEnv(int _n) 
	{
		n = _n;
		Init();
	}

	void Init() {
		Rows = new NLane[n];
		Cols = new NLane[n];

		for (int l = 0; l < n; l++) {
			Rows[l].Init(n, l, true);
			Cols[l].Init(n, l, false);
		}

		NHisto.Init(n, RowFull, ColFull, &Mat);
		RngTrack.pOutRep = &cout;
	}

	NLane* Rows;
	NLane* Cols;


	// Dump methods
	virtual void DumpLane(ostream& out);
	virtual void DumpPSet(ostream& out);
	virtual void DumpBack(ostream& out);
	virtual void DumpDist(ostream& out);

	// BUilding
	//
	bool ReadRaw(string& fname, string sortMode);
	void ProcessMat();
	void FillBlanks();
	int RegenPSets();
	int UniqueRemove();
	void LinkBlanks();

	virtual void FillEnts();

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
	//
	int dLevel;
	int dPos;
	BitSet* pBSet;
	NEntry* pEnt;

	void SearchDInit();
	int SearchD();

	// Reduction Search
	//
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


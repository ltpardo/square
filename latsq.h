#pragma once
#include "stdinc.h"
#include <iomanip>
#include <fstream>

class Entry;

class Blank {
public:
	Blank() {}

	void Set(Entry* _pEnt, int _idx) {
		pEnt = _pEnt;
		idx = _idx;
	}

	Entry* pEnt;
	int idx;
	u64 choiceSet;		// chose among these
	u64 selSet;			// selected values so far
	u64 chosenBit;		// chosen value bit
	u8 selV;
};

template <class M> class Lane {
public:
	Lane() {}
	Lane(int _n) : n(_n) {}

	void Init(int _n) {
		n = _n;
		assert(n < DIMMAX - 2);
		all = (ONE << n) - 1;
		missing = all;
		present = 0;
		presCnt = 0;
		missCnt = n;

		blankCnt = 0;

		optEstimate = 1;
	}

	static const u8 DIMMAX = sizeof(M) * 8;
	static const M ONE = 1;
	inline M BitMask(u8 v) { return ONE << v; }
	int n;

	M present;
	int presCnt;
	M missing;
	int missCnt;
	M all;
	u8 Map[DIMMAX];

	Blank BStack[DIMMAX];
	int blankCnt;

	double optEstimate;
	void EstimateFactor(int opt) {
		optEstimate *= opt;
	}

	void Present(u8 v);
	void Missing(u8 v);
	void AddBlank(Entry* pEnt, int idx);
	bool Check();

	u64 visitCnt;
	u64 failCnt;
	Blank* pBlank;
	int level;

	bool PermuteStart()
	{
		visitCnt = 0;
		failCnt = 0;
		pBlank = BStack;
		pBlank->choiceSet = pBlank->pEnt->missMask;
		pBlank->selSet = 0;
		level = 0;

		return true;
	}

	bool PermuteStep()
	{
		while (level >= 0) {
			// Entering level
			if (pBlank->choiceSet == 0) {
				// Failed branch
				failCnt++;
				// Climb up
				pBlank--;
				level--;
			}
			else {
				// Select a candidate
				pBlank->chosenBit = pBlank->choiceSet;
				pBlank->chosenBit &= (~pBlank->chosenBit + 1);

				//   Delete selected
				pBlank->choiceSet &= (~pBlank->chosenBit);

				// Branch has a candidate
				if (level == missCnt - 1) {
					// Bottom branch: found one
					//   Only one candidate should remain...
					assert(pBlank->choiceSet == 0);

					visitCnt++;
					// Climb up
					pBlank--;
					level--;
					return true;
				}
				else {
					//   and add it to the selected set for next level
					u64 newselSet = pBlank->selSet | pBlank->chosenBit;

					// ... and proceed down
					pBlank++;
					level++;
					pBlank->selSet = newselSet;
					pBlank->choiceSet = pBlank->pEnt->missMask
						& (~newselSet);
				}
			}
		}

		// Iteration finished
		return false;
	}

	bool VerifyPerm();

	u64 PermuteCount();
	u64 PermuteCountOld();
};

template<class L> class LaneRank {
public:
	LaneRank() {}

	L* pLane;
	u64 permCnt;
	int idx;
};

class Entry {
public:
	Entry() : val(0), optCnt(0), missMask(0) {}
	u8 val;
	u8 optCnt;

	u8 operator= (u8 v) { return val = v; }

	u64 missMask;
	u64 visitMask;

	static const u8 BLANK = 99;
	static const u8 INVALID = 99;

	u8 CharToVal(char c) {
		optCnt = 1;
		if (c == '.') {
			val = BLANK;
			optCnt = 0;
		}
		else if (c >= '1' && c <= '9')
			val = c - '1';
		else if (c >= 'a' && c <= 'z')
			val = c - 'a' + 9;
		else {
			cerr << "  INVALID INPUT " << c << endl;
			val = INVALID;
			optCnt = 0;
		}

		return val;
	}

	int OptCount(Lane<u64>& row, Lane<u64>& col);
};

class LatSqEnv {
public:
	LatSqEnv(int _n) : n(_n)
	{
		Rows = new Lane<u64>[n];
		Cols = new Lane<u64>[n];

		for (int l = 0; l < n; l++) {
			Rows[l].Init(n);
			Cols[l].Init(n);
		}
	}

	int n;
	InOut<Entry> IO;

	FastMatrix<6, Entry> Mat;
	FastMatrix<6, Entry> MatCpy;
	Lane<u64>* Rows;
	Lane<u64>* Cols;

	bool Read(char* fname);
	bool Dump(ostream& out);
	void ProcessMat();

	LaneRank<Lane<u64>> LaneTab[Lane<u64>::DIMMAX];
	void SortCols(bool doSort = true);

	int solveCnt;
	int colLevel;
	Lane<u64>* pColCur;
	Lane<u64>* pColNext;

	int Solve();
	void SolveInit();
	bool PublishSolution();
	int FailCnt[Lane<u64>::DIMMAX];
	int ClogCnt[Lane<u64>::DIMMAX];

	int RestrictRows(Lane<u64> *pCol);
	void RestoreRows(Lane<u64>* pCol);
	int UpdateEntries(Lane<u64>* pCol);
};


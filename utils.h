#pragma once
#include "basic.h"
#include "assert.h"
#include <intrin.h>
#include <algorithm>
#include <string>
#include "simpletimer.h"
#include "logger.h"
#include "sets.h"

using namespace std;


static const int DIMMAX = sizeof(u64) * 8;
static const int BLANKCNTLOG = 4;
static const int BLANKCNTMAX = 1 << BLANKCNTLOG;
static const int BLANKCNTMASK = BLANKCNTMAX - 1;

enum { DUMPALL, DUMPMAT, DUMPBACK, DUMPSORTED, DUMPRINGS, 
	DUMPLANES, DUMPPSET, DUMPDIST, DUMPRSLT };

class DebugState {
public:
	DebugState() : dumpExpand(0), dumpTrace(0), dumpRange(0),
		dumpCompare(0), dumpTerm(0), dumpMakeSingle(0), dumpResult(0),
		dumpLevel(DIMMAX * 2), watchLevel(INT_MAX), searchLim(INT_MAX),
		noPotSort(true), lowPotFirst(true), doLadder(0), reportCreate(false),
		reportSearch(false), inspect(-1)
	{
		pOut = &cout;
		watchLevel = 36;
	}

	typedef long long Counter;
	Counter dumpExpand;
	Counter dumpTrace;
	Counter dumpRange;
	Counter dumpCompare;
	Counter dumpTerm;
	Counter dumpMakeSingle;
	Counter dumpResult;
	Counter searchLim;

	// Limits
	int dumpLevel;
	int watchLevel;

	// Params
	bool noPotSort;
	bool lowPotFirst;
	int doLadder;
	bool reportCreate;
	bool reportSearch;
	int inspect;

public:
	ostream* pOut;
	string fname;
	ofstream out;

	bool SetOut(string& name) {
		if (pOut != NULL && pOut != &cout)
			out.close();
		fname = name;
		out.open(fname);
		pOut = &out;
		return out.is_open();
	}

	Counter DmpExpand() { return dumpExpand > 0 ? dumpExpand-- : 0; }
	Counter DmpTrace() { return dumpTrace > 0 ? dumpTrace-- : 0; }
	Counter DmpRange() { return dumpRange > 0 ? dumpRange-- : 0; }
	Counter DmpCompare() { return dumpCompare > 0 ? dumpCompare-- : 0; }
	Counter DmpTerm() { return dumpTerm > 0 ? dumpTerm-- : 0; }
	Counter DmpMakeSingle() { return dumpMakeSingle > 0 ? dumpMakeSingle-- : 0; }
	Counter DmpResult() { return dumpResult > 0 ? dumpResult-- : 0; }
	Counter SearchLim() { return searchLim > 0 ? --searchLim : 0; }

	bool Inspect() { return inspect >= 0; }
	bool InspectCollect() { return inspect == 2; }
	bool InspectPause() { return inspect >= 1; }

	bool Set(string& par0, int cnt) {
		if (par0 == "exp") {
			dumpExpand = cnt;
		}
		else if (par0 == "trace") {
			dumpTrace = cnt;
		}
		else if (par0 == "range") {
			dumpRange = cnt;
		}
		else if (par0 == "watch") {
			watchLevel = cnt;
		}
		else if (par0 == "comp") {
			dumpCompare = cnt;
		}
		else if (par0 == "ladder") {
			doLadder = cnt;
		}
		else if (par0 == "term") {
			dumpTerm = cnt;
		}
		else if (par0 == "single") {
			dumpMakeSingle = cnt;
		}
		else if (par0 == "searchlim") {
			searchLim = cnt;
		}
		else if (par0 == "dumpres") {
			dumpResult = cnt;
		}
		else if (par0 == "reportcreate") {
			reportCreate = true;
		}
		else if (par0 == "reportsearch") {
			reportSearch = true;
		}
		else if (par0 == "inspect") {
			inspect = cnt;
		}
		else {
			cerr << " DEBUG bad param " << par0 << endl;
			return false;
		}
		return true;
	}
};

extern DebugState DbgSt;

enum { BLKENT = 99 };

template<class Ent> class InOut {
public:
	InOut() {}

	string fname;
	ifstream in;

	static const u8 BLANK = 99;
	static const u8 INVALID = 99;

	u8 CharToVal(char c) {
		u8 val;
		if (c == '.') {
			val = BLANK;
		}
		else if (c >= '1' && c <= '9')
			val = c - '1';
		else if (c >= 'a' && c <= 'z')
			val = c - 'a' + 9;
		else {
			cerr << "  INVALID INPUT " << c << endl;
			val = INVALID;
		}

		return val;
	}

	bool ReadOpen(char* _fname) {
		fname = _fname;
		in.open(_fname, ios::in);

		return in.is_open();
	}

	bool ReadOpen(string& _fname) {
		fname = _fname;
		in.open(_fname, ios::in);

		return in.is_open();
	}

	char buf[150];
	void ReadLine(Ent* pLine, int skip, int cnt) {
		in.getline(buf, sizeof(buf));

		for (int p = 0; p < cnt; p++)
			pLine[p] = CharToVal(buf[skip + p]);
	}

	void OutLine(ostream& out, Ent* pLine, int i, int cnt) {
		out << "[" << setw(2) << i << "]  ";
		for (int j = 0; j < cnt; j++) {
			out << " " << setw(2) << (int)pLine[j].val;
		}
		out << endl;
	}

	void OutHeader(int cnt, Ent* pHdr = 0) {
		*pOut << "      ";
		for (int j = 0; j < cnt; j++) {
			*pOut << " " << setw(2) << (pHdr == 0 ? j : (int)pHdr[j]);
		}
		*pOut << endl;
	}

	ostream* pOut;
	void OutSet(ostream& out) { pOut = &out;  }

	void OutHeaderDirect(int cnt, int wd = 2) {
		*pOut << "      ";
		for (int j = 0; j < cnt; j++) {
			*pOut << " " << dec << setw(wd) << j;
		}
		*pOut << endl;
	}

	void OutRowNum(int r) {
		if (r >= 0)
			*pOut << "[" << dec << setw(2) << r << "]  ";
		else
			*pOut << "      ";
	}

	void OutSep(int j) {
		*pOut << (((j & 3) != 0) ? " " : "|");
	}

	void OutPad(int wd) {
		char bl[] = "        ";
		int disp = wd > sizeof(bl) ? 0 : sizeof(bl) - wd - 1;
		*pOut << bl + disp;
	}

	void OutPair(char desc, int v, int wd) {
		*pOut << desc << dec << setw(wd - 1) << v;
	}

	void OutStr(const char *str) {
		*pOut << str;
	}

	void OutVal(int v, int wd) {
		*pOut << dec << setw(wd) << v;
	}

	void EndLine() { *pOut << endl;  }

	void OutLineV(Ent* pLine, int i, int cnt, int wd = 2) {
		char bl[] = "       *";
		int disp = wd > sizeof(bl) ? 0 : sizeof(bl) - wd - 1;
		OutRowNum(i);
		for (int j = 0; j < cnt; j++) {
			OutSep(j);
			if ((int)pLine[j] > cnt)
				*pOut << bl + disp;
			else
				*pOut << setw(wd) << (int)pLine[j];
		}
		*pOut << endl;
	}

};

class BktHash {
public:
	BktHash() {}

	enum { 
		BKTCNTLOG = 5,
		BKTCNT = 1 << BKTCNTLOG,
		BKTCNT1_2 = 1 << (BKTCNTLOG - 1),
		BKTCNT1_4 = 1 << (BKTCNTLOG - 2),
		BKTCNT1_8 = 1 << (BKTCNTLOG - 3),

	};

	const int Map8[16] = {
		3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 0, 0
	};

	int BktFromQuarter(int v) {
		int half;
		if ((half = v >> BKTCNT1_8) == 0) {
			// Bottom eightth
			return Map8[v];
		} else
			return Map8[half] + BKTCNT1_8;
	}

	int Bkt(int v) {
		int bkt;
		int half;
		if ((half = v >> BKTCNT1_2) == 0) {
			// Low Half buckets
			if ((half = v >> BKTCNT1_4) == 0) {
				// Quarter 0 buckets
				bkt = BktFromQuarter(v);
			}
			else {
				// Quarter 1 buckets
				bkt = BktFromQuarter(half) + BKTCNT1_4;
			}
		}
		else {
			// Upper Half buckets
			v = half;
			if ((half = v >> BKTCNT1_4) == 0) {
				// Quarter 2 buckets
				bkt = BktFromQuarter(v) + BKTCNT1_2;
			}
			else {
				// Quarter 3 buckets
				bkt = BktFromQuarter(half) + BKTCNT1_2 + BKTCNT1_4;
			}
		}

		return bkt;
	}

};

typedef int SortKey;

template <class Node, int AREASZLOG> class AreaMgr {
public:
	AreaMgr() {}
	enum {
		AREASZ = 1 << AREASZLOG,
		ARESZMASK = AREASZ - 1
	};

	int szAreas;

	int* AreaTab;
	int freeHd;
	Node* AreaPool;
	void InitAlloc(int _szAreas) {
		szAreas = _szAreas;
		AreaTab = new int[szAreas];
		AreaPool = new Node * [szAreas << AREASZLOG];

		freeHd = 0;
		int idx;
		for (idx = 0; idx < szAreas - 1; idx++)
			AreaTab[idx] = idx + 1;
		AreaTab[idx] = 0;
	}
};

template <class Node, int AREASZLOG> class BucketStore {
public:
	BucketStore() {}

	BktHash BHash;
	typedef struct {
		Node* pBase;
		SortKey kHi;
		SortKey kLo;

	} BktEnt;

	BktEnt BktTab[BktHash::BKTCNT];

	enum {
		AREASZ = 1 << AREASZLOG,
		ARESZMASK = AREASZ - 1
	};

	AreaMgr<Node, AREASZLOG> AMgr;
	int nodeCntMax;
	int allocSzAreas;
	int bucketCnt;

	bool Alloc(int _nodeCntMax) {
		nodeCntMax = _nodeCntMax;
		allocSzAreas =
			((nodeCntMax + AREASZ - 1) >> AREASZLOG) + BktHash::BKTCNT;
		AMgr.InitAlloc(allocSzAreas);
	}


};

template <class T, int PGSZLOG, int LVLSZPG> class LevelArea {
public:
	LevelArea() {
		free = 0;
		pageId = 0;
		for (int p = 0; p < LVLSZPG; p++)
			PageTab[p] = NULL;
		AllocPage();
	}

	~ LevelArea() {
		for (int p = 0; p <= pageId; p++)
			delete PageTab[p];
	}
	const int PGSZ = 1 << PGSZLOG;
	const int PGSZMASK = PGSZ - 1;

	int free;
	int pageId;
	int pageAvail;

	void AllocPage() {
		assert(pageId < LVLSZPG);
		pageAvail = PGSZ;
		pPage = new T[PGSZ];
		PageTab[pageId] = pPage;
	}

	T* PageTab[LVLSZPG];
	T* pPage;

	int Store(T* pSrc, int sz) {
		int resDisp = free;
		int cpySz;
		for (; ; ) {
			cpySz = sz;
			if (cpySz > pageAvail)
				cpySz = pageAvail;
			memcpy_s(pPage + (free & PGSZMASK), pageAvail * sizeof(T), pSrc, cpySz * sizeof(T));
			free += cpySz;
			if ((pageAvail -= cpySz) <= 0) {
				pageId++;
				AllocPage();
			}
			if ((sz -= cpySz) > 0)
				pSrc += cpySz;
			else
				break;
		}
		return resDisp;
	}

	T* CopyLevel(T* pDst, int& availSz) {
		if (availSz < free)
			return NULL;

		int cpySz;
		int pendSz = free;
		for (int p = 0; p <= pageId; p ++) {
			cpySz = pendSz < PGSZ ? pendSz : PGSZ;
			pPage = PageTab[p];
			memcpy_s(pDst, availSz * sizeof(T), pPage, cpySz * sizeof(T));
			pDst += cpySz;
			availSz -= cpySz;
			if ((pendSz -= cpySz) <= 0)
				break;
		}
		return pDst;
	}
};

template <class T, int PGSZLOG, int LVLCNT, int LVLSZPG> class LevelStore {
public:
	LevelStore(int _depth) {
		depth = _depth;
		totalSz = 0;
		for (int l = 0; l < depth; l++) {
			LArea[l] = new LevelArea<T, PGSZLOG, LVLSZPG>();
		}
	}

	~LevelStore() {
		for (int l = 0; l < depth; l++) {
			delete LArea[l];
		}
	}

	const int PGSZ = 1 << PGSZLOG;
	const int PGSZMASK = PGSZ - 1;

	int depth;
	int totalSz;
	LevelArea<T, PGSZLOG, LVLSZPG> *LArea[LVLCNT];

	int Store(T* pSrc, int level, int sz) {
		totalSz += sz;
		return LArea[level]->Store(pSrc, sz);
	}

	int StoredSz() { return totalSz; }

	T* CopyLinear(T **LvlBase) {
		T* pArea = new T[totalSz];
		T* pRes = pArea;
		int availSz = totalSz;
		for (int l = 0; l < depth; l++) {
			LvlBase[l] = pArea;
			pArea = LArea[l]->CopyLevel(pArea, availSz);
		}
		assert(availSz == 0);
		return pRes;
	}
};

typedef u32 SortT;
typedef int SortIdx;
enum { SORTMAX = BLANKCNTMAX };
class Sorter {
public:
	Sorter() {}

	void SetPs(SortT* _pS) { pS = _pS;  }
	int n;
	SortT* pS;
	SortT Buf[SORTMAX];
	SortT Aux[SORTMAX];

	inline void Swap(SortT& a, SortT& b) {
		SortT temp;
		temp = a;
		a = b;
		b = temp;
	}

	void Sort(SortIdx _n);
	void SetBuf();
	void Merge2x1(SortIdx iFr);
	void Merge2x2s();
	void Merge(SortIdx iFr, SortIdx aSz, SortIdx bSz);

	bool Test(int testCnt, int size);

	void TestSort()
	{
		Sorter TSort;
		for (int sz = 2; sz <= 12; sz++) {
			cout << "Testing sz " << sz << endl;
			TSort.Test(2000, sz);
		}
	}
};

typedef u16 LMask;
typedef u16 ShortLink;
typedef u32 LongLink;
typedef u16 ShortCnt;
typedef u32 LongCnt;
typedef u32 LvlBranch;

// Reference  PSet
class PermSetRef {
public:
	PermSetRef() {}

	PermSetRef(bool _isRow, int _idx, int _depth, Set & absMiss, Set *_pMissTab)
		: isRow(_isRow), idx(_idx), depth(_depth), cnt(0) {
		totalMiss = absMiss;
		pMissTab = _pMissTab;
		for (int d = 0; d < depth; d++)
			V[d] = 0xFF;
	}

	int idx;
	int depth;
	bool isRow;
	int cnt;
	ofstream out;

	string fname;
	u8 V[BLANKCNTMAX];
	void Name();
	void Create();
	void Out();
	int Close();

	ifstream in;
	int readCnt;
	int errCnt;
	bool ReadOpen();
	void ReadClose();
	bool Read();

	bool Compare(u8* pCmp) {
		bool success = true;
		for (int i = 0; i < depth; i++) {
			if (pCmp[i] != V[i]) {
				errCnt++;
				success = false;
			}
		}
		return success;
	}

	Set totalMiss;
	Set* pMissTab;
	bool Gen(int level, Mask sel, ShortCnt& cnt);

};

class PermCounts {
public:
	PermCounts() {}

	void SetEmpty() {
		Init(0);
	}

	int permCnt;
	int nodeCnt;
	int singleCnt;
	int depth;
	int depthTerm;		// Level at which terminal encoding starts

	int LvlCnt[BLANKCNTMAX];
	int LvlMax[BLANKCNTMAX];
	int NodeCnt[BLANKCNTMAX];
	int SingleCnt[BLANKCNTMAX];
	void Init(int _depth);
	void InitLevels();

	bool badCounts;
	bool CheckCounts();

	bool outOflo;
};

class PermBase {
public:
	PermBase() {}

	bool isRow;
	int idx;
	string searchId;
	void SetId(bool _isRow, int _idx) {
		isRow = _isRow;
		idx = _idx;
		searchId = isRow ? "ROW" : "COL";
		searchId += to_string(idx);
	}

	void SetEmpty() { 
		depth = 0;
		Counts.SetEmpty();
	}

	bool IsEmpty() { return depth == 0; }

	LongLink baseLnk;
	int vCnt;
	u8 V[BLANKCNTMAX];
	void VDictSet(Set abs) {
		abs.ExtractValues(V, sizeof(V));
	}

	int termHt;			// Number of levels for terminal encoding
	int depthTerm;		// Level at which terminal encoding starts

	enum { LNKNULL = 0xFFFF };
	int depth;			// Lane depth == Lane missCnt

	PermCounts Counts;
	PermSetRef* pChkRef;
	bool VisitProcess();

	u8 VisitV[BLANKCNTMAX];
	Set VisitMiss[BLANKCNTMAX];

	static const LongLink LONGLNKNULL = -1;
};

class PermExp : public PermBase {
public:
	PermExp() : baseLnk(0), Pool(0), TempPool(0), freeLnk(0),
		lMissing(0), pMiss(0), missCnt(0)
	{}

	enum {
		POOLSZ = 1 << 19,
		TEMPAREASZ = BLANKCNTMAX * 4,
	};

	LongLink baseLnk;
	LongLink* Pool;
	LongLink* TempPool;
	int freeLnk;

	void Alloc();
	void Dealloc();

	LMask lMissing;
	LMask *pMiss;
	int missCnt;

	void GenInit(LMask _lMissing, LMask *_pMiss, int _missCnt) {
		lMissing = _lMissing;
		pMiss = _pMiss;
		missCnt = _missCnt;
		depth = missCnt;
	}
	LongLink Gen(int level, LMask sel, ShortCnt& cnt);

	LongLink* TempArea(int level);
	LongLink* NodeStart(int level);
	LongLink* NodeBranch(LongLink* pTemp, ShortCnt cnt, LongLink lnk);
	LongLink* NodeBotBranch(LongLink* pTemp, LongLink mask);
	LongLink NodeEnd(int level, LongLink* pEnd, ShortCnt cnt, ShortCnt vMask);
	bool VerifyCounts();
	void FollowBranch(int level, LongLink lnk);

	bool CheckRef(PermSetRef* pRef);
	bool Visit(int level, LongLink lnk);
};

#define SIMPLE_PSET

#ifdef SIMPLE_PSET
class PermSet : public PermBase {
public:
	PermSet() {}

	enum { TEMPAREASZ = BLANKCNTMAX + 2 };
	void Init(int _depth, int _termHt, bool countOnly);

	///////////////////////////////////////////////////////
	//		LEVELED PSET
	///////////////////////////////////////////////////////
	enum {
		LVSZ1 = 6,
		LVSZ1MASK = (1 << LVSZ1) - 1,
		SINGLESZ4 = sizeof(LvlBranch) * 8 / LVSZ1,
		ENCODESZ1 = 24,
		BTERMPOS = LVSZ1 + ENCODESZ1,
		BTERMBIT = 1 << BTERMPOS,
		BLASTBIT = 1 << (BTERMPOS + 1),
		LVLNULL = -1,
	};

#if 0
	enum {
		BSMALLSZ1 = 8,
		BLARGESZ1 = 16,
		BLVLSWITCH = 3,
		BSMALLMAX = 1 << BSMALLSZ1,
		BSMALLMASK = BSMALLMAX - 1,
		BLARGEMAX = 1 << BLARGESZ1,
		BLARGEMASK = BLARGEMAX - 1,
		BFLAGSPOS = LVSZ1 + BSMALLSZ1 + BLARGESZ1,
	};

	LvlBranch LvlBranchFill(int level, int cnt, int lvlDisp) {
		LvlBranch branch;
		if (level <= BLVLSWITCH) {
			assert(cnt < BLARGEMAX&& lvlDisp < BSMALLMAX);
			branch = (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + BLARGESZ1));
		}
		else {
			assert(cnt < BSMALLMAX&& lvlDisp < BLARGEMAX);
			branch = (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + BSMALLSZ1));
		}
		return branch;
	}

	inline int LvlGetCntNonTiered(int level, LvlBranch branch) {
		return (branch >> LVSZ1) & ((level <= BLVLSWITCH) ? BLARGEMASK : BSMALLMASK);
	}

#endif

	enum {
		CNTSZ1 = 6,
		CNTMASK = (1 << CNTSZ1) - 1,
		CNTINPOSMASK = CNTMASK << LVSZ1,
		CNTONEMASK = 1 << LVSZ1,
		DISPSZ1 = ENCODESZ1 - CNTSZ1,
		DISPMASK = (1 << DISPSZ1) - 1,
		BRANCHNULL = -1,
	};

	LvlBranch LvlBranchTiered(int cnt, int lvlDisp) {
		LvlBranch branch = 0;
		if (cnt > CNTMASK)
			cnt = 0;
		assert(lvlDisp <= DISPMASK);
		branch = (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + CNTSZ1));
		return branch;
	}
#endif

#if 0
	enum {
		CDLOG = 22,
		FLD1LOG = 6  /* 6 */,
		FLD2LOG = 9  /* 9 */,
		FLD3LOG = CDLOG - FLD2LOG,
		FLD4LOG = CDLOG - FLD1LOG,

		FLD1MAX = 1 << FLD1LOG,
		FLD2MAX = 1 << FLD2LOG,
		FLD3MAX = 1 << FLD3LOG,
		FLD4MAX = 1 << FLD4LOG,

		FLD1MASK = FLD1MAX - 1,
		FLD2MASK = FLD2MAX - 1,
		FLD3MASK = FLD3MAX - 1,
		FLD4MASK = FLD4MAX - 1,

		FLAG1 = 0,
		FLAG2 = 1 << (CDLOG + LVSZ1),
		FLAG3 = 2 << (CDLOG + LVSZ1),
		FLAG4 = 3 << (CDLOG + LVSZ1),

		FLAGMASK = FLAG4,
	};

	LvlBranch LvlBranchTiered(int cnt, int lvlDisp) {
		LvlBranch branch = 0;
		if (cnt < FLD1MAX && lvlDisp < FLD4MAX) {
			branch = (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + FLD1LOG));
		}
		else if (cnt < FLD2MAX && lvlDisp < FLD3MAX) {
			branch = FLAG2 | (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + FLD2LOG));
		}
		else if (cnt < FLD3MAX && lvlDisp < FLD2MAX) {
			branch = FLAG3 | (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + FLD3LOG));
		}
		else if (cnt < FLD4MAX && lvlDisp < FLD1MAX) {
			branch = FLAG4 | (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + FLD4LOG));
		}
		else
			assert(0);

		return branch;
	}

	static inline LvlBranch LvlFlag(LvlBranch branch) { return branch & FLAGMASK; }
#endif

	static void LvlBranchFinish(LvlBranch& branch, int lV, bool isLast) {
		branch |= (lV & LVSZ1MASK);
		if (isLast)
			branch |= BLASTBIT;
	}

	static inline bool LvlIsLast(LvlBranch branch) { return (branch & BLASTBIT) != 0; }
	static inline bool LvlIsTerm(LvlBranch branch) { return (branch & BTERMBIT) != 0; }
	static inline u8 LvlGetV(LvlBranch branch) { return branch & LVSZ1MASK; }
	static inline u8 LvlExtractV(LvlBranch& branch) {
		u8 v = (u8)(branch & LVSZ1MASK);
		// Shift mask down, keeping flags
		branch = ((branch & (~(BLASTBIT | BTERMBIT))) >> LVSZ1)
			| (branch & (BLASTBIT | BTERMBIT));
		return v;
	}

	static inline u8 LvlTermAdv(LvlBranch& branch) {
		// Discard previus v: shift mask down, keeping flags
		branch = ((branch & (~(BLASTBIT | BTERMBIT))) >> LVSZ1)
			| (branch & (BLASTBIT | BTERMBIT));
		u8 v = (u8)(branch & LVSZ1MASK);
		return v;
	}

	static inline int LvlGetCnt(LvlBranch branch) {
		if (LvlIsTerm(branch))
			return 1;

		int cnt = (int)(branch >> LVSZ1) & CNTMASK;
		if (cnt == 0)
			cnt = CNTMASK + 1;
		return cnt;
	}

	static inline bool LvlCntIsOne(LvlBranch branch) {
		if (LvlIsTerm(branch))
			return true;
		else
			return (branch & CNTINPOSMASK) == CNTONEMASK;
	}

	static inline int LvlGetDisp(LvlBranch branch) {
		return (branch >> (LVSZ1 + CNTSZ1)) & DISPMASK;
	}

	static const u32 POTMASK = (1 << (sizeof(u32) * 8 - BLANKCNTLOG)) - 1;
	inline u32 Potential(int pos, LvlBranch b0, LvlBranch b1) {
		u32 pot = LvlGetCnt(b0) * LvlGetCnt(b1);
		if (pot > POTMASK)
			pot = POTMASK;
		return (pot << LVSZ1) | pos;
	}

	inline u32 PotGetPos(u32 pot) { return pot & BLANKCNTMASK; }

	int minLvlForSingles;
	int lvlSz32;
	LevelStore<u32, 12, BLANKCNTMAX, 64>* LvlStore;
	void LvlAlloc();
	void LvlDealloc();
	LvlBranch LvlMakeSingle(int level, LongLink lnk, u16 permMask);
	void LvlFromExp(PermExp* _pExpSet, u16 permMask);
	LvlBranch LvlExpVisit(int level, LongLink lnk, u16 permMask);

	LvlBranch* LvlBase[BLANKCNTMAX];
	LvlBranch* BranchPtr(int level, ShortLink lnk) { return LvlBase[level] + lnk; }
	LvlBranch* BranchPtr(int level, LvlBranch branch) {
		return LvlBase[level] + LvlGetDisp(branch);
	}


	PermExp* pExpSet;
	bool LvlCheckRef(PermSetRef* pRef);
	bool LvlVisit(int level, LvlBranch* pNode);
	bool LvlVisitTerm(int level, LvlBranch br);
	//int LvlSetMiss(NLane *pLane);


	bool ExpCheckRef(PermSetRef* pRef);
	bool ExpVisit(int level, LongLink lnk);


	///////////////////////////////////////////////////////
	//		INTERSECTION
	///////////////////////////////////////////////////////

	u32 Pot[BLANKCNTMAX];
	LvlBranch Branches[BLANKCNTMAX];
	LvlBranch CrBranches[BLANKCNTMAX];

	int iCnt;
	int LvlIntersect(LvlBranch* pT, LvlBranch* pC, Set& ThSet);

	Sorter PSort;
	void LvlSortInter();

	inline int SortedIdx(int i) { return PotGetPos(Pot[i]); }
	LvlBranch LvlGetBranch(int lvl, ShortLink nodeLink, u8 v);
	LvlBranch LvlBranchAdv(int lvl, LvlBranch inBranch, u8 v);

	//  DUMPS
	//
	void Dump(ostream& out, bool doNodes);
	void DumpLvl(ostream& out, int lvl);
	void DumpLvls(ostream& out, int lvlFr, int lvlTo = -1);
	void DumpBranch(ostream& out, int lvl, LvlBranch br);
	void DumpExpanded(ostream& out, int lvl, LvlBranch br);

	string nodeId;
	static void DumpNodeId(ostream& out, bool isRow, int lvl, ShortLink nodeDisp = LVLNULL);
};

class RangeTrack {
public:
	RangeTrack() {}

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
		*pOutRep << searchDir << /*colLevel <<*/ " ";
	}

	int lvlMin, lvlMax;
	int posMin, posMax;
	int lvlTotal;
	int period;
	long long searchIdx;
	const int searchIdxPeriod = 10000000;
	//const int dumpFreq = 10;
	int dumpFreq = 1;
	void Init(ostream* _pOutRep = nullptr) {
		pOutRep = (_pOutRep == nullptr) ? &cout : _pOutRep;
		searchIdx = 0;
		lvlTotal = 0;
		Tim.Start();
		CollRgInit();
		PeriodInit();
	}

	void PeriodInit() {
		lvlMin = 0xff;
		lvlMax = 0;
		period = searchIdxPeriod;
		posMin = DIMMAX * DIMMAX;
		posMax = 0;
		CollRgPeriodInit();
	}

	void Collect(int colLevel, int pos = -1) {
		period--;
		if (lvlMin > colLevel)
			lvlMin = colLevel;
		if (lvlMax < colLevel)
			lvlMax = colLevel;

		if (rgCut > 0) {
			CollRg(colLevel);
		}

		if (period == 0) {
			if (searchIdx % dumpFreq == 0)
				RangeDump();
			if (lvlTotal < lvlMax)
				lvlTotal = lvlMax;
			PeriodInit();
			searchIdx++;
		}
	}

	const int rgCutINIT = -1;
	int rgCut;
	int rgCnt;
	int lowerCnt, higherCnt;
	int higherCntMax;
	enum { LOWER, HIGHER } cutSt;

	void CollRgInit() {
		rgCut = rgCutINIT;
		cutSt = LOWER;
		CollRgPeriodInit();
	}

	void CollRgPeriodInit() {
		lowerCnt = 0;
		higherCnt = 0;
		higherCntMax = -1;
	}

	void CollRg(int colLevel) {
		if (cutSt == LOWER) {
			if (colLevel < rgCut) {
				rgCnt++;
			}
			else {
				lowerCnt++;
				cutSt = HIGHER;
				rgCnt = 1;
			}
		}
		else {
			if (colLevel >= rgCut) {
				rgCnt++;
			}
			else {
				higherCnt++;
				if (higherCntMax < rgCnt)
					higherCntMax = higherCnt;
				cutSt = LOWER;
				rgCnt = 1;
			}
		}
	}

	SimpleTimer Tim;
	long long ms;

	void RangeDump() {
		ms = Tim.LapsedMSecs();
		*pOutRep << "[" << dec << setw(9) << ms / 1000 << " s] ";
		//*pOutRep << "(POS" << setw(3) << posMin << ":" << posMax << ") ";
		*pOutRep << "ITR " << dec << setw(8) << searchIdx
			<< " MAX " << setw(2) << lvlTotal << " ";
		for (int i = 0; i < lvlMin; i++)
			*pOutRep << "  -";
		for (int i = lvlMin; i <= lvlMax; i++)
			*pOutRep << setw(3) << i;
		*pOutRep << endl;

		if (rgCut > 0)
			CollRgDump();
	}

	void CollRgDump() {
		*pOutRep << "         RG  LOWER " << dec << lowerCnt
			<< "  HIGHER " << higherCnt << "   HIGHERMAX " << higherCntMax
			<< endl;
	}

	long long totalSteps;
	void StepsCnt() {
		ms = Tim.LapsedMSecs();
		totalSteps = searchIdx * searchIdxPeriod + (searchIdxPeriod - period);
	}

	void StepsDump() {
		StepsCnt();
		*pOutRep << " Total Steps " << dec << totalSteps
			<< "   time " << ms << " msecs" << endl;
	}

};

template<class Lane> class Histo : public LogClient {
public:
	Histo() {}

	void Init(int _n, Lane** _Rows, Lane** _Cols,
		FastMatrix<FASTMATDIMLOG, u8>* _pMat)
	{
		n = _n;
		RowTab = _Rows;
		ColTab = _Cols;
		pMat = _pMat;
	}

	int n;
	Lane** RowTab;
	Lane** ColTab;
	FastMatrix<FASTMATDIMLOG, u8>* pMat;

	// Initial histo processing
	u8 VCnt[DIMMAX];
	u8 LastCol[DIMMAX];
	int cnt;
	void Start();
	void Add(Set s, int idx);
	int Scan(int j);
	int Fill();


};

class LatSqGen {
public:
	LatSqGen() : n(-1) {}

	int n;

	void Seed(int seed) {
		srand(seed);
	}

	int RandLim(int lim) {
		return rand() % lim;
	}
	void RandExch(int aIn, int bIn, int& a, int& b);

	// Arrays
	int xy[DIMMAX][DIMMAX];
	int xz[DIMMAX][DIMMAX];
	int yz[DIMMAX][DIMMAX];

	u8 blk[DIMMAX][DIMMAX];
	int blkRowCnt[DIMMAX];

	// Generate a Latin Square matrix in xy
	void Gen(int _n);

	// Blank blkCnt entries for each row and col of xy
	int laneBlkCnt;
	void BlankRowGen(int j);
	void Blank(int blkCnt);
	void Dump(ostream& out, const char *title, int m[DIMMAX][DIMMAX]);
};


#pragma once
#include "basic.h"
#include "assert.h"
#include <intrin.h>
#include <algorithm>
#include <string>

using namespace std;

typedef u64 Mask;

static const int DIMMAX = sizeof(u64) * 8;
static const int DIM = 35;
static const int BLANKCNTLOG = 4;
static const int BLANKCNTMAX = 1 << BLANKCNTLOG;
static const int BLANKCNTMASK = BLANKCNTMAX - 1;

class DebugState {
public:
	DebugState() : dumpExpand(0), dumpRange(INT_MAX),
		dumpCompare(0), dumpTerm(0), dumpMakeSingle(0), 
		dumpLevel(DIMMAX), noPotSort(true), lowPotFirst(true)
	{
		pOut = &cout;
	}

	typedef long long Counter;
	Counter dumpExpand;
	Counter dumpRange;
	Counter dumpCompare;
	Counter dumpTerm;
	Counter dumpMakeSingle;

	// Limits
	int dumpLevel;

	// Params
	bool noPotSort;
	bool lowPotFirst;

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
	Counter DmpRange() { return dumpRange > 0 ? dumpRange-- : 0; }
	Counter DmpCompare() { return dumpCompare > 0 ? dumpCompare-- : 0; }
	Counter DmpTerm() { return dumpTerm > 0 ? dumpTerm-- : 0; }
	Counter DmpMakeSingle() { return dumpMakeSingle > 0 ? dumpMakeSingle-- : 0; }
};

extern DebugState DbgSt;

class Set {
public:
	Set() : mask(0) {}
	Set(Mask v) : mask(v) {}

	Mask mask;

	Mask operator= (Mask m) { return mask = m; }
	bool operator== (Mask m) { return mask == m; }
	bool operator== (Set& s) { return mask == s.mask; }
	bool operator!= (Mask m) { return mask != m; }
	bool operator!= (Set& s) { return mask != s.mask; }
	bool operator>= (Set& s) { return (mask & s.mask) == s.mask; }
	bool operator<= (Set& s) { return (mask & s.mask) == mask; }

	void operator+= (Mask m) { mask |= m; }
	void operator-= (Mask m) { mask &= ~m; }
	void operator|= (Mask m) { mask |= m; }
	void operator&= (Mask m) { mask &= m; }

	void operator+= (Set s) { mask |= s.mask; }
	void operator-= (Set s) { mask &= ~s.mask; }
	void operator|= (Set s) { mask |= s.mask; }
	void operator&= (Set s) { mask &= s.mask; }
	void operator^= (Set s) { mask ^= s.mask; }

	Mask operator+ (Mask m) { return mask | m; }
	Mask operator| (Mask m) { return mask | m; }
	Mask operator& (Mask m) { return mask & m; }

	Mask operator+ (Set s) { return mask | s.mask; }
	Mask operator- (Set s) { return mask & (~s.mask); }
	Mask operator| (Set s) { return mask | s.mask; }
	Mask operator& (Set s) { return mask & s.mask; }

	Mask operator~ () { return ~mask; }

	Mask GetFirst() {
		return mask & (~mask + 1);
	}

	u8 GetFirstIdx() {
		Mask m = mask & (~mask + 1);
		return (u8)__popcnt64(m - 1);
	}

	Mask ExtractFirst() {
		Mask m = mask & (~mask + 1);
		mask &= ~m;
		return m;
	}

	u8 ExtractIdx() {
		Mask m = mask & (~mask + 1);
		mask &= ~m;
		return (u8)__popcnt64(m - 1);
	}

	int ExtractValues(u8* pV, int cntMax) {
		int pos;
		Set m = mask;
		u8 v;

		for (pos = 0; ; pos++) {
			v = (u8)m.ExtractIdx();
			pV[pos] = v;
			assert(pos < cntMax);
			if (m == 0)
				break;
		}

		return pos + 1;
	}

	bool CountIsOne() { return mask == GetFirst(); }
	int Count() { return (int)__popcnt64(mask); }
	bool Contains(int idx) { return Contains(BitMask(idx)); }
	bool Contains(Mask m) { return (mask & m) == m; }
	bool Contains(Set s) { return Contains(s.mask); }

	static const u8 DIMMAX = sizeof(Mask) * 8;
	static const Mask ONE = 1;
	static Mask BitMask(u8 v) { return ONE << v; }

	void operator+= (u8 v) { mask |= (ONE << v); }
	void operator-= (u8 v) { mask &= (~(ONE << v)); }

	static u8 GetIdx(Mask m) {
		m &= (~m + 1);
		return (u8)__popcnt64(m - 1);
	}


};

/////////////////////////////////////////////////
//  Aux static methods
//
class Set16 {
public:
	Set16() {}

	static inline u16 GetFirst(u16 mask) {
		return mask & (~mask + 1);
	}

	static  inline u16 GetFirstIdx(u16 mask) {
		u16 m = mask & (~mask + 1);
		return __popcnt16(m - 1);
	}

	static inline  u16 ExtractFirst(u16 &mask) {
		u16 m = mask & (~mask + 1);
		mask &= ~m;
		return m;
	}

	static  inline u16 ExtractIdx(u16 &mask) {
		u16 m = mask & (~mask + 1);
		mask &= ~m;
		return __popcnt16(m - 1);
	}


};

enum { FASTMATDIMLOG = 6 };

template < int DIMLOG, class T> class FastMatrix {
public:
	FastMatrix() {}
	static const int dim = 1 << DIMLOG;
	T Val[dim * dim];

	inline T& operator()(int i, int j) {
		return Val[(i << DIMLOG) + j];
	}
	inline T* operator[](int i) {
		return Val + (i << DIMLOG);
	}
};

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

	void OutHeader(ostream& out, int cnt, Ent* pHdr = 0) {
		out << "      ";
		for (int j = 0; j < cnt; j++) {
			out << " " << setw(2) << (pHdr == 0 ? j : (int)pHdr[j]);
		}
		out << endl;
	}

	void OutLineV(ostream& out, Ent* pLine, int i, int cnt) {
		out << "[" << setw(2) << i << "]  ";
		for (int j = 0; j < cnt; j++) {
			out << (((j & 3) != 0) ? " " : "|");
			if ((int)pLine[j] > cnt)
				out << " *";
			else
				out << setw(2) << (int)pLine[j];
		}
		out << endl;
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
	PermSetRef(bool _isRow, int _idx, int _depth)
		: isRow(_isRow), idx(_idx), depth(_depth), cnt(0) {
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
	void Name(char* dirName = NULL);
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
};

class NLane;

class PermSet {
public:
	PermSet() {}

	bool outOflo;

	typedef enum { SPLIT, EXP, LVL } Format_enum;
	Format_enum format;

	bool isRow;
	int idx;
	string searchId;
	void SetId(bool _isRow, int _idx) {
		isRow = _isRow;
		idx = _idx;
		searchId = isRow ? "ROW" : "COL";
		searchId += to_string(idx);
	}


	LongLink baseLnk;
	int permCnt;
	int nodeCnt;
	int singleCnt;
	int vCnt;
	u8 V[BLANKCNTMAX];
	void VDictSet(Set abs) {
		abs.ExtractValues(V, sizeof(V));
	}

	int depth;			// Lane depth == Lane missCnt
	int termHt;			// Number of levels for terminal encoding
	int depthTerm;		// Level at which terminal encoding starts

	///////////////////////////////////////////////////////
	//		SPLIT PSET
	///////////////////////////////////////////////////////

	enum { LNKNULL = 0xFFFF };
	enum { TEMPAREASZ = BLANKCNTMAX + 2	};
	ShortLink* TempPool;

	enum { POOLSZ = 1 << 16 };
	u16* Pool;
	int freeLnk;
	u16* TermPool;
	int freeTermLnk;

	bool Alloc(bool countOnly);
	u16* TempArea(int level);
	void Init(int _depth, int _termHt, bool countOnly);
	u16* NodeStart(int level);
	u16* NodeBranch(u16* pTemp, u16 cnt, u16 lnk);
	u16 NodeEnd(int level, u16* pEnd, u16 cnt, u16 vMask);
	bool VerifyCounts();
	void FollowBranch(int level, ShortLink lnk);
		
	///////////////////////////////////////////////////////
	//		EXPANDED PSET
	///////////////////////////////////////////////////////
	static const LongLink LONGLNKNULL = -1;
	enum {
		EXPPOOLSZ = 1 << 19,
		EXPTEMPAREASZ = BLANKCNTMAX * 4,
	};
	LongLink expBaseLnk;
	LongLink* ExpPool;
	LongLink* ExpTempPool;
	int freeExpLnk;

	void ExpAlloc();
	void ExpDealloc();
	LongLink* ExpTempArea(int level);
	LongLink* NodeExpStart(int level);
	LongLink* NodeExpBranch(LongLink* pTemp, ShortCnt cnt, LongLink lnk);
	LongLink* NodeExpBotBranch(LongLink* pTemp, LongLink mask);
	LongLink NodeExpEnd(int level, LongLink* pEnd, ShortCnt cnt, ShortCnt vMask);
	bool ExpVerifyCounts();
	void ExpFollowBranch(int level, LongLink lnk);

	u16 term;
	u16 unsel;
	u16 startLnk;
	u16 TermStart(u16 _unsel);
	void TermEncode(int level, u16 chosenBit);
	void TermOut();
	u16 TermEnd();

	///////////////////////////////////////////////////////
	//		LEVELED PSET
	///////////////////////////////////////////////////////
	enum {
		LVSZ1 = 6,
		LVSZ1MASK = (1 << LVSZ1) - 1,
		SINGLESZ4 = sizeof(LvlBranch) * 8 / LVSZ1,
		BLVLSWITCH = 3,
		BSMALLSZ1 = 8,
		BSMALLMAX = 1 << BSMALLSZ1,
		BSMALLMASK = BSMALLMAX - 1,
		BLARGESZ1 = 16,
		BLARGEMAX = 1 << BLARGESZ1,
		BLARGEMASK = BLARGEMAX - 1,
		BFLAGSPOS = LVSZ1 + BSMALLSZ1 + BLARGESZ1,
		BTERMBIT = 1 << BFLAGSPOS,
		BLASTBIT = 1 << (BFLAGSPOS + 1),
		LVLNULL = -1,
	};

	LvlBranch LvlBranchFill(int level, int cnt, int lvlDisp) {
		LvlBranch branch;
		if (level <= BLVLSWITCH) {
			assert(cnt < BLARGEMAX && lvlDisp < BSMALLMAX);
			branch = (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + BLARGESZ1));
		}
		else {
			assert(cnt < BSMALLMAX && lvlDisp < BLARGEMAX);
			branch = (cnt << LVSZ1) | (lvlDisp << (LVSZ1 + BSMALLSZ1));
		}
		return branch;
	}

	enum {
		CDLOG = 22,
		FLD1LOG = 6,
		FLD2LOG = 9,
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
		} else if (cnt < FLD2MAX && lvlDisp < FLD3MAX) {
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

	void LvlBranchFinish(LvlBranch & branch, int lV, bool isLast) {
		branch |= (lV & LVSZ1MASK);
		if (isLast)
			branch |= BLASTBIT;
	}

	inline bool LvlIsLast(LvlBranch branch) { return (branch & BLASTBIT) != 0; }
	inline bool LvlIsTerm(LvlBranch branch) { return (branch & BTERMBIT) != 0; }
	inline u8 LvlGetV(LvlBranch branch) { return branch & LVSZ1MASK; }
	inline u8 LvlExtractV(LvlBranch& branch) {
		u8 v = (u8)(branch & LVSZ1MASK);
		// Shift mask down, keeping flags
		branch = ((branch & (~(BLASTBIT | BTERMBIT))) >> LVSZ1)
			| (branch & (BLASTBIT | BTERMBIT));
		return v;
	}

	inline u8 LvlTermAdv(LvlBranch& branch) {
		// Discard previus v: shift mask down, keeping flags
		branch = ((branch & (~(BLASTBIT | BTERMBIT))) >> LVSZ1)
			| (branch & (BLASTBIT | BTERMBIT));
		u8 v = (u8)(branch & LVSZ1MASK);
		return v;
	}

	inline int LvlGetCntNonTiered(int level, LvlBranch branch) {
		return (branch >> LVSZ1) & ((level <= BLVLSWITCH) ? BLARGEMASK : BSMALLMASK);
	}

	static inline LvlBranch LvlFlag(LvlBranch branch) { return branch & FLAGMASK; }
	inline int LvlGetCnt(LvlBranch branch) {
		if (LvlIsTerm(branch))
			return 1;

		int cnt = (int) (branch >> LVSZ1);
		switch (LvlFlag(branch)) {
		case FLAG1:
			return cnt & FLD1MASK;
		case FLAG2:
			return cnt & FLD2MASK;
		case FLAG3:
			return cnt & FLD3MASK;
		default:
			return cnt & FLD4MASK;
		}
	}

	inline int LvlGetDisp(LvlBranch branch) {
		switch (LvlFlag(branch)) {
		case FLAG1:
			return (branch >> (LVSZ1 + FLD1LOG)) & FLD4MASK;
		case FLAG2:
			return (branch >> (LVSZ1 + FLD2LOG)) & FLD3MASK;
		case FLAG3:
			return (branch >> (LVSZ1 + FLD3LOG)) & FLD2MASK;
		default:
			return (branch >> (LVSZ1 + FLD4LOG)) & FLD1MASK;
		}
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
	void LvlFromExp(u16 permMask);
	LvlBranch LvlExpVisit(int level, LongLink lnk, u16 permMask);

	LvlBranch* LvlBase[BLANKCNTMAX];
	LvlBranch* BranchPtr(int level, ShortLink lnk) { return LvlBase[level] + lnk; }
	LvlBranch* BranchPtr(int level, LvlBranch branch) {
		return LvlBase[level] + LvlGetDisp(branch);
	}

	u8 VisitV[BLANKCNTMAX];
	Set VisitMiss[BLANKCNTMAX];
	int MissDel[BLANKCNTMAX];
	PermSetRef* pRef;
	bool LvlCheckRef(PermSetRef* pRef);
	bool LvlVisit(int level, LvlBranch* pNode);
	bool LvlVisitTerm(int level, LvlBranch br);
	int LvlSetMiss(NLane *pLane);

	bool VisitProcess();

	bool ExpCheckRef(PermSetRef* pRef);
	bool ExpVisit(int level, LongLink lnk);


	///////////////////////////////////////////////////////
	//		MISC
	///////////////////////////////////////////////////////

	u32 Pot[BLANKCNTMAX]; 
	LvlBranch Branches[BLANKCNTMAX];
	LvlBranch CrBranches[BLANKCNTMAX];

	int iCnt;
	int LvlIntersect(LvlBranch* pT, LvlBranch* pC, Set & ThSet);

	Sorter PSort;
	void LvlSortInter();

	inline int SortedIdx(int i) { return PotGetPos(Pot[i]); }
	LvlBranch LvlGetBranch(int lvl, ShortLink nodeLink, u8 v);
	LvlBranch LvlBranchAdv(int lvl, LvlBranch inBranch, u8 v);

	void CountTerm() { permCnt++; }
	bool OutOflo() { return freeLnk >= POOLSZ || freeTermLnk >= POOLSZ; }

	int LvlCnt[BLANKCNTMAX];
	int LvlMax[BLANKCNTMAX];
	int NodeCnt[BLANKCNTMAX];
	int SingleCnt[BLANKCNTMAX];
	void InitCounts();
	void InitTotals();

	bool badCounts;
	bool CheckCounts();

	void Dump(ostream& out, bool doNodes);
	void DumpLvl(ostream& out, int lvl);
	void DumpLvls(ostream& out, int lvlFr, int lvlTo = -1);
	void DumpBranch(ostream& out, int lvl, LvlBranch br);
	void DumpExpanded(ostream& out, int lvl, LvlBranch br);

	string nodeId;
	void DumpNodeId(ostream &out, int lvl, ShortLink nodeDisp = LVLNULL);
};
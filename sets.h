#pragma once
#include "basic.h"
#include "assert.h"
#include <intrin.h>

typedef u64 Mask;

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

	static inline u16 ExtractFirst(u16& mask) {
		u16 m = mask & (~mask + 1);
		mask &= ~m;
		return m;
	}

	static inline u16 ExtractIdx(u16& mask) {
		u16 m = mask & (~mask + 1);
		mask &= ~m;
		return __popcnt16(m - 1);
	}
};

class Set32 {
public:
	Set32() {}

	static inline u32 GetFirst(u32 mask) {
		return mask & (~mask + 1);
	}

	static inline u32 GetFirstIdx(u32 mask) {
		u32 m = mask & (~mask + 1);
		return __popcnt(m - 1);
	}

	static inline u32 ExtractFirst(u32& mask) {
		u32 m = mask & (~mask + 1);
		mask &= ~m;
		return m;
	}

	static inline u32 ExtractIdx(u32& mask) {
		u32 m = mask & (~mask + 1);
		mask &= ~m;
		return __popcnt(m - 1);
	}
};

typedef u32 BSetMask;
typedef Set32 BSetSet;

enum {
	BSETMASKSZ1LOG = 5,
	BSETMASKSZ1 = 1 << BSETMASKSZ1LOG,
	BSETMASKSZ1MASK = BSETMASKSZ1 - 1,
	BSETMASKFULL = 0xFFFFFFFF,
};

class BitSet {
public:
	BitSet(int _size) {
		size = _size;
		sizeMask = (size + BSETMASKSZ1 - 1) >> BSETMASKSZ1LOG;
		MPool = new BSetMask[sizeMask];
		Init(1);
	}

	int size;
	int sizeMask;
	BSetMask* MPool;

	void Init(int bit) {
		BSetMask bm = bit == 0 ? 0 : BSETMASKFULL;
		for (int p = 0; p < sizeMask; p++)
			MPool[p] = bm;
		posFirst = 0;
		posLast = size - 1;
	}

	int posFirst;
	int posLast;
	int posAt;

	inline void AdvanceFirst() {
		int p = posFirst >> BSETMASKSZ1LOG;
		for (; p < sizeMask; p++) {
			if (MPool[p] != 0) {
				posFirst = (p << BSETMASKSZ1LOG)
					+ BSetSet::GetFirstIdx(MPool[p]);
				return;
			}
		}
		posFirst = posLast + 1;
	}

	int GetFirst() {
		posAt = posFirst >> BSETMASKSZ1LOG;
		return BSetSet::GetFirstIdx(MPool[posAt]);
	}

	int ExtractFirst() {
		posAt = posFirst >> BSETMASKSZ1LOG;
		int pos = 
			BSetSet::ExtractIdx(MPool[posAt]) + (posAt << BSETMASKSZ1LOG);
		assert(pos == posFirst);
		AdvanceFirst();
		return pos;
	}

	void Set(int pos) {
		posAt = pos >> BSETMASKSZ1LOG;
		MPool[posAt] |= (1 << (pos & BSETMASKSZ1MASK));
		if (posFirst > pos)
			posFirst = pos;
	}

	int Test(int rg) {
		cerr << "TEST " << rg << endl;
		Init(0);
		int tsz = 0;
		int *SPos = new int[size];
		
		for (int pos = 0; pos < size; ) {
			Set(pos);
			SPos[tsz] = pos;
			tsz++;
			pos += (rand() % rg) + 1;
		}

		int errCnt = 0;
		for (int p = 0; p < tsz; p++) {
			int sp = ExtractFirst();
			if (sp != SPos[p]) {
				cerr << " BSet " << sp << " != " << SPos[p] << endl;
				errCnt++;
			}
		}
		if (errCnt > 0) {
			cerr << "   errCnt " << errCnt << endl;
		}
		else
			cerr << "  SUCCESS" << endl;
		return errCnt;
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


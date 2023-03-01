#include "stdinc.h"

int	const Seed = 12345;
int	const N = 16;

void LatSqGen::RandExch(int aIn, int bIn, int &a, int &b)
{
	if (RandLim(2) == 0) {
		a = aIn;
		b = bIn;
	}
	else {
		a = bIn;
		b = aIn;
	}
}

void LatSqGen::Gen(int _n)
{
	n = _n;
	int i, j, k;
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			k = (i + j) % n;
			xy[i][j] = k;
			xz[i][k] = j;
			yz[j][k] = i;
		}
	}

	int mxy, mxz, myz;
	int m[3];
	bool proper = true;
	int minIter = n * n * n;
	int iter;
	for (iter = 0; iter < minIter || !proper; iter++) {
		int i, j, k, i2, j2, k2;
		int i2_, j2_, k2_;

		if (proper) {
			// Pick a random 0 in the array
			i = RandLim(n);
			j = RandLim(n);
			k = RandLim(n);
			while (xy[i][j] == k) {
				i = RandLim(n);
				j = RandLim(n);
				k = RandLim(n);
			}

			// find i2 such that [i2, j, k] is 1. same for j2, k2
			i2 = yz[j][k];
			j2 = xz[i][k];
			k2 = xy[i][j];
			i2_ = i;
			j2_ = j;
			k2_ = k;
		}
		else {
			i = m[0];
			j = m[1];
			k = m[2];

			// find one such that [i2, j, k] is 1, same for j2, k2.
			// each is either the value stored in the corresponding
			// slice, or one of our three temporary "extra" 1s.
			// That's because (i, j, k) is -1.
			RandExch(yz[j][k], myz, i2, i2_);
			RandExch(xz[i][k], mxz, j2, j2_);
			RandExch(xy[i][j], mxy, k2, k2_);
		}

		proper = xy[i2][j2] == k2;
		if (! proper) {
			m[0] = i2;
			m[1] = j2;
			m[2] = k2;
			mxy = xy[i2][j2];
			myz = yz[j2][k2];
			mxz = xz[i2][k2];
		}

		xy[i][j] = k2_;
		xy[i][j2] = k2;
		xy[i2][j] = k2;
		xy[i2][j2] = k;

		yz[j][k] = i2_;
		yz[j][k2] = i2;
		yz[j2][k] = i2;
		yz[j2][k2] = i;

		xz[i][k] = j2_;
		xz[i][k2] = j2;
		xz[i2][k] = j2;
		xz[i2][k2] = j;
	}

	if (DbgSt.reportCreate)
		Dump(cout, "LATSQ", xy);
}

void LatSqGen::BlankRowGen(int j)
{
	int i;
	int chgCnt = 0;
	int colsToGo = n - j;

	// Check for trail end of rows
	for (i = 0; i < n; i++) {
		if (laneBlkCnt - blkRowCnt[i] >= colsToGo) {
			// Forced change
			assert(laneBlkCnt - blkRowCnt[i] == colsToGo);
			assert(chgCnt < laneBlkCnt);
			blk[i][j] = BLKENT;
			blkRowCnt[i]++;
			chgCnt++;
		}
	}

	// Randomly change until col quota
	for (; chgCnt < laneBlkCnt; ) {
		i = RandLim(n);
 		if (blkRowCnt[i] < laneBlkCnt && blk[i][j] != BLKENT) {
			blk[i][j] = BLKENT;
			blkRowCnt[i]++;
			chgCnt++;
		}
	}
}

void LatSqGen::Blank(int _laneBlkCnt)
{
	laneBlkCnt = _laneBlkCnt;

	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			blk[i][j] = (u8)xy[i][j];

	for (int i = 0; i < n; i++)
		blkRowCnt[i] = 0;

	for (int j = 0; j < n; j++) {
		BlankRowGen(j);
	}

	Dump(cout, "BLANKED", blk);
}

void LatSqGen::Dump(ostream& out, const char* title, int m[DIMMAX][DIMMAX])
{
	out << endl << title << endl;
	for (int i = 0; i < n; i++) {
		out << "[" << setw(2) << i << "] ";
		for (int j = 0; j < n; j++) {
			if (m[i][j] == BLKENT)
				out << "  *";
			else
				out << setw(3) << m[i][j];
		}
		out << endl;
	}
}

void LatSqGen::Dump(ostream& out, const char* title, u8 m[DIMMAX][DIMMAX])
{
	out << endl << title << endl;
	for (int i = 0; i < n; i++) {
		out << "[" << setw(2) << i << "] ";
		for (int j = 0; j < n; j++) {
			if (m[i][j] == BLKENT)
				out << "  *";
			else
				out << setw(3) << (int) (m[i][j]);
		}
		out << endl;
	}
}

void LatSqGen::SaveEncoded(ostream& out)
{
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (blk[i][j] == BLKENT)
				out << blnkChar;
			else
				out << Encode(blk[i][j] + 1);
		}
		out << endl;
	}
}

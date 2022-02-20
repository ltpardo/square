#include "stdinc.h"
#include "latsq.h"
#include "senv.h"


int Entry::OptCount(Lane<u64>& row, Lane<u64>& col)
{
	missMask = row.missing & col.missing;
	optCnt = (u8)__popcnt64(missMask);
	row.EstimateFactor(optCnt);
	col.EstimateFactor(optCnt);
	return optCnt;
}

void Lane<u64>::Present(u8 v) {
	if (v >= n)
		return;
	presCnt++;
	present |= BitMask(v);

	missCnt--;
	missing &= ~BitMask(v);
}

void Lane<u64>::Missing(u8 v) {
	if (v < n)
		return;
	missCnt++;
	missing &= ~BitMask(v);
}

bool Lane<u64>::Check() {
	if (presCnt + missCnt != n)
		return false;
	if ((present | missing) != all)
		return false;
	return true;
}

int FindFirstOne(u64 mask)
{
	int index;
	u32 mask32 = (u32)mask;
	index = (int) __lzcnt64(mask);
	if (mask)
		index = __lzcnt(mask32);
	else
		index = __lzcnt((u32) (mask >> 32)) + 32;

	return index;
}

void Lane<u64>::AddBlank(Entry* pEnt, int idx)
{
	BStack[blankCnt].Set(pEnt, idx);
	blankCnt++;
}

u64 Lane<u64>::PermuteCount()
{
	PermuteStart();

	while (PermuteStep())
		;
	return visitCnt;
}

bool Lane<u64>::VerifyPerm()
{
	bool success = true;
	for (int d = 0; d < missCnt; d++) {
		if ((BStack[d].chosenBit & BStack[d].pEnt->missMask) == 0)
			success = false;
	}
	return success;
}

bool LatSqEnv::Read(char* fname)
{
	IO.ReadOpen(fname);
	for (int i = 0; i < n; i++) {
		IO.ReadLine(Mat[i], 2, n);
	}

	return true;
}

bool LatSqEnv::Dump(ostream& out)
{
	for (int i = 0; i < n; i++) {
		IO.OutLine(out, Mat[i], i, n);
	}

	return true;
}

void LatSqEnv::ProcessMat()
{
	u8 v;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			v = Mat(i, j).val;
			if (v < n) {
				Rows[i].Present(v);
				Cols[j].Present(v);
			}
			else {
				Rows[i].AddBlank(&Mat(i, j), j);
				Cols[j].AddBlank(&Mat(i, j), i);
			}
		}
	}

	for (int l = 0; l < n; l++) {
		Rows[l].Check();
		Cols[l].Check();
	}

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			v = Mat(i, j).val;
			if (v >= n) {
				Mat(i, j).OptCount(Rows[i], Cols[j]);
			}
		}
	}

	// Copy Mat for future verification
	MatCpy = Mat;
}

bool LaneComp(LaneRank<Lane<u64>> a, LaneRank<Lane<u64>> b)
{
	return a.permCnt < b.permCnt;
}

void LatSqEnv::SortCols(bool doSort)
{
	for (int j = 0; j < n; j++) {
		LaneTab[j].permCnt = Cols[j].PermuteCount();
		LaneTab[j].pLane = &Cols[j];
		LaneTab[j].idx = j;
		cout << " Col " << j << " PermCnt  " << LaneTab[j].permCnt << endl;
	}

	if (doSort) {
		std::sort(LaneTab, LaneTab + n, LaneComp);

		cout << endl;
		for (int j = 0; j < n; j++) {
			cout << " Sorted " << setw(2) << LaneTab[j].idx
				<< " PermCnt  " << setw(5) << LaneTab[j].permCnt << endl;
		}
	}
	else {
		cout << endl << " NO SORT" << endl;
	}
}

void LatSqEnv::SolveInit()
{
}

bool LatSqEnv::PublishSolution()
{
	Lane<u64> pLane;
	bool success = true;
	for (int j = 0; j < n; j++) {
		if (! Cols[j].VerifyPerm())
			success = false;
	}
	solveCnt++;
	cout << " SOLUTION # " << solveCnt << endl;
	return success;
}

int LatSqEnv::RestrictRows(Lane<u64>* pCol)
{
	int zCnt = 0;
	Blank* pBlank = pCol->BStack;
	for (int d = 0; d < pCol->missCnt; d++, pBlank++) {
		Rows[pBlank->idx].missing &= (~pBlank->chosenBit);
		if (Rows[pBlank->idx].missing == 0)
			zCnt++;
	}
	return zCnt;
}

int LatSqEnv::UpdateEntries(Lane<u64>* pCol)
{
	int zCnt = 0;
	u64 allMask = 0;
	// Update blanks for next column
	Blank* pBlank = pCol->BStack;
	for (int d = 0; d < pCol->missCnt; d++, pBlank++) {
		pBlank->pEnt->missMask = 
			Rows[pBlank->idx].missing & pCol->missing;
		allMask |= Rows[pBlank->idx].missing;
		if (pBlank->pEnt->missMask == 0)
			zCnt++;

#if 0
		u64 mask = pBlank->pEnt->missMask; 
		mask &= (~mask + 1);
		if (~pBlank->pEnt->missMask == mask)
			zCnt++;
#endif
	}

	u64 needSet = allMask & pCol->missing;
	int needCnt;
	if (needSet != pCol->missing) {
		needSet ^= pCol->missing;
		needCnt = (int)__popcnt64(needSet);
		if (needCnt == 1)
			zCnt += n;
		else
			zCnt += needCnt * n;
	}
	return zCnt;
}

void LatSqEnv::RestoreRows(Lane<u64>* pCol)
{
	Blank* pBlank = pCol->BStack;
	for (int d = 0; d < pCol->missCnt; d++, pBlank++) {
		Rows[pBlank->idx].missing |= pBlank->chosenBit;
	}
}

int LatSqEnv::Solve()
{
	SolveInit();
	pColCur = LaneTab[0].pLane;
	pColCur->PermuteStart();
	for (int s = 0; s < n; s++) {
		FailCnt[s] = 0;
		ClogCnt[0] = 0;
	}

	for (colLevel = 0; colLevel >= 0; ) {
		if (pColCur->PermuteStep()) {
			if (colLevel >= n - 1) {
				PublishSolution();
				colLevel--;
				pColCur = LaneTab[colLevel].pLane;
				RestoreRows(pColCur);
			}
			else {
				RestrictRows(pColCur);
				colLevel++;
				pColCur = LaneTab[colLevel].pLane;
				if (UpdateEntries(pColCur) > 0) {
					ClogCnt[colLevel]++;
					colLevel--;
					pColCur = LaneTab[colLevel].pLane;
					RestoreRows(pColCur);
					continue;
				}
				pColCur->PermuteStart();
			}
		}
		else {
			// Level has been exhausted
			if (colLevel > 0) {
				FailCnt[colLevel]++;
				colLevel--;
				pColCur = LaneTab[colLevel].pLane;
				RestoreRows(pColCur);
			}
			else
				break;
		}
	}

	return solveCnt;
}


LatSqEnv LSq(DIM);

bool LatSqTest(char* fname)
{
	LSq.Read(fname);
	LSq.Dump(cout);

	LSq.ProcessMat();

	for (int j = 0; j < DIM; j++) {
		u64 cnt = LSq.Cols[j].PermuteCount();
		cout << " Col " << j << " PermCnt  " << cnt << endl;
	}

	LSq.SortCols(true);

	LSq.Solve();

	return true;
}


/////////////////////////////////////////////////////////////

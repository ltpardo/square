#include "stdinc.h"
#include "nlane.h"
#include "nenv.h"

void PermSet::Init(int _depth, int _termHt, bool countOnly)
{
	depth = _depth;
	termHt = _termHt;
	depthTerm = depth - termHt;

	Counts.InitLevels();
	Counts.InitTotals(_depth);

	pExpSet = 0;
}

///////////////////////////////////////////////////////
//		EXPANDED PSET
///////////////////////////////////////////////////////

void PermExp::Alloc()
{
	assert(Pool == 0);
	assert(TempPool == 0);
	Pool = new u32[POOLSZ];
	TempPool = new u32[TEMPAREASZ * BLANKCNTMAX];
	baseLnk = 0;
	freeLnk = 0;
}

void PermExp::Dealloc()
{
	if (Pool)
		delete Pool;
	if (TempPool)
		delete TempPool;
}

// Returns lnk to node and node cnt
//	Input: lane lMissing and stack[level].lMiss
//
LongLink PermExp::Gen(int level, LMask sel, ShortCnt& cnt)
{
	LongLink branch;

	LMask chosenBit;
	LMask unsel = lMissing & (~sel);
	LMask choice = pMiss[level] & unsel;
	LMask nextSel;

	if (choice == 0)
		// Exhausted branch
		return PermSet::LONGLNKNULL;

	if (level == missCnt - 1) {
		// At Bottom
		assert(choice == (choice & (~choice + 1)));
		// Returns mask
		cnt = 1;
		Counts.permCnt++;
		return choice;
	}

	// Internal branch: iterate through all descendants
	ShortLink nodeCnt = 0;
	LongLink* pTemp = NodeStart(level);
	ShortCnt vMask = 0;
	ShortCnt branchCnt;

	do {
		// Extract candidate from choiceSet
		chosenBit = choice & (~choice + 1);
		choice &= ~chosenBit;
		nextSel = sel | chosenBit;

		if ((branch = Gen(level + 1, nextSel, branchCnt)) == PermSet::LONGLNKNULL)
			// Branch is not viable
			continue;

		if (level >= missCnt - 2) {
			// Bottom branch: 
			pTemp = NodeBotBranch(pTemp, branch);
		}
		else {
			// Intermediate level - tree zone
			pTemp = NodeBranch(pTemp, branchCnt, branch);
		}
		nodeCnt += branchCnt;
		vMask |= chosenBit;
	} while (choice != 0);

	// Create tree node and return
	if (nodeCnt > 0) {
		cnt = nodeCnt;
		return NodeEnd(level, pTemp, nodeCnt, vMask);
	}
	else
		return PermSet::LONGLNKNULL;
}

bool PermExp::VerifyCounts()
{
	Counts.InitLevels();
	FollowBranch(0, baseLnk);
	return Counts.CheckCounts();
}

void PermExp::FollowBranch(int level, LongLink lnk)
{
	LongLink* pNode = Pool + lnk;
	u16 mask = (u16)(*pNode++);
	u16 nodeCnt = __popcnt16(mask);

	assert(nodeCnt <= depth);
	for (int l = 0; l < nodeCnt; l++) {
		int cnt = (level >= depth - 2) ? 1 : *(pNode++);
		Counts.LvlCnt[level] += cnt;
		if (Counts.LvlMax[level] < cnt)
			Counts.LvlMax[level] = cnt;
		if (level < depth - 2)
			FollowBranch(level + 1, *pNode);
		pNode++;
	}
}

LongLink* PermExp::TempArea(int level) {
	return TempPool + level * TEMPAREASZ;
}

LongLink* PermExp::NodeStart(int level) {
	return TempArea(level) + 1;
}

LongLink* PermExp::NodeBranch(LongLink* pTemp, ShortCnt cnt, LongLink lnk) {
	*(pTemp++) = cnt;
	*(pTemp++) = lnk;
	return pTemp;
}

LongLink* PermExp::NodeBotBranch(LongLink* pTemp, LongLink mask) {
	*(pTemp++) = mask;
	return pTemp;
}

LongLink PermExp::NodeEnd(int level, LongLink* pEnd, ShortCnt cnt, ShortCnt vMask) {
	LongLink* pTemp = TempArea(level);
	Counts.nodeCnt++;
	Counts.NodeCnt[level]++;
	if (cnt == 1) {
		Counts.SingleCnt[level]++;
		Counts.singleCnt++;
	}

	ShortCnt sz = (ShortCnt)(pEnd - pTemp);
	assert(sz < TEMPAREASZ);

	LongLink* pPool;
	// Store node in ExpPool area
	pPool = Pool + freeLnk;
	baseLnk = freeLnk;
	freeLnk += sz;
	if (freeLnk >= POOLSZ) {
		Counts.outOflo = true;
		return LONGLNKNULL;
	}
	*(pTemp) = vMask;
	if (Pool != NULL) {
		while (pTemp < pEnd) {
			*(pPool++) = *(pTemp++);
		}
	}
	return baseLnk;
}


bool PermExp::CheckRef(PermSetRef* _pRef)
{
	pChkRef = _pRef;
	pChkRef->ReadOpen();
	Visit(0, baseLnk);
	pChkRef->ReadClose();
	return pChkRef->errCnt <= 0;
}

bool PermExp::Visit(int level, LongLink lnk)
{
	LongLink* pNode = Pool + lnk;
	u16 mask = (u16)(*pNode++);
	u16 branchCnt = __popcnt16(mask);
	int lV;

	for (int l = 0; mask != 0; l++) {
		lV = V[Set16::ExtractIdx(mask)];
		VisitV[level] = lV;
		int cnt = (level >= depth - 2) ? 1 : *(pNode++);

		if (level < depth - 2) {
			Visit(level + 1, *pNode);
		}
		else if (level == depth - 2) {
			lV = V[Set16::GetFirstIdx((u16)(*pNode))];
			VisitV[level + 1] = lV;
			VisitProcess();
		}
		pNode++;
	}

	return false;
}

///////////////////////////////////////////////////////
//		LEVELED PSET
///////////////////////////////////////////////////////

void PermSet::LvlAlloc()
{
	LvlStore = new LevelStore<LvlBranch, 12, BLANKCNTMAX, 64>(depth);
}

void PermSet::LvlDealloc()
{
	delete LvlBase[0];
}

LvlBranch PermSet::LvlMakeSingle(int level, LongLink nodeLnk, u16 permMask)
{
	LongLink* pNode;
	int shift = LVSZ1;
	LvlBranch lV;
	u16 nodeMask;
	LvlBranch branch = 0;
	int lvl;

#ifdef DBGREL
	if (DbgSt.DmpMakeSingle()) {
		*(DbgSt.pOut) << " MakeSingle " << permCnt << " lvl " << level << "  V: ";
	}
#endif

	if (level == depth - 1) {
		// Bottom: insert only value left
		lV = Set16::ExtractIdx(permMask);
		lV = V[lV];
		branch |= (lV << shift);
		Counts.LvlCnt[level]++;
	}
	else {
		for (lvl = level; lvl <= depth - 2; lvl++) {
			pNode = pExpSet->Pool + nodeLnk;
			nodeMask = (u16)(*pNode++);
			permMask &= (~nodeMask);
			lV = V[Set16::ExtractIdx(nodeMask)];
			assert(nodeMask == 0);
#ifdef DBGREL
			if (DbgSt.DmpMakeSingle())
				*(DbgSt.pOut) << "  @" << hex << nodeLnk
					<< ":" << hex << setw(3) << (int)lV;
#endif				
			branch |= (lV << shift);
			shift += LVSZ1;

			Counts.LvlCnt[lvl]++;
			if (lvl < depth - 2) {
				assert(*pNode == 1);
				pNode++;
				nodeLnk = *pNode;
			}
			else {
				// lvl == depth - 2: bottom lV is stored in the node branch
				nodeMask = (u16)(*pNode);
				permMask &= (~nodeMask);
				lV = Set16::ExtractIdx(nodeMask);
				lV = V[lV];

#ifdef DBGREL
				if (DbgSt.DmpMakeSingle())
					*(DbgSt.pOut) << "  @bot:" << hex << setw(3) << (int)lV;
#endif
				assert(nodeMask == 0);
				assert(permMask == 0);
				branch |= (lV << shift);
				Counts.LvlCnt[lvl + 1]++;
				break;
			}
		}
	}
	assert(shift <= sizeof(LvlBranch) * 8 - LVSZ1);
	Counts.permCnt++;

#ifdef DBGREL
	if (DbgSt.DmpMakeSingle())
		*(DbgSt.pOut) << hex << setw(10) << (branch | BTERMBIT) << endl;
#endif
	return branch | BTERMBIT;
}

void PermSet::LvlFromExp(PermExp *_pExpSet, u16 permMask)
{
	pExpSet = _pExpSet;
	minLvlForSingles = depth - SINGLESZ4;
	LvlAlloc();
	Counts.InitLevels();
	Counts.InitTotals(depth);

	LvlBranch base = LvlExpVisit(0, pExpSet->baseLnk, permMask);

	// Resulting tree starts at LvlBase[0]
	LvlStore->CopyLinear(LvlBase);
	if (!Counts.CheckCounts())
		cerr << "LVL FROM EXP: Bad check counts" << endl;
	lvlSz32 = LvlStore->totalSz;
	delete LvlStore;
}

LvlBranch PermSet::LvlExpVisit(int level, LongLink nodeLnk, u16 permMask)
{
	LongLink* pNode = pExpSet->Pool + nodeLnk;
	u16 mask = (u16)(*pNode++);
	u16 curMask;
	u16 branchCnt = __popcnt16(mask);
	LvlBranch branch;
	LvlBranch LNode[BLANKCNTMAX];
	int lV;
	int childrenCnt = 0;

	for (int l = 0; mask != 0; l++) {
		lV = Set16::ExtractIdx(mask);
		curMask = permMask & (~(1 << lV));
		int cnt = (level >= depth - 2) ? 1 : *(pNode++);
		childrenCnt += cnt;
		if (cnt == 1)
			Counts.singleCnt++;
		if (cnt == 1 && level >= minLvlForSingles) {
			// Single
			branch = LvlMakeSingle(level + 1, *pNode, curMask);
			Counts.SingleCnt[level]++;

		}
		else if (level < depth - 2) {
			branch = LvlExpVisit(level + 1, *pNode, curMask);
		}
		else if (level == depth - 2) {
			// Here we only have singles (cnt == 1)
			assert(0);
		}
		if (Counts.LvlMax[level] < cnt)
			Counts.LvlMax[level] = cnt;
		LvlBranchFinish(branch, V[lV], mask == 0);
		LNode[l] = branch;
		pNode++;
	}

	Counts.LvlCnt[level] += childrenCnt;
	Counts.NodeCnt[level]++;
	Counts.nodeCnt++;
	int lvlDisp = LvlStore->Store(LNode, level, branchCnt);
	branch = LvlBranchTiered(childrenCnt, lvlDisp);
	return branch;
}

LvlBranch PermSet::LvlGetBranch(int lvl, ShortLink nodeLink, u8 v)
{
	for (LvlBranch* pBr = BranchPtr(lvl, nodeLink); ; pBr ++) {
		if (LvlGetV(*pBr) == v)
			return *pBr;
		if (LvlIsLast(*pBr))
			break;
	}
	return LVLNULL;
}


LvlBranch PermSet::LvlBranchAdv(int lvl, LvlBranch inBranch, u8 v)
{
	if (LvlIsTerm(inBranch)) {
		if (LvlTermAdv(inBranch) == v)
			return inBranch;
		else
			return LVLNULL;
	}
	else {
		ShortLink nodeLnk = LvlGetDisp(inBranch);
		return LvlGetBranch(lvl, nodeLnk, v);
	}
}

int PermSet::LvlIntersect(LvlBranch* pT, LvlBranch* pC, Set& ThSet)
{
	iCnt = 0;
	//ThSet = 0;
	LvlBranch brT = *pT++;
	LvlBranch brC = *pC++;
	u8 vT = LvlGetV(brT);
	//ThSet += vT;
	u8 vC = LvlGetV(brC);
	for (;;) {
		if (vT <= vC) {
			if (vT == vC) {
				// Match
				if (! DbgSt.noPotSort)
					Pot[iCnt] = Potential(iCnt, brT, brC);
				Branches[iCnt] = brT;
				CrBranches[iCnt++] = brC;

				if (LvlIsLast(brC)) {
					// Complete ThSet
					while (!LvlIsLast(brT)) {
						brT = *pT++;
						vT = LvlGetV(brT);
						//ThSet += vT;
					}
					break;
				}
				if (LvlIsLast(brT))
					break; 
				brT = *pT++;
				vT = LvlGetV(brT);
				//ThSet += vT;
				brC = *pC++;
				vC = LvlGetV(brC);
			}
			else {
				if (LvlIsLast(brT))
					break;
				brT = *pT++;
				vT = LvlGetV(brT);
				//ThSet += vT;
			}
		}
		else {
			if (LvlIsLast(brC)) {
				// Complete ThSet
				while (!LvlIsLast(brT)) {
					brT = *pT++;
					vT = LvlGetV(brT);
					//ThSet += vT;
				}
				break;
			}
			brC = *pC++;
			vC = LvlGetV(brC);
		}
	}
	return iCnt;
}

void PermSet::LvlSortInter()
{
	PSort.SetPs(Pot);
	PSort.Sort(iCnt);
}

#ifdef DBGREL
			if (DbgSt.DmpCompare()) {
				*(DbgSt.pOut) << " Diff term " << pRef->readCnt;
				*(DbgSt.pOut) << " ref ";
				for (int d = 0; d < depth; d++)
					*(DbgSt.pOut) << hex << setw(3) << (int)(pRef->V[d]);

				*(DbgSt.pOut) << " lvl ";
				for (int d = 0; d < depth; d++)
					*(DbgSt.pOut) << hex << setw(3) << (int)VisitV[d];
				*(DbgSt.pOut) << endl;
			}
#endif

bool PermBase::VisitProcess()
{
	if (pChkRef != NULL) {
		if (!pChkRef->Read())
			return false;
		if (!pChkRef->Compare(VisitV)) {
			// Coukd use dump code above
			return false;
		}
		else
			return true;
	}
	else {
		for (int d = 0; d < depth; d++) {
			VisitMiss[d] += VisitV[d];
		}
		return true;
	}
}

bool PermSet::LvlVisitTerm(int level, LvlBranch br)
{
	for (int d = level; d < depth; d++) {
		VisitV[d] = LvlExtractV(br);
	}
	return VisitProcess();
}

#ifdef DBGREL
			if (DbgSt.DmpTerm()) {
				*(DbgSt.pOut) << " Term " << pRef->readCnt << " lvl " << level << "  br ";
				DumpBranch(*(DbgSt.pOut), level, br);
				*(DbgSt.pOut) << endl;
			}
#endif

bool PermSet::LvlVisit(int level, LvlBranch* pNode)
{
	LvlBranch br;
	do {
		br = *pNode++;
		if (LvlIsTerm(br)) {
			// Dump code?
			LvlVisitTerm(level, br);
		}
		else {
			VisitV[level] = LvlGetV(br);
			LvlVisit(level + 1, BranchPtr(level + 1, br));
		}
	} while (!LvlIsLast(br));

	return true;;
}

bool PermSet::LvlCheckRef(PermSetRef* _pRef)
{
	pChkRef = _pRef;
	pChkRef->ReadOpen();
	LvlVisit(0, BranchPtr(0, (ShortLink)0));
	pChkRef->ReadClose();
	return pChkRef->errCnt <= 0;
}

bool PermSet::ExpCheckRef(PermSetRef* _pRef)
{
	pChkRef = _pRef;
	pChkRef->ReadOpen();
	ExpVisit(0, baseLnk);
	pChkRef->ReadClose();
	return pChkRef->errCnt <= 0;
}

bool PermSet::ExpVisit(int level, LongLink lnk)
{
	LongLink* pNode = pExpSet->Pool + lnk;
	u16 mask = (u16)(*pNode++);
	u16 branchCnt = __popcnt16(mask);
	int lV;

	for (int l = 0; mask != 0; l++) {
		lV = V[Set16::ExtractIdx(mask)];
		VisitV[level] = lV;
		int cnt = (level >= depth - 2) ? 1 : *(pNode++);
		
		if (level < depth - 2) {
			ExpVisit(level + 1, *pNode);
		}
		else if (level == depth - 2) {
			lV = V[Set16::GetFirstIdx((u16) (*pNode))];
			VisitV[level + 1] = lV;
			VisitProcess();
		}
		pNode++;
	}

	return false;
}

void  PermCounts::InitLevels()
{
	for (int d = 0; d < depth; d++) {
		LvlCnt[d] = 0;
		LvlMax[d] = 0;
		NodeCnt[d] = 0;
		SingleCnt[d] = 0;
	}
}

void  PermCounts::InitTotals(int _depth)
{
	depth = _depth;
	depthTerm = depth;
	permCnt = 0;
	nodeCnt = 0;
	singleCnt = 0;
	outOflo = false;
}

bool  PermCounts::CheckCounts()
{
	for (int d = 0; d < depthTerm; d++) {
		if (LvlCnt[d] != permCnt) {
			badCounts = true;
			return false;
		}
	}
	badCounts = false;
	return true;
}

void PermSet::Dump(ostream& out, bool doNodes) {
	out << " cnt: " << setw(6) << Counts.permCnt
		<< " nodes: " << setw(6) << Counts.nodeCnt
		<< " singles: " << setw(6) << Counts.singleCnt;

	out << "  LEVEL sz32:" << setw(6) << lvlSz32;
	out << endl;

	if (doNodes) {
		out << "  NODES";
		for (int d = 0; d < depth - 1; d++)
			out << "," << setw(5) << Counts.NodeCnt[d];
		out << endl;
		out << "  SINGS";
		for (int d = 0; d < depth - 1; d++)
			out << "," << setw(5) << Counts.SingleCnt[d];
		out << endl;
		out << "  LVMAX";
		for (int d = 0; d < depth - 1; d++)
			out << "," << setw(5) << Counts.LvlMax[d];
		out << endl;
		out << "  MSCNT";
		for (int d = 0; d < depth - 1; d++)
			out << "," << setw(5) << VisitMiss[d].Count();
		out << endl;
	}
}

void PermSet::DumpLvls(ostream& out, int lvlFr, int lvlTo)
{
	if (lvlTo < 0)
		lvlTo = depth - 1;

	for (int lvl = lvlFr; lvl <= lvlTo; lvl++)
		DumpLvl(out, lvl);
}

void PermSet::DumpBranch(ostream& out, int lvl, LvlBranch br)
{
	if (LvlIsTerm(br)) {
		out << "{" << hex << (int) LvlExtractV(br) << ":";
		for (int l = 0; l < depth - lvl - 1; l++) {
			if (l > 0)
				out << ",";
			out << hex << (br & LVSZ1MASK);
			br >>= LVSZ1;
		}
		out << "} ";
	}
	else {
		out << "<" << hex << (int) LvlGetV(br) << ":";
		out << hex << LvlGetCnt(br) << ",";
		DumpNodeId(out, lvl + 1, LvlGetDisp(br));
		out << nodeId << "> ";
	}
}

void PermSet::DumpNodeId(ostream& out, int lvl, ShortLink nodeDisp)
{
	out << (isRow ? "R" : "C") << hex << setw(1) << lvl;
	if (nodeDisp != (ShortLink) LVLNULL)
		out << "_" << nodeDisp;
	else
		out << " ";
}

void PermSet::DumpLvl(ostream& out, int lvl)
{
	out << (isRow ? "ROW" : "COL") << setw(2) << hex << idx << "/" << lvl << " ";

	ShortLink disp = 0;
	LvlBranch br;
	LvlBranch *pBr = LvlBase[lvl];
	string lead = "   ";
	out << endl << lead;
	for (int node = 0; node < Counts.NodeCnt[lvl]; node ++) {
		DumpNodeId(out, lvl, disp);
		out << " ";
		do {
			br = *pBr++;
			disp++;
			DumpBranch(out, lvl, br);
		} while (!LvlIsLast(br));
		if ((node & 1) == 1)
			out << endl << lead;
		else if (node < Counts.NodeCnt[lvl] - 1)
			out << " | ";
	}
	out << endl;
}

void PermSet::DumpExpanded(ostream& out, int lvl, LvlBranch br)
{
	if (LvlIsTerm(br)) {
		out << "TERM" << hex << lvl << " ";
		LvlTermAdv(br);
		DumpBranch(out, lvl, br);
	}
	else {
		ShortLink nodeDisp = LvlGetDisp(br);
		LvlBranch* pBr = LvlBase[lvl] + nodeDisp;
		DumpNodeId(out, lvl, nodeDisp);
		out << " ";
		do {
			br = *pBr++;
			DumpBranch(out, lvl, br);
		} while (!LvlIsLast(br));
	}
	out << " ";
}


void PermSetRef::Name()
{
	if (EPars.refDir != "") {
		fname = EPars.refDir;
		fname += "\\";
	}
	fname += isRow ? "Row_" : "Col_";
	char idxStr[10];
	sprintf_s(idxStr, "%2.2d", idx);
	fname += idxStr;
	fname += ".txt";
}

void PermSetRef::Create()
{
	Name();
	out.open(fname);
}

int PermSetRef::Close()
{
	out.close();
	return cnt;
}

void PermSetRef::Out()
{
	out << dec << setw(5) << cnt << ":";
	for (int d = 0; d < depth; d++) {
		out << hex << setw(3) << (int) V[d];
	}
	out << endl;
	cnt++;
}

bool PermSetRef::Read()
{
	int i;
	char c;
	in >> dec >> i >> c;
	if (i != readCnt || c != ':')
		return false;
	for (int d = 0; d < depth; d++) {
		in >> hex >> i;
		V[d] = (char)i;
	}
	readCnt++;
	return true;
}

bool PermSetRef::ReadOpen()
{
	in.open(fname);
	if (!in.is_open())
		return false;
	readCnt = 0;
	errCnt = 0;
	return true;
}

void PermSetRef::ReadClose()
{
	assert(in.is_open());
	in.close();
}

bool PermSetRef::Gen(int level, Mask sel, ShortCnt& cnt)
{
	Mask chosenBit;
	Mask unsel = totalMiss & (~sel);
	Mask choice = pMissTab[level].mask & unsel;

	if (choice == 0)
		// Exhausted branch
		return false;

	Mask nextSel;
	ShortCnt brCnt;
	bool notEmpty = false;
	do {
		// Extract level value from choiceSet
		chosenBit = choice & (~choice + 1);
		V[level] = Set::GetIdx(chosenBit);
		choice &= ~chosenBit;
		if (level == depth - 1) {
			// At Bottom
			assert(choice == 0);
			Out();
			return true;
		}

		nextSel = sel | chosenBit;
		if (!Gen(level + 1, nextSel, brCnt)) {
			// Branch is not viable
			continue;
		}
		cnt += brCnt;
		notEmpty = true;
	} while (choice != 0);

	return notEmpty;
}

void Sorter::Sort(SortIdx _n)
{
	n = _n;
	if (n == 2) {
		if (pS[0] < pS[1]) {
			Buf[0] = pS[0];
			Buf[1] = pS[1];
		}
		else {
			Buf[0] = pS[1];
			Buf[1] = pS[0];
		}
		return;
	}

	SetBuf();
	if (n & 1)
		Buf[n - 1] = pS[n - 1];

	if (n == 3) {
		Merge2x1(0);
		return;
	}

	Merge2x2s();
	if(n == 4)
		return;

	switch (n) {
	case 5:
		Merge(0, 4, 1);
		return;
	case 6:
		Merge(0, 4, 2);
		return;
	case 7:
		Merge2x1(4);
		Merge(0, 4, 3);
		return;
	case 8:
		Merge(0, 4, 4);
		return;
	case 9:
		Merge(4, 4, 1);
		Merge(0, 4, 5);
		return;
	case 10:
		Merge(4, 4, 2);
		Merge(0, 4, 6);
		return;
	case 11:
		Merge2x1(8);
		Merge(4, 4, 3);
		Merge(0, 4, 7);
		return;
	case 12:
		Merge(4, 4, 4);
		Merge(0, 4, 8);
		return;
	default:
		assert(0);
	}
}

void Sorter::Merge2x1(SortIdx iFr)
{
	if (Buf[iFr] > Buf[iFr + 2]) {
		Buf[iFr + 2] = Buf[iFr + 1];
		Buf[iFr + 1] = Buf[iFr];
		Buf[iFr] = pS[iFr + 2];
	}
	else if (Buf[iFr + 1] > Buf[iFr + 2]) {
		Buf[iFr + 2] = Buf[iFr + 1];
		Buf[iFr + 1] = pS[iFr + 2];
	}

}

void Sorter::SetBuf()
{
	SortIdx lim = n - 1;
	for (SortIdx p = 0; p < lim; p += 2) {
		if (pS[p] < pS[p + 1]) {
			Buf[p] = pS[p];
			Buf[p + 1 ] = pS[p + 1];
		}
		else {
			Buf[p] = pS[p + 1];
			Buf[p + 1] = pS[p];
		}
	}
}

void Sorter::Merge2x2s()
{
	SortIdx lim = n - 3;
	for (SortIdx p = 0; p < lim; p += 4) {
		// Set first
		if (Buf[p] > Buf[p + 2])
			Swap(Buf[p], Buf[p + 2]);
		// Set last
		if (Buf[p + 1] > Buf[p + 3])
			Swap(Buf[p + 1], Buf[p + 3]);
		// Set middle pair
		if (Buf[p + 1] > Buf[p + 2])
			Swap(Buf[p + 1], Buf[p + 2]);
	}
}

void Sorter::Merge(SortIdx iFr, SortIdx aSz, SortIdx bSz)
{
	SortIdx k;
	for (k = 0; k < bSz; k++)
		Aux[k] = Buf[iFr + aSz + k];

	SortIdx aIdx = iFr + aSz - 1;
	SortIdx bIdx = bSz - 1;
	SortIdx dIdx = aIdx + bSz;
	for (;;) {
		if (Aux[bIdx] >= Buf[aIdx]) {
			Buf[dIdx--] = Aux[bIdx--];
			if (bIdx < 0)
				break;
		}
		else {
			Buf[dIdx--] = Buf[aIdx--];
			if (aIdx < iFr)
				break;
		}
	}
	while (bIdx >= 0) {
		Buf[dIdx--] = Aux[bIdx--];
	}
}


bool Comp(SortT a, SortT b)
{
	return a < b;
}

bool Sorter::Test(int testCnt, int size)
{
	SortT TBuf[SORTMAX];
	if (size > SORTMAX)
		size = SORTMAX;

	SetPs(TBuf);

	for (int t = 0; t < testCnt; t++) {
		for (int i = 0; i < size; i++) {
			TBuf[i] = rand();
		}

		Sort(size);

		std::sort(TBuf, TBuf + size, Comp);

		for (int i = 0; i < size; i++) {
			if (TBuf[i] != Buf[i])
				return false;;
		}
	}

	return true;
}



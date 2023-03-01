#define maxn 61
#define qmod ((1<<14) -1) 
#define encode(x) ((x) <10?'0'+(x) :(x) <36?'a'+(x) -10:(x) <62?'A'+(x) -36:'*') 
#define decode(c) ((c) >='0'&&(c) <='9'?(c) -'0':(c) >='a'&&(c) <='z'?(c) -'a'+10: \
(c) >='A'&&(c) <='Z'?(c) -'A'+36:-1) 
#define bufsize 80
#define O "%" \

#define showsols (argc> 1) 
#define shownodes (argc> 2) 
#define showcauses (argc> 3) 
#define showlong (argc> 4) 
#define showmoves (argc> 5) 
#define showprunes (argc> 6) 
#define showdomains (argc> 7) 
#define showsubproblems (argc> 8) 
#define showmatches (argc> 9) 
#define showT (argc> 10) 
#define showHK (argc> 11)  \

#define o mems++
#define oo mems+= 2
#define ooo mems+= 3
#define oooo mems+= 4 \

#define maxvars (3*maxn*maxn)  \

#define len itm \

#define msize -1
#define mparent -2
#define mstamp -3
#define mprev -4
#define mextra 4
#define mchsize 1000000 \

#define pruned 1 \

#define sanity_checking 0 \

#define tallyfiletemplate "plgad"O"d.tally" \

/*1:*/
////#line 51 ".\\partial-latin-gad.w"

#include <stdio.h> 
#include <stdlib.h> 
char buf[bufsize];
int board[maxn][maxn];
int P[maxn][maxn], R[maxn][maxn], C[maxn][maxn];
/*11:*/
////#line 358 ".\\partial-latin-gad.w"

typedef struct {
	int up, down;
	int itm;
	char aux[4];
}tetrad;

/*:11*//*12:*/
////#line 407 ".\\partial-latin-gad.w"

typedef struct {
	unsigned long long tally;
	int bmate;
	int gmate;
	int pos;
	int matching;
	int mark;
	int arcs;
	int rank;
	int link;
	int parent;
	int min;
	char name[4];
	int filler;
}variable;

/*:12*//*27:*/
//#line 659 ".\\partial-latin-gad.w"

typedef struct {
	int mchptrstart;
	int trailstart;
	int branchvar;
	int curchoice;
	int choices;
	int choiceno;
	unsigned long long nodeid;
}node;

/*:27*//*31:*/
//#line 730 ".\\partial-latin-gad.w"

typedef struct {
	int tip;
	int next;
}Arc;

/*:31*/
//#line 57 ".\\partial-latin-gad.w"
;
/*3:*/
//#line 104 ".\\partial-latin-gad.w"

unsigned long long mems;
unsigned long long thresh = 10000000000;
unsigned long long delta = 10000000000;
unsigned long long GADstart;
unsigned long long GADone;
unsigned long long GADtot;
unsigned long long GADtries;
unsigned long long GADaborts;
unsigned long long nodes;
unsigned long long count;
int originaln;

/*:3*//*13:*/
//#line 426 ".\\partial-latin-gad.w"

tetrad* tet;
int vars[maxvars];
int active;
variable var[maxvars + 1];

/*:13*//*20:*/
//#line 544 ".\\partial-latin-gad.w"

int totvars;
int mch[mchsize];
int mchptr = mextra;
int maxmchptr;

/*:20*//*28:*/
//#line 670 ".\\partial-latin-gad.w"

int trail[maxvars];
int trailptr;
int forced[maxvars];
int forcedptr;
int tofilter[qmod + 1];
int tofilterhead, tofiltertail;
node move[maxvars];
int level;
int maxl;
int mina;

/*:28*//*39:*/
//#line 850 ".\\partial-latin-gad.w"

int queue[maxn];
int marked[maxn];
int dlink;
Arc arc[maxn + maxn];
int lboy[maxn];

/*:39*//*74:*/
//#line 1437 ".\\partial-latin-gad.w"

FILE* tallyfile;
char tallyfilename[32];

/*:74*/
//#line 58 ".\\partial-latin-gad.w"
;
/*5:*/
//#line 131 ".\\partial-latin-gad.w"

void confusion(char* flaw, int why) {
	fprintf(stderr, "confusion: "O"s("O"d)!\n", flaw, why);
}

/*:5*//*19:*/
//#line 516 ".\\partial-latin-gad.w"

void print_options(int v) {
	register q;
	fprintf(stderr, "options for "O"s ("O"sactive, length "O"d):\n",
		var[v].name, var[v].pos < active ? "" : "in", tet[v].len);
	for (q = tet[v].down; q != v; q = tet[q].down)
		fprintf(stderr, " "O"s", tet[q & -4].aux);
	fprintf(stderr, "\n");
}

/*:19*//*25:*/
//#line 601 ".\\partial-latin-gad.w"

void print_match_prob(int m) {
	register int k;
	fprintf(stderr, "Matching problem "O"d (parent "O"d, size "O"d):\n",
		m, mch[m + mparent], mch[m + msize]);
	fprintf(stderr, "girls");
	for (k = 0; k < mch[m + msize]; k++)
		fprintf(stderr, " "O"s", var[mch[m + k]].name);
	fprintf(stderr, "\n");
	fprintf(stderr, "boys");
	for (; k < 2 * mch[m + msize]; k++)
		fprintf(stderr, " "O"s", var[mch[m + k]].name);
	fprintf(stderr, "\n");
}

/*:25*//*26:*/
//#line 641 ".\\partial-latin-gad.w"

void print_forced(void) {
	register int k;
	for (k = 0; k < forcedptr; k++)
		fprintf(stderr, " "O"s", tet[forced[k]].aux);
	fprintf(stderr, "\n");
}

void print_tofilter(void) {
	register int k;
	for (k = tofilterhead; k != tofiltertail; k = (k + 1) & qmod)
		fprintf(stderr, " "O"d("O"d)", tofilter[k], mch[tofilter[k] + msize]);
	fprintf(stderr, "\n");
}

/*:26*//*29:*/
//#line 682 ".\\partial-latin-gad.w"

void print_trail(void) {
	register int k, l;
	for (k = l = 0; k < trailptr; k++) {
		if (k == move[l].trailstart) {
			fprintf(stderr, "--- level "O"d\n", l);
			l++;
		}
		fprintf(stderr, " "O"s"O"s\n", tet[trail[k] & -4].aux, (trail[k] & 0x3) ? "*" : "");
	}
}

/*:29*//*30:*/
//#line 700 ".\\partial-latin-gad.w"

void sanity(void) {
	register int k, v, p, l, q;
	for (k = 0; k < totvars; k++) {
		v = vars[k];
		if (var[v].pos != k)
			fprintf(stderr, "wrong pos field in variable "O"d("O"s)!\n",
				v, var[v].name);
		if (k < active) {
			if (var[v].matching > move[level].mchptrstart)
				fprintf(stderr, " "O"s("O"d) has matching > "O"d!\n",
					var[v].name, v, move[level].mchptrstart);
			for (l = tet[v].len, p = tet[v].down, q = 0; q < l; q++, p = tet[p].down) {
				if (tet[tet[p].up].down != p)
					fprintf(stderr, "up-down off at "O"d!\n", p);
				if (tet[tet[p].down].up != p)
					fprintf(stderr, "down-up off at "O"d!\n", p);
				if (p == v) {
					fprintf(stderr, "list "O"d("O"s) too short!\n", v, var[v].name);
					break;
				}
			}
			if (p != v)fprintf(stderr, "list "O"d("O"s) too long!\n", v, var[v].name);
		}
	}
}

/*:30*//*73:*/
//#line 1422 ".\\partial-latin-gad.w"

void save_tallies(int z) {
	register int v;
	sprintf(tallyfilename, tallyfiletemplate, z);
	tallyfile = fopen(tallyfilename, "w");
	if (!tallyfile) {
		fprintf(stderr, "I can't open file `"O"s' for writing!\n", tallyfilename);
	}
	else {
		for (v = 1; v <= z; v++)
			fprintf(tallyfile, ""O"20lld "O"s\n", var[v].tally, var[v].name);
		fclose(tallyfile);
		fprintf(stderr, "Tallies saved in file `"O"s'.\n", tallyfilename);
	}
}

/*:73*//*76:*/
//#line 1464 ".\\partial-latin-gad.w"

void print_progress(void) {
	register int l, k, d, c, p;
	register double f, fd;
	fprintf(stderr, " after "O"lld mems: "O"lld sols,", mems, count);
	for (f = 0.0, fd = 1.0, l = 1; l < level; l++) {
		k = move[l].choiceno, d = move[l].choices;
		fd *= d, f += (k - 1) / fd;
		fprintf(stderr, " "O"c"O"c", encode(k), encode(d));
	}
	fprintf(stderr, " "O".5f\n", f + 0.5 / fd);
}

/*:76*//*77:*/
//#line 1479 ".\\partial-latin-gad.w"
\
void print_state(void) {
	register int l, v;
	fprintf(stderr, "Current state (level "O"d):\n", level);
	for (l = 1; l <= level; l++) {
		switch (move[l].curchoice & 0x3) {
		case 1:case 2:v = tet[move[l].curchoice + 1].itm; break;
		case 3:v = tet[move[l].curchoice - 2].itm; break;
		}
		fprintf(stderr, " "O"s="O"s ("O"d of "O"d), node "O"lld\n",
			var[move[l].branchvar].name, var[v].name,
			move[l].choiceno, move[l].choices, move[l].nodeid);
	}
	fprintf(stderr,
		" "O"lld solution"O"s, "O"lld mems, maxl "O"d, mina "O"d so far.\n",
		count, count == 1 ? "" : "s", mems, maxl, mina);
}

/*:77*/
////#line 59 ".\\partial-latin-gad.w"
//;
pls_main(int argc) {
	/*4:*/
//#line 124 ".\\partial-latin-gad.w"

	register int a, i, j, k, l, m, p, q, r, s, t, u, v, x, y, z;

	/*:4*//*32:*/
//#line 746 ".\\partial-latin-gad.w"

	register int b, g, boy, girl, n, nn, del, delp;

	/*:32*//*35:*/
//#line 803 ".\\partial-latin-gad.w"

	register int f, qq, marks, fin_level;

	/*:35*//*46:*/
//#line 954 ".\\partial-latin-gad.w"

	register int stack, pboy, newn;

	/*:46*//*63:*/
//#line 1242 ".\\partial-latin-gad.w"

	int bvar, opt, pij, rik, cjk, vv, zerofound, maxtally;

	/*:63*/
//#line 61 ".\\partial-latin-gad.w"
	;
	/*6:*/
//#line 139 ".\\partial-latin-gad.w"

	FILE* fin;
	const char* finName = "gadin.txt";
	fin = fopen(finName, "r");
	if(fin == NULL)
		fprintf(stderr, "File %s not found\n", finName);

	for (z = m = n = y = 0;; m++) {
		if (!fgets(buf, bufsize, fin))break;
		if (m == maxn) {
			fprintf(stderr, "Too many lines of input!\n");
			exit(-1);
		}
		for (p = 0;; p++) {
			if (buf[p] == '.') {
				z++;
				continue;
			}
			x = decode(buf[p]);
			if (x < 1)break;
			if (x > y)y = x;
			if (p == maxn) {
				fprintf(stderr, "Line way too long: %s",
					buf);
				exit(-2);
			}
			if (R[m][x - 1]) {
				fprintf(stderr, "Duplicate `%c' in row %d!\n",
					encode(x), m + 1);
				exit(-3);
			}
			if (C[p][x - 1]) {
				fprintf(stderr, "Duplicate `%c' in column %d!\n",
					encode(x), p + 1);
				exit(-4);
			}
			board[m][p] = P[m][p] = x, R[m][x - 1] = p + 1, C[p][x - 1] = m + 1;
		}
		if (n == 0)n = p;
		if (n > p) {
			fprintf(stderr, "Line has fewer than %d characters: %s",
				n, buf);
			exit(-5);
		}
		if (n < p) {
			fprintf(stderr, "Line has more than %d characters: %s",
				n, buf);
			exit(-6);
		}
	}
	if (m < n) {
		fprintf(stderr, "Fewer than %d lines!\n",
			n);
		exit(-7);
	}
	if (m > n) {
		fprintf(stderr, "more than %d lines!\n",
			n);
		exit(-8);
	}
	if (y > n) {
		fprintf(stderr, "the entry `%c' exceeds %d!\n",
			encode(y), n);
		exit(-9);
	}
	fprintf(stderr,
		"OK, I've read a %dx%d partial latin square with %d missing entries.\n",
		n, n, z);
	originaln = n;

	/*:6*/
//#line 62 ".\\partial-latin-gad.w"
	;
	/*15:*/
//#line 447 ".\\partial-latin-gad.w"

	active = mina = totvars = 3 * z;
	for (p = i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				if (ooo, (!P[i][j] && !R[i][k] && !C[j][k]))
					p++;
	q = (totvars & -4) + 4 * (p + 1);
	tet = (tetrad*)malloc(q * sizeof(tetrad));
	if (!tet) {
		fprintf(stderr, "Couldn't allocate the tetrad table!\n");
		exit(-66);
	}
	for (k = 0; k < totvars; k++)
		oo, vars[k] = k + 1, var[k + 1].pos = k;
	for (k = 1; k <= totvars; k++)
		o, tet[k].up = tet[k].down = k;
	/*16:*/
//#line 463 ".\\partial-latin-gad.w"

	for (p = i = 0; i < n; i++)for (j = 0; j < n; j++) {
		if (P[i][j])P[i][j] = 0; else
			P[i][j] = ++p, sprintf(var[p].name, "p"O"c"O"c", encode(i + 1), encode(j + 1));
	}
	for (i = 0; i < n; i++)for (k = 0; k < n; k++) {
		if (R[i][k])R[i][k] = 0; else
			R[i][k] = ++p, sprintf(var[p].name, "r"O"c"O"c", encode(i + 1), encode(k + 1));
	}
	for (j = 0; j < n; j++)for (k = 0; k < n; k++) {
		if (C[j][k])C[j][k] = 0; else
			C[j][k] = ++p, sprintf(var[p].name, "c"O"c"O"c", encode(j + 1), encode(k + 1));
	}

	/*:16*/
//#line 459 ".\\partial-latin-gad.w"
	;
	/*17:*/
//#line 481 ".\\partial-latin-gad.w"

	for (q = totvars & -4, i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				if (ooo, (P[i][j] && R[i][k] && C[j][k])) {
					q += 4;
					sprintf(tet[q].aux, ""O"c"O"c"O"c", encode(i + 1), encode(j + 1), encode(k + 1));
					sprintf(tet[q + 1].aux, "p"O"c"O"c", encode(i + 1), encode(j + 1));
					sprintf(tet[q + 2].aux, "r"O"c"O"c", encode(i + 1), encode(k + 1));
					sprintf(tet[q + 3].aux, "c"O"c"O"c", encode(j + 1), encode(k + 1));
					p = P[i][j];
					oo, tet[q + 1].itm = p, r = tet[p].up;
					ooo, tet[p].up = tet[r].down = q + 1, tet[q + 1].up = r;
					p = R[i][k];
					oo, tet[q + 2].itm = p, r = tet[p].up;
					ooo, tet[p].up = tet[r].down = q + 2, tet[q + 2].up = r;
					p = C[j][k];
					oo, tet[q + 3].itm = p, r = tet[p].up;
					ooo, tet[p].up = tet[r].down = q + 3, tet[q + 3].up = r;
				}
	for (p = 1; p <= totvars; p++)oo, q = tet[p].up, tet[q].down = p;

	/*:17*/
//#line 460 ".\\partial-latin-gad.w"
	;
	/*18:*/
//#line 508 ".\\partial-latin-gad.w"

	for (p = 1; p <= totvars; p++) {
		for (o, q = tet[p].down, k = 0; q != p; o, q = tet[q].down)
			k++;
		o, tet[p].len = k;
	}

	/*:18*/
//#line 461 ".\\partial-latin-gad.w"
	;

	/*:15*//*21:*/
//#line 550 ".\\partial-latin-gad.w"

	if (mchsize < 2 * totvars + 4 * n * mextra) {
		fprintf(stderr, "Match table initial overflow (mchsize="O"d)!\n", mchsize);
		exit(-667);
	}
	/*22:*/
//#line 559 ".\\partial-latin-gad.w"

	for (i = 0; i < n; i++) {
		for (p = j = 0; j < n; j++)
			if (o, P[i][j])
				oo, mch[mchptr + p++] = P[i][j], var[P[i][j]].matching = mchptr;
		if (p) {
			mch[mchptr + msize] = p;
			for (k = 0; k < n; k++)if (o, R[i][k])o, mch[mchptr + p++] = R[i][k];
			if (p != 2 * mch[mchptr + msize])confusion("Ri girls != boys", p);
			if (showsubproblems)print_match_prob(mchptr);
			q = mchptr, mchptr += p + mextra, mch[mchptr + mprev] = q;
			tofilter[tofiltertail++] = q, mch[q + mstamp] = 1;
		}
	}

	/*:22*/
//#line 555 ".\\partial-latin-gad.w"
	;
	/*23:*/
//#line 573 ".\\partial-latin-gad.w"

	for (j = 0; j < n; j++) {
		for (p = k = 0; k < n; k++)if (o, C[j][k])
			oo, mch[mchptr + p++] = C[j][k], var[C[j][k]].matching = mchptr;
		if (p) {
			mch[mchptr + msize] = p;
			for (i = 0; i < n; i++)if (o, P[i][j])o, mch[mchptr + p++] = P[i][j];
			if (p != 2 * mch[mchptr + msize])confusion("Cj girls != boys", p);
			if (showsubproblems)print_match_prob(mchptr);
			q = mchptr, mchptr += p + mextra, mch[mchptr + mprev] = q;
			tofilter[tofiltertail++] = q, mch[q + mstamp] = 1;
		}
	}

	/*:23*/
//#line 556 ".\\partial-latin-gad.w"
	;
	/*24:*/
//#line 587 ".\\partial-latin-gad.w"

	for (k = 0; k < n; k++) {
		for (p = i = 0; i < n; i++)if (o, R[i][k])
			oo, mch[mchptr + p++] = R[i][k], var[R[i][k]].matching = mchptr;
		if (p) {
			mch[mchptr + msize] = p;
			for (j = 0; j < n; j++)if (o, C[j][k])o, mch[mchptr + p++] = C[j][k];
			if (p != 2 * mch[mchptr + msize])confusion("Vk girls != boys", p);
			if (showsubproblems)print_match_prob(mchptr);
			q = mchptr, mchptr += p + mextra, mch[mchptr + mprev] = q;
			tofilter[tofiltertail++] = q, mch[q + mstamp] = 1;
		}
	}

	/*:24*/
//#line 557 ".\\partial-latin-gad.w"
	;

	/*:21*//*75:*/
//#line 1443 ".\\partial-latin-gad.w"

	sprintf(tallyfilename, tallyfiletemplate, totvars);
	tallyfile = fopen(tallyfilename, "r");
	if (tallyfile) {
		for (v = 1; v <= totvars; v++) {
			if (!fgets(buf, bufsize, tallyfile))break;
			if (var[v].name[0] != buf[21] ||
				var[v].name[1] != buf[22] ||
				var[v].name[2] != buf[23])break;
			sscanf(buf, ""O"20lld", &var[v].tally);
		}
		if (v <= totvars)for (v--; v >= 1; v--)var[v].tally = 0;
		else fprintf(stderr, "(tallies initialized from file `"O"s')\n", tallyfilename);
	}

	/*:75*/
//#line 63 ".\\partial-latin-gad.w"
	;
	/*72:*/
//#line 1403 ".\\partial-latin-gad.w"

/*65:*/
//#line 1272 ".\\partial-latin-gad.w"

	o, move[0].mchptrstart = mchptr;
	for (v = 1; v <= totvars; v++)if (o, tet[v].len <= 1) {
		if (!tet[v].len) {
			if (showcauses)
				fprintf(stderr, " "O"s already has no options!\n", var[v].name);
			goto abort;
		}
		o, t = tet[v].down & -4;
		if (o, !tet[t].itm)oo, tet[t].itm = 1, forced[forcedptr++] = t;
	}

	/*:65*/
//#line 1404 ".\\partial-latin-gad.w"
	;
	goto mainplayer;
	/*67:*/
//#line 1312 ".\\partial-latin-gad.w"

	choose:level++;
	if (level > maxl)
		maxl = level;
	/*66:*/
//#line 1295 ".\\partial-latin-gad.w"

	if (showdomains)
		fprintf(stderr, "Branching at level "O"d:", level);
	for (t = totvars, k = 0; k < active; k++) {
		o, v = vars[k];
		if (showdomains)
			fprintf(stderr, " "O"s("O"d)", var[v].name, tet[v].len);
		if (o, tet[v].len <= t) {
			if (tet[v].len <= 1)
				confusion("missed force", v);
			if (tet[v].len < t)
				oo, bvar = v, t = tet[v].len, maxtally = var[v].tally;
			else if (o, var[v].tally > maxtally)
				o, bvar = v, t = tet[v].len, maxtally = var[v].tally;
		}
	}
	if (showdomains)
		fprintf(stderr, "\n");

	/*:66*/
//#line 1315 ".\\partial-latin-gad.w"
	;
	o, move[level].mchptrstart = mchptr, move[level].trailstart = trailptr;
	o, move[level].branchvar = bvar, move[level].choices = t;
	o, move[level].curchoice = tet[bvar].down, move[level].choiceno = 1;
enternode:move[level].nodeid = ++nodes;
	if (sanity_checking)
		sanity();
	if (shownodes) {
		v = move[level].branchvar;
		u = tet[move[level].curchoice + (var[v].name[0] == 'c' ? -2 : +1)].itm;
		fprintf(stderr,
			"L"O"d: "O"s="O"s ("O"d of "O"d), node "O"lld, "O"lld mems\n",
			level, var[v].name, var[u].name,
			move[level].choiceno, move[level].choices,
			move[level].nodeid, mems);
	}
	if (mems >= thresh) {
		thresh += delta;
		if (showlong)
			print_state();
		else
			print_progress();
	}
	o, opt = move[level].curchoice & -4;
	/*60:*/
//#line 1162 ".\\partial-latin-gad.w"

	{
		if (showmoves)
			fprintf(stderr, " forcing "O"s\n", tet[opt].aux);
		if (o, tet[opt].up) {
			if (showcauses)
				fprintf(stderr, " option "O"s was deleted\n", tet[opt].aux);
			goto abort;
		}
		zerofound = 0;
		o, trail[trailptr++] = opt;
		ooo, pij = tet[opt + 1].itm, rik = tet[opt + 2].itm, cjk = tet[opt + 3].itm;
		o, m = var[pij].matching;
		if (!mch[m + mstamp])
			oo, mch[m + mstamp] = 1, tofilter[tofiltertail] = m,
			tofiltertail = (tofiltertail + 1) & qmod;
		o, m = var[rik].matching;
		if (!mch[m + mstamp])
			oo, mch[m + mstamp] = 1, tofilter[tofiltertail] = m,
			tofiltertail = (tofiltertail + 1) & qmod;
		o, m = var[cjk].matching;
		if (!mch[m + mstamp])
			oo, mch[m + mstamp] = 1, tofilter[tofiltertail] = m,
			tofiltertail = (tofiltertail + 1) & qmod;
		/*62:*/
//#line 1222 ".\\partial-latin-gad.w"

		o, var[pij].bmate = rik, var[pij].gmate = cjk;
		o, p = var[pij].pos;
		if (p >= active)
			confusion("inactive pij", pij);
		o, v = vars[--active];
		oo, vars[active] = pij, var[pij].pos = active;
		o, vars[p] = v, var[v].pos = p;
		o, var[rik].bmate = cjk, var[rik].gmate = pij;
		o, p = var[rik].pos;
		if (p >= active)
			confusion("inactive rik", rik);
		o, v = vars[--active];
		o, vars[active] = rik, var[rik].pos = active;
		o, vars[p] = v, var[v].pos = p;
		o, var[cjk].bmate = pij, var[cjk].gmate = rik;
		o, p = var[cjk].pos;
		if (p >= active)
			confusion("inactive cjk", cjk);
		o, v = vars[--active];
		o, vars[active] = cjk, var[cjk].pos = active;
		o, vars[p] = v, var[v].pos = p;

		/*:62*/
//#line 1181 ".\\partial-latin-gad.w"
		;
		for (o, a = tet[pij].down; a != pij; o, a = tet[a].down)
			if (a != opt + 1) {
				t = a + 1;/*56:*/
	//#line 1111 ".\\partial-latin-gad.w"

				oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
				oo, tet[p].down = q, tet[q].up = p;
				oo, l = tet[r].len - 1, tet[r].len = l;
				o, s = var[r].matching;
				if (o, !mch[s + mstamp])
					oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
				if (l <= 1) {
					if (l == 0)
						oo, var[r].tally++, zerofound = r;
					else {
						o, p = tet[r].down & -4;
						if (o, !tet[p].itm)
							oo, tet[p].itm = 1, forced[forcedptr++] = p;
					}
				}

				/*:56*/
	//#line 1183 ".\\partial-latin-gad.w"
				;
				t = a + 2;/*56:*/
	//#line 1111 ".\\partial-latin-gad.w"

				oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
				oo, tet[p].down = q, tet[q].up = p;
				oo, l = tet[r].len - 1, tet[r].len = l;
				o, s = var[r].matching;
				if (o, !mch[s + mstamp])
					oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
				if (l <= 1) {
					if (l == 0)
						oo, var[r].tally++, zerofound = r;
					else {
						o, p = tet[r].down & -4;
						if (o, !tet[p].itm)
							oo, tet[p].itm = 1, forced[forcedptr++] = p;
					}
				}

				/*:56*/
	//#line 1184 ".\\partial-latin-gad.w"
				;
		}
		for (o, a = tet[rik].down; a != rik; o, a = tet[a].down)if (a != opt + 2) {
			t = a + 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

			oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
			oo, tet[p].down = q, tet[q].up = p;
			oo, l = tet[r].len - 1, tet[r].len = l;
			o, s = var[r].matching;
			if (o, !mch[s + mstamp])
				oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
			if (l <= 1) {
				if (l == 0)oo, var[r].tally++, zerofound = r;
				else {
					o, p = tet[r].down & -4;
					if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
				}
			}

			/*:56*/
//#line 1187 ".\\partial-latin-gad.w"
			;
			t = a - 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

			oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
			oo, tet[p].down = q, tet[q].up = p;
			oo, l = tet[r].len - 1, tet[r].len = l;
			o, s = var[r].matching;
			if (o, !mch[s + mstamp])
				oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
			if (l <= 1) {
				if (l == 0)oo, var[r].tally++, zerofound = r;
				else {
					o, p = tet[r].down & -4;
					if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
				}
			}

			/*:56*/
//#line 1188 ".\\partial-latin-gad.w"
			;
		}
		for (o, a = tet[cjk].down; a != cjk; o, a = tet[a].down)if (a != opt + 3) {
			t = a - 2;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

			oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
			oo, tet[p].down = q, tet[q].up = p;
			oo, l = tet[r].len - 1, tet[r].len = l;
			o, s = var[r].matching;
			if (o, !mch[s + mstamp])
				oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
			if (l <= 1) {
				if (l == 0)oo, var[r].tally++, zerofound = r;
				else {
					o, p = tet[r].down & -4;
					if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
				}
			}

			/*:56*/
//#line 1191 ".\\partial-latin-gad.w"
			;
			t = a - 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

			oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
			oo, tet[p].down = q, tet[q].up = p;
			oo, l = tet[r].len - 1, tet[r].len = l;
			o, s = var[r].matching;
			if (o, !mch[s + mstamp])
				oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
			if (l <= 1) {
				if (l == 0)oo, var[r].tally++, zerofound = r;
				else {
					o, p = tet[r].down & -4;
					if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
				}
			}

			/*:56*/
//#line 1192 ".\\partial-latin-gad.w"
			;
		}
		if (zerofound) {
			if (showcauses)
				fprintf(stderr, " no options for "O"s\n", var[zerofound].name);
			goto abort;
		}
	}

	/*:60*/
//#line 1336 ".\\partial-latin-gad.w"
	;
mainplayer:
	/*68:*/
//#line 1363 ".\\partial-latin-gad.w"

	while (1) {
		while (forcedptr) {
			o, opt = forced[--forcedptr];
			o, tet[opt].itm = 0;
			/*60:*/
//#line 1162 ".\\partial-latin-gad.w"

			{
				if (showmoves)fprintf(stderr, " forcing "O"s\n", tet[opt].aux);
				if (o, tet[opt].up) {
					if (showcauses)fprintf(stderr, " option "O"s was deleted\n", tet[opt].aux);
					goto abort;
				}
				zerofound = 0;
				o, trail[trailptr++] = opt;
				ooo, pij = tet[opt + 1].itm, rik = tet[opt + 2].itm, cjk = tet[opt + 3].itm;
				o, m = var[pij].matching; if (!mch[m + mstamp])
					oo, mch[m + mstamp] = 1, tofilter[tofiltertail] = m,
					tofiltertail = (tofiltertail + 1) & qmod;
				o, m = var[rik].matching; if (!mch[m + mstamp])
					oo, mch[m + mstamp] = 1, tofilter[tofiltertail] = m,
					tofiltertail = (tofiltertail + 1) & qmod;
				o, m = var[cjk].matching; if (!mch[m + mstamp])
					oo, mch[m + mstamp] = 1, tofilter[tofiltertail] = m,
					tofiltertail = (tofiltertail + 1) & qmod;
				/*62:*/
//#line 1222 ".\\partial-latin-gad.w"

				o, var[pij].bmate = rik, var[pij].gmate = cjk;
				o, p = var[pij].pos;
				if (p >= active)confusion("inactive pij", pij);
				o, v = vars[--active];
				oo, vars[active] = pij, var[pij].pos = active;
				o, vars[p] = v, var[v].pos = p;
				o, var[rik].bmate = cjk, var[rik].gmate = pij;
				o, p = var[rik].pos;
				if (p >= active)confusion("inactive rik", rik);
				o, v = vars[--active];
				o, vars[active] = rik, var[rik].pos = active;
				o, vars[p] = v, var[v].pos = p;
				o, var[cjk].bmate = pij, var[cjk].gmate = rik;
				o, p = var[cjk].pos;
				if (p >= active)confusion("inactive cjk", cjk);
				o, v = vars[--active];
				o, vars[active] = cjk, var[cjk].pos = active;
				o, vars[p] = v, var[v].pos = p;

				/*:62*/
//#line 1181 ".\\partial-latin-gad.w"
				;
				for (o, a = tet[pij].down; a != pij; o, a = tet[a].down)if (a != opt + 1) {
					t = a + 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = q, tet[q].up = p;
					oo, l = tet[r].len - 1, tet[r].len = l;
					o, s = var[r].matching;
					if (o, !mch[s + mstamp])
						oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
					if (l <= 1) {
						if (l == 0)oo, var[r].tally++, zerofound = r;
						else {
							o, p = tet[r].down & -4;
							if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
						}
					}

					/*:56*/
//#line 1183 ".\\partial-latin-gad.w"
					;
					t = a + 2;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = q, tet[q].up = p;
					oo, l = tet[r].len - 1, tet[r].len = l;
					o, s = var[r].matching;
					if (o, !mch[s + mstamp])
						oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
					if (l <= 1) {
						if (l == 0)oo, var[r].tally++, zerofound = r;
						else {
							o, p = tet[r].down & -4;
							if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
						}
					}

					/*:56*/
//#line 1184 ".\\partial-latin-gad.w"
					;
				}
				for (o, a = tet[rik].down; a != rik; o, a = tet[a].down)if (a != opt + 2) {
					t = a + 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = q, tet[q].up = p;
					oo, l = tet[r].len - 1, tet[r].len = l;
					o, s = var[r].matching;
					if (o, !mch[s + mstamp])
						oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
					if (l <= 1) {
						if (l == 0)oo, var[r].tally++, zerofound = r;
						else {
							o, p = tet[r].down & -4;
							if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
						}
					}

					/*:56*/
//#line 1187 ".\\partial-latin-gad.w"
					;
					t = a - 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = q, tet[q].up = p;
					oo, l = tet[r].len - 1, tet[r].len = l;
					o, s = var[r].matching;
					if (o, !mch[s + mstamp])
						oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
					if (l <= 1) {
						if (l == 0)oo, var[r].tally++, zerofound = r;
						else {
							o, p = tet[r].down & -4;
							if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
						}
					}

					/*:56*/
//#line 1188 ".\\partial-latin-gad.w"
					;
				}
				for (o, a = tet[cjk].down; a != cjk; o, a = tet[a].down)if (a != opt + 3) {
					t = a - 2;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = q, tet[q].up = p;
					oo, l = tet[r].len - 1, tet[r].len = l;
					o, s = var[r].matching;
					if (o, !mch[s + mstamp])
						oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
					if (l <= 1) {
						if (l == 0)oo, var[r].tally++, zerofound = r;
						else {
							o, p = tet[r].down & -4;
							if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
						}
					}

					/*:56*/
//#line 1191 ".\\partial-latin-gad.w"
					;
					t = a - 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = q, tet[q].up = p;
					oo, l = tet[r].len - 1, tet[r].len = l;
					o, s = var[r].matching;
					if (o, !mch[s + mstamp])
						oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
					if (l <= 1) {
						if (l == 0)oo, var[r].tally++, zerofound = r;
						else {
							o, p = tet[r].down & -4;
							if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
						}
					}

					/*:56*/
//#line 1192 ".\\partial-latin-gad.w"
					;
				}
				if (zerofound) {
					if (showcauses)
						fprintf(stderr, " no options for "O"s\n", var[zerofound].name);
					goto abort;
				}
			}

			/*:60*/
//#line 1368 ".\\partial-latin-gad.w"
			;
		}
		if (tofilterhead == tofiltertail)break;
		o, m = tofilter[tofilterhead], tofilterhead = (tofilterhead + 1) & qmod;
		o, mch[m + mstamp] = 0;
		/*33:*/
//#line 752 ".\\partial-latin-gad.w"

		if (showmatches)fprintf(stderr, "GAD filtering for problem "O"d\n", m);
		GADstart = mems, GADtries++;
		o, mch[m + mstamp] = 0;
		o, n = mch[m + msize], nn = n + n;
		switch (oo, var[mch[m]].name[0]) {
		case'p':del = +1, delp = +2; break;
		case'c':del = -2, delp = -1; break;
		case'r':del = +1, delp = -1; break;
		}
		/*34:*/
//#line 777 ".\\partial-latin-gad.w"

		for (b = n; b < nn; b++) {
			o, boy = mch[m + b];
			if (o, var[boy].pos < active)
				oo, var[boy].gmate = var[boy].mark = 0;
			else o, var[boy].mark = -2;
		}
		for (f = g = 0; g < n; g++) {
			o, girl = mch[m + g];
			if (o, var[girl].pos >= active)continue;
			for (o, a = tet[girl].down; a != girl; o, a = tet[a].down) {
				o, boy = tet[a + del].itm;
				if (o, !var[boy].gmate)break;
			}
			if (a != girl)oo, var[girl].bmate = boy, var[boy].gmate = girl;
			else ooo, var[girl].bmate = 0, var[girl].parent = f, queue[f++] = girl;

		}
		if (f)/*36:*/
//#line 806 ".\\partial-latin-gad.w"

			if (showHK)/*37:*/
//#line 822 ".\\partial-latin-gad.w"

			{
				for (p = n; p < nn; p++) {
					girl = var[mch[m + p]].gmate;
					fprintf(stderr, " "O"s", girl ? var[girl].name : "???");
				}
				fprintf(stderr, "\n");
			}

		/*:37*/
//#line 807 ".\\partial-latin-gad.w"
		;
		for (r = 1; f; r++) {
			if (showHK)fprintf(stderr, "Beginning round "O"d...\n", r);
			/*38:*/
//#line 831 ".\\partial-latin-gad.w"

			fin_level = -1, k = 0;
			for (marks = l = i = 0, q = f;; l++) {
				for (qq = q; i < qq; i++) {
					o, girl = queue[i];
					if (var[girl].pos >= active)confusion("inactive girl in SAP", girl);
					for (o, a = tet[girl].down; a != girl; o, a = tet[a].down) {
						oo, boy = tet[a + del].itm, p = var[boy].mark;
						if (p == 0)/*40:*/
//#line 857 ".\\partial-latin-gad.w"

						{
							if (fin_level >= 0 && var[boy].gmate)continue;
							else if (fin_level < 0 && (o, !var[boy].gmate))fin_level = l, dlink = 0, q = qq;
							oo, var[boy].mark = l + 1, marked[marks++] = boy, var[boy].arcs = 0;
							if (o, var[boy].gmate)o, queue[q++] = var[boy].gmate;
							else {
								if (showHK)fprintf(stderr, " top->"O"s\n", var[boy].name);
								o, arc[++k].tip = boy, arc[k].next = dlink, dlink = k;
							}
						}

						/*:40*/
//#line 839 ".\\partial-latin-gad.w"

						else if (p <= l)continue;
						if (showHK)fprintf(stderr, " "O"s->"O"s=>"O"s\n",
							var[boy].name, var[girl].name,
							var[girl].bmate ? var[var[girl].bmate].name : "bot");
						ooo, arc[++k].tip = girl, arc[k].next = var[boy].arcs, var[boy].arcs = k;
					}
				}
				if (q == qq)break;
			}

			/*:38*/
//#line 810 ".\\partial-latin-gad.w"
			;
			/*41:*/
//#line 871 ".\\partial-latin-gad.w"

			if (fin_level < 0) {
				if (showcauses)fprintf(stderr, " problem "O"d has no matching\n", m);
				GADone += mems - GADstart;
				GADtot += mems - GADstart;
				GADaborts++;
				goto abort;
			}

			/*:41*/
//#line 811 ".\\partial-latin-gad.w"
			;
			/*43:*/
//#line 883 ".\\partial-latin-gad.w"

			while (dlink) {
				o, boy = arc[dlink].tip, dlink = arc[dlink].next;
				l = fin_level;
			enter_level:o, lboy[l] = boy;
			advance:if (o, var[boy].arcs) {
				o, girl = arc[var[boy].arcs].tip, var[boy].arcs = arc[var[boy].arcs].next;
				o, b = var[girl].bmate;
				if (!b)/*44:*/
//#line 905 ".\\partial-latin-gad.w"

				{
					if (l)confusion("free girl", l);
					/*45:*/
//#line 921 ".\\partial-latin-gad.w"

					f--;
					o, j = var[girl].parent;
					ooo, i = queue[f], queue[j] = i, var[i].parent = j;

					/*:45*/
//#line 908 ".\\partial-latin-gad.w"
					;
					while (1) {
						if (showHK)fprintf(stderr, ""O"s "O"s-"O"s", l ? "," : " match",
							var[boy].name, var[girl].name);
						o, var[boy].mark = -1;
						ooo, j = var[boy].gmate, var[boy].gmate = girl, var[girl].bmate = boy;
						if (j == 0)break;
						o, girl = j, boy = lboy[++l];
					}
					if (showHK)fprintf(stderr, "\n");
					continue;
				}

				/*:44*/
//#line 891 ".\\partial-latin-gad.w"
				;
				if (o, var[b].mark < 0)goto advance;
				boy = b, l--;
				goto enter_level;
			}
			if (++l > fin_level)continue;
			o, boy = lboy[l];
			goto advance;
			}
			/*42:*/
//#line 880 ".\\partial-latin-gad.w"

			while (marks)oo, var[marked[--marks]].mark = 0;

			/*:42*/
//#line 900 ".\\partial-latin-gad.w"
			;

			/*:43*/
//#line 813 ".\\partial-latin-gad.w"
			;
			if (showHK) {
				fprintf(stderr, " ... "O"d pairs now matched (rank "O"d).\n", n - f, fin_level);
				/*37:*/
//#line 822 ".\\partial-latin-gad.w"

				{
					for (p = n; p < nn; p++) {
						girl = var[mch[m + p]].gmate;
						fprintf(stderr, " "O"s", girl ? var[girl].name : "???");
					}
					fprintf(stderr, "\n");
				}

				/*:37*/
//#line 816 ".\\partial-latin-gad.w"
				;
			}
		}

		/*:36*/
//#line 796 ".\\partial-latin-gad.w"
		;

		/*:34*/
//#line 762 ".\\partial-latin-gad.w"
		;
		GADone += mems - GADstart;
		/*47:*/
//#line 957 ".\\partial-latin-gad.w"

/*49:*/
//#line 981 ".\\partial-latin-gad.w"

		for (b = n; b < nn; b++) {
			o, boy = mch[m + b];
			oo, var[boy].rank = var[boy].arcs = 0;
		}

		/*:49*/
//#line 958 ".\\partial-latin-gad.w"
		;
		/*48:*/
//#line 969 ".\\partial-latin-gad.w"

		for (k = 0, g = 0; g < n; g++) {
			o, girl = mch[m + g];
			if (o, var[girl].pos >= active)continue;
			o, boy = var[girl].bmate;
			for (o, a = tet[girl].down; a != girl; o, a = tet[a].down) {
				o, pboy = tet[a + del].itm;
				if (pboy != boy)
					ooo, arc[++k].tip = boy, arc[k].next = var[pboy].arcs, var[pboy].arcs = k;
			}
		}

		/*:48*/
//#line 959 ".\\partial-latin-gad.w"
		;
		r = stack = 0;
		for (b = n; b < nn; b++) {
			o, v = mch[m + b];
			if (o, !var[v].rank) {
				/*50:*/
//#line 987 ".\\partial-latin-gad.w"

				{
					o, var[v].parent = 0;
					/*51:*/
//#line 996 ".\\partial-latin-gad.w"

					oo, var[v].rank = ++r, var[v].link = stack, stack = v;
					o, var[v].min = v;

					/*:51*/
//#line 990 ".\\partial-latin-gad.w"
					;
					do/*52:*/
//#line 1000 ".\\partial-latin-gad.w"

					{
						o, a = var[v].arcs;
						if (showT)fprintf(stderr, " Tarjan sees "O"s(rank "O"d)->"O"s\n",
							var[v].name, var[v].rank, a ? var[arc[a].tip].name : "/\\");
						if (a) {
							oo, u = arc[a].tip, var[v].arcs = arc[a].next;
							if (o, var[u].rank) {
								if (oo, var[u].rank < var[var[v].min].rank)
									o, var[v].min = u;
							}
							else {
								o, var[u].parent = v;
								v = u;
								/*51:*/
//#line 996 ".\\partial-latin-gad.w"

								oo, var[v].rank = ++r, var[v].link = stack, stack = v;
								o, var[v].min = v;

								/*:51*/
//#line 1013 ".\\partial-latin-gad.w"
								;
							}
						}
						else {
							o, u = var[v].parent;
							if (var[v].min == v)/*53:*/
//#line 1027 ".\\partial-latin-gad.w"

							{
								t = stack;
								o, stack = var[v].link;
								for (newn = 0, p = t;; o, p = var[p].link) {
									o, var[p].rank = maxn + mchptr;
									newn++;
									if (p == v)break;
								}
								if (newn == n)goto doneGAD;
								if (newn > 1 || (o, var[v].pos < active)) {
									/*54:*/
//#line 1050 ".\\partial-latin-gad.w"

									if (mchptr + mextra + newn + newn >= mchsize) {
										fprintf(stderr, "Match table overflow (mchsize="O"d)!\n", mchsize);
										exit(-666);
									}
									oo, mch[mchptr + mstamp] = 0, mch[mchptr + mparent] = m, mch[mchptr + msize] = newn;
									for (k = mchptr;; o, k++, t = var[t].link) {
										o, mch[k + newn] = t;
										ooo, girl = var[t].gmate, mch[k] = girl, var[girl].matching = mchptr;
										if (t == v)break;
									}
									if (showsubproblems)print_match_prob(mchptr);
									o, k = mchptr, mchptr += mextra + newn + newn, mch[mchptr + mprev] = k;
									if (mchptr > maxmchptr)maxmchptr = mchptr;

									/*:54*/
//#line 1038 ".\\partial-latin-gad.w"
									;
									if (newn == 1) {
										o, girl = var[v].gmate;
										for (o, a = tet[girl].down; a != girl; o, a = tet[a].down)
											if (o, tet[a + del].itm == v)break;
										if (a == girl)confusion("lost option", girl);
										opt = a & -4;
										if (o, !tet[opt].itm)oo, tet[opt].itm = 1, forced[forcedptr++] = opt;
									}
								}
							}

							/*:53*/
//#line 1018 ".\\partial-latin-gad.w"

							else

								if (ooo, var[var[v].min].rank < var[var[u].min].rank)
									o, var[u].min = var[v].min;
							v = u;
						}
					}

					/*:52*/
//#line 992 ".\\partial-latin-gad.w"

					while (v);
				}

				/*:50*/
//#line 965 ".\\partial-latin-gad.w"
				;
			}
		}

		/*:47*/
//#line 764 ".\\partial-latin-gad.w"
		;
		/*55:*/
//#line 1069 ".\\partial-latin-gad.w"

		for (g = 0; g < n; g++) {
			o, girl = mch[m + g];
			if (o, var[girl].pos >= active)continue;
			for (o, a = tet[girl].down; a != girl; o, a = tet[a].down) {
				o, boy = tet[a + del].itm;
				if (oo, maxn + var[girl].matching != var[boy].rank) {
					opt = a & -4;/*58:*/
//#line 1131 ".\\partial-latin-gad.w"

					{
						if (showprunes)fprintf(stderr, " pruning "O"s\n", tet[opt].aux);
						o, trail[trailptr++] = opt + pruned;
						o, tet[opt].up = 1;
						zerofound = 0;
						t = opt + 1;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

						oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
						oo, tet[p].down = q, tet[q].up = p;
						oo, l = tet[r].len - 1, tet[r].len = l;
						o, s = var[r].matching;
						if (o, !mch[s + mstamp])
							oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
						if (l <= 1) {
							if (l == 0)oo, var[r].tally++, zerofound = r;
							else {
								o, p = tet[r].down & -4;
								if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
							}
						}

						/*:56*/
//#line 1137 ".\\partial-latin-gad.w"
						;
						t = opt + 2;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

						oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
						oo, tet[p].down = q, tet[q].up = p;
						oo, l = tet[r].len - 1, tet[r].len = l;
						o, s = var[r].matching;
						if (o, !mch[s + mstamp])
							oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
						if (l <= 1) {
							if (l == 0)oo, var[r].tally++, zerofound = r;
							else {
								o, p = tet[r].down & -4;
								if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
							}
						}

						/*:56*/
//#line 1138 ".\\partial-latin-gad.w"
						;
						t = opt + 3;/*56:*/
//#line 1111 ".\\partial-latin-gad.w"

						oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
						oo, tet[p].down = q, tet[q].up = p;
						oo, l = tet[r].len - 1, tet[r].len = l;
						o, s = var[r].matching;
						if (o, !mch[s + mstamp])
							oo, mch[s + mstamp] = 1, tofilter[tofiltertail] = s, tofiltertail = (tofiltertail + 1) & qmod;
						if (l <= 1) {
							if (l == 0)oo, var[r].tally++, zerofound = r;
							else {
								o, p = tet[r].down & -4;
								if (o, !tet[p].itm)oo, tet[p].itm = 1, forced[forcedptr++] = p;
							}
						}

						/*:56*/
//#line 1139 ".\\partial-latin-gad.w"
						;
						if (zerofound) {
							if (showcauses)fprintf(stderr, " no options for "O"s\n", var[zerofound].name);
							goto abort;
						}
					}

					/*:58*/
//#line 1076 ".\\partial-latin-gad.w"
					;
					oo, t = var[tet[a + del].itm].matching;
					if (o, !mch[t + mstamp])
						oo, mch[t + mstamp] = 1, tofilter[tofiltertail] = t,
						tofiltertail = (tofiltertail + 1) & qmod;
					oo, t = var[tet[a + delp].itm].matching;
					if (o, !mch[t + mstamp])
						oo, mch[t + mstamp] = 1, tofilter[tofiltertail] = t,
						tofiltertail = (tofiltertail + 1) & qmod;
				}
			}
		}

		/*:55*/
//#line 765 ".\\partial-latin-gad.w"
		;
	doneGAD:GADtot += mems - GADstart;

		/*:33*/
//#line 1373 ".\\partial-latin-gad.w"
		;
	}

	/*:68*/
//#line 1338 ".\\partial-latin-gad.w"
	;
	if (active < mina)mina = active;
	if (active)goto choose;
	count++;
	if (showsols)/*78:*/
//#line 1497 ".\\partial-latin-gad.w"

	{
		printf("Solution #"O"lld:\n", count);
		for (t = 0; t < trailptr; t++)if ((trail[t] & 0x3) == 0) {
			opt = trail[t];
			i = decode(tet[opt].aux[0]);
			j = decode(tet[opt].aux[1]);
			k = decode(tet[opt].aux[2]);
			board[i - 1][j - 1] = k;
		}
		for (i = 0; i < originaln; i++) {
			for (j = 0; j < originaln; j++)printf(""O"c", encode(board[i][j]));
			printf("\n");
		}
		print_state();
	}

	/*:78*/
//#line 1342 ".\\partial-latin-gad.w"
	;
abort:
	if (level) {
		/*69:*/
	//#line 1376 ".\\partial-latin-gad.w"

		while (forcedptr) {
			o, opt = forced[--forcedptr];
			o, tet[opt].itm = 0;
		}
		while (tofilterhead != tofiltertail) {
			o, m = tofilter[tofilterhead], tofilterhead = (tofilterhead + 1) & qmod;
			o, mch[m + mstamp] = 0;
		}

		/*:69*/
	//#line 1344 ".\\partial-latin-gad.w"
		;
		/*71:*/
	//#line 1394 ".\\partial-latin-gad.w"

		while (mchptr > move[level].mchptrstart) {
			oo, m = mch[mchptr + mprev], n = mch[m + msize], p = mch[m + mparent];
			for (k = 0; k < n; k++)oo, var[mch[m + k]].matching = p;
			mchptr = m;
		}
		if (mchptr != move[level].mchptrstart)
			confusion("mchptrstart", mchptr - move[level].mchptrstart);

		/*:71*/
	//#line 1345 ".\\partial-latin-gad.w"
		;
		/*70:*/
	//#line 1386 ".\\partial-latin-gad.w"

		o;
		while (trailptr != move[level].trailstart) {
			o, opt = trail[--trailptr] & -4;
			if (trail[trailptr] & 0x3)/*59:*/
	//#line 1146 ".\\partial-latin-gad.w"

			{
				t = opt + 3;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

				oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
				oo, tet[p].down = tet[q].up = t;
				oo, l = tet[r].len + 1, tet[r].len = l;

				/*:57*/
	//#line 1148 ".\\partial-latin-gad.w"
				;
				t = opt + 2;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

				oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
				oo, tet[p].down = tet[q].up = t;
				oo, l = tet[r].len + 1, tet[r].len = l;

				/*:57*/
	//#line 1149 ".\\partial-latin-gad.w"
				;
				t = opt + 1;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

				oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
				oo, tet[p].down = tet[q].up = t;
				oo, l = tet[r].len + 1, tet[r].len = l;

				/*:57*/
	//#line 1150 ".\\partial-latin-gad.w"
				;
				o, tet[opt].up = 0;
			}

			/*:59*/
	//#line 1390 ".\\partial-latin-gad.w"

			else/*61:*/
	//#line 1201 ".\\partial-latin-gad.w"

			{
				ooo, pij = tet[opt + 1].itm, rik = tet[opt + 2].itm, cjk = tet[opt + 3].itm;
				for (o, a = tet[cjk].up; a != cjk; o, a = tet[a].up)if (a != opt + 3) {
					t = a - 2;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = tet[q].up = t;
					oo, l = tet[r].len + 1, tet[r].len = l;

					/*:57*/
	//#line 1205 ".\\partial-latin-gad.w"
					;
					t = a - 1;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = tet[q].up = t;
					oo, l = tet[r].len + 1, tet[r].len = l;

					/*:57*/
	//#line 1206 ".\\partial-latin-gad.w"
					;
				}
				for (o, a = tet[rik].up; a != rik; o, a = tet[a].up)if (a != opt + 2) {
					t = a - 1;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = tet[q].up = t;
					oo, l = tet[r].len + 1, tet[r].len = l;

					/*:57*/
	//#line 1209 ".\\partial-latin-gad.w"
					;
					t = a + 1;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = tet[q].up = t;
					oo, l = tet[r].len + 1, tet[r].len = l;

					/*:57*/
	//#line 1210 ".\\partial-latin-gad.w"
					;
				}
				for (o, a = tet[pij].up; a != pij; o, a = tet[a].up)if (a != opt + 1) {
					t = a + 2;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = tet[q].up = t;
					oo, l = tet[r].len + 1, tet[r].len = l;

					/*:57*/
	//#line 1213 ".\\partial-latin-gad.w"
					;
					t = a + 1;/*57:*/
	//#line 1126 ".\\partial-latin-gad.w"

					oo, p = tet[t].up, q = tet[t].down, r = tet[t].itm;
					oo, tet[p].down = tet[q].up = t;
					oo, l = tet[r].len + 1, tet[r].len = l;

					/*:57*/
	//#line 1214 ".\\partial-latin-gad.w"
					;
				}
				active += 3;
			}

			/*:61*/
	//#line 1391 ".\\partial-latin-gad.w"
			;
		}

		/*:70*/
	//#line 1346 ".\\partial-latin-gad.w"
		;
		if (o, move[level].choiceno < move[level].choices) {
			oo, move[level].curchoice = tet[move[level].curchoice].down;
			o, move[level].choiceno++;
			goto enternode;
		}
		level--;
		if (showcauses)fprintf(stderr, "done with branches from node "O"lld\n",
			move[level].nodeid);
		goto abort;
	}

	/*:67*/
	//#line 1406 ".\\partial-latin-gad.w"
	;

	/*:72*/
	//#line 64 ".\\partial-latin-gad.w"
	;
	/*79:*/
	//#line 1516 ".\\partial-latin-gad.w"

	save_tallies(totvars);
	fprintf(stderr,
		"Altogether "O"llu solution"O"s, "O"llu mems, "O"llu nodes.\n",
		count, count == 1 ? "" : "s", mems, nodes);
	fprintf(stderr, "(GAD time "O"llu+"O"llu, "O"llu/"O"llu aborted;",
		GADone, GADtot - GADone, GADaborts, GADtries);
	fprintf(stderr, " maxl="O"d, mina="O"d, maxmchptr="O"d)\n",
		maxl, mina, maxmchptr);


	/*:79*/
	//#line 65 ".\\partial-latin-gad.w"
	;
}

/*:1*/

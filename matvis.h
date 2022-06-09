#pragma once
#include "stdinc.h"

class MatVis : public LogClient {
public:
	MatVis(int _n) {
		n = _n;
		hSz = (n + 1) * hEntSz;
		vSz = (n + 1) * vEntSz;
		assert(hEntSz <= (1 << ENTSZLOG));
		assert(vEntSz <= (1 << ENTSZLOG));
		Canvas.Preset(' ');
		FillGrid();
	}

	int n;
	static const int ENTSZLOG = 2;
	const int hEntSz = 3;
	const int vEntSz = 2;
	const int hEntCtr = hEntSz >> 1;
	const int vEntCtr = 1;
	const int hMarg = hEntSz + hEntCtr;
	const int vMarg = vEntSz;
	const int numSz = hEntSz - hEntCtr;

	int hSz, vSz;
	FastMatrix<FASTMATDIMLOG + ENTSZLOG, char> Canvas;
	
	int HEntCtr(int j) { return (j) * hEntSz + hMarg; }
	int VEntCtr(int i) { return (i) * vEntSz + vMarg; }

	void SetNum(int v, int sz, int cv, int ch) {
		stringstream conv;
		string res;
		conv << dec << v;
		conv >> res;
		int del = sz - (int) res.length(); 
		ch += del;
		sz -= del;
		for (int l = 0; l < sz; l++) {
			*Canvas.Address(ch, cv) = res.c_str()[l];
			ch++;
		}
	}

	void SetTopMarg() {
		int mi = 0;
		for (int j = 0; j < n; j++) {
			SetNum(j, numSz, mi, HEntCtr(j));
		}
	}

	void SetLftMarg() {
		int mj = hEntCtr;
		for (int i = 0; i < n; i++) {
			SetNum(i, numSz, VEntCtr(i), mj);
		}
	}

	void SetHLine(int v, int start, int last, int freq, char c) {
		for (int h = start; h < last; h += freq) {
			*Canvas.Address(h, v) = c;
		}
	}

	const char cLeft = '<';
	void ArrowLeft(int i, int jLft, int jRt) {
		int v = VEntCtr(i);
		SetHLine(v, HEntCtr(jLft), HEntCtr(jRt), 1, cLeft);
	}

	void SetVLine(int h, int start, int last, int freq, char c) {
		for (int v = start; v < last; v += freq) {
			*Canvas.Address(h, v) = c;
		}
	}

	const char cUp = '^';
	void ArrowUp(int j, int iTop, int iBot) {
		int h = HEntCtr(j);
		SetVLine(h, VEntCtr(iTop), VEntCtr(iBot), 1, cUp);
	}

	void SetEnt(int i, int j, char cEnt = '*') {
		*Canvas.Address(HEntCtr(j), VEntCtr(i)) = cEnt;
	}

	void Out(ostream* pOut = nullptr) {
		for (int v = 0; v < vSz; v++) {
			string line = "";
			for (int h = 0; h < hSz; h++) 
				line += Canvas(h, v);
			line += '\n';
			if (pOut == nullptr) {
				Report(line.c_str());
			} else
				*pOut << line << endl;
		}
	}

	const int gridSz = 4;
	void FillGrid() {
		SetLftMarg();
		SetTopMarg();
		SetHLine(vMarg - 1, 0, hSz, 1, '=');
		SetVLine(hMarg - 1, 0, vSz, 1, '|');

		for (int i = gridSz; i < n; i += gridSz)
			SetHLine(VEntCtr(i) - hEntCtr, 0, hSz, 1, '.');

		for (int j = gridSz; j < n; j += gridSz)
			SetVLine(HEntCtr(j) - vEntCtr, 0, vSz, 1, '.');

	}

	void Test() {
		SetLftMarg();
		SetTopMarg();
		SetHLine(vMarg - 1, 0, hSz, 1, '=');
		SetVLine(hMarg - 1, 0, vSz, 1, '|');

		for (int i = gridSz; i < n; i += gridSz)
			SetHLine(VEntCtr(i) - hEntCtr, 0, hSz, 1, '.');

		for (int j = gridSz; j < n; j += gridSz)
			SetVLine(HEntCtr(j) - vEntCtr, 0, vSz, 1, '.');

		ArrowLeft(5, 3, 8);
		ArrowLeft(7, 0, 11);
		ArrowLeft(2, 4, 4);

		ArrowUp(5, 3, 8);
		ArrowUp(6, 3, 8);
		ArrowUp(11, 0, 11);


		Out(&cout);
	}
};
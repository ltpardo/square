#pragma once

class LoggerSrv {
public:
	LoggerSrv() : strCnt(0)
	{
		for (int s = 0; s < strCnt; s++) {
			StrTab[s] = { "", 0 };
		}

		dir = "../common/logs/";
	}

	static const int STRMAX = 8;
	int strCnt;

	typedef struct {
		string name;
		ofstream* pStr;
	} StrEnt;

	StrEnt StrTab[STRMAX];

	bool StrTabCompact() {
		return false;
	}

	string dir;
	inline string FullName(string name) { return dir + name; }

	bool StrOpen(string sname) {
		if (strCnt >= STRMAX)
				return false;

		if (sname == "cout") {
			StrTab[strCnt++] = { "cout", (ofstream *) &cout };
			return true;
		}

		StrTab[strCnt].name = FullName(sname);
		ofstream* pStr = new ofstream(StrTab[strCnt].name);
		if (pStr == 0) {
			StrTab[strCnt].name = "";
			return false;
		}
		StrTab[strCnt++].pStr = pStr;
		return true;
	}

	bool StrClose(string sname) {
		int s;
		string full = sname == "cout" ? sname : FullName(sname);
		for (s = 0; s < strCnt; s++) {
			if (StrTab[s].name == full) {
				if (sname != "cout")
					StrTab[s].pStr->close();
				break;
			}
		}
		if (s >= strCnt)
			// Stream not found
			return false;

		// Compact StrTab
		for (; s < strCnt - 1; s++) {
			StrTab[s] = StrTab[s + 1];
		}
		StrTab[--strCnt] = { "", 0 };
		return true;
	}
};

extern LoggerSrv Logger;

class LogClient {
public:
	LogClient() {}

	int strCur;
	ostream* StrFirst() {
		strCur = 1;
		return (ostream*)Logger.StrTab[0].pStr;
	}

	ostream* StrNext() {
		if (strCur >= Logger.strCnt)
			return 0;
		else
			return (ostream*)Logger.StrTab[strCur++].pStr;
	}

	virtual void Dump(ostream& Str, int dumpType = 0) {
		cerr << "Missing dump method" << endl;
	}

	void Dump(int dumpType = 0) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			Dump(*pStr, dumpType);
	}

	void ReportDec() {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << dec;
	}
	void Report(string& msg) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg;
	}
	void Report(string& msg, int par0) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg << " " << par0 << endl;
	}
	void Report(string& msg, int par0, int par1) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg << " " << par0 << " " << par1 << endl;
	}
	void Report(string& msg, int par0, int par1, int par2) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg << " " << par0 << " " << par1 << " " << par2 << endl;
	}

	void Report(const char* msg) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg;
	}
	void Report(const char* msg, int par0) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg << " " << par0 << endl;
	}
	void Report(const char* msg, int par0, int par1) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg << " " << par0 << " " << par1 << endl;
	}
	void Report(const char* msg, int par0, int par1, int par2) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg << " " << par0 << " " << par1 << " " << par2 << endl;
	}
	void Report(const char* msg, int par0, int par1, int par2, int par3) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << msg << " " << par0 << " " << par1 << " " << par2 << " " << par3 << endl;
	}
	void Report(string & str1, string & str2, int par = 0) {
		for (ostream* pStr = StrFirst(); pStr != 0; pStr = StrNext())
			*pStr << str1 << " " << str2 << " " << par << endl;
	}
};


//
//  Command console
//

#include "stdinc.h"

//////////////////////////////////////////////////////////
//
//	Input handling
//
//////////////////////////////////////////////////////////

void InputState::set(istream* _pIn, const char* _prompt,
	const char* _name, bool _isBottom)
{
	pIn = _pIn;
	prompt = _prompt;
	name = _name;
	isBottom = _isBottom;
}

void InputState::set(istream* _pIn, string _prompt,
	string _name, bool _isBottom)
{
	pIn = _pIn;
	prompt = _prompt;
	name = _name;
	isBottom = _isBottom;
}


//////////////////////////////////////////////////////////
//
//	Consola
//
//////////////////////////////////////////////////////////

int Consola::tokToInt(string& tok)
{
	int v;
	std::istringstream strm(tok);
	strm >> v;
	return v;
}

float Consola::tokToFloat(string& tok)
{
	float v;
	std::istringstream strm(tok);
	strm >> v;
	return v;
}

// Input line and tokenize
bool Consola::getCmdLine()
{
	tokenCnt = 0;
	inputSt.pIn->getline(lineBuf, sizeof(lineBuf));
	if (lineEof = inputSt.pIn->eof())
		return false;

	// Tokenize
	std::istringstream strm(lineBuf);
	string tok;
	for (; tokenCnt < tokenMax; ) {
		tok = "";
		strm >> tok;
		if (tok.size() > 0) {
			if (tok[0] == commentChar)
				// Rest of line is a comment
				break;
			Tokens[tokenCnt++] = tok;
		}
		if (strm.eof())
			break;
	}
	return true;
}

// Echo line on console output
void Consola::showLine()
{
	if (!inputSt.isBottom)
		std::cout << lineBuf << endl;
}

// Echo line on Logger
void Consola::logLine()
{
	string msg = "\n>>  ";
	msg += lineBuf;
	msg += " >>\n";
	Report(msg);
}

// Keep track of IF / FI / ELSE depth
bool Consola::processDepth()
{
	if (Tokens[0] == "if") {
		DSt.push(dpst);
		if (!dpst.exec) {
			dpst = NestState(false, NestState::TOFI);
		}
		else {
			bool cond = (Tokens[1] == "t");
			dpst = NestState(cond, NestState::TOELSE);
		}
	}
	else if (Tokens[0] == "else") {
		if (!dpst.mode == NestState::TOELSE)
			cerr << "Invalid else" << endl;
		dpst = NestState(!dpst.exec, NestState::TOFI);
	}
	else if (Tokens[0] == "fi") {
		dpst = DSt.top();
		DSt.pop();
	}
	else
		return false;
	return true;
}

void Consola::setAtBottom()
{
	inputSt.set(&cin, ">> ", "console", true);
	InStk.push(inputSt);
}

void Consola::popInput()
{
	if (inputSt.pIn != &cin) {
		pFS = (ifstream*)inputSt.pIn;
		pFS->close();
		delete pFS;
	}
	inputSt = InStk.top();
	InStk.pop();
}

void Consola::pushInput(const char *fName)
{
	string openName = scriptdir + fName;
	pFS = new std::ifstream(openName);
	if (!pFS->is_open()) {
		cerr << "Failed exec " << openName << endl;
	}
	else {
		InStk.push(inputSt);
		inputSt.set(pFS, (string("[") + fName + "] "),
			string(fName), false);
	}
}

// Process file execution
void  Consola::procExec()
{
	if (tokenCnt <= 1) {
		pushInput(scriptDefault.c_str());
	} else
		pushInput(Tokens[1].c_str());
}

// Process end of execution from a command stream
bool Consola::procEnd()
{
	if (Tokens[1] == "run") {
		// Pop all levels and end session
		while (!inputSt.isBottom) {
			popInput();
		}
	}

	if (!inputSt.isBottom) {
		popInput();
		return true;
	}

	// Already at bottom, end of console session
	return false;
}

// Push in a new console
void Consola::pushCons()
{
	InStk.push(inputSt);
	inputSt.set(&cin, string(">>"), "", false);
}

// Top level dispatcher of user commands
//	Returns false when user access should stop:
//		either "end" command was issued at bottom level
//		or a response is needed
//
bool Consola::getCmd()
{
	bool retVal = true;

	outPrompt();
	getCmdLine();
	if (tokenCnt <= 0)
		// Empty line
		return true;

	showLine();
	logLine();

	// Process structure commands first
	if (processDepth())
		// Nesting command
		return true;

	if (!dpst.exec)
		// Nested Skipping
		return true;

	if (Tokens[0] == "exec") {
		procExec();
	}
	else if (Tokens[0] == "end") {
		return procEnd();
	}
	else if (Tokens[0] == "cons") {
		pushCons();
	}
	else if (Tokens[0] == "quit") {
		// Brute force exit
		exit(0);
	}
	else 
		return processCmd();

	return retVal;
}

void Consola::ReportTime()
{
	ostringstream msStr;
	msStr << ms;
	string msg = " [Cmd Time ";
	msg += msStr.str();
	msg += " ms]\n";
	Report(msg);
}



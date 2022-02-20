#pragma once

#include <chrono>
using namespace std;

class SimpleTimer {
public:
    SimpleTimer() {}

    chrono::high_resolution_clock::time_point begin;
    chrono::high_resolution_clock::time_point end;
    chrono::high_resolution_clock::time_point current;
    chrono::high_resolution_clock::duration lapsed;
    long long ms;
    long long mus;

    void Start() {
        begin = chrono::high_resolution_clock::now();
    }
    void Lapsed() {
        current = chrono::high_resolution_clock::now();
        lapsed = current - begin;
    }

    long long LapsedMuSecs(ostream* pOut = 0) {
        Lapsed();
        mus = std::chrono::duration_cast<std::chrono::microseconds>(lapsed).count();
        if (pOut)
            *pOut << "========================  " << mus << " muSecs" << endl;
        return mus;
    }

    long long LapsedMSecs(ostream *pOut = 0) {
        Lapsed();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(lapsed).count();
        if (pOut)
            *pOut << "========================  " << ms << " mSecs" << endl;
        return ms;
    }


};

extern SimpleTimer Timer;

#include "DebugStack.h"
#include <iostream>
#include <thread>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#endif

void LogStack(const char* tag) {
    std::ostringstream oss;
    oss << "[LogStack] tag=" << (tag ? tag : "") << " thread=" << std::this_thread::get_id() << "\n";
#ifdef _WIN32
    void* stack[62];
    WORD frames = CaptureStackBackTrace(0, 62, stack, NULL);
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    for (WORD i = 0; i < frames; ++i) {
        DWORD64 address = (DWORD64)(stack[i]);
        if (SymFromAddr(process, address, 0, symbol)) {
            oss << frames - i - 1 << " " << symbol->Name << " [0x" << std::hex << symbol->Address << std::dec << "]\n";
        } else {
            oss << frames - i - 1 << " 0x" << std::hex << address << std::dec << "\n";
        }
    }
    free(symbol);
#else
    const int max_frames = 64;
    void* addrlist[max_frames+1];
    int frames = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
    char** symbollist = backtrace_symbols(addrlist, frames);
    for (int i = 0; i < frames; ++i) {
        oss << i << " " << (symbollist ? symbollist[i] : "") << "\n";
    }
    free(symbollist);
#endif
    std::cerr << oss.str() << std::flush;
}

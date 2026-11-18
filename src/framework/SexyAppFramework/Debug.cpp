#include "Common.h"
#include "Debug.h"

#include <time.h>

// Switch port: the leak tracker (memmgr.h's operator new hooks + SexyDumpUnfreed) is gone; tracing and OutputDebug
// remain, OutputDebug going to stderr (nxlink) via win32compat's OutputDebugStringA.

bool gInAssert = false;

static FILE *gTraceFile = NULL;
static int gTraceFileLen = 0;
static int gTraceFileNum = 1;

using namespace Sexy;

static void TraceWrite(const char* theStr)
{
	if (gTraceFile==NULL)
	{
		gTraceFileNum = (gTraceFileNum+1)%2;
		char aBuf[50];
		sprintf(aBuf,"trace%d.txt",gTraceFileNum+1);
		gTraceFile = fopen(aBuf,"w");
		if (gTraceFile==NULL)
			return;
	}

	fputs(theStr, gTraceFile);
	fflush(gTraceFile);

	gTraceFileLen += strlen(theStr);
	if (gTraceFileLen > 100000)
	{
		fclose(gTraceFile);
		gTraceFile = NULL;
		gTraceFileLen = 0;
	}
}

void SexyTrace(const char *theStr)
{
	TraceWrite(theStr);
	TraceWrite("\n");
}

void SexyTraceFmt(const SexyChar* fmt ...)
{
	// Does not append a newline by default, also takes vararg parameters
	va_list argList;
	va_start(argList, fmt);
	std::string result = SexyStringToStringFast(vformat(fmt, argList));
	va_end(argList);

	TraceWrite(result.c_str());
}

// FUNCTION: GAME187 0x4691e0
void OutputDebug(const SexyChar* fmt ...)
{
	va_list argList;
	va_start(argList, fmt);
	std::string result = SexyStringToStringFast(vformat(fmt, argList));
	va_end(argList);

	OutputDebugStringA(result.c_str());
}

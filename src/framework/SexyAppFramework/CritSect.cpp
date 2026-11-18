#pragma warning( disable : 4786 )

#include "CritSect.h"
#include <windows.h>

using namespace Sexy;

////////////////////////////////////////////////////////////////////////////////

// FUNCTION: GAME187 0x469240
CritSect::CritSect(void)
{
	InitializeCriticalSection(&mCriticalSection);
}

////////////////////////////////////////////////////////////////////////////////

// FUNCTION: GAME187 0x469250
CritSect::~CritSect(void)
{
	DeleteCriticalSection(&mCriticalSection);
}

// Minimal stand-ins for the Win32 types and functions the 1.87 framework and game still reference.
// Only plain types, constants and small helpers live here; anything with real platform behaviour (window, input,
// audio, registry, files) is implemented in platform/switch/*.cpp instead.
#ifndef __WIN32COMPAT_H__
#define __WIN32COMPAT_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include <wctype.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <sys/stat.h>

// --- Basic types -----------------------------------------------------------------------------------
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t BOOL;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint32_t UINT;
typedef int64_t __int64;
typedef int64_t LONGLONG;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef char TCHAR;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef const char* LPCTSTR;
typedef char* LPTSTR;
typedef void* LPVOID;
typedef DWORD* LPDWORD;
typedef BYTE* LPBYTE;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef int32_t HRESULT;
typedef DWORD COLORREF;

typedef void* HANDLE;
struct HWND__;
typedef HWND__* HWND;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HCURSOR;
typedef void* HICON;
typedef void* HDC;
typedef void* HFONT;
typedef void* HBITMAP;
typedef void* HKEY;
typedef void* HGLOBAL;

union LARGE_INTEGER
{
    struct { DWORD LowPart; LONG HighPart; };
    int64_t QuadPart;
};

struct POINT { LONG x, y; };
struct RECT { LONG left, top, right, bottom; };
struct SIZE { LONG cx, cy; };

struct MSG
{
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
};

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define WINAPI
#define CALLBACK
#define APIENTRY
#define __stdcall
#define __cdecl
#define __forceinline inline __attribute__((always_inline))

#define MAX_PATH 260
#define S_OK 0
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr) ((HRESULT)(hr) < 0)

#define LOWORD(l) ((WORD)(((uintptr_t)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((uintptr_t)(l)) >> 16) & 0xffff))
#define MAKELONG(a, b) ((LONG)(((WORD)(((uintptr_t)(a)) & 0xffff)) | ((DWORD)((WORD)(((uintptr_t)(b)) & 0xffff))) << 16))
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))

// --- MessageBox flags (SexyAppBase::MsgBox keeps the Win32 values) ----------------------------------
#define MB_OK 0x00000000
#define MB_OKCANCEL 0x00000001
#define MB_YESNOCANCEL 0x00000003
#define MB_YESNO 0x00000004
#define MB_ICONHAND 0x00000010
#define MB_ICONERROR 0x00000010
#define MB_ICONQUESTION 0x00000020
#define MB_ICONEXCLAMATION 0x00000030
#define MB_ICONWARNING 0x00000030
#define MB_ICONINFORMATION 0x00000040
#define MB_TOPMOST 0x00040000
#define MB_SYSTEMMODAL 0x00001000
#define MB_TASKMODAL 0x00002000
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7

// --- Registry value types (the registry itself is a file, see platform/switch/Registry.cpp) ----------
#define REG_NONE 0
#define REG_SZ 1
#define REG_BINARY 3
#define REG_DWORD 4
#define HKEY_CLASSES_ROOT ((HKEY)(uintptr_t)0x80000000)
#define HKEY_CURRENT_USER ((HKEY)(uintptr_t)0x80000001)
#define HKEY_LOCAL_MACHINE ((HKEY)(uintptr_t)0x80000002)

// --- Critical sections: recursive, like Win32's -------------------------------------------------------
struct CRITICAL_SECTION
{
    std::recursive_mutex mMutex;
};
typedef CRITICAL_SECTION* LPCRITICAL_SECTION;
inline void InitializeCriticalSection(LPCRITICAL_SECTION) {}
inline void DeleteCriticalSection(LPCRITICAL_SECTION) {}
inline void EnterCriticalSection(LPCRITICAL_SECTION cs) { cs->mMutex.lock(); }
inline void LeaveCriticalSection(LPCRITICAL_SECTION cs) { cs->mMutex.unlock(); }
inline BOOL TryEnterCriticalSection(LPCRITICAL_SECTION cs) { return cs->mMutex.try_lock(); }

// --- Time -------------------------------------------------------------------------------------------
inline DWORD GetTickCount()
{
    using namespace std::chrono;
    static const steady_clock::time_point sStart = steady_clock::now();
    return (DWORD)duration_cast<milliseconds>(steady_clock::now() - sStart).count();
}
inline DWORD timeGetTime() { return GetTickCount(); }
inline void Sleep(DWORD theMilliseconds) { std::this_thread::sleep_for(std::chrono::milliseconds(theMilliseconds)); }
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* f) { f->QuadPart = 1000000000LL; return TRUE; }
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* c)
{
    c->QuadPart = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return TRUE;
}

// --- Threads ----------------------------------------------------------------------------------------
// Small sequential ids; the framework only compares them against the main thread's.
inline DWORD GetCurrentThreadId()
{
    static std::atomic<DWORD> sNextId(1);
    static thread_local DWORD sId = sNextId++;
    return sId;
}

// --- Window messages ----------------------------------------------------------------------------------
// SexyAppBase still queues input as MSG records (see SexyAppBase::ProcessSDLEvents), so the Win32 numbers stay.
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_CLOSE 0x0010
#define WM_SYSCOLORCHANGE 0x0015
#define WM_ACTIVATEAPP 0x001C
#define WM_DISPLAYCHANGE 0x007E
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_TIMER 0x0113
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MOUSEWHEEL 0x020A
#define SIZE_RESTORED 0
#define SIZE_MINIMIZED 1
#define SIZE_MAXIMIZED 2

// Virtual key codes are the KEYCODE_* values in KeyCodes.h; these are the names the framework uses directly.
#define VK_RETURN 0x0D
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_SNAPSHOT 0x2C
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F8 0x77
#define VK_F10 0x79
#define VK_F11 0x7A

// Mouse capture means nothing with touch input.
inline HWND SetCapture(HWND theWnd) { return theWnd; }
inline BOOL ReleaseCapture() { return TRUE; }

// Only the demo handle bookkeeping waits on handles, and nothing on the Switch hands it one.
#define INFINITE		0xFFFFFFFF
#define WAIT_OBJECT_0	0
#define WAIT_TIMEOUT	258
inline DWORD WaitForSingleObject(HANDLE theHandle, DWORD theMilliseconds) { return WAIT_OBJECT_0; }

// --- Memory -----------------------------------------------------------------------------------------
#define ZeroMemory(p, n) memset((p), 0, (n))
#define CopyMemory(d, s, n) memcpy((d), (s), (n))

// --- Debug output -----------------------------------------------------------------------------------
inline void OutputDebugStringA(const char* s) { fputs(s, stderr); }
#define OutputDebugString OutputDebugStringA

// --- CRT names MSVC has and newlib doesn't -----------------------------------------------------------
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _snwprintf swprintf
#define _vsnwprintf vswprintf
#define vsnwprintf vswprintf
#define _strdup strdup
#define _unlink unlink
#define _atoi64 atoll
#define _MAX_PATH MAX_PATH

inline int wcsicmp(const wchar_t* a, const wchar_t* b)
{
    for (;; a++, b++)
    {
        wint_t ca = towlower(*a), cb = towlower(*b);
        if (ca != cb || !ca)
            return (int)ca - (int)cb;
    }
}
#define _wcsicmp wcsicmp
inline int _wtoi(const wchar_t* s) { return (int)wcstol(s, nullptr, 10); }

inline char* _itoa(int value, char* buf, int radix)
{
    if (radix == 16)
        sprintf(buf, "%x", value);
    else
        sprintf(buf, "%d", value);
    return buf;
}
#define itoa _itoa

// windows.h's min/max macros take mixed argument types; std::min/max don't. When both types match, std's
// overloads are more specialised and win, so `using namespace std` code still resolves.
template <class A, class B>
inline typename std::common_type<A, B>::type min(A a, B b) { return (a < b) ? a : b; }
template <class A, class B>
inline typename std::common_type<A, B>::type max(A a, B b) { return (a > b) ? a : b; }

inline char* _strlwr(char* s) { for (char* p = s; *p; p++) *p = (char)tolower((unsigned char)*p); return s; }
inline char* _strupr(char* s) { for (char* p = s; *p; p++) *p = (char)toupper((unsigned char)*p); return s; }
#define strlwr _strlwr
#define strupr _strupr

#endif // __WIN32COMPAT_H__

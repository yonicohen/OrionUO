#pragma once

#if !defined(ORION_WINDOWS)

#include <unistd.h>
#include <time.h>

#include <chrono>
#include <thread>

#define NO_SDL_GLEXT
#if defined(ORION_GLES)
// Android has no desktop GL and no GLEW: GLES needs no extension loader, since
// the core entry points are exported directly by libGLESv1_CM. GLCompat.h fills
// in what the renderer still calls that GLES does not have.
#include <GLES/gl.h>
// Without this glext.h declares the extension enums but not their entry points,
// so glBindFramebufferOES and friends come out as undeclared identifiers.
#define GL_GLEXT_PROTOTYPES 1
#include <GLES/glext.h>
#else
#include <GL/glew.h>
#if defined(ORION_OSX)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include <SDL2/SDL.h>
#include <zlib.h>
#if !defined(ORION_GLES)
#include <FreeImage.h>
#endif

using namespace std;

#if defined(__clang__)
// Enable these incrementally to cleanup bad code
#pragma clang diagnostic ignored "-Wint-to-pointer-cast" // FIXME: CGLTextTexture
#pragma clang diagnostic ignored                                                                   \
    "-Wtautological-constant-out-of-range-compare" // FIXME: always true expression
#pragma clang diagnostic ignored                                                                   \
    "-Winconsistent-missing-override" // FIXME: OnCharPress, OnKeyDown, OnLeftMouse... etc.
#pragma clang diagnostic ignored                                                                   \
    "-Woverloaded-virtual" // FIXME: CGameItem::GetLightID, CTextContainer::Add, CJournal::Add etc.
#pragma clang diagnostic ignored "-Wlogical-op-parentheses" // FIXME!!!!!!!!!!!!!!!!!!!!!!!!
#pragma clang diagnostic ignored "-Wnull-conversion"        // FIXME: NULL to bool m_CanProcessAlpha
#pragma clang diagnostic ignored "-Wnull-arithmetic"        // FIXME: comparing NULL to non-pointer
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wshadow"  // FIXME: shadowing local variables
#pragma clang diagnostic ignored "-Wreorder" // FIXME: Initialization order in class fields
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wnon-pod-varargs" // FIXME: glshader
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wformat"               // %li
#pragma clang diagnostic ignored "-Wstring-plus-char"     // FIXME: PacketManager.cpp wtf
#pragma clang diagnostic ignored "-Wmultichar"            // FIXME: 'ENU'
#pragma clang diagnostic ignored "-Wchar-subscripts"      // FIXME: [' ']
#pragma clang diagnostic ignored "-Wc++11-narrowing"      // FIXME: ID_BGS_BUTTON_*
#pragma clang diagnostic ignored "-Wunused-private-field" // FIXME: m_FakeInsertionPin
#pragma clang diagnostic ignored "-Wcomment"
#elif defined(__GNUC__)
// GCC warnings
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wmultichar"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wformat="
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wconversion-null"
#pragma GCC diagnostic ignored "-Wmultichar"
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

typedef int SOCKET;
typedef uint16_t WORD;
#if defined(ORION_GLES)
// On desktop POSIX builds FreeImage.h supplies this. FreeImage is only used by
// ScreenshotBuilder, so it is not built for Android, and the type has to come
// from somewhere.
typedef uint32_t DWORD;
typedef int BOOL;
#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif
#endif
typedef uintptr_t LPARAM;
typedef uintptr_t LRESULT;
typedef uintptr_t WPARAM;
typedef int32_t HRESULT;
typedef int32_t LONG;
typedef unsigned char BYTE;
typedef unsigned char *PBYTE;
typedef void *PVOID;
typedef void *LPVOID;
typedef void *HANDLE;
typedef void *HWND;
typedef void *HICON;
typedef void *HBRUSH;
typedef void *HCURSOR;
typedef void *HINSTANCE;
typedef void *HDC;
typedef void *HMODULE;
typedef void *HSTREAM;
typedef void *HGLRC;
typedef const void *LPCVOID;
typedef size_t SIZE_T;
typedef const char *LPCSTR;
typedef const wchar_t *LPWSTR;
typedef const char *LPTSTR;
typedef const char *LPSTR;
typedef wchar_t WCHAR;
typedef unsigned int UINT;

#define S_OK 0x0L
#define WM_USER 0x0400
#define WM_MOUSEWHEEL 1
#define WM_MBUTTONUP 2
#define WM_MBUTTONDOWN 3
#define WM_RBUTTONUP 4
#define WM_RBUTTONDOWN 5
#define WM_LBUTTONUP 6
#define WM_LBUTTONDOWN 7
#define WM_CLOSE 10
#define WM_XBUTTONDOWN 11
#define WM_SETTEXT 13
#define WM_SHOWWINDOW 14
#define WM_NCACTIVATE 15
#define WM_SYSKEYUP 16
#define WM_KEYUP 17
#define WM_SYSKEYDOWN 18
#define WM_KEYDOWN 19
#define WM_MOUSEMOVE 21
#define WM_SYSCOMMAND 25
// Both used to be 0, so maximize and restore were indistinguishable.
#define SC_MAXIMIZE 0xF030
#define SC_RESTORE 0xF120

const unsigned int WM_NCPAINT = 0x85;
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1

// On this platform OnKey()/OnKeyDown() are handed SDL keycodes, so the VK_
// aliases must BE the SDL keycodes. They used to be invented sequential numbers
// matching neither Win32 nor SDL: VK_BACK was 32, which is SDLK_SPACE, and
// VK_UP was 8, which is SDLK_BACKSPACE - so space deleted a character, backspace
// moved the caret, and only Return/Tab/Escape worked, by coincidence.
#define VK_RETURN SDLK_RETURN
#define VK_ESCAPE SDLK_ESCAPE
#define VK_TAB SDLK_TAB
#define VK_SHIFT SDLK_LSHIFT
#define VK_CONTROL SDLK_LCTRL
#define VK_MENU SDLK_LALT
#define VK_RMENU SDLK_RALT
#define VK_LEFT SDLK_LEFT
#define VK_RIGHT SDLK_RIGHT
#define VK_DOWN SDLK_DOWN
#define VK_UP SDLK_UP
#define VK_END SDLK_END
#define VK_HOME SDLK_HOME
#define VK_NEXT SDLK_PAGEDOWN
#define VK_PRIOR SDLK_PAGEUP
#define VK_DELETE SDLK_DELETE
#define VK_BACK SDLK_BACKSPACE
#define VK_SPACE SDLK_SPACE
#define VK_CAPITAL SDLK_CAPSLOCK
#define VK_PAUSE SDLK_PAUSE
#define VK_SCROLL SDLK_SCROLLLOCK
#define VK_F1 SDLK_F1
#define VK_F2 SDLK_F2
#define VK_F3 SDLK_F3
#define VK_F4 SDLK_F4
#define VK_F5 SDLK_F5
#define VK_F6 SDLK_F6
#define VK_F7 SDLK_F7
#define VK_F8 SDLK_F8
#define VK_F9 SDLK_F9
#define VK_F10 SDLK_F10
#define VK_F11 SDLK_F11
#define VK_F12 SDLK_F12
#define VK_NUMPAD0 SDLK_KP_0
#define VK_NUMPAD1 SDLK_KP_1
#define VK_NUMPAD2 SDLK_KP_2
#define VK_NUMPAD3 SDLK_KP_3
#define VK_NUMPAD4 SDLK_KP_4
#define VK_NUMPAD5 SDLK_KP_5
#define VK_NUMPAD6 SDLK_KP_6
#define VK_NUMPAD7 SDLK_KP_7
#define VK_NUMPAD8 SDLK_KP_8
#define VK_NUMPAD9 SDLK_KP_9

#define PM_REMOVE 0x0001
#define MK_MBUTTON 0
#define MK_RBUTTON 1
#define MK_LBUTTON 2

#define GENERIC_READ 0
#define OPEN_EXISTING 0
#define FILE_ATTRIBUTE_NORMAL 0
#define FILE_MAP_READ 0
#define CP_UTF8 0
#define CALLBACK
#define HWND_TOP 0
#define GWL_STYLE 0
#define GWL_EXSTYLE 0
#define CS_HREDRAW 0
#define CS_OWNDC 0
#define CS_VREDRAW 0
#define WS_OVERLAPPEDWINDOW 0
#define WS_EX_WINDOWEDGE 0
// These previously all aliased to 0, i.e. to SM_CXSCREEN, so every frame-metric
// query silently returned the screen width.
#define SM_CYFRAME 2
#define SM_CYCAPTION 3
#define SM_CXSIZEFRAME 4
#define COLOR_WINDOW 0
#define SW_SHOWNORMAL 0
#define IDI_ORIONUO 0
#define IDC_CURSOR1 1
#define MAKEINTRESOURCE(x) x
// CF_TEXT and CF_UNICODETEXT both used to be 0, so the clipboard could not
// tell an ANSI request from a wide one.
#define CF_UNICODETEXT 2

#define LANG_RUSSIAN 0
#define LANG_FRENCH 1
#define LANG_GERMAN 2
#define LANG_SPANISH 3
#define LANG_JAPANESE 4
#define LANG_KOREAN 5
// Not a real Win32 LANGID; just a value outside the mapped set so the
// GetCurrentLocale switch falls through to its English default.
#define LANG_ENGLISH 6

#define CF_TEXT 1

#define MAX_PATH 256

struct SYSTEMTIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
};
struct RECT
{
    int left;
    int top;
    int right;
    int bottom;
};

// Registered by CWindow::Create so the Win32 window shims below can act on the
// real SDL window; the HWND they are handed is always null off Windows.
void SetStubWindow(SDL_Window *window);

// Bad and very ugly "API" stuff
bool GetWindowRect(void *, RECT *);
bool SetWindowPos(void *, void *, int, int, int, int, int);
int GetSystemMetrics(int);
int DefWindowProc(void *, unsigned int, uintptr_t, uintptr_t);
bool SendMessage(void *, int, int, int);
void PostMessage(void *, int, int, int);
#define LOBYTE(x) (int)(x & 0xff)
int GetSystemDefaultLangID();
int GetProfileStringA(const char *, const char *, const char *, char *, int);
void *GlobalLock(void *);
bool GlobalUnlock(void *);

// cmd line
// Captured from main(); the Orion Launcher passes the shard address as
// "-login host,port", so without this the client has no server to reach.
void SetStubCommandLine(int argc, char **argv);
wchar_t *GetCommandLineW();
const wchar_t **CommandLineToArgvW(wchar_t *, int *);
void *ShellExecuteA(void *, const char *, const char *, const char *, const char *, int);
void *LocalFree(void *p);

// Input
bool OpenClipboard(void *);
void *GetClipboardData(unsigned);
bool CloseClipboard();

// Thread
void CloseHandle(void *); // WispThread.cpp
void KillTimer(void *, unsigned int);
void SetTimer(void *, unsigned int, unsigned int, void *);
int timeBeginPeriod(int);
void *_beginthreadex(void *, unsigned, unsigned (*)(void *), void *, unsigned, unsigned *);
void _endthreadex(int);
int timeEndPeriod(int);
void GetLocalTime(SYSTEMTIME *);

// Socket
struct WSADATA
{
    WORD wVersion;
    WORD wHighVersion;
    char szDescription[256];
    char szSystemStatus[256];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char *lpVendorInfo;
};
bool WSAStartup(int, void *);
void WSASetLastError(int);
int WSACleanup(void);
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define MAKEWORD(x, y) (int)(x)
typedef struct hostent HOSTENT;
typedef HOSTENT *LPHOSTENT;
#define SOCKADDR struct sockaddr
// Win32 spells this sockaddr_in. Aliasing it to in_addr made
// sizeof(SOCKADDR_IN) four bytes, which is passed to sendto() as the
// address length and makes every ICMP ping fail.
#define SOCKADDR_IN struct sockaddr_in
#define LPIN_ADDR struct in_addr *
#define LPSOCKADDR const SOCKADDR *

// String
// MSVC spells this localtime_s(tm *, const time_t *); POSIX spells it
// localtime_r(const time_t *, tm *), with the arguments the other way round.
inline int localtime_s(struct tm *result, const time_t *timer)
{
    return (result == nullptr || timer == nullptr || localtime_r(timer, result) == nullptr) ? -1 : 0;
}

#define strncpy_s strncpy
#define lstrlenW wcslen
#define sprintf_s sprintf
#define sscanf_s sscanf
#define vsprintf_s vsprintf
#define vswprintf_s(a, b, c) vswprintf(a, 0, b, c)
int WideCharToMultiByte(int, int, const wchar_t *, int, char *, int, void *, void *);
int MultiByteToWideChar(int, int, const char *, int, wchar_t *, int);

// http://en.cppreference.com/w/cpp/locale/codecvt_utf8
inline int _wtoi(const wchar_t *a)
{
    return std::stoi(wstring(a));
}
char *_strlwr(char *s);
char *_strupr(char *s);
wchar_t *_wcslwr(wchar_t *s);
wchar_t *_wcsupr(wchar_t *s);

// BASS
#define BASS_OK 0
#define BASS_ERROR_FILEOPEN 1
#define BASS_ERROR_DRIVER 2
#define BASS_ERROR_BUFLOST 3
#define BASS_ERROR_HANDLE 4
#define BASS_ERROR_FORMAT 5
#define BASS_ERROR_POSITION 6
#define BASS_ERROR_INIT 7
#define BASS_ERROR_START 8
#define BASS_ERROR_SSL 9
#define BASS_ERROR_ALREADY 10
#define BASS_ERROR_NOCHAN 11
#define BASS_ERROR_ILLTYPE 12
#define BASS_ERROR_ILLPARAM 13
#define BASS_ERROR_NO3D 14
#define BASS_ERROR_NOEAX 15
#define BASS_ERROR_DEVICE 16
#define BASS_ERROR_NOPLAY 17
#define BASS_ERROR_UNKNOWN 18
#define BASS_ERROR_BUSY 19
#define BASS_ERROR_ENDED 19
#define BASS_ERROR_CODEC 19
#define BASS_ERROR_VERSION 19
#define BASS_ERROR_SPEAKER 19
#define BASS_ERROR_FILEFORM 19
#define BASS_ERROR_TIMEOUT 19
#define BASS_ERROR_DX 19
#define BASS_ERROR_DECODE 19
#define BASS_ERROR_NOTAVAIL 19
#define BASS_ERROR_NOFX 19
#define BASS_ERROR_CREATE 19
#define BASS_ERROR_NONET 19
#define BASS_ERROR_NOHW 19
#define BASS_ERROR_EMPTY 19
#define BASS_ERROR_NOTFILE 19
#define BASS_ERROR_FREQ 19
#define BASS_ERROR_MEM 19
#define BASS_ErrorGetCode() 0
// Audio. These used to expand to no-ops, so a non-Windows build was completely
// silent. They are now backed by SDL_mixer in Managers/SoundBackend.cpp, keeping
// the BASS names so the CSoundManager logic above is unchanged.
//
// The flag values match real BASS so the constants stay meaningful; note that
// BASS_SAMPLE_LOOP in particular must be a distinct bit, because it is how the
// caller asks for looping music.
#define BASS_SAMPLE_LOOP 4
#define BASS_SAMPLE_3D 8
#define BASS_SAMPLE_SOFTWARE 16
#define BASS_SAMPLE_FLOAT 256
#define BASS_ATTRIB_VOL 2
#define BASS_MIDI_DECAYEND 0x400
#define BASS_DEVICE_3D 4
#define BASS_3DALG_FULL 1
#define BASS_CONFIG_3DALGORITHM 21
#define BASS_CONFIG_SRC 43
#define BASS_CONFIG_MIDI_DEFFONT 0x10403
#define MAXERRORLENGTH 64
#define mciGetErrorString(a, b, c) false

bool BASS_Init(int device, int frequency, int flags, void *window, void *guid);
void BASS_Free();
void BASS_Start();
void BASS_Pause();
float BASS_GetVolume();
bool BASS_SetConfig(int option, int value);
bool BASS_SetConfigPtr(int option, const char *value);
HSTREAM
BASS_StreamCreateFile(bool fromMemory, const void *file, uint64_t offset, uint64_t length, uint32_t flags);
HSTREAM BASS_MIDI_StreamCreateFile(
    bool fromMemory, const void *file, uint64_t offset, uint64_t length, uint32_t flags, uint32_t frequency);
bool BASS_ChannelPlay(HSTREAM handle, bool loop);
void BASS_ChannelStop(HSTREAM handle);
bool BASS_ChannelIsActive(HSTREAM handle);
void BASS_ChannelSetAttribute(HSTREAM handle, int attribute, float value);
bool BASS_StreamFree(HSTREAM handle);

#endif

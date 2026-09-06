#include "Stubs.h"

#if !defined(ORION_WINDOWS)

#include <cassert>
#include <cwctype>
#include <errno.h>
#include <codecvt>
#include <locale>
#include <string>
#include <sys/time.h>
#include <map>
#include <vector>
#include <time.h>

namespace
{
// Two-letter ISO 639-1 code for the user's preferred locale, lowercased.
// Empty when it cannot be determined.
std::string PreferredLanguage()
{
    std::string code;

#if SDL_VERSION_ATLEAST(2, 0, 14)
    if (SDL_Locale *locales = SDL_GetPreferredLocales())
    {
        if (locales[0].language != nullptr)
            code = locales[0].language;
        SDL_free(locales);
    }
#endif

    if (code.empty())
    {
        const char *env = getenv("LANG");
        if (env == nullptr)
            env = getenv("LC_ALL");
        if (env != nullptr)
            for (int i = 0; i < 2 && env[i] != 0 && env[i] != '_' && env[i] != '.'; i++)
                code.push_back(env[i]);
    }

    for (auto &c : code)
        c = (char)tolower((unsigned char)c);

    return code;
}

// Win32 clipboard emulation over SDL. Callers follow the Win32 dance
// (Open -> GetClipboardData -> GlobalLock -> ... -> GlobalUnlock -> Close), so
// keep that shape and back it with a snapshot of the SDL clipboard taken at
// Open time.
std::string g_ClipboardAnsi;
std::wstring g_ClipboardWide;
bool g_ClipboardOpen = false;

// The real window behind the null HWNDs the Win32 shims are handed.
SDL_Window *g_Window = nullptr;

// Wide copies of argv, kept alive for CommandLineToArgvW.
std::vector<std::wstring> g_Argv;
std::wstring g_CommandLine;
} // namespace

void SetStubCommandLine(int argc, char **argv)
{
    g_Argv.clear();
    g_CommandLine.clear();

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    for (int i = 0; i < argc; i++)
    {
        if (argv[i] == nullptr)
            continue;

        try
        {
            g_Argv.push_back(converter.from_bytes(argv[i]));
        }
        catch (const std::range_error &)
        {
            continue;
        }

        if (!g_CommandLine.empty())
            g_CommandLine += L' ';
        g_CommandLine += g_Argv.back();
    }
}

void SetStubWindow(SDL_Window *window)
{
    g_Window = window;
}

// Bad and very ugly "API" stuff
int GetSystemMetrics(int index)
{
    // Window frame metrics are Win32 concepts; SDL sizes windows by their client
    // area, so the frame contributions are zero here.
    switch (index)
    {
        case SM_CYFRAME:
        case SM_CYCAPTION:
        case SM_CXSIZEFRAME:
            return 0;
        default:
            break;
    }

    const bool videoReady = SDL_WasInit(SDL_INIT_VIDEO) != 0;
    if (!videoReady && SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        return 0;

    SDL_DisplayMode mode;
    const bool ok = SDL_GetCurrentDisplayMode(0, &mode) == 0;

    if (!videoReady)
        SDL_QuitSubSystem(SDL_INIT_VIDEO);

    if (!ok)
        return 0;

    switch (index)
    {
        case SM_CXSCREEN:
            return mode.w;
        case SM_CYSCREEN:
            return mode.h;
        default:
            return 0;
    }
}
int DefWindowProc(void *, unsigned int, uintptr_t, uintptr_t)
{
    NOT_IMPLEMENTED;
    return 0;
}
int WideCharToMultiByte(int, int, const wchar_t *, int, char *, int, void *, void *)
{
    NOT_IMPLEMENTED;
    return 0;
}
int MultiByteToWideChar(int, int, const char *, int, wchar_t *, int)
{
    NOT_IMPLEMENTED;
    return 0;
}
bool GetWindowRect(void *, RECT *rect)
{
    if (rect == nullptr || g_Window == nullptr)
        return false;

    int x = 0, y = 0, width = 0, height = 0;
    SDL_GetWindowPosition(g_Window, &x, &y);
    SDL_GetWindowSize(g_Window, &width, &height);

    rect->left = x;
    rect->top = y;
    rect->right = x + width;
    rect->bottom = y + height;
    return true;
}
bool SetWindowPos(void *, void *, int x, int y, int width, int height, int)
{
    if (g_Window == nullptr)
        return false;

    if (width > 0 && height > 0)
        SDL_SetWindowSize(g_Window, width, height);

    SDL_SetWindowPosition(g_Window, x, y);
    return true;
}
bool AdjustWindowRectEx(RECT *, int, bool, int)
{
    NOT_IMPLEMENTED;
    return false;
}
bool SendMessage(void *, int message, int wParam, int)
{
    if (g_Window == nullptr)
        return false;

    // Only the window-state commands mean anything here; the rest of the
    // WM_/UOMSG_ traffic belongs to the Windows-only plugin interface.
    if (message != WM_SYSCOMMAND)
        return false;

    switch (wParam)
    {
        case SC_MAXIMIZE:
            SDL_MaximizeWindow(g_Window);
            return true;
        case SC_RESTORE:
            SDL_RestoreWindow(g_Window);
            return true;
        default:
            return false;
    }
}
void PostMessage(void *, int message, int, int)
{
    if (message != WM_CLOSE)
        return;

    // Route a close request through the normal event loop.
    SDL_Event quit = {};
    quit.type = SDL_QUIT;
    SDL_PushEvent(&quit);
}
bool OpenClipboard(void *)
{
    if (g_ClipboardOpen)
        return false;

    char *text = SDL_GetClipboardText();
    if (text == nullptr)
        return false;

    g_ClipboardAnsi = text;
    SDL_free(text);

    try
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        g_ClipboardWide = converter.from_bytes(g_ClipboardAnsi);
    }
    catch (const std::range_error &)
    {
        // Clipboard held bytes that are not valid UTF-8; treat it as empty
        // rather than letting the exception escape into the UI code.
        g_ClipboardWide.clear();
    }

    g_ClipboardOpen = true;
    return true;
}
void *GetClipboardData(unsigned format)
{
    if (!g_ClipboardOpen)
        return nullptr;

    if (format == CF_UNICODETEXT)
        return g_ClipboardWide.empty() ? nullptr : (void *)g_ClipboardWide.c_str();

    if (format == CF_TEXT)
        return g_ClipboardAnsi.empty() ? nullptr : (void *)g_ClipboardAnsi.c_str();

    return nullptr;
}
bool CloseClipboard()
{
    g_ClipboardOpen = false;
    g_ClipboardAnsi.clear();
    g_ClipboardWide.clear();
    return true;
}
wchar_t *GetCommandLineW()
{
    // Only ever handed straight back to CommandLineToArgvW below.
    return const_cast<wchar_t *>(g_CommandLine.c_str());
}
const wchar_t **CommandLineToArgvW(wchar_t *, int *argc)
{
    if (argc != nullptr)
        *argc = 0;

    if (g_Argv.empty())
        return nullptr;

    // The caller releases this with LocalFree, which is plain free() here, so
    // the array itself must be a malloc'd block we hand over. The strings it
    // points at stay owned by g_Argv.
    auto **result = (const wchar_t **)malloc(g_Argv.size() * sizeof(const wchar_t *));
    if (result == nullptr)
        return nullptr;

    for (size_t i = 0; i < g_Argv.size(); i++)
        result[i] = g_Argv[i].c_str();

    if (argc != nullptr)
        *argc = (int)g_Argv.size();

    return result;
}
int GetSystemDefaultLangID()
{
    const std::string lang = PreferredLanguage();

    if (lang == "ru")
        return LANG_RUSSIAN;
    if (lang == "fr")
        return LANG_FRENCH;
    if (lang == "de")
        return LANG_GERMAN;
    if (lang == "es")
        return LANG_SPANISH;
    if (lang == "ja")
        return LANG_JAPANESE;
    if (lang == "ko")
        return LANG_KOREAN;

    return LANG_ENGLISH;
}
void *ShellExecuteA(void *, const char *, const char *file, const char *, const char *, int)
{
#if SDL_VERSION_ATLEAST(2, 0, 14)
    if (file != nullptr)
        SDL_OpenURL(file);
#else
    UNUSED(file);
    NOT_IMPLEMENTED;
#endif
    return nullptr;
}
void *LocalFree(void *p)
{
    free(p); /*wtf*/
    return nullptr;
}
void *GlobalLock(void *handle)
{
    // The clipboard snapshot is already a plain pointer; nothing to lock.
    return handle;
}
bool GlobalUnlock(void *)
{
    return true;
}
int GetProfileStringA(
    const char *section, const char *key, const char *, char *out, int size)
{
    // Only ever queried for the user's language, as intl/sLanguage.
    if (out == nullptr || size <= 0 || section == nullptr || key == nullptr)
        return 0;

    if (SDL_strcasecmp(section, "intl") != 0 || SDL_strcasecmp(key, "sLanguage") != 0)
        return 0;

    static const struct
    {
        const char *iso;
        const char *abbreviature;
    } languages[] = {
        { "en", "enu" }, { "ru", "rus" }, { "fr", "fra" }, { "de", "deu" }, { "es", "esp" },
        { "ja", "jpn" }, { "ko", "kor" }, { "it", "ita" }, { "pt", "ptb" }, { "zh", "chs" },
    };

    const std::string lang = PreferredLanguage();
    for (const auto &entry : languages)
    {
        if (lang != entry.iso)
            continue;

        const int length = (int)strlen(entry.abbreviature);
        if (length >= size)
            return 0;

        memcpy(out, entry.abbreviature, length);
        out[length] = 0;
        return length;
    }

    // Unknown locale: report failure so the caller falls back to its own default.
    return 0;
}

// Thread
void CloseHandle(void *)
{
    NOT_IMPLEMENTED;
}
namespace
{
// SDL timer callbacks run on their own thread, so post the tick through the
// event queue and let the main loop dispatch it.
Uint32 StubTimerCallback(Uint32 interval, void *param)
{
    SDL_Event ev = {};
    ev.type = SDL_USEREVENT;
    ev.user.code = (int)(intptr_t)param;
    SDL_PushEvent(&ev);
    return interval;
}

std::map<unsigned int, SDL_TimerID> g_Timers;
} // namespace

void KillTimer(void *, unsigned int id)
{
    const auto it = g_Timers.find(id);
    if (it == g_Timers.end())
        return;

    SDL_RemoveTimer(it->second);
    g_Timers.erase(it);
}

void SetTimer(void *, unsigned int id, unsigned int delay, void *)
{
    KillTimer(nullptr, id);

    const SDL_TimerID handle = SDL_AddTimer(delay, StubTimerCallback, (void *)(intptr_t)id);
    if (handle == 0)
    {
        LOG("SDL_AddTimer failed: %s\n", SDL_GetError());
        return;
    }

    g_Timers[id] = handle;
}
int timeBeginPeriod(int)
{
    NOT_IMPLEMENTED;
    return 0;
}
void *_beginthreadex(void *, unsigned, unsigned (*)(void *), void *, unsigned, unsigned *)
{
    NOT_IMPLEMENTED;
    return nullptr;
}
void _endthreadex(int)
{
    NOT_IMPLEMENTED;
}
int timeEndPeriod(int)
{
    NOT_IMPLEMENTED;
    return 0;
}
void GetLocalTime(SYSTEMTIME *st)
{
    if (st == nullptr)
        return;

    timeval tv = { 0, 0 };
    gettimeofday(&tv, nullptr);

    const time_t seconds = (time_t)tv.tv_sec;
    tm local = {};
    localtime_r(&seconds, &local);

    st->wYear = (WORD)(local.tm_year + 1900);
    st->wMonth = (WORD)(local.tm_mon + 1);
    st->wDayOfWeek = (WORD)local.tm_wday;
    st->wDay = (WORD)local.tm_mday;
    st->wHour = (WORD)local.tm_hour;
    st->wMinute = (WORD)local.tm_min;
    st->wSecond = (WORD)local.tm_sec;
    st->wMilliseconds = (WORD)(tv.tv_usec / 1000);
}

// Socket
bool WSAStartup(int, void *)
{
    NOT_IMPLEMENTED;
    return false;
}
void WSASetLastError(int)
{
    NOT_IMPLEMENTED;
}
int WSACleanup(void)
{
    NOT_IMPLEMENTED;
    return 0;
}

char *_strlwr(char *s)
{
    char *tmp = s;

    for (; *tmp; ++tmp)
    {
        *tmp = tolower((unsigned char)*tmp);
    }

    return s;
}

char *_strupr(char *s)
{
    char *tmp = s;

    for (; *tmp; ++tmp)
    {
        *tmp = toupper((unsigned char)*tmp);
    }

    return s;
}

wchar_t *_wcslwr(wchar_t *s)
{
    wchar_t *tmp = s;

    for (; *tmp; ++tmp)
    {
        *tmp = towlower(*tmp);
    }

    return s;
}

wchar_t *_wcsupr(wchar_t *s)
{
    wchar_t *tmp = s;

    for (; *tmp; ++tmp)
    {
        *tmp = towupper(*tmp);
    }

    return s;
}

#endif

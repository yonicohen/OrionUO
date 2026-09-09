// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//----------------------------------------------------------------------------------
#include "stdafx.h"
#include "FileSystem.h"
#include "WispThread.h"
#include <SDL_timer.h>
namespace WISP_APPLICATION
{
CApplication *g_WispApplication = nullptr;
//----------------------------------------------------------------------------------
CApplication::CApplication()
{
    LOG("INITIATING CAPPLICATION\n");
    g_MainThread = CThread::GetCurrentThreadId();
    WISPFUN_DEBUG("c1_f1");
    g_WispApplication = this;
#if defined(__ANDROID__)
    // Android has no useful working directory - the process starts at '/' - so
    // fall back to app-specific external storage. It needs no runtime permission
    // and is where 'adb push' can place the UO data, which cannot be bundled in
    // the APK because it is copyright and about 2.6 GB.
    const char *androidStorage = SDL_AndroidGetExternalStoragePath();
    if (androidStorage != nullptr)
        m_UOPath = m_ExePath = os_path(androidStorage);
    else
        m_UOPath = m_ExePath = fs_path_current();
#else
    m_UOPath = m_ExePath = fs_path_current();
#endif
    g_MainScreen.LoadCustomPath();
}
//----------------------------------------------------------------------------------
CApplication::~CApplication()
{
    WISPFUN_DEBUG("c1_f2");
    g_WispApplication = nullptr;
    Hinstance = 0;
}
//----------------------------------------------------------------------------------
int CApplication::Run(HINSTANCE hinstance)
{
    // WISPFUN_DEBUG("c1_f3");
#if USE_WISP
    timeBeginPeriod(1);
    Hinstance = hinstance;

    MSG msg = { 0 };

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
            SDL_Delay(1);

        OnMainLoop();
    }

    timeEndPeriod(1);

    return (int)msg.wParam;
#else
    // Mirror the Windows loop above: drain input, yield when idle, and run
    // OnMainLoop every iteration. It used to run only when an SDL event arrived,
    // which meant g_ConnectionManager.Recv() and g_Orion.Process() only advanced
    // while the user happened to be moving the mouse or typing - and the loop
    // span at 100% CPU whenever they were not.
    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        bool hadEvent = false;

        while (!quit && SDL_PollEvent(&event))
        {
            hadEvent = true;
            quit = WISP_WINDOW::g_WispWindow->OnWindowProc(event);
        }

        if (quit)
            break;

        if (!hadEvent)
            SDL_Delay(1);

        OnMainLoop();
    }

    return EXIT_SUCCESS;
#endif
}
//---------------------------------------------------------------------------
// Orion client release this port tracks. Must match what the shard expects;
// Latest release per the vendor's own update manifest
// (orionuo.online/Updates5152/BackupsList64.html). Shards that demand the
// latest client compare against this.
#define ORION_VERSION_MAJOR 1
#define ORION_VERSION_MINOR 0
#define ORION_VERSION_REVISION 37
#define ORION_VERSION_BUILD 0
#define ORION_VERSION_STRING "1.0.37.0"
//---------------------------------------------------------------------------
string CApplication::GetFileVersion(uint *numericVerion) const
{
#if USE_WISP
    //File version info
    wchar_t szFilename[MAX_PATH] = { 0 };

    if (GetModuleFileName(Hinstance, &szFilename[0], sizeof(szFilename)))
    {
        DWORD dummy = 0;
        DWORD dwSize = GetFileVersionInfoSize(&szFilename[0], &dummy);

        if (dwSize > 0)
        {
            UCHAR_LIST lpVersionInfo(dwSize, 0);

            if (GetFileVersionInfoW(&szFilename[0], NULL, dwSize, &lpVersionInfo[0]))
            {
                UINT uLen = 0;
                VS_FIXEDFILEINFO *lpFfi = NULL;

                VerQueryValueW(&lpVersionInfo[0], PATH_SEP, (LPVOID *)&lpFfi, &uLen);

                DWORD dwFileVersionMS = 0;
                DWORD dwFileVersionLS = 0;

                if (lpFfi != NULL)
                {
                    dwFileVersionMS = lpFfi->dwFileVersionMS;
                    dwFileVersionLS = lpFfi->dwFileVersionLS;
                }

                int dwLeftMost = (int)HIWORD(dwFileVersionMS);
                int dwSecondLeft = (int)LOWORD(dwFileVersionMS);
                int dwSecondRight = (int)HIWORD(dwFileVersionLS);
                int dwRightMost = (int)LOWORD(dwFileVersionLS);

                if (numericVerion != NULL)
                    *numericVerion = ((dwLeftMost & 0xFF) << 24) | ((dwSecondLeft & 0xFF) << 16) |
                                     ((dwSecondRight & 0xFF) << 8) | (dwRightMost & 0xFF);

                char fileVersion[100] = { 0 };
                sprintf_s(
                    fileVersion,
                    "%i.%i.%i.%i",
                    dwLeftMost,
                    dwSecondLeft,
                    dwSecondRight,
                    dwRightMost);

                return fileVersion;
            }
        }
    }

    return "unknown";
#else
    // There is no PE version resource to read off Windows, and the numeric
    // version was previously left at 0. Servers check it: this shard replies
    // "This server requires the latest ORION version" and disconnects after a
    // few seconds in world. Report the Orion release this port corresponds to.
    if (numericVerion != nullptr)
        *numericVerion = ((ORION_VERSION_MAJOR & 0xFF) << 24) |
                         ((ORION_VERSION_MINOR & 0xFF) << 16) |
                         ((ORION_VERSION_REVISION & 0xFF) << 8) | (ORION_VERSION_BUILD & 0xFF);

    return ORION_VERSION_STRING;
#endif
}
//---------------------------------------------------------------------------
os_path CApplication::ExeFilePath(const char *str, ...) const
{
    WISPFUN_DEBUG("c1_f4");
    va_list arg;
    va_start(arg, str);

    char out[MAX_PATH] = { 0 };
    vsprintf_s(out, str, arg);

    va_end(arg);

    os_path res{ m_ExePath.c_str() };
    res.append(PATH_SEP);
    res.append(ToPath(out));
    return res;
}
//---------------------------------------------------------------------------
os_path CApplication::UOFilesPath(const string &str, ...) const
{
    return UOFilesPath(str.c_str());
}
//---------------------------------------------------------------------------
os_path CApplication::UOFilesPath(const char *str, ...) const
{
    WISPFUN_DEBUG("c1_f6");
    va_list arg;
    va_start(arg, str);

    char out[MAX_PATH] = { 0 };
    vsprintf_s(out, str, arg);

    va_end(arg);

    os_path res{ m_UOPath.c_str() };
    res.append(PATH_SEP);
    res.append(ToPath(out));
    return res;
}
//----------------------------------------------------------------------------------
}; // namespace WISP_APPLICATION
//----------------------------------------------------------------------------------

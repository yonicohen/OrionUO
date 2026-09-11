#include "stdafx.h"
#include <exception>

#include "FileSystem.h"
#include <SDL.h>

#if USE_WISP

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    WISPFUN_DEBUG("c_main");
    INITLOGGER(L"uolog.txt");

    //ParseCommandLine(); // FIXME
#if defined(__ANDROID__)
    // COrionWindow translates finger gestures itself - a press and hold has to
    // become the right button, which SDL's own synthesis cannot express - so
    // its left-button-only version of the same touches is turned off rather
    // than arriving alongside.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif

    if (SDL_Init(SDL_INIT_TIMER) < 0)
    {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    auto path = g_App.ExeFilePath("crashlogs");
    fs_path_create(path);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char buf[100] = { 0 };

    sprintf_s(
        buf,
        "/crash_%i_%i_%i___%i_%i_%i_%i.txt",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds);

    path += ToPath(buf);

    INITCRASHLOGGER(path);

    socket_init();
    g_OrionWindow.hInstance = hInstance;
    if (!g_OrionWindow.Create("Orion UO Client", "Ultima Online", true, 640, 480))
    {
        socket_shutdown();
        return 0;
    }

    g_OrionWindow.ShowWindow(true);
    g_OrionWindow.NoResize = true;

    g_Orion.LoadPluginConfig();

    auto r = g_App.Run(hInstance);
    socket_shutdown();
    SDL_Quit();
    return r;
}

#else

#if USE_ORIONDLL
ENCRYPTION_TYPE g_EncryptionType;
#endif

static bool g_isHeadless = false;
extern ENCRYPTION_TYPE g_EncryptionType;

extern COrionWindow g_OrionWindow;

// Uncaught exceptions abort the process with nothing but 'terminating due to
// uncaught exception' on the terminal, which tells a player nothing and tells us
// less. Log what was thrown first; the log is line buffered, so it survives the
// abort that follows.
static void OnTerminate()
{
    if (std::exception_ptr current = std::current_exception())
    {
        try
        {
            std::rethrow_exception(current);
        }
        catch (const std::exception &e)
        {
            LOG("FATAL: uncaught exception: %s\n", e.what());
        }
        catch (...)
        {
            LOG("FATAL: uncaught exception of unknown type\n");
        }
    }
    else
        LOG("FATAL: terminate called without an active exception\n");

    fflush(stdout);
    abort();
}
//----------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    WISPFUN_DEBUG();
    std::set_terminate(OnTerminate);

    // LOG() is fprintf(stdout, ...) here. Redirected to a file, stdout is block
    // buffered, so up to 8 KB of output - which can be minutes of a session, and
    // is exactly the part you want when something goes wrong - sits unwritten
    // until the next flush. Line buffering makes 'tail -f' on the log tell the
    // truth, and keeps the log useful if the process is killed rather than
    // exiting cleanly.
    setvbuf(stdout, nullptr, _IOLBF, 0);

    SetStubCommandLine(argc, argv);

    // TODO: good cli parsing api
    // keep this simple for now just for travis-ci
    for (int i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "--headless"))
            g_isHeadless = true;
        else if (!strcmp(argv[i], "--nocrypt"))
            g_EncryptionType = ET_NOCRYPT;
        else if (!strcmp(argv[i], "--noupscale"))
            g_UpscaleArt = false;
        else if (!strcmp(argv[i], "--smoothfilter"))
            g_SharpFilter = false;
        else if (!strcmp(argv[i], "--novsync"))
            g_UseVSync = false;
        else if (!strcmp(argv[i], "--gridcontainers"))
            g_ForceGridContainers = true;
        else if (!strcmp(argv[i], "--gridbg") && i + 1 < argc)
            g_GridContainerBackground = (ushort)strtol(argv[++i], nullptr, 0);
    }

    if (SDL_Init(SDL_INIT_TIMER) < 0)
    {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Log("SDL Initialized.");

#if defined(__ANDROID__)
    // Android starts the process at '/', so the working directory CApplication
    // picked up is useless. The data lives in app storage instead.
    //
    // External storage is the normal place - it is where 'adb push' can write and
    // it has room for the ~2.6 GB of UO data, which cannot ship in the APK. But it
    // can be unmounted or unavailable, so fall back to internal storage, and
    // prefer whichever actually holds Client.cuo.
    //
    // This has to happen here rather than in CApplication's constructor: that
    // runs while dlopen maps the library, before SDLActivity.nativeSetupJNI()
    // has cached its method IDs, and calling SDL's JNI glue that early aborts
    // the process with 'mid == null'.
    {
        const char *candidates[2] = { SDL_AndroidGetExternalStoragePath(),
                                      SDL_AndroidGetInternalStoragePath() };
        const char *chosen = nullptr;
        for (const char *candidate : candidates)
        {
            if (candidate == nullptr)
                continue;
            if (chosen == nullptr)
                chosen = candidate;
            if (fs_path_exists(os_path(candidate) + PATH_SEP + ToPath("Client.cuo")))
            {
                chosen = candidate;
                break;
            }
        }

        if (chosen != nullptr)
        {
            g_App.m_UOPath = g_App.m_ExePath = os_path(chosen);
            g_MainScreen.LoadCustomPath();

            // LOG() is fprintf(stdout, ...) in this build and Android discards
            // stdout, so point it at a file next to the data. Line buffering
            // keeps the log useful if the process is killed rather than exiting.
            const string logPath = StringFromPath(g_App.ExeFilePath("uolog.txt"));
            if (freopen(logPath.c_str(), "w", stdout) != nullptr)
            {
                setvbuf(stdout, nullptr, _IOLBF, 0);
                dup2(fileno(stdout), fileno(stderr));
            }
            LOG("Android data path: %s\n", chosen);
        }
    }
#endif

    INITLOGGER("uolog.txt");
    auto path = g_App.ExeFilePath("crashlogs");
    fs_path_create(path);

    // FIXME: log stuff
    /*
	SYSTEMTIME st;
	GetLocalTime(&st);

	char buf[100] = { 0 };

	sprintf_s(buf, "\\crash_%i_%i_%i___%i_%i_%i_%i.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	path += buf;

	INITCRASHLOGGER(path.c_str());
	*/

    if (!g_isHeadless)
    {
        if (!g_OrionWindow.Create("Orion UO Client", "Ultima Online", false, 640, 480))
        {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to create OrionUO client window. Fallbacking to headless mode.\n");
            g_isHeadless = true;
        }
    }

    // FIXME: headless: lets end here so we can run on travis for now
    if (g_isHeadless)
        return EXIT_SUCCESS;

    g_Orion.LoadPluginConfig();

    auto ret = g_App.Run(nullptr);
    SDL_Quit();
    return ret;
}

#endif

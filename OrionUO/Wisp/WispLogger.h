//----------------------------------------------------------------------------------
#ifndef WISPLOGGER_H
#define WISPLOGGER_H
//----------------------------------------------------------------------------------
#if defined(__ANDROID__)
#include <android/log.h>
#endif
//----------------------------------------------------------------------------------
// Declared out here, not inside the namespace below, so it is the same
// g_LogVerbose that Globals.cpp defines.
extern bool g_LogVerbose;
//----------------------------------------------------------------------------------
namespace WISP_LOGGER
{
//----------------------------------------------------------------------------------
#define CWISPLOGGER 1

#if USE_WISP
#if CWISPLOGGER != 0
#define INITLOGGER(path) WISP_LOGGER::g_WispLogger.Init(path);
#define LOG WISP_LOGGER::g_WispLogger.Print
#if CWISPLOGGER == 2
#define LOG_DUMP(...)
#else //CWISPLOGGER != 2
#define LOG_DUMP WISP_LOGGER::g_WispLogger.Dump
#endif //CWISPLOGGER == 2
#else  //CWISPLOGGER == 0
#define INITLOGGER(path)
#define LOG(...)
#define LOG_DUMP(...)
#endif //CWISPLOGGER!=0
#else
#if CWISPLOGGER
#define INITLOGGER(path)
#if defined(__ANDROID__)
// Android discards stdout, so a client built without Wisp would log nowhere at
// all. logcat is where every other diagnostic on the platform goes, and is what
// `adb logcat -s OrionUO` reads. The header is included above the namespace,
// so the names it declares stay at global scope.
#define LOG(...) ::__android_log_print(ANDROID_LOG_INFO, "OrionUO", __VA_ARGS__)
#else
#define LOG(...) fprintf(stdout, " LOG: " __VA_ARGS__)
#endif
#define LOG_DUMP(...)
#else //CWISPLOGGER == 0
#define INITLOGGER(path)
#define LOG(...)
#define LOG_DUMP(...)
#endif //CWISPLOGGER!=0
#endif

// Per-frame chatter - the socket poll, texture sweeps, packet sizes, the frame
// counter - drowns everything worth reading, and a session writes megabytes of
// it. Those calls say LOG_VERBOSE and are silent unless --verbose is passed.
#define LOG_VERBOSE(...)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (::g_LogVerbose)                                                                        \
            LOG(__VA_ARGS__);                                                                      \
    } while (0)

#define INITCRASHLOGGER(path) WISP_LOGGER::g_WispCrashLogger.Init(path);
#define CRASHLOG WISP_LOGGER::g_WispCrashLogger.Print
#define CRASHLOG_DUMP WISP_LOGGER::g_WispCrashLogger.Dump
//----------------------------------------------------------------------------------
class CLogger
{
public:
    os_path FileName;

protected:
    FILE *m_File{ nullptr };

public:
    CLogger();
    ~CLogger();

    void Close();

    bool Ready() const { return m_File != nullptr; }

    void Init(const os_path &filePath);

    void Print(const char *format, ...);
    void VPrint(const char *format, va_list ap);
    void Print(const wchar_t *format, ...);
    void VPrint(const wchar_t *format, va_list ap);
    void Dump(uchar *buf, int size);
};
//----------------------------------------------------------------------------------
extern CLogger g_WispLogger;
extern CLogger g_WispCrashLogger;
} // namespace WISP_LOGGER
//----------------------------------------------------------------------------------
#endif //WISPLOGGER_H
//----------------------------------------------------------------------------------

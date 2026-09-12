// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//----------------------------------------------------------------------------------
#include "stdafx.h"
#include "WispWindow.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_timer.h>
namespace WISP_WINDOW
{
CWindow *g_WispWindow = nullptr;
CTouchStick g_TouchStick;
#if defined(__ANDROID__)
static int SDLCALL AppLifecycleWatch(void *userdata, SDL_Event *event);
#endif
//---------------------------------------------------------------------------
#if USE_WISP
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WISPFUN_DEBUG("c_ww_wp");
    if (g_WispWindow != NULL)
        return g_WispWindow->OnWindowProc(hWnd, message, wParam, lParam);

    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif
//----------------------------------------------------------------------------------
CWindow::CWindow()
{
    WISPFUN_DEBUG("c14_f1");
    g_WispWindow = this;
}
//----------------------------------------------------------------------------------
CWindow::~CWindow()
{
}
//----------------------------------------------------------------------------------
float CWindow::GetPixelRatio() const
{
#if USE_WISP
    return 1.0f;
#else
    if (m_window == nullptr)
        return 1.0f;

    int pixelWidth = 0;
    int pixelHeight = 0;
    SDL_GL_GetDrawableSize(m_window, &pixelWidth, &pixelHeight);

    int pointWidth = 0;
    int pointHeight = 0;
    SDL_GetWindowSize(m_window, &pointWidth, &pointHeight);

    if (pointWidth <= 0 || pixelWidth <= 0)
        return 1.0f;

    return (float)pixelWidth / (float)pointWidth;
#endif
}
//----------------------------------------------------------------------------------
void CWindow::SetSize(const WISP_GEOMETRY::CSize &size)
{
#if USE_WISP
    WISPFUN_DEBUG("c14_f2");
    RECT pos = { 0, 0, 0, 0 };
    GetWindowRect(Handle, &pos);

    RECT r = { 0, 0, 0, 0 };
    r.right = size.Width;
    r.bottom = size.Height;
    AdjustWindowRectEx(
        &r, GetWindowLongA(Handle, GWL_STYLE), FALSE, GetWindowLongA(Handle, GWL_EXSTYLE));

    if (r.left < 0)
        r.right += -r.left;

    if (r.top < 0)
        r.bottom += -r.top;

    SetWindowPos(Handle, HWND_TOP, pos.left, pos.top, r.right, r.bottom, 0);
#else
    // SDL sizes windows by client area, so there is no frame to account for.
    //
    // Skip the call when nothing changes: every SDL_SetWindowSize is a visible,
    // animated resize on macOS, and the screen transitions ask for the same size
    // repeatedly.
    if (m_window != nullptr)
    {
        int currentWidth = 0;
        int currentHeight = 0;
        SDL_GetWindowSize(m_window, &currentWidth, &currentHeight);

        if (currentWidth != size.Width || currentHeight != size.Height)
        {
            LOG("Window resize: %dx%d -> %dx%d\n",
                currentWidth,
                currentHeight,
                size.Width,
                size.Height);
            SDL_SetWindowSize(m_window, size.Width, size.Height);
        }
    }
#endif
    m_Size = size;
}
//----------------------------------------------------------------------------------
void CWindow::SetMinSize(const WISP_GEOMETRY::CSize &newMinSize)
{
    WISPFUN_DEBUG("c14_f3");
#if USE_WISP
    if (m_Size.Width < newMinSize.Width || m_Size.Height < newMinSize.Height)
    {
        int width = m_Size.Width;
        int height = m_Size.Height;

        if (width < newMinSize.Width)
            width = newMinSize.Width;

        if (height < newMinSize.Height)
            height = newMinSize.Height;

        RECT pos = { 0, 0, 0, 0 };
        GetWindowRect(Handle, &pos);

        RECT r = { 0, 0, 0, 0 };
        r.right = width;
        r.bottom = height;
        AdjustWindowRectEx(
            &r, GetWindowLongA(Handle, GWL_STYLE), FALSE, GetWindowLongA(Handle, GWL_EXSTYLE));

        if (r.left < 0)
            r.right += -r.left;

        if (r.top < 0)
            r.bottom += -r.top;

        SetWindowPos(Handle, HWND_TOP, pos.left, pos.top, r.right, r.bottom, 0);
    }
#else
    // SDL_GetWindowPosition
    // SDL_GetWindowSize
    NOT_IMPLEMENTED;
#endif
    m_MinSize = newMinSize;
}
//----------------------------------------------------------------------------------
void CWindow::SetMaxSize(const WISP_GEOMETRY::CSize &newMaxSize)
{
    WISPFUN_DEBUG("c14_f4");
#if USE_WISP
    if (m_Size.Width > newMaxSize.Width || m_Size.Height > newMaxSize.Height)
    {
        int width = m_Size.Width;
        int height = m_Size.Height;

        if (width > newMaxSize.Width)
            width = newMaxSize.Width;

        if (height > newMaxSize.Height)
            height = newMaxSize.Height;

        RECT pos = { 0, 0, 0, 0 };
        GetWindowRect(Handle, &pos);

        RECT r = { 0, 0, 0, 0 };
        r.right = width;
        r.bottom = height;
        AdjustWindowRectEx(
            &r, GetWindowLongA(Handle, GWL_STYLE), FALSE, GetWindowLongA(Handle, GWL_EXSTYLE));

        if (r.left < 0)
            r.right += -r.left;

        if (r.top < 0)
            r.bottom += -r.top;

        SetWindowPos(Handle, HWND_TOP, pos.left, pos.top, r.right, r.bottom, 0);
    }
#else
    // SDL_GetWindowPosition
    // SDL_GetWindowSize
    NOT_IMPLEMENTED;
#endif
    m_MaxSize = newMaxSize;
}
//----------------------------------------------------------------------------------
bool CWindow::Create(
    const char *className, const char *title, bool showCursor, int width, int height)
{
    WISPFUN_DEBUG("c14_f5");

#if USE_WISP
    HICON icon = LoadIcon(g_OrionWindow.hInstance, MAKEINTRESOURCE(IDI_ORIONUO));
    HCURSOR cursor = LoadCursor(g_OrionWindow.hInstance, MAKEINTRESOURCE(IDC_CURSOR1));

    static wstring wclassName = ToWString(className);
    static wstring wtitle = ToWString(title);
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = cursor;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = wclassName.c_str();
    wcex.hIcon = icon;
    wcex.hIconSm = icon;

    RegisterClassEx(&wcex);

    width += 2 * GetSystemMetrics(SM_CXSIZEFRAME);
    height += GetSystemMetrics(SM_CYCAPTION) + (GetSystemMetrics(SM_CYFRAME) * 2);

    Handle = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        wclassName.c_str(),
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        width,
        height,
        NULL,
        NULL,
        hInstance,
        NULL);
    if (!Handle)
        return false;

    RECT r = { 0, 0, 0, 0 };
    r.right = width;
    r.bottom = height;
    AdjustWindowRectEx(
        &r, GetWindowLongA(Handle, GWL_STYLE), FALSE, GetWindowLongA(Handle, GWL_EXSTYLE));

    if (r.left < 0)
        r.right += -r.left;

    if (r.top < 0)
        r.bottom += -r.top;

    SetWindowPos(Handle, HWND_TOP, 0, 0, r.right, r.bottom, 0);

    srand(unsigned(time(NULL)));

    GetClientRect(Handle, &r);
    m_Size.Width = r.right - r.left;
    m_Size.Height = r.bottom - r.top;

    ::ShowCursor(showCursor);
    ::ShowWindow(Handle, FALSE);
    ::UpdateWindow(Handle);
#else
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        return false;

    m_Size.Width = width;
    m_Size.Height = height;

    // These must be set before the window is created. macOS in particular hands
    // back a context matching whatever was requested at window-creation time, so
    // setting them afterwards silently leaves you without a usable context.
    // The renderer is fixed-function GL 2.x (it even uses display lists), so ask
    // for the legacy/compatibility profile rather than core.
#if defined(ORION_GLES)
    // Android has no desktop GL. Ask for a GLES 1.1 context specifically: SDL
    // defaults to loading libGLESv2, which cannot give us the fixed function
    // pipeline the renderer is built on, and window creation then fails with
    // 'Could not initialize OpenGL / GLES library'.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
#if defined(ORION_GLES)
        // Android gives the app one fullscreen surface; it is not resizable and
        // the requested size is ignored in favour of the actual surface.
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
#else
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
    if (!m_window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Coult not create window: %s\n", SDL_GetError());
        return false;
    }

    // The UI is laid out for 640x480 upwards; smaller clips the game window.
    SDL_SetWindowMinimumSize(m_window, 640, 480);

    SetStubWindow(m_window);

    // Without this SDL delivers no SDL_TEXTINPUT events at all, so every text
    // field in the client silently ignores typing. It is not implicitly enabled:
    // SDL3 (which sdl2-compat sits on) requires it per window.
    //
    // Not on Android, where it also raises the soft keyboard: leaving it on from
    // startup put the keyboard over the bottom half of the screen for the whole
    // session, including over the login panel it was covering. There it is
    // turned on and off to follow the focused text field instead - see
    // UpdateTextInput().
#if !defined(__ANDROID__)
    SDL_StartTextInput();
#else
    SDL_AddEventWatch(AppLifecycleWatch, nullptr);
#endif

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(m_window, &info))
    {
        SDL_Log("SDL %d.%d.%d\n", info.version.major, info.version.minor, info.version.patch);

        const char *subsystem = "Unknown";
        switch (info.subsystem)
        {
            case SDL_SYSWM_UNKNOWN:
                break;
            case SDL_SYSWM_WINDOWS:
                subsystem = "Microsoft Windows(TM)";
                break;
            case SDL_SYSWM_X11:
                subsystem = "X Window System";
                break;
#if SDL_VERSION_ATLEAST(2, 0, 3)
            case SDL_SYSWM_WINRT:
                subsystem = "WinRT";
                break;
#endif
            case SDL_SYSWM_DIRECTFB:
                subsystem = "DirectFB";
                break;
            case SDL_SYSWM_COCOA:
                subsystem = "Apple OS X";
                break;
            case SDL_SYSWM_UIKIT:
                subsystem = "UIKit";
                break;
#if SDL_VERSION_ATLEAST(2, 0, 2)
            case SDL_SYSWM_WAYLAND:
                subsystem = "Wayland";
                break;
            case SDL_SYSWM_MIR:
                subsystem = "Mir";
                break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 4)
            case SDL_SYSWM_ANDROID:
                subsystem = "Android";
                break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_SYSWM_VIVANTE:
                subsystem = "Vivante";
                break;
#endif
        }

        SDL_Log("System: %s\n", subsystem);
#if defined(ORION_WINDOWS)
        Handle = info.info.win.window;
#endif
    }

    SDL_ShowCursor(showCursor);
#endif // USE_WISP

    return OnCreate();
}

//----------------------------------------------------------------------------------
void CWindow::Destroy()
{
    WISPFUN_DEBUG("c14_f6");
#if USE_WISP
    PostMessage(Handle, WM_CLOSE, 0, 0);
#else
    if (m_window)
        SDL_DestroyWindow(m_window);
#endif
}
//----------------------------------------------------------------------------------
void CWindow::ShowMessage(const string &text, const string &title)
{
    WISPFUN_DEBUG("c14_f7");
#if USE_WISP
    MessageBoxA(Handle, text.c_str(), title.c_str(), MB_OK);
#else
    SDL_Log("%s: %s\n", title.c_str(), text.c_str());
#endif
}
//----------------------------------------------------------------------------------
void CWindow::ShowMessage(const wstring &text, const wstring &title)
{
    WISPFUN_DEBUG("c14_f8");
#if USE_WISP
    MessageBoxW(Handle, text.c_str(), title.c_str(), MB_OK);
#else
    SDL_Log("%s: %s\n", title.c_str(), text.c_str());
#endif
}
//----------------------------------------------------------------------------------
#if USE_WISP
LRESULT CWindow::OnWindowProc(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam)
{
    WISPFUN_DEBUG("c14_f9");
    //DebugMsg("m=0x%08X, w0x%08X l0x%08X\n", message, wParam, lParam);
    static bool parse = true;

    if (!parse)
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch (message)
    {
        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTCLIENT)
            {
                SetCursor(NULL);
                return 0;
            }
            break;
        }
        case WM_GETMINMAXINFO:
        case WM_SIZE:
        {
            if (IsMinimizedWindow())
                return DefWindowProc(hWnd, message, wParam, lParam);

            if (message == WM_GETMINMAXINFO)
            {
                MINMAXINFO *pInfo = (MINMAXINFO *)lParam;

                if (NoResize)
                {
                    RECT r = { 0, 0, 0, 0 };
                    r.right = m_Size.Width;
                    r.bottom = m_Size.Height;
                    AdjustWindowRectEx(
                        &r,
                        GetWindowLongA(Handle, GWL_STYLE),
                        FALSE,
                        GetWindowLongA(Handle, GWL_EXSTYLE));

                    if (r.left < 0)
                        r.right -= r.left;

                    if (r.top < 0)
                        r.bottom -= r.top;

                    POINT min = { r.right, r.bottom };
                    POINT max = { r.right, r.bottom };

                    pInfo->ptMinTrackSize = min;
                    pInfo->ptMaxTrackSize = max;
                }
                else
                {
                    RECT r = { 0, 0, 0, 0 };
                    r.right = m_Size.Width;
                    r.bottom = m_Size.Height;
                    AdjustWindowRectEx(
                        &r,
                        GetWindowLongA(Handle, GWL_STYLE),
                        FALSE,
                        GetWindowLongA(Handle, GWL_EXSTYLE));

                    if (r.left < 0)
                        r.right -= r.left;

                    if (r.top < 0)
                        r.bottom -= r.top;

                    POINT min = { m_MinSize.Width, m_MinSize.Height };
                    POINT max = { m_MaxSize.Width, m_MaxSize.Height };
                    pInfo->ptMinTrackSize = min;
                    pInfo->ptMaxTrackSize = max;
                }

                return 0;
            }

            WISP_GEOMETRY::CSize newSize(LOWORD(lParam), HIWORD(lParam));

            OnResize(newSize);
            m_Size = newSize;

            break;
        }
        case WM_CLOSE:
        case WM_NCDESTROY:
        case WM_DESTROY:
        {
            parse = false;

            OnDestroy();

            //ExitProcess(0);
            PostQuitMessage(0);

            return 0;
        }
        case WM_MOUSEMOVE:
        {
            WISP_MOUSE::g_WispMouse->LeftButtonPressed = (bool)(wParam & MK_LBUTTON);
            WISP_MOUSE::g_WispMouse->RightButtonPressed = (bool)(wParam & MK_RBUTTON);
            WISP_MOUSE::g_WispMouse->MidButtonPressed = (bool)(wParam & MK_MBUTTON);
            WISP_MOUSE::g_WispMouse->Update(); // TODO: check if is correct

            if (WISP_MOUSE::g_WispMouse->Dragging)
                OnDragging();

            break;
        }
        case WM_LBUTTONDOWN:
        {
            WISP_MOUSE::g_WispMouse->Capture();

            WISP_MOUSE::g_WispMouse->Update();
            WISP_MOUSE::g_WispMouse->LeftButtonPressed = true;
            WISP_MOUSE::g_WispMouse->LeftDropPosition = WISP_MOUSE::g_WispMouse->Position;
            WISP_MOUSE::g_WispMouse->CancelDoubleClick = false;

            uint ticks = SDL_GetTicks();

            if (WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer +
                    WISP_MOUSE::g_WispMouse->DoubleClickDelay >=
                ticks)
            {
                WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = 0;

                if (!OnLeftMouseButtonDoubleClick())
                    OnLeftMouseButtonDown();
                else
                    WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = 0xFFFFFFFF;

                break;
            }

            OnLeftMouseButtonDown();

            if (WISP_MOUSE::g_WispMouse->CancelDoubleClick)
                WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = 0;
            else
                WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = ticks;

            break;
        }
        case WM_LBUTTONUP:
        {
            WISP_MOUSE::g_WispMouse->Update();

            if (WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer != 0xFFFFFFFF)
                OnLeftMouseButtonUp();

            WISP_MOUSE::g_WispMouse->LeftButtonPressed = false;
            WISP_MOUSE::g_WispMouse->Release();

            break;
        }
        case WM_RBUTTONDOWN:
        {
            WISP_MOUSE::g_WispMouse->Capture();

            WISP_MOUSE::g_WispMouse->Update();
            WISP_MOUSE::g_WispMouse->RightButtonPressed = true;
            WISP_MOUSE::g_WispMouse->RightDropPosition = WISP_MOUSE::g_WispMouse->Position;
            WISP_MOUSE::g_WispMouse->CancelDoubleClick = false;

            uint ticks = SDL_GetTicks();

            if (WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer +
                    WISP_MOUSE::g_WispMouse->DoubleClickDelay >=
                ticks)
            {
                WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = 0;

                if (!OnRightMouseButtonDoubleClick())
                    OnRightMouseButtonDown();
                else
                    WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = 0xFFFFFFFF;

                break;
            }

            OnRightMouseButtonDown();

            if (WISP_MOUSE::g_WispMouse->CancelDoubleClick)
                WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = 0;
            else
                WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = ticks;

            break;
        }
        case WM_RBUTTONUP:
        {
            WISP_MOUSE::g_WispMouse->Update();

            if (WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer != 0xFFFFFFFF)
                OnRightMouseButtonUp();

            WISP_MOUSE::g_WispMouse->RightButtonPressed = false;
            WISP_MOUSE::g_WispMouse->Release();

            break;
        }
        //Нажатие на колесико мышки
        case WM_MBUTTONDOWN:
        {
            WISP_MOUSE::g_WispMouse->Capture();

            WISP_MOUSE::g_WispMouse->Update();
            WISP_MOUSE::g_WispMouse->MidButtonPressed = true;
            WISP_MOUSE::g_WispMouse->MidDropPosition = WISP_MOUSE::g_WispMouse->Position;
            WISP_MOUSE::g_WispMouse->CancelDoubleClick = false;

            uint ticks = SDL_GetTicks();

            if (WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer +
                    WISP_MOUSE::g_WispMouse->DoubleClickDelay >=
                ticks)
            {
                if (!OnMidMouseButtonDoubleClick())
                    OnMidMouseButtonDown();

                WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer = 0;

                break;
            }

            OnMidMouseButtonDown();

            if (WISP_MOUSE::g_WispMouse->CancelDoubleClick)
                WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer = 0;
            else
                WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer = ticks;

            break;
        }
        //Отпускание колесика мышки
        case WM_MBUTTONUP:
        {
            WISP_MOUSE::g_WispMouse->Update();
            OnMidMouseButtonUp();

            WISP_MOUSE::g_WispMouse->MidButtonPressed = false;
            WISP_MOUSE::g_WispMouse->Release();

            break;
        }
        //Колесико мышки вверх/вниз
        case WM_MOUSEWHEEL:
        {
            WISP_MOUSE::g_WispMouse->Update();
            OnMidMouseButtonScroll(!(short(HIWORD(wParam)) > 0));

            break;
        }
        //Доп. кнопки мыши
        case WM_XBUTTONDOWN:
        {
            WISP_MOUSE::g_WispMouse->Update();
            OnXMouseButton(!(short(HIWORD(wParam)) > 0));

            break;
        }
        case WM_CHAR:
        {
            OnCharPress(wParam, lParam);

            return 0; //break;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            OnKeyDown(wParam, lParam);

            if (wParam == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x80000000)) //Alt + F4
                break;

            return 0; //break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            OnKeyUp(wParam, lParam);

            return 0; //break;
        }
        case WM_NCACTIVATE:
        {
            HRESULT res = (HRESULT)DefWindowProc(Handle, WM_NCACTIVATE, wParam, lParam);

            if (wParam == 0)
                OnDeactivate();
            else
                OnActivate();

            return res;
        }
        case WM_NCPAINT:
            return OnRepaint(wParam, lParam);
        case WM_SHOWWINDOW:
        {
            HRESULT res = (HRESULT)DefWindowProc(Handle, WM_SHOWWINDOW, wParam, lParam);

            OnShow(wParam != 0);

            return res;
        }
        case WM_SETTEXT:
        {
            HRESULT res = (HRESULT)DefWindowProc(Handle, WM_SETTEXT, wParam, lParam);

            OnSetText(lParam);

            return res;
        }
        case WM_TIMER:
        {
            OnTimer((uint)wParam);

            break;
        }
        case WISP_THREADED_TIMER::CThreadedTimer::MessageID:
        {
            OnThreadedTimer((uint)wParam, (WISP_THREADED_TIMER::CThreadedTimer *)lParam);

            //DebugMsg("OnThreadedTimer %i, 0x%08X\n", wParam, lParam);

            return 0;
        }
        case WM_SYSCHAR:
        {
            if (wParam == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x80000000)) //Alt + F4
                break;
            return 0;
        }
        default:
            break;
    }

    if (message >= WM_USER)
        return OnUserMessages(message, wParam, lParam);

    return DefWindowProc(hWnd, message, wParam, lParam);
}
#else
//----------------------------------------------------------------------------------
#if defined(__ANDROID__)
namespace
{
// The client is written for a two-button mouse: the left button selects, drags
// and double-clicks, and walking is the right button held down in the direction
// to move.
//
// A finger only ever stands in for the left button here - tap to click, drag to
// drag - so that touching the world does what touching it looks like it should.
// Walking is the on-screen stick instead, which is both easier to aim and does
// not fight everything else a press might have meant.
// Sixteen pixels is about a millimetre and a half on a screen this dense, which
// no fingertip holds still inside: every touch travelled further than that and
// so was read as a drag rather than a tap.
const int TouchSlop = 26;        // pixels of travel before a press is a drag
// Whether it turned out to be a tap after all is decided on the way up, by how
// far and how long, rather than by which branch the press took on the way down.
const int TapTravel = 64;
const uint TapDuration = 400;
// A fingertip is neither as quick nor as steady as a mouse, and the client's own
// 350ms/one-pixel double click is far too strict for one: two deliberate taps
// land tens of pixels and half a second apart.
const uint DoubleTapDelay = 600;
const int DoubleTapSlop = 64;

// How long the stick has to be held still, thumb centred, before it stops
// steering and starts being dragged somewhere else.

struct
{
    SDL_FingerID Finger = 0;
    bool Active = false;
    bool LeftDown = false;
    bool RightDown = false;
    uint StartTicks = 0;
    WISP_GEOMETRY::CPoint2Di Start;
    WISP_GEOMETRY::CPoint2Di Current;
} g_Touch;

// The stick is tracked separately so it can be worked at the same time as a tap
// or a drag somewhere else - walking while picking something up.
struct
{
    SDL_FingerID Finger = 0;
    bool Active = false;
    bool RightDown = false;
    bool Moving = false;
    uint DownTicks = 0;
    // Where the finger landed and where it is now, so a hold can be told from
    // a push without needing it on the exact centre of the ring.
    WISP_GEOMETRY::CPoint2Di DownAt;
    WISP_GEOMETRY::CPoint2Di CurrentAt;
} g_Stick;

// The war toggle's own finger, tracked the same way: held still it toggles on
// release, dragged it carries the button somewhere else.
struct
{
    bool Moving = false;
    WISP_GEOMETRY::CPoint2Di DownAt;
} g_Button;

// Where the ring has been dragged to, as a fraction of the window so it keeps
// its place across a rotation. Negative means "never moved, use the default".
float g_StickPlaceX = -1.0f;
float g_StickPlaceY = -1.0f;
// Same for the war toggle, which used to be pinned above the ring.
float g_ButtonPlaceX = -1.0f;
float g_ButtonPlaceY = -1.0f;
bool g_StickPlaceLoaded = false;
SDL_FingerID g_ButtonFinger = 0;
// When a finger event last arrived; the stale-press watchdog goes by this.
uint g_LastTouchEventTicks = 0;
// The last tap, for spotting the second one of a pair.
uint g_LastTapTicks = 0;
WISP_GEOMETRY::CPoint2Di g_LastTapAt;

// Where the stick sits, and how hard it has to be pushed. The ring is parked in
// the bottom-left corner of the window, out of the way of the status bar and
// paperdoll, which the client puts on the right.
const int StickMargin = 28;
const int StickDeadZone = 6; // pixels of deflection that still count as centred

// Deflection maps onto how far the cursor would be from the player. The client
// walks below roughly two tiles and runs beyond that, so a half-pushed stick
// walks and a fully pushed one runs.
const int StickWalkDistance = 40;
const int StickRunDistance = 190;

// Finger coordinates are fractions of the window; the client works in the same
// units SDL_GetMouseState reports, so scale them by the window size.
WISP_GEOMETRY::CPoint2Di TouchToWindow(float normalizedX, float normalizedY)
{
    int width = 0;
    int height = 0;
    if (g_WispWindow != nullptr)
        SDL_GetWindowSize(g_WispWindow->m_window, &width, &height);

    return WISP_GEOMETRY::CPoint2Di((int)(normalizedX * width), (int)(normalizedY * height));
}
} // namespace
#endif
//----------------------------------------------------------------------------------
bool g_InSyntheticTouchEvent = false;
// The client draws its own keyboard rather than asking for Android's, which
// takes about half a landscape screen, cannot be told not to, and forces the
// window out of immersive mode to have somewhere to appear. Ours is a gump: it
// opens where it was left, moves like anything else, and closes with its own
// button or by tapping it twice like any other gump.
void CWindow::ToggleKeyboardGump()
{
#if defined(__ANDROID__)
    if (g_GumpKeyboard != NULL)
    {
        g_GumpManager.RemoveGump(g_GumpKeyboard);
        return;
    }

    // In the world it goes along the bottom, clear of the status bar and
    // paperdoll the client puts on the right; before that the login screen is
    // drawn into a letterboxed 640x480 scene, so it is placed in those units.
    const WISP_GEOMETRY::CSize size = GetSize();

    int width = 0;
    int height = 0;
    CGumpKeyboard::PreferredSize(width, height);

    int x = (size.Width - width) / 2;
    int y = size.Height - height - 20;

    if (g_GameState < GS_GAME)
    {
        // The pre-game screens are a letterboxed 640x480 scene, and a gump there
        // is placed in those units rather than the window's. It sits at the top
        // there: the account and password fields are in the lower half, and a
        // keyboard over the thing being typed into is no use.
        x = (640 - width) / 2;
        y = 8;
    }

    // Wherever it was last put, if it has been put anywhere.
    CGumpKeyboard::RecallPlacement(x, y);

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    g_GumpManager.AddGump(new CGumpKeyboard((short)x, (short)y));
#endif
}
//----------------------------------------------------------------------------------
void CWindow::TouchMouseEvent(uint type, uchar button, const WISP_GEOMETRY::CPoint2Di &at)
{
#if defined(__ANDROID__)
    WISP_MOUSE::g_WispMouse->UseTouchPosition = true;
    WISP_MOUSE::g_WispMouse->TouchPosition = at;

    // Handled here and now rather than pushed onto the queue: by the time a
    // queued event came back the finger would have moved, and a press would be
    // delivered at wherever it had got to.
    SDL_Event synthetic;
    SDL_memset(&synthetic, 0, sizeof(synthetic));
    synthetic.type = type;

    if (type == SDL_MOUSEMOTION)
    {
        synthetic.motion.x = at.X;
        synthetic.motion.y = at.Y;
    }
    else
    {
        synthetic.button.button = button;
        synthetic.button.state = (type == SDL_MOUSEBUTTONDOWN) ? SDL_PRESSED : SDL_RELEASED;
        synthetic.button.clicks = 1;
        synthetic.button.x = at.X;
        synthetic.button.y = at.Y;
    }

    g_InSyntheticTouchEvent = true;
    OnWindowProc(synthetic);
    g_InSyntheticTouchEvent = false;
#else
    (void)type;
    (void)button;
    (void)at;
#endif
}
//----------------------------------------------------------------------------------
#if defined(__ANDROID__)
#include <jni.h>

// A desktop client saves its profile - gump placement, macros, options - when it
// is closed. An Android one is never closed: it is swiped away, or killed while
// backgrounded, so nothing was ever written and everything was lost each time.
//
// Backgrounding is the platform's real shutdown, but the event cannot be handled
// from the main loop: SDL blocks the game thread as soon as Android pauses the
// app, so a queued event is never pumped. A watch is called synchronously by
// whoever sends the event, which is what SDL documents for exactly this on
// mobile, and it runs while there is still time to write.
static int SDLCALL AppLifecycleWatch(void * /*userdata*/, SDL_Event *event)
{
    if (event != nullptr &&
        (event->type == SDL_APP_WILLENTERBACKGROUND || event->type == SDL_APP_TERMINATING ||
         event->type == SDL_APP_DIDENTERBACKGROUND))
    {
        g_Orion.SaveLocalConfig(g_PacketManager.ConfigSerial);
    }

    return 0;
}

static void AndroidSetImmersive(bool immersive)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (env == nullptr || activity == nullptr)
        return;

    jclass cls = env->GetObjectClass(activity);
    if (cls != nullptr)
    {
        jmethodID method = env->GetStaticMethodID(cls, "setImmersiveMode", "(Z)V");
        if (method != nullptr)
            env->CallStaticVoidMethod(cls, method, (jboolean)immersive);
        else
            env->ExceptionClear();

        env->DeleteLocalRef(cls);
    }

    env->DeleteLocalRef(activity);
}
#endif
//----------------------------------------------------------------------------------
void CWindow::ToggleTextInput()
{
    m_TextInputRequested = !m_TextInputRequested;
}
//----------------------------------------------------------------------------------
void CWindow::UpdateTextInput(bool wanted)
{
#if defined(__ANDROID__)
    // The client draws its own keyboard everywhere now, login screens included:
    // it does not take half the screen and does not force the window out of
    // immersive mode to find somewhere to appear, and it carries the chat modes.
    // Something taking focus opens it - once, on the change, so that closing it
    // while a field is still focused does not reopen it every frame.
    const bool changed = (wanted != m_TextInputActive);

    m_TextInputActive = wanted;
    m_TextInputDirty = false;

    if (changed && wanted && g_GumpKeyboard == NULL)
        ToggleKeyboardGump();
#else
    (void)wanted;
#endif
}
//----------------------------------------------------------------------------------
#if defined(__ANDROID__)
static os_path TouchStickPlacementPath()
{
    return g_App.ExeFilePath("touchstick.txt");
}

static void LoadTouchStickPlacement()
{
    g_StickPlaceLoaded = true;

    FILE *file = fopen(StringFromPath(TouchStickPlacementPath()).c_str(), "r");
    if (file == nullptr)
        return;

    float x = -1.0f;
    float y = -1.0f;
    if (fscanf(file, "%f %f", &x, &y) == 2 && x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f)
    {
        g_StickPlaceX = x;
        g_StickPlaceY = y;

        // Written by a later version; a file with only the ring in it leaves
        // the button where it has always been, just above the ring.
        if (fscanf(file, "%f %f", &x, &y) == 2 && x >= 0.0f && x <= 1.0f && y >= 0.0f &&
            y <= 1.0f)
        {
            g_ButtonPlaceX = x;
            g_ButtonPlaceY = y;
        }
    }

    fclose(file);
}

static void SaveTouchStickPlacement()
{
    FILE *file = fopen(StringFromPath(TouchStickPlacementPath()).c_str(), "w");
    if (file == nullptr)
        return;

    fprintf(file, "%.4f %.4f\n", g_StickPlaceX, g_StickPlaceY);
    fprintf(file, "%.4f %.4f\n", g_ButtonPlaceX, g_ButtonPlaceY);
    fclose(file);
}

// Places the ring, and says whether it should be on screen at all - only in the
// world, where walking means anything.
static void UpdateTouchStickBounds()
{
    if (g_WispWindow == nullptr)
        return;

    if (!g_StickPlaceLoaded)
        LoadTouchStickPlacement();

    const WISP_GEOMETRY::CSize size = g_WispWindow->GetSize();
    const int shorter = (size.Width < size.Height) ? size.Width : size.Height;

    // A seventh of the short side. A tenth looked right beside the world but is
    // a small thing to aim a thumb at, and left the grip on its rim smaller
    // still - too small to read as something to be pressed.
    g_TouchStick.Radius = shorter / 7;
    if (g_TouchStick.Radius < 72)
        g_TouchStick.Radius = 72;

    if (g_StickPlaceX >= 0.0f)
    {
        g_TouchStick.CenterX = (int)(g_StickPlaceX * size.Width);
        g_TouchStick.CenterY = (int)(g_StickPlaceY * size.Height);
    }
    else
    {
        // Bottom left by default: the client puts the status bar and paperdoll
        // on the right.
        g_TouchStick.CenterX = g_TouchStick.Radius + StickMargin;
        g_TouchStick.CenterY = size.Height - g_TouchStick.Radius - StickMargin;
    }

    g_TouchStick.ButtonRadius = g_TouchStick.Radius / 3;

    if (g_ButtonPlaceX >= 0.0f)
    {
        g_TouchStick.ButtonX = (int)(g_ButtonPlaceX * size.Width);
        g_TouchStick.ButtonY = (int)(g_ButtonPlaceY * size.Height);
    }
    else
    {
        g_TouchStick.ButtonX = g_TouchStick.CenterX;
        g_TouchStick.ButtonY =
            g_TouchStick.CenterY - g_TouchStick.Radius - g_TouchStick.ButtonRadius - 14;

        // Flipped below the ring if there is no room above it.
        if (g_TouchStick.ButtonY - g_TouchStick.ButtonRadius < 0)
        {
            g_TouchStick.ButtonY =
                g_TouchStick.CenterY + g_TouchStick.Radius + g_TouchStick.ButtonRadius + 14;
        }
    }

    // On the upper-left rim, away from the war toggle above and from where a
    // right handed thumb rests.
    g_TouchStick.GripRadius = g_TouchStick.Radius / 4;
    g_TouchStick.GripX = g_TouchStick.CenterX - (int)(g_TouchStick.Radius * 0.72f);
    g_TouchStick.GripY = g_TouchStick.CenterY - (int)(g_TouchStick.Radius * 0.72f);

    g_TouchStick.Visible = (g_GameState >= GS_GAME);
    g_TouchStick.Active = g_Stick.Active;
    g_TouchStick.Moving = g_Stick.Moving;
    g_TouchStick.ButtonMoving = g_Button.Moving;
}

// Drops the ring wherever the finger is, kept fully on screen.
static void TouchStickPlaceAt(const WISP_GEOMETRY::CPoint2Di &at)
{
    if (g_WispWindow == nullptr)
        return;

    const WISP_GEOMETRY::CSize size = g_WispWindow->GetSize();
    const int edge = g_TouchStick.Radius + 4;

    int x = at.X;
    int y = at.Y;

    if (x < edge)
        x = edge;
    if (y < edge)
        y = edge;
    if (x > size.Width - edge)
        x = size.Width - edge;
    if (y > size.Height - edge)
        y = size.Height - edge;

    g_TouchStick.CenterX = x;
    g_TouchStick.CenterY = y;
    g_StickPlaceX = (float)x / (float)size.Width;
    g_StickPlaceY = (float)y / (float)size.Height;
}

// Drops the war toggle wherever the finger is, kept fully on screen.
static void TouchButtonPlaceAt(const WISP_GEOMETRY::CPoint2Di &at)
{
    if (g_WispWindow == nullptr)
        return;

    const WISP_GEOMETRY::CSize size = g_WispWindow->GetSize();
    const int edge = g_TouchStick.ButtonRadius + 4;

    int x = at.X;
    int y = at.Y;

    if (x < edge)
        x = edge;
    if (y < edge)
        y = edge;
    if (x > size.Width - edge)
        x = size.Width - edge;
    if (y > size.Height - edge)
        y = size.Height - edge;

    g_TouchStick.ButtonX = x;
    g_TouchStick.ButtonY = y;
    g_ButtonPlaceX = (float)x / (float)size.Width;
    g_ButtonPlaceY = (float)y / (float)size.Height;
}

// Walks in the direction the knob is pushed.
//
// This used to fake the right button at a cursor position near the player, the
// way a desktop walk works. That put a synthetic press wherever the cursor
// landed - and gumps float over the world, so pushing the stick towards one
// stopped the walk dead (the client only walks while RightGump is null) and
// letting go right-clicked it, which in UO closes it. That is where the
// paperdoll went. Asking the pathfinder directly keeps the stick clear of the
// interface entirely.
static void TouchStickWalk()
{
    if (g_PathFinder.AutoWalking)
        return;

    const float deflection =
        sqrtf(g_TouchStick.OffsetX * g_TouchStick.OffsetX +
              g_TouchStick.OffsetY * g_TouchStick.OffsetY);

    const int centerX = g_ConfigManager.GameWindowX + g_ConfigManager.GameWindowWidth / 2;
    const int centerY = g_ConfigManager.GameWindowY + g_ConfigManager.GameWindowHeight / 2;

    // GetFacing wants a point to aim at; the distance does not matter to it, only
    // the angle, so push it well clear of the centre to keep the angle stable.
    int direction = g_MouseManager.GetFacing(
        centerX,
        centerY,
        centerX + (int)(g_TouchStick.OffsetX * 256.0f),
        centerY + (int)(g_TouchStick.OffsetY * 256.0f),
        1);

    if (direction == 0)
        direction = 8;

    // Half pushed walks, hard over runs - the same split the mouse gets from its
    // 190 pixel range.
    g_PathFinder.Walk(deflection > 0.66f, direction - 1);
}
//----------------------------------------------------------------------------------
static WISP_GEOMETRY::CPoint2Di TouchStickToCursor()
{
    const float deflection =
        sqrtf(g_TouchStick.OffsetX * g_TouchStick.OffsetX +
              g_TouchStick.OffsetY * g_TouchStick.OffsetY);

    const float distance =
        StickWalkDistance + deflection * (StickRunDistance - StickWalkDistance);

    const int centerX = g_RenderBounds.GameWindowPosX + g_RenderBounds.GameWindowWidth / 2;
    const int centerY = g_RenderBounds.GameWindowPosY + g_RenderBounds.GameWindowHeight / 2;

    // Normalised so the direction is what the deflection says even when the
    // knob is only nudged; distance carries the speed.
    const float length = (deflection > 0.0001f) ? deflection : 1.0f;

    return WISP_GEOMETRY::CPoint2Di(
        centerX + (int)(g_TouchStick.OffsetX / length * distance),
        centerY + (int)(g_TouchStick.OffsetY / length * distance));
}

// Feeds the finger's position into the knob, clamped to the ring.
static void TouchStickSetFrom(const WISP_GEOMETRY::CPoint2Di &at)
{
    float dx = (float)(at.X - g_TouchStick.CenterX);
    float dy = (float)(at.Y - g_TouchStick.CenterY);

    const float distance = sqrtf(dx * dx + dy * dy);
    if (distance > (float)g_TouchStick.Radius)
    {
        dx = dx / distance * (float)g_TouchStick.Radius;
        dy = dy / distance * (float)g_TouchStick.Radius;
    }

    g_TouchStick.OffsetX = dx / (float)g_TouchStick.Radius;
    g_TouchStick.OffsetY = dy / (float)g_TouchStick.Radius;
}

static bool TouchStickCentred()
{
    const float deflection =
        sqrtf(g_TouchStick.OffsetX * g_TouchStick.OffsetX +
              g_TouchStick.OffsetY * g_TouchStick.OffsetY);

    return deflection * g_TouchStick.Radius < StickDeadZone;
}
#endif
//----------------------------------------------------------------------------------
void CWindow::ProcessTouch()
{
#if defined(__ANDROID__)
    UpdateTouchStickBounds();

    // The stick has to keep pushing: the client walks a step per right-button
    // poll, and a finger held still on the ring sends no events.
    if (g_Stick.Active)
    {
        if (!g_Stick.Moving && !TouchStickCentred() && g_GameState >= GS_GAME)
            TouchStickWalk();
    }

    // Nothing is touching the screen, yet the client still believes a button is
    // held. SDL can lose a FINGERUP - a lift that leaves the window, a gesture
    // the system swallows - and the press then never ends: a dragged paperdoll
    // stayed stuck to the cursor and jumped to wherever was touched next.
    //
    // Asking SDL how many fingers are down turned out to be no way to tell. It
    // reported an empty screen during a live drag, and the watchdog then cut the
    // drag short after a fraction of a second - which is what made the status bar
    // move a few pixels and snap back. Go by silence instead: a finger that is
    // really there produces motion, and one whose lift was swallowed produces
    // nothing at all. The timeout is long enough that no real drag reaches it.
    const bool holding = (g_Touch.Active || g_Touch.LeftDown || g_Touch.RightDown ||
                          g_Stick.Active || g_TouchStick.ButtonHeld);

    if (holding && SDL_GetTicks() - g_LastTouchEventTicks >= 3000)
    {

        if (g_Touch.LeftDown)
            TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, g_Touch.Current);

        if (g_Touch.RightDown)
            TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, g_Touch.Current);

        if (g_Stick.Moving)
            SaveTouchStickPlacement();

        g_Touch.Active = false;
        g_Touch.LeftDown = false;
        g_Touch.RightDown = false;
        g_Stick.Active = false;
        g_Stick.RightDown = false;
        g_Stick.Moving = false;
        g_TouchStick.ButtonHeld = false;
        g_Button.Moving = false;
        g_TouchStick.OffsetX = 0.0f;
        g_TouchStick.OffsetY = 0.0f;
    }
#endif
}
//----------------------------------------------------------------------------------
bool CWindow::OnWindowProc(SDL_Event &ev)
{
#if defined(__ANDROID__)
    // Belt and braces for the hint above: whatever SDL decides to synthesise
    // from a finger is marked with this device id, and the gesture handler has
    // already turned that same finger into the events the client should see.
    // Letting both through presses twice, the emulated one at wherever the
    // cursor happened to be.
    switch (ev.type)
    {
        case SDL_MOUSEMOTION:
            if (ev.motion.which == SDL_TOUCH_MOUSEID && !g_InSyntheticTouchEvent)
                return true;
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (ev.button.which == SDL_TOUCH_MOUSEID && !g_InSyntheticTouchEvent)
                return true;
            break;
        case SDL_MOUSEWHEEL:
            if (ev.wheel.which == SDL_TOUCH_MOUSEID)
                return true;
            break;
        default:
            break;
    }
#endif

    switch (ev.type)
    {
        case SDL_QUIT:
        {
            OnDestroy();
            return true;
            break;
        }



        // SDL_WINDOWEVENT_* are subtypes carried in ev.window.event, not event
        // types. They used to be matched against ev.type directly, so none of
        // them ever fired: show/hide never reached the plugin or the sound and
        // FPS handling, and resizes were not noticed at all.
        case SDL_WINDOWEVENT:
        {
            switch (ev.window.event)
            {
                case SDL_WINDOWEVENT_CLOSE:
                {
                    OnDestroy();
                    return true;
                }

                case SDL_WINDOWEVENT_SHOWN:
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    OnShow(true); // Plugin
                    OnActivate(); // Sound + FPS
                }
                break;

                case SDL_WINDOWEVENT_HIDDEN:
                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    OnShow(false);  // Plugin
                    OnDeactivate(); // Sound + FPS
                }
                break;

                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    m_Size.Width = ev.window.data1;
                    m_Size.Height = ev.window.data2;
                    OnResize(m_Size);
                }
                break;

                default:
                    break;
            }
        }
        break;

            // FIXME
            //case WM_GETMINMAXINFO:
            //case WM_SIZE:
            // WISP_THREADED_TIMER::CThreadedTimer::MessageID:
            //case WM_TIMER:

        case SDL_USEREVENT:
        {
            OnTimer((uint)ev.user.code);
        }
        break;

        case SDL_KEYDOWN:
        {
            OnKeyDown(ev.key);
        }
        break;

        case SDL_KEYUP:
        {
            OnKeyUp(ev.key);
        }
        break;

        // https://wiki.libsdl.org/Tutorials/TextInput
        case SDL_TEXTINPUT: // WM_CHAR
        {
            OnTextInput(ev.text);
        }
        break;

#if defined(__ANDROID__)
        case SDL_FINGERDOWN:
        {
            g_LastTouchEventTicks = SDL_GetTicks();
            const WISP_GEOMETRY::CPoint2Di down = TouchToWindow(ev.tfinger.x, ev.tfinger.y);

            // The war toggle sits beside the stick and takes precedence over it.
            if (g_TouchStick.Visible && !g_TouchStick.ButtonHeld)
            {
                const int dx = down.X - g_TouchStick.ButtonX;
                const int dy = down.Y - g_TouchStick.ButtonY;
                const int reach = g_TouchStick.ButtonRadius + g_TouchStick.ButtonRadius / 2;

                if (dx * dx + dy * dy <= reach * reach)
                {
                    g_TouchStick.ButtonHeld = true;
                    g_Button.Moving = false;
                    g_Button.DownAt = down;
                    g_ButtonFinger = ev.tfinger.fingerId;
                    break;
                }
            }

            // The grip carries the ring, with no timing to get right: pressing
            // it is already the whole gesture.
            if (g_TouchStick.Visible && !g_Stick.Active)
            {
                const int dx = down.X - g_TouchStick.GripX;
                const int dy = down.Y - g_TouchStick.GripY;
                const int reach = g_TouchStick.GripRadius + g_TouchStick.GripRadius / 2;

                if (dx * dx + dy * dy <= reach * reach)
                {
                    g_Stick.Finger = ev.tfinger.fingerId;
                    g_Stick.Active = true;
                    g_Stick.RightDown = false;
                    g_Stick.Moving = true;
                    g_Stick.DownTicks = SDL_GetTicks();
                    g_Stick.DownAt = down;
                    g_Stick.CurrentAt = down;
                    break;
                }
            }

            // The stick takes the finger before any of the gesture logic does,
            // and keeps its own, so walking and handling something at the same
            // time are two separate fingers rather than a conflict.
            if (g_TouchStick.Visible && !g_Stick.Active)
            {
                const int dx = down.X - g_TouchStick.CenterX;
                const int dy = down.Y - g_TouchStick.CenterY;
                const int reach = g_TouchStick.Radius + g_TouchStick.Radius / 3;

                if (dx * dx + dy * dy <= reach * reach)
                {
                    g_Stick.Finger = ev.tfinger.fingerId;
                    g_Stick.Active = true;
                    g_Stick.RightDown = false;
                    g_Stick.Moving = false;
                    g_Stick.DownTicks = SDL_GetTicks();
                    g_Stick.DownAt = down;
                    g_Stick.CurrentAt = down;
                    TouchStickSetFrom(down);
                    break;
                }
            }

            if (g_Touch.Active)
            {
                // A second finger while the first is still deciding what it is:
                // the gesture for the soft keyboard, which is the only way to
                // start typing in the world - the chat console holds focus
                // there permanently, so following focus would leave the
                // keyboard up over the game forever.
                if (!g_Touch.LeftDown && !g_Touch.RightDown)
                {
                    ToggleKeyboardGump();
                    g_Touch.Active = false;
                }
                break;
            }

            g_Touch.Finger = ev.tfinger.fingerId;
            g_Touch.Active = true;
            g_Touch.LeftDown = false;
            g_Touch.RightDown = false;
            g_Touch.StartTicks = SDL_GetTicks();
            g_Touch.Start = down;
            g_Touch.Current = g_Touch.Start;

            // Nothing is sent yet: which button this is depends on what the
            // finger does next. Move the cursor there so the client draws it
            // under the finger in the meantime.
            WISP_MOUSE::g_WispMouse->UseTouchPosition = true;
            WISP_MOUSE::g_WispMouse->TouchPosition = g_Touch.Start;
        }
        break;

        case SDL_FINGERMOTION:
        {
            g_LastTouchEventTicks = SDL_GetTicks();

            // The toggle has nothing to do with a drag, so dragging it is what
            // moves it: press and release to switch war mode, press and slide
            // to carry the button somewhere else.
            if (g_TouchStick.ButtonHeld && ev.tfinger.fingerId == g_ButtonFinger)
            {
                const WISP_GEOMETRY::CPoint2Di at = TouchToWindow(ev.tfinger.x, ev.tfinger.y);

                if (!g_Button.Moving)
                {
                    const int dx = at.X - g_Button.DownAt.X;
                    const int dy = at.Y - g_Button.DownAt.Y;

                    if (dx * dx + dy * dy < TouchSlop * TouchSlop)
                        break;

                    g_Button.Moving = true;
                }

                TouchButtonPlaceAt(at);
                break;
            }

            if (g_Stick.Active && ev.tfinger.fingerId == g_Stick.Finger)
            {
                const WISP_GEOMETRY::CPoint2Di at = TouchToWindow(ev.tfinger.x, ev.tfinger.y);
                g_Stick.CurrentAt = at;

                if (g_Stick.Moving)
                    TouchStickPlaceAt(at);
                else
                    TouchStickSetFrom(at);

                break;
            }

            if (!g_Touch.Active || ev.tfinger.fingerId != g_Touch.Finger)
                break;

            g_Touch.Current = TouchToWindow(ev.tfinger.x, ev.tfinger.y);

            if (!g_Touch.LeftDown && !g_Touch.RightDown)
            {
                const int dx = g_Touch.Current.X - g_Touch.Start.X;
                const int dy = g_Touch.Current.Y - g_Touch.Start.Y;
                if (dx * dx + dy * dy < TouchSlop * TouchSlop)
                    break;

                // Moved before the hold delay: this is a drag, so press the
                // left button where the finger started rather than where it is
                // now - a gump picked up by its edge has to stay under the
                // finger.
                g_Touch.LeftDown = true;
                TouchMouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, g_Touch.Start);
            }

            TouchMouseEvent(SDL_MOUSEMOTION, 0, g_Touch.Current);
        }
        break;

        case SDL_FINGERUP:
        {
            g_LastTouchEventTicks = SDL_GetTicks();
            if (g_TouchStick.ButtonHeld && ev.tfinger.fingerId == g_ButtonFinger)
            {
                g_TouchStick.ButtonHeld = false;

                if (g_Button.Moving)
                {
                    g_Button.Moving = false;
                    SaveTouchStickPlacement();
                }
                else
                    g_Orion.ChangeWarmode(0xFF);

                break;
            }

            if (g_Stick.Active && ev.tfinger.fingerId == g_Stick.Finger)
            {
                if (g_Stick.Moving)
                    SaveTouchStickPlacement();

                g_Stick.Active = false;
                g_Stick.RightDown = false;
                g_Stick.Moving = false;
                g_TouchStick.OffsetX = 0.0f;
                g_TouchStick.OffsetY = 0.0f;
                break;
            }

            if (!g_Touch.Active || ev.tfinger.fingerId != g_Touch.Finger)
                break;

            const WISP_GEOMETRY::CPoint2Di at = g_Touch.Current;

            const uint now = SDL_GetTicks();
            const int moveX = at.X - g_Touch.Start.X;
            const int moveY = at.Y - g_Touch.Start.Y;

            // A press that went nowhere and did not last is a tap, whether or
            // not the finger wandered far enough on the way to have started a
            // drag. Deciding this only on the way down meant a press that had
            // once crossed the slop could never be a tap again, and on a dense
            // screen that was every press there is.
            const bool wasTap =
                (!g_Touch.RightDown && now - g_Touch.StartTicks <= TapDuration &&
                 moveX * moveX + moveY * moveY < TapTravel * TapTravel);


            if (g_Touch.RightDown)
                TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, at);
            else if (!wasTap && g_Touch.LeftDown)
                TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, at);
            else
            {
                const int tapX = at.X - g_LastTapAt.X;
                const int tapY = at.Y - g_LastTapAt.Y;

                const bool secondTap =
                    (g_LastTapTicks != 0 && now - g_LastTapTicks <= DoubleTapDelay &&
                     tapX * tapX + tapY * tapY < DoubleTapSlop * DoubleTapSlop);

                // Tapped twice on a gump, on nothing in particular: that closes
                // it. UO closes a gump with the right button, which a touch
                // screen has no way of producing, so without this there is no
                // way to close one at all.
                //
                // Only where the client has nothing else bound. Anything
                // carrying a serial - an item in a container, a button - is
                // something a double click already means something to, and
                // using an item has to keep working.

                if (secondTap && g_SelectedObject.Gump != NULL && !g_SelectedObject.Serial)
                {
                    // A drag was already begun if the finger wandered on the
                    // way; end it where it started so the gump is not left
                    // nudged a few pixels by the press that closed it.
                    if (g_Touch.LeftDown)
                        TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, g_Touch.Start);

                    TouchMouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, at);
                    TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, at);
                    g_LastTapTicks = 0;
                }
                else
                {
                    // A tap is a left click. The press and release go through
                    // together so the double-click timer sees a complete click;
                    // if a drag already sent the press, only the release is
                    // still owed.
                    if (!g_Touch.LeftDown)
                        TouchMouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, at);

                    TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, at);

                    g_LastTapTicks = now;
                    g_LastTapAt = at;

                    // Whatever the tap landed on, re-ask for the keyboard if a
                    // text field ends up focused: tapping a field the client
                    // already considered focused is exactly how someone brings
                    // it back.
                    m_TextInputDirty = true;
                }
            }

            g_Touch.Active = false;
            g_Touch.LeftDown = false;
            g_Touch.RightDown = false;
        }
        break;
#endif

        case SDL_MOUSEMOTION:
        {
            WISP_MOUSE::g_WispMouse->Update();
            if (WISP_MOUSE::g_WispMouse->Dragging)
                OnDragging();
        }
        break;

        case SDL_MOUSEWHEEL:
        {
            WISP_MOUSE::g_WispMouse->Update();

            const bool isUp = ev.wheel.y > 0;
            OnMidMouseButtonScroll(isUp);
        }
        break;

        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        {
            WISP_MOUSE::g_WispMouse->Update();

            const bool isDown = ev.type == SDL_MOUSEBUTTONDOWN;
            auto &mouse = ev.button;
            switch (mouse.button)
            {
                case SDL_BUTTON_LEFT:
                    // Only set it here for the press. On release it must stay
                    // true until after OnLeftMouseButtonUp(), because
                    // LeftDroppedOffset() returns (0,0) when the button reads as
                    // up - which made every gump drag move by zero pixels. The
                    // Win32 path clears it after the callback for this reason.
                    if (isDown)
                        WISP_MOUSE::g_WispMouse->LeftButtonPressed = true;

                    if (isDown)
                    {
                        WISP_MOUSE::g_WispMouse->Capture();
                        WISP_MOUSE::g_WispMouse->LeftDropPosition =
                            WISP_MOUSE::g_WispMouse->Position;
                        WISP_MOUSE::g_WispMouse->CancelDoubleClick = false;
                        uint ticks = SDL_GetTicks();
                        if (WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer +
                                WISP_MOUSE::g_WispMouse->DoubleClickDelay >=
                            ticks)
                        {
                            WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = 0;
                            if (!OnLeftMouseButtonDoubleClick())
                                OnLeftMouseButtonDown();
                            else
                                WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = 0xFFFFFFFF;
                            break;
                        }

                        OnLeftMouseButtonDown();

                        if (WISP_MOUSE::g_WispMouse->CancelDoubleClick)
                            WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = 0;
                        else
                            WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer = ticks;
                        break;
                    }
                    else
                    {
                        if (WISP_MOUSE::g_WispMouse->LastLeftButtonClickTimer != 0xFFFFFFFF)
                            OnLeftMouseButtonUp();

                        WISP_MOUSE::g_WispMouse->LeftButtonPressed = false;
                        WISP_MOUSE::g_WispMouse->Release();
                    }
                    break;

                case SDL_BUTTON_MIDDLE:
                    WISP_MOUSE::g_WispMouse->MidButtonPressed = isDown;
                    if (isDown)
                    {
                        WISP_MOUSE::g_WispMouse->Capture();
                        WISP_MOUSE::g_WispMouse->MidDropPosition =
                            WISP_MOUSE::g_WispMouse->Position;
                        WISP_MOUSE::g_WispMouse->CancelDoubleClick = false;
                        uint ticks = SDL_GetTicks();
                        if (WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer +
                                WISP_MOUSE::g_WispMouse->DoubleClickDelay >=
                            ticks)
                        {
                            if (!OnMidMouseButtonDoubleClick())
                                OnMidMouseButtonDown();

                            WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer = 0;
                            break;
                        }

                        OnMidMouseButtonDown();

                        if (WISP_MOUSE::g_WispMouse->CancelDoubleClick)
                            WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer = 0;
                        else
                            WISP_MOUSE::g_WispMouse->LastMidButtonClickTimer = ticks;
                    }
                    else
                    {
                        OnMidMouseButtonUp();
                        WISP_MOUSE::g_WispMouse->Release();
                    }
                    break;

                case SDL_BUTTON_RIGHT:
                    WISP_MOUSE::g_WispMouse->RightButtonPressed = isDown;
                    if (isDown)
                    {
                        WISP_MOUSE::g_WispMouse->Capture();
                        WISP_MOUSE::g_WispMouse->RightDropPosition =
                            WISP_MOUSE::g_WispMouse->Position;
                        WISP_MOUSE::g_WispMouse->CancelDoubleClick = false;
                        uint ticks = SDL_GetTicks();
                        if (WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer +
                                WISP_MOUSE::g_WispMouse->DoubleClickDelay >=
                            ticks)
                        {
                            WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = 0;
                            if (!OnRightMouseButtonDoubleClick())
                                OnRightMouseButtonDown();
                            else
                                WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = 0xFFFFFFFF;
                            break;
                        }

                        OnRightMouseButtonDown();

                        if (WISP_MOUSE::g_WispMouse->CancelDoubleClick)
                            WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = 0;
                        else
                            WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer = ticks;
                    }
                    else
                    {
                        if (WISP_MOUSE::g_WispMouse->LastRightButtonClickTimer != 0xFFFFFFFF)
                            OnRightMouseButtonUp();
                        WISP_MOUSE::g_WispMouse->Release();
                    }
                    break;

                case SDL_BUTTON_X1:
                    OnXMouseButton(!isDown);
                    break;

                case SDL_BUTTON_X2:
                    break;
            }
        }
        break;

            // Used by plugins only?
            // - OnShow
            // - OnRepaint
            // - OnSetText
            // - OnXMouseButton

        default:
            break;
    }

    //g_Orion.Process(true);
    //g_Orion.Process(false);

    return false;
}
#endif
//----------------------------------------------------------------------------------
void CWindow::CreateThreadedTimer(
    uint id, int delay, bool oneShot, bool waitForProcessMessage, bool synchronizedDelay)
{
    WISPFUN_DEBUG("c14_f10");
    for (deque<WISP_THREADED_TIMER::CThreadedTimer *>::iterator i = m_ThreadedTimersStack.begin();
         i != m_ThreadedTimersStack.end();
         ++i)
    {
        if ((*i)->TimerID == id)
            return;
    }

    WISP_THREADED_TIMER::CThreadedTimer *timer =
        new WISP_THREADED_TIMER::CThreadedTimer(id, Handle, waitForProcessMessage);
    m_ThreadedTimersStack.push_back(timer);
    timer->Run(!oneShot, delay, synchronizedDelay);
}
//----------------------------------------------------------------------------------
void CWindow::RemoveThreadedTimer(uint id)
{
    WISPFUN_DEBUG("c14_f11");
    for (deque<WISP_THREADED_TIMER::CThreadedTimer *>::iterator i = m_ThreadedTimersStack.begin();
         i != m_ThreadedTimersStack.end();
         ++i)
    {
        if ((*i)->TimerID == id)
        {
            (*i)->Stop();
            m_ThreadedTimersStack.erase(i);

            break;
        }
    }
}
//----------------------------------------------------------------------------------
WISP_THREADED_TIMER::CThreadedTimer *CWindow::GetThreadedTimer(uint id)
{
    WISPFUN_DEBUG("c14_f12");
    for (deque<WISP_THREADED_TIMER::CThreadedTimer *>::iterator i = m_ThreadedTimersStack.begin();
         i != m_ThreadedTimersStack.end();
         ++i)
    {
        if ((*i)->TimerID == id)
            return *i;
    }

    return 0;
}
//----------------------------------------------------------------------------------
}; // namespace WISP_WINDOW
//----------------------------------------------------------------------------------

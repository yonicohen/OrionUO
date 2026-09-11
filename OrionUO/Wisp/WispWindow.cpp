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
    SDL_StartTextInput();

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
// to move. A touch screen has neither button, so gestures stand in:
//
//   tap                 left click - and a second tap inside the double-click
//                       window is a double click, which is how items are used
//   drag                left button held, for moving gumps and items
//   press and hold      right button held: walk towards the finger, steering by
//                       moving it, until the finger lifts
//
// A press only becomes a hold once TouchHoldDelay has passed with the finger
// still roughly in place, which is what keeps a tap and a drag distinguishable
// from the start of a walk.
const uint TouchHoldDelay = 350; // ms before a press starts walking
const int TouchSlop = 16;        // pixels of travel before a press is a drag

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

    OnWindowProc(synthetic);
#else
    (void)type;
    (void)button;
    (void)at;
#endif
}
//----------------------------------------------------------------------------------
void CWindow::ProcessTouch()
{
#if defined(__ANDROID__)
    if (!g_Touch.Active || g_Touch.LeftDown || g_Touch.RightDown)
        return;

    if (SDL_GetTicks() - g_Touch.StartTicks < TouchHoldDelay)
        return;

    g_Touch.RightDown = true;
    TouchMouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, g_Touch.Current);
#endif
}
//----------------------------------------------------------------------------------
bool CWindow::OnWindowProc(SDL_Event &ev)
{
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
            if (g_Touch.Active) // a second finger; the first one owns the gesture
                break;

            g_Touch.Finger = ev.tfinger.fingerId;
            g_Touch.Active = true;
            g_Touch.LeftDown = false;
            g_Touch.RightDown = false;
            g_Touch.StartTicks = SDL_GetTicks();
            g_Touch.Start = TouchToWindow(ev.tfinger.x, ev.tfinger.y);
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
            if (!g_Touch.Active || ev.tfinger.fingerId != g_Touch.Finger)
                break;

            const WISP_GEOMETRY::CPoint2Di at = g_Touch.Current;

            if (g_Touch.RightDown)
                TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, at);
            else if (g_Touch.LeftDown)
                TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, at);
            else
            {
                // Lifted before either the hold delay or the drag threshold: a
                // tap, which is a left click. The press and release go through
                // together so the double-click timer sees a complete click.
                TouchMouseEvent(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, at);
                TouchMouseEvent(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, at);
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

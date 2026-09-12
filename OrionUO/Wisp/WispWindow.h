//----------------------------------------------------------------------------------
#pragma once
#include <SDL_video.h>
#include <SDL_events.h>
namespace WISP_WINDOW
{
//----------------------------------------------------------------------------------
// On-screen movement stick.
//
// Walking in UO is the right mouse button held down in the direction to go,
// which press-and-hold reproduces - but that means holding a finger over the
// world you are trying to look at, and it fights every other thing a press
// might mean. The stick is the familiar touch-game answer: a ring parked in the
// corner that stands in for the cursor's offset from the player.
//
// The window layer owns the geometry and the finger tracking; CGameScreen draws
// it, because it is the only thing that knows the world is on screen.
struct CTouchStick
{
    bool Visible = false;
    bool Active = false;
    // Being carried to a new spot rather than steered.
    bool Moving = false;
    int CenterX = 0;
    int CenterY = 0;
    int Radius = 0;
    // Deflection of the knob, each -1..1.
    float OffsetX = 0.0f;
    float OffsetY = 0.0f;

    // War/peace toggle, parked just above the ring. UO puts this on the
    // paperdoll, which is a long reach from a thumb on the stick.
    int ButtonX = 0;
    int ButtonY = 0;
    int ButtonRadius = 0;
    bool ButtonHeld = false;
};
extern CTouchStick g_TouchStick;
//----------------------------------------------------------------------------------
class CWindow
{
public:
    HWND Handle = 0;
    bool NoResize = false;

protected:
    WISP_GEOMETRY::CSize m_Size = WISP_GEOMETRY::CSize();
    WISP_GEOMETRY::CSize m_MinSize = WISP_GEOMETRY::CSize(100, 100);
    WISP_GEOMETRY::CSize m_MaxSize =
        WISP_GEOMETRY::CSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

public:
    WISP_GEOMETRY::CSize GetSize() { return m_Size; };

    // Ratio of framebuffer pixels to logical points. 2.0 on a Retina display,
    // 1.0 everywhere else. GetSize() and everything laid out in the UI stay in
    // points; only the GL viewport is expressed in pixels.
    float GetPixelRatio() const;
    void SetSize(const WISP_GEOMETRY::CSize &val);

    WISP_GEOMETRY::CSize GetMinSize() { return m_MinSize; };
    void SetMinSize(const WISP_GEOMETRY::CSize &val);

    WISP_GEOMETRY::CSize GetMaxSize() { return m_MaxSize; };
    void SetMaxSize(const WISP_GEOMETRY::CSize &val);

    // GetSystemMetrics -> SDL_GetCurrentDisplayMode
    SDL_Window *m_window = nullptr;

private:
    deque<WISP_THREADED_TIMER::CThreadedTimer *> m_ThreadedTimersStack;

public:
    CWindow();
    virtual ~CWindow();

    void SetMinSize(int width, int height)
    {
        m_MinSize.Width = width;
        m_MinSize.Height = height;
    }
    void SetMaxSize(int width, int height)
    {
        m_MaxSize.Width = width;
        m_MaxSize.Height = height;
    }

    bool Create(
        const char *className,
        const char *title,
        bool showCursor = false,
        int width = 800,
        int height = 600);
    void Destroy();

    void ShowMessage(const string &text, const string &title);
    void ShowMessage(const wstring &text, const wstring &title);

#if USE_WISP
    HINSTANCE hInstance = 0;
    LRESULT OnWindowProc(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam);
#else
    bool OnWindowProc(SDL_Event &ev);

    // Finger gestures become the mouse events the client is written for; a
    // press only turns into a held right button after a delay, so this has to
    // be given a chance to run on frames where no event arrives. No-op away
    // from Android.
    void ProcessTouch();

    // Shows or hides the soft keyboard on Android, following whether a text
    // field has focus. No-op elsewhere, where text input stays on for the life
    // of the window.
    void UpdateTextInput(bool wanted);

    // In the world the chat console always holds focus, so following focus
    // alone would leave the keyboard up over the game for the whole session.
    // A two-finger tap asks for it instead, and asks again to dismiss it.
    void ToggleTextInput();

private:
    bool m_TextInputActive = false;
    bool m_TextInputRequested = false;
    // The keyboard can be dismissed behind the client's back - the Back button,
    // a swipe - and nothing says so. Acting only on changes then left the state
    // stuck at "shown" and every later tap on a field did nothing, so a tap
    // always re-asks.
    bool m_TextInputDirty = false;
    void TouchMouseEvent(uint type, uchar button, const WISP_GEOMETRY::CPoint2Di &at);

public:
#endif

#if USE_WISP
    bool IsActive() const { return (::GetForegroundWindow() == Handle); }
    void SetTitle(const string &text) const { ::SetWindowTextA(Handle, text.c_str()); }
    void ShowWindow(bool show) const { ::ShowWindow(Handle, show ? TRUE : FALSE); }
    bool IsMinimizedWindow() const { return ::IsIconic(Handle); }
    bool IsMaximizedWindow() const { return (::IsZoomed(Handle) != FALSE); }
#else
    // SDL_GetGrabbedWindow reports the mouse-grab owner, which is normally null,
    // so this used to report the window as inactive almost always - which in turn
    // suppressed every sound effect unless BackgroundSound was on.
    bool IsActive() const
    {
        return m_window != nullptr &&
               (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
    void SetTitle(const string &text) const { SDL_SetWindowTitle(m_window, text.c_str()); }
    void ShowWindow(bool show) const { show ? SDL_ShowWindow(m_window) : SDL_HideWindow(m_window); }
    bool IsMinimizedWindow() const { return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) != 0; }
    bool IsMaximizedWindow() const { return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) != 0; }
#endif

    // May be done using: SDL_AddTimer / SDL_RemoveTimer
    void CreateTimer(uint id, int delay) { ::SetTimer(Handle, id, delay, NULL); }
    void RemoveTimer(uint id) { ::KillTimer(Handle, id); }

    void CreateThreadedTimer(
        uint id,
        int delay,
        bool oneShot = false,
        bool waitForProcessMessage = true,
        bool synchronizedDelay = false);
    void RemoveThreadedTimer(uint id);
    WISP_THREADED_TIMER::CThreadedTimer *GetThreadedTimer(uint id);

protected:
    virtual bool OnCreate() { return true; }
    virtual void OnDestroy() {}
    virtual void OnResize(WISP_GEOMETRY::CSize &newSize) {}
    virtual void OnLeftMouseButtonDown() {}
    virtual void OnLeftMouseButtonUp() {}
    virtual bool OnLeftMouseButtonDoubleClick() { return false; }
    virtual void OnRightMouseButtonDown() {}
    virtual void OnRightMouseButtonUp() {}
    virtual bool OnRightMouseButtonDoubleClick() { return false; }
    virtual void OnMidMouseButtonDown() {}
    virtual void OnMidMouseButtonUp() {}
    virtual bool OnMidMouseButtonDoubleClick() { return false; }
    virtual void OnMidMouseButtonScroll(bool up) {}
    virtual void OnXMouseButton(bool up) {}
    virtual void OnDragging() {}
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}
    virtual void OnShow(bool show) {}

    virtual void OnTimer(uint id) {}
    virtual void OnThreadedTimer(uint nowTime, WISP_THREADED_TIMER::CThreadedTimer *timer) {}
    virtual void OnSetText(const LPARAM &lParam) {}
    virtual HRESULT OnRepaint(const WPARAM &wParam, const LPARAM &lParam)
    {
        return (HRESULT)DefWindowProc(Handle, WM_NCPAINT, wParam, lParam);
    }
    virtual LRESULT OnUserMessages(int message, const WPARAM &wParam, const LPARAM &lParam)
    {
        return S_OK;
    }
#if USE_WISP
    virtual void OnCharPress(const WPARAM &wParam, const LPARAM &lParam) {}
    virtual void OnKeyDown(const WPARAM &wParam, const LPARAM &lParam) {}
    virtual void OnKeyUp(const WPARAM &wParam, const LPARAM &lParam) {}
#else
    virtual void OnTextInput(const SDL_TextInputEvent &ev) {}
    virtual void OnKeyDown(const SDL_KeyboardEvent &ev) {}
    virtual void OnKeyUp(const SDL_KeyboardEvent &ev) {}
#endif
};
//----------------------------------------------------------------------------------
extern CWindow *g_WispWindow;
//----------------------------------------------------------------------------------
};  // namespace WISP_WINDOW
    //----------------------------------------------------------------------------------
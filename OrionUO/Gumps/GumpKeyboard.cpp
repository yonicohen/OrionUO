// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** GumpKeyboard.cpp
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
CGumpKeyboard *g_GumpKeyboard = NULL;

float CGumpKeyboard::PlaceX = -1.0f;
float CGumpKeyboard::PlaceY = -1.0f;
float CGumpKeyboard::PreWorldX = -1.0f;
float CGumpKeyboard::PreWorldY = -1.0f;
//----------------------------------------------------------------------------------
const uint CGumpKeyboard::ID_GK_SHIFT = 0x3001;
const uint CGumpKeyboard::ID_GK_SYMBOLS = 0x3002;
const uint CGumpKeyboard::ID_GK_BACKSPACE = 0x3003;
const uint CGumpKeyboard::ID_GK_SPACE = 0x3004;
const uint CGumpKeyboard::ID_GK_ENTER = 0x3005;
const uint CGumpKeyboard::ID_GK_CLOSE = 0x3006;
const uint CGumpKeyboard::ID_GK_MOVE = 0x3007;
//----------------------------------------------------------------------------------
namespace
{
// Four rows, the top one doubling as the number row so there is no layer to
// switch into for the commonest symbols a player types.
const char *g_Rows[4] = { "qwertyuiop", "asdfghjkl", "zxcvbnm", "" };
const char *g_SymbolRows[4] = { "1234567890", "-/:;()$&@", ".,?!'\"", "" };

// UO's interface is gold on dark wood, and the keys are drawn to match rather
// than borrowing a system palette that would look pasted on.
const uint KeyFill = 0xC0201810;
const uint KeyEdge = 0xFF6E5A2E;
const uint KeyFillWide = 0xC0302418;
const uint KeyFillActive = 0xFF6E5A2E;
const uint PanelFill = 0xD0100C08;
} // namespace
//----------------------------------------------------------------------------------
// Ten keys across the row, taking a bit under half the width of the screen, and
// never smaller than a fingertip nor wider than is useful on a tablet.
void CGumpKeyboard::MeasureKeys()
{
    // A key has to be a certain size under a fingertip, and that is a number of
    // real screen pixels - so whatever coordinate space this is being measured
    // in, the floor has to be converted into it or it means nothing.
    float unitsPerPixel = 1.0f;

    // Before the world, everything is laid out in the letterboxed 640x480 scene
    // the login screens are drawn into, not in window pixels. That scene is
    // blown up to fill the screen, so a key there is worth SceneScale pixels.
    WISP_GEOMETRY::CSize size(640, 480);

    if (g_GameState < GS_GAME)
    {
        if (g_GL.SceneScale > 0.1f)
            unitsPerPixel = 1.0f / g_GL.SceneScale;
    }
    else
    {
        const WISP_GEOMETRY::CSize window = g_OrionWindow.GetSize();

        // The gump is drawn magnified like every other, and the key labels are
        // fixed-size bitmap glyphs - so the only way to get letters a person can
        // read is to let that magnification do it, and measure the keys in the
        // smaller coordinates it is applied to.
        const float scale = InterfaceScale();
        size = WISP_GEOMETRY::CSize(
            (int)(window.Width / scale), (int)(window.Height / scale));
    }

    const int shorter = (size.Width < size.Height) ? size.Width : size.Height;

    const int minWidth = (int)(44.0f * unitsPerPixel);
    const int minHeight = (int)(38.0f * unitsPerPixel);
    const int maxWidth = (int)(130.0f * unitsPerPixel);

    KeyWidth = size.Width / 24;

    if (KeyWidth < minWidth)
        KeyWidth = minWidth;
    else if (KeyWidth > maxWidth)
        KeyWidth = maxWidth;

    KeyHeight = (shorter / 16 < KeyWidth) ? shorter / 16 : KeyWidth;

    if (KeyHeight < minHeight)
        KeyHeight = minHeight;

    KeyGap = KeyWidth / 10;
    if (KeyGap < 3)
        KeyGap = 3;

    Border = KeyGap * 3;
}
//----------------------------------------------------------------------------------
void CGumpKeyboard::PreferredSize(int &width, int &height)
{
    CGumpKeyboard measure(0, 0);
    measure.MeasureKeys();

    // Returned in screen pixels, which is what a gump's position is measured
    // in even though its contents are drawn magnified from its own origin.
    const float scale = measure.InterfaceScale();

    width = (int)((measure.PanelWidth() + measure.Border * 2) * scale);
    height = (int)(
        (measure.Border * 2 + measure.GrabHeight() + 5 * (measure.KeyHeight + measure.KeyGap)) *
        scale);

    // Built only to be measured, and must not be left as the live one.
    g_GumpKeyboard = NULL;
}
//----------------------------------------------------------------------------------
CGumpKeyboard::CGumpKeyboard(short x, short y)
    : CGump(GT_KEYBOARD, 0, x, y)
{
    g_GumpKeyboard = this;
    m_Locker.Serial = ID_GK_CLOSE;
    MeasureKeys();
}
//----------------------------------------------------------------------------------
CGumpKeyboard::~CGumpKeyboard()
{
    g_GumpKeyboard = NULL;
}
//----------------------------------------------------------------------------------
// A key is a plate, an inset face and a label. Three cheap elements rather than
// art, because UO has no key caps to borrow and a resizepic per key would be
// thirty nine-slice backgrounds on every redraw.
void CGumpKeyboard::AddKey(int serial, int x, int y, int width, const string &label, uint fill)
{
    const int keyHeight = (serial == (int)ID_GK_CLOSE) ? GrabHeight() : KeyHeight;

    // CallOnMouseUp is what turns a coloured rectangle into a button. Without
    // it the click is taken for a press on a gump's background and the gump is
    // never told, which is why none of these did anything at all.
    CGUIColoredPolygone *edge = (CGUIColoredPolygone *)Add(
        new CGUIColoredPolygone(serial, 0, x, y, width, keyHeight, KeyEdge));
    edge->CallOnMouseUp = true;

    CGUIColoredPolygone *face = (CGUIColoredPolygone *)Add(
        new CGUIColoredPolygone(serial, 0, x + 1, y + 1, width - 2, keyHeight - 2, fill));
    face->CallOnMouseUp = true;

    // The label is drawn over the key and must never be selected: text is
    // hit-testable across its whole texture and is added after the key it sits
    // on, so it won every hit test and no key press ever reached the gump.
    // Unicode face 0 rather than 1: these are fixed-size bitmap glyphs and 1 is
    // the smaller of the two, which on a key is legible only if you know what it
    // says already.
    CGUIText *text = (CGUIText *)Add(new CGUIText(0x0481, x, y + (keyHeight - 20) / 2));
    text->CreateTextureW(0, ToWString(label), 30, width, TS_CENTER);
    text->Enabled = false;
}
//----------------------------------------------------------------------------------
// The chat modes, which otherwise have to be typed as a prefix before anything
// can be typed at all.
void CGumpKeyboard::AddModeRow(int y, int width)
{
    static const char *labels[] = { "Say", "Yell", "Whisp", "Emote", "Cmd", "Broad", "Party" };
    const int count = 7;
    const int keyWidth = (width - KeyGap * (count - 1)) / count;

    IFOR (i, 0, count)
    {
        const uint fill = ((int)i == m_SelectedMode) ? KeyFillActive : KeyFillWide;
        AddKey(
            ID_GK_MODE_BASE + (int)i,
            Border + (int)i * (keyWidth + KeyGap),
            y,
            keyWidth,
            labels[i],
            fill);
    }
}
//----------------------------------------------------------------------------------
// A bar to pick the whole thing up by. The keys all carry a serial, so a press
// on one is a key press and never a drag; this carries none, which is what the
// gump layer takes as "move me".
void CGumpKeyboard::AddGrabBar(int width)
{
    const int height = GrabHeight();
    const int closeWidth = height * 2;
    const int barWidth = width - closeWidth - KeyGap;

    // The bar carries a serial and asks to be moved rather than pressed. It has
    // to have one: a screen only delivers the mouse release when the thing under
    // it has a serial, so a bar with none was dragged and then snapped back the
    // moment the finger came off it.
    CGUIColoredPolygone *bar = (CGUIColoredPolygone *)Add(
        new CGUIColoredPolygone(ID_GK_MOVE, 0, Border, Border / 2, barWidth, height, KeyFillWide));
    bar->MoveOnDrag = true;

    // Three grooves, so it reads as something to be held.
    const int grooveWidth = barWidth / 6;
    const int grooveX = Border + (barWidth - grooveWidth) / 2;

    IFOR (i, 0, 3)
    {
        CGUIColoredPolygone *groove = (CGUIColoredPolygone *)Add(new CGUIColoredPolygone(
            ID_GK_MOVE,
            0,
            grooveX,
            Border / 2 + height / 2 - 3 + (int)i * 3,
            grooveWidth,
            1,
            KeyEdge));
        groove->MoveOnDrag = true;
    }

    AddKey(ID_GK_CLOSE, Border + barWidth + KeyGap, Border / 2, closeWidth, "X", KeyFillWide);
}
//----------------------------------------------------------------------------------
void CGumpKeyboard::UpdateContent()
{
    Clear();

    MeasureKeys();

    const char **rows = m_Symbols ? g_SymbolRows : g_Rows;

    // Ten keys across is what fixes the width; every other row is centred in it.
    const int width = PanelWidth();
    const int height = Border * 2 + GrabHeight() + 5 * (KeyHeight + KeyGap);

    Add(new CGUIColoredPolygone(0, 0, 0, 0, width + Border * 2, height, PanelFill));
    Add(new CGUIColoredPolygone(0, 0, 0, 0, width + Border * 2, 1, KeyEdge));

    AddGrabBar(width);

    int y = Border + GrabHeight();

    AddModeRow(y, width);
    y += KeyHeight + KeyGap;

    IFOR (row, 0, 3)
    {
        const string keys = rows[row];
        const int count = (int)keys.length();

        if (!count)
            continue;

        int x = Border + ((10 - count) * (KeyWidth + KeyGap)) / 2;

        IFOR (i, 0, count)
        {
            char ch = keys[i];

            if (!m_Symbols && (m_Shift || m_ShiftLock))
                ch = (char)toupper(ch);

            AddKey(ID_GK_CHAR_BASE + (uchar)ch, x, y, KeyWidth, string(1, ch), KeyFill);
            x += KeyWidth + KeyGap;
        }

        y += KeyHeight + KeyGap;
    }

    // The bottom row: the things that are not letters.
    const int wide = KeyWidth + KeyWidth / 2;
    int x = Border;

    AddKey(
        ID_GK_SHIFT,
        x,
        y,
        wide,
        m_ShiftLock ? "CAPS" : "Shift",
        (m_Shift || m_ShiftLock) ? KeyFillActive : KeyFillWide);
    x += wide + KeyGap;

    AddKey(ID_GK_SYMBOLS, x, y, wide, m_Symbols ? "abc" : "?123", KeyFillWide);
    x += wide + KeyGap;

    const int spaceWidth = width - (x - Border) - (wide * 2 + KeyGap * 2);
    AddKey(ID_GK_SPACE, x, y, spaceWidth, "space", KeyFill);
    x += spaceWidth + KeyGap;

    AddKey(ID_GK_BACKSPACE, x, y, wide, "Del", KeyFillWide);
    x += wide + KeyGap;

    AddKey(ID_GK_ENTER, x, y, wide, "Send", KeyFillWide);
}
//----------------------------------------------------------------------------------
// The space a keyboard position is measured in: the window in the world, the
// letterboxed scene before it.
static void KeyboardSpace(int &width, int &height)
{
    if (g_GameState < GS_GAME)
    {
        width = 640;
        height = 480;
        return;
    }

    const WISP_GEOMETRY::CSize size = g_OrionWindow.GetSize();
    width = size.Width;
    height = size.Height;
}
//----------------------------------------------------------------------------------
void CGumpKeyboard::RememberPlacement(int x, int y)
{
    int width = 0;
    int height = 0;
    KeyboardSpace(width, height);

    if (width <= 0 || height <= 0)
        return;

    float &placeX = (g_GameState < GS_GAME) ? PreWorldX : PlaceX;
    float &placeY = (g_GameState < GS_GAME) ? PreWorldY : PlaceY;

    placeX = (float)x / (float)width;
    placeY = (float)y / (float)height;
}
//----------------------------------------------------------------------------------
bool CGumpKeyboard::RecallPlacement(int &x, int &y)
{
    const float placeX = (g_GameState < GS_GAME) ? PreWorldX : PlaceX;
    const float placeY = (g_GameState < GS_GAME) ? PreWorldY : PlaceY;

    if (placeX < 0.0f || placeY < 0.0f)
        return false;

    int width = 0;
    int height = 0;
    KeyboardSpace(width, height);

    x = (int)(placeX * width);
    y = (int)(placeY * height);

    return true;
}
//----------------------------------------------------------------------------------
// Recorded here rather than where the drag is committed: in the world that is
// the gump manager and before it the screen, and this happens to be the one
// place that sees every frame whoever moved it.
void CGumpKeyboard::Draw()
{
    RememberPlacement(GetX(), GetY());

    CGump::Draw();
}
//----------------------------------------------------------------------------------
// Straight into the screen's own character handler, which is where a keystroke
// would have arrived from SDL. Nothing below this knows the difference.
void CGumpKeyboard::PressCharacter(wchar_t ch)
{
    if (g_CurrentScreen == NULL)
        return;

#if USE_WISP
    g_CurrentScreen->OnCharPress(ch, 0);
#else
    // Not OnCharPress: that overload is the Win32 one, and on the screens that
    // matter here - the login screen above all - its override lives inside
    // #if USE_WISP and does not exist in this build at all. A press fell through
    // to the base class, which forwards to a gump that has no text handling, and
    // vanished. Arriving as the SDL event a real key would produce is what every
    // screen on this platform actually listens for.
    SDL_TextInputEvent ev;
    SDL_memset(&ev, 0, sizeof(ev));
    ev.type = SDL_TEXTINPUT;

    // The keys are ASCII, which is its own UTF-8.
    ev.text[0] = (char)(ch & 0x7F);
    ev.text[1] = 0;

    g_CurrentScreen->OnTextInput(ev);
#endif
}
//----------------------------------------------------------------------------------
void CGumpKeyboard::PressKey(int key)
{
    if (g_CurrentScreen == NULL)
        return;

#if USE_WISP
    g_CurrentScreen->OnKeyDown(key, 0);
#else
    SDL_KeyboardEvent ev;
    SDL_memset(&ev, 0, sizeof(ev));
    ev.type = SDL_KEYDOWN;
    ev.state = SDL_PRESSED;
    ev.keysym.sym = (SDL_Keycode)key;

    g_CurrentScreen->OnKeyDown(ev);
#endif
}
//----------------------------------------------------------------------------------
void CGumpKeyboard::ApplyMode(int mode)
{
    m_SelectedMode = mode;

    if (g_GumpConsoleType != NULL)
    {
        // Let the mode gump own this: it already knows how to swap one prefix
        // for another without disturbing what has been typed.
        g_GumpConsoleType->SetSelectedType(mode);
        return;
    }

    // The mode is a prefix on the line being typed, so choosing one means taking
    // off whichever is already there before putting the new one on. Only adding
    // it - which is what this did - left an earlier prefix in place, and since
    // Say's prefix is nothing at all, picking Say changed nothing and the line
    // still went wherever the last mode pointed.
    wstring text = g_GameConsole.Data();

    IFOR (i, GCTT_YELL, GCTT_PARTY + 1)
    {
        const wstring &worn = g_ConsolePrefix[i];

        if (worn.length() != 0 && text.length() >= worn.length() &&
            text.compare(0, worn.length(), worn) == 0)
        {
            text.erase(0, worn.length());
            break;
        }
    }

    text.insert(0, g_ConsolePrefix[mode]);

    g_GameConsole.SetText(text);
    g_GameConsole.SetPos((int)text.length());
}
//----------------------------------------------------------------------------------
void CGumpKeyboard::GUMP_BUTTON_EVENT_C
{


    if (serial >= ID_GK_CHAR_BASE && serial < ID_GK_CHAR_BASE + 0x100)
    {
        PressCharacter((wchar_t)(serial - ID_GK_CHAR_BASE));

        // A single shift is spent on one letter, the way it is on every phone.
        if (m_Shift && !m_ShiftLock)
        {
            m_Shift = false;
            WantUpdateContent = true;
        }

        return;
    }

    if (serial >= ID_GK_MODE_BASE && serial < ID_GK_MODE_BASE + 16)
    {
        ApplyMode(serial - ID_GK_MODE_BASE);
        WantUpdateContent = true;
        return;
    }

    switch (serial)
    {
        case ID_GK_SHIFT:
        {
            // Tapped again while already shifted, it locks.
            if (m_ShiftLock)
            {
                m_ShiftLock = false;
                m_Shift = false;
            }
            else if (m_Shift)
            {
                m_ShiftLock = true;
                m_Shift = false;
            }
            else
                m_Shift = true;

            WantUpdateContent = true;
            break;
        }
        case ID_GK_SYMBOLS:
        {
            m_Symbols = !m_Symbols;
            WantUpdateContent = true;
            break;
        }
        case ID_GK_SPACE:
        {
            PressCharacter(L' ');
            break;
        }
        case ID_GK_BACKSPACE:
        {
            PressKey(VK_BACK);
            break;
        }
        case ID_GK_ENTER:
        {
            PressKey(VK_RETURN);
            break;
        }
        case ID_GK_CLOSE:
        {
            // Marked, not deleted. This runs from inside the walk over this
            // gump's own items, so destroying it here frees the list out from
            // under the loop that is still reading it.
            RemoveMark = true;
            break;
        }
        default:
            break;
    }
}
//----------------------------------------------------------------------------------

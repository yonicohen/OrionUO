/***********************************************************************************
**
** GumpKeyboard.h
**
** An on-screen keyboard, for a screen that has no other one.
**
** Android's own keyboard takes about half a landscape screen and cannot be told
** not to: the client has to give up its world view to make room, and leave the
** immersive mode it otherwise runs in. This is the same thing drawn as a gump -
** compact, movable, closed like any other gump, and it never disturbs the view
** it is drawn over.
**
** It carries the chat modes as well. Say, yell, whisper, emote and the rest are
** otherwise chosen by typing a prefix, which is a thing that has to be typed
** before you can type anything - so they belong on the keyboard itself.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GUMPKEYBOARD_H
#define GUMPKEYBOARD_H
//----------------------------------------------------------------------------------
class CGumpKeyboard : public CGump
{
private:
    // Which face of the keyboard is showing: letters, or numbers and symbols.
    bool m_Symbols{ false };
    // Next letter is capital. Tapped once for one letter, twice to lock.
    bool m_Shift{ false };
    bool m_ShiftLock{ false };

    int m_SelectedMode{ 0 };

    void AddKey(int serial, int x, int y, int width, const string &label, uint fill);
    int PanelWidth() const { return 10 * KeyWidth + 9 * KeyGap; }
    void AddModeRow(int y, int width);
    void AddGrabBar(int width);

    void PressCharacter(wchar_t ch);
    void PressKey(int key);
    void ApplyMode(int mode);

public:
    CGumpKeyboard(short x, short y);
    virtual ~CGumpKeyboard();

    // Sized against the screen rather than fixed: a key has to be something a
    // thumb can hit, and a phone's pixels are much smaller than a monitor's.
    int KeyWidth{ 40 };
    int KeyHeight{ 34 };
    int KeyGap{ 4 };
    int Border{ 12 };

    void MeasureKeys();

    // The whole keyboard's size, for placing it before one exists.
    static void PreferredSize(int &width, int &height);

    // Serial ranges. Characters are carried as the character itself so the key
    // table stays a table rather than a switch.
    static const int ID_GK_CHAR_BASE = 0x1000;
    static const int ID_GK_MODE_BASE = 0x2000;
    static const uint ID_GK_SHIFT;
    static const uint ID_GK_SYMBOLS;
    static const uint ID_GK_BACKSPACE;
    static const uint ID_GK_SPACE;
    static const uint ID_GK_ENTER;
    static const uint ID_GK_CLOSE;
    static const uint ID_GK_MOVE;

    int GrabHeight() const { return KeyHeight / 2; }

    virtual void UpdateContent();
    virtual void Draw();

    // Where it was last left, as a fraction of whatever space it was in. The
    // world and the login screens are measured differently, so they are
    // remembered separately. Negative means "never moved, use the default".
    static float PlaceX;
    static float PlaceY;
    static float PreWorldX;
    static float PreWorldY;

    static void RememberPlacement(int x, int y);
    static bool RecallPlacement(int &x, int &y);

    GUMP_BUTTON_EVENT_H;
};
//----------------------------------------------------------------------------------
extern CGumpKeyboard *g_GumpKeyboard;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------

/***********************************************************************************
**
** GumpContainer.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GUMPCONTAINER_H
#define GUMPCONTAINER_H
//----------------------------------------------------------------------------------
class CGumpContainer : public CGump
{
    bool IsGameBoard = false;

private:
    uint m_CorpseEyesTicks{ 0 };
    uchar m_CorpseEyesOffset{ 0 };

    CTextRenderer m_TextRenderer{ CTextRenderer() };

    CGUIGumppic *m_CorpseEyes{ NULL };
    CGUIDataBox *m_DataBox{ NULL };

protected:
    virtual void CalculateGumpState();

public:
    CGumpContainer(uint serial, uint id, short x, short y);
    virtual ~CGumpContainer();

    static const uint ID_GC_LOCK_MOVING;
    static const uint ID_GC_MINIMIZE;
    static const uint ID_GC_GRID_SCROLL_UP;
    static const uint ID_GC_GRID_SCROLL_DOWN;

    CGUIGumppic *m_BodyGump{ NULL };

    // Backing panel for grid mode, which replaces the container artwork because
    // that art is a picture of a specific bag with its own irregular interior.
    CGUIResizepic *m_GridBackground{ NULL };

    // Created once and shown only when the contents overflow; adding them in
    // UpdateContent would append a fresh pair on every rebuild.
    CGUIButton *m_GridScrollUp{ NULL };
    CGUIButton *m_GridScrollDown{ NULL };

    // A cell is one world tile plus a little breathing room, which is what the
    // item artwork is drawn at. Four by four of those, plus the border, comes to
    // 224 square - square rather than a long strip, and compact enough to leave
    // the game window room.
    static const int GridCellSize = 50;
    static const int GridColumns = 4;
    static const int GridBorder = 12;

    // Beyond this the panel would grow taller than most screens, so it scrolls
    // instead.
    static const int GridMaxRows = 4;

    // First visible row.
    int m_GridScrollRow{ 0 };

    bool UseGrid() const;

    // Stack size shortened to fit a cell: 15.0k rather than 15000.
    static string GridCountText(int count);

    void UpdateItemCoordinates(class CGameObject *item);

    CTextRenderer *GetTextRenderer() { return &m_TextRenderer; }

    virtual void PrepareTextures();

    virtual void PrepareContent();

    virtual void UpdateContent();

    virtual void InitToolTip();

    virtual void Draw();
    virtual CRenderObject *Select();

    GUMP_BUTTON_EVENT_H;

    virtual void OnLeftMouseButtonUp();
    virtual bool OnLeftMouseButtonDoubleClick();
};
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------

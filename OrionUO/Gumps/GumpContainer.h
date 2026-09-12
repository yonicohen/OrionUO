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
#include <SDL_events.h>
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
    static const uint ID_GC_GRID_RESIZE;
    static const uint ID_GC_GRID_SEARCH;

    CGUIGumppic *m_BodyGump{ NULL };

    // Backing panel for grid mode, which replaces the container artwork because
    // that art is a picture of a specific bag with its own irregular interior.
    CGUIResizepic *m_GridBackground{ NULL };

    // Created once and shown only when the contents overflow; adding them in
    // UpdateContent would append a fresh pair on every rebuild.
    CGUIButton *m_GridScrollUp{ NULL };
    CGUIButton *m_GridScrollDown{ NULL };

    // The corner handle, for pulling the panel to whatever shape suits the bag,
    // on a plate that gives it something to be seen and aimed at.
    CGUIResizeButton *m_GridResizer{ NULL };
    CGUIColoredPolygone *m_GridResizerPlate{ NULL };

    static const int GridResizerPlate = 30;

    // The cell edges. They cannot live in the item box: that is drawn with the
    // hue colouriser bound, and an untextured quad drawn through a shader that
    // expects a texture comes out as nothing at all.
    CGUIDataBox *m_GridLines{ NULL };

    // Searching the bag by what is written on its contents - the item's name and
    // whatever properties the server has sent for it.
    CGUITextEntry *m_GridSearch{ NULL };
    // Both live at the gump's top level, beside the entry itself: the click
    // that focuses a field is resolved by looking for an entry with the same
    // serial among its siblings, and a hint added per update would be added
    // again on every update.
    CGUIColoredPolygone *m_GridSearchBack{ NULL };
    CGUIText *m_GridSearchHint{ NULL };

    bool MatchesSearch(class CGameItem *item) const;
    bool Searching() const;

    // Every matching item in the bag and in the bags inside it, depth first.
    void CollectMatches(class CGameItem *parent, std::vector<class CGameItem *> &out) const;

    static const int GridSearchHeight = 24;

    // Keeps the cells inside the panel while it is scrolled, so a row at the
    // edge is cut off rather than vanishing whole.
    CGUIScissor *m_GridScissorOn{ NULL };
    CGUIScissor *m_GridScissorOff{ NULL };

    // A cell is one world tile plus a little breathing room, which is what the
    // item artwork is drawn at.
    static const int GridCellSize = 50;
    static const int GridBorder = 12;

    // How many cells across and down, which the corner handle changes. Four by
    // four is square rather than a long strip, and compact enough to leave the
    // game window room.
    static const int GridMinColumns = 2;
    static const int GridMaxColumns = 12;
    static const int GridMinRows = 2;
    static const int GridMaxRows = 10;

    // Taken from the shared setting so every bag opens the same shape, and
    // written back when this one's handle is dragged.
    int GridColumns{ 4 };
    int GridRows{ 4 };

    // Shape while a resize is in progress; zero when one is not.
    int m_StartResizeColumns{ 0 };
    int m_StartResizeRows{ 0 };

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
    GUMP_TEXT_ENTRY_EVENT_H;
    GUMP_RESIZE_START_EVENT_H;
    GUMP_RESIZE_EVENT_H;
    GUMP_RESIZE_END_EVENT_H;

    virtual void OnCharPress(const WPARAM &wParam, const LPARAM &lParam);
    virtual void OnKeyDown(const WPARAM &wParam, const LPARAM &lParam);
#if !USE_WISP
    virtual void OnTextInput(const SDL_TextInputEvent &ev) override;
    virtual void OnKeyDown(const SDL_KeyboardEvent &ev) override;
#endif

    virtual void OnLeftMouseButtonUp();
    virtual bool OnLeftMouseButtonDoubleClick();
};
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------

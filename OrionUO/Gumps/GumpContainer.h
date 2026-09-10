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

    CGUIGumppic *m_BodyGump{ NULL };

    // Backing panel for grid mode, which replaces the container artwork because
    // that art is a picture of a specific bag with its own irregular interior.
    CGUIResizepic *m_GridBackground{ NULL };

    // A cell is one world tile plus a little breathing room, which is what the
    // item artwork is drawn at.
    static const int GridCellSize = 50;
    static const int GridColumns = 10;
    static const int GridBorder = 12;

    bool UseGrid() const;

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

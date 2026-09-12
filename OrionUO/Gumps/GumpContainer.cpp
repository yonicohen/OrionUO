// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** GumpContainer.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
const uint CGumpContainer::ID_GC_LOCK_MOVING = 0xFFFFFFFE;
const uint CGumpContainer::ID_GC_MINIMIZE = 0xFFFFFFFF;
const uint CGumpContainer::ID_GC_GRID_SCROLL_UP = 0xFFFFFFFD;
const uint CGumpContainer::ID_GC_GRID_SCROLL_DOWN = 0xFFFFFFFC;
const uint CGumpContainer::ID_GC_GRID_RESIZE = 0xFFFFFFFB;
const uint CGumpContainer::ID_GC_GRID_SEARCH = 0xFFFFFFFA;
//----------------------------------------------------------------------------------
CGumpContainer::CGumpContainer(uint serial, uint id, short x, short y)
    : CGump(GT_CONTAINER, serial, x, y)
    , IsGameBoard(id == 0x091A || id == 0x092E)
{
    WISPFUN_DEBUG("c93_f1");
    Page = 1;
    m_Locker.Serial = ID_GC_LOCK_MOVING;
    ID = id;

    Add(new CGUIPage(1));
    Add(new CGUIGumppic(0x0050, 0, 0));

    Add(new CGUIPage(2));

    m_BodyGump = (CGUIGumppic *)Add(new CGUIGumppic((ushort)ID, 0, 0));

    if (ID == 0x0009)
    {
        if (m_CorpseEyesTicks < g_Ticks)
        {
            m_CorpseEyesOffset = (BYTE)!m_CorpseEyesOffset;
            m_CorpseEyesTicks = g_Ticks + 750;
        }

        m_CorpseEyes = (CGUIGumppic *)Add(new CGUIGumppic(0x0045, 45, 30));
    }

    if (ID == 0x003C)
    {
        CGUIHitBox *box = (CGUIHitBox *)Add(new CGUIHitBox(ID_GC_MINIMIZE, 106, 162, 16, 16, true));
        box->ToPage = 1;
    }

    // Added before the item box so it draws behind the contents. Size is set in
    // UpdateContent once the item count is known.
    m_GridBackground =
        (CGUIResizepic *)Add(new CGUIResizepic(
            0, g_GridContainerBackground, 0, 0, GridCellSize, GridCellSize));
    m_GridBackground->Visible = false;

    m_GridScrollUp = (CGUIButton *)Add(
        new CGUIButton(ID_GC_GRID_SCROLL_UP, 0x0824, 0x0824, 0x0824, 0, 0));
    m_GridScrollUp->Visible = false;

    m_GridScrollDown = (CGUIButton *)Add(
        new CGUIButton(ID_GC_GRID_SCROLL_DOWN, 0x0825, 0x0825, 0x0825, 0, 0));
    m_GridScrollDown->Visible = false;

    // The corner handle. Same art the world map uses for the same job, so it
    // reads as the same thing.
    m_GridResizerPlate = (CGUIColoredPolygone *)Add(
        new CGUIColoredPolygone(0, 0, 0, 0, GridResizerPlate, GridResizerPlate, 0x30326A8E));
    m_GridResizerPlate->Visible = false;

    m_GridResizer = (CGUIResizeButton *)Add(
        new CGUIResizeButton(ID_GC_GRID_RESIZE, 0x0837, 0x0838, 0x0838, 0, 0));
    m_GridResizer->Visible = false;
#if defined(__ANDROID__)
    m_GridResizer->HitPadding = GridResizerPlate / 2;
#endif

    m_GridSearchBack = (CGUIColoredPolygone *)Add(
        new CGUIColoredPolygone(ID_GC_GRID_SEARCH, 0, 0, 0, 10, 10, 0x60000000));
    m_GridSearchBack->Visible = false;

    m_GridSearchHint = (CGUIText *)Add(new CGUIText(0x0386, 0, 0));
    m_GridSearchHint->CreateTextureW(1, L"Search", 30, 100, TS_LEFT);
    m_GridSearchHint->Visible = false;
    // Drawn but never selected. Text is hit-testable across its whole texture
    // and this one is added after the field's backing, so it won every hit test
    // and swallowed the click that was supposed to focus the field.
    m_GridSearchHint->Enabled = false;

    m_GridSearch = (CGUITextEntry *)Add(new CGUITextEntry(
        ID_GC_GRID_SEARCH, 0x0481, 0x0481, 0x0481, 0, 0, 0, false, 1));
    m_GridSearch->CheckOnSerial = true;
    m_GridSearch->m_Entry.MaxLength = 32;
    m_GridSearch->Visible = false;


    m_GridLines = (CGUIDataBox *)Add(new CGUIDataBox());

    Add(new CGUIShader(&g_ColorizerShader, true));

    // The cells are clipped to the panel so a row at the edge of a scrolled view
    // is cut off rather than disappearing whole, and so nothing is ever drawn
    // outside the panel it belongs to.
    m_GridScissorOn = (CGUIScissor *)Add(new CGUIScissor(true, 0, 0, 0, 0, 1, 1));
    m_GridScissorOn->Visible = false;

    m_DataBox = (CGUIDataBox *)Add(new CGUIDataBox());

    m_GridScissorOff = (CGUIScissor *)Add(new CGUIScissor(false, 0, 0, 0, 0, 0, 0));
    m_GridScissorOff->Visible = false;

    Add(new CGUIShader(&g_ColorizerShader, false));
}
//----------------------------------------------------------------------------------
CGumpContainer::~CGumpContainer()
{
    // The focused entry is a raw pointer into this gump's own field, and the
    // world keeps typing into whatever it points at.
    if (m_GridSearch != NULL && g_EntryPointer == &m_GridSearch->m_Entry)
        g_EntryPointer = &g_GameConsole;
}
//----------------------------------------------------------------------------------
string CGumpContainer::GridCountText(int count)
{
    // A cell is 50 pixels wide, so five figures of gold will not fit. Shorten the
    // way the rest of the interface does: 15.0k rather than 15000.
    if (count < 1000)
        return std::to_string(count);

    if (count < 1000000)
    {
        char buffer[32] = { 0 };
        sprintf_s(buffer, "%.1fk", count / 1000.0f);
        return string(buffer);
    }

    char buffer[32] = { 0 };
    sprintf_s(buffer, "%.1fm", count / 1000000.0f);
    return string(buffer);
}
//----------------------------------------------------------------------------------
// Matched against the item's name and against whatever the server has said
// about it, so "vanq" or "exceptional" finds things a name never would.
bool CGumpContainer::MatchesSearch(CGameItem *item) const
{
    if (m_GridSearch == NULL || m_GridSearch->m_Entry.Length() == 0)
        return true;

    const wstring needle = ToLowerW(m_GridSearch->m_Entry.Data());

    if (needle.length() == 0)
        return true;

    const wstring name = ToLowerW(ToWString(item->GetName()));

    if (name.length() != 0 && name.find(needle) != wstring::npos)
        return true;

    // UO sends an item's name and properties only when something asks for them -
    // hovering it, or picking it up - so a bag that has just been opened has
    // nothing to match against and the search looked broken. The tile data name
    // is in the client's own files and is there from the start: it is what the
    // server would call the thing anyway, short of a custom name.
    const ushort graphic = item->Graphic;

    if (graphic < g_Orion.m_StaticData.size())
    {
        const wstring tileName = ToLowerW(ToWString(g_Orion.m_StaticData[graphic].Name));

        if (tileName.length() != 0 && tileName.find(needle) != wstring::npos)
            return true;
    }

    const CObjectProperty *properties = g_ObjectPropertiesManager.Get(item->Serial);

    if (properties != NULL)
    {
        if (ToLowerW(properties->Name).find(needle) != wstring::npos)
            return true;

        if (ToLowerW(properties->Data).find(needle) != wstring::npos)
            return true;
    }

    return false;
}
//----------------------------------------------------------------------------------
bool CGumpContainer::Searching() const
{
    return (m_GridSearch != NULL && m_GridSearch->m_Entry.Length() != 0);
}
//----------------------------------------------------------------------------------
// A bag holds bags, and "where is my dagger" should not depend on remembering
// which one it went into. While a search is running the grid shows matches from
// the whole tree rather than only this container's own contents.
//
// Only what the client already knows: UO sends a container's contents when it is
// opened, so a pouch that has never been opened this session has nothing in it
// to find.
void CGumpContainer::CollectMatches(CGameItem *parent, std::vector<CGameItem *> &out) const
{
    if (parent == NULL)
        return;

    QFOR(item, parent->m_Items, CGameItem *)
    {
        if (item->Count <= 0)
            continue;

        // Worn layers belong to the wearer, not to the bag - except on a corpse,
        // where the client shows what it was carrying.
        if (item->Layer != OL_NONE && !(parent->IsCorpse() && LAYER_UNSAFE[item->Layer]))
            continue;

        if (MatchesSearch(item))
            out.push_back(item);

        // Anything the client already holds contents for, rather than anything
        // the tile data calls a container: the flag is what the art means, and
        // what matters here is whether there is something inside to look at.
        if (item->m_Items != NULL)
            CollectMatches(item, out);
    }
}
//----------------------------------------------------------------------------------
bool CGumpContainer::UseGrid() const
{
    // The game boards place their pieces meaningfully - a chess piece's position
    // is the state of the game - so they are never gridded.
    return g_ConfigManager.GetUseGridContainers() && !IsGameBoard;
}
//----------------------------------------------------------------------------------
void CGumpContainer::UpdateItemCoordinates(CGameObject *item)
{
    WISPFUN_DEBUG("c93_f3");

    // In grid mode the item's own coordinates are not used for drawing, so there
    // is nothing to clamp into the artwork's interior.
    if (UseGrid())
        return;

    if (Graphic < g_ContainerOffset.size())
    {
        const CContainerOffsetRect &rect = g_ContainerOffset[Graphic].Rect;

        if (item->GetX() < rect.MinX)
            item->SetX(rect.MinX);

        if (item->GetY() < rect.MinY)
            item->SetY(rect.MinY);

        if (item->GetX() > rect.MinX + rect.MaxX)
            item->SetX(rect.MinX + rect.MaxX);

        if (item->GetY() > rect.MinY + rect.MaxY)
            item->SetY(rect.MinY + rect.MaxY);
    }
}
//----------------------------------------------------------------------------------
void CGumpContainer::CalculateGumpState()
{
    WISPFUN_DEBUG("c93_f4");
    CGump::CalculateGumpState();

    if (g_GumpPressed && g_PressedObject.LeftObject != NULL && g_PressedObject.LeftObject->IsText())
    {
        g_GumpMovingOffset.Reset();

        if (Minimized)
        {
            g_GumpTranslate.X = (float)MinimizedX;
            g_GumpTranslate.Y = (float)MinimizedY;
        }
        else
        {
            g_GumpTranslate.X = (float)m_X;
            g_GumpTranslate.Y = (float)m_Y;
        }
    }
}
//----------------------------------------------------------------------------------
void CGumpContainer::PrepareTextures()
{
    WISPFUN_DEBUG("c93_f5");
    CGump::PrepareTextures();
    g_Orion.ExecuteGumpPart(0x0045, 2); //Corpse eyes
}
//----------------------------------------------------------------------------------
void CGumpContainer::InitToolTip()
{
    WISPFUN_DEBUG("c93_f6");
    if (!Minimized)
    {
        if (g_SelectedObject.Serial == ID_GC_MINIMIZE)
            g_ToolTip.Set(L"Minimize the container gump");
        else if (g_SelectedObject.Serial == ID_GC_LOCK_MOVING)
            g_ToolTip.Set(L"Lock moving/closing the container gump");
    }
    else
        g_ToolTip.Set(L"Double click to maximize container gump");
}
//----------------------------------------------------------------------------------
void CGumpContainer::PrepareContent()
{
    WISPFUN_DEBUG("c93_f7");
    if (!g_Player->Dead() &&
        GetTopObjDistance(g_Player, g_World->FindWorldObject(Serial)) <= DRAG_ITEMS_DISTANCE &&
        g_PressedObject.LeftGump == this && !g_ObjectInHand.Enabled &&
        g_PressedObject.LeftSerial != ID_GC_MINIMIZE &&
        g_MouseManager.LastLeftButtonClickTimer < g_Ticks)
    {
        WISP_GEOMETRY::CPoint2Di offset = g_MouseManager.LeftDroppedOffset();

        if (CanBeDraggedByOffset(offset) ||
            (g_MouseManager.LastLeftButtonClickTimer + g_MouseManager.DoubleClickDelay < g_Ticks))
        {
            CGameItem *selobj = g_World->FindWorldItem(g_PressedObject.LeftSerial);

            if (selobj != NULL && selobj->IsStackable() && selobj->Count > 1 && !g_ShiftPressed)
            {
                CGumpDrag *newgump = new CGumpDrag(
                    g_PressedObject.LeftSerial,
                    g_MouseManager.Position.X - 80,
                    g_MouseManager.Position.Y - 34);

                g_GumpManager.AddGump(newgump);
                g_OrionWindow.EmulateOnLeftMouseButtonDown();
                selobj->Dragged = true;
            }
            else if (selobj != NULL)
            {
                //if (g_Target.IsTargeting())
                //	g_Target.SendCancelTarget();

                g_Orion.PickupItem(selobj, 0, IsGameBoard);

                g_PressedObject.ClearLeft();

                WantRedraw = true;
            }
        }
    }

    if (ID == 0x09 && m_CorpseEyes != NULL)
    {
        if (m_CorpseEyesTicks < g_Ticks)
        {
            m_CorpseEyesOffset = (uchar)!m_CorpseEyesOffset;
            m_CorpseEyesTicks = g_Ticks + 750;

            m_CorpseEyes->Graphic = 0x0045 + m_CorpseEyesOffset;
            WantRedraw = true;
        }
    }

    if (Minimized)
    {
        if (Page != 1)
        {
            Page = 1;
            WantUpdateContent = true;
        }
    }
    else
    {
        if (Page != 2)
        {
            Page = 2;
            WantUpdateContent = true;
        }

        if (m_TextRenderer.CalculatePositions(false))
            WantRedraw = true;
    }
}
//----------------------------------------------------------------------------------
void CGumpContainer::UpdateContent()
{
    WISPFUN_DEBUG("c93_f8");
    CGameItem *container = g_World->FindWorldItem(Serial);

    if (container == NULL)
        return;

    if ((ushort)ID == 0x003C)
    {
        ushort graphic = (ushort)ID;

        CGameItem *backpack = g_Player->FindLayer(OL_BACKPACK);

        if (backpack != NULL && backpack->Serial == Serial)
        {
            switch (g_ConfigManager.GetCharacterBackpackStyle())
            {
                case CBS_SUEDE:
                    graphic = 0x775E;
                    break;
                case CBS_POLAR_BEAR:
                    graphic = 0x7760;
                    break;
                case CBS_GHOUL_SKIN:
                    graphic = 0x7762;
                    break;
                default:
                    graphic = 0x003C;
                    break;
            }

            if (g_Orion.ExecuteGump(graphic) == NULL)
                graphic = 0x003C;

            m_BodyGump->Graphic = graphic;
        }
    }

    m_DataBox->Clear();
    m_GridLines->Clear();

    IsGameBoard = (ID == 0x091A || ID == 0x092E);

    const bool grid = UseGrid();
    int gridIndex = 0;

    // What the panel is showing. Normally the bag's own contents; while a search
    // is running, every match in the bag and in the bags inside it.
    std::vector<CGameItem *> shown;

    if (grid && Searching())
        CollectMatches(container, shown);
    else
    {
        QFOR(obj, container->m_Items, CGameItem *)
        {
            if (obj->Count > 0 &&
                (obj->Layer == OL_NONE ||
                 (container->IsCorpse() && LAYER_UNSAFE[obj->Layer])))
            {
                shown.push_back(obj);
            }
        }
    }

    if (grid)
    {
        // Not while the handle is being dragged: this gump is the one deciding
        // the shape until the finger comes off it.
        if (!m_StartResizeColumns)
        {
            GridColumns = g_ConfigManager.GridContainerColumns;
            GridRows = g_ConfigManager.GridContainerRows;
        }

        if (GridColumns < GridMinColumns)
            GridColumns = GridMinColumns;
        else if (GridColumns > GridMaxColumns)
            GridColumns = GridMaxColumns;

        if (GridRows < GridMinRows)
            GridRows = GridMinRows;
        else if (GridRows > GridMaxRows)
            GridRows = GridMaxRows;

        // Size the panel to the contents before laying anything out. Counting
        // first costs one pass and avoids a panel that lags a frame behind.
        const int itemCount = (int)shown.size();

        const int rows = (itemCount + GridColumns - 1) / GridColumns;
        const int visibleRows = (rows < GridRows) ? rows : GridRows;

        // Clamp here rather than when scrolling: the contents can shrink under a
        // scrolled view at any time, when something is moved out or used up.
        const int maxScroll = (rows > visibleRows) ? (rows - visibleRows) : 0;
        if (m_GridScrollRow > maxScroll)
            m_GridScrollRow = maxScroll;
        if (m_GridScrollRow < 0)
            m_GridScrollRow = 0;

        // A fixed square, not a panel that shrinks to whatever is in the bag: a
        // container that changes shape every time an item is picked up is worse
        // to use than one that keeps its place on screen.
        const int gridTop = GridBorder + GridSearchHeight;

        m_GridBackground->Width = GridColumns * GridCellSize + GridBorder * 2;
        m_GridBackground->Height = GridRows * GridCellSize + gridTop + GridBorder;

        // The search field runs the width of the panel, less room for the
        // highlight/only toggle beside it.
        m_GridSearch->Visible = true;
        m_GridSearch->SetX(GridBorder + 2);
        m_GridSearch->SetY(GridBorder);


        // Carries the field's serial: a text entry is only clickable where its
        // text is drawn, and an empty search box has no text at all.
        const int fieldWidth = GridColumns * GridCellSize;

        m_GridSearchBack->Visible = true;
        m_GridSearchBack->SetX(GridBorder);
        m_GridSearchBack->SetY(GridBorder - 2);
        m_GridSearchBack->Width = fieldWidth;
        m_GridSearchBack->Height = GridSearchHeight - 2;

        // Says what the field is for while there is nothing in it.
        m_GridSearchHint->Visible = (m_GridSearch->m_Entry.Length() == 0);
        m_GridSearchHint->SetX(GridBorder + 4);
        m_GridSearchHint->SetY(GridBorder);



        // Without cell edges the panel reads as items scattered on a blank sheet
        // rather than a grid. Lines rather than a box per cell: fourteen thin
        // rectangles instead of thirty-six outlines.
        // UO's own gump gold, kept faint: the cells have to read as cells
        // without competing with what is in them.
        const uint lineColor = 0x50326A8E;
        const int gridWidth = GridColumns * GridCellSize;
        const int gridHeight = GridRows * GridCellSize;

        for (int column = 0; column <= GridColumns; column++)
        {
            m_GridLines->Add(new CGUIColoredPolygone(
                0,
                0,
                GridBorder + column * GridCellSize,
                gridTop,
                1,
                gridHeight,
                lineColor));
        }

        for (int row = 0; row <= GridRows; row++)
        {
            m_GridLines->Add(new CGUIColoredPolygone(
                0,
                0,
                GridBorder,
                gridTop + row * GridCellSize,
                gridWidth,
                1,
                lineColor));
        }

        const int buttonX = m_GridBackground->Width - 4;
        m_GridScrollUp->Visible = (maxScroll > 0);
        m_GridScrollDown->Visible = (maxScroll > 0);
        m_GridScrollUp->SetX(buttonX);
        m_GridScrollUp->SetY(gridTop);
        m_GridScrollDown->SetX(buttonX);
        m_GridScrollDown->SetY(m_GridBackground->Height - GridBorder - 14);

        const int handleX = m_GridBackground->Width - 6;
        const int handleY = m_GridBackground->Height - 6;

        m_GridResizer->Visible = true;
        m_GridResizer->SetX(handleX);
        m_GridResizer->SetY(handleY);

        // Only where a fingertip has to find it. With a mouse the eight pixel
        // handle is target enough, and a tinted square under it is just a smudge.
#if defined(__ANDROID__)
        m_GridResizerPlate->Visible = true;
#else
        m_GridResizerPlate->Visible = false;
#endif
        m_GridResizerPlate->SetX(handleX - GridResizerPlate / 2 + 4);
        m_GridResizerPlate->SetY(handleY - GridResizerPlate / 2 + 4);

        m_GridScissorOn->Visible = true;
        m_GridScissorOn->SetX(GridBorder);
        m_GridScissorOn->SetY(gridTop);
        m_GridScissorOn->Width = gridWidth;
        m_GridScissorOn->Height = gridHeight;
        m_GridScissorOff->Visible = true;
    }
    else
    {
        m_GridResizer->Visible = false;
        m_GridResizerPlate->Visible = false;
        m_GridSearch->Visible = false;
        m_GridSearchBack->Visible = false;
        m_GridSearchHint->Visible = false;
        m_GridScissorOn->Visible = false;
        m_GridScissorOff->Visible = false;
    }

    m_GridBackground->Graphic = g_GridContainerBackground;
    m_GridBackground->Visible = grid;

    if (!grid)
    {
        m_GridScrollUp->Visible = false;
        m_GridScrollDown->Visible = false;
    }

    // The container artwork is a picture of one particular bag, with its own
    // irregular interior; a grid replaces it rather than sits inside it.
    if (m_BodyGump != NULL)
        m_BodyGump->Visible = !grid;

    for (CGameItem *obj : shown)
    {
        {
            bool doubleDraw = false;
            ushort graphic = obj->GetDrawGraphic(doubleDraw);
            CGUIGumppicHightlighted *item = NULL;

            // Grid mode only changes where the widget goes. It is the same widget,
            // carrying the same serial, so dragging, dropping, double-click and
            // tooltips all keep working without knowing about any of this.
            int drawX = obj->GetX();
            int drawY = obj->GetY();

            if (grid)
            {
                const int gridTop = GridBorder + GridSearchHeight;
                const int row = gridIndex / GridColumns - m_GridScrollRow;
                const int cellX = GridBorder + (gridIndex % GridColumns) * GridCellSize;
                const int cellY = gridTop + row * GridCellSize;

                drawX = cellX;
                drawY = cellY;
                gridIndex++;

                // Item art varies in size - a ring is tiny, a halberd is not - and
                // the widget draws from its top left, so without this everything
                // huddles against the top left corner of its cell instead of
                // sitting in it.
                CGLTexture *art = g_Orion.ExecuteStaticArt(graphic);
                if (art != NULL)
                {
                    drawX += (GridCellSize - art->Width) / 2;
                    drawY += (GridCellSize - art->Height) / 2;
                }

                // Wholly outside the visible window. A row at the edge is left
                // in and cut off by the scissor, which is what makes scrolling
                // read as movement rather than as rows blinking in and out.
                if (row < -1 || row > GridRows)
                    continue;
            }

            if (IsGameBoard)
            {
                item = (CGUIGumppicHightlighted *)m_DataBox->Add(new CGUIGumppicHightlighted(
                    obj->Serial,
                    graphic - GAME_FIGURE_GUMP_OFFSET,
                    obj->Color & 0x3FFF,
                    0x0035,
                    drawX,
                    drawY - 20));
                item->PartialHue = false;
            }
            else
            {
                item = (CGUIGumppicHightlighted *)m_DataBox->Add(new CGUITilepicHightlighted(
                    obj->Serial,
                    graphic,
                    obj->Color & 0x3FFF,
                    0x0035,
                    drawX,
                    drawY,
                    doubleDraw));
                item->PartialHue = IsPartialHue(g_Orion.GetStaticFlags(graphic));
            }

            // A stack's size is invisible in the classic layout - the artwork is
            // the same whether it is one arrow or a thousand - and you had to
            // hover each one to find out. A cell has room to just say so.
            if (grid && obj->Count > 1)
            {
                // Positioned against the cell, not the art, which is centred and
                // so starts at a different place for every item. Font 1 rather
                // than 0: the large runic face overflowed into the next cell.
                const int cellX = GridBorder + ((gridIndex - 1) % GridColumns) * GridCellSize;
                const int cellY = GridBorder + GridSearchHeight +
                                  ((gridIndex - 1) / GridColumns - m_GridScrollRow) * GridCellSize;

                CGUIText *countText =
                    (CGUIText *)m_DataBox->Add(new CGUIText(0x0481, cellX, cellY + GridCellSize - 16));
                // Font 0. Font 9 renders nothing here - this was already found once and
                // then undone by a careless edit.
                // Unicode rather than the ASCII faces: font 0 there is the large
                // runic one that overflowed the cell, and 1 and 9 render nothing
                // at all in this gump. The unicode face is small and legible.
                // Unicode face 1 is a size down from 0, which is the one used for
                // body text and was bigger than a stack label needs. Given the
                // cell's width and TS_CENTER it centres itself along the bottom,
                // rather than being pinned to a corner.
                countText->CreateTextureW(
                    1, ToWString(GridCountText(obj->Count)), 30, GridCellSize, TS_CENTER);
            }
        }
    }
}
//----------------------------------------------------------------------------------
void CGumpContainer::GUMP_RESIZE_START_EVENT_C
{
    m_StartResizeColumns = GridColumns;
    m_StartResizeRows = GridRows;
}
//----------------------------------------------------------------------------------
void CGumpContainer::GUMP_RESIZE_EVENT_C
{
    if (!m_StartResizeColumns || !m_StartResizeRows)
        return;

    // Whole cells rather than pixels: a panel two thirds of a cell wide has a
    // column that can never hold anything. Rounding rather than truncating so
    // the handle follows the finger instead of lagging half a cell behind it.
    const WISP_GEOMETRY::CPoint2Di offset = g_MouseManager.LeftDroppedOffset();

    const int columns =
        m_StartResizeColumns + (int)floor(offset.X / (float)GridCellSize + 0.5f);
    const int rows = m_StartResizeRows + (int)floor(offset.Y / (float)GridCellSize + 0.5f);

    const int wasColumns = GridColumns;
    const int wasRows = GridRows;

    GridColumns = (columns < GridMinColumns) ? GridMinColumns
                                             : (columns > GridMaxColumns ? GridMaxColumns
                                                                         : columns);
    GridRows = (rows < GridMinRows) ? GridMinRows : (rows > GridMaxRows ? GridMaxRows : rows);

    if (GridColumns != wasColumns || GridRows != wasRows)
    {
        g_ConfigManager.GridContainerColumns = GridColumns;
        g_ConfigManager.GridContainerRows = GridRows;

        // Every open bag follows, so the shape is one decision rather than one
        // per container.
        QFOR(gump, g_GumpManager.m_Items, CGump *)
        {
            if (gump->GumpType == GT_CONTAINER)
            {
                gump->WantUpdateContent = true;
                gump->WantRedraw = true;
            }
        }
    }
}
//----------------------------------------------------------------------------------
void CGumpContainer::GUMP_RESIZE_END_EVENT_C
{
    m_StartResizeColumns = 0;
    m_StartResizeRows = 0;

    // The shape is part of where a player has put their bags, so it is saved
    // with the rest of the profile rather than forgotten on the next login.
    g_Orion.SaveLocalConfig(g_PacketManager.ConfigSerial);
}
//----------------------------------------------------------------------------------
void CGumpContainer::Draw()
{
    WISPFUN_DEBUG("c93_f9");
    CGump::Draw();

    if (!Minimized)
    {
        g_GLMatrix.Translate(g_GumpTranslate.X, g_GumpTranslate.Y, 0.0f);

        g_FontColorizerShader.Use();

        m_TextRenderer.Draw();

        UnuseShader();

        g_GLMatrix.Translate(-g_GumpTranslate.X, -g_GumpTranslate.Y, 0.0f);
    }
}
//----------------------------------------------------------------------------------
CRenderObject *CGumpContainer::Select()
{
    WISPFUN_DEBUG("c93_f10");
    CRenderObject *selected = CGump::Select();

    if (!Minimized)
    {
        WISP_GEOMETRY::CPoint2Di oldPos = g_MouseManager.Position;
        g_MouseManager.Position = WISP_GEOMETRY::CPoint2Di(
            oldPos.X - (int)g_GumpTranslate.X, oldPos.Y - (int)g_GumpTranslate.Y);

        m_TextRenderer.Select(this);

        g_MouseManager.Position = oldPos;
    }

    return selected;
}
//----------------------------------------------------------------------------------
// Clicking the field is what focuses it. Nothing in the client does this on its
// own: a gump that wants an entry focused says so, which is why typing went to
// the world instead however hard the box was clicked.
void CGumpContainer::GUMP_TEXT_ENTRY_EVENT_C
{

    if (m_GridSearch == NULL || serial != (int)ID_GC_GRID_SEARCH)
        return;

    g_EntryPointer = &m_GridSearch->m_Entry;
    m_GridSearch->Focused = true;
    WantRedraw = true;

#if defined(__ANDROID__)
    // There is no hardware keyboard to start typing on.
    g_OrionWindow.ToggleKeyboardGump();
#endif
}
//----------------------------------------------------------------------------------
// Typing goes to the search field, and only while it is the focused entry -
// otherwise every letter meant for the world would filter a bag instead.
void CGumpContainer::OnCharPress(const WPARAM &wParam, const LPARAM &lParam)
{

    if (m_GridSearch == NULL || g_EntryPointer != &m_GridSearch->m_Entry)
        return;

    if (g_EntryPointer->Insert((wchar_t)wParam))
        WantUpdateContent = true;
}
//----------------------------------------------------------------------------------
void CGumpContainer::OnKeyDown(const WPARAM &wParam, const LPARAM &lParam)
{
    if (m_GridSearch == NULL || g_EntryPointer != &m_GridSearch->m_Entry)
        return;

    switch (wParam)
    {
        case VK_RETURN:
        case VK_ESCAPE:
        {
            // Done searching: hand the keyboard back to the world, which is
            // what every other line of text in this client goes to.
            g_EntryPointer = &g_GameConsole;
            m_GridSearch->Focused = false;
            WantRedraw = true;
            break;
        }
        case VK_HOME:
        case VK_END:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_BACK:
        case VK_DELETE:
        {
            // The gump is passed in because the entry redraws through it; with
            // a null there, backspace edited nothing anyone could see.
            g_EntryPointer->OnKey(this, wParam);
            WantUpdateContent = true;
            break;
        }
        default:
            break;
    }
}
#if !USE_WISP
//----------------------------------------------------------------------------------
void CGumpContainer::OnTextInput(const SDL_TextInputEvent &ev)
{
    for (const wchar_t &ch : DecodeUTF8(ev.text))
        OnCharPress((WPARAM)ch, 0);
}
//----------------------------------------------------------------------------------
void CGumpContainer::OnKeyDown(const SDL_KeyboardEvent &ev)
{
    OnKeyDown((WPARAM)ev.keysym.sym, 0);
}
#endif
//----------------------------------------------------------------------------------
void CGumpContainer::GUMP_BUTTON_EVENT_C
{
    WISPFUN_DEBUG("c93_f11");
    if (serial == ID_GC_GRID_SEARCH)
    {
        OnTextEntry(ID_GC_GRID_SEARCH);
    }
    else if (!Minimized && serial == ID_GC_MINIMIZE && ID == 0x003C)
        Minimized = true;
    else if (serial == ID_GC_LOCK_MOVING)
    {
        LockMoving = !LockMoving;
        g_MouseManager.CancelDoubleClick = true;
    }
    else if (serial == ID_GC_GRID_SCROLL_UP)
    {
        if (m_GridScrollRow > 0)
        {
            m_GridScrollRow--;
            WantUpdateContent = true;
        }
    }
    else if (serial == ID_GC_GRID_SCROLL_DOWN)
    {
        // UpdateContent clamps against the real row count, so overshooting here
        // is corrected before anything is drawn.
        m_GridScrollRow++;
        WantUpdateContent = true;
    }
}
//----------------------------------------------------------------------------------
void CGumpContainer::OnLeftMouseButtonUp()
{
    WISPFUN_DEBUG("c93_f12");
    CGump::OnLeftMouseButtonUp();

    uint dropContainer = Serial;
    uint selectedSerial = g_SelectedObject.Serial;

    if (g_Target.IsTargeting() && !g_ObjectInHand.Enabled && selectedSerial &&
        selectedSerial != ID_GC_MINIMIZE && selectedSerial != ID_GC_LOCK_MOVING)
    {
        g_Target.SendTargetObject(selectedSerial);
        g_MouseManager.CancelDoubleClick = true;

        return;
    }

    bool canDrop =
        (GetTopObjDistance(g_Player, g_World->FindWorldObject(dropContainer)) <=
         DRAG_ITEMS_DISTANCE);

    if (canDrop && selectedSerial && selectedSerial != ID_GC_MINIMIZE &&
        selectedSerial != ID_GC_LOCK_MOVING)
    {
        canDrop = false;

        if (g_ObjectInHand.Enabled)
        {
            canDrop = true;

            CGameItem *target = g_World->FindWorldItem(selectedSerial);

            if (target != NULL)
            {
                if (target->IsContainer())
                    dropContainer = target->Serial;
                else if (target->IsStackable() && target->Graphic == g_ObjectInHand.Graphic)
                    dropContainer = target->Serial;
                else
                {
                    switch (target->Graphic)
                    {
                        case 0x0EFA:
                        case 0x2253:
                        case 0x2252:
                        case 0x238C:
                        case 0x23A0:
                        case 0x2D50:
                        {
                            dropContainer = target->Serial;
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
    }

    if (!canDrop && g_ObjectInHand.Enabled)
        g_Orion.PlaySoundEffect(0x0051);

    int x = g_MouseManager.Position.X - m_X;
    int y = g_MouseManager.Position.Y - m_Y;

    if (canDrop && g_ObjectInHand.Enabled)
    {
        const CContainerOffsetRect &r = g_ContainerOffset[Graphic].Rect;

        bool doubleDraw = false;
        ushort graphic = g_ObjectInHand.GetDrawGraphic(doubleDraw);

        CGLTexture *th = g_Orion.ExecuteStaticArt(graphic);

        if (IsGameBoard)
        {
            th = g_Orion.ExecuteGump(graphic - GAME_FIGURE_GUMP_OFFSET);
            y += 20;
        }

        if (th != NULL)
        {
            x -= (th->Width / 2);
            y -= (th->Height / 2);

            if (x + th->Width > r.MaxX)
                x = r.MaxX - th->Width;

            if (y + th->Height > r.MaxY)
                y = r.MaxY - th->Height;
        }

        if (x < r.MinX)
            x = r.MinX;

        if (y < r.MinY)
            y = r.MinY;

        if (dropContainer != Serial)
        {
            CGameItem *target = g_World->FindWorldItem(selectedSerial);

            if (target->IsContainer())
            {
                x = -1;
                y = -1;
            }
        }

        g_Orion.DropItem(dropContainer, x, y, 0);
        g_MouseManager.CancelDoubleClick = true;
    }
    else if (!g_ObjectInHand.Enabled)
    {
        if (!g_ClickObject.Enabled)
        {
            CGameObject *clickTarget = g_World->FindWorldObject(selectedSerial);

            if (clickTarget != NULL)
            {
                g_ClickObject.Init(clickTarget);
                g_ClickObject.Timer = g_Ticks + g_MouseManager.DoubleClickDelay;
                g_ClickObject.X = x;
                g_ClickObject.Y = y;
            }
        }
    }
}
//----------------------------------------------------------------------------------
bool CGumpContainer::OnLeftMouseButtonDoubleClick()
{
    WISPFUN_DEBUG("c93_f13");
    bool result = false;

    if (!g_PressedObject.LeftSerial && Minimized && ID == 0x003C)
    {
        Minimized = false;
        Page = 2;
        WantUpdateContent = true;

        result = true;
    }
    else if (g_PressedObject.LeftSerial && g_PressedObject.LeftSerial != ID_GC_MINIMIZE)
    {
        g_Orion.DoubleClick(g_PressedObject.LeftSerial);
        FrameCreated = false;

        result = true;
    }

    return result;
}
//----------------------------------------------------------------------------------

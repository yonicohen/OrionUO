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

    Add(new CGUIShader(&g_ColorizerShader, true));

    m_DataBox = (CGUIDataBox *)Add(new CGUIDataBox());

    Add(new CGUIShader(&g_ColorizerShader, false));
}
//----------------------------------------------------------------------------------
CGumpContainer::~CGumpContainer()
{
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

    IsGameBoard = (ID == 0x091A || ID == 0x092E);

    const bool grid = UseGrid();
    int gridIndex = 0;

    if (grid)
    {
        // Size the panel to the contents before laying anything out. Counting
        // first costs one pass and avoids a panel that lags a frame behind.
        int itemCount = 0;
        QFOR(counted, container->m_Items, CGameItem *)
        {
            if ((counted->Layer == OL_NONE ||
                 (container->IsCorpse() && LAYER_UNSAFE[counted->Layer])) &&
                counted->Count > 0)
            {
                itemCount++;
            }
        }

        const int rows = (itemCount + GridColumns - 1) / GridColumns;
        const int visibleRows = (rows < GridMaxRows) ? rows : GridMaxRows;

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
        m_GridBackground->Width = GridColumns * GridCellSize + GridBorder * 2;
        m_GridBackground->Height = GridMaxRows * GridCellSize + GridBorder * 2;

        // Without cell edges the panel reads as items scattered on a blank sheet
        // rather than a grid. Lines rather than a box per cell: fourteen thin
        // rectangles instead of thirty-six outlines.
        const uint lineColor = 0x60000000;
        const int gridWidth = GridColumns * GridCellSize;
        const int gridHeight = GridMaxRows * GridCellSize;

        for (int column = 0; column <= GridColumns; column++)
        {
            m_DataBox->Add(new CGUIColoredPolygone(
                0,
                0,
                GridBorder + column * GridCellSize,
                GridBorder,
                1,
                gridHeight,
                lineColor));
        }

        for (int row = 0; row <= GridMaxRows; row++)
        {
            m_DataBox->Add(new CGUIColoredPolygone(
                0,
                0,
                GridBorder,
                GridBorder + row * GridCellSize,
                gridWidth,
                1,
                lineColor));
        }

        const int buttonX = m_GridBackground->Width - 4;
        m_GridScrollUp->Visible = (maxScroll > 0);
        m_GridScrollDown->Visible = (maxScroll > 0);
        m_GridScrollUp->SetX(buttonX);
        m_GridScrollUp->SetY(GridBorder);
        m_GridScrollDown->SetX(buttonX);
        m_GridScrollDown->SetY(m_GridBackground->Height - GridBorder - 14);
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

    QFOR(obj, container->m_Items, CGameItem *)
    {
        int count = obj->Count;

        if ((obj->Layer == OL_NONE || (container->IsCorpse() && LAYER_UNSAFE[obj->Layer])) &&
            count > 0)
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
                const int row = gridIndex / GridColumns - m_GridScrollRow;
                drawX = GridBorder + (gridIndex % GridColumns) * GridCellSize;
                drawY = GridBorder + row * GridCellSize;
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

                // Outside the visible window: skip it rather than draw it beyond
                // the panel, since nothing here clips to the panel's bounds.
                if (row < 0 || row >= GridMaxRows)
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
                const int cellY =
                    GridBorder + ((gridIndex - 1) / GridColumns - m_GridScrollRow) * GridCellSize;

                CGUIText *countText =
                    (CGUIText *)m_DataBox->Add(new CGUIText(0x0481, cellX + 3, cellY + GridCellSize - 18));
                // Font 0. Font 9 renders nothing here - this was already found once and
                // then undone by a careless edit.
                // Unicode rather than the ASCII faces: font 0 there is the large
                // runic one that overflowed the cell, and 1 and 9 render nothing
                // at all in this gump. The unicode face is small and legible.
                countText->CreateTextureW(0, ToWString(GridCountText(obj->Count)));
            }
        }
    }
}
//----------------------------------------------------------------------------------
void CGumpContainer::Draw()
{
    WISPFUN_DEBUG("c93_f9");
    CGump::Draw();

    if (!Minimized)
    {
        glTranslatef(g_GumpTranslate.X, g_GumpTranslate.Y, 0.0f);

        g_FontColorizerShader.Use();

        m_TextRenderer.Draw();

        UnuseShader();

        glTranslatef(-g_GumpTranslate.X, -g_GumpTranslate.Y, 0.0f);
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
void CGumpContainer::GUMP_BUTTON_EVENT_C
{
    WISPFUN_DEBUG("c93_f11");
    if (!Minimized && serial == ID_GC_MINIMIZE && ID == 0x003C)
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

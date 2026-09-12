// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** GUIResizeButton.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
CGUIResizeButton::CGUIResizeButton(
    int serial, ushort graphic, ushort graphicSelected, ushort graphicPressed, int x, int y)
    : CGUIButton(serial, graphic, graphicSelected, graphicPressed, x, y)
{
    Type = GOT_RESIZEBUTTON;
}
//----------------------------------------------------------------------------------
CGUIResizeButton::~CGUIResizeButton()
{
}
//----------------------------------------------------------------------------------
bool CGUIResizeButton::Select()
{
    CGLTexture *th = g_Orion.ExecuteGump(Graphic);

    if (th == NULL)
        return false;

    if (HitPadding <= 0)
        return th->Select(m_X, m_Y, !CheckPolygone);

    return g_Orion.PolygonePixelsInXY(
        m_X - HitPadding,
        m_Y - HitPadding,
        th->Width + HitPadding * 2,
        th->Height + HitPadding * 2);
}
//----------------------------------------------------------------------------------

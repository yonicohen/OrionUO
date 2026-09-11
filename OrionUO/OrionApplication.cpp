// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** OrionApplication.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
#include <SDL_timer.h>
//----------------------------------------------------------------------------------
COrionApplication g_App;
//----------------------------------------------------------------------------------
void COrionApplication::OnMainLoop()
{
    //WISPFUN_DEBUG("c193_f1");
    g_Ticks = SDL_GetTicks();

    // A finger resting on the screen produces no events, so the press-and-hold
    // that stands in for the right button has to be noticed here.
    g_OrionWindow.ProcessTouch();

    // The soft keyboard is only wanted while a text field has focus; anywhere
    // else it covers the bottom half of the screen for nothing.
    // Not the game console: in the world it holds focus permanently, so
    // following it would keep the keyboard over the game for the whole session.
    // A two-finger tap raises it there instead.
    g_OrionWindow.UpdateTextInput(g_EntryPointer != nullptr && g_EntryPointer != &g_GameConsole);

    // The keyboard sliding in or out changes how much of the window the
    // pre-game screens have to fit into, and produces no resize event to hang
    // that off.
    if (g_GameState < GS_GAME && g_GL.ObscuredHeight() != g_GL.m_ObscuredHeight)
        g_GL.UpdateRect();

    if (NextRenderTime <= g_Ticks)
    {
        NextUpdateTime = g_Ticks + 50;
        NextRenderTime = NextUpdateTime; // g_Ticks + g_OrionWindow.RenderTimerDelay;

#if !USE_WISP
        g_Orion.Process(true);
#endif
        g_ConnectionManager.Recv();
        g_PacketManager.ProcessPluginPackets();
        g_PacketManager.SendMegaClilocRequests();
    }
    else if (NextUpdateTime <= g_Ticks)
    {
        NextUpdateTime = g_Ticks + 50;

#if !USE_WISP
        g_Orion.Process(false);
#endif
        g_ConnectionManager.Recv();
        g_PacketManager.ProcessPluginPackets();
        g_PacketManager.SendMegaClilocRequests();
    }
}
//----------------------------------------------------------------------------------

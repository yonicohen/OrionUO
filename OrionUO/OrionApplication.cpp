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

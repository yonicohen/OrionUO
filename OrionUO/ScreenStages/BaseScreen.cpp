// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** BaseScreen.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
CBaseScreen *g_CurrentScreen = NULL;
//----------------------------------------------------------------------------------
CBaseScreen::CBaseScreen(CGump &gump)
    : CBaseQueue()
    , m_Gump(gump)
{
}
//----------------------------------------------------------------------------------
/*!
Отрисовка/выбор объектов
@param [__in] mode true - отрисовка, false - выбор
@return При выборе объектов - идентификатор выбранного объекта
*/
void CBaseScreen::Render(bool mode)
{
    WISPFUN_DEBUG("c159_f1");
    if (mode)
    {
        g_GL.BeginDraw();

        if (DrawSmoothMonitor())
            return;

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        m_Gump.Draw();

        // The login and character screens draw one gump of their own and never
        // go near the gump manager, so the on-screen keyboard has to be drawn
        // and picked here or there is no way to type a name or a password
        // without the system keyboard taking half the screen.
        if (g_GumpKeyboard != NULL)
            g_GumpKeyboard->Draw();

        // Only for something of this screen's own. A screen's tooltip code reads
        // the selected object's serial as one of its own ids - the server list
        // looks a shard up by it - so handing it a keyboard key's serial had it
        // dereference a shard that does not exist.
        if (g_SelectedObject.Gump == &m_Gump)
            InitToolTip();

        DrawSmoothMonitorEffect();

        g_MouseManager.Draw(CursorGraphic);

        g_GL.EndDraw();
    }
    else
    {
        g_SelectedObject.Clear();

        // The keyboard is drawn over the screen, so it is asked first and wins.
        CRenderObject *selected = NULL;

        if (g_GumpKeyboard != NULL)
            selected = g_GumpKeyboard->Select();

        if (selected == NULL)
        {
            selected = m_Gump.Select();

            if (selected != NULL)
                g_SelectedObject.Init(selected, &m_Gump);
        }

        if (g_SelectedObject.Object != g_LastSelectedObject.Object)
        {
            if (g_SelectedObject.Object != NULL)
                g_SelectedObject.Object->OnMouseEnter();

            if (g_LastSelectedObject.Object != NULL)
                g_LastSelectedObject.Object->OnMouseExit();
        }

        g_LastSelectedObject.Init(g_SelectedObject);
    }
}
//----------------------------------------------------------------------------------
/*!
Вычисление состояния перехода
@return Индекс состояния
*/
int CBaseScreen::DrawSmoothMonitor()
{
    WISPFUN_DEBUG("c159_f2");
    if (g_ScreenEffectManager.Process() && SmoothScreenAction)
    {
        ProcessSmoothAction();

        g_GL.EndDraw();

        return 1;
    }

    return 0;
}
//----------------------------------------------------------------------------------
/*!
Наложение эффекта перехода
@return 
*/
void CBaseScreen::DrawSmoothMonitorEffect()
{
    WISPFUN_DEBUG("c159_f3");
    g_ScreenEffectManager.Draw();
}
//----------------------------------------------------------------------------------
/*!
Создание плавного затемнения экрана
@param [__in] action Идентификатор действия
@return 
*/
void CBaseScreen::CreateSmoothAction(BYTE action)
{
    WISPFUN_DEBUG("c159_f4");
    if (g_ScreenEffectManager.UseSunset())
        SmoothScreenAction = action;
    else
        ProcessSmoothAction(action);
}
//----------------------------------------------------------------------------------

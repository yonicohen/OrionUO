/***********************************************************************************
**
** BaseScreen.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#pragma once
#include <SDL_events.h>
//----------------------------------------------------------------------------------
//Базовый класс стадий окна клиента
class CBaseScreen : public CBaseQueue
{
public:
    //!Индекс действия, которое необходимо совершить после окончания плавного перехода затемненного состояния экрана
    uchar SmoothScreenAction = 0;
    ushort CursorGraphic = 0x2073;

protected:
    CGump &m_Gump;

public:
    CBaseScreen(CGump &gump);
    virtual ~CBaseScreen() {}

    virtual void PrepareContent() { m_Gump.PrepareContent(); }

    virtual void UpdateContent() { m_Gump.UpdateContent(); }

    /*!
	Инициализация
	@return 
	*/
    virtual void Init() {}

    /*!
	Инициализация всплывающих подсказок
	@return 
	*/
    virtual void InitToolTip() { m_Gump.InitToolTip(); }

    /*!
	Отрисовка/выбор объектов
	@param [__in] mode true - отрисовка, false - выбор
	@return При выборе объектов - идентификатор выбранного объекта
	*/
    virtual void Render(bool mode);

    /*!
	Создание плавного затемнения экрана
	@param [__in] action Идентификатор действия
	@return 
	*/
    virtual void CreateSmoothAction(uchar action);

    /*!
	Обработка события после перехода
	@param [__in_opt] action Идентификатор действия
	@return 
	*/
    virtual void ProcessSmoothAction(uchar action = 0xFF) {}

    /*!
	Вычисление состояния перехода
	@return Индекс состояния
	*/
    virtual int DrawSmoothMonitor();

    /*!
	Наложение эффекта перехода
	@return 
	*/
    virtual void DrawSmoothMonitorEffect();

    // A press that landed on the keyboard belongs to the keyboard; anything
    // else goes to the screen's own gump as before.
    bool PressedKeyboard() const
    {
        return (g_GumpKeyboard != NULL && g_PressedObject.LeftGump == g_GumpKeyboard);
    }

    virtual void OnLeftMouseButtonDown()
    {
        if (PressedKeyboard())
            g_GumpKeyboard->OnLeftMouseButtonDown();
        else
            m_Gump.OnLeftMouseButtonDown();
    }
    virtual void OnLeftMouseButtonUp()
    {

        if (PressedKeyboard())
        {
            // Dragging is committed by the gump manager in the world, and these
            // screens never go near it - so a keyboard carried around here would
            // snap back to where it started unless the move is applied by hand.
            if (g_PressedObject.LeftObject == NULL || !g_PressedObject.LeftSerial ||
                g_PressedObject.TestMoveOnDrag())
            {
                const WISP_GEOMETRY::CPoint2Di offset = g_MouseManager.LeftDroppedOffset();

                if (offset.X || offset.Y)
                {
                    g_GumpKeyboard->SetX(g_GumpKeyboard->GetX() + offset.X);
                    g_GumpKeyboard->SetY(g_GumpKeyboard->GetY() + offset.Y);
                }
            }

            g_GumpKeyboard->OnLeftMouseButtonUp();
            g_GumpKeyboard->WantRedraw = true;

            // Anything that asked to be closed is closed here, where nothing is
            // walking its items any more. In the world the gump manager does
            // this itself; these screens never call into it.
            g_GumpManager.RemoveMarked();
            return;
        }

        m_Gump.OnLeftMouseButtonUp();
        m_Gump.WantRedraw = true;
    }
    virtual bool OnLeftMouseButtonDoubleClick() { return m_Gump.OnLeftMouseButtonDoubleClick(); }
    virtual void OnRightMouseButtonDown() { m_Gump.OnRightMouseButtonDown(); }
    virtual void OnRightMouseButtonUp() { m_Gump.OnRightMouseButtonUp(); }
    virtual bool OnRightMouseButtonDoubleClick() { return m_Gump.OnRightMouseButtonDoubleClick(); }
    virtual void OnMidMouseButtonDown() { m_Gump.OnMidMouseButtonDown(); }
    virtual void OnMidMouseButtonUp() { m_Gump.OnMidMouseButtonUp(); }
    virtual bool OnMidMouseButtonDoubleClick() { return m_Gump.OnMidMouseButtonDoubleClick(); }
    virtual void OnMidMouseButtonScroll(bool up) { m_Gump.OnMidMouseButtonScroll(up); }
    virtual void OnDragging() { m_Gump.OnDragging(); }
    virtual void OnCharPress(const WPARAM &wParam, const LPARAM &lParam)
    {
        m_Gump.OnCharPress(wParam, lParam);
    }
    virtual void OnKeyDown(const WPARAM &wParam, const LPARAM &lParam)
    {
        m_Gump.OnKeyDown(wParam, lParam);
    }
    virtual void OnKeyUp(const WPARAM &wParam, const LPARAM &lParam)
    {
        m_Gump.OnKeyUp(wParam, lParam);
    }
#if !USE_WISP
    virtual void OnTextInput(const SDL_TextInputEvent &ev) { m_Gump.OnTextInput(ev); }
    virtual void OnKeyDown(const SDL_KeyboardEvent &ev) { m_Gump.OnKeyDown(ev); }
    virtual void OnKeyUp(const SDL_KeyboardEvent &ev) { m_Gump.OnKeyUp(ev); }
#endif
};
//----------------------------------------------------------------------------------
//!Указатель на текущий экран
extern CBaseScreen *g_CurrentScreen;
//----------------------------------------------------------------------------------

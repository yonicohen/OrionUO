/***********************************************************************************
**
** ObjectPripertiesManager.h
**
** Copyright (C) October 2017 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef OBJECTPROPERTIESMANAGER_H
#define OBJECTPROPERTIESMANAGER_H
//----------------------------------------------------------------------------------
class CObjectProperty
{
public:
    uint Serial = 0;
    uint Revision = 0;
    wstring Name = L"";
    wstring Data = L"";

    CObjectProperty() {}
    CObjectProperty(int serial, int revision, const wstring &name, const wstring &data);

    bool Empty();

    wstring CreateTextData(bool extended);
};
//----------------------------------------------------------------------------------
typedef map<uint, CObjectProperty> OBJECT_PROPERTIES_MAP;
//----------------------------------------------------------------------------------
class CObjectPropertiesManager
{
    uint Timer = 0;

private:
    OBJECT_PROPERTIES_MAP m_Map;

    class CRenderObject *m_Object{ NULL };

public:
    CObjectPropertiesManager() {}
    virtual ~CObjectPropertiesManager();

    void Reset();

    bool RevisionCheck(int serial, int revision);

    void OnItemClicked(int serial);

    void Display(int serial);

    void Add(int serial, const CObjectProperty &objectProperty);

    // What the server has said about one object, for anything that wants to
    // read the properties rather than draw them - searching a container by what
    // is written on its contents, for one.
    const CObjectProperty *Get(uint serial) const
    {
        OBJECT_PROPERTIES_MAP::const_iterator found = m_Map.find(serial);
        return (found != m_Map.end()) ? &found->second : NULL;
    }
};
//----------------------------------------------------------------------------------
extern CObjectPropertiesManager g_ObjectPropertiesManager;
//----------------------------------------------------------------------------------
#endif //OBJECTPROPERTIESMANAGER_H
//----------------------------------------------------------------------------------

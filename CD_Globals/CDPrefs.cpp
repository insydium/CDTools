//
//  CDPrefs.cpp
//

#include "CDPrefs.h"
#include "CDPluginsPrefs.h"

Bool CDPrefs::InitValues(const DescID &id, Description *desc)
{
    BaseContainer* bc = GetCDPrefs();
    if (!bc) return FALSE;
	
    switch (id[0].id)
    {
		//case PREF_MYPREFS_BOOL1: InitPrefsValue(WPREFS_BOOL1,GeData(TRUE),desc,id,bc); return TRUE;
		//case PREF_MYPREFS_BOOL2: InitPrefsValue(WPREFS_BOOL2,GeData(TRUE),desc,id,bc); return TRUE;
    }
	
    return TRUE;
}

Bool CDPrefs::Init(GeListNode* node)
{
    //InitValues(PREF_MYPREFS_BOOL1);
    //InitValues(PREF_MYPREFS_BOOL2);
	
    return TRUE;
}

Bool CDPrefs::GetDParameter(GeListNode *node, const DescID &id, GeData &t_data, DESCFLAGS_GET &flags)
{
    BaseContainer* bc = GetCDPrefs();
    if (!bc) SUPER::GetDParameter(node,id,t_data,flags);
	
    switch (id[0].id)
    {
		//case PREF_MYPREFS_BOOL1: t_data = bc->GetBool(WPREFS_BOOL1,TRUE); flags |= DESCFLAGS_GET_PARAM_GET; return TRUE;
		//case PREF_MYPREFS_BOOL2: t_data = bc->GetBool(WPREFS_BOOL2,TRUE); flags |= DESCFLAGS_GET_PARAM_GET; return TRUE;
    }
	
    return SUPER::GetDParameter(node,id,t_data,flags);
}

Bool CDPrefs::SetDParameter(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_SET &flags)
{
    BaseContainer* bc = GetCDPrefs();
    if (!bc) SUPER::SetDParameter(node,id,t_data,flags);
	
    switch (id[0].id)
    {
		//case PREF_MYPREFS_BOOL1: bc->SetBool(WPREFS_BOOL1,t_data.GetLong()); flags |= DESCFLAGS_SET_PARAM_SET; GeUpdateUI();  return TRUE;
		//case PREF_MYPREFS_BOOL2: bc->SetBool(WPREFS_BOOL2,t_data.GetLong()); flags |= DESCFLAGS_SET_PARAM_SET; GeUpdateUI();  return TRUE;
    }
	
    return SUPER::SetDParameter(node,id,t_data,flags);
}

Bool CDPrefs::GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
{
    BaseContainer* bc = GetCDPrefs();
    if(!bc) SUPER::GetDEnabling(node,id,t_data,flags,itemdesc);
	else
	{
		switch(id[0].id)
		{
			case CD_PREFS_BOOL1:
				return TRUE;
			case CD_PREFS_BOOL2:
				return FALSE;
		}
	}
    return SUPER::GetDEnabling(node,id,t_data,flags,itemdesc);
}

Bool CDPrefs::GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
{
    if(!description->LoadDescription("Prefsmyprefs")) return FALSE;
	
    if( flags & DESCFLAGS_DESC_NEEDDEFAULTVALUE )
    {
		//InitValues(PREF_MYPREFS_BOOL1,description);
		//InitValues(PREF_MYPREFS_BOOL2,description);
    }
	
    flags |= DESCFLAGS_DESC_LOADED;
    return SUPER::GetDDescription(node, description, flags);
}

BaseContainer* CDPrefs::GetCDPrefs()
{
    BaseContainer* bc = GetWorldContainerInstance()->GetContainerInstance(ID_CDPREFSDIALOG);
    if (!bc)
    {
		GetWorldContainerInstance()->SetContainer(ID_CDPREFSDIALOG, BaseContainer());
		bc = GetWorldContainerInstance()->GetContainerInstance(ID_CDPREFSDIALOG);
		if (!bc) return NULL;
    }
	
    return bc;
}


Bool RegisterCDPrefs()
{
    PrefsDialogObject::Register(ID_CDPREFERENCES, CDPrefs::Alloc, GeLoadString(IDS_CDPREFS), "CDPluginsPrefs", 0, PREFS_PRI_MODULES);
    return TRUE;
}

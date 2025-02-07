//
//  CDPrefs.h
//

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_prefs.h"

#ifndef _CDPREFS_H_
#define _CDPREFS_H_

#define ID_CDPREFERENCES			1023204
#define ID_CDPREFSDIALOG			1030883


#if API_VERSION < 12000
class CDPrefsDialog: public PrefsDlg_Base
{
public:
	CDPrefsDialog(PrefsDialogHookClass *hook) : PrefsDlg_Base(ID_CDPREFERENCES,hook) { }

};

class CDPrefs: public PrefsDialogHookClass
{
public:
	virtual SubDialog *Alloc() { return gNew CDPrefsDialog(this); }
	virtual void Free(SubDialog *dlg) { gDelete(dlg); }

};
#else
class CDPrefs : public PrefsDialogObject
{
    INSTANCEOF(CDPrefs, PrefsDialogObject)
	
public:
	virtual Bool InitValues(const DescID &id, Description* desc = NULL);
	
	virtual Bool Init(GeListNode* node);
	virtual Bool GetDParameter(GeListNode *node, const DescID &id, GeData &t_data, DESCFLAGS_GET &flags);
	virtual Bool SetDParameter(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_SET &flags);
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc);
	virtual Bool GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);
	
	BaseContainer *GetCDPrefs();
	
	static NodeData *Alloc() { return gNew CDPrefs; }
};
#endif

#endif
//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "tCDSkin.h"
#include "CDPaintSkin.h"

class CDDeleteJointsDialog : public GeModalDialog
{
private:
	CDJSOptionsUA ua;
	
public:	
	Bool	autoWt, autoBnd;
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDDeleteJointsDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDJDELETE));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDJS_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDJS_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_WP_DYN_TITLE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddCheckbox (IDC_CDADDJ_AUTO_BIND,BFH_LEFT,0,0,GeLoadString(IDC_CDADDJ_AUTO_BIND));
				
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}
	
	return res;
}

Bool CDDeleteJointsDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDDELETEJOINTS);
	if(!bc)
	{
		SetBool(IDC_CDADDJ_AUTO_BIND,false);
	}
	else
	{
		SetBool(IDC_CDADDJ_AUTO_BIND,bc->GetBool(IDC_CDADDJ_AUTO_BIND));
	}
	
	return true;
}

Bool CDDeleteJointsDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDC_CDADDJ_AUTO_BIND,autoBnd);
	
	BaseContainer bc;
	bc.SetBool(IDC_CDADDJ_AUTO_BIND,autoBnd);
	SetWorldPluginData(ID_CDDELETEJOINTS,bc,false);
	
	return true;
}

class CDDeleteJoints : public CommandData
{
private:
	CDDeleteJointsDialog dlg;
	
	Bool DeleteChildJoints(BaseDocument* doc, BaseObject *op, BaseTag *tag, LONG type);

public:
	virtual Bool Execute(BaseDocument* doc);

};

Bool CDDeleteJoints::DeleteChildJoints(BaseDocument* doc, BaseObject *op, BaseTag *tag, LONG type)
{
	BaseContainer *tagData = tag->GetDataInstance();
	LONG j, jSubCount, jCount, jIndex = SKN_J_LINK;
	while(op)
	{
		if(op->GetType() == type)
		{
			jSubCount = tagData->GetLong(SKN_J_COUNT_CHANGE), jCount = tagData->GetLong(SKN_J_COUNT);
			for(j=0; j<jCount; j++)
			{
				if(op == tagData->GetObjectLink(jIndex+j,doc))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					tagData->SetLink(jIndex+j,NULL);
					jSubCount--;
					tagData->SetLong(SKN_J_COUNT_CHANGE,jSubCount);
				}
			}
		}
		DeleteChildJoints(doc,op->GetDown(),tag,type);
		op = op->GetNext();
	}
	return true;
}

Bool CDDeleteJoints::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseTag* tag = doc->GetActiveTag();
	if(!tag)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	else if(tag && tag->GetType() != ID_CDSKINPLUGIN)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	
	BaseObject *mesh = tag->GetObject();
	if(mesh)
	{
		if(!IsValidPointObject(mesh))
		{
			MessageDialog(GeLoadString(D_AD_JOINT_ERR));
			return false;
		}
	}
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG opCount = objects->GetCount();
	if(opCount < 1)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	else if(opCount == 1 && mesh && static_cast<BaseObject*>(objects->GetIndex(0)) == mesh)
	{
		MessageDialog(GeLoadString(D_AD_JOINT_ERR));
		return false;
	}
	
	BaseContainer *tagData = tag->GetDataInstance();
	if(tagData->GetBool(SKN_BOUND))
	{
		MessageDialog(GeLoadString(D_UNBIND_FIRST));
		return false;
	}
	
	LONG i, j, jSubCount, jCount = tagData->GetLong(SKN_J_COUNT);
	BaseContainer *tbc=GetToolData(GetActiveDocument(),ID_SKINWEIGHTTOOL);
	if(tbc) tbc->SetLong(WP_SKIN_JOINT_COUNT,jCount);
	jSubCount = jCount;
	LONG jIndex = SKN_J_LINK;

	Bool subChild = true, showDialog = false;
	BaseObject *op = NULL;
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op->GetDown()) showDialog = true;
	}
	if(showDialog)
	{
		if(!QuestionDialog(GeLoadString(D_AD_CHILDREN))) subChild = false;
		else  subChild = true;
	}
	
	doc->StartUndo();
	CDAddUndo(doc,CD_UNDO_CHANGE,tag);
	tagData->SetLong(SKN_J_COUNT_CHANGE,jSubCount);
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		for(j=0; j<jCount; j++)
		{
			if(op == tagData->GetObjectLink(jIndex+j,doc))
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				jSubCount = tagData->GetLong(SKN_J_COUNT_CHANGE);
				tagData->SetLink(jIndex+j,NULL);
				jSubCount--;
				tagData->SetLong(SKN_J_COUNT_CHANGE,jSubCount);
				if(subChild)
				{
					if(op->GetDown())
					{
						if(!DeleteChildJoints(doc,op->GetDown(),tag,op->GetType())) return true;
					}
				}
			}
		}
	}
	DescriptionCommand dc;
	dc.id = DescID(SKN_DELETE_JOINT);
	tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDDELETEJOINTS);
	if(wpData)
	{
		Bool bind = wpData->GetBool(IDC_CDADDJ_AUTO_BIND);
		if(bind)
		{
			dc.id = DescID(SKN_BIND_SKIN);
			tag->Message(MSG_DESCRIPTION_COMMAND,&dc);
		}
	}
	
	doc->EndUndo();
	EventAdd();

	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDDeleteJointsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDDeleteJoints(void)
{
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
	CHAR chr;
	
	aK = Mod(aK,25);
	bK = Mod(bK,3);
	if(Mod(aK,2) == 0) chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) | cK);
	else chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) & cK);
	b = chr;
	
	String s, hStr, oldS;
	CHAR ch[2];
	LONG i, rem, n = Abs(b);
	
	ch[1] = 0;
	for(i=0; i<2; i++)
	{
		rem = Mod(n,16);
		ch[0] = GetHexCharacter(rem);
		oldS.SetCString(ch,1);
		hStr += oldS;
		n /= 16;
	}
	s = hStr;
	if(kb != s) reg = false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = true;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) reg = true;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) reg = false;
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDJDELETE); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDDELETEJOINTS,name,PLUGINFLAG_HIDE,"CDDeleteJoints.TIF","CD Remove Joints",CDDataAllocator(CDDeleteJointsR));
	else return CDRegisterCommandPlugin(ID_CDDELETEJOINTS,name,0,"CDDeleteJoints.TIF","CD Remove Joints",CDDataAllocator(CDDeleteJoints));
}

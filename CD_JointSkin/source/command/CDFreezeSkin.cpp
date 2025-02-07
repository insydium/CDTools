//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinRefTag.h"

#include "tCDSkin.h"
#include "tCDMorphRef.h"

class CDFreezeSkinDialog : public GeModalDialog
{
private:
	CDJSOptionsUA ua;
	
public:	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDFreezeSkinDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDFRZSKIN));
		
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
				
				AddCheckbox (IDC_RESET_ORIGINAL,BFH_LEFT,0,0,GeLoadString(IDC_RESET_ORIGINAL));
				
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}
	
	return res;
}

Bool CDFreezeSkinDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDFREEZESKINSTATE);
	if(!bc)
	{
		SetBool(IDC_RESET_ORIGINAL,false);
	}
	else
	{
		SetBool(IDC_RESET_ORIGINAL,bc->GetBool(IDC_RESET_ORIGINAL));
	}
	
	return true;
}

Bool CDFreezeSkinDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool resetOrig;
	GetBool(IDC_RESET_ORIGINAL,resetOrig);
	
	BaseContainer bc;
	bc.SetBool(IDC_RESET_ORIGINAL,resetOrig);
	SetWorldPluginData(ID_CDFREEZESKINSTATE,bc,false);
	
	return true;
}

class CDFreezeSkinState : public CommandData
{
	private:
		CDFreezeSkinDialog dlg;
	
	public:		
		virtual Bool Execute(BaseDocument* doc);
		
};

Bool CDFreezeSkinState::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDFREEZESKINSTATE);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetBool(IDC_RESET_ORIGINAL,false);
		SetWorldPluginData(ID_CDFREEZESKINSTATE,bc,false);
		wpData = GetWorldPluginData(ID_CDFREEZESKINSTATE);
	}
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG i, opCnt = objects->GetCount();
	
	if(opCnt > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCnt; i++)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				BaseTag *skin = op->GetTag(ID_CDSKINPLUGIN);
				if(skin)
				{
					if(wpData->GetBool(IDC_RESET_ORIGINAL))
					{
						BaseTag *skrT = op->GetTag(ID_CDSKINREFPLUGIN);
						if(skrT)
						{
							DescriptionCommand dc;
							dc.id = DescID(SKR_SET_REFERENCE);
							skrT->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							dc.id = DescID(SKN_UNBIND_SKIN);
							skin->Message(MSG_DESCRIPTION_COMMAND,&dc);
							op->Message(MSG_UPDATE);
							
							dc.id = DescID(SKN_BIND_SKIN);
							skin->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							BaseTag *mrT = op->GetTag(ID_CDMORPHREFPLUGIN);
							if(mrT)
							{
								DescriptionCommand dc;
								dc.id = DescID(MR_RESET_REF);
								mrT->Message(MSG_DESCRIPTION_COMMAND,&dc);
							}
						}
					}
					else
					{
						BaseObject *cln = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_ANIMATION,NULL);
						if(cln)
						{
							doc->InsertObject(cln,NULL,NULL,true);
							CDAddUndo(doc,CD_UNDO_NEW,cln);
							cln->SetMg(op->GetMg());
							
							BaseTag *skrT = cln->GetTag(ID_CDSKINREFPLUGIN);
							if(skrT)
							{
								DescriptionCommand dc;
								dc.id = DescID(SKR_SET_REFERENCE);
								skrT->Message(MSG_DESCRIPTION_COMMAND,&dc);
								
								BaseTag *skCln = cln->GetTag(ID_CDSKINPLUGIN);
								if(skCln)
								{
									dc.id = DescID(SKN_UNBIND_SKIN);
									skCln->Message(MSG_DESCRIPTION_COMMAND,&dc);
									cln->Message(MSG_UPDATE);
									
									dc.id = DescID(SKN_BIND_SKIN);
									skCln->Message(MSG_DESCRIPTION_COMMAND,&dc);
								}
							}
							
							BaseTag *mrT = cln->GetTag(ID_CDMORPHREFPLUGIN);
							if(mrT)
							{
								DescriptionCommand dc;
								dc.id = DescID(MR_RESET_REF);
								mrT->Message(MSG_DESCRIPTION_COMMAND,&dc);
							}
						}
					}
				}
			}
		}
		
		doc->SetActiveObject(op);
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDFreezeSkinStateR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDFreezeSkinState(void)
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
	String name=GeLoadString(IDS_CDFRZSKIN); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDFREEZESKINSTATE,name,PLUGINFLAG_HIDE,"CDFreezeSkin.TIF","CD Freeze Skin State",CDDataAllocator(CDFreezeSkinStateR));
	else return CDRegisterCommandPlugin(ID_CDFREEZESKINSTATE,name,0,"CDFreezeSkin.TIF","CD Freeze Skin State",CDDataAllocator(CDFreezeSkinState));
}

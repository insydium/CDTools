//	Cactus Dan's Symmetry Tools 1.0
//	Copyright 2009 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"

#include "CDSymmetry.h"
#include "CDSymmetryTag.h"

#include "tCDSymTag.h"

class CDAddSymmetryTagDialog : public GeModalDialog
{
	private:
		CDSymOptionsUA 			ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddSymmetryTagDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDSYMTAG));
			
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDSY_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDSY_OPTIONS_IMAGE);
		}
		GroupEnd();
			
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_MIRROR_AXIS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(D_MIRROR_AXIS,BFH_CENTER,3,1);
					AddChild(D_MIRROR_AXIS, 0, GeLoadString(M_X));
					AddChild(D_MIRROR_AXIS, 1, GeLoadString(M_Y));
					AddChild(D_MIRROR_AXIS, 2, GeLoadString(M_Z));
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
			{
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_TOLERANCE),0);
				AddEditNumberArrows(D_TOLERANCE,BFH_LEFT);
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
			{
				GroupSpace(4,1);
				AddCheckbox (D_OPEN_SYM_ASGN,BFH_LEFT,0,0,GeLoadString(D_OPEN_SYM_ASGN));
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddSymmetryTagDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDADDSYMMETRYTAG);
	if(!wpData)
	{
		SetLong(D_MIRROR_AXIS,0);
		SetReal(D_TOLERANCE,0.01,0.001,100,0.001,FORMAT_REAL);
		SetBool(D_OPEN_SYM_ASGN,false);
	}
	else
	{
		SetLong(D_MIRROR_AXIS,wpData->GetLong(D_MIRROR_AXIS));
		SetReal(D_TOLERANCE,wpData->GetReal(D_TOLERANCE),0.001,100,0.001,FORMAT_REAL);
		SetBool(D_OPEN_SYM_ASGN,wpData->GetBool(D_OPEN_SYM_ASGN));
	}
	
	return true;
}

Bool CDAddSymmetryTagDialog::Command(LONG id,const BaseContainer &msg)
{
	Real t;
	LONG a;
	Bool tool;

	GetReal(D_TOLERANCE,t);
	GetLong(D_MIRROR_AXIS,a);
	GetBool(D_OPEN_SYM_ASGN,tool);
	
	BaseContainer bc;
	bc.SetReal(D_TOLERANCE,t);
	bc.SetLong(D_MIRROR_AXIS,a);
	bc.SetBool(D_OPEN_SYM_ASGN,tool);
	SetWorldPluginData(ID_CDADDSYMMETRYTAG,bc,false);
	
	return true;
}


class CDAddSymmetryTag : public CommandData
{
	private:
		CDAddSymmetryTagDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddSymmetryTag::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG  i, opCount = objects->GetCount();
	DescriptionCommand dc;
	
	if(opCount > 0)
	{
		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
		{
			if(!dlg.Open()) return false;
		}
		
		BaseTag *mpTag = NULL;
		BaseObject *op = NULL;
		
		doc->SetActiveObject(NULL);
		doc->StartUndo();
		BaseContainer *wpData = GetWorldPluginData(ID_CDADDSYMMETRYTAG);
		if(!wpData)
		{
			BaseContainer bc;
			bc.SetReal(D_TOLERANCE,0.01);
			bc.SetLong(D_MIRROR_AXIS,0);
			bc.SetBool(D_OPEN_SYM_ASGN,false);
			SetWorldPluginData(ID_CDADDSYMMETRYTAG,bc,false);
			wpData = GetWorldPluginData(ID_CDADDSYMMETRYTAG);
		}
		for(i=0; i<opCount; i++)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i)); 
			if(op)
			{
				if(IsValidPointObject(op))
				{
					if(!op->GetTag(ID_CDSYMMETRYTAG))
					{
						mpTag = BaseTag::Alloc(ID_CDSYMMETRYTAG);
						op->InsertTag(mpTag,NULL);
						mpTag->Message(MSG_MENUPREPARE);
						CDAddUndo(doc,CD_UNDO_NEW,mpTag);
						
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,mpTag);
						mpTag->Message(MSG_MENUPREPARE);
						
						BaseContainer *tData = mpTag->GetDataInstance();
						tData->SetReal(SYM_TOLERANCE,wpData->GetReal(D_TOLERANCE));
						if(wpData->GetBool(D_OPEN_SYM_ASGN)) tData->SetBool(SYM_OPEN_TOOL,true);
						switch(wpData->GetLong(D_MIRROR_AXIS))
						{
							case 0:
								tData->SetLong(SYM_SYMMETRY_AXIS,SYM_MX);
								break;
							case 1:
								tData->SetLong(SYM_SYMMETRY_AXIS,SYM_MY);
								break;
							case 2:
								tData->SetLong(SYM_SYMMETRY_AXIS,SYM_MZ);
								break;
						}
						mpTag->Message(MSG_UPDATE);
						
						dc.id = DescID(SYM_SET_SYMMETRY);
						mpTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
						
					}
				}
			}
		}
		
		doc->SetActiveObject(op);
		doc->SetActiveTag(mpTag);
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddSymmetryTagR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddSymmetryTag(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) reg = false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdsnr.FindFirst("-",&pos);
	while(h)
	{
		cdsnr.Delete(pos,1);
		h = cdsnr.FindFirst("-",&pos);
	}
	cdsnr.ToUpper();
	
	kb = cdsnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDADDSYMTAG); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDSYMMETRYTAG,name,PLUGINFLAG_HIDE,"CDAddSymTag.tif","CD Add Symmetry Tag",CDDataAllocator(CDAddSymmetryTagR));
	else return CDRegisterCommandPlugin(ID_CDADDSYMMETRYTAG,name,0,"CDAddSymTag.tif","CD Add Symmetry Tag",CDDataAllocator(CDAddSymmetryTag));
}

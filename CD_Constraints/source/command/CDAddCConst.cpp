//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tCDCConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	9

enum
{
	CC_POLY_SURFACE		= 1140,

	CC_TARGET_COUNT			= 3001
};

class CDAddCConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		Bool	local;
		LONG	axis;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddCConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDC));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDC_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDC_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDS_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddCheckbox(C_LOCAL_AXIS,BFH_LEFT,0,0,GeLoadString(C_LOCAL_AXIS));
				
				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(C_CLAMP_AXIS),0);
					AddComboBox(C_CLAMP_AXIS, BFH_SCALEFIT);
						AddChild(C_CLAMP_AXIS, 0, GeLoadString(C_CLAMP_X));
						AddChild(C_CLAMP_AXIS, 1, GeLoadString(C_CLAMP_Y));
						AddChild(C_CLAMP_AXIS, 2, GeLoadString(C_CLAMP_Z));
						AddChild(C_CLAMP_AXIS, 3, GeLoadString(C_CLAMP_NX));
						AddChild(C_CLAMP_AXIS, 4, GeLoadString(C_CLAMP_NY));
						AddChild(C_CLAMP_AXIS, 5, GeLoadString(C_CLAMP_NZ));
				}
			}
			GroupEnd();
			
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddCConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDADDCCONSTRAINT);
	if(!bc)
	{
		SetBool(C_LOCAL_AXIS,false);
		SetLong(C_CLAMP_AXIS,1);
	}
	else
	{
		SetBool(C_LOCAL_AXIS,bc->GetBool(C_LOCAL_AXIS));
		SetLong(C_CLAMP_AXIS,bc->GetLong(C_CLAMP_AXIS));
	}
	
	return true;
}

Bool CDAddCConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(C_LOCAL_AXIS,local);
	GetLong(C_CLAMP_AXIS,axis);
	
	BaseContainer bc;
	bc.SetBool(C_LOCAL_AXIS,local);
	bc.SetLong(C_CLAMP_AXIS,axis);
	SetWorldPluginData(ID_CDADDCCONSTRAINT,bc,false);
	
	return true;
}

class CDAddCConstraint : public CommandData
{
	private:
		CDAddCConstraintDialog dlg;

		void SetOptions(BaseContainer *tData, BaseContainer *wData, LONG cnt);
		
	public:
		virtual Bool Execute(BaseDocument* doc);

};

void CDAddCConstraint::SetOptions(BaseContainer *tData, BaseContainer *wpData, LONG cnt)
{
	switch (wpData->GetLong(C_CLAMP_AXIS))
	{
		case 0:
			tData->SetLong(CC_CLAMP_AXIS+cnt,CC_CLAMP_X+cnt);
			break;
		case 1:
			tData->SetLong(CC_CLAMP_AXIS+cnt,CC_CLAMP_Y+cnt);
			break;
		case 2:
			tData->SetLong(CC_CLAMP_AXIS+cnt,CC_CLAMP_Z+cnt);
			break;
		case 3:
			tData->SetLong(CC_CLAMP_AXIS+cnt,CC_CLAMP_NX+cnt);
			break;
		case 4:
			tData->SetLong(CC_CLAMP_AXIS+cnt,CC_CLAMP_NY+cnt);
			break;
		case 5:
			tData->SetLong(CC_CLAMP_AXIS+cnt,CC_CLAMP_NZ+cnt);
			break;
	}
	tData->SetBool(CC_LOCAL_AXIS+cnt,wpData->GetBool(C_LOCAL_AXIS));
}

Bool CDAddCConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;

		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
		{
			if(!dlg.Open()) return false;
		}
		
		doc->SetActiveObject(NULL);
		doc->StartUndo();

		BaseTag *cTag = NULL;
		if(!destObject->GetTag(ID_CDCCONSTRAINTPLUGIN))
		{
			cTag = BaseTag::Alloc(ID_CDCCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(cTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,cTag);
			cTag->Message(MSG_MENUPREPARE);
		}
		else cTag = destObject->GetTag(ID_CDCCONSTRAINTPLUGIN);
		
		BaseContainer *tData = cTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *wpData = GetWorldPluginData(ID_CDADDCCONSTRAINT);
			if(!wpData)
			{
				BaseContainer bc;
				bc.SetBool(C_LOCAL_AXIS,false);
				bc.SetLong(C_CLAMP_AXIS,1);
				SetWorldPluginData(ID_CDADDCCONSTRAINT,bc,false);
				wpData = GetWorldPluginData(ID_CDADDCCONSTRAINT);
			}
			
			BaseObject *target = NULL;
			LONG i, cCount = tData->GetLong(CC_TARGET_COUNT);
			if(opCount > 1)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,destObject);
				
				for(i=1; i<opCount; i++)
				{
					if(cCount < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(cCount == 1 && tData->GetObjectLink(CC_TARGET,doc) == NULL)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,cTag);
							tData->SetLink(CC_TARGET,target);
							
							SetOptions(tData,wpData,0);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(CC_ADD_CLAMP);
							cTag->Message(MSG_DESCRIPTION_COMMAND,&dc);

							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,cTag);
							tData->SetLink(CC_TARGET+cCount,target);
							
							SetOptions(tData,wpData,cCount);
							cCount = tData->GetLong(CC_TARGET_COUNT);
						}
					}
				}
			}

			doc->SetActiveObject(destObject);
			doc->SetActiveTag(cTag);
			cTag->Message(MSG_UPDATE);
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddCConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddCConstraint(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDADDC); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDCCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddCConst.tif","CD Add Clamp Constraint",CDDataAllocator(CDAddCConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDCCONSTRAINT,name,0,"CDAddCConst.tif","CD Add Clamp Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddCConstraint));
}

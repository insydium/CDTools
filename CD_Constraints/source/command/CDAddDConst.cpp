//	Cactus Dan's Constraints 1.1 plugin
//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tCDDConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	9

enum
{
	DC_TARGET_COUNT			= 1040,
	
	DC_INIT_DISTANCE		= 3000
};

class CDAddDConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		LONG	axis;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddDConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDD));
		
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
				
				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDS_CLAMP),0);
					AddComboBox(IDS_CLAMP, BFH_SCALEFIT);
						AddChild(IDS_CLAMP, 0, GeLoadString(IDS_CLAMP_MIN));
						AddChild(IDS_CLAMP, 1, GeLoadString(IDS_CLAMP_MAX));
						AddChild(IDS_CLAMP, 2, GeLoadString(IDS_CLAMP_BOTH));
				}
			}
			GroupEnd();
			
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddDConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDADDDCONSTRAINT);
	if(!bc)
	{
		SetLong(IDS_CLAMP,0);
	}
	else
	{
		SetLong(IDS_CLAMP,bc->GetLong(IDS_CLAMP));
	}
	
	return true;
}

Bool CDAddDConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDS_CLAMP,axis);
	
	BaseContainer bc;
	bc.SetLong(IDS_CLAMP,axis);
	SetWorldPluginData(ID_CDADDDCONSTRAINT,bc,false);
	
	return true;
}

class CDAddDConstraint : public CommandData
{
	private:
		CDAddDConstraintDialog dlg;
		
		void SetOptions(BaseContainer *tData, BaseContainer *wData, LONG cnt);

	public:
		virtual Bool Execute(BaseDocument* doc);

};

void CDAddDConstraint::SetOptions(BaseContainer *tData, BaseContainer *wpData, LONG cnt)
{
	switch(wpData->GetLong(IDS_CLAMP))
	{
		case 0:
			tData->SetLong(DC_CLAMP_DISTANCE+cnt,DC_CLAMP_MIN+cnt);
			break;
		case 1:
			tData->SetLong(DC_CLAMP_DISTANCE+cnt,DC_CLAMP_MAX+cnt);
			break;
		case 2:
			tData->SetLong(DC_CLAMP_DISTANCE+cnt,DC_CLAMP_BOTH+cnt);
			break;
	}
}

Bool CDAddDConstraint::Execute(BaseDocument* doc)
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

		BaseTag *dTag = NULL;
		if(!destObject->GetTag(ID_CDDCONSTRAINTPLUGIN))
		{
			dTag = BaseTag::Alloc(ID_CDDCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(dTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,dTag);
			dTag->Message(MSG_MENUPREPARE);
		}
		else dTag = destObject->GetTag(ID_CDDCONSTRAINTPLUGIN);
		
		BaseContainer *tData = dTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *wpData = GetWorldPluginData(ID_CDADDDCONSTRAINT);
			if(!wpData)
			{
				BaseContainer bc;
				bc.SetBool(C_LOCAL_AXIS,false);
				bc.SetLong(C_CLAMP_AXIS,1);
				SetWorldPluginData(ID_CDADDDCONSTRAINT,bc,false);
				wpData = GetWorldPluginData(ID_CDADDDCONSTRAINT);
			}
			
			BaseObject *target = NULL;
			LONG i, dCount = tData->GetLong(DC_TARGET_COUNT);
			if(opCount > 1)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,destObject);
				
				for(i=1; i<opCount; i++)
				{
					if(dCount < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(dCount == 1 && tData->GetObjectLink(DC_TARGET,doc) == NULL)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,dTag);
							tData->SetLink(DC_TARGET,target);
							
							SetOptions(tData,wpData,0);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(DC_ADD_DIST);
							dTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,dTag);
							tData->SetLink(DC_TARGET+dCount,target);
							
							SetOptions(tData,wpData,dCount);
							dCount = tData->GetLong(DC_TARGET_COUNT);
						}
					}
				}
			}

			doc->SetActiveObject(target);
			doc->SetActiveTag(dTag);
			dTag->Message(MSG_UPDATE);
		}
				
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddDConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddDConstraint(void)
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
	String name=GeLoadString(IDS_CDADDD); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDDCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddDConst.tif","CD Add Distance Constraint",CDDataAllocator(CDAddDConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDDCONSTRAINT,name,0,"CDAddDConst.tif","CD Add Distance Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddDConstraint));
}

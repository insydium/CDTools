//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tCDPConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	9

enum
{
	PC_TARGET_COUNT			= 1040
};

class CDAddPConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		Bool usePoint, usePoly, setOffsets;
		BaseObject *secondOp;

		void DoEnable(void);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddPConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDP));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,2,0,GeLoadString(IDS_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddCheckbox(IDS_POINT,BFH_LEFT,0,0,GeLoadString(IDS_POINT));
				AddCheckbox(IDS_NORMAL,BFH_LEFT,0,0,GeLoadString(IDS_NORMAL));
				AddCheckbox(IDC_SET_OFFSETS,BFH_LEFT,0,0,GeLoadString(IDC_SET_OFFSETS));
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddPConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDADDPCONSTRAINT);
	if(!wpData)
	{
		SetBool(IDS_POINT,false);
		SetBool(IDS_NORMAL,false);
		SetBool(IDC_SET_OFFSETS,false);
	}
	else
	{
		SetBool(IDS_POINT,wpData->GetBool(IDS_POINT));
		SetBool(IDS_NORMAL,wpData->GetBool(IDS_NORMAL));
		SetBool(IDC_SET_OFFSETS,wpData->GetBool(IDC_SET_OFFSETS));
	}
	
	DoEnable();
	
	return true;
}

Bool CDAddPConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDS_POINT,usePoint);
	GetBool(IDS_NORMAL,usePoly);
	GetBool(IDC_SET_OFFSETS,setOffsets);
	
	if(secondOp)
	{
		//check for point object
		if(!IsValidPointObject(secondOp)) usePoint = false;
		//check for polygon object
		if(!IsValidPolygonObject(secondOp)) usePoly = false;
	}
	else
	{
		usePoint = false;
		usePoly = false;
	}
	
	DoEnable();
	
	BaseContainer bc;
	bc.SetBool(IDS_POINT,usePoint);
	bc.SetBool(IDS_NORMAL,usePoly);
	bc.SetBool(IDC_SET_OFFSETS,setOffsets);
	SetWorldPluginData(ID_CDADDPCONSTRAINT,bc,false);

	return true;
}

void CDAddPConstraintDialog::DoEnable(void)
{
	if(!secondOp)
	{
		SetBool(IDS_POINT,false);
		Enable(IDS_POINT,false);
		SetBool(IDS_NORMAL,false);
		Enable(IDS_NORMAL,false);
		SetBool(IDC_SET_OFFSETS,false);
		Enable(IDC_SET_OFFSETS,false);
	}
	else
	{
		if(!IsValidPointObject(secondOp))
		{
			SetBool(IDS_POINT,false);
			Enable(IDS_POINT,false);
			SetBool(IDS_NORMAL,false);
			Enable(IDS_NORMAL,false);
		}
		else
		{
			if(!IsValidPolygonObject(secondOp))
			{
				Enable(IDS_POINT,true);
				SetBool(IDS_NORMAL,false);
				Enable(IDS_NORMAL,false);
			}
			else
			{
				if(!usePoly && !usePoint)
				{
					Enable(IDS_POINT,true);
					Enable(IDS_NORMAL,true);
				}
				else if(usePoly)
				{
					SetBool(IDS_POINT,false);
					Enable(IDS_POINT,false);
					Enable(IDS_NORMAL,true);
				}
				else if(usePoint)
				{
					Enable(IDS_POINT,true);
					SetBool(IDS_NORMAL,false);
					Enable(IDS_NORMAL,false);
				}
			}
		}
	}
}

class CDAddPConstraint : public CommandData
{
	private:
		CDAddPConstraintDialog dlg;
		
		void SetOptions(BaseTag *pTag, BaseObject *op, BaseContainer *tData, LONG cnt);
		
	public:
		virtual Bool Execute(BaseDocument* doc);

};

void CDAddPConstraint::SetOptions(BaseTag *pTag, BaseObject *op, BaseContainer *tData, LONG cnt)
{
	DescriptionCommand dc;
	
	if(dlg.usePoint && IsValidPointObject(op))
	{
		tData->SetBool(PC_USE_POINT+cnt,dlg.usePoint);
		dc.id = DescID(PC_SET_P_SEL+cnt);
		pTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	}
	else if(dlg.usePoly && IsValidPolygonObject(op))
	{
		tData->SetBool(PC_USE_NORMAL+cnt,dlg.usePoly);
		dc.id = DescID(PC_SET_P_SEL+cnt);
		pTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	}
}

Bool CDAddPConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		BaseObject *target = NULL;
		
		Matrix opM = destObject->GetMg();
		
		BaseTag *pTag = NULL;
		Bool setOptions = false, offsetEnabled = true;
		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		
		if(opCount == 2) dlg.secondOp = static_cast<BaseObject*>(opSelLog->GetIndex(1));
		else dlg.secondOp = NULL;
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
		{
			if(!dlg.Open()) return false;
			setOptions = true;
		}
		
		if(!destObject->GetTag(ID_CDPCONSTRAINTPLUGIN))
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();

			pTag = BaseTag::Alloc(ID_CDPCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(pTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,pTag);
			pTag->Message(MSG_MENUPREPARE);
		}
		else
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();

			pTag = destObject->GetTag(ID_CDPCONSTRAINTPLUGIN);
			offsetEnabled = false;
		}
		
		BaseContainer *tData = pTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *wpData = GetWorldPluginData(ID_CDADDPCONSTRAINT);
			if(!wpData)
			{
				BaseContainer bc;
				bc.SetBool(IDS_POINT,false);
				bc.SetBool(IDS_NORMAL,false);
				bc.SetBool(IDC_SET_OFFSETS,false);
				SetWorldPluginData(ID_CDADDPCONSTRAINT,bc,false);
				wpData = GetWorldPluginData(ID_CDADDPCONSTRAINT);
			}
			
			LONG i, cnt = tData->GetLong(PC_TARGET_COUNT);
			if(opCount > 1)
			{
				Vector mixPosition = Vector(0,0,0);
				DescriptionCommand dc;
				for(i=1; i<opCount; i++)
				{
					if(cnt < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(cnt == 1 && tData->GetObjectLink(PC_TARGET,doc) == NULL)
						{
							tData->SetLink(PC_TARGET,target);
							
							if(setOptions) SetOptions(pTag,target,tData,0);
						}
						else
						{
							dc.id = DescID(PC_ADD_POS);
							pTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,pTag);
							tData->SetLink(PC_TARGET+cnt,target);
							
							if(setOptions) SetOptions(pTag,target,tData,cnt);
							
							cnt = tData->GetLong(PC_TARGET_COUNT);
						}
						mixPosition += target->GetMg().off * (1.0/Real(opCount-1));
					}
				}
				if(setOptions && offsetEnabled && wpData->GetBool(IDC_SET_OFFSETS))
				{
					Matrix targetM = Matrix();
					targetM.off = mixPosition;
					Matrix localM = MInv(targetM) * opM;
					tData->SetReal(PC_OFFSET_X,localM.off.x);
					tData->SetReal(PC_OFFSET_Y,localM.off.y);
					tData->SetReal(PC_OFFSET_Z,localM.off.z);
				}
				else
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,destObject);
					opM.off = mixPosition;
					destObject->SetMg(opM);
				}
			}

			doc->SetActiveObject(target);
			doc->SetActiveTag(pTag);
			pTag->Message(MSG_UPDATE);
		}
		
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddPConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddPConstraint(void)
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
	String name=GeLoadString(IDS_CDADDP); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDPCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddPConst.tif","CD Add Position Constraint",CDDataAllocator(CDAddPConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDPCONSTRAINT,name,0,"CDAddPConst.tif","CD Add Position Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddPConstraint));
}

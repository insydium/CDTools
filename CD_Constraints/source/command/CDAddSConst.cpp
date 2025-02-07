//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "tCDSConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	9

enum
{
	SC_TARGET_COUNT			= 1040
};

class CDAddSConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		Bool setOffsets;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddSConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDS));
		
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
				
				AddCheckbox(IDC_SET_OFFSETS,BFH_LEFT,0,0,GeLoadString(IDC_SET_OFFSETS));
			}
			GroupEnd();
			
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddSConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDADDSCONSTRAINT);
	if(!bc)
	{
		SetBool(IDC_SET_OFFSETS,false);
	}
	else
	{
		SetBool(IDC_SET_OFFSETS,bc->GetBool(IDC_SET_OFFSETS));
	}
	
	return true;
}

Bool CDAddSConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDC_SET_OFFSETS,setOffsets);
	
	BaseContainer bc;
	bc.SetBool(IDC_SET_OFFSETS,setOffsets);
	SetWorldPluginData(ID_CDADDSCONSTRAINT,bc,false);
	
	return true;
}

class CDAddSConstraint : public CommandData
{
	private:
		CDAddSConstraintDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddSConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		

		BaseTag *sTag = NULL;
		Bool setOptions = false;
		if(!destObject->GetTag(ID_CDSCONSTRAINTPLUGIN))
		{
			BaseContainer state;
			GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
			if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
			{
				if(!dlg.Open()) return false;
				setOptions = true;
			}
			doc->SetActiveObject(NULL);
			doc->StartUndo();
			sTag = BaseTag::Alloc(ID_CDSCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(sTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,sTag);
			sTag->Message(MSG_MENUPREPARE);
		}
		else
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();

			sTag = destObject->GetTag(ID_CDSCONSTRAINTPLUGIN);
		}
		
		BaseContainer *tData = sTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *bc = GetWorldPluginData(ID_CDADDSCONSTRAINT);
			if(!bc)
			{
				BaseContainer wpbc;
				wpbc.SetBool(IDC_SET_OFFSETS,false);
				SetWorldPluginData(ID_CDADDSCONSTRAINT,wpbc,false);
				bc = GetWorldPluginData(ID_CDADDSCONSTRAINT);
			}

			BaseObject *target = NULL;
			LONG i, sCount = tData->GetLong(SC_TARGET_COUNT);
			if(opCount > 1)
			{
				Vector mixScale = Vector(0,0,0);
				for(i=1; i<opCount; i++)
				{
					if(sCount < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(sCount == 1 && tData->GetObjectLink(SC_TARGET,doc) == NULL)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,sTag);
							tData->SetLink(SC_TARGET,target);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(SC_ADD_SCA);
							sTag->Message(MSG_DESCRIPTION_COMMAND,&dc);

							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,sTag);
							tData->SetLink(SC_TARGET+sCount,target);
							
							sCount = tData->GetLong(SC_TARGET_COUNT);
						}
						mixScale += CDGetScale(target) * (1.0/Real(opCount-1));
					}
				}
				if(setOptions && bc->GetBool(IDC_SET_OFFSETS))
				{
					Matrix targetM = MatrixScale(mixScale);
					tData->SetReal(SC_OFFSET_X,Len(destObject->GetMg().v1) - Len(targetM.v1));
					tData->SetReal(SC_OFFSET_Y,Len(destObject->GetMg().v2) - Len(targetM.v2));
					tData->SetReal(SC_OFFSET_Z,Len(destObject->GetMg().v3) - Len(targetM.v3));
				}
				else
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,destObject);
					CDSetScale(destObject,mixScale);
				}
			}

			doc->SetActiveObject(target);
			doc->SetActiveTag(sTag);
			sTag->Message(MSG_UPDATE);
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddSConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};
Bool RegisterCDAddSConstraint(void)
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
	String name=GeLoadString(IDS_CDADDS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDSCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddSConst.tif","CD Add Scale Constraint",CDDataAllocator(CDAddSConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDSCONSTRAINT,name,0,"CDAddSConst.tif","CD Add Scale Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddSConstraint));
}

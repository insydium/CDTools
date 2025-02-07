//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tCDTAConstraint.h"
#include "CDConstraint.h"

class CDAddTAConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddTAConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDTA));
		
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
				
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_SPEED),0);
				AddEditNumberArrows(IDC_SPEED,BFH_LEFT);
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_LENGTH),0);
				AddEditNumberArrows(IDC_LENGTH,BFH_LEFT);
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_HEIGHT),0);
				AddEditNumberArrows(IDC_HEIGHT,BFH_LEFT);
			}
			GroupEnd();
			
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddTAConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDADDTACONSTRAINT);
	if(!wpData)
	{
		SetReal(IDC_SPEED,0.5,0.0,1.0,0.01,FORMAT_PERCENT);
		SetReal(IDC_LENGTH,300.0,0.0,CDMAXREAL,1.0,FORMAT_REAL);
		SetReal(IDC_HEIGHT,100.0,0.0,CDMAXREAL,1.0,FORMAT_REAL);
	}
	else
	{
		SetReal(IDC_SPEED,wpData->GetReal(IDC_SPEED),0.0,1.0,0.01,FORMAT_PERCENT);
		SetReal(IDC_LENGTH,wpData->GetReal(IDC_LENGTH),0.0,CDMAXREAL,1.0,FORMAT_REAL);
		SetReal(IDC_HEIGHT,wpData->GetReal(IDC_HEIGHT),0.0,CDMAXREAL,1.0,FORMAT_REAL);
	}
	
	return true;
}

Bool CDAddTAConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	Real speed, length, height;

	GetReal(IDC_SPEED,speed);
	GetReal(IDC_LENGTH,length);
	GetReal(IDC_HEIGHT,height);
	
	BaseContainer bc;
	bc.SetReal(IDC_SPEED,speed);
	bc.SetReal(IDC_LENGTH,length);
	bc.SetReal(IDC_HEIGHT,height);
	SetWorldPluginData(ID_CDADDTACONSTRAINT,bc,false);
	
	return true;
}

class CDAddTAConstraint : public CommandData
{
	private:
		CDAddTAConstraintDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddTAConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 1)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		BaseObject *target = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
		if(!target) return false;
		
		if(opCount > 2)
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return true;
		}
		if(destObject->GetTag(ID_CDTACONSTRAINTPLUGIN)) return true;

		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
		{
			if(!dlg.Open()) return false;
		}

		doc->SetActiveObject(NULL);
		doc->StartUndo();

		BaseTag *taTag = BaseTag::Alloc(ID_CDTACONSTRAINTPLUGIN);
		
		BaseTag *prev = GetPreviousConstraintTag(destObject);
		destObject->InsertTag(taTag,prev);
		
		CDAddUndo(doc,CD_UNDO_NEW,taTag);
		taTag->Message(MSG_MENUPREPARE);
		
		CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,taTag);
		BaseContainer *tData = taTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *wpData = GetWorldPluginData(ID_CDADDTACONSTRAINT);
			if(!wpData)
			{
				BaseContainer bc;
				bc.SetReal(IDC_SPEED,0.5);
				bc.SetReal(IDC_LENGTH,300.0);
				bc.SetReal(IDC_HEIGHT,100.0);
				SetWorldPluginData(ID_CDADDTACONSTRAINT,bc,false);
				wpData = GetWorldPluginData(ID_CDADDTACONSTRAINT);
			}

			tData->SetLink(TAC_TARGET,target);
			tData->SetReal(TAC_SPEED,wpData->GetReal(IDC_SPEED));
			tData->SetReal(TAC_LENGTH,wpData->GetReal(IDC_LENGTH));
			tData->SetReal(TAC_HEIGHT,wpData->GetReal(IDC_HEIGHT));
			
			doc->SetActiveObject(target);
			doc->SetActiveTag(taTag);
			taTag->Message(MSG_UPDATE);
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddTAConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddTAConstraint(void)
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
	String name=GeLoadString(IDS_CDADDTA); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDTACONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddTAConst.tif","CD Add Tag Along Constraint",CDDataAllocator(CDAddTAConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDTACONSTRAINT,name,0,"CDAddTAConst.tif","CD Add Tag Along Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddTAConstraint));
}

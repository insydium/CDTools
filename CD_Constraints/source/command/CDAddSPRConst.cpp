//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDConstraint.h"

#include "tCDSPRConstraint.h"

class CDAddSPRConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		Bool	lin, rot, offs, grav;
		Real	mass;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddSPRConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDSPR));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDS_SPRING_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_SCALEFIT,3,0,"",0);
					GroupSpace(4,1);
					AddCheckbox (IDC_LINEAR_SPRING,BFH_LEFT,0,0,GeLoadString(IDC_LINEAR_SPRING));
					AddCheckbox (IDC_ROTATIONAL_SPRING,BFH_RIGHT,0,0,GeLoadString(IDC_ROTATIONAL_SPRING));
					AddCheckbox (IDC_SET_OFFSETS,BFH_RIGHT,0,0,GeLoadString(IDC_SET_OFFSETS));
				GroupEnd();

				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					GroupSpace(4,1);
					AddCheckbox (IDC_USE_GRAVITY,BFH_LEFT,0,0,GeLoadString(IDC_USE_GRAVITY));
					GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					{
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_SET_MASS),0);
						AddEditNumberArrows(IDC_SET_MASS,BFH_LEFT);
					}
					GroupEnd();
				GroupEnd();
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddSPRConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDADDSPRCONSTRAINT);
	if(!wpData)
	{
		SetBool(IDC_LINEAR_SPRING,true);
		SetBool(IDC_ROTATIONAL_SPRING,false);
		SetBool(IDC_SET_OFFSETS,false);
		SetBool(IDC_USE_GRAVITY,true);
		SetReal(IDC_SET_MASS,10.0,1.0,100.0,0.1,FORMAT_REAL);
	}
	else
	{
		SetBool(IDC_LINEAR_SPRING,wpData->GetBool(IDC_LINEAR_SPRING));
		SetBool(IDC_ROTATIONAL_SPRING,wpData->GetBool(IDC_ROTATIONAL_SPRING));
		SetBool(IDC_SET_OFFSETS,wpData->GetBool(IDC_SET_OFFSETS));
		SetBool(IDC_USE_GRAVITY,wpData->GetBool(IDC_USE_GRAVITY));
		SetReal(IDC_SET_MASS,wpData->GetReal(IDC_SET_MASS),1.0,100.0,0.1,FORMAT_REAL);
		
	}

	return true;
}

Bool CDAddSPRConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDC_LINEAR_SPRING,lin);
	GetBool(IDC_ROTATIONAL_SPRING,rot);
	GetBool(IDC_SET_OFFSETS,offs);
	GetBool(IDC_USE_GRAVITY,grav);
	GetReal(IDC_SET_MASS,mass);
	
	BaseContainer bc;
	bc.SetBool(IDC_LINEAR_SPRING,lin);
	bc.SetBool(IDC_ROTATIONAL_SPRING,rot);
	bc.SetBool(IDC_SET_OFFSETS,offs);
	bc.SetBool(IDC_USE_GRAVITY,grav);
	bc.SetReal(IDC_SET_MASS,mass);
	SetWorldPluginData(ID_CDADDSPRCONSTRAINT,bc,false);

	return true;
}

class CDAddSPRConstraint : public CommandData
{
	private:
		CDAddSPRConstraintDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddSPRConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 1)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		
		BaseObject *anchor = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
		if(!anchor) return false;
		
		if(opCount > 3)
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return true;
		}
		if(destObject->GetTag(ID_CDSPRCONSTRAINTPLUGIN)) return true;
		
		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
		{
			if(!dlg.Open()) return false;
		}

		doc->SetActiveObject(NULL);
		doc->StartUndo();

		BaseTag *sprTag = BaseTag::Alloc(ID_CDSPRCONSTRAINTPLUGIN);
		
		BaseTag *prev = GetPreviousConstraintTag(destObject);
		destObject->InsertTag(sprTag,prev);
		
		CDAddUndo(doc,CD_UNDO_NEW,sprTag);
		sprTag->Message(MSG_MENUPREPARE);
		
		CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,sprTag);
		BaseContainer *tData = sprTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *wpData = GetWorldPluginData(ID_CDADDSPRCONSTRAINT);
			if(!wpData)
			{
				BaseContainer bc;
				bc.SetBool(IDC_LINEAR_SPRING,true);
				bc.SetBool(IDC_ROTATIONAL_SPRING,false);
				bc.SetBool(IDC_SET_OFFSETS,false);
				bc.SetBool(IDC_USE_GRAVITY,true);
				bc.SetReal(IDC_SET_MASS,10.0);
				SetWorldPluginData(ID_CDADDSPRCONSTRAINT,bc,false);
				wpData = GetWorldPluginData(ID_CDADDSPRCONSTRAINT);
			}
			
			tData->SetLink(SPRC_TARGET,anchor);
			if(opCount > 2)
			{
				BaseObject *attachment = static_cast<BaseObject*>(opSelLog->GetIndex(2));
				tData->SetLink(SPRC_ATTACHMENT,attachment);
			}
			
			tData->SetBool(SPRC_USE_POSITION,wpData->GetBool(IDC_LINEAR_SPRING));
			tData->SetBool(SPRC_USE_ROTATION,wpData->GetBool(IDC_ROTATIONAL_SPRING));
			tData->SetReal(SPRC_MASS,wpData->GetReal(IDC_SET_MASS));
			if(wpData->GetBool(IDC_SET_OFFSETS))
			{
				Matrix localM = MInv(anchor->GetMg()) * destObject->GetMg();
				tData->SetVector(SPRC_P_OFFSET,localM.off);
				Vector offset = CDMatrixToHPB(localM);
				if(offset.x > Rad(180.0)) offset.x -= Rad(360.0);
				if(offset.y > Rad(180.0)) offset.y -= Rad(360.0);
				if(offset.z > Rad(180.0)) offset.z -= Rad(360.0);
				tData->SetVector(SPRC_R_OFFSET,offset);
			}
			
			if(wpData->GetBool(IDC_USE_GRAVITY)) tData->SetReal(SPRC_GRAVITY,9.8);
			
			doc->SetActiveObject(anchor);
			doc->SetActiveTag(sprTag);
			sprTag->Message(MSG_UPDATE);
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddSPRConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddSPRConstraint(void)
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
	String name=GeLoadString(IDS_CDADDSPR); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDSPRCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddSPRConst.tif","CD Add Spring Constraint",CDDataAllocator(CDAddSPRConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDSPRCONSTRAINT,name,0,"CDAddSPRConst.tif","CD Add Spring Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddSPRConstraint));
}

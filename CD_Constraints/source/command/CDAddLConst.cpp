//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDConstraint.h"

#include "tCDLConstraint.h"

class CDAddLConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);

		void DoEnable(void);
};

Bool CDAddLConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDL));
		
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
				
				GroupBegin(0,BFH_LEFT,4,0,"",0);
				{
					AddCheckbox(L_ENABLE_POS,BFH_LEFT,0,0,GeLoadString(L_ENABLE_POS));
					AddCheckbox(P_AXIS_X,BFH_LEFT,0,0,GeLoadString(P_AXIS_X));
					AddCheckbox(P_AXIS_Y,BFH_LEFT,0,0,GeLoadString(P_AXIS_Y));
					AddCheckbox(P_AXIS_Z,BFH_LEFT,0,0,GeLoadString(P_AXIS_Z));
				}
				GroupEnd();
			
				GroupBegin(0,BFH_LEFT,5,0,"",0);
				{
					GroupBorderSpace(0,8,0,0);
					AddCheckbox(L_ENABLE_AXIS,BFH_LEFT,0,0,GeLoadString(L_ENABLE_AXIS));
					
					AddRadioGroup(L_GROUP,BFH_CENTER,4,1);
						AddChild(L_GROUP, 0, GeLoadString(L_AXIS_ALL));
						AddChild(L_GROUP, 1, GeLoadString(L_AXIS_X));
						AddChild(L_GROUP, 2, GeLoadString(L_AXIS_Y));
						AddChild(L_GROUP, 3, GeLoadString(L_AXIS_Z));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					GroupBorderSpace(0,8,0,0);
					AddCheckbox(L_USE_ORIENTATION,BFH_LEFT,0,0,GeLoadString(L_USE_ORIENTATION));
					
				}
				GroupEnd();
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddLConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDADDLCONSTRAINT);
	if(!bc)
	{
		SetBool(L_ENABLE_POS,true);
		SetBool(P_AXIS_X,true);
		SetBool(P_AXIS_Y,true);
		SetBool(P_AXIS_Z,true);
		
		SetBool(L_ENABLE_AXIS,false);
		SetLong(L_GROUP,0);

		SetBool(L_USE_ORIENTATION,false);
	}
	else
	{
		SetBool(L_ENABLE_POS,bc->GetBool(L_ENABLE_POS));
		SetBool(P_AXIS_X,bc->GetBool(P_AXIS_X));
		SetBool(P_AXIS_Y,bc->GetBool(P_AXIS_Y));
		SetBool(P_AXIS_Z,bc->GetBool(P_AXIS_Z));
		
		SetBool(L_ENABLE_AXIS,bc->GetBool(L_ENABLE_AXIS));
		SetLong(L_GROUP,bc->GetLong(L_GROUP));

		SetBool(L_USE_ORIENTATION,bc->GetBool(L_USE_ORIENTATION));
	}
	
	DoEnable();
	
	return true;
}

Bool CDAddLConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool posEn, pX, pY, pZ, axEn, oEn;
	LONG axis;
	
	GetBool(L_ENABLE_POS,posEn);
	GetBool(P_AXIS_X,pX);
	GetBool(P_AXIS_Y,pY);
	GetBool(P_AXIS_Z,pZ);

	GetBool(L_ENABLE_AXIS,axEn);
	GetLong(L_GROUP,axis);

	GetBool(L_USE_ORIENTATION,oEn);
	
	BaseContainer bc;
	bc.SetBool(L_ENABLE_POS,posEn);
	bc.SetBool(P_AXIS_X,pX);
	bc.SetBool(P_AXIS_Y,pY);
	bc.SetBool(P_AXIS_Z,pZ);
	bc.SetBool(L_ENABLE_AXIS,axEn);
	bc.SetLong(L_GROUP,axis);
	bc.SetBool(L_USE_ORIENTATION,oEn);
	SetWorldPluginData(ID_CDADDLCONSTRAINT,bc,false);
	
	DoEnable();
	
	return true;
}

void CDAddLConstraintDialog::DoEnable(void)
{
	Bool posEn, axEn;
	
	GetBool(L_ENABLE_POS,posEn);
	GetBool(L_ENABLE_AXIS,axEn);
	
	if(!posEn)
	{
		Enable(P_AXIS_X,false);
		Enable(P_AXIS_Y,false);
		Enable(P_AXIS_Z,false);
	}
	else
	{
		Enable(P_AXIS_X,true);
		Enable(P_AXIS_Y,true);
		Enable(P_AXIS_Z,true);
	}
	
	if(!axEn) Enable(L_GROUP,false);
	else Enable(L_GROUP,true);
}

class CDAddLConstraint : public CommandData
{
	private:
		CDAddLConstraintDialog dlg;
		
		void SetOptions(BaseObject *op, BaseContainer *tData, BaseContainer *wpData);

	public:
		virtual Bool Execute(BaseDocument* doc);

};

void CDAddLConstraint::SetOptions(BaseObject *op, BaseContainer *tData, BaseContainer *wpData)
{
	if(wpData->GetBool(L_ENABLE_POS))
	{
		Vector pos = CDGetPos(op);
		
		if(wpData->GetBool(P_AXIS_X))
		{
			tData->SetReal(LC_POSITION_X,pos.x);
			tData->SetLong(LC_P_LOCK_X,true);
		}
		if(wpData->GetBool(P_AXIS_Y))
		{
			tData->SetReal(LC_POSITION_Y,pos.y);
			tData->SetLong(LC_P_LOCK_Y,true);
		}
		if(wpData->GetBool(P_AXIS_Z))
		{
			tData->SetReal(LC_POSITION_Z,pos.z);
			tData->SetLong(LC_P_LOCK_Z,true);
		}
	}
	
	if(wpData->GetBool(L_ENABLE_AXIS))
	{
		switch(wpData->GetLong(L_GROUP))
		{
			case 0:
				tData->SetLong(LC_A_LOCK_ALL,true);
				break;
			case 1:
				tData->SetLong(LC_A_LOCK_X,true);
				break;
			case 2:
				tData->SetLong(LC_A_LOCK_Y,true);
				break;
			case 3:
				tData->SetLong(LC_A_LOCK_Z,true);
				break;
		}
	}

	if(wpData->GetBool(L_USE_ORIENTATION)) tData->SetBool(LC_USE_ORIENTATION,true);
}

Bool CDAddLConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	BaseTag *tag = NULL;
	
	AutoAlloc<AtomArray> opActiveList; if(!opActiveList) return false;
	CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG i, activeCnt = opActiveList->GetCount();
	if(activeCnt > 0)
	{
		Bool addLock = false;
		for(i=0; i<activeCnt; i++)
		{
			op = static_cast<BaseObject*>(opActiveList->GetIndex(i));
			if(op)
			{
				if(!op->GetTag(ID_CDLCONSTRAINTPLUGIN)) addLock = true;
			}
		}
		if(addLock)
		{
			BaseContainer state;
			GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
			if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
			{
				if(!dlg.Open()) return false;
			}
			
			BaseContainer *wpData = GetWorldPluginData(ID_CDADDLCONSTRAINT);
			if(!wpData)
			{
				BaseContainer bc;
				bc.SetBool(C_LOCAL_AXIS,false);
				bc.SetLong(C_CLAMP_AXIS,1);
				SetWorldPluginData(ID_CDADDLCONSTRAINT,bc,false);
				wpData = GetWorldPluginData(ID_CDADDLCONSTRAINT);
			}
			
			doc->StartUndo();
			for(i=0; i<activeCnt; i++)
			{
				op = static_cast<BaseObject*>(opActiveList->GetIndex(i));
				if(op)
				{
					if(!op->GetTag(ID_CDLCONSTRAINTPLUGIN))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						
						tag = BaseTag::Alloc(ID_CDLCONSTRAINTPLUGIN);
						
						BaseTag *prev = GetPreviousConstraintTag(op);
						op->InsertTag(tag,prev);
						
						CDAddUndo(doc,CD_UNDO_NEW,tag);
						tag->Message(MSG_MENUPREPARE);
						
						BaseContainer *tData = tag->GetDataInstance();
						if(tData) SetOptions(op,tData,wpData);
						
						tag->Message(MSG_UPDATE);
					}
				}
			}
			doc->SetActiveObject(op);
			doc->SetActiveTag(tag);
			
			doc->EndUndo();
			EventAdd(EVENT_FORCEREDRAW);
		}
	}
	
	return true;
}

class CDAddLConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddLConstraint(void)
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
	String name=GeLoadString(IDS_CDADDL); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDLCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddLConst.tif","CD Add Lock Constraint",CDDataAllocator(CDAddLConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDLCONSTRAINT,name,0,"CDAddLConst.tif","CD Add Lock Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddLConstraint));
}

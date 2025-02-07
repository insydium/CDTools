//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "tCDRConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	10

enum
{
	RC_TARGET_COUNT			= 1036
};

class CDAddRConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		Bool setOffsets;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddRConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDR));
		
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

Bool CDAddRConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDADDRCONSTRAINT);
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

Bool CDAddRConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDC_SET_OFFSETS,setOffsets);
	
	BaseContainer bc;
	bc.SetBool(IDC_SET_OFFSETS,setOffsets);
	SetWorldPluginData(ID_CDADDRCONSTRAINT,bc,false);
	
	return true;
}

class CDAddRConstraint : public CommandData
{
	private:
		CDAddRConstraintDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddRConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		
		BaseTag *rTag = NULL;
		Bool setOptions = false;
		if(!destObject->GetTag(ID_CDRCONSTRAINTPLUGIN))
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
			
			rTag = BaseTag::Alloc(ID_CDRCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(rTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,rTag);
			rTag->Message(MSG_UPDATE);
			rTag->Message(MSG_MENUPREPARE);
		}
		else
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();

			rTag = destObject->GetTag(ID_CDRCONSTRAINTPLUGIN);
		}
		
		BaseContainer *tData = rTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *bc = GetWorldPluginData(ID_CDADDRCONSTRAINT);
			if(!bc)
			{
				BaseContainer wpbc;
				wpbc.SetBool(IDC_SET_OFFSETS,false);
				SetWorldPluginData(ID_CDADDRCONSTRAINT,wpbc,false);
				bc = GetWorldPluginData(ID_CDADDRCONSTRAINT);
			}

			BaseObject *target = NULL;
			LONG i, rCount = tData->GetLong(RC_TARGET_COUNT);
			
			if(opCount > 1)
			{
				Real angle = 0;
				Vector axis = Vector(0,0,0);
				CDQuaternion qA, qB, mixQuat;
				for(i=1; i<opCount; i++)
				{
					if(rCount < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(rCount == 1 && tData->GetObjectLink(RC_TARGET,doc) == NULL)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,rTag);
							tData->SetLink(RC_TARGET,target);
						}
						else
						{
							rCount = tData->GetLong(RC_TARGET_COUNT);
							
							DescriptionCommand dc;
							dc.id = DescID(RC_ADD_ROT);
							rTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,rTag);
							tData->SetLink(RC_TARGET+rCount,target);
						}
						qA.SetMatrix(target->GetMg());
						axis += qA.v * (1.0/Real(opCount-1));
						angle += qA.w * (1.0/Real(opCount-1));
					}
				}
				
				qB = CDQuaternion(angle, axis.x, axis.y, axis.z);
				mixQuat = !qB;
				Matrix targetM = mixQuat.GetMatrix();
				targetM.off = destObject->GetMg().off;
				if(setOptions && bc->GetBool(IDC_SET_OFFSETS))
				{
					Matrix localM = MInv(targetM) * destObject->GetMg();
					Vector rot = CDMatrixToHPB(localM);
					if(rot.x > Rad(180.0)) rot.x -= Rad(360.0);
					if(rot.y > Rad(180.0)) rot.y -= Rad(360.0);
					if(rot.z > Rad(180.0)) rot.z -= Rad(360.0);
					tData->SetReal(RC_OFFSET_X,rot.x);
					tData->SetReal(RC_OFFSET_Y,rot.y);
					tData->SetReal(RC_OFFSET_Z,rot.z);
				}
				else
				{
					Vector theScale = CDGetScale(destObject);
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,destObject);
					destObject->SetMg(targetM);
					CDSetScale(destObject,theScale);
				}
			}

			doc->SetActiveObject(target);
			doc->SetActiveTag(rTag);
			rTag->Message(MSG_UPDATE);
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddRConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddRConstraint(void)
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
	String name=GeLoadString(IDS_CDADDR); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDRCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddRConst.tif","CD Add Rotation Constraint",CDDataAllocator(CDAddRConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDRCONSTRAINT,name,0,"CDAddRConst.tif","CD Add Rotation Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddRConstraint));
}

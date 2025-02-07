//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "tCDPSRConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	9

enum
{
	PSRC_TARGET_COUNT			= 1034
};

class CDAddPSRConstraintDialog : public GeModalDialog
{
	private:
		CDCOptionsUA 			ua;
		
	public:	
		Bool pos, sca, rot, setOffsets;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddPSRConstraintDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDADDPSR));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,3,0,GeLoadString(IDS_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddCheckbox(P_GROUP,BFH_LEFT,0,0,GeLoadString(P_GROUP));
				AddCheckbox(S_GROUP,BFH_LEFT,0,0,GeLoadString(S_GROUP));
				AddCheckbox(R_GROUP,BFH_LEFT,0,0,GeLoadString(R_GROUP));
				AddCheckbox(IDC_SET_OFFSETS,BFH_LEFT,0,0,GeLoadString(IDC_SET_OFFSETS));
			}
			GroupEnd();
			
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddPSRConstraintDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDADDPSRCONSTRAINT);
	if(!bc)
	{
		SetBool(P_GROUP,true);
		SetBool(S_GROUP,true);
		SetBool(R_GROUP,true);
		SetBool(IDC_SET_OFFSETS,false);
	}
	else
	{
		SetBool(P_GROUP,bc->GetBool(P_GROUP));
		SetBool(S_GROUP,bc->GetBool(S_GROUP));
		SetBool(R_GROUP,bc->GetBool(R_GROUP));
		SetBool(IDC_SET_OFFSETS,bc->GetBool(IDC_SET_OFFSETS));
	}
		
	return true;
}

Bool CDAddPSRConstraintDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(P_GROUP,pos);
	GetBool(S_GROUP,sca);
	GetBool(R_GROUP,rot);
	GetBool(IDC_SET_OFFSETS,setOffsets);
	
	BaseContainer bc;
	bc.SetBool(P_GROUP,pos);
	bc.SetBool(S_GROUP,sca);
	bc.SetBool(R_GROUP,rot);
	bc.SetBool(IDC_SET_OFFSETS,setOffsets);
	SetWorldPluginData(ID_CDADDPSRCONSTRAINT,bc,false);
	
	return true;
}

class CDAddPSRConstraint : public CommandData
{
	private:
		CDAddPSRConstraintDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddPSRConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		
		BaseTag *psrTag = NULL;
		Bool setOptions = false;
		if(!destObject->GetTag(ID_CDPSRCONSTRAINTPLUGIN))
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

			psrTag = BaseTag::Alloc(ID_CDPSRCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(psrTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,psrTag);
			psrTag->Message(MSG_UPDATE);
			psrTag->Message(MSG_MENUPREPARE);
		}
		else
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();

			psrTag = destObject->GetTag(ID_CDPSRCONSTRAINTPLUGIN);
		}
		
		BaseContainer *tData = psrTag->GetDataInstance();
		if(tData)
		{
			BaseContainer *wpData = GetWorldPluginData(ID_CDADDPSRCONSTRAINT);
			if(!wpData)
			{
				BaseContainer wpbc;
				wpbc.SetBool(P_GROUP,true);
				wpbc.SetBool(S_GROUP,true);
				wpbc.SetBool(R_GROUP,true);
				wpbc.SetBool(IDC_SET_OFFSETS,false);
				SetWorldPluginData(ID_CDADDPSRCONSTRAINT,wpbc,false);
				wpData = GetWorldPluginData(ID_CDADDPSRCONSTRAINT);
			}
			
			BaseObject *target = NULL;
			LONG i, cnt = tData->GetLong(PSRC_TARGET_COUNT);
			if(opCount > 1)
			{
				Real angle = 0;
				Vector axis = Vector(0,0,0);
				Vector mixPosition = Vector(0,0,0);
				Vector mixScale = Vector(0,0,0);
				Vector destScale = CDGetScale(destObject);
				CDQuaternion qA, qB, mixQuat;
				for(i=1; i<opCount; i++)
				{
					if(cnt < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(cnt == 1 && tData->GetObjectLink(PSRC_TARGET,doc) == NULL)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,psrTag);
							tData->SetLink(PSRC_TARGET,target);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(PSRC_ADD_PSR);
							psrTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,psrTag);
							tData->SetLink(PSRC_TARGET+cnt,target);
							
							cnt = tData->GetLong(PSRC_TARGET_COUNT);
						}
						qA.SetMatrix(target->GetMg());
						axis += qA.v * (1.0/Real(opCount-1));
						angle += qA.w * (1.0/Real(opCount-1));
						mixScale += CDGetScale(target) * (1.0/Real(opCount-1));
						mixPosition += target->GetMg().off * (1.0/Real(opCount-1));
					}
				}
				qB = CDQuaternion(angle, axis.x, axis.y, axis.z);
				mixQuat = !qB;
				
				Matrix targetM, localM, targetPM = Matrix(), targetRM = Matrix(), targetSM = Matrix();
				targetPM.off = destObject->GetMg().off;
				
				if(!wpData->GetBool(P_GROUP)) tData->SetBool(PSRC_USE_P, false);
				else targetPM.off = mixPosition;

				if(!wpData->GetBool(S_GROUP)) tData->SetBool(PSRC_USE_S, false);
				else targetSM = MatrixScale(mixScale);
				
				if(!wpData->GetBool(R_GROUP)) tData->SetBool(PSRC_USE_R, false);
				else targetRM = mixQuat.GetMatrix();
				
				if(setOptions && wpData->GetBool(IDC_SET_OFFSETS))
				{
					if(dlg.pos)
					{
						localM = MInv(targetPM) * destObject->GetMg();
						tData->SetReal(P_OFFSET_X,localM.off.x);
						tData->SetReal(P_OFFSET_Y,localM.off.y);
						tData->SetReal(P_OFFSET_Z,localM.off.z);
					}
					if(dlg.sca)
					{
						tData->SetReal(S_OFFSET_X,Len(destObject->GetMg().v1) - Len(targetSM.v1));
						tData->SetReal(S_OFFSET_Y,Len(destObject->GetMg().v2) - Len(targetSM.v2));
						tData->SetReal(S_OFFSET_Z,Len(destObject->GetMg().v3) - Len(targetSM.v3));
					}
					if(dlg.rot)
					{
						localM = MInv(targetRM) * destObject->GetMg();
						Vector rot = CDMatrixToHPB(localM);
						if(rot.x > Rad(180.0)) rot.x -= Rad(360.0);
						if(rot.y > Rad(180.0)) rot.y -= Rad(360.0);
						if(rot.z > Rad(180.0)) rot.z -= Rad(360.0);
						tData->SetReal(R_OFFSET_X,rot.x);
						tData->SetReal(R_OFFSET_Y,rot.y);
						tData->SetReal(R_OFFSET_Z,rot.z);
					}
				}
				else
				{
					targetM = targetRM * targetSM;
					targetM.off = targetPM.off;
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,destObject);
					destObject->SetMg(targetM);
					if(!wpData->GetBool(S_GROUP)) CDSetScale(destObject,destScale);
				}
			}

			doc->SetActiveObject(target);
			doc->SetActiveTag(psrTag);
			psrTag->Message(MSG_UPDATE);
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddPSRConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddPSRConstraint(void)
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
	String name=GeLoadString(IDS_CDADDPSR); if (!name.Content()) return true;
	
	if(!reg) return CDRegisterCommandPlugin(ID_CDADDPSRCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddPSRConst.tif","CD Add PSR Constraint",CDDataAllocator(CDAddPSRConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDPSRCONSTRAINT,name,0,"CDAddPSRConst.tif","CD Add PSR Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddPSRConstraint));
}

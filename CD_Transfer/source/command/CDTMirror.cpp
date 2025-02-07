//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class CDTMirrorDialog : public GeModalDialog
{
	private:
		CDTROptionsUA 			ua;
		
	public:	
		void DoEnable(void);
		
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDTMirrorDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDMIRTRANS));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDTR_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDTR_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_MIRROR),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_MIRROR,BFH_CENTER,2,1);
					AddChild(CDTR_MIRROR, 0, GeLoadString(CDTR_MIR_GLOBAL));
					AddChild(CDTR_MIRROR, 1, GeLoadString(CDTR_MIR_LOCAL));

				AddRadioGroup(CDTR_AXIS,BFH_CENTER,3,1);
					AddChild(CDTR_AXIS, 0, GeLoadString(CDTR_MIR_X));
					AddChild(CDTR_AXIS, 1, GeLoadString(CDTR_MIR_Y));
					AddChild(CDTR_AXIS, 2, GeLoadString(CDTR_MIR_Z));
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_TRANSFER),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,4,0,"",0);
				{
					AddCheckbox(CDTR_POS,BFH_LEFT,0,0,GeLoadString(CDTR_POS));
					AddCheckbox(CDTR_POS_X,BFH_LEFT,0,0,GeLoadString(CDTR_POS_X));
					AddCheckbox(CDTR_POS_Y,BFH_LEFT,0,0,GeLoadString(CDTR_POS_Y));
					AddCheckbox(CDTR_POS_Z,BFH_LEFT,0,0,GeLoadString(CDTR_POS_Z));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDTR_SCA,BFH_LEFT,0,0,GeLoadString(CDTR_SCA));
					AddCheckbox(CDTR_ROT,BFH_LEFT,0,0,GeLoadString(CDTR_ROT));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					GroupBorderSpace(0,8,0,0);
					AddCheckbox(CDTR_FRZ_CHILD,BFH_LEFT,0,0,GeLoadString(CDTR_FRZ_CHILD));
				}
				GroupEnd();
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_KEYS),0);
			{
				GroupBorder(BORDER_GROUP_IN);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDTR_AUTO_KEY,BFH_LEFT,0,0,GeLoadString(CDTR_AUTO_KEY));
					AddCheckbox(CDTR_KEEP_SELECTION,BFH_LEFT,0,0,GeLoadString(CDTR_KEEP_SELECTION));
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

Bool CDTMirrorDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDMIRRORTRANSCOM);
	if(!bc)
	{
		SetLong(CDTR_MIRROR,0);
		SetLong(CDTR_AXIS,0);
		
		SetBool(CDTR_POS,true);
		SetBool(CDTR_POS_X,true);
		SetBool(CDTR_POS_Y,true);
		SetBool(CDTR_POS_Z,true);
		SetBool(CDTR_SCA,true);
		SetBool(CDTR_ROT,true);
		SetBool(CDTR_FRZ_CHILD,false);
		
		SetBool(CDTR_AUTO_KEY,false);
		SetBool(CDTR_KEEP_SELECTION,false);
	}
	else
	{
		SetLong(CDTR_MIRROR,bc->GetLong(CDTR_MIRROR));
		SetLong(CDTR_AXIS,bc->GetLong(CDTR_AXIS));
		
		SetBool(CDTR_POS,bc->GetBool(CDTR_POS));
		SetBool(CDTR_POS_X,bc->GetBool(CDTR_POS_X));
		SetBool(CDTR_POS_Y,bc->GetBool(CDTR_POS_Y));
		SetBool(CDTR_POS_Z,bc->GetBool(CDTR_POS_Z));
		SetBool(CDTR_SCA,bc->GetBool(CDTR_SCA));
		SetBool(CDTR_ROT,bc->GetBool(CDTR_ROT));
		SetBool(CDTR_FRZ_CHILD,bc->GetBool(CDTR_FRZ_CHILD));
		
		SetBool(CDTR_AUTO_KEY,bc->GetBool(CDTR_AUTO_KEY));
		SetBool(CDTR_KEEP_SELECTION,bc->GetBool(CDTR_KEEP_SELECTION));
	}
		
	DoEnable();
	
	return true;
}

Bool CDTMirrorDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool pos, sca, rot, px, py, pz, sel, akey, child;
	LONG mir, axis;
	
	GetLong(CDTR_MIRROR,mir);
	GetLong(CDTR_AXIS,axis);
	
	GetBool(CDTR_POS,pos);
	GetBool(CDTR_POS_X,px);
	GetBool(CDTR_POS_Y,py);
	GetBool(CDTR_POS_Z,pz);
	GetBool(CDTR_SCA,sca);
	GetBool(CDTR_ROT,rot);
	GetBool(CDTR_FRZ_CHILD,child);
	
	GetBool(CDTR_AUTO_KEY,akey);
	GetBool(CDTR_KEEP_SELECTION,sel);

	
	BaseContainer bc;
	bc.SetLong(CDTR_MIRROR,mir);
	bc.SetLong(CDTR_AXIS,axis);
	
	bc.SetBool(CDTR_POS,pos);
	bc.SetBool(CDTR_POS_X,px);
	bc.SetBool(CDTR_POS_Y,py);
	bc.SetBool(CDTR_POS_Z,pz);
	bc.SetBool(CDTR_SCA,sca);
	bc.SetBool(CDTR_ROT,rot);
	bc.SetBool(CDTR_FRZ_CHILD,child);
	
	bc.SetBool(CDTR_AUTO_KEY,akey);
	bc.SetBool(CDTR_KEEP_SELECTION,sel);
	
	SetWorldPluginData(ID_CDMIRRORTRANSCOM,bc,false);
	
	DoEnable();
	
	return true;
}

void CDTMirrorDialog::DoEnable(void)
{
	Bool pos;
	GetBool(CDTR_POS,pos);
	if(pos)
	{
		Enable(CDTR_POS_X,true);
		Enable(CDTR_POS_Y,true);
		Enable(CDTR_POS_Z,true);
	}
	else
	{
		Enable(CDTR_POS_X,false);
		Enable(CDTR_POS_Y,false);
		Enable(CDTR_POS_Z,false);
	}
	
	BaseDocument *doc = GetActiveDocument();
	AtomArray *opSelLog = GetSelectionLog(doc);
	BaseContainer *wpData = GetWorldPluginData(ID_CDMIRRORTRANSCOM);
	if(opSelLog && wpData)
	{
		if(opSelLog->GetCount() == 2) Enable(CDTR_AUTO_KEY,true);
		else
		{
			SetBool(CDTR_AUTO_KEY,false);
			if(wpData) wpData->SetBool(CDTR_AUTO_KEY,false);
			Enable(CDTR_AUTO_KEY,false);
		}
		
		if(!wpData->GetBool(CDTR_AUTO_KEY))
		{
			SetBool(CDTR_KEEP_SELECTION,false);
			Enable(CDTR_KEEP_SELECTION,false);
		}
		else Enable(CDTR_KEEP_SELECTION,true);
	}
}

class CDTMirrorCommand : public CommandData
{
private:
	CDTMirrorDialog dlg;

public:
	Matrix	GetMirrorMatrix(Matrix opM, Matrix parM, LONG axis);
	Bool DoMirrorChildren(BaseDocument *doc, BaseObject *op, BaseObject *target, BaseContainer *data, Bool shift);
	Bool DoMirror(BaseDocument *doc, BaseContainer *data, Bool shift);

	virtual Bool Execute(BaseDocument* doc);

};

Matrix CDTMirrorCommand::GetMirrorMatrix(Matrix opM, Matrix prM, LONG axis)
{
	Matrix retM;
	CDQuaternion opQuat, targetQuat;
	Vector targetPos, opPos;
	
	opQuat.SetMatrix(MInv(prM) * opM);
	opPos = MInv(prM) * opM.off;
	switch (axis)
	{
		case 0:	
			targetPos.x = -opPos.x;
			targetPos.y = opPos.y;
			targetPos.z = opPos.z;
			targetQuat.w = -opQuat.w;
			targetQuat.v.x = -opQuat.v.x;
			targetQuat.v.y = opQuat.v.y;
			targetQuat.v.z = opQuat.v.z;
			break;
		case 1:
			targetPos.x = opPos.x;
			targetPos.y = -opPos.y;
			targetPos.z = opPos.z;
			targetQuat.w = -opQuat.w;
			targetQuat.v.x = opQuat.v.x;
			targetQuat.v.y = -opQuat.v.y;
			targetQuat.v.z = opQuat.v.z;
			break;
		case 2:
			targetPos.x = opPos.x;
			targetPos.y = opPos.y;
			targetPos.z = -opPos.z;
			targetQuat.w = -opQuat.w;
			targetQuat.v.x = opQuat.v.x;
			targetQuat.v.y = opQuat.v.y;
			targetQuat.v.z = -opQuat.v.z;
			break;
	}

	retM = targetQuat.GetMatrix();
	retM.off = targetPos;
	
	return prM * retM;
	
}

Bool CDTMirrorCommand::DoMirrorChildren(BaseDocument *doc, BaseObject *op, BaseObject *target, BaseContainer *data, Bool shift)
{
	if(!op || !target) return false;
	
	while(op && target)
	{
		Vector theScale = CDGetScale(op);
		Matrix opM = op->GetMg(), transM, prM = Matrix();

		if(data->GetLong(CDTR_MIRROR) == 1 && target->GetUp())
		{
			prM = target->GetUp()->GetMg();
		}
		transM = GetMirrorMatrix(target->GetMg(), prM, data->GetLong(CDTR_AXIS));
		transM.v1 = VNorm(transM.v1) * Len(target->GetMg().v1);
		transM.v2 = VNorm(transM.v2) * Len(target->GetMg().v2);
		transM.v3 = VNorm(transM.v3) * Len(target->GetMg().v3);

		if(data->GetBool(CDTR_POS))
		{
			if(data->GetBool(CDTR_POS_X)) opM.off.x = transM.off.x;
			if(data->GetBool(CDTR_POS_Y)) opM.off.y = transM.off.y;
			if(data->GetBool(CDTR_POS_Z)) opM.off.z = transM.off.z;
		}
		Vector gScale;
		if(data->GetBool(CDTR_ROT))
		{
			gScale = GetGlobalScale(op);
			opM.v1 = VNorm(transM.v1) * gScale.x;
			opM.v2 = VNorm(transM.v2) * gScale.y;
			opM.v3 = VNorm(transM.v3) * gScale.z;
		}
		if(data->GetBool(CDTR_SCA))
		{
			opM.v1 = VNorm(opM.v1) * Len(transM.v1);
			opM.v2 = VNorm(opM.v2) * Len(transM.v2);
			opM.v3 = VNorm(opM.v3) * Len(transM.v3);
		}
		
		CDAddUndo(doc,CD_UNDO_CHANGE,op);
		if(CDIsAxisMode(doc))
		{
			if(IsValidPointObject(op))
			{
				if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
				{
					RecalculateReference(doc,op,opM,op->GetMg());
				}
				else RecalculatePoints(op,opM,op->GetMg());
			}
			
			if(op->GetDown())
			{
				RepositionChildren(doc,op->GetDown(),opM,op->GetMg(),true);
			}
			op->SetMg(opM);
		}
		else
		{
			if(!data->GetBool(CDTR_FRZ_CHILD) && shift) RepositionChildren(doc,op->GetDown(),opM,op->GetMg(),true);
			if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
			{
				BaseObject *undoOp = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY,NULL);
				op->SetMg(opM);
				doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
			}
			else op->SetMg(opM);
			if(!data->GetBool(CDTR_SCA)) CDSetScale(op,theScale);
		}
		op->Message(MSG_UPDATE);
		
		DoMirrorChildren(doc,op->GetDown(),target->GetDown(),data,shift);
		
		op = op->GetNext();
		target = target->GetNext();
	}
	
	return true;
}

Bool CDTMirrorCommand::DoMirror(BaseDocument *doc, BaseContainer *data, Bool shift)
{
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	BaseObject *op = NULL, *target = NULL, *undoOp = NULL;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		if(opCount < 3)
		{
			op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
			if(!op) return false;
			
			if(opCount == 2 && data->GetBool(CDTR_FRZ_CHILD))
			{
				target = static_cast<BaseObject*>(opSelLog->GetIndex(1));
				if(target)
				{
					if(!IsHierarchyEqual(op->GetDown(),target->GetDown()))
					{
						if(!QuestionDialog(GeLoadString(MD_HIERARCHY_NOT_SAME))) return false;
					}
				}
			}
				
			if(opCount > 1 && !data->GetBool(CDTR_KEEP_SELECTION)) doc->SetActiveObject(NULL);
			doc->StartUndo();
			
			Vector theScale = CDGetScale(op);
			Matrix opM = op->GetMg(), transM, prM = Matrix();
			if(opCount == 2)
			{
				target = static_cast<BaseObject*>(opSelLog->GetIndex(1));
				if(target)
				{
					if(data->GetLong(CDTR_MIRROR) == 1 && target->GetUp())
					{
						prM = target->GetUp()->GetMg();
					}
					transM = GetMirrorMatrix(target->GetMg(), prM, data->GetLong(CDTR_AXIS));
					transM.v1 = VNorm(transM.v1) * Len(target->GetMg().v1);
					transM.v2 = VNorm(transM.v2) * Len(target->GetMg().v2);
					transM.v3 = VNorm(transM.v3) * Len(target->GetMg().v3);
				}
			} 
			else
			{
				if(data->GetLong(CDTR_MIRROR) == 1 && op->GetUp())
				{
					prM = op->GetUp()->GetMg();
				}
				transM = GetMirrorMatrix(op->GetMg(), prM, data->GetLong(CDTR_AXIS));
				transM.v1 = VNorm(transM.v1) * Len(opM.v1);
				transM.v2 = VNorm(transM.v2) * Len(opM.v2);
				transM.v3 = VNorm(transM.v3) * Len(opM.v3);
			}
			
			if(data->GetBool(CDTR_POS))
			{
				if(data->GetBool(CDTR_POS_X)) opM.off.x = transM.off.x;
				if(data->GetBool(CDTR_POS_Y)) opM.off.y = transM.off.y;
				if(data->GetBool(CDTR_POS_Z)) opM.off.z = transM.off.z;
			}
			Vector gScale;
			if(data->GetBool(CDTR_ROT))
			{
				gScale = GetGlobalScale(op);
				opM.v1 = VNorm(transM.v1) * gScale.x;
				opM.v2 = VNorm(transM.v2) * gScale.y;
				opM.v3 = VNorm(transM.v3) * gScale.z;
			}
			if(data->GetBool(CDTR_SCA))
			{
				opM.v1 = VNorm(opM.v1) * Len(transM.v1);
				opM.v2 = VNorm(opM.v2) * Len(transM.v2);
				opM.v3 = VNorm(opM.v3) * Len(transM.v3);
			}
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			if(CDIsAxisMode(doc))
			{
				if(IsValidPointObject(op))
				{
					if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
					{
						RecalculateReference(doc,op,opM,op->GetMg());
					}
					else RecalculatePoints(op,opM,op->GetMg());
				}
				
				if(op->GetDown()) RepositionChildren(doc,op->GetDown(),opM,op->GetMg(),true);
				op->SetMg(opM);
			}
			else
			{
				if(!data->GetBool(CDTR_FRZ_CHILD) && shift) RepositionChildren(doc,op->GetDown(),opM,op->GetMg(),true);
				if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
				{
					undoOp = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY,NULL);
					op->SetMg(opM);
					doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
				}
				else op->SetMg(opM);
				if(!data->GetBool(CDTR_SCA)) CDSetScale(op,theScale);
				
			}
			op->Message(MSG_UPDATE);

			if(data->GetBool(CDTR_FRZ_CHILD) && target) DoMirrorChildren(doc,op->GetDown(),target->GetDown(),data,shift);
			
			if(!data->GetBool(CDTR_KEEP_SELECTION)) doc->SetActiveObject(op);
			doc->EndUndo();
			
			EventAdd(EVENT_FORCEREDRAW);
		}
		else
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return false;
		}
	}

	return true;	
}

Bool CDTMirrorCommand::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	}
	Bool shift = (state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT);

	BaseContainer *wpData = GetWorldPluginData(ID_CDMIRRORTRANSCOM);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetLong(CDTR_MIRROR,0);
		bc.SetLong(CDTR_AXIS,0);
		
		bc.SetBool(CDTR_POS,true);
		bc.SetBool(CDTR_POS_X,true);
		bc.SetBool(CDTR_POS_Y,true);
		bc.SetBool(CDTR_POS_Z,true);
		bc.SetBool(CDTR_SCA,true);
		bc.SetBool(CDTR_ROT,true);
		bc.SetBool(CDTR_FRZ_CHILD,true);
		
		bc.SetBool(CDTR_AUTO_KEY,false);
		bc.SetBool(CDTR_KEEP_SELECTION,false);
		
		SetWorldPluginData(ID_CDMIRRORTRANSCOM,bc,false);
		wpData = GetWorldPluginData(ID_CDMIRRORTRANSCOM);
	}

	Bool optnOff = true;
	if(wpData->GetBool(CDTR_ROT)) optnOff = false;
	if(wpData->GetBool(CDTR_SCA)) optnOff = false;
	if(wpData->GetBool(CDTR_POS))
	{
		if(wpData->GetBool(CDTR_POS_X)) optnOff = false;
		if(wpData->GetBool(CDTR_POS_Y)) optnOff = false;
		if(wpData->GetBool(CDTR_POS_Z)) optnOff = false;
	}
	if(optnOff)
	{
		MessageDialog(GeLoadString(MD_ALL_OPTIONS_OFF));
		return false;	
	}
	
	return DoMirror(doc, wpData, shift);
}

class CDTMirrorCommandR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTMirrorCommand(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDTR_SERIAL_SIZE];
	String cdtnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = false;
	
	cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDMIRTRANS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMIRRORTRANSCOM,name,PLUGINFLAG_HIDE,"CDTMirror.tif","CD Transfer Mirror",CDDataAllocator(CDTMirrorCommandR));
	else return CDRegisterCommandPlugin(ID_CDMIRRORTRANSCOM,name,0,"CDTMirror.tif","CD Transfer Mirror"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDTMirrorCommand));
}

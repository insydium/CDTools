//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class CDTSwapDialog : public GeModalDialog
{
	private:
		CDTROptionsUA 			ua;
		
	public:	
		void DoEnable(void);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDTSwapDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDSWPTRANS));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_SWAP),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_MIRROR,BFH_LEFT,2,1);
					AddChild(CDTR_MIRROR, 0, GeLoadString(CDTR_MIR_GLOBAL));
					AddChild(CDTR_MIRROR, 1, GeLoadString(CDTR_MIR_LOCAL));

				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddCheckbox(CDTR_LOCK_M_AXIS,BFH_LEFT,0,0,GeLoadString(CDTR_LOCK_M_AXIS));
					
					AddRadioGroup(CDTR_AXIS,BFH_CENTER,3,1);
						AddChild(CDTR_AXIS, 0, GeLoadString(CDTR_POS_X));
						AddChild(CDTR_AXIS, 1, GeLoadString(CDTR_POS_Y));
						AddChild(CDTR_AXIS, 2, GeLoadString(CDTR_POS_Z));
				}
				GroupEnd();
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
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_KEYS),0);
			{
				GroupBorder(BORDER_GROUP_IN);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDTR_AUTO_KEY,BFH_LEFT,0,0,GeLoadString(CDTR_AUTO_KEY));
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

Bool CDTSwapDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDSWAPTRANSFERCOM);
	if(!bc)
	{
		SetLong(CDTR_MIRROR,0);
		SetBool(CDTR_LOCK_M_AXIS,false);
		SetLong(CDTR_AXIS,0);
		
		SetBool(CDTR_POS,true);
		SetBool(CDTR_POS_X,true);
		SetBool(CDTR_POS_Y,true);
		SetBool(CDTR_POS_Z,true);
		SetBool(CDTR_SCA,true);
		SetBool(CDTR_ROT,true);
		
		SetBool(CDTR_AUTO_KEY,false);
	}
	else
	{
		SetLong(CDTR_MIRROR,bc->GetLong(CDTR_MIRROR));
		SetBool(CDTR_LOCK_M_AXIS,bc->GetBool(CDTR_LOCK_M_AXIS));
		SetLong(CDTR_AXIS,bc->GetLong(CDTR_AXIS));
		
		SetBool(CDTR_POS,bc->GetBool(CDTR_POS));
		SetBool(CDTR_POS_X,bc->GetBool(CDTR_POS_X));
		SetBool(CDTR_POS_Y,bc->GetBool(CDTR_POS_Y));
		SetBool(CDTR_POS_Z,bc->GetBool(CDTR_POS_Z));
		SetBool(CDTR_SCA,bc->GetBool(CDTR_SCA));
		SetBool(CDTR_ROT,bc->GetBool(CDTR_ROT));
		
		SetBool(CDTR_AUTO_KEY,bc->GetBool(CDTR_AUTO_KEY));
	}
		
	DoEnable();
	
	return true;
}

Bool CDTSwapDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool lock, pos, sca, rot, px, py, pz, akey;
	LONG mir, axis;
	
	GetLong(CDTR_MIRROR,mir);
	GetBool(CDTR_LOCK_M_AXIS,lock);
	GetLong(CDTR_AXIS,axis);
	
	GetBool(CDTR_POS,pos);
	GetBool(CDTR_POS_X,px);
	GetBool(CDTR_POS_Y,py);
	GetBool(CDTR_POS_Z,pz);
	GetBool(CDTR_SCA,sca);
	GetBool(CDTR_ROT,rot);
	
	GetBool(CDTR_AUTO_KEY,akey);

	
	BaseContainer bc;
	bc.SetLong(CDTR_MIRROR,mir);
	bc.SetBool(CDTR_LOCK_M_AXIS,lock);
	bc.SetLong(CDTR_AXIS,axis);
	
	bc.SetBool(CDTR_POS,pos);
	bc.SetBool(CDTR_POS_X,px);
	bc.SetBool(CDTR_POS_Y,py);
	bc.SetBool(CDTR_POS_Z,pz);
	bc.SetBool(CDTR_SCA,sca);
	bc.SetBool(CDTR_ROT,rot);
	
	bc.SetBool(CDTR_AUTO_KEY,akey);
	
	SetWorldPluginData(ID_CDSWAPTRANSFERCOM,bc,false);
	
	DoEnable();
	
	return true;
}

void CDTSwapDialog::DoEnable(void)
{
	Bool pos, lock;
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
	
	GetBool(CDTR_LOCK_M_AXIS,lock);
	if(lock) Enable(CDTR_AXIS,true);
	else Enable(CDTR_AXIS,false);
	
	BaseDocument *doc = GetActiveDocument();
	AtomArray *opSelLog = GetSelectionLog(doc);
	BaseContainer *wpData = GetWorldPluginData(ID_CDSWAPTRANSFERCOM);
	if(opSelLog && wpData)
	{
		if(opSelLog->GetCount() == 2) Enable(CDTR_AUTO_KEY,true);
		else
		{
			SetBool(CDTR_AUTO_KEY,false);
			if(wpData) wpData->SetBool(CDTR_AUTO_KEY,false);
			Enable(CDTR_AUTO_KEY,false);
		}
	}
}

class CDTSwapCommand : public CommandData
{
	private:
		CDTSwapDialog dlg;

	public:
		Matrix GetMirrorMatrix(Matrix opM, Matrix prM, LONG axis);
		Bool DoSwap(BaseDocument *doc, BaseContainer *data, Bool shift);
		Bool SwapUserData(BaseDocument *doc, BaseObject *src, BaseObject *dst);

		virtual Bool Execute(BaseDocument* doc);

};

Bool CDTSwapCommand::SwapUserData(BaseDocument *doc, BaseObject *src, BaseObject *dst)
{
	DynamicDescription *srcDD = src->GetDynamicDescription(); if(!srcDD) return false;
	DynamicDescription *dstDD = dst->GetDynamicDescription(); if(!dstDD)return false;
	
	CDAddUndo(doc,CD_UNDO_CHANGE,src);
	CDAddUndo(doc,CD_UNDO_CHANGE,dst);
	
	void *srcHnd = srcDD->BrowseInit();
	void *dstHnd = dstDD->BrowseInit();
	DescID srcID, dstID;
	const BaseContainer *srcBC, *dstBC;
	
	Bool sameUD = true;
	while(srcDD->BrowseGetNext(srcHnd, &srcID, &srcBC) && dstDD->BrowseGetNext(dstHnd, &dstID, &dstBC))
	{
		if(srcBC->GetType(srcID[0].id) != dstBC->GetType(dstID[0].id)) sameUD = false;
	}
	srcDD->BrowseFree(srcHnd);
	dstDD->BrowseFree(dstHnd);
	
	if(!sameUD) return false;
	
	srcHnd = srcDD->BrowseInit();
	dstHnd = dstDD->BrowseInit();
	while(srcDD->BrowseGetNext(srcHnd, &srcID, &srcBC) && dstDD->BrowseGetNext(dstHnd, &dstID, &dstBC))
	{
		GeData srcData, dstData;

		CDGetParameter(src, srcID, srcData);
		CDGetParameter(dst, dstID, dstData);
		CDSetParameter(src, srcID, dstData, CD_DESCFLAGS_SET_DONTCHECKMINMAX);
		CDSetParameter(dst, dstID, srcData, CD_DESCFLAGS_SET_DONTCHECKMINMAX);
	}
	srcDD->BrowseFree(srcHnd);
	dstDD->BrowseFree(dstHnd);
	
	return true;
}

Matrix CDTSwapCommand::GetMirrorMatrix(Matrix opM, Matrix prM, LONG axis)
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

Bool CDTSwapCommand::DoSwap(BaseDocument *doc, BaseContainer *data, Bool shift)
{
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	BaseObject *op = NULL, *target = NULL, *undoOp = NULL, *undoTarg = NULL;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		if(opCount < 3)
		{
			op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
			if(!op) return false;
			target = static_cast<BaseObject*>(opSelLog->GetIndex(1));
			if(!target) return false;
						
			doc->StartUndo();
			
			Vector opScale = CDGetScale(op), trgScale = CDGetScale(target);
			Matrix opM = op->GetMg(), targM = target->GetMg(), aTransM, bTransM, aPrM = Matrix(), bPrM = Matrix();
			if(data->GetLong(CDTR_MIRROR) == 1 && target->GetUp())
			{
				aPrM = target->GetUp()->GetMg();
			}
			if(data->GetLong(CDTR_MIRROR) == 1 && op->GetUp())
			{
				bPrM = op->GetUp()->GetMg();
			}
			aTransM = MInv(aPrM) * targM;
			bTransM = MInv(bPrM) * opM;
			
			if(data->GetBool(CDTR_POS))
			{
				Vector opPos = opM.off, trgPos = targM.off;
				if(data->GetBool(CDTR_POS_X))
				{
					Real xfactor = 1;
					if(data->GetBool(CDTR_LOCK_M_AXIS) && data->GetLong(CDTR_AXIS) == 0) xfactor = -1;
					
					opPos.x = aTransM.off.x * xfactor;
					trgPos.x = bTransM.off.x * xfactor;
				}
				if(data->GetBool(CDTR_POS_Y))
				{
					Real yfactor = 1;
					if(data->GetBool(CDTR_LOCK_M_AXIS) && data->GetLong(CDTR_AXIS) == 1) yfactor = -1;
					
					opPos.y = aTransM.off.y * yfactor;
					trgPos.y = bTransM.off.y * yfactor;
				}
				if(data->GetBool(CDTR_POS_Z))
				{
					Real zfactor = 1;
					if(data->GetBool(CDTR_LOCK_M_AXIS) && data->GetLong(CDTR_AXIS) == 2) zfactor = -1;
					
					opPos.z = aTransM.off.z * zfactor;
					trgPos.z = bTransM.off.z * zfactor;
				}
				opM.off = bPrM * opPos;
				targM.off = aPrM * trgPos;
			}
			Vector gScale;
			if(data->GetBool(CDTR_ROT))
			{
				if(data->GetBool(CDTR_LOCK_M_AXIS))
				{
					Matrix aMirM = GetMirrorMatrix(aTransM, aPrM, data->GetLong(CDTR_AXIS));
					opM.v1 = VNorm(aMirM.v1);
					opM.v2 = VNorm(aMirM.v2);
					opM.v3 = VNorm(aMirM.v3);
					
					Matrix bMirM = GetMirrorMatrix(bTransM, bPrM, data->GetLong(CDTR_AXIS));
					targM.v1 = VNorm(bMirM.v1);
					targM.v2 = VNorm(bMirM.v2);
					targM.v3 = VNorm(bMirM.v3);
				}
				else
				{
					opM.v1 = VNorm(aTransM.v1);
					opM.v2 = VNorm(aTransM.v2);
					opM.v3 = VNorm(aTransM.v3);
					
					targM.v1 = VNorm(bTransM.v1);
					targM.v2 = VNorm(bTransM.v2);
					targM.v3 = VNorm(bTransM.v3);
				}
			}
			if(data->GetBool(CDTR_SCA))
			{
				opM.v1 = VNorm(opM.v1) * Len(aTransM.v1);
				opM.v2 = VNorm(opM.v2) * Len(aTransM.v2);
				opM.v3 = VNorm(opM.v3) * Len(aTransM.v3);
				
				targM.v1 = VNorm(targM.v1) * Len(bTransM.v1);
				targM.v2 = VNorm(targM.v2) * Len(bTransM.v2);
				targM.v3 = VNorm(targM.v3) * Len(bTransM.v3);
			}
			
			if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
			{
				undoOp = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY,NULL);
				undoTarg = (BaseObject*)CDGetClone(target,CD_COPYFLAGS_NO_HIERARCHY,NULL);
			}
			SwapUserData(doc, op, target);
			
			if(!IsParentObject(op,target))
			{
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
					if(shift) RepositionChildren(doc,op->GetDown(),opM,op->GetMg(),true);
					if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
					{
						op->SetMg(opM);
						doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
					}
					else op->SetMg(opM);
				}
				if(!data->GetBool(CDTR_SCA)) CDSetScale(op,opScale);
				
				CDAddUndo(doc,CD_UNDO_CHANGE,target);
				if(CDIsAxisMode(doc))
				{
					if(IsValidPointObject(op))
					{
						if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
						{
							RecalculateReference(doc,op,targM,target->GetMg());
						}
						else RecalculatePoints(target,targM,target->GetMg());
					}
					
					if(target->GetDown()) RepositionChildren(doc,target->GetDown(),targM,target->GetMg(),true);
					target->SetMg(targM);
				}
				else
				{
					if(shift) RepositionChildren(doc,target->GetDown(),targM,target->GetMg(),true);
					if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
					{
						target->SetMg(targM);
						doc->AutoKey(undoTarg,target,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
					}
					else target->SetMg(targM);
				}
				if(!data->GetBool(CDTR_SCA)) CDSetScale(target,trgScale);
			}
			else
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,target);
				if(CDIsAxisMode(doc))
				{
					if(IsValidPointObject(op))
					{
						if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
						{
							RecalculateReference(doc,op,targM,target->GetMg());
						}
						else RecalculatePoints(target,targM,target->GetMg());
					}
					
					if(target->GetDown()) RepositionChildren(doc,target->GetDown(),targM,target->GetMg(),true);
					target->SetMg(targM);
				}
				else
				{
					if(shift) RepositionChildren(doc,target->GetDown(),targM,target->GetMg(),true);
					if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
					{
						target->SetMg(targM);
						doc->AutoKey(undoTarg,target,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
					}
					else target->SetMg(targM);
				}
				if(!data->GetBool(CDTR_SCA)) CDSetScale(target,trgScale);
				
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
					if(shift) RepositionChildren(doc,op->GetDown(),opM,op->GetMg(),true);
					if(data->GetBool(CDTR_AUTO_KEY) && IsCommandEnabled(12425) && CDIsCommandChecked(12425))
					{
						op->SetMg(opM);
						doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
					}
					else op->SetMg(opM);
				}
				if(!data->GetBool(CDTR_SCA)) CDSetScale(op,opScale);
			}

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

Bool CDTSwapCommand::Execute(BaseDocument* doc)
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

	BaseContainer *wpData = GetWorldPluginData(ID_CDSWAPTRANSFERCOM);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetLong(CDTR_MIRROR,0);
		bc.SetBool(CDTR_LOCK_M_AXIS,false);
		bc.SetLong(CDTR_AXIS,0);
		
		bc.SetBool(CDTR_POS,true);
		bc.SetBool(CDTR_POS_X,true);
		bc.SetBool(CDTR_POS_Y,true);
		bc.SetBool(CDTR_POS_Z,true);
		bc.SetBool(CDTR_SCA,true);
		bc.SetBool(CDTR_ROT,true);
		
		bc.SetBool(CDTR_AUTO_KEY,false);
		
		SetWorldPluginData(ID_CDSWAPTRANSFERCOM,bc,false);
		wpData = GetWorldPluginData(ID_CDSWAPTRANSFERCOM);
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
	
	return DoSwap(doc, wpData, shift);
}

class CDTSwapCommandR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTSwapCommand(void)
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
	String name=GeLoadString(IDS_CDSWPTRANS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDSWAPTRANSFERCOM,name,PLUGINFLAG_HIDE,"CDTSwap.tif","CD Transfer Swap",CDDataAllocator(CDTSwapCommandR));
	else return CDRegisterCommandPlugin(ID_CDSWAPTRANSFERCOM,name,0,"CDTSwap.tif","CD Transfer Swap"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDTSwapCommand));
}

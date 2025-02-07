//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	

#include "c4d.h"
#include "c4d_symbols.h"

//#include "CDTransfer.h"
#include "CDFBX.h"
//#include "CDDebug.h"

// CD Aim Constraint
enum
{
	AC_POLE_AXIS		= 1015,
		AC_POLE_X		= 1016,
		AC_POLE_Y		= 1017,
		AC_POLE_NX		= 1018,
		AC_POLE_NY		= 1019
};

enum
{
	DT_POLE_AXIS				= 10010,
		DT_POLE_Y				= 10011,
		DT_POLE_X				= 10012,
		DT_POLE_NX				= 10022,
		DT_POLE_NY				= 10023
	
};



class CDTAxisDialog : public GeModalDialog
{
private:
		CDFbxOptionsUA 			ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDTAxisDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDTRANSAXIS));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_AXES),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddCheckbox(CDTR_FRZ_CHILD,BFH_LEFT,0,0,GeLoadString(CDTR_FRZ_CHILD));
					AddCheckbox(CDTR_FRZ_PRIM,BFH_LEFT,0,0,GeLoadString(CDTR_FRZ_PRIM));
				}
				GroupEnd();
			
				GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_AIM),0);
				{
					AddRadioGroup(CDTR_AXIS,BFH_LEFT,1,0);
						AddChild(CDTR_AXIS, 0, GeLoadString(CDTR_ZXY));
						AddChild(CDTR_AXIS, 1, GeLoadString(CDTR_YZX));
						AddChild(CDTR_AXIS, 2, GeLoadString(CDTR_YXZ));
						AddChild(CDTR_AXIS, 3, GeLoadString(CDTR_XYZ));
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

Bool CDTAxisDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDTRANSFERAXIS);
	if(!bc)
	{
		SetBool(CDTR_FRZ_CHILD,false);
		SetBool(CDTR_FRZ_PRIM,false);
		SetLong(CDTR_AXIS,1);
	}
	else
	{
		SetBool(CDTR_FRZ_CHILD,bc->GetBool(CDTR_FRZ_CHILD));
		SetBool(CDTR_FRZ_PRIM,bc->GetBool(CDTR_FRZ_PRIM));
		SetLong(CDTR_AXIS,bc->GetLong(CDTR_AXIS));
	}
	
	return true;
}

Bool CDTAxisDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool prim, child;
	LONG axis;
	
	GetBool(CDTR_FRZ_CHILD,child);
	GetBool(CDTR_FRZ_PRIM,prim);
	GetLong(CDTR_AXIS,axis);

	BaseContainer bc;
	bc.SetBool(CDTR_FRZ_CHILD,child);
	bc.SetBool(CDTR_FRZ_PRIM,prim);
	bc.SetLong(CDTR_AXIS,axis);
	SetWorldPluginData(ID_CDTRANSFERAXIS,bc,false);
	
	return true;
}

class CDTransAxisCommand : public CommandData
{
	private:
		CDTAxisDialog		dlg;
		
		Matrix GetRotatedMatrix(Vector aR, Vector bR, Real mix);
		void TransferChildPositionTracks(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM);
		void TransferRotationTracks(BaseDocument *doc, BaseObject *op, Matrix rotM, LONG axis);
		void TransferChildRotationTracks(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM, LONG axis);
		Bool RecalculateChildTracks(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM, LONG axis);
		
		void CheckUpVectorConstraint(BaseDocument *doc, BaseObject *op, LONG axis);

		Bool DoAxisAlign(BaseDocument *doc, BaseContainer *data, AtomArray *objects, LONG oCnt);
		Bool DoAxisAlignChildren(BaseDocument *doc, BaseObject *op, BaseContainer *data);
		
	public:
		virtual Bool Execute(BaseDocument* doc);

};

void CDTransAxisCommand::CheckUpVectorConstraint(BaseDocument *doc, BaseObject *op, LONG axis)
{
	if(!doc || !op) return;
	
	BaseTag *acTag = op->GetTag(ID_CDACONSTRAINTPLUGIN);
	BaseTag *dtTag = op->GetTag(ID_CDDUALTARGETPLUGIN);
	
	if(acTag)
	{
		BaseContainer *acData = acTag->GetDataInstance();
		if(acData)
		{
			switch(axis)
			{
				case 2:
				{
					switch(acData->GetLong(AC_POLE_AXIS))
					{
						case AC_POLE_X:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_Y);
							break;
						case AC_POLE_Y:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_NX);
							break;
						case AC_POLE_NX:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_NY);
							break;
						case AC_POLE_NY:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_X);
							break;
					}
					break;
				}
				case 3:
				{
					switch(acData->GetLong(AC_POLE_AXIS))
					{
						case AC_POLE_X:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_NY);
							break;
						case AC_POLE_Y:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_X);
							break;
						case AC_POLE_NX:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_Y);
							break;
						case AC_POLE_NY:
							acData->SetLong(AC_POLE_AXIS,AC_POLE_NX);
							break;
					}
					break;
				}
			}
		}
	}
	if(dtTag)
	{
		BaseContainer *dtData = dtTag->GetDataInstance();
		if(dtData)
		{
			switch(axis)
			{
				case 2:
				{
					switch(dtData->GetLong(DT_POLE_AXIS))
					{
						case DT_POLE_X:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_Y);
							break;
						case DT_POLE_Y:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_NX);
							break;
						case DT_POLE_NX:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_NY);
							break;
						case DT_POLE_NY:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_X);
							break;
					}
					break;
				}
				case 3:
				{
					switch(dtData->GetLong(DT_POLE_AXIS))
					{
						case DT_POLE_X:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_NY);
							break;
						case DT_POLE_Y:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_X);
							break;
						case DT_POLE_NX:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_Y);
							break;
						case DT_POLE_NY:
							dtData->SetLong(DT_POLE_AXIS,DT_POLE_NX);
							break;
					}
					break;
				}
			}
		}
	}
}

Matrix CDTransAxisCommand::GetRotatedMatrix(Vector aR, Vector bR, Real mix)
{
	Matrix m;
	
	CDQuaternion aQ, bQ, mixQ, q25, q50, q75;
	aQ.SetHPB(aR);
	bQ.SetHPB(bR);
	q50 = !(aQ + bQ);
	q25 = !(q50 + aQ);
	q75 = !(q50 + bQ);
	mixQ = CDQSlerpBezier(aQ,q25,q50,q75,bQ,mix);
	m = mixQ.GetMatrix();
	
	return m;
}

void CDTransAxisCommand::TransferRotationTracks(BaseDocument *doc, BaseObject *op, Matrix rotM, LONG axis)
{
	BaseObject *cln = BaseObject::Alloc(Onull);
	if(cln && op)
	{
		LONG fps = doc->GetFps();
		
		// store the current frame
		LONG curFrame = doc->GetTime().GetFrame(fps);
		BaseTime curTime = BaseTime(curFrame,fps);
		
		LONG frm = CDGetFirstVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_ROTATION);
		BaseTime time = BaseTime(frm,fps);
		CDAnimateObject(doc, op, time, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
		Matrix opM = op->GetMl();
	
		Matrix clnM = opM * rotM;
		cln->SetMl(clnM);
		CDSetVectorKeys(doc,cln,time,CD_ID_BASEOBJECT_ROTATION);
		
		LONG prvFrm = frm;
		frm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION);
		
		StatusSetText("Transposing Object Tracks");
		Matrix prvM = opM;
		while(frm < CDMAXLONG)
		{
			StatusSetSpin();
			
			Vector prvOpRot = CDGetRot(op);
			Vector prvClnRot = CDGetRot(cln);
			
			time = BaseTime(frm,fps);
			CDAnimateObject(doc, op, time, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
			opM = op->GetMl();
			Vector nxtOpRot = CDGetRot(op);
			
			clnM = opM * rotM;
			cln->SetMl(clnM);
			
			Vector rot, newRot, clnRot = CDGetRot(cln);
			
			Matrix nxtM = opM;
			LONG strtFrm = prvFrm;
			Vector oldRot = prvClnRot;
			
			prvFrm++;
			while(prvFrm < frm)
			{
				StatusSetSpin();
				
				Real mix = Real(prvFrm-strtFrm)/Real(frm-strtFrm);
				
				BaseTime t = BaseTime(prvFrm,fps);
				CDAnimateObject(doc, op, t, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);

				opM = op->GetMl();
				Matrix newM = GetRotatedMatrix(prvOpRot,nxtOpRot,mix);
				
				rot = CDGetOptimalAngle(oldRot, CDMatrixToHPB(newM * rotM, op), op);
				oldRot = rot;
				
				prvFrm++;
			}
			CDSetRot(cln, CDGetOptimalAngle(oldRot, clnRot, cln));
			CDSetVectorKeys(doc,cln,time,CD_ID_BASEOBJECT_ROTATION);
			
			prvFrm = frm;
			prvM = nxtM;
			frm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION);
		}
		
		// transfer track keys
		BaseList2D *prvTrk = NULL;
		
		TransferVectorTracks(doc,cln,op,prvTrk,CD_ID_BASEOBJECT_ROTATION);
		CDAnimateObject(doc, op, curTime, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
		
		BaseObject::Free(cln);
	}
}

void CDTransAxisCommand::TransferChildRotationTracks(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM, LONG axis)
{
	BaseObject *cln = BaseObject::Alloc(Onull);
	if(cln && op)
	{
		LONG fps = doc->GetFps();
		
		// store the current frame
		LONG curFrame = doc->GetTime().GetFrame(fps);
		BaseTime curTime = BaseTime(curFrame,fps);
		
		LONG frm = CDGetFirstVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_ROTATION);
		BaseTime time = BaseTime(frm,fps);
		CDAnimateObject(doc, op, time, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
	
		Matrix opM = op->GetMg();
		Matrix clnM = MInv(newM) * (oldM * (MInv(newM) * opM));
		cln->SetMl(clnM);
		CDSetVectorKeys(doc,cln,time,CD_ID_BASEOBJECT_ROTATION);
		
		LONG prvFrm = frm;
		frm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION);
			
		StatusSetText("Transposing Child Tracks");
		Matrix prvM = opM;
		while(frm < CDMAXLONG)
		{
			StatusSetSpin();
			
			Vector prvOpRot = CDGetRot(op);
			Vector prvClnRot = CDGetRot(cln);
			
			time = BaseTime(frm,fps);
			CDAnimateObject(doc, op, time, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
			opM = op->GetMg();
			Vector nxtOpRot = CDGetRot(op);
			
			clnM = MInv(newM) * (oldM * (MInv(newM) * opM));
			cln->SetMl(clnM);
			
			Matrix rotM = MInv(opM) * clnM;
			rotM.off = Vector(0,0,0);
			
			Vector rot, newRot, clnRot = CDGetRot(cln);
			
			Matrix nxtM = opM;
			Vector oldRot = prvClnRot;
			
			prvFrm++;
			while(prvFrm < frm)
			{
				StatusSetSpin();
				
				BaseTime t = BaseTime(prvFrm,fps);
				CDAnimateObject(doc, op, t, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
				opM = op->GetMg();
				
				clnM = MInv(newM) * (oldM * (MInv(newM) * opM));
				cln->SetMl(clnM);

				rot = CDGetOptimalAngle(oldRot,CDMatrixToHPB(clnM,cln),cln);
				oldRot = rot;
				
				prvFrm++;
			}
			CDSetRot(cln,CDGetOptimalAngle(oldRot,clnRot,cln));
			CDSetVectorKeys(doc,cln,time,CD_ID_BASEOBJECT_ROTATION);
			
			prvFrm = frm;
			frm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION);
		}
		
		// transfer track keys
		BaseList2D *prvTrk = NULL;
		
		TransferVectorTracks(doc,cln,op,prvTrk,CD_ID_BASEOBJECT_ROTATION);
		CDAnimateObject(doc, op, curTime, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
		
		BaseObject::Free(cln);
	}
}

void CDTransAxisCommand::TransferChildPositionTracks(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM)
{
	BaseObject *cln = BaseObject::Alloc(Onull);
	if(cln && op)
	{
		LONG fps = doc->GetFps();
		
		// store the current frame
		LONG curFrame = doc->GetTime().GetFrame(fps);
		BaseTime curTime = BaseTime(curFrame,fps);
		
		LONG frm = CDGetFirstVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_ROTATION);
		BaseTime time = BaseTime(frm,fps);
		CDAnimateObject(doc, op, time, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
	
		Matrix opM = op->GetMg();
		Matrix clnM = MInv(newM) * (oldM * (MInv(newM) * opM));
		CDSetPos(cln,clnM.off);
		CDSetVectorKeys(doc,cln,time,CD_ID_BASEOBJECT_POSITION);
		
		frm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION);
			
		StatusSetText("Transposing Child Tracks");
		while(frm < CDMAXLONG)
		{
			StatusSetSpin();
			
			time = BaseTime(frm,fps);
			CDAnimateObject(doc, op, time, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
			opM = op->GetMg();
			
			clnM = MInv(newM) * (oldM * (MInv(newM) * opM));
			CDSetPos(cln,clnM.off);
			CDSetVectorKeys(doc,cln,time,CD_ID_BASEOBJECT_POSITION);
			
			frm = CDGetNextVectorKeyFrame(op,frm,fps,CD_ID_BASEOBJECT_ROTATION);
		}
		
		// transfer track keys
		BaseList2D *prvTrk = NULL;
		
		TransferVectorTracks(doc,cln,op,prvTrk,CD_ID_BASEOBJECT_POSITION);
		CDAnimateObject(doc, op, curTime, CD_ANIMATEFLAGS_NO_PARTICLES|CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
		
		BaseObject::Free(cln);
	}
}

Bool CDTransAxisCommand::RecalculateChildTracks(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM, LONG axis)
{
	while(op)
	{
		CDAddUndo(doc,CD_UNDO_CHANGE,op);
		Matrix transM = oldM * MInv(newM) * op->GetMg();
		
		if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_POSITION)) TransferChildPositionTracks(doc,op,newM,oldM);
		else op->SetMg(transM);
		
		if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION)) TransferChildRotationTracks(doc,op,newM,oldM,axis);
		
		op = op->GetNext();
	}
	
	return true;
}

Bool CDTransAxisCommand::DoAxisAlignChildren(BaseDocument *doc, BaseObject *op, BaseContainer *data)
{
	BaseObject *nextOp = NULL, *opR = NULL;
	Matrix oldM, newM, rotM = Matrix();
	
	LONG axis = data->GetLong(CDTR_AXIS);
	switch(axis)
	{
		case 0:
			rotM = CDHPBToMatrix(Vector(-pi05,0,-pi05));
			break;
		case 1:
			rotM = CDHPBToMatrix(Vector(0,pi05,pi05));
			break;
		case 2:
			rotM = CDHPBToMatrix(Vector(0,0,pi05));
			break;
		case 3:
			rotM = CDHPBToMatrix(Vector(0,0,-pi05));
			break;
	}
	
	while(op)
	{
		oldM = op->GetMg();
		newM = oldM * rotM;
		
		nextOp = op->GetNext();
		if(IsValidPointObject(op))
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
			{
				RecalculateReference(doc,op,newM,oldM);
			}
			else RecalculatePoints(op,newM,oldM);
			
			op->SetMg(newM);
			op->Message(MSG_UPDATE);
			
			if(op->GetDown())
			{
				RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
				DoAxisAlignChildren(doc,op->GetDown(),data);	
			}
			opR = op;
		}
		else
		{
			if(data->GetBool(CDTR_FRZ_PRIM) && (op->GetInfo()&OBJECT_GENERATOR) && !(op->GetInfo()&OBJECT_INPUT))
			{
				RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
				
				ModelingCommandData mcd;
				mcd.doc = doc;
			#if API_VERSION < 12000
				mcd.flags = MODELINGCOMMANDFLAG_CREATEUNDO;
			#else
				mcd.flags = MODELINGCOMMANDFLAGS_CREATEUNDO;
			#endif
				mcd.op = op;

				if(!SendModelingCommand(MCOMMAND_MAKEEDITABLE, mcd)) return false;
				BaseObject *newOp = static_cast<BaseObject*>(mcd.result->GetIndex(0));
				
				CDAddUndo(doc,CD_UNDO_CHANGE,newOp);
				RecalculatePoints(newOp,newM,oldM);
				newOp->SetMg(newM);
				newOp->Message(MSG_UPDATE);
				
				if(newOp->GetDown())
				{
					RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
					DoAxisAlignChildren(doc,newOp->GetDown(),data);	
				}
				opR = newOp;
			}
			else
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				op->SetMg(newM);
				op->Message(MSG_UPDATE);
				
				if(op->GetDown())
				{
					RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
					DoAxisAlignChildren(doc,op->GetDown(),data);	
				}
				opR = op;
			}
		}
		if(CDHasVectorTrack(opR,CD_ID_BASEOBJECT_ROTATION)) TransferRotationTracks(doc,opR,rotM,axis);
		if(axis > 1) CheckUpVectorConstraint(doc,opR,axis);
		
		op = nextOp;
	}
	
	return true;
}

Bool CDTransAxisCommand::DoAxisAlign(BaseDocument *doc, BaseContainer *data, AtomArray *objects, LONG oCnt)
{
	BaseObject *op = NULL, *opR = NULL;
	LONG i;
	Matrix oldM, newM, rotM = Matrix();
	
	if(oCnt > 0)
	{
		LONG axis = data->GetLong(CDTR_AXIS);
		switch(axis)
		{
			case 0:
				rotM = CDHPBToMatrix(Vector(-pi05,0,-pi05));
				break;
			case 1:
				rotM = CDHPBToMatrix(Vector(0,pi05,pi05));
				break;
			case 2:
				rotM = CDHPBToMatrix(Vector(0,0,pi05));
				break;
			case 3:
				rotM = CDHPBToMatrix(Vector(0,0,-pi05));
				break;
		}
		
		doc->StartUndo();
		for(i=0; i<oCnt; i++)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				oldM = op->GetMg();
				newM = oldM * rotM;
				
				if(IsValidPointObject(op))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
					{
						RecalculateReference(doc,op,newM,oldM);
					}
					else RecalculatePoints(op,newM,oldM);
					
					op->SetMg(newM);
					op->Message(MSG_UPDATE);
					
					if(op->GetDown())
					{
						RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
						if(data->GetBool(CDTR_FRZ_CHILD)) DoAxisAlignChildren(doc,op->GetDown(),data);	
					}
					opR = op;
				}
				else
				{
					if(data->GetBool(CDTR_FRZ_PRIM) && (op->GetInfo()&OBJECT_GENERATOR) && !(op->GetInfo()&OBJECT_INPUT))
					{					
						RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
						
						ModelingCommandData mcd;
						mcd.doc = doc;
					#if API_VERSION < 12000
						mcd.flags = MODELINGCOMMANDFLAG_CREATEUNDO;
					#else
						mcd.flags = MODELINGCOMMANDFLAGS_CREATEUNDO;
					#endif
						mcd.op = op;

						if(!SendModelingCommand(MCOMMAND_MAKEEDITABLE, mcd)) return false;
						BaseObject *newOp = static_cast<BaseObject*>(mcd.result->GetIndex(0));
						
						CDAddUndo(doc,CD_UNDO_CHANGE,newOp);
						RecalculatePoints(newOp,newM,oldM);
						newOp->SetMg(newM);
						newOp->Message(MSG_UPDATE);
						
						if(newOp->GetDown())
						{
							RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
							if(data->GetBool(CDTR_FRZ_CHILD)) DoAxisAlignChildren(doc,newOp->GetDown(),data);	
						}
						opR = newOp;
					}
					else
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						op->SetMg(newM);
						op->Message(MSG_UPDATE);
						
						if(op->GetDown())
						{
							RecalculateChildTracks(doc,op->GetDown(),newM,oldM,axis);
							if(data->GetBool(CDTR_FRZ_CHILD)) DoAxisAlignChildren(doc,op->GetDown(),data);	
						}
						opR = op;
					}
				}
				if(CDHasVectorTrack(opR,CD_ID_BASEOBJECT_ROTATION)) TransferRotationTracks(doc,opR,rotM,axis);
				if(axis > 1) CheckUpVectorConstraint(doc,opR,axis);
			}
		}
		doc->EndUndo();
		
		StatusClear();
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
	
}

Bool CDTransAxisCommand::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	LONG oCnt = objects->GetCount();
	if(oCnt < 1) return false;
	
	if(!dlg.Open()) return false;

	BaseContainer *wpData = GetWorldPluginData(ID_CDTRANSFERAXIS);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetBool(CDTR_FRZ_CHILD,false);
		bc.SetBool(CDTR_FRZ_PRIM,false);
		bc.SetLong(CDTR_AXIS,1);
		SetWorldPluginData(ID_CDTRANSFERAXIS,bc,false);
		wpData = GetWorldPluginData(ID_CDTRANSFERAXIS);
	}
	
	return DoAxisAlign(doc, wpData,objects,oCnt);
}

class CDTAimCommandR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTransAxisCommand(void)
{
	if(CDFindPlugin(1022171,CD_COMMAND_PLUGIN) && CDFindPlugin(ID_CDTRANSFERAXIS,CD_COMMAND_PLUGIN)) return true;

	Bool reg = true;
		
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDFBX_SERIAL_SIZE];
	String cdfnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDFBXPLUGIN,data,CDFBX_SERIAL_SIZE)) reg = false;
	
	cdfnr.SetCString(data,CDFBX_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdfnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdfnr.FindFirst("-",&pos);
	while(h)
	{
		cdfnr.Delete(pos,1);
		h = cdfnr.FindFirst("-",&pos);
	}
	cdfnr.ToUpper();
	kb = cdfnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDTRANSAXIS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDTRANSFERAXIS,name,PLUGINFLAG_HIDE,"CDTransAxes.tif","CD Transfer Axes",CDDataAllocator(CDTAimCommandR));
	else return CDRegisterCommandPlugin(ID_CDTRANSFERAXIS,name,0,"CDTransAxes.tif","CD Transfer Axes"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDTransAxisCommand));
}

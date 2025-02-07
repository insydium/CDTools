//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "c4d_graphview.h"
#include "c4d_operatorplugin.h"
#include "c4d_operatordata.h"
#include "gvconst.h"
#include "gvcondition.h"
#include "gvdynamic.h"
#include "gvobject.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
//#include "CDDebug.h"

#include "oCDJoint.h"

class CDFreezeTranformationDialog : public GeModalDialog
{
	private:
		CDJSOptionsUA ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDFreezeTranformationDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDFTRANS));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDJS_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDJS_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDJS_FREEZE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,2,0,"",0);
				{
					AddCheckbox(CDJS_FRZ_CHILD,BFH_LEFT,0,0,GeLoadString(CDJS_FRZ_CHILD));
					AddCheckbox(CDJS_FRZ_PRIM,BFH_LEFT,0,0,GeLoadString(CDJS_FRZ_PRIM));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(CDJS_POS,BFH_LEFT,0,0,GeLoadString(CDJS_POS));
					AddCheckbox(CDJS_SCA,BFH_LEFT,0,0,GeLoadString(CDJS_SCA));
					AddCheckbox(CDJS_ROT,BFH_LEFT,0,0,GeLoadString(CDJS_ROT));
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

Bool CDFreezeTranformationDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDFREEZETRANSFORMATION);
	if(!bc)
	{
		SetBool(CDJS_FRZ_CHILD,false);
		SetBool(CDJS_FRZ_PRIM,false);
		SetBool(CDJS_POS,false);
		SetBool(CDJS_SCA,true);
		SetBool(CDJS_ROT,true);
	}
	else
	{
		SetBool(CDJS_FRZ_CHILD,bc->GetBool(CDJS_FRZ_CHILD));
		SetBool(CDJS_FRZ_PRIM,bc->GetBool(CDJS_FRZ_PRIM));
		SetBool(CDJS_POS,bc->GetBool(CDJS_POS));
		SetBool(CDJS_SCA,bc->GetBool(CDJS_SCA));
		SetBool(CDJS_ROT,bc->GetBool(CDJS_ROT));
	}
		
	return true;
}

Bool CDFreezeTranformationDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool prim, child, pos, sca, rot; 
	
	GetBool(CDJS_FRZ_CHILD,child);
	GetBool(CDJS_FRZ_PRIM,prim);
	GetBool(CDJS_POS,pos);
	GetBool(CDJS_SCA,sca);
	GetBool(CDJS_ROT,rot);
	
	BaseContainer bc;
	bc.SetBool(CDJS_FRZ_CHILD,child);
	bc.SetBool(CDJS_FRZ_PRIM,prim);
	bc.SetBool(CDJS_POS,pos);
	bc.SetBool(CDJS_SCA,sca);
	bc.SetBool(CDJS_ROT,rot);
	SetWorldPluginData(ID_CDFREEZETRANSFORMATION,bc,false);
	
	return true;
}

class CDFreezeTransformation : public CommandData
{
	private:
		CDFreezeTranformationDialog dlg;

	public:
		void FreezePositionTrack(BaseDocument *doc, BaseList2D *op, Vector sca);
		void FreezeAttributes(BaseList2D *lst, Vector sca);
		void FreezeTagAttributes(BaseObject *op, BaseContainer *data, Vector newSca, Vector oldSca);
		
		void FreezeXpressoPositionPorts(BaseTag *tag, BaseObject *op, Vector sca);
		void ScaleIncomingConnection(GvPort *inport, Vector sca);
		void ScaleIncomingConnection(GvPort *inport, Real sca);
		
		Matrix GetFreezeMatrix(BaseObject *op, BaseContainer *data);
		Bool DoFreezeChildren(BaseDocument *doc, BaseObject *op, BaseContainer *data);
		Bool DoFreezeTransformation(BaseDocument *doc, BaseContainer *data);
		
		virtual Bool Execute(BaseDocument* doc);

};

static Bool IsJoint(BaseObject *op)
{
	Bool jnt = false;
	
	if(op->GetType() == ID_CDJOINTOBJECT) jnt = true;
	if(op->GetType() == 5123) jnt = true;
	if(op->GetType() == 1019362) jnt = true;
	
	return jnt;
}

void CDFreezeTransformation::FreezePositionTrack(BaseDocument *doc, BaseList2D *op, Vector sca)
{
	CDScaleVectorTrack(doc,op,sca,CD_ID_BASEOBJECT_POSITION);
}

void CDFreezeTransformation::ScaleIncomingConnection(GvPort *inport, Vector sca)
{
	GvNode *srcNode = NULL;
	GvPort *srcPort = NULL;
	
	inport->GetIncomingSource(srcNode,srcPort);
	if(srcNode && srcPort)
	{
		BaseContainer *srcData = srcNode->GetOpContainerInstance();
		if(srcData)
		{
			if(srcData->GetLong(GV_DYNAMIC_DATATYPE) == DA_VECTOR)
			{
				Bool animated = false;
				DescID dscID;
				switch(srcNode->GetOperatorID())
				{
					case ID_OPERATOR_CONST:
					{
						dscID = DescID(DescLevel(GV_CONST_VALUE,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
						if(CDHasAnimationTrack(srcNode,dscID))
						{
							CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca.x);
							animated = true;
						}
						dscID = DescID(DescLevel(GV_CONST_VALUE,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
						if(CDHasAnimationTrack(srcNode,dscID))
						{
							CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca.y);
							animated = true;
						}
						dscID = DescID(DescLevel(GV_CONST_VALUE,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
						if(CDHasAnimationTrack(srcNode,dscID))
						{
							CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca.z);
							animated = true;
						}
						
						if(!animated)
						{
							Vector p = srcData->GetVector(GV_CONST_VALUE);
							p.x *= sca.x;
							p.y *= sca.y;
							p.z *= sca.z;
							srcData->SetVector(GV_CONST_VALUE,p);
						}
						break;
					}
					case ID_OPERATOR_CONDITION:
					{
						BaseContainer *subBc = srcData->GetContainerInstance(GV_CONDITION_INPUT);
						
						LONG i, cnt = srcNode->GetInPortCount();
						for(i=1; i<cnt; i++)
						{
							GvPort *input = srcNode->GetInPort(i);
							if(input)
							{
								if(!input->IsIncomingConnected())
								{
									dscID = DescID(DescLevel(GV_CONDITION_INPUT),DescLevel(input->GetSubID(),DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
									if(CDHasAnimationTrack(srcNode,dscID))
									{
										CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca.x);
										animated = true;
									}
									dscID = DescID(DescLevel(GV_CONDITION_INPUT),DescLevel(input->GetSubID(),DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
									if(CDHasAnimationTrack(srcNode,dscID))
									{
										CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca.y);
										animated = true;
									}
									dscID = DescID(DescLevel(GV_CONDITION_INPUT),DescLevel(input->GetSubID(),DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
									if(CDHasAnimationTrack(srcNode,dscID))
									{
										CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca.z);
										animated = true;
									}
									
									if(!animated && subBc)
									{
										Vector p = subBc->GetVector(input->GetSubID());
										p.x *= sca.x;
										p.y *= sca.y;
										p.z *= sca.z;
										subBc->SetVector(input->GetSubID(),p);
									}
								}
								else ScaleIncomingConnection(input,sca);
							}
						}
						break;
					}
				}
			}
		}
	}
}

void CDFreezeTransformation::ScaleIncomingConnection(GvPort *inport, Real sca)
{
	GvNode *srcNode = NULL;
	GvPort *srcPort = NULL;
	
	inport->GetIncomingSource(srcNode,srcPort);
	if(srcNode && srcPort)
	{
		BaseContainer *srcData = srcNode->GetOpContainerInstance();
		if(srcData)
		{
			if(srcData->GetLong(GV_DYNAMIC_DATATYPE) == DA_REAL)
			{
				DescID dscID;
				switch(srcNode->GetOperatorID())
				{
					case ID_OPERATOR_CONST:
					{
						dscID = DescID(DescLevel(GV_CONST_VALUE,DTYPE_REAL,0));
						if(CDHasAnimationTrack(srcNode,dscID))
						{
							CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca);
						}
						else
						{
							Real r = srcData->GetReal(GV_CONST_VALUE);
							r *= sca;
							srcData->SetReal(GV_CONST_VALUE,r);
						}
						break;
					}
					case ID_OPERATOR_CONDITION:
					{
						BaseContainer *subBc = srcData->GetContainerInstance(GV_CONDITION_INPUT);
						
						LONG i, cnt = srcNode->GetInPortCount();
						for(i=1; i<cnt; i++)
						{
							GvPort *input = srcNode->GetInPort(i);
							if(input)
							{
								if(!input->IsIncomingConnected())
								{
									dscID = DescID(DescLevel(GV_CONDITION_INPUT),DescLevel(input->GetSubID(),DTYPE_REAL,0));
									if(CDHasAnimationTrack(srcNode,dscID))
									{
										CDScaleTrack(srcNode->GetDocument(),srcNode,dscID,sca);
									}
									else if(subBc)
									{
										Real r = subBc->GetReal(input->GetSubID());
										r *= sca;
										subBc->SetReal(input->GetSubID(),r);
									}
								}
								else ScaleIncomingConnection(input,sca);
							}
						}
						break;
					}
				}
			}
		}
	}
}

void CDFreezeTransformation::FreezeXpressoPositionPorts(BaseTag *tag, BaseObject *op, Vector sca)
{
	BaseDocument *doc = GetActiveDocument();
	if(doc && tag && op)
	{
		XPressoTag *xTag = static_cast<XPressoTag*>(tag);
		GvNode *gvN = xTag->GetNodeMaster()->GetRoot()->GetDown();
		while(gvN)
		{
			BaseContainer *gvData = gvN->GetOpContainerInstance();
			if(gvData)
			{
				BaseList2D *list = gvData->GetLink(GV_OBJECT_OBJECT_ID,doc);
				if(list == op)
				{
					GvOperatorData *gvOp = gvN->GetOperatorData();
					DescID dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION));
					LONG portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					
					GvPort *inPort = gvN->GetInPortFirstMainID(portID);
					if(inPort)
					{
						if(inPort->IsIncomingConnected())
						{
							ScaleIncomingConnection(inPort,sca);
						}
					}
					
					LONG i;
					Real portSca;
					for(i=0; i<3; i++)
					{
						switch(i)
						{
							case 0:
							{
								dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION), DescLevel(VECTOR_X));
								portSca = sca.x;
								break;
							}
							case 1:
							{
								dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION), DescLevel(VECTOR_Y));
								portSca = sca.y;
								break;
							}
							case 2:
							{
								dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION), DescLevel(VECTOR_Z));
								portSca = sca.z;
								break;
							}
						}
						portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
						
						inPort = gvN->GetInPortFirstMainID(portID);
						if(inPort)
						{
							if(inPort->IsIncomingConnected())
							{
								ScaleIncomingConnection(inPort,portSca);
							}
						}
					}
				}
			}
			gvN = gvN->GetNext();
		}
	}
}

void CDFreezeTransformation::FreezeAttributes(BaseList2D *lst, Vector sca)
{
	BaseContainer *lstData = lst->GetDataInstance();
	Real rSca = sca.x;
	if(rSca < sca.y) rSca = sca.y;
	if(rSca < sca.z) rSca = sca.z;
	
	AutoAlloc<Description> desc;
	CDGetDescription(lst, desc, CD_DESCFLAGS_DESC_0);
	void* h = desc->BrowseInit();
	const BaseContainer *bc = NULL;
	DescID id, groupid, scope;
	LONG i;
	while (desc->GetNext(h, &bc, id, groupid))
	{
		if(bc->GetLong(DESC_UNIT) == DESC_UNIT_METER && !bc->GetBool(DESC_HIDE))
		{
			for (i=0; i<id.GetDepth(); ++i)
			{
				if(id[i].id > 999)
				{
					if(lst->GetType() == ID_CDJOINTOBJECT && id[i].id > 1004 && id[i].id < 1010) continue;
					if(id[i].dtype == DTYPE_VECTOR)
					{
						Vector v = lstData->GetVector(id[i].id);
						v.x *= sca.x;
						v.y *= sca.y;
						v.z *= sca.z;
						lstData->SetVector(id[i].id,v);
					}
					if(id[i].dtype == DTYPE_REAL)
					{
						Real r = lstData->GetReal(id[i].id);
						r *= rSca;
						lstData->SetReal(id[i].id,r);
					}
				}
			}
		}
	}
	desc->BrowseFree(h);
}

void CDFreezeTransformation::FreezeTagAttributes(BaseObject *op, BaseContainer *data, Vector newSca, Vector oldSca)
{
	Vector trnsSca;
	trnsSca.x = oldSca.x / newSca.x;
	trnsSca.y = oldSca.y / newSca.y;
	trnsSca.z = oldSca.z / newSca.z;

	BaseTag *tag = op->GetFirstTag();
	while(tag)
	{
		if(tag->GetInfo() & TAG_VISIBLE && tag->GetType() != ID_CDMORPHSLIDERPLUGIN)
		{
			if(data->GetBool(CDJS_SCA))
			{
				FreezeAttributes(tag,trnsSca);
				if(tag->GetType() == Texpresso)
				{
					FreezeXpressoPositionPorts(tag,op,trnsSca);
				}
			}
		}
		tag->Message(CD_MSG_FREEZE_TRANSFORMATION,&trnsSca);
		tag = tag->GetNext();
	}
}

Matrix CDFreezeTransformation::GetFreezeMatrix(BaseObject *op, BaseContainer *data)
{
	Matrix rotM, oldM = op->GetMg(), newM = Matrix();
	Vector sca = CDGetScale(op);
	
	if(!data->GetBool(CDJS_POS)) newM.off = oldM.off;
	if(!data->GetBool(CDJS_SCA))
	{
		newM.v1 = VNorm(newM.v1) * Len(oldM.v1);
		newM.v2 = VNorm(newM.v2) * Len(oldM.v2);
		newM.v3 = VNorm(newM.v3) * Len(oldM.v3);
	}
	if(!data->GetBool(CDJS_ROT))
	{
		rotM.off = Vector(0,0,0);
		rotM.v1 = VNorm(oldM.v1 * sca.x);
		rotM.v2 = VNorm(oldM.v2 * sca.y);
		rotM.v3 = VNorm(oldM.v3 * sca.z);
		if(VDot(VNorm(VCross(rotM.v2, rotM.v3)), rotM.v1) < 0.0)
		{
			rotM.v1 *= -1;
		}
		newM = newM * rotM;
	}
	
	return newM;
}

Bool CDFreezeTransformation::DoFreezeChildren(BaseDocument *doc, BaseObject *op, BaseContainer *data)
{
	BaseObject *nextOp = NULL, *opR = NULL;
	Matrix oldM, newM;
	
	while(op)
	{
		oldM = op->GetMg();
		Vector newSca, trnsSca, oldSca = CDGetScale(op), oldRot = CDGetRot(op);
		
		nextOp = op->GetNext();
		if(IsValidPointObject(op))
		{
			newM = GetFreezeMatrix(op, data);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
			{
				RecalculateReference(doc,op,newM,oldM);
			}
			else RecalculatePoints(op,newM,oldM);
			
			op->SetMg(newM);
			op->Message(MSG_UPDATE);
			newSca = CDGetScale(op);
			trnsSca.x = oldSca.x / newSca.x;
			trnsSca.y = oldSca.y / newSca.y;
			trnsSca.z = oldSca.z / newSca.z;
			
			if(!data->GetBool(CDJS_SCA)) CDSetScale(op,oldSca);
			else CDSetScale(op,Vector(1.0,1.0,1.0));
			
			if(!data->GetBool(CDJS_ROT)) CDSetRot(op,oldRot);
			else CDSetRot(op,Vector(0.0,0.0,0.0));
			
			if(op->GetDown())
			{
				RepositionChildren(doc,op->GetDown(),newM,oldM,true);
				DoFreezeChildren(doc,op->GetDown(),data);	
			}
			opR = op;
		}
		else
		{
			if(data->GetBool(CDJS_FRZ_PRIM) && (op->GetInfo()&OBJECT_GENERATOR) && !(op->GetInfo()&OBJECT_INPUT))
			{
				newM = GetFreezeMatrix(op, data);
				RepositionChildren(doc,op->GetDown(),newM,oldM,true);
				
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
					if(data->GetBool(CDJS_FRZ_CHILD)) DoFreezeChildren(doc,newOp->GetDown(),data);	
				}
				opR = newOp;
			}
			else
			{
				if(!(op->GetInfo() & OBJECT_MODIFIER)  || IsJoint(op))
				{
					if(IsJoint(op))
					{
						newM.off = oldM.off;
						newM.v1 = VNorm(oldM.v1);
						newM.v2 = VNorm(oldM.v2);
						newM.v3 = VNorm(oldM.v3);
					}
					else newM = GetFreezeMatrix(op, data);
					
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					op->SetMg(newM);
					newSca = CDGetScale(op);
					trnsSca.x = oldSca.x / newSca.x;
					trnsSca.y = oldSca.y / newSca.y;
					trnsSca.z = oldSca.z / newSca.z;
					
					FreezeAttributes(op,trnsSca);
					
					if(!data->GetBool(CDJS_SCA)) CDSetScale(op,oldSca);
					else CDSetScale(op,Vector(1.0,1.0,1.0));
					
					if(!data->GetBool(CDJS_ROT)) CDSetRot(op,oldRot);
					else CDSetRot(op,Vector(0.0,0.0,0.0));
					
					op->Message(MSG_UPDATE);
					if(op->GetType() == ID_CDJOINTOBJECT && data->GetBool(CDJS_ROT))
					{
						DescriptionCommand dc;
						dc.id = DescID(JNT_ZERO_COORDS);
						op->Message(MSG_DESCRIPTION_COMMAND,&dc);
					}
				}
				else newM = oldM;

				if(op->GetDown())
				{
					RepositionChildren(doc,op->GetDown(),newM,oldM,true);
					DoFreezeChildren(doc,op->GetDown(),data);	
				}
				opR = op;
			}
		}
		FreezeTagAttributes(opR,data,newSca,oldSca);
		if(data->GetBool(CDJS_SCA)) FreezePositionTrack(doc,opR,trnsSca);
		
		op = nextOp;
	}
	
	return true;
}

Bool CDFreezeTransformation::DoFreezeTransformation(BaseDocument *doc, BaseContainer *data)
{
	BaseObject *op = NULL, *opR = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	LONG i, opCount = objects->GetCount();
	Matrix oldM, newM;
	
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				oldM = op->GetMg();
				Vector newSca, trnsSca, oldSca = CDGetScale(op), oldRot = CDGetRot(op);
				if(IsValidPointObject(op))
				{
					newM = GetFreezeMatrix(op, data);
					
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					if(op->GetTag(ID_CDSKINREFPLUGIN) || op->GetTag(ID_CDMORPHREFPLUGIN))
					{
						RecalculateReference(doc,op,newM,oldM);
					}
					else RecalculatePoints(op,newM,oldM);
					
					op->SetMg(newM);
					op->Message(MSG_UPDATE);
					newSca = CDGetScale(op);
					trnsSca.x = Abs(oldSca.x / newSca.x);
					trnsSca.y = Abs(oldSca.y / newSca.y);
					trnsSca.z = Abs(oldSca.z / newSca.z);
					
					if(!data->GetBool(CDJS_SCA)) CDSetScale(op,oldSca);
					else CDSetScale(op,Vector(1.0,1.0,1.0));
					
					if(!data->GetBool(CDJS_ROT)) CDSetRot(op,oldRot);
					else CDSetRot(op,Vector(0.0,0.0,0.0));
					
					if(op->GetDown())
					{
						RepositionChildren(doc,op->GetDown(),newM,oldM,true);
						if(data->GetBool(CDJS_FRZ_CHILD)) DoFreezeChildren(doc,op->GetDown(),data);	
					}
				}
				else
				{
					if(data->GetBool(CDJS_FRZ_PRIM) && (op->GetInfo()&OBJECT_GENERATOR) && !(op->GetInfo()&OBJECT_INPUT))
					{					
						newM = GetFreezeMatrix(op, data);
						RepositionChildren(doc,op->GetDown(),newM,oldM,true);
						
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
							if(data->GetBool(CDJS_FRZ_CHILD)) DoFreezeChildren(doc,newOp->GetDown(),data);	
						}
						opR = newOp;
					}
					else
					{
						if(!(op->GetInfo()&OBJECT_MODIFIER)  || IsJoint(op))
						{
							if(IsJoint(op))
							{
								newM.off = oldM.off;
								newM.v1 = VNorm(oldM.v1);
								newM.v2 = VNorm(oldM.v2);
								newM.v3 = VNorm(oldM.v3);
							}
							else newM = GetFreezeMatrix(op, data);
							
							CDAddUndo(doc,CD_UNDO_CHANGE,op);
							op->SetMg(newM);
							Matrix m = op->GetMg();
							
							newSca = CDGetScale(op);
							trnsSca.x = Abs(oldSca.x / newSca.x);
							trnsSca.y = Abs(oldSca.y / newSca.y);
							trnsSca.z = Abs(oldSca.z / newSca.z);
							
							FreezeAttributes(op,trnsSca);
							
							if(!data->GetBool(CDJS_SCA)) CDSetScale(op,oldSca);
							else CDSetScale(op,Vector(1.0,1.0,1.0));
							
							if(!data->GetBool(CDJS_ROT)) CDSetRot(op,oldRot);
							else CDSetRot(op,Vector(0.0,0.0,0.0));
							
							op->Message(MSG_UPDATE);
							if(op->GetType() == ID_CDJOINTOBJECT && data->GetBool(CDJS_ROT))
							{
								DescriptionCommand dc;
								dc.id = DescID(JNT_ZERO_COORDS);
								op->Message(MSG_DESCRIPTION_COMMAND,&dc);
							}
						}
						else newM = oldM;
						
						if(op->GetDown())
						{
							RepositionChildren(doc,op->GetDown(),newM,oldM,true);
							if(data->GetBool(CDJS_FRZ_CHILD)) DoFreezeChildren(doc,op->GetDown(),data);	
						}
						opR = op;
					}
				}
				FreezeTagAttributes(opR,data,newSca,oldSca);
				if(data->GetBool(CDJS_SCA)) FreezePositionTrack(doc,opR,trnsSca);
			}
		}
		doc->EndUndo();
		doc->SetActiveObject(NULL,SELECTION_NEW);
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
	
}

Bool CDFreezeTransformation::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}

	BaseContainer *wpData = GetWorldPluginData(ID_CDFREEZETRANSFORMATION);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetBool(CDJS_FRZ_CHILD,false);
		bc.SetBool(CDJS_FRZ_PRIM,false);
		bc.SetBool(CDJS_POS,false);
		bc.SetBool(CDJS_SCA,true);
		bc.SetBool(CDJS_ROT,true);
		SetWorldPluginData(ID_CDFREEZETRANSFORMATION,bc,false);
		wpData = GetWorldPluginData(ID_CDFREEZETRANSFORMATION);
	}

	Bool optnOff = true;
	if(wpData->GetBool(CDJS_POS)) optnOff = false;
	if(wpData->GetBool(CDJS_SCA)) optnOff = false;
	if(wpData->GetBool(CDJS_ROT)) optnOff = false;
	if(optnOff) return false;	

	return DoFreezeTransformation(doc, wpData);

}

class CDFreezeTransformationR : public CommandData
{
	public:		
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDFreezeTransformation(void)
{
	if(CDFindPlugin(ID_CDFREEZETRANSFORMATION,CD_COMMAND_PLUGIN)) return true;
	
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDFTRANS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDFREEZETRANSFORMATION,name,PLUGINFLAG_HIDE,"CDTFreeze.tif","CD Freeze Transformation",CDDataAllocator(CDFreezeTransformationR));
	else return CDRegisterCommandPlugin(ID_CDFREEZETRANSFORMATION,name,0,"CDTFreeze.tif","CD Freeze Transformation"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDFreezeTransformation));
}

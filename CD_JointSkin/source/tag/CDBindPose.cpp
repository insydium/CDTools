//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"
#include "customgui_inexclude.h"

//#include "CDCompatibility.h"
#include "CDTagData.h"

#include "CDJointSkin.h"
//#include "CDMatrixArray.h"
#include "CDArray.h"

#include "tCDBindPose.h"

#define MAX_POSE				50

enum // CD Bind Pose
{
	//BND_PURCHASE				= 1000,
	//BND_LOCAL_CENTER			= 1001,
	
	//BND_OBJECT_GROUP			= 1100,
	//BND_OBJECT_LIST			= 1101,
	BND_OBJECT_COUNT			= 1102,
	BND_POSE_COUNT				= 1103,
	BND_DISK_LEVEL				= 1104,
	
	//BND_RIG_MIRROR				= 1200,
	//BND_MIRROR_AXIS				= 1201,
		//BND_X_AXIS				= 1202,
		//BND_Y_AXIS				= 1203,
		//BND_Z_AXIS				= 1204,
		
	//BND_ASSIGN_MIRROR				= 1300,
	
	//BND_POSE_GROUP			= 2000,
	//BND_POSE_ADD				= 2001,
	//BND_POSE_SUB				= 2002,
	//BND_POSE_SUBGROUP			= 2003,
	BND_POSE_MIRROR				= 2004,
	BND_POSE_MIRROR_SET			= 2005,
	
	//BND_POSE_NAME				= 2100,
	//BND_POSE_SET				= 2200,
	//BND_POSE_EDIT				= 2300,
	//BND_POSE_RESTORE			= 2400,
	
	BND_POSE_IS_SET				= 2500,	
	BND_POSE_BUTTON_GROUP		= 3500,
	BND_LINE_ADD				= 4500,
	
	BND_OBJECT_LINK				= 40000,
	BND_OBJ_UD_COUNT			= 50000,
	BND_OBJ_MIR_LINK			= 60000,
	BND_POSE_CONTAINER			= 70000
};

// Object user data subcontainer ID's
enum
{
	OBJ_UD_CONTAINER		= 1000,
	UD_ENTRY				= 2000,
	UD_ID					= 3000,
	UD_TYPE					= 4000,
	UD_DATA					= 5000
	
	//UD_NAME					= 6000 // for debugging
};

enum // CD Hand Tag ID's
{
	HND_USE_MIXER					= 1051,
	
	HND_POSE_COUNT					= 6004,
	
	HND_POSE_MIX					= 6100,
	
	HND_POSE_RESTORE				= 6500
};

struct CDPose
{
	CDArray<Matrix>		mList;
};

class CDBindPosePlugin : public CDTagData
{
private:
	CDArray<CDPose>		pList;

	void CheckTagReg(BaseContainer *tData, BaseObject *op);
	Bool CheckTagAssign(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	
	void FlushOldData(BaseTag *tag, BaseContainer *tData);
	
	Bool RemoveEmptyLinks(BaseTag *tag, BaseDocument *doc, BaseContainer *tData);
	void ConfirmObjectCount(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, InExcludeData *opList);
	Bool ConfirmObjectUserData(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, LONG ind);
	
	Bool RemoveObjectsFromList(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, InExcludeData *opList);
	Bool AppendObjectList(BaseDocument *doc, BaseContainer *tData, InExcludeData *opList);
	
	Bool RemoveObjectUserData(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, LONG oInd, LONG pInd);
	Bool SetObjectUserData(BaseContainer *tData, BaseObject *op, LONG oInd, LONG pInd);
	
	Bool InitPoseList(void);
	Bool AddPose(BaseDocument *doc, BaseContainer *tData);
	Bool RemovePose(BaseDocument *doc, BaseContainer *tData);
	
	BaseObject* FindCommonParent(BaseObject *op, BaseObject *mop);
	LONG GetMirrorIndex(BaseDocument *doc, BaseContainer *tData, BaseObject *mop);
	Matrix GetMirrorPoseMatrix(BaseContainer *tData, Matrix opM);
	Matrix GetMirrorPoseMatrix(BaseTag *tag, BaseContainer *tData, BaseObject *op, BaseObject *mop, Matrix mirMl);
	
	void SetMirrorAssignment(BaseTag *tag, BaseDocument *doc, BaseContainer *tData);
	void RestoreMirroredPose(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, LONG ind);
	
	Bool StoreUserData(BaseContainer *tData, BaseObject *op, LONG oInd, LONG pInd);
	void SetPose(BaseDocument *doc, BaseContainer *tData, LONG ind);
	void RestorePose(BaseDocument *doc, BaseContainer *tData, LONG ind);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDBindPosePlugin); }
};

Bool CDBindPosePlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
			
	tData->SetLong(BND_MIRROR_AXIS,BND_X_AXIS);
	tData->SetBool(BND_POSE_MIRROR_SET, false);
	
	tData->SetString(BND_POSE_NAME,"Bind Pose");
	tData->SetLong(BND_POSE_COUNT,1);
	tData->SetLong(BND_OBJECT_COUNT,0);
	tData->SetBool(BND_POSE_IS_SET,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	CDJSData bpd;
	PluginMessage(ID_CDBINDPOSETAG,&bpd);
	if(bpd.list) bpd.list->Append(node);
	
	return true;
}

void CDBindPosePlugin::Free(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	pList.Free();
	
	CDJSData bpd;
	PluginMessage(ID_CDBINDPOSETAG,&bpd);
	if(bpd.list) bpd.list->Remove(node);
}

Bool CDBindPosePlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(BND_DISK_LEVEL,level);
	
	LONG i, pCnt = tData->GetLong(BND_POSE_COUNT);
	LONG j, oCnt = tData->GetLong(BND_OBJECT_COUNT);// 
	
	if(level > 0)
	{
		if(pCnt > 0 && oCnt > 0)
		{
			if(!pList.Alloc(pCnt)) return false;
			
			// read the matrix list
			for (i=0; i<pCnt; i++)
			{
				if(!pList[i].mList.Alloc(oCnt)) return false;
				
				for (j=0; j<oCnt; j++)
				{
					Matrix m;
					hf->ReadMatrix(&m);
					pList[i].mList[j] = m;
				}
			}
		}
	}
	
	return true;
}

Bool CDBindPosePlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	LONG i, pCnt = tData->GetLong(BND_POSE_COUNT);
	LONG j, oCnt = tData->GetLong(BND_OBJECT_COUNT);

	//Level 1
	if(pCnt > 0 && oCnt > 0)
	{
		// write the matrix list
		for (i=0; i<pCnt; i++)
		{
			for (j=0; j<oCnt; j++)
			{
				hf->WriteMatrix(pList[i].mList[j]);
				
			}
		}
	}
	
	return true;
}

Bool CDBindPosePlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag  = (BaseTag*)snode;
	BaseContainer *tData = tag->GetDataInstance();
	
	LONG i, pCnt = tData->GetLong(BND_POSE_COUNT);
	LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);//j, 
	
	if(pCnt > 0)
	{
		pList.Copy(((CDBindPosePlugin*)dest)->pList);
	}

	return true;
}

Bool CDBindPosePlugin::InitPoseList(void)
{
	pList.Alloc(1);
	pList[0].mList.Init();
	
	return true;
}

Bool CDBindPosePlugin::AppendObjectList(BaseDocument *doc, BaseContainer *tData, InExcludeData *opList)
{
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	LONG i, opCnt = opList->GetObjectCount();
	LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
	LONG p, pCnt = tData->GetLong(BND_POSE_COUNT);
	
	// store objects from tData links into AtomArray
	BaseObject *op = NULL;
	for(i=0; i<oCnt; i++)
	{
		op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
		if(op) objects->Append(op);
	}
	
	// add objects from InExlude list that are not in AtomArray
	for(i=0; i<opCnt; i++)
	{
		op = (BaseObject*)opList->ObjectFromIndex(doc,i);
		if(op)
		{
			if(objects->Find(op) == NOTOK)
			{
				tData->SetLink(BND_OBJECT_LINK+oCnt,op);
				
				// add object to poses
				for(p=0; p<pCnt; p++)
				{
					pList[p].mList.Append(op->GetMl());
					StoreUserData(tData,op,oCnt,p);
				}
				oCnt++;
			}
		}
	}
	
	tData->SetLong(BND_OBJECT_COUNT,oCnt);
	
	return true;
}

Bool CDBindPosePlugin::AddPose(BaseDocument *doc, BaseContainer *tData)
{
	LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
	LONG pCnt = tData->GetLong(BND_POSE_COUNT);
	
	CDPose p;
	pList.Append(p);
	
	pCnt++;
	tData->SetLong(BND_POSE_COUNT,pCnt);
	tData->SetBool(BND_POSE_IS_SET+pCnt,false);
	
	return true;
}

Bool CDBindPosePlugin::RemovePose(BaseDocument *doc, BaseContainer *tData)
{
	LONG pCnt = tData->GetLong(BND_POSE_COUNT);
	
	pCnt--;
	pList.RemoveEnd();
	
	tData->SetLong(BND_POSE_COUNT,pCnt);
	tData->SetString(BND_POSE_NAME+pCnt,"");
	tData->SetBool(BND_POSE_IS_SET+pCnt,false);
	
	return true;
}

Bool CDBindPosePlugin::ConfirmObjectUserData(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, LONG ind)
{
	LONG uCnt = GetUserDataCount(op), eCnt = tData->GetLong(BND_OBJ_UD_COUNT+ind);
	if(uCnt < eCnt)
	{
		CDAddUndo(doc,CD_UNDO_CHANGE,tag);
		
		DynamicDescription* dd = op->GetDynamicDescription();
		if(!dd)
		{
			tData->SetLong(BND_OBJ_UD_COUNT+ind,0);
			return false;
		}
		
		LONG i, p, pCnt = tData->GetLong(BND_POSE_COUNT);
		for(p=0; p<pCnt; p++)
		{
			BaseContainer *poseBC = tData->GetContainerInstance(BND_POSE_CONTAINER+p);
			if(poseBC)
			{
				BaseContainer *opudBC = poseBC->GetContainerInstance(OBJ_UD_CONTAINER+ind);
				if(opudBC)
				{
					BaseContainer oClone;
					
					DescID id;
					const BaseContainer* bc;
					
					void* handle = dd->BrowseInit();
					LONG st = 0;
					while (dd->BrowseGetNext(handle, &id, &bc))
					{
						GeData data;
						CDGetParameter(op,id,data);
						
						BaseContainer udClone;
						for(i=0; i<eCnt; i++)
						{
							BaseContainer *udBC = opudBC->GetContainerInstance(UD_ENTRY+i);
							if(udBC)
							{
								if(udBC->GetLong(UD_ID) == id[id.GetDepth()-1].id)
								{
									udClone.SetLong(UD_ID,udBC->GetLong(UD_ID));
									udClone.SetLong(UD_TYPE,udBC->GetLong(UD_TYPE));
									udClone.SetData(UD_DATA,udBC->GetData(UD_DATA));
									oClone.SetContainer(UD_ENTRY+st,udClone);
									st++;
								}
							}
						}
					}
					dd->BrowseFree(handle);
					
					poseBC->SetContainer(OBJ_UD_CONTAINER+ind,oClone);
				}
			}
		}
		tData->SetLong(BND_OBJ_UD_COUNT+ind,uCnt);
	}
	else if(uCnt > eCnt)
	{
		CDAddUndo(doc,CD_UNDO_CHANGE,tag);
		
		DynamicDescription* dd = op->GetDynamicDescription();
		if(!dd)
		{
			tData->SetLong(BND_OBJ_UD_COUNT+ind,0);
			return false;
		}
		
		LONG i, p, pCnt = tData->GetLong(BND_POSE_COUNT);
		for(p=0; p<pCnt; p++)
		{
			BaseContainer poseBC = tData->GetContainer(BND_POSE_CONTAINER+p);
			BaseContainer opudBC = poseBC.GetContainer(OBJ_UD_CONTAINER+ind);
			
			DescID id;
			const BaseContainer* bc;
			
			GeData data;
			void* handle = dd->BrowseInit();
			
			Bool foundNewUD = false;
			while (dd->BrowseGetNext(handle, &id, &bc))
			{
				CDGetParameter(op,id,data);
				
				BaseContainer udClone;
				for(i=0; i<eCnt; i++)
				{
					BaseContainer *udBC = opudBC.GetContainerInstance(UD_ENTRY+i);
					if(udBC)
					{
						if(udBC->GetLong(UD_ID) != id[id.GetDepth()-1].id)
						{
							foundNewUD = true;
							break;
						}
					}
				}
				if(foundNewUD) break;
			}
			BaseContainer udAdd;
			udAdd.SetLong(UD_ID,id[id.GetDepth()-1].id);
			udAdd.SetLong(UD_TYPE,data.GetType());
			udAdd.SetData(UD_DATA,data);
			opudBC.SetContainer(UD_ENTRY+eCnt,udAdd);
			
			dd->BrowseFree(handle);
			
			poseBC.SetContainer(OBJ_UD_CONTAINER+ind,opudBC);
			tData->SetContainer(BND_POSE_CONTAINER+p,poseBC);
			tData->SetLong(BND_OBJ_UD_COUNT+ind,uCnt);
		}
	}
	
	return true;
}

Bool CDBindPosePlugin::SetObjectUserData(BaseContainer *tData, BaseObject *op, LONG oInd, LONG pInd)
{
	DynamicDescription* dd = op->GetDynamicDescription(); if(!dd) return false;
	
	BaseContainer *poseBC = tData->GetContainerInstance(BND_POSE_CONTAINER+pInd); if(!poseBC)return false;
	BaseContainer *opudBC = poseBC->GetContainerInstance(OBJ_UD_CONTAINER+oInd); if(!opudBC) return false;

	LONG udCnt = tData->GetLong(BND_OBJ_UD_COUNT+oInd);
	if(udCnt != GetUserDataCount(op)) return false;
	
	if(udCnt > 0)
	{
		DescID id;
		const BaseContainer* bc;
		
		void* handle = dd->BrowseInit();
		LONG i=0;
		while (dd->BrowseGetNext(handle, &id, &bc))
		{
			GeData data;
			CDGetParameter(op,id,data);
			
			BaseContainer *udBc = opudBC->GetContainerInstance(UD_ENTRY+i);
			if(udBc)
			{
				if(udBc->GetLong(UD_ID) == id[id.GetDepth()-1].id && udBc->GetLong(UD_TYPE) == data.GetType())
				{
					CDSetParameter(op,id,udBc->GetData(UD_DATA),CD_DESCFLAGS_SET_DONTCHECKMINMAX);
				}
			}
			i++;
		}
		dd->BrowseFree(handle);
	}
	
	return true;
}

Bool CDBindPosePlugin::StoreUserData(BaseContainer *tData, BaseObject *op, LONG oInd, LONG pInd)
{
	DynamicDescription* dd = op->GetDynamicDescription(); if(!dd) return false;
	
	BaseContainer poseBC = tData->GetContainer(BND_POSE_CONTAINER+pInd);
	BaseContainer opudBC = poseBC.GetContainer(OBJ_UD_CONTAINER+oInd);
	
	LONG udCnt = GetUserDataCount(op);
	tData->SetLong(BND_OBJ_UD_COUNT+oInd,udCnt);
	if(udCnt > 0)
	{
		DescID id;
		const BaseContainer* bc;
		
		void* handle = dd->BrowseInit();
		LONG i=0;
		while (dd->BrowseGetNext(handle, &id, &bc))
		{
			GeData data;
			CDGetParameter(op,id,data);
			
			BaseContainer udBc;
			udBc.SetLong(UD_ID,id[id.GetDepth()-1].id);
			udBc.SetLong(UD_TYPE,data.GetType());
			udBc.SetData(UD_DATA,data);
			opudBC.SetContainer(UD_ENTRY+i,udBc);
			i++;
		}
		dd->BrowseFree(handle);
		
		poseBC.SetContainer(OBJ_UD_CONTAINER+oInd,opudBC);
		tData->SetContainer(BND_POSE_CONTAINER+pInd,poseBC);
	}
	
	return true;
}

void CDBindPosePlugin::SetPose(BaseDocument *doc, BaseContainer *tData, LONG ind)
{
	LONG i, oCnt = tData->GetLong(BND_OBJECT_COUNT);
	
	BaseObject *op = NULL;
	for(i=0; i<oCnt; i++)
	{
		op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
		if(op)
		{
			pList[ind].mList[i] = op->GetMl();
			StoreUserData(tData,op,i,ind);
		}
	}
	
	tData->SetBool(BND_POSE_IS_SET+ind,true);
}

void CDBindPosePlugin::SetMirrorAssignment(BaseTag *tag, BaseDocument *doc, BaseContainer *tData)
{
	LONG i, j, oCnt = tData->GetLong(BND_OBJECT_COUNT);
	
	BaseObject *pr = tag->GetObject(), *op = NULL, *mop = NULL;
	Matrix prM = pr->GetMg();
	
	for(i=0; i<oCnt; i++)
	{
		op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
		if(op)
		{
			Vector opPos, mopPos, mirPos;
			
			if(!tData->GetBool(BND_LOCAL_CENTER)) opPos = op->GetMg().off;
			else opPos = MInv(prM) * op->GetMg().off;
			
			mirPos = opPos;
			switch(tData->GetLong(BND_MIRROR_AXIS))
			{
				case BND_X_AXIS:
				{
					mirPos.x *= -1;
					break;
				}
				case BND_Y_AXIS:
				{
					mirPos.y *= -1;
					break;
				}
				case BND_Z_AXIS:
				{
					mirPos.z *= -1;
					break;
				}
			}
			
			if(VEqual(mirPos,opPos,0.001))
			{
				tData->SetLink(BND_OBJ_MIR_LINK+i,op);
			}
			else
			{
				Real prvDif = CDMAXREAL;
				for(j=0; j<oCnt; j++)
				{
					mop = tData->GetObjectLink(BND_OBJECT_LINK+j,doc);
					if(mop)
					{
						if(!IsParentObject(op, mop))
						{
							if(!tData->GetBool(BND_LOCAL_CENTER)) mopPos = mop->GetMg().off;
							else mopPos = MInv(prM) * mop->GetMg().off;
							
							Real dif = Len(mirPos - mopPos);
							if(dif < prvDif)
							{
								if(IsHierarchyEqual(op->GetDown(),mop->GetDown()))
								{
									tData->SetLink(BND_OBJ_MIR_LINK+i,mop);
									prvDif = dif;
								}
							}
						}
					}
				}
			}
		}
	}
	
	tData->SetBool(BND_POSE_MIRROR_SET,true);
}

BaseObject* CDBindPosePlugin::FindCommonParent(BaseObject *op, BaseObject *mop)
{
	if(!op || !mop) return NULL;
	
	BaseObject *opPr = op->GetUp();
	BaseObject *mopPr = mop->GetUp();
	
	if(opPr == mopPr) return opPr;
	
	while(opPr && mopPr && opPr != mopPr)
	{
		opPr = opPr->GetUp();
		mopPr = mopPr->GetUp();
		if(opPr == mopPr) return opPr;
	}
	
	return NULL;
}

LONG CDBindPosePlugin::GetMirrorIndex(BaseDocument *doc, BaseContainer *tData, BaseObject *mop)
{
	LONG i, mi, oCnt = tData->GetLong(BND_OBJECT_COUNT);
	
	for(i=0; i<oCnt; i++)
	{
		BaseObject *op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
		if(op && op == mop) mi = i;
	}

	return mi;
}

Matrix CDBindPosePlugin::GetMirrorPoseMatrix(BaseContainer *tData, Matrix opM)
{
	Matrix retM;
	CDQuaternion opQuat, targetQuat;
	Vector targetPos, opPos;
	
	opQuat.SetMatrix(opM);
	opPos = opM.off;
	switch(tData->GetLong(BND_MIRROR_AXIS))
	{
		case BND_X_AXIS:	
			targetPos.x = -opPos.x;
			targetPos.y = opPos.y;
			targetPos.z = opPos.z;
			targetQuat.w = -opQuat.w;
			targetQuat.v.x = -opQuat.v.x;
			targetQuat.v.y = opQuat.v.y;
			targetQuat.v.z = opQuat.v.z;
			break;
		case BND_Y_AXIS:
			targetPos.x = opPos.x;
			targetPos.y = -opPos.y;
			targetPos.z = opPos.z;
			targetQuat.w = -opQuat.w;
			targetQuat.v.x = opQuat.v.x;
			targetQuat.v.y = -opQuat.v.y;
			targetQuat.v.z = opQuat.v.z;
			break;
		case BND_Z_AXIS:
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
	
	return retM;
}
	
Matrix CDBindPosePlugin::GetMirrorPoseMatrix(BaseTag *tag, BaseContainer *tData, BaseObject *op, BaseObject *mop, Matrix mirMl)
{
	Matrix retM, mpMirMl, mpPrMirMl, rotM = Matrix();
	
	BaseObject *tagOp = FindCommonParent(op,mop);
	GePrint(op->GetName()+" & "+mop->GetName()+" common parent is "+tagOp->GetName());
	BaseObject *opPr = op->GetUp(), *mopPr = mop->GetUp();
	
	Matrix tagOpM = tagOp->GetMg();
	Matrix mopPrMl = MInv(tagOpM) * mopPr->GetMg(), opPrMl = MInv(tagOpM) * opPr->GetMg();
	
	mopPrMl.off = Vector(0,0,0);
	opPrMl.off = Vector(0,0,0);
	
	Matrix prM = opPrMl;
	Real dotx, dotz = VDot(VNorm(mopPrMl.v3), VNorm(opPrMl.v3));
	if(dotz < 0)
	{
		GePrint("    parent Z Axis in opposite direction");
		switch(tData->GetLong(BND_MIRROR_AXIS))
		{
			case BND_X_AXIS:
				mpPrMirMl = GetMirrorMatrix(mopPrMl*mirMl, Matrix(),0);
				dotx = VDot(VNorm(mpPrMirMl.v1), VNorm(opPrMl.v1));
				if(dotx < 0)
				{
					rotM = MatrixRotZ(pi);
				}
				mpMirMl = GetMirrorMatrix(mopPrMl*mirMl, Matrix(),0) * rotM;
				break;
			case BND_Y_AXIS:
				mpPrMirMl = GetMirrorMatrix(mopPrMl*mirMl, Matrix(),1);
				dotx = VDot(VNorm(mpPrMirMl.v1), VNorm(opPrMl.v1));
				if(dotx < 0)
				{
					rotM = MatrixRotZ(pi);
				}
				mpMirMl = GetMirrorMatrix(mopPrMl*mirMl, Matrix(),1) * rotM;
				break;
			case BND_Z_AXIS:
				mpPrMirMl = GetMirrorMatrix(mopPrMl*mirMl, Matrix(),2);
				dotx = VDot(VNorm(mpPrMirMl.v1), VNorm(opPrMl.v1));
				if(dotx < 0)
				{
					rotM = MatrixRotZ(pi);
				}
				mpMirMl = GetMirrorMatrix(mopPrMl*mirMl, Matrix(),2) * rotM;
				break;
		}
	}
	else
	{
		GePrint("    parent Z Axis in same direction");
		mpPrMirMl = GetMirrorPoseMatrix(tData,mopPrMl);
		dotx = VDot(VNorm(mpPrMirMl.v2), VNorm(opPrMl.v2));
		if(dotx < 0)
		{
			rotM = MatrixRotZ(pi);
			prM = mpPrMirMl * rotM;
		}
		mpMirMl = GetMirrorPoseMatrix(tData,mopPrMl*mirMl) * rotM;
	}
	
	retM = MInv(prM) * mpMirMl;
	
	return retM;
}

void CDBindPosePlugin::RestoreMirroredPose(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, LONG ind)
{
	if(tData->GetBool(BND_POSE_IS_SET+ind))
	{
		LONG i, oCnt = tData->GetLong(BND_OBJECT_COUNT);
		
		BaseObject *op = NULL, *mop = NULL;
		for(i=0; i<oCnt; i++)
		{
			op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
			if(op)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				mop = tData->GetObjectLink(BND_OBJ_MIR_LINK+i,doc);
				if(mop)
				{
					if(IsCommandEnabled(IDM_AUTOKEYS) && CDIsCommandChecked(IDM_AUTOKEYS))
					{
						AutoAlloc<AliasTrans> trans;
						BaseObject *undo = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,trans);
						if(undo)
						{
							LONG mi = GetMirrorIndex(doc,tData,mop);
							Matrix mirMl = pList[ind].mList[mi];
							if(op->GetUp() && mop->GetUp() && !IsParentObject(mop,op->GetUp()))
							{
								op->SetMl(GetMirrorPoseMatrix(tag,tData,op,mop,mirMl));
							}
							else
							{
								op->SetMl(GetMirrorPoseMatrix(tData,mirMl));
							}
							BaseTag *hTag = op->GetTag(ID_CDHANDPLUGIN);
							BaseTag *mhTag = mop->GetTag(ID_CDHANDPLUGIN);
							if(hTag && mhTag)
							{
								CDPoseCopyData pcd;
								pcd.pInd = ind;
								pcd.hTag = mhTag;
								
								hTag->Message(CD_MSG_POSE_COPY,&pcd);
							}
							Bool parm = false;
							if(SetObjectUserData(tData,op,mi,ind))
							{
								if(CDIsCommandChecked(IDM_A_PARAMETER)) parm = true;
							}
							doc->AutoKey(undo,op,false,CDIsCommandChecked(IDM_A_POS),CDIsCommandChecked(IDM_A_SIZE),CDIsCommandChecked(IDM_A_DIR),parm,false);
							
							BaseObject::Free(undo);
						}
					}
					else
					{
						LONG mi = GetMirrorIndex(doc,tData,mop);
						Matrix mirMl = pList[ind].mList[mi];
						if(op->GetUp() && mop->GetUp() && !IsParentObject(mop,op->GetUp()))
						{
							op->SetMl(GetMirrorPoseMatrix(tag,tData,op,mop,mirMl));
						}
						else
						{
							op->SetMl(GetMirrorPoseMatrix(tData,mirMl));
						}
						BaseTag *hTag = op->GetTag(ID_CDHANDPLUGIN);
						BaseTag *mhTag = mop->GetTag(ID_CDHANDPLUGIN);
						if(hTag && mhTag)
						{
							CDPoseCopyData pcd;
							pcd.pInd = ind;
							pcd.hTag = mhTag;
							
							hTag->Message(CD_MSG_POSE_COPY,&pcd);
						}
						SetObjectUserData(tData,op,mi,ind);
					}
				}
			}
		}
	}
}

void CDBindPosePlugin::RestorePose(BaseDocument *doc, BaseContainer *tData, LONG ind)
{
	if(tData->GetBool(BND_POSE_IS_SET+ind))
	{
		LONG i, oCnt = tData->GetLong(BND_OBJECT_COUNT);
		
		BaseObject *op = NULL;
		for(i=0; i<oCnt; i++)
		{
			op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
			if(op)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				if(IsCommandEnabled(IDM_AUTOKEYS) && CDIsCommandChecked(IDM_AUTOKEYS))
				{
					AutoAlloc<AliasTrans> trans;
					BaseObject *undo = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,trans);
					if(undo)
					{
						op->SetMl(pList[ind].mList[i]);
						Bool parm = false;
						BaseTag *hTag = op->GetTag(ID_CDHANDPLUGIN);
						if(hTag)
						{
							BaseContainer *htData = hTag->GetDataInstance();
							LONG hp, hpCnt = htData->GetLong(HND_POSE_COUNT);
							
							if(htData->GetReal(HND_USE_MIXER) > 0.0 && ind < hpCnt)
							{
								for(hp=0; hp<hpCnt; hp++)
								{
									if(hp == ind) htData->SetReal(HND_POSE_MIX+hp,1.0);
									else htData->SetReal(HND_POSE_MIX+hp,0.0);
								}
							}
							else
							{
								DescriptionCommand dc;
								dc.id = DescID(HND_POSE_RESTORE+ind); // restore the CD Hand tag's pose
								hTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							}
						}
						if(SetObjectUserData(tData,op,i,ind))
						{
							if(CDIsCommandChecked(IDM_A_PARAMETER)) parm = true;
						}
						doc->AutoKey(undo,op,false,CDIsCommandChecked(IDM_A_POS),CDIsCommandChecked(IDM_A_SIZE),CDIsCommandChecked(IDM_A_DIR),parm,false);
						
						BaseObject::Free(undo);
					}
				}
				else
				{
					op->SetMl(pList[ind].mList[i]);
					BaseTag *hTag = op->GetTag(ID_CDHANDPLUGIN);
					if(hTag)
					{
						BaseContainer *htData = hTag->GetDataInstance();
						LONG hp, hpCnt = htData->GetLong(HND_POSE_COUNT);
						
						if(htData->GetReal(HND_USE_MIXER) > 0.0 && ind < hpCnt)
						{
							for(hp=0; hp<hpCnt; hp++)
							{
								if(hp == ind) htData->SetReal(HND_POSE_MIX+hp,1.0);
								else htData->SetReal(HND_POSE_MIX+hp,0.0);
							}
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(HND_POSE_RESTORE+ind); // restore the CD Hand tag's pose
							hTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
						}
					}
					SetObjectUserData(tData,op,i,ind);
				}
			}
		}
	}
}

Bool CDBindPosePlugin::RemoveObjectsFromList(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, InExcludeData *opList)
{
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	LONG i, opCnt = opList->GetObjectCount();
	LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
	
	BaseObject *op = NULL;
	for(i=0; i<opCnt; i++)
	{
		op = (BaseObject*)opList->ObjectFromIndex(doc,i);
		if(op) objects->Append(op);
	}
	
	for(i=0; i<oCnt; i++)
	{
		op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
		if(op)
		{
			if(objects->Find(op) == NOTOK)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				tData->SetLink(BND_OBJECT_LINK+i,NULL);
			}
		}
	}
	
	RemoveEmptyLinks(tag,doc,tData);
	
	return true;
}

Bool CDBindPosePlugin::RemoveObjectUserData(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, LONG oInd, LONG pInd)
{
	LONG i, oCnt = tData->GetLong(BND_OBJECT_COUNT);
	
	BaseContainer *poseBC = tData->GetContainerInstance(BND_POSE_CONTAINER+pInd); if(!poseBC) return false;
	
	CDAddUndo(doc,CD_UNDO_CHANGE,tag);
	for(i=oInd+1; i<oCnt; i++)
	{
		BaseContainer *srcBC = poseBC->GetContainerInstance(OBJ_UD_CONTAINER+i);
		if(srcBC)
		{
			BaseContainer *dstBC = poseBC->GetContainerInstance(OBJ_UD_CONTAINER+(i-1));
			if(dstBC)
			{
			#if API_VERSION < 12000
				srcBC->CopyTo(dstBC,NULL);
			#else
				srcBC->CopyTo(dstBC,COPYFLAGS_0,NULL);
			#endif
			}
		}
	}
		
	return true;
}

Bool CDBindPosePlugin::RemoveEmptyLinks(BaseTag *tag, BaseDocument *doc, BaseContainer *tData)
{
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	
	LONG i, oCnt = tData->GetLong(BND_OBJECT_COUNT);
	LONG p, pCnt = tData->GetLong(BND_POSE_COUNT);
	
	BaseObject *op = NULL;
	for(i=oCnt-1; i>-1; i--)
	{
		op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
		if(!op)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			
			for(p=0; p<pCnt; p++)
			{
				pList[p].mList.Remove(i);
				RemoveObjectUserData(tag,doc,tData,op,i,p);
			}
			
			if(i < oCnt-1)
			{
				for(LONG n=i; n<oCnt-1; n++)
				{
					tData->SetLong(BND_OBJ_UD_COUNT+n,tData->GetLong(BND_OBJ_UD_COUNT+n+1));
					tData->SetLink(BND_OBJ_MIR_LINK+n,tData->GetLink(BND_OBJ_MIR_LINK+n+1,doc));
				}
			}
		}
		else objects->Append(op);
	}
	
	oCnt = objects->GetCount();
	LONG oInd = 0;
	
	CDAddUndo(doc,CD_UNDO_CHANGE,tag);
	for(i=oCnt-1; i>-1; i--)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(oInd));
		if(op) tData->SetLink(BND_OBJECT_LINK+i,op);
		oInd++;
	}
	tData->SetLong(BND_OBJECT_COUNT,oCnt);
	
	return true;
}

void CDBindPosePlugin::ConfirmObjectCount(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, InExcludeData *opList)
{
	LONG opCnt = opList->GetObjectCount();
	LONG i;
	
	Bool opDeleted = false;
	for(i=0; i<opCnt; i++)
	{
		BaseList2D *op = opList->ObjectFromIndex(doc, i);
		if(!op)
		{
			opDeleted = true;
			CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			opList->DeleteObject(i);
		}
	}
	if(opDeleted) RemoveEmptyLinks(tag,doc,tData);
}

void CDBindPosePlugin::FlushOldData(BaseTag *tag, BaseContainer *tData)
{
	tData->FlushAll();
	
	tData->SetLong(BND_MIRROR_AXIS,BND_X_AXIS);
	tData->SetBool(BND_POSE_MIRROR_SET, false);
	
	tData->SetString(BND_POSE_NAME,"Bind Pose");
	tData->SetLong(BND_POSE_COUNT,1);
	tData->SetLong(BND_OBJECT_COUNT,0);
	tData->SetBool(BND_POSE_IS_SET,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	GeData d;
	if(CDGetParameter(tag,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(tag,DescLevel(EXPRESSION_PRIORITY),d);
	}
}

Bool CDBindPosePlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdjnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdjnr);
			break;
		}
	}
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	CheckTagReg(tData,op);
	
	InExcludeData *opList = (InExcludeData*)tData->GetCustomDataType(BND_OBJECT_LIST,CUSTOMDATATYPE_INEXCLUDE_LIST); 

	LONG i, pCnt = tData->GetLong(BND_POSE_COUNT);
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			if(tData->GetLong(BND_DISK_LEVEL) < 1) FlushOldData(tag,tData);
			if(pList.IsEmpty()) InitPoseList();
			break;
		}
		case MSG_MENUPREPARE:
		{
			InitPoseList();
			break;
		}
		case CD_MSG_UPDATE:
		{
			if(opList)
			{
				ConfirmObjectCount(tag,doc,tData,opList); // check objects deleted from doc
				
				LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
				for(i=0; i<oCnt; i++)
				{
					op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
					if(op)
					{
						if(CDIsDirty(op,CD_DIRTY_DATA))
						{
							ConfirmObjectUserData(tag,doc,tData,op,i);
						}
					}
				}
			}
			break;
		}
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			Vector *trnsSca = (Vector*)data;
			if(trnsSca)
			{
				Vector sca = *trnsSca;
				LONG p, oCnt = tData->GetLong(BND_OBJECT_COUNT);
				for(p=0; p<pCnt; p++)
				{
					for(i=0; i<oCnt; i++)
					{
						BaseObject *op = tData->GetObjectLink(BND_OBJECT_LINK+i,doc);
						if(op)
						{
							Matrix opM = pList[p].mList[i];
							opM.off.x *= sca.x;
							opM.off.y *= sca.y;
							opM.off.z *= sca.z;
							pList[p].mList[i] = opM;
						}
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_INITUNDO:
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == BND_OBJECT_LIST)
			{
				if(opList)
				{
					LONG opCnt = opList->GetObjectCount();
					LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
					if(opCnt < oCnt)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						RemoveObjectsFromList(tag,doc,tData,opList);
					}
					else if(opCnt > oCnt)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						AppendObjectList(doc,tData,opList);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==BND_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==BND_ASSIGN_MIRROR)
			{
				if(tData->GetBool(T_REG))
				{
					if(opList)
					{
						LONG oIndA, oIndB, sCnt = 0;
						LONG i, opCnt = opList->GetObjectCount();
						for(i=0; i<opCnt; i++)
						{
							BaseContainer *sData = opList->GetData(i);
							if(sData->GetBool(IN_EXCLUDE_DATA_SELECTION))
							{
								if(sCnt == 0) oIndA = i;
								else if(sCnt == 1) oIndB = i;
								sCnt++;
							}
						}
						
						if(sCnt < 3)
						{
							BaseObject *opA = NULL, *opB = NULL;
							if(sCnt == 1)
							{
								opA = (BaseObject*)opList->ObjectFromIndex(doc,oIndA);
								
								LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
								for(i=0; i<oCnt; i++)
								{
									if(opA == tData->GetObjectLink(BND_OBJECT_LINK+i,doc))
									{
										tData->SetLink(BND_OBJ_MIR_LINK+i,opA);
									}
								}
							}
							else if(sCnt == 2)
							{
								opA = (BaseObject*)opList->ObjectFromIndex(doc,oIndA);
								opB = (BaseObject*)opList->ObjectFromIndex(doc,oIndB);
								
								LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
								for(i=0; i<oCnt; i++)
								{
									if(opA == tData->GetObjectLink(BND_OBJECT_LINK+i,doc))
									{
										tData->SetLink(BND_OBJ_MIR_LINK+i,opB);
									}
									if(opB == tData->GetObjectLink(BND_OBJECT_LINK+i,doc))
									{
										tData->SetLink(BND_OBJ_MIR_LINK+i,opA);
									}
								}
							}
						}
					}
				}
			}
			else if(dc->id[0].id==BND_POSE_ADD)
			{
				if(tData->GetBool(T_REG))
				{
					if(pCnt < MAX_POSE)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						AddPose(doc,tData);
					}
				}
			}
			else if(dc->id[0].id==BND_POSE_SUB)
			{
				if(tData->GetBool(T_REG))
				{
					if(pCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						RemovePose(doc,tData);
					}
				}
			}
			
			for(i=0; i<pCnt;i++)
			{
				if(dc->id[0].id==BND_POSE_SET+i)
				{
					if(tData->GetBool(T_REG))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
						SetPose(doc,tData,i);
						if(i==0 && tData->GetBool(BND_RIG_MIRROR))
						{
							SetMirrorAssignment(tag, doc,tData);
						}
					}
				}
				else if(dc->id[0].id==BND_POSE_EDIT+i)
				{
					if(tData->GetBool(T_REG))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
						RestorePose(doc,tData,i);
						tData->SetBool(BND_POSE_IS_SET+i,false);
					}
				}
				else if(dc->id[0].id==BND_POSE_RESTORE+i)
				{
					RestorePose(doc,tData,i);
				}
				else if(dc->id[0].id==BND_POSE_MIRROR+i)
				{
					RestoreMirroredPose(tag,doc,tData,i);
				}
			}
		}
	}

	return true;
}

LONG CDBindPosePlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,tData,op)) return false;
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDBindPosePlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if(!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(BND_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	bc = description->GetParameterI(DescLevel(BND_OBJECT_GROUP), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, true);
	}
	
	LONG i, pCnt = tData->GetLong(BND_POSE_COUNT);
	for (i=1; i<pCnt; i++)
	{
		BaseContainer bcLn = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bcLn.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bcLn.SetBool(DESC_SEPARATORLINE,true);
		if(!description->SetParameter(DescLevel(BND_LINE_ADD+i, DTYPE_SEPARATOR, 0),bcLn,DescLevel(BND_POSE_SUBGROUP))) return false;

		BaseContainer subgroup = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup.SetLong(DESC_COLUMNS, 5);
		if(!description->SetParameter(DescLevel(BND_POSE_BUTTON_GROUP+i, DTYPE_GROUP, 0), subgroup, DescLevel(BND_POSE_SUBGROUP))) return true;

		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_STRING);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_STRING);
		bc1.SetString(DESC_NAME,GeLoadString(IDS_POSE)+"."+CDLongToString(i));
		bc1.SetLong(DESC_ANIMATE, DESC_ANIMATE_OFF);
		if(!description->SetParameter(DescLevel(BND_POSE_NAME+i, DTYPE_STRING, 0), bc1, DescLevel(BND_POSE_BUTTON_GROUP+i))) return true;

		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_POSE_SET));
		if(!description->SetParameter(DescLevel(BND_POSE_SET+i,DTYPE_BUTTON,0),bc2,DescLevel(BND_POSE_BUTTON_GROUP+i))) return false;

		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc3.SetString(DESC_NAME,GeLoadString(IDS_POSE_EDIT));
		if(!description->SetParameter(DescLevel(BND_POSE_EDIT+i,DTYPE_BUTTON,0),bc3,DescLevel(BND_POSE_BUTTON_GROUP+i))) return false;

		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc4.SetString(DESC_NAME,GeLoadString(IDS_POSE_RESTORE));
		if(!description->SetParameter(DescLevel(BND_POSE_RESTORE+i,DTYPE_BUTTON,0),bc4,DescLevel(BND_POSE_BUTTON_GROUP+i))) return false;

		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc5.SetString(DESC_NAME,GeLoadString(IDS_POSE_MIRROR));
		if(!description->SetParameter(DescLevel(BND_POSE_MIRROR+i,DTYPE_BUTTON,0),bc5,DescLevel(BND_POSE_BUTTON_GROUP+i))) return false;
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

void CDBindPosePlugin::CheckTagReg(BaseContainer *tData, BaseObject *op)
{
	Bool reg = true;
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);

	CHAR b;
	String kb, cdjnr = tData->GetString(T_STR);
	SerialInfo si;
	
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
	
	if(GeGetVersionType() & VERSION_NET) reg = true;
	if(GeGetVersionType() & VERSION_SERVER) reg = true;
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = true;
#endif
	
	tData->SetBool(T_REG,reg);
	if(!reg)
	{
		if(!tData->GetBool(T_SET))
		{
			tData->SetBool(BND_RIG_MIRROR+T_LST,tData->GetBool(BND_RIG_MIRROR));
			tData->SetLong(BND_MIRROR_AXIS+T_LST,tData->GetLong(BND_MIRROR_AXIS));
			tData->SetBool(BND_LOCAL_CENTER+T_LST,tData->GetBool(BND_LOCAL_CENTER));
			
			tData->SetLink(T_OID,op);
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDBindPosePlugin::CheckTagAssign(BaseDocument *doc, BaseContainer *tData, BaseObject *op)
{
	Bool enable = true;
	
	if(!tData->GetBool(T_REG))
	{
		if(!tData->GetBool(T_SET)) enable = false;
		
		Bool tagMoved = false;
		if(op != tData->GetObjectLink(T_OID,doc))
		{
			BaseObject *tOp = tData->GetObjectLink(T_OID,doc);
			if(tOp)
			{
				if(tOp->GetDocument())
				{
					tagMoved = true;
					tData->SetBool(T_MOV,true);
				}
			}
			if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_OID,op);
		}
		else tData->SetBool(T_MOV,false);
		if(tagMoved || tData->GetBool(T_MOV)) enable = false;
		
		tData->SetBool(BND_RIG_MIRROR,tData->GetBool(BND_RIG_MIRROR+T_LST));
		tData->SetLong(BND_MIRROR_AXIS,tData->GetLong(BND_MIRROR_AXIS+T_LST));
		tData->SetBool(BND_LOCAL_CENTER,tData->GetBool(BND_LOCAL_CENTER+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDBindPosePlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	LONG i, pCnt = tData->GetLong(BND_POSE_COUNT);
	LONG oCnt = tData->GetLong(BND_OBJECT_COUNT);
	switch (id[0].id)
	{
		case BND_RIG_MIRROR:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case BND_MIRROR_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(BND_RIG_MIRROR)) return false;
				else return true;
			}
		case BND_LOCAL_CENTER:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case BND_ASSIGN_MIRROR:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case BND_OBJECT_LIST:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case BND_POSE_ADD:
			if(oCnt < 1) return false;	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(BND_POSE_IS_SET);
		case BND_POSE_SUB:	
			if(oCnt < 1) return false;	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(BND_POSE_IS_SET);
	}

	for(i=0; i<pCnt;i++)
	{
		if(id[0].id == BND_POSE_SET+i)
		{
			if(oCnt < 1) return false;
			if(!tData->GetBool(T_REG)) return false;
			else 
			{
				if(!tData->GetBool(BND_POSE_IS_SET+i)) return true;
				else return false;
			}
		}
		else if(id[0].id == BND_POSE_EDIT+i)
		{
			if(oCnt < 1) return false;
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(BND_POSE_IS_SET+i);
		}
		else if(id[0].id == BND_POSE_RESTORE+i)
		{
			if(oCnt < 1) return false;
			return tData->GetBool(BND_POSE_IS_SET+i);
		}
		else if(id[0].id == BND_POSE_MIRROR+i && i > 0)
		{
			if(oCnt < 1) return false;
			if(tData->GetBool(BND_POSE_IS_SET+i) && tData->GetBool(BND_RIG_MIRROR) && tData->GetBool(BND_POSE_MIRROR_SET)) return true;
			else return false;
		}
	}
	return true;
}

Bool RegisterCDBindPosePlugin(void)
{
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
	
	if(GeGetVersionType() & VERSION_NET) reg = true;
	if(GeGetVersionType() & VERSION_SERVER) reg = true;
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = true;
#endif
	
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE;
	
	// decide by name ifthe plugin shall be registered - just for user convenience    
	String name=GeLoadString(IDS_CDBINDPOSE); if(!name.Content()) return true;

	return CDRegisterTagPlugin(ID_CDBINDPOSETAG,name,info,CDBindPosePlugin::Alloc,"tCDBindPose","CDBindPose.tif",1);
}

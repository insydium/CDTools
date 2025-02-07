//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"
#include "c4d_graphview.h"

//#include "CDCompatibility.h"
#include "CDJoint.h"
#include "CDJointSkin.h"
#include "CDSkinRefTag.h"
#include "CDSkinTag.h"
//#include "CDMatrixArray.h"
#include "CDArray.h"

#include "oCDJoint.h"
#include "tCDMechIK.h"

static AtomArray *objectList = NULL;
static AtomArray *mirrorList = NULL;
static AtomArray *objTagList = NULL;
static AtomArray *mirTagList = NULL;

// CD Hand Tag
enum
{	
	FINGER_COUNT				= 1020,
	
	LEFT_HAND					= 1031,
	
	TAG_LINK					= 3000
};

// CD Limb IK & CD Quadleg IK
enum
{
	IK_POLE_AXIS				= 10006,
		IK_POLE_Y				= 10007,
		IK_POLE_X				= 10008,
	
		IK_POLE_NX				= 10019,
		IK_POLE_NY				= 10020
};


class CDJointMirrorDialog : public GeDialog
{
private:
	CDJSOptionsUA 	ua;
	CDArray<Matrix>		mArray;
	
	
	Bool DoMirror(LONG axis, LONG gl);
	void CheckIKTagsSolverDirection(BaseObject *op, BaseObject *mop, BaseTag *tag, BaseTag *mTag);
	void MirrorTagLinkedObjects(BaseDocument *doc, LONG ind, LONG axis);
	
	Bool CopySkinning(BaseDocument *doc, BaseTag *tag, BaseTag *mTag);
	void MirrorSkinReference(BaseTag *skrTag, LONG axis);
	void MirrorMorphReference(BaseTag *mrTag, LONG axis);
	Bool MirrorMechIK(BaseTag *tag, BaseTag *mrTag);
	
	void MirrorPoints(BaseObject *op, LONG axis);
	Bool MirrorChildren(BaseDocument* doc, BaseObject *mpar, BaseObject *op, Matrix prM, LONG axis);

public:
	String oldName, newName;

	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};


Bool CDJointMirrorDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDJMIRROR));
		
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
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_MIRROR_AXIS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_GLOBAL_LOCAL,BFH_CENTER,2,1);
					AddChild(IDC_GLOBAL_LOCAL, 0, GeLoadString(M_GLOBAL));
					AddChild(IDC_GLOBAL_LOCAL, 1, GeLoadString(M_LOCAL));

				AddRadioGroup(IDC_MIRROR_AXIS,BFH_CENTER,3,1);
					AddChild(IDC_MIRROR_AXIS, 0, GeLoadString(M_X));
					AddChild(IDC_MIRROR_AXIS, 1, GeLoadString(M_Y));
					AddChild(IDC_MIRROR_AXIS, 2, GeLoadString(M_Z));
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,4,0,GeLoadString(D_MIRROR_RENAME),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_OLD_NAME),0);
				AddEditText(IDC_OLD_NAME,BFH_CENTER,70,0);
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_NEW_NAME),0);
				AddEditText(IDC_NEW_NAME,BFH_CENTER,70,0);
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
			{
				GroupBorderNoTitle(BORDER_NONE);
				GroupBorderSpace(100,4,100,4);
				
				AddButton(GADGET_MIRROR,BFH_CENTER,0,0,GeLoadString(D_MIRROR));
			}
			GroupEnd();
		}
		GroupEnd();
	}

	return res;
}

Bool CDJointMirrorDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDJOINTMIRRORPLUGIN);
	if(!bc)
	{
		SetLong(IDC_MIRROR_AXIS,0);
		SetLong(IDC_GLOBAL_LOCAL,0);
		SetString(IDC_OLD_NAME,oldName);
		SetString(IDC_NEW_NAME,newName);
	}
	else
	{
		SetLong(IDC_MIRROR_AXIS,bc->GetLong(IDC_MIRROR_AXIS));
		SetLong(IDC_GLOBAL_LOCAL,bc->GetLong(IDC_GLOBAL_LOCAL));
		SetString(IDC_OLD_NAME,bc->GetString(IDC_OLD_NAME));
		SetString(IDC_NEW_NAME,bc->GetString(IDC_NEW_NAME));
	}

	return true;
}

Bool CDJointMirrorDialog::Command(LONG id,const BaseContainer &msg)
{
	LONG a, gl;
	
	GetLong(IDC_MIRROR_AXIS,a);
	GetLong(IDC_GLOBAL_LOCAL,gl);
	GetString(IDC_OLD_NAME,oldName);
	GetString(IDC_NEW_NAME,newName);
	
	BaseContainer bc;
	bc.SetLong(IDC_MIRROR_AXIS,a);
	bc.SetLong(IDC_GLOBAL_LOCAL,gl);
	bc.SetString(IDC_OLD_NAME,oldName);
	bc.SetString(IDC_NEW_NAME,newName);
	SetWorldPluginData(ID_CDJOINTMIRRORPLUGIN,bc,false);

	switch (id)
	{
		case GADGET_MIRROR:
			StopAllThreads(); // so the document can be safely modified
			DoMirror(a,gl);
			break;
	}

	return true;
}

void CDJointMirrorDialog::MirrorPoints(BaseObject *op, LONG axis)
{
	// mirror points
	Vector *padr = GetPointArray(op);
	if(padr)
	{
		LONG i, pCnt = ToPoint(op)->GetPointCount();
		for(i=0; i<pCnt; i++)
		{
			switch(axis)
			{
				case 0:
					padr[i].x *= -1;
					break;
				case 1:
					padr[i].y *= -1;
					break;
				case 2:
					padr[i].z *= -1;
					break;
			}
		}
	
		// mirror spline tangents
		if(op->IsInstanceOf(Ospline))
		{
			BaseContainer *opData = op->GetDataInstance();
			if(opData)
			{
				if(opData->GetLong(SPLINEOBJECT_TYPE) == SPLINEOBJECT_TYPE_BEZIER)
				{
				#if API_VERSION < 9800
					Tangent *tngt = ToSpline(op)->GetTangent();
				#else
					Tangent *tngt = ToSpline(op)->GetTangentW();
				#endif
					
					if(tngt)
					{
						for(i=0; i<pCnt; i++)
						{
							switch(axis)
							{
								case 0:
									tngt[i].vl.x *= -1;
									tngt[i].vr.x *= -1;
									break;
								case 1:
									tngt[i].vl.y *= -1;
									tngt[i].vr.y *= -1;
									break;
								case 2:
									tngt[i].vl.z *= -1;
									tngt[i].vr.z *= -1;
									break;
							}
						}
					}
				}
			}
		}
		
		// flip polygon normals
		if(IsValidPolygonObject(op))
		{
			if(axis == 2)
			{
				Matrix oldM = op->GetMg();
				Matrix newM = oldM * RotAxisToMatrix(Vector(0,1,0),pi);
				RecalculatePoints(op,newM,oldM);
			}
			
			CPolygon *vadr = GetPolygonArray(op);
			if(vadr)
			{
				Bool hnw = false;
				HNData hnwData;
				HNWeightTag *hnwTag = (HNWeightTag*)op->GetTag(Tsds);
				if(hnwTag)
				{
					hnw = hnwTag->GetTagData(&hnwData);
				}
				
				LONG plyCnt = ToPoly(op)->GetPolygonCount();
				for(i=0; i<plyCnt; i++)
				{
					CPolygon ply = vadr[i];
					PolyWeight plyWt;
					
					if(ply.c == ply.d)
					{
						vadr[i].a = ply.c;
						vadr[i].b = ply.b;
						vadr[i].c = ply.a;
						vadr[i].d = ply.a;
						
						if(hnwTag && hnw)
						{
							plyWt.a = (*hnwData.polyweight)[i].a;
							plyWt.b = (*hnwData.polyweight)[i].b;
							plyWt.c = (*hnwData.polyweight)[i].c;
							plyWt.d = (*hnwData.polyweight)[i].d;
							
							(*hnwData.polyweight)[i].a = plyWt.c;
							(*hnwData.polyweight)[i].b = plyWt.b;
							(*hnwData.polyweight)[i].c = plyWt.a;
							(*hnwData.polyweight)[i].d = plyWt.a;
						}
					}
					else
					{
						vadr[i].a = ply.d;
						vadr[i].b = ply.c;
						vadr[i].c = ply.b;
						vadr[i].d = ply.a;
						
						if(hnwTag && hnw)
						{
							plyWt.a = (*hnwData.polyweight)[i].a;
							plyWt.b = (*hnwData.polyweight)[i].b;
							plyWt.c = (*hnwData.polyweight)[i].c;
							plyWt.d = (*hnwData.polyweight)[i].d;

							(*hnwData.polyweight)[i].a = plyWt.d;
							(*hnwData.polyweight)[i].b = plyWt.c;
							(*hnwData.polyweight)[i].c = plyWt.b;
							(*hnwData.polyweight)[i].d = plyWt.a;
						}
					}
				}
			
				// mirror UVW
				if(op->GetTag(Tuvw))
				{
					BaseTag *tag = op->GetFirstTag();
					while(tag)
					{
						if(tag->IsInstanceOf(Tuvw))
						{
							UVWTag *uvTag = static_cast<UVWTag*>(tag);
							for(i=0; i<plyCnt; i++)
							{
								UVWStruct muv, uv;
                                CDGetUVWStruct(uvTag,uv,i);
                                
								CPolygon ply = vadr[i];
								if(ply.c == ply.d)
								{
									muv.a = uv.c;
									muv.b = uv.b;
									muv.c = uv.a;
								}
								else
								{
									muv.a = uv.d;
									muv.b = uv.c;
									muv.c = uv.b;
									muv.d = uv.a;
								}
                                CDSetUVWStruct(uvTag,muv,i);
							}
						}
						tag = tag->GetNext();
					}
				}
			}
		}
	}
	
	op->Message(MSG_UPDATE);
}

void CDJointMirrorDialog::MirrorTagLinkedObjects(BaseDocument *doc, LONG ind, LONG axis)
{
	BaseObject *op = static_cast<BaseObject*>(objectList->GetIndex(ind));
	BaseObject *mop = static_cast<BaseObject*>(mirrorList->GetIndex(ind));
	Matrix prM = mArray[ind];

	if(op && mop)
	{
		BaseTag *mTag = mop->GetFirstTag();
		while(mTag)
		{
			BaseContainer *tData = mTag->GetDataInstance();
			if(mTag->GetInfo() & TAG_VISIBLE)
			{
				if(mTag->GetType() == Texpresso)
				{
					XPressoTag *xTag = static_cast<XPressoTag*>(mTag);
					GvNode *gvN = xTag->GetNodeMaster()->GetRoot()->GetDown();
					BaseContainer *gvData = NULL;
					
					while(gvN)
					{
						gvData = gvN->GetOpContainerInstance();
						BaseList2D *list = gvData->GetLink(1000,doc);
						if(list)
						{
							if(list->IsInstanceOf(Obase))
							{
								BaseObject *targOp = static_cast<BaseObject*>(list);
								if(targOp)
								{
									BaseObject *mlnk = NULL;
												
									if(objectList->Find(targOp) == NOTOK)
									{
										if(!IsParentObject(mop,targOp) && !IsParentObject(op,targOp))
										{
											BaseObject *mpr = NULL;
											
											objectList->Append(targOp);
											LONG sPos;
											String opName = targOp->GetName();
											if(opName.FindFirst(oldName,&sPos,0))
											{
												opName.Delete(sPos,oldName.GetLength());
												opName.Insert(sPos,newName);
											}
											mlnk = (BaseObject*)CDGetClone(targOp,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_ANIMATION|CD_COPYFLAGS_PRIVATE_NO_INTERNALS,NULL);
											
											mlnk->SetName(opName);
											
											if(objectList->Find(targOp->GetUp()) > NOTOK)
											{
												mpr = static_cast<BaseObject*>(mirrorList->GetIndex(objectList->Find(targOp->GetUp())));
											}
											
											doc->InsertObject(mlnk,mpr,NULL,false);
											mlnk->Message(MSG_MENUPREPARE);
											CDAddUndo(doc,CD_UNDO_NEW,mlnk);
											
											Matrix mirM = GetMirrorMatrix(targOp->GetMg(),prM,axis);
											
											mlnk->SetMg(mirM);
											CDSetScale(mlnk,CDGetScale(targOp));
										}
									}
									else
									{
										mlnk = static_cast<BaseObject*>(mirrorList->GetIndex(objectList->Find(targOp)));
									}
									
									gvData->SetLink(1000,mlnk);
								}
							}
							else if(list->IsInstanceOf(Tbase))
							{
								BaseTag *trgTag = static_cast<BaseTag*>(list);
								if(trgTag)
								{
									BaseTag *mTag = NULL;
									if(objTagList->Find(trgTag) > NOTOK)
									{
										mTag = static_cast<BaseTag*>(mirTagList->GetIndex(objTagList->Find(trgTag)));
										if(mTag) gvData->SetLink(1000,mTag);
									}
								}
							}
						}
						
						gvN = gvN->GetNext();
					}
					
				}
				else
				{
					AutoAlloc<Description> desc;
					CDGetDescription(mTag, desc, CD_DESCFLAGS_DESC_0);
					void* h = desc->BrowseInit();
					const BaseContainer *bc = NULL;
					DescID id, groupid, scope;
					LONG i;
					while(desc->GetNext(h, &bc, id, groupid))
					{
						for (i=0; i<id.GetDepth(); ++i)
						{
							if(id[i].id > 999)
							{
								if(id[i].dtype == DTYPE_BASELISTLINK)
								{
									LONG linkID = id[i].id;
									BaseList2D *list = tData->GetLink(linkID,doc);
									if(list)
									{
										if(list->IsInstanceOf(Obase))
										{
											BaseObject *targOp = static_cast<BaseObject*>(list);
											if(targOp)
											{
												BaseObject *mlnk = NULL;
												
												if(objectList->Find(targOp) == NOTOK)
												{
													if(!IsParentObject(mop,targOp) && !IsParentObject(op,targOp))
													{
														BaseObject *mpr = NULL;
														
														objectList->Append(targOp);
														LONG sPos;
														String opName = targOp->GetName();
														if(opName.FindFirst(oldName,&sPos,0))
														{
															opName.Delete(sPos,oldName.GetLength());
															opName.Insert(sPos,newName);
														}
														mlnk = (BaseObject*)CDGetClone(targOp,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_ANIMATION|CD_COPYFLAGS_PRIVATE_NO_INTERNALS,NULL);
														
														mlnk->SetName(opName);
														
														if(objectList->Find(targOp->GetUp()) > NOTOK)
														{
															mpr = static_cast<BaseObject*>(mirrorList->GetIndex(objectList->Find(targOp->GetUp())));
														}
														
														doc->InsertObject(mlnk,mpr,NULL,false);
														mlnk->Message(MSG_MENUPREPARE);
														CDAddUndo(doc,CD_UNDO_NEW,mlnk);
														
														Matrix mirM = GetMirrorMatrix(targOp->GetMg(),prM,axis);
														
														mlnk->SetMg(mirM);
														CDSetScale(mlnk,CDGetScale(targOp));
													}
												}
												else
												{
													mlnk = static_cast<BaseObject*>(mirrorList->GetIndex(objectList->Find(targOp)));
												}
												
												tData->SetLink(linkID,mlnk);
											}
										}
										else if(list->IsInstanceOf(Tbase))
										{
											BaseTag *trgTag = static_cast<BaseTag*>(list);
											if(trgTag)
											{
												BaseTag *linkTag = NULL;
												if(objTagList->Find(trgTag) > NOTOK)
												{
													linkTag = static_cast<BaseTag*>(mirTagList->GetIndex(objTagList->Find(trgTag)));
													if(linkTag) tData->SetLink(linkID,linkTag);
												}
											}
										}
									}
								}
							}
						}
					}
					desc->BrowseFree(h);
					
					if(mTag->GetType() == ID_CDHANDPLUGIN)
					{
						BaseTag *tag = op->GetTag(ID_CDHANDPLUGIN);
						if(tag)
						{
							BaseContainer *tData = tag->GetDataInstance();
							BaseContainer *mtData = mTag->GetDataInstance();
							if(tData && mtData)
							{
								if(!tData->GetBool(LEFT_HAND)) mtData->SetBool(LEFT_HAND,true);
								else mtData->SetBool(LEFT_HAND,false);
							}
						}
					}
				}
			}
			mTag = mTag->GetNext();
		}
	}
}

Bool CDJointMirrorDialog::CopySkinning(BaseDocument *doc, BaseTag *tag, BaseTag *mTag)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseContainer *mData = mTag->GetDataInstance(); if(!mData) return false;
	
	BaseObject *mop = mTag->GetObject();
	mData->SetLink(SKN_DEST_ID,mop);
	
	if(tData->GetBool(SKN_BOUND))
	{
		SpecialEventAdd(ID_CDJOINTMIRRORPLUGIN);
		return false;
	}
	LONG i, jCnt = tData->GetLong(SKN_J_COUNT);
	for(i=0; i<jCnt; i++)
	{
		BaseObject *jnt = tData->GetObjectLink(SKN_J_LINK+i,doc);
		if(jnt)
		{
			BaseObject *mJnt = static_cast<BaseObject*>(mirrorList->GetIndex(objectList->Find(jnt)));
			if(mJnt) mData->SetLink(SKN_J_LINK+i,mJnt);
		}
	}
	
	return true;
}

void CDJointMirrorDialog::MirrorSkinReference(BaseTag *skrTag, LONG axis)
{
	CDMRefPoint *skinRef = NULL;
	LONG i, rCnt;
	
	skinRef = CDGetSkinReference(skrTag);
	if(skinRef)
	{
		BaseContainer *skrData = skrTag->GetDataInstance();
		if(skrData)
		{
			rCnt = skrData->GetLong(SKR_S_POINT_COUNT);
			for(i=0; i<rCnt; i++)
			{
				Vector rPt = skinRef[i].GetVector();
				switch(axis)
				{
					case 0:
						rPt.x *= -1;
						break;
					case 1:
						rPt.y *= -1;
						break;
					case 2:
						rPt.z *= -1;
						break;
				}
				skinRef[i].SetVector(rPt);
			}
		}
	}
}
	
void CDJointMirrorDialog::MirrorMorphReference(BaseTag *mrTag, LONG axis)
{
	BaseContainer *mrData = mrTag->GetDataInstance();
	if(mrData)
	{
		if(mrData->GetBool(MR_REFERENCE_NEW))
		{
			LONG i, rCnt;
			CDMRefPoint *mRef = NULL;
			
			mRef = CDGetMorphCache(mrTag);
			if(mRef)
			{
				rCnt = mrData->GetLong(MR_POINT_COUNT);
				for(i=0; i<rCnt; i++)
				{
					Vector rPt = mRef[i].GetVector();
					switch(axis)
					{
						case 0:
							rPt.x *= -1;
							break;
						case 1:
							rPt.y *= -1;
							break;
						case 2:
							rPt.z *= -1;
							break;
					}
					mRef[i].SetVector(rPt);
				}
			}
		}
	}
}

Bool CDJointMirrorDialog::MirrorMechIK(BaseTag *tag, BaseTag *mTag)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseContainer *mtData = mTag->GetDataInstance(); if(!mtData) return false;
	
	if(!tData->GetBool(MCHIK_ROOT_FLIP)) mtData->SetBool(MCHIK_ROOT_FLIP,true);
	else mtData->SetBool(MCHIK_ROOT_FLIP,false);
	
	return true;
}

Bool CDJointMirrorDialog::MirrorChildren(BaseDocument *doc, BaseObject *mpar, BaseObject *op, Matrix prM, LONG axis)
{
	BaseObject *prev = NULL, *mop = NULL; 
	Matrix mirM,opMatrix;
	BaseContainer *opData = NULL, *mopData = NULL;
	Vector theScale, mVec;
	LONG sPos;
	String opName;
	Real xm, ym;

	while(op)
	{
		objectList->Append(op);
		
		opMatrix = op->GetMg();
		theScale = CDGetScale(op);
		
		opName = op->GetName();
		if(opName.FindFirst(oldName,&sPos,0))
		{
			opName.Delete(sPos,oldName.GetLength());
			opName.Insert(sPos,newName);
		}
		
		opData = op->GetDataInstance();
		mop = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_ANIMATION|CD_COPYFLAGS_PRIVATE_NO_INTERNALS,NULL);
		
		mop->SetName(opName);
		doc->InsertObject(mop, mpar, prev,false);
		mop->Message(MSG_MENUPREPARE);
		mirrorList->Append(mop);
		
		mirM = GetMirrorMatrix(opMatrix,prM,axis);
		mop->SetMg(mirM);
		CDSetScale(mop,theScale);
		
		mopData = mop->GetDataInstance();
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			mopData->SetReal(JNT_JOINT_RADIUS,opData->GetReal(JNT_JOINT_RADIUS));
			mopData->SetBool(JNT_CONNECTED,opData->GetBool(JNT_CONNECTED));
			mopData->SetBool(JNT_SHOW_ENVELOPE,opData->GetBool(JNT_SHOW_ENVELOPE));
			mopData->SetVector(JNT_ROOT_OFFSET,opData->GetVector(JNT_ROOT_OFFSET));
			mopData->SetVector(JNT_TIP_OFFSET,opData->GetVector(JNT_TIP_OFFSET));
			mopData->SetBool(JNT_SHOW_PROXY,opData->GetBool(JNT_SHOW_PROXY));
			//Mirror skew parameters
			mVec = VNorm(mirM.off - opMatrix.off);
			if(Abs(VDot(VNorm(mirM.v1),mVec)) < Abs(VDot(VNorm(mirM.v2),mVec)))
			{
				xm = 1;
				ym = -1;
			}
			else
			{
				xm = -1;
				ym = 1;
			}
			mopData->SetReal(JNT_SKEW_RX,opData->GetReal(JNT_SKEW_RX)*xm);
			mopData->SetReal(JNT_SKEW_RY,opData->GetReal(JNT_SKEW_RY)*ym);
			mopData->SetReal(JNT_SKEW_TX,opData->GetReal(JNT_SKEW_TX)*xm);
			mopData->SetReal(JNT_SKEW_TY,opData->GetReal(JNT_SKEW_TY)*ym);
			CDAddUndo(doc,CD_UNDO_NEW,mop);
			
			// Set mirror links
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			opData->SetLink(JNT_JOINT_M,mop);
			mopData->SetLink(JNT_JOINT_M,op);
		
		}
		else
		{
			if(IsValidPointObject(mop))
			{
				MirrorPoints(mop,axis);
			}
		#if API_VERSION < 12000
			opData->CopyTo(mopData, NULL);
		#else
			opData->CopyTo(mopData,COPYFLAGS_0,NULL);
		#endif
		}

		mArray.Append(prM);
		BaseTag *tag = op->GetFirstTag(), *mTag = mop->GetFirstTag();
		while(tag && mTag)
		{
			if(tag->GetInfo() & TAG_VISIBLE) objTagList->Append(tag);
			if(mTag->GetInfo() & TAG_VISIBLE) mirTagList->Append(mTag);
			
			String tagName = tag->GetName();
			if(tagName.FindFirst(oldName,&sPos,0))
			{
				tagName.Delete(sPos,oldName.GetLength());
				tagName.Insert(sPos,newName);
			}
			mTag->SetName(tagName);
			
			if(tag->GetType() == ID_CDSKINREFPLUGIN) MirrorSkinReference(mTag,axis);
			if(tag->GetType() == ID_CDSKINPLUGIN) CopySkinning(doc,tag,mTag);
			if(tag->GetType() == ID_CDMORPHREFPLUGIN) MirrorMorphReference(mTag,axis);
			if(tag->GetType() == ID_CDMECHLIMBPLUGIN) MirrorMechIK(tag,mTag);
			
			CheckIKTagsSolverDirection(op,mop,tag,mTag);
				
			tag = tag->GetNext();
			mTag = mTag->GetNext();
		}
		
		CDAddUndo(doc,CD_UNDO_NEW,mop);
		
		MirrorChildren(doc,mop,op->GetDown(),prM,axis);
		
		prev = mop;
		op = op->GetNext();
	}
	return true;
}

void CDJointMirrorDialog::CheckIKTagsSolverDirection(BaseObject *op, BaseObject *mop, BaseTag *tag, BaseTag *mTag)
{
	//GePrint("CheckIKTagsSolverDirection");
	LONG type = tag->GetType(), mType = mTag->GetType();
	
	if((type == ID_CDLIMBIKPLUGIN && mType == ID_CDLIMBIKPLUGIN) || (type == ID_CDQUADLEGPLUGIN && mType == ID_CDQUADLEGPLUGIN))
	{
		BaseContainer *tData = tag->GetDataInstance(), *mtData = mTag->GetDataInstance();
		if(tData && mtData)
		{
			Matrix opM = op->GetMg(), mopM = mop->GetMg();
			switch(tData->GetLong(IK_POLE_AXIS))
			{
				case IK_POLE_X:
				{
					Real dot = VDot(opM.v2, mopM.v2);
					if(dot < 0.0) mtData->SetLong(IK_POLE_AXIS,IK_POLE_NX);
					break;
				}	
				case IK_POLE_Y:
				{
					Real dot = VDot(opM.v1, mopM.v1);
					if(dot < 0.0) mtData->SetLong(IK_POLE_AXIS,IK_POLE_NY);
					break;
				}	
				case IK_POLE_NX:
				{
					Real dot = VDot(opM.v2, mopM.v2);
					if(dot < 0.0) mtData->SetLong(IK_POLE_AXIS,IK_POLE_X);
					break;
				}	
				case IK_POLE_NY:
				{
					Real dot = VDot(opM.v1, mopM.v1);
					if(dot < 0.0) mtData->SetLong(IK_POLE_AXIS,IK_POLE_Y);
					break;
				}	
			}
		}
	}
}

Bool CDJointMirrorDialog::DoMirror(LONG axis, LONG gl)
{
	//GePrint("CDJointMirrorDialog::DoMirror");
	BaseDocument *doc = GetActiveDocument();
	BaseObject *op = NULL, *mop = NULL;
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	
	LONG i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		// allocate the mirror lists
		objectList = AtomArray::Alloc();
		mirrorList = AtomArray::Alloc();
		objTagList = AtomArray::Alloc();
		mirTagList = AtomArray::Alloc();
		mArray.Init();
		
		doc->StartUndo();
		for(i=0; i<opCnt; i++)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i));
			
			Vector theScale = CDGetScale(op);
			Matrix opMatrix = op->GetMg(), mirM, prM = Matrix();
			
			BaseObject *parent = op->GetUp();
			if(parent)
			{
				if(gl == 1) prM = parent->GetMg();
			}
			
			objectList->Append(op);
			
			LONG sPos;
			String opName = op->GetName();
			if(opName.FindFirst(oldName,&sPos,0))
			{
				opName.Delete(sPos,oldName.GetLength());
				opName.Insert(sPos,newName);
			}
			
			BaseContainer *opData = op->GetDataInstance();
			mop = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_ANIMATION|CD_COPYFLAGS_PRIVATE_NO_INTERNALS,NULL);

			mop->SetName(opName);
			doc->InsertObject(mop, parent, op,false);
			mop->Message(MSG_MENUPREPARE);
			mirrorList->Append(mop);
			
			mirM = GetMirrorMatrix(opMatrix,prM,axis);
			mop->SetMg(mirM);
			CDSetScale(mop,theScale);
			BaseContainer *mopData = mop->GetDataInstance();
			
			if(op->GetType() == ID_CDJOINTOBJECT)
			{
				mopData->SetReal(JNT_JOINT_RADIUS,opData->GetReal(JNT_JOINT_RADIUS));
				mopData->SetBool(JNT_CONNECTED,opData->GetBool(JNT_CONNECTED));
				mopData->SetBool(JNT_SHOW_ENVELOPE,opData->GetBool(JNT_SHOW_ENVELOPE));
				mopData->SetVector(JNT_ROOT_OFFSET,opData->GetVector(JNT_ROOT_OFFSET));
				mopData->SetVector(JNT_TIP_OFFSET,opData->GetVector(JNT_TIP_OFFSET));
				mopData->SetBool(JNT_SHOW_PROXY,opData->GetBool(JNT_SHOW_PROXY));
				//Mirror skew parameters
				Real xm, ym;
				Vector mVec = VNorm(mirM.off - opMatrix.off);
				if(Abs(VDot(VNorm(mirM.v1),mVec)) < Abs(VDot(VNorm(mirM.v2),mVec)))
				{
					xm = 1;
					ym = -1;
				}
				else
				{
					xm = -1;
					ym = 1;
				}
				mopData->SetReal(JNT_SKEW_RX,opData->GetReal(JNT_SKEW_RX)*xm);
				mopData->SetReal(JNT_SKEW_RY,opData->GetReal(JNT_SKEW_RY)*ym);
				mopData->SetReal(JNT_SKEW_TX,opData->GetReal(JNT_SKEW_TX)*xm);
				mopData->SetReal(JNT_SKEW_TY,opData->GetReal(JNT_SKEW_TY)*ym);

				// Set mirror links
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				opData->SetLink(JNT_JOINT_M,mop);
				mopData->SetLink(JNT_JOINT_M,op);
				
			}
			else
			{
			#if API_VERSION < 12000
				opData->CopyTo(mopData, NULL);
			#else
				opData->CopyTo(mopData,COPYFLAGS_0,NULL);
			#endif
				if(IsValidPointObject(mop))
				{
					MirrorPoints(mop,axis);
				}
				
			}
			CDAddUndo(doc,CD_UNDO_NEW,mop);
			
			mArray.Append(prM);
			BaseTag *tag = op->GetFirstTag(), *mTag = mop->GetFirstTag();
			while(tag && mTag)
			{
				if(tag->GetInfo() & TAG_VISIBLE) objTagList->Append(tag);
				if(mTag->GetInfo() & TAG_VISIBLE) mirTagList->Append(mTag);
				
				String tagName = tag->GetName();
				if(tagName.FindFirst(oldName,&sPos,0))
				{
					tagName.Delete(sPos,oldName.GetLength());
					tagName.Insert(sPos,newName);
				}
				mTag->SetName(tagName);
				
				if(tag->GetType() == ID_CDSKINREFPLUGIN) MirrorSkinReference(mTag,axis);
				if(tag->GetType() == ID_CDSKINPLUGIN) CopySkinning(doc,tag,mTag);
				if(tag->GetType() == ID_CDMORPHREFPLUGIN) MirrorMorphReference(mTag,axis);
				if(tag->GetType() == ID_CDMECHLIMBPLUGIN) MirrorMechIK(tag,mTag);
				
				CheckIKTagsSolverDirection(op,mop,tag,mTag);
				
				tag = tag->GetNext();
				mTag = mTag->GetNext();
			}
			
			//Mirror child objects
			MirrorChildren(doc,mop,op->GetDown(),prM,axis);

		}
		
		LONG mCnt = mirrorList->GetCount();
		for(i=0; i<mCnt; i++)
		{
			// mirror tag links
			MirrorTagLinkedObjects(doc,i,axis);
		}

		doc->SetActiveObject(mop);
		doc->EndUndo();
		
		AtomArray::Free(objectList);
		AtomArray::Free(mirrorList);
		AtomArray::Free(objTagList);
		AtomArray::Free(mirTagList);
		mArray.Free();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDJointMirrorPlugin : public CommandData
{
	private:
		CDJointMirrorDialog dlg;

	public:

		virtual Bool Execute(BaseDocument *doc)
		{
		#if API_VERSION < 12000
			return dlg.Open(true,ID_CDJOINTMIRRORPLUGIN,-1,-1);
		#else
			return dlg.Open(DLG_TYPE_ASYNC,ID_CDJOINTMIRRORPLUGIN,-1,-1);
		#endif
		}

		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDJOINTMIRRORPLUGIN,0,secret);
		}
};

class CDJointMirrorPluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDJointMirrorPlugin(void)
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
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDJMIRROR); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDJOINTMIRRORPLUGIN,name,PLUGINFLAG_HIDE,"CDJointMirror.tif","CD Joint Mirror",CDDataAllocator(CDJointMirrorPluginR));
	else return CDRegisterCommandPlugin(ID_CDJOINTMIRRORPLUGIN,name,0,"CDJointMirror.tif","CD Joint Mirror",CDDataAllocator(CDJointMirrorPlugin));
}

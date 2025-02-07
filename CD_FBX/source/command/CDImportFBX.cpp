//	Cactus Dan's FBX Import/Export plugin
//	Copyright 2011 by Cactus Dan Libisch
	
// c4d includes
#include "c4d.h"
#include "c4d_symbols.h"
#include "olight.h"
#include "lib_ngon.h"
#include "tpolygonselection.h"
#if API_VERSION > 9799
	#include "ckvalue.h"
#endif

// fbx sdk includes
#include "CommonFbx.h"

// CD includes
#include "CDDebug.h"
#include "CDFBX.h"
#include "ObjectNode.h"
#include "TakeNode.h"
#include "ShapeNode.h"
#include "NgonUVW.h"

static ObjectNodeArray			objectList;
static CDArray<NgonUVW>			ngonList;
static CDArray<ShapeNode>		shapeList;
static CDArray<TakeNode>		takeList;

// static functions
static Real InchToMillimeter(Real in)
{
	return in * 25.4;
}

static LONG GetTakeLength(int ind)
{
	LONG tcnt = takeList.Size();
	
	if(tcnt < 1) return 0;
	if(ind >= tcnt) return 0;
	
	TakeNode *tlist = takeList.Array();
	
	LONG start = tlist[ind].start;
	LONG end = tlist[ind].end;
	
	return end-start;
}

static LONG GetTakeStart(int ind)
{
	LONG tcnt = takeList.Size();
	
	if(tcnt < 1) return 0;
	if(ind >= tcnt) return 0;
	
	TakeNode *tlist = takeList.Array();
	
	LONG start = tlist[ind].start;
	
	return start;
}

static LONG GetTakeEnd(int ind)
{
	LONG tcnt = takeList.Size();
	
	if(tcnt < 1) return 0;
	if(ind >= tcnt) return 0;
	
	TakeNode *tlist = takeList.Array();
	
	LONG end = tlist[ind].end;
	
	return end;
}

static Vector GetTranlationOffset(KFbxNode *kNode, LONG handed)
{
	Vector offset;
	
	KFbxVector4 lTmpVector = kNode->GetRotationPivot(KFbxNode::eSOURCE_SET);
	
	offset.x = Real(lTmpVector[0]);
	offset.y = Real(lTmpVector[1]);
	if(handed == 1)
		offset.z = Real(lTmpVector[2])*-1;
	else
		offset.z = Real(lTmpVector[2]);
	
	return offset;
}

static void BreakPhongShading(void)
{
#if API_VERSION < 9800
	CallCommand(16718);// Break Phong Shading command R9
#else
	CallCommand(16716);// Break Phong Shading command R10+
#endif
}



class CDFbxImportDialog : public GeModalDialog
{
private:
	CDFbxOptionsUA 			ua;
	
	void DoEnable(void);
	
public:
	Real	scale;
	LONG	take, start, offset, end, nlod;
	Bool	joints, meshes, cameras, lights, animations, markers, morphs, nurbs, nulls;
	Bool	pla, freeze, smoothing, materials, textures, userdata, curves, splines, constraints;
	String	vendorName, appName, versionName;
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
	
};

Bool CDFbxImportDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDFBXIMPRT));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDFBX_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDFBX_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDS_FBX_FILE_INFO),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(10,6,8,6);
				
				AddStaticText(0,BFH_LEFT,0,0,"Vendor:  "+vendorName,0);
				AddStaticText(0,BFH_LEFT,0,0,"Application:  "+appName,0);
				AddStaticText(0,BFH_LEFT,0,0,"Version:  "+versionName,0);
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDC_IMPORT_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					AddCheckbox(IDC_IMPORT_JOINT,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_JOINT));
					
					AddCheckbox(IDC_IMPORT_MESH,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_MESH));
					AddCheckbox(IDC_IMPORT_MORPH,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_MORPH));
				
					GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					{
						GroupBorderSpace(0,0,0,4);
						
						AddCheckbox(IDC_IMPORT_NURBS,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_NURBS));
						AddComboBox(IDC_NURBS_SURFACE_LOD,BFH_CENTER);
							AddChild(IDC_NURBS_SURFACE_LOD, 0, GeLoadString(IDS_NURBS_LOW));
							AddChild(IDC_NURBS_SURFACE_LOD, 1, GeLoadString(IDS_NURBS_MEDIUM));
							AddChild(IDC_NURBS_SURFACE_LOD, 2, GeLoadString(IDS_NURBS_HIGH));
					}
					GroupEnd();
					
					AddCheckbox(IDC_IMPORT_CAMERA,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_CAMERA));
					AddCheckbox(IDC_IMPORT_LIGHT,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_LIGHT));
					AddCheckbox(IDC_IMPORT_MARKER,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_MARKER));
					AddCheckbox(IDC_IMPORT_NULL,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_NULL));
					AddCheckbox(IDS_IMPORT_SPLINE,BFH_LEFT,0,0,GeLoadString(IDS_IMPORT_SPLINE));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					GroupBorderSpace(0,8,0,0);
					
					AddCheckbox(IDS_IMPORT_SMOOTHING,BFH_LEFT,0,0,GeLoadString(IDS_IMPORT_SMOOTHING));
					AddCheckbox(IDS_IMPORT_MATERIALS,BFH_LEFT,0,0,GeLoadString(IDS_IMPORT_MATERIALS));
					AddCheckbox(IDS_IMPORT_TEXTURES,BFH_LEFT,0,0,GeLoadString(IDS_IMPORT_TEXTURES));
					AddCheckbox(IDS_IMPORT_USER_DATA,BFH_LEFT,0,0,GeLoadString(IDS_IMPORT_USER_DATA));
					AddCheckbox(IDS_IMPORT_CONSTRAINTS,BFH_LEFT,0,0,GeLoadString(IDS_IMPORT_CONSTRAINTS));
				}
				GroupEnd();

				
				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(0,8,0,0);
					
					AddCheckbox(IDC_IMPORT_ANIMATION,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_ANIMATION));
					AddCheckbox(IDS_IMPORT_CURVES,BFH_LEFT,0,0,GeLoadString(IDS_IMPORT_CURVES));
					AddCheckbox(IDC_IMPORT_PLA,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_PLA));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(0,0,0,8);
					
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_TAKE),0);
					AddComboBox(IDC_TAKE,BFH_CENTER);
					
					TakeNode *tlist = takeList.Array();
					for(int i=0; i<takeList.Size(); i++)
					{
						String str = tlist[i].name;
						AddChild(IDC_TAKE, i, str);
						take = i;
					}
				}
				GroupEnd();

				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_START),0);
					AddEditNumberArrows(IDC_IMPORT_START,BFH_LEFT);
				}
				GroupEnd();
				
				GroupBegin(0,BFH_SCALEFIT,4,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_OFFSET),0);
					AddEditNumberArrows(IDC_IMPORT_OFFSET,BFH_LEFT);
					
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_END),0);
					AddEditNumberArrows(IDC_IMPORT_END,BFH_LEFT);
				}
				GroupEnd();
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
			{
				GroupBorderNoTitle(BORDER_NONE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_IMPORT_SCALE),0);
					AddEditNumberArrows(IDC_IMPORT_SCALE,BFH_LEFT);
				}
				GroupEnd();
				AddCheckbox(IDS_FREEZE_SCALE,BFH_LEFT,0,0,GeLoadString(IDS_FREEZE_SCALE));
			}
			GroupEnd();
		}
		GroupEnd();

		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}
	
	return res;
}

Bool CDFbxImportDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDFBXIMPORTER);
	if(!bc)
	{
		SetBool(IDC_IMPORT_JOINT,true);
		SetBool(IDC_IMPORT_MESH,true);
		SetBool(IDC_IMPORT_MORPH,true);
		SetBool(IDC_IMPORT_NURBS,true);
		SetLong(IDC_NURBS_SURFACE_LOD,1);
		
		SetBool(IDC_IMPORT_CAMERA,true);
		SetBool(IDC_IMPORT_LIGHT,true);
		SetBool(IDC_IMPORT_MARKER,true);
		SetBool(IDC_IMPORT_NULL,false);
		SetBool(IDS_IMPORT_SPLINE,false);
		
		SetBool(IDS_IMPORT_SMOOTHING,true);
		SetBool(IDS_IMPORT_MATERIALS,true);
		SetBool(IDS_IMPORT_TEXTURES,true);
		SetBool(IDS_IMPORT_USER_DATA,true);
		SetBool(IDS_IMPORT_CONSTRAINTS,true);
		
		SetBool(IDC_IMPORT_ANIMATION,true);
		SetBool(IDS_IMPORT_CURVES,false);
		SetBool(IDC_IMPORT_PLA,true);
		
		SetReal(IDC_IMPORT_SCALE,1.0,0.001,1000.0,1.0,FORMAT_REAL);
		SetBool(IDS_FREEZE_SCALE,true);
	}
	else
	{
		SetBool(IDC_IMPORT_JOINT,bc->GetBool(IDC_IMPORT_JOINT));
		SetBool(IDC_IMPORT_MESH,bc->GetBool(IDC_IMPORT_MESH));
		SetBool(IDC_IMPORT_MORPH,bc->GetBool(IDC_IMPORT_MORPH));
		SetBool(IDC_IMPORT_NURBS,bc->GetBool(IDC_IMPORT_NURBS));
		SetLong(IDC_NURBS_SURFACE_LOD,bc->GetLong(IDC_NURBS_SURFACE_LOD));
		
		SetBool(IDC_IMPORT_CAMERA,bc->GetBool(IDC_IMPORT_CAMERA));
		SetBool(IDC_IMPORT_LIGHT,bc->GetBool(IDC_IMPORT_LIGHT));
		SetBool(IDC_IMPORT_MARKER,bc->GetBool(IDC_IMPORT_MARKER));
		SetBool(IDC_IMPORT_NULL,bc->GetBool(IDC_IMPORT_NULL));
		SetBool(IDS_IMPORT_SPLINE,bc->GetBool(IDS_IMPORT_SPLINE));
		
		SetBool(IDS_IMPORT_SMOOTHING,bc->GetBool(IDS_IMPORT_SMOOTHING));
		SetBool(IDS_IMPORT_MATERIALS,bc->GetBool(IDS_IMPORT_MATERIALS));
		SetBool(IDS_IMPORT_TEXTURES,bc->GetBool(IDS_IMPORT_TEXTURES));
		SetBool(IDS_IMPORT_USER_DATA,bc->GetBool(IDS_IMPORT_USER_DATA));
		SetBool(IDS_IMPORT_CONSTRAINTS,bc->GetBool(IDS_IMPORT_CONSTRAINTS));
		
		SetBool(IDC_IMPORT_ANIMATION,bc->GetBool(IDC_IMPORT_ANIMATION));
		SetBool(IDS_IMPORT_CURVES,bc->GetBool(IDS_IMPORT_CURVES));
		SetBool(IDC_IMPORT_PLA,bc->GetBool(IDC_IMPORT_PLA));
		
		SetReal(IDC_IMPORT_SCALE,bc->GetReal(IDC_IMPORT_SCALE),0.001,1000.0,1.0,FORMAT_REAL);
		SetBool(IDS_FREEZE_SCALE,bc->GetBool(IDS_FREEZE_SCALE));
	}
	
	// set the take parameters
	SetLong(IDC_TAKE,take);
	SetLong(IDC_IMPORT_START,GetTakeStart(take));
	SetLong(IDC_IMPORT_OFFSET,0);
	SetLong(IDC_IMPORT_END,GetTakeEnd(take));
	
	DoEnable();

	return true;
}

Bool CDFbxImportDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDC_TAKE,take);
	GetLong(IDC_IMPORT_START,start);
	GetLong(IDC_IMPORT_OFFSET,offset);
	GetLong(IDC_IMPORT_END,end);
	
	GetBool(IDC_IMPORT_JOINT,joints);
	GetBool(IDC_IMPORT_MESH,meshes);
	GetBool(IDC_IMPORT_MORPH,morphs);
	GetBool(IDC_IMPORT_NURBS,nurbs);
	GetLong(IDC_NURBS_SURFACE_LOD,nlod);
	
	GetBool(IDC_IMPORT_CAMERA,cameras);
	GetBool(IDC_IMPORT_LIGHT,lights);
	GetBool(IDC_IMPORT_MARKER,markers);
	GetBool(IDC_IMPORT_NULL,nulls);
	GetBool(IDS_IMPORT_SPLINE,splines);
	
	GetBool(IDS_IMPORT_SMOOTHING,smoothing);
	GetBool(IDS_IMPORT_MATERIALS,materials);
	GetBool(IDS_IMPORT_TEXTURES,textures);
	GetBool(IDS_IMPORT_USER_DATA,userdata);
	GetBool(IDS_IMPORT_CONSTRAINTS,constraints);
	
	GetBool(IDC_IMPORT_ANIMATION,animations);
	GetBool(IDS_IMPORT_CURVES,curves);
	GetBool(IDC_IMPORT_PLA,pla);
	
	GetReal(IDC_IMPORT_SCALE,scale);
	GetBool(IDS_FREEZE_SCALE,freeze);
	
	BaseContainer bc;
	bc.SetBool(IDC_IMPORT_JOINT,joints);
	bc.SetBool(IDC_IMPORT_MESH,meshes);
	bc.SetBool(IDC_IMPORT_MORPH,morphs);
	bc.SetBool(IDC_IMPORT_NURBS,nurbs);
	bc.SetLong(IDC_NURBS_SURFACE_LOD,nlod);
	
	bc.SetBool(IDC_IMPORT_CAMERA,cameras);
	bc.SetBool(IDC_IMPORT_LIGHT,lights);
	bc.SetBool(IDC_IMPORT_MARKER,markers);
	bc.SetBool(IDC_IMPORT_NULL,nulls);
	bc.SetBool(IDS_IMPORT_SPLINE,splines);
	
	bc.SetBool(IDS_IMPORT_SMOOTHING,smoothing);
	bc.SetBool(IDS_IMPORT_MATERIALS,materials);
	bc.SetBool(IDS_IMPORT_TEXTURES,textures);
	bc.SetBool(IDS_IMPORT_USER_DATA,userdata);
	bc.SetBool(IDS_IMPORT_CONSTRAINTS,constraints);
	
	bc.SetBool(IDC_IMPORT_ANIMATION,animations);
	bc.SetBool(IDS_IMPORT_CURVES,curves);
	bc.SetBool(IDC_IMPORT_PLA,pla);
	
	bc.SetReal(IDC_IMPORT_SCALE,scale);
	bc.SetBool(IDS_FREEZE_SCALE,freeze);
	
	LONG tlen = GetTakeLength(take);
	switch(id)
	{
		case IDC_TAKE:
		{
			SetLong(IDC_IMPORT_START,GetTakeStart(take));
			SetLong(IDC_IMPORT_OFFSET,0);
			SetLong(IDC_IMPORT_END,GetTakeStart(take)+GetTakeEnd(take));
			break;
		}
		case IDC_IMPORT_START:
		{
			SetLong(IDC_IMPORT_OFFSET,0);
			SetLong(IDC_IMPORT_END,tlen+start);
			break;
		}
		case IDC_IMPORT_OFFSET:
		{
			SetLong(IDC_IMPORT_END,tlen+(start-offset));
			break;
		}
		case IDC_IMPORT_END:
		{
			LONG lastFrm = tlen+(start-offset);
			if(end > lastFrm) SetLong(IDC_IMPORT_END,lastFrm);
			break;
		}
	}

	GetLong(IDC_TAKE,take);
	GetLong(IDC_IMPORT_START,start);
	GetLong(IDC_IMPORT_OFFSET,offset);
	GetLong(IDC_IMPORT_END,end);
	
	bc.SetLong(IDC_TAKE,take);
	bc.SetLong(IDC_IMPORT_START,start);
	bc.SetLong(IDC_IMPORT_OFFSET,offset);
	bc.SetLong(IDC_IMPORT_END,end);
	SetWorldPluginData(ID_CDFBXIMPORTER,bc,false);

	DoEnable();

	return true;
}

void CDFbxImportDialog::DoEnable(void)
{
	Bool anim, msh, nrb, mtrl;
	GetBool(IDC_IMPORT_ANIMATION,anim);
	GetBool(IDC_IMPORT_MESH,msh);
	GetBool(IDC_IMPORT_NURBS,nrb);
	GetBool(IDS_IMPORT_MATERIALS,mtrl);
	
	if(!nrb) Enable(IDC_NURBS_SURFACE_LOD,false);
	else Enable(IDC_NURBS_SURFACE_LOD,true);
	
	if(!anim)
	{
		Enable(IDS_IMPORT_CURVES,false);
		SetBool(IDS_IMPORT_CURVES,false);
		Enable(IDC_IMPORT_PLA,false);
		SetBool(IDC_IMPORT_PLA,false);
		
		Enable(IDC_TAKE,false);
		Enable(IDC_IMPORT_START,false);
		Enable(IDC_IMPORT_OFFSET,false);
		Enable(IDC_IMPORT_END,false);
	}
	else
	{
		Enable(IDC_IMPORT_PLA,true);
		Enable(IDS_IMPORT_CURVES,true);
		
		Enable(IDC_TAKE,true);
		Enable(IDC_IMPORT_START,true);
		Enable(IDC_IMPORT_OFFSET,true);
		Enable(IDC_IMPORT_END,true);
	}
	
	if(!msh)
	{
		Enable(IDS_IMPORT_SMOOTHING,false);
		SetBool(IDS_IMPORT_SMOOTHING,false);
		Enable(IDS_IMPORT_MATERIALS,false);
		SetBool(IDS_IMPORT_MATERIALS,false);
		Enable(IDC_IMPORT_MORPH,false);
		SetBool(IDC_IMPORT_MORPH,false);
		Enable(IDC_IMPORT_PLA,false);
		SetBool(IDC_IMPORT_PLA,false);
	}
	else
	{
		Enable(IDS_IMPORT_SMOOTHING,true);
		Enable(IDS_IMPORT_MATERIALS,true);
		Enable(IDC_IMPORT_MORPH,true);
		if(anim) Enable(IDC_IMPORT_PLA,true);
	}
	
	if(!mtrl)
	{
		Enable(IDS_IMPORT_TEXTURES,false);
		SetBool(IDS_IMPORT_TEXTURES,false);
	}
	else Enable(IDS_IMPORT_TEXTURES,true);
}

class CDImportFBXCommand : public CommandData
{
public:
	CDFbxImportDialog			dlg;
	
	AtomArray					*skndMeshes;

	KFbxSdkManager				*pSdkManager;
	KFbxScene					*pScene;
	KFbxNode					*lRootNode;
	
	KArrayTemplate<KString*>	gTakeNameArray;	
	
	Matrix						scaM, jRotM;
	String						fPath, importError;
	Real						imScale;
	LONG						handed, frameStart, frameOffset, frameEnd, nurbsLOD, exportApp;
	CDArray<LONG>						polygonList;
	int							curTake;
	
	Bool	importJoint, importMesh, importCamera, importLight, importAnimation, importMorph, importNURBS, importSpline, importNull;
	Bool	importMarker, importPLA, freezeScale, importSmoothing, importMaterial, importTexture, importUserData, importCurves, importConstraint;

	
	
public:
	BaseObject* KFbxNodeToJoint(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode);
	BaseObject* KFbxNodeToMesh(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode);
	
	void GetMorphShapes(BaseDocument *doc, BaseObject *op, KFbxGeometry *pGoemetry, LONG shpCnt, LONG ptCnt);
	bool ReadVertexCacheData(KFbxGeometry* pGoemetry, KTime& pTime, KFbxVector4* pVertexArray);
	Bool HasVertexCacheDeformer(KFbxGeometry *pGoemetry);
	
	void LoadChannelTexture(BaseChannel *color, KFbxTexture* lTexture);
	void LoadMaterials(BaseDocument *doc, KFbxNode *kNode, BaseObject *op);
	void SetMaterialMapping(KFbxNode *kNode, BaseObject *op);
	LONG GetNgonID(LONG pind);
	
	BaseObject* KFbxNodeToCamera(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode);
	BaseObject* KFbxNodeToLight(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode);
	BaseObject* KFbxNodeToNull(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode);
	BaseObject* KFbxNodeToSpline(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode);
	
	void BuildSkinning(BaseDocument *doc);
	void SetWeight(BaseDocument *doc, BaseContainer *tData, CDSkinVertex *skinWeight, Real *tWeight, LONG pCnt);
	
	void SetCameraTargets(void);
	void SetLightTargets(void);
	Vector GetRotationalOffset(KFbxNode *kNode, Matrix m);
	
	LONG GetUserPropertyCount(KFbxNode *kNode);
	void AddUserData(BaseObject *op, KFbxNode *kNode);
	BaseTag* GetPreviousConstraintTag(BaseObject *op);
	void ConvertConstriants(BaseDocument *doc, KFbxScene *pScene);
	
	BaseObject* GetObjectFromNode(KFbxNode *kNode);
	KFbxNode* GetNodeFromObject(BaseObject *op);
	
	Bool HasBindPose(KFbxNode *kNode);
	KFbxXMatrix GetImportMatrix(KFbxNode *kNode);
	LONG GetValidPolygonCount(KFbxMesh *pMesh, LONG pCnt);
	void SelectEdgesFromPoints(BaseSelect *edgeS, BaseObject *op, Vector *padr, CPolygon *vadr, int aInd, int bInd);

	Bool ImportFBX(BaseDocument* doc);
	void LoadContent(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode);
	void LoadAnimation(BaseDocument *doc, BaseObject *root);
	kFCurveIndex GetKeyIndex(KTime time, KFCurve *fc);
	Bool HasKeyAtTime(KTime time, KFCurve *fc);
	LONG GetKeyInterpolation(KTime time, KFCurve *fc);
	Bool SetKeyTangents(BaseDocument *doc, BaseList2D *op, const BaseTime &docTime, DescID dscID, KTime time, KFCurve *fc, Bool rad, Real scale=1.0);
	
	void SetPositionCurveKeys(BaseDocument *doc, BaseObject *op, BaseTime docTime, KFbxNode *kNode, KTime curTime, char* takeName, Vector pos);
	void SetRotationCurveKeys(BaseDocument *doc, BaseObject *op, BaseTime docTime, KFbxNode *kNode, KTime curTime, char* takeName, Vector rot);
	void SetScaleCurveKeys(BaseDocument *doc, BaseObject *op, BaseTime docTime, KFbxNode *kNode, KTime curTime, char* takeName, Vector sca);
	
	void GetVertexCacheRange(KFbxScene *pScene, KFbxNode *kNode, int &start, int &end);
	void BuildTakeList(KFbxScene *pScene);
	
	void DestroySdkObjects(void);
	void FreeListMemory(void);
	
	Bool LoadFBX(Filename *fName);
	Bool LoadScene(KFbxSdkManager *pSdkManager, KFbxScene *pScene, const char *pFilename);
	
	Bool SetImportOptions(void);
	void MoveSkinnedMeshes(BaseDocument *doc, BaseObject *root);
	Bool ImportFBX(Filename fName);
	
	virtual Bool Execute(BaseDocument* doc);

	// Coffee interface functions
	Bool ExecuteFileImport(String filename, LONG t);
};

bool CDImportFBXCommand::ReadVertexCacheData(KFbxGeometry* pGoemetry, KTime& pTime, KFbxVector4* pVertexArray)
{
    KFbxVertexCacheDeformer *lDeformer = static_cast<KFbxVertexCacheDeformer*>(pGoemetry->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE)); if(!lDeformer) return false;
    KFbxCache *lCache = lDeformer->GetCache(); if(!lCache) return false;
	
    int lChannelIndex = -1;
    unsigned int lVertexCount = (unsigned int)pGoemetry->GetControlPointsCount();
    double *lReadBuf = new double[3*lVertexCount];
	
    bool lReadSucceed = false;
	
    if(!lCache->OpenFileForRead())
	{
		lDeformer->SetActive(false);
	}
	else
	{
		if(lCache->GetCacheFileFormat() == KFbxCache::eMC)
		{
			if((lChannelIndex = lCache->GetChannelIndex(lDeformer->GetCacheChannel())) > -1)
			{
				lReadSucceed = lCache->Read(lChannelIndex, pTime, lReadBuf, lVertexCount);
			}
		}
		else // ePC2
		{
			lReadSucceed = lCache->Read((unsigned int)pTime.GetFrame(true), lReadBuf, lVertexCount);
		}
	}
	
    if(lReadSucceed)
    {
        unsigned int lReadBufIndex = 0;
		
        while(lReadBufIndex < 3*lVertexCount)
        {
            // In statements like "pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex++])", 
            // on Mac platform, "lReadBufIndex++" is evaluated before "lReadBufIndex/3". 
            // So separate them.
            pVertexArray[lReadBufIndex/3].SetAt(0, lReadBuf[lReadBufIndex]); lReadBufIndex++;
            pVertexArray[lReadBufIndex/3].SetAt(1, lReadBuf[lReadBufIndex]); lReadBufIndex++;
            pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex]); lReadBufIndex++;
        }
    }
	
    delete [] lReadBuf;
	
	return lReadSucceed;
}

void CDImportFBXCommand::SetWeight(BaseDocument *doc, BaseContainer *tData, CDSkinVertex *skinWeight, Real *tWeight, LONG pCnt)
{
	Bool normPaint = tData->GetBool(NORMALIZED_WEIGHT);
	Real totalWeight,totalWt;
	
	BaseObject *jnt = NULL;
	LONG i, j, jCnt = tData->GetLong(J_COUNT), jIndex = J_LINK;
	for(i=0; i<pCnt; i++)
	{
		if(skinWeight[i].jw[tData->GetLong(J_LINK_ID)] != tWeight[i])
		{
			skinWeight[i].jw[tData->GetLong(J_LINK_ID)] = tWeight[i];
			skinWeight[i].jn = 0;
			totalWeight = 0.0;
			for(j=0; j<jCnt; j++)
			{
				jnt = tData->GetObjectLink(jIndex+j,doc);
				if(jnt)
				{							
					if(skinWeight[i].jw[j] > 0.01)
					{
						totalWeight += skinWeight[i].jw[j];
						skinWeight[i].jPlus[skinWeight[i].jn] = j;
						skinWeight[i].jn += 1;
					}
				}
			}
			if(!normPaint)
			{
				skinWeight[i].taw = totalWeight;
			}
			else
			{
				LONG j, jCnt, indPlus;
				if(tWeight[i] < 1.0)
				{
					totalWt = totalWeight - tWeight[i];
					if(totalWt > 0.0)
					{
						jCnt = skinWeight[i].jn;
						for(j=0; j<jCnt; j++)
						{
							indPlus = skinWeight[i].jPlus[j];
							jnt = tData->GetObjectLink(jIndex+indPlus,doc);
							if(jnt && jnt != tData->GetObjectLink(jIndex+tData->GetLong(J_LINK_ID),doc))
							{
								Real wt = (1.0 - tWeight[i]) * (skinWeight[i].jw[indPlus]/totalWt);
								skinWeight[i].jw[indPlus] = wt;
							}
						}
						skinWeight[i].taw = 1.0;
					}
					else
					{
						skinWeight[i].jPlus[0] = tData->GetLong(J_LINK_ID);
						skinWeight[i].jw[tData->GetLong(J_LINK_ID)] = tWeight[i];
						skinWeight[i].jn = 1;
						skinWeight[i].taw = tWeight[i];
					}
				}
				else
				{
					jCnt = skinWeight[i].jn;
					for(j=0; j<jCnt; j++)
					{
						indPlus = skinWeight[i].jPlus[j];
						jnt = tData->GetObjectLink(jIndex+indPlus,doc);
						if(jnt && jnt != tData->GetObjectLink(jIndex+tData->GetLong(J_LINK_ID),doc))
						{
							skinWeight[i].jw[indPlus] = 0.0;
						}
					}
					skinWeight[i].jPlus[0] = tData->GetLong(J_LINK_ID);
					skinWeight[i].jw[tData->GetLong(J_LINK_ID)] = 1.0;
					skinWeight[i].jn = 1;
					skinWeight[i].taw = 1.0;
				}
				
			}
		}
		else
		{
			if(normPaint && tWeight[i] == 1.0)
			{
				LONG jCnt, indPlus;
				jCnt = skinWeight[i].jn;
				for(j=0; j<jCnt; j++)
				{
					indPlus = skinWeight[i].jPlus[j];
					jnt = tData->GetObjectLink(jIndex+indPlus,doc);
					if(jnt && jnt != tData->GetObjectLink(jIndex+tData->GetLong(J_LINK_ID),doc))
					{
						skinWeight[i].jw[indPlus] = 0.0;
					}
				}
				skinWeight[i].jPlus[0] = tData->GetLong(J_LINK_ID);
				skinWeight[i].jw[tData->GetLong(J_LINK_ID)] = 1.0;
				skinWeight[i].jn = 1;
				skinWeight[i].taw = 1.0;
			}
		}
	}
}

void CDImportFBXCommand::BuildSkinning(BaseDocument *doc)
{
	ObjectNode *olist = objectList.Array(); if(!olist) return;
	for(int o=0; o<objectList.Size(); o++)
	{
		if(olist && olist[o].nodeType == NODE_TYPE_MESH && olist[o].skinned)
		{
			BaseObject *mesh = olist[o].object;
			KFbxNode *kNode = olist[o].node;
			KFbxMesh *pMesh = (KFbxMesh*)kNode->GetNodeAttribute();
			
			KFbxSkin* lSkinDeformer = reinterpret_cast<KFbxSkin*>(pMesh->GetDeformer(0, KFbxDeformer::eSKIN));
			if(!lSkinDeformer) continue;
			
			KFbxCluster* pCluster = lSkinDeformer->GetCluster(0);
			if(!pCluster) continue;

			KFbxCluster::ELinkMode lClusterMode = pCluster->GetLinkMode();
			
			int i, lClusterCount = lSkinDeformer->GetClusterCount();
			for(i= 0 ; i < lClusterCount; i ++ )
			{
				KFbxXMatrix clusterBindMatrix;
				KFbxXMatrix meshBindMatrix;
				
				pCluster = lSkinDeformer->GetCluster(i);
				KFbxNode* lLinkNode = pCluster->GetLink();
				if(lLinkNode)
				{
					if(lClusterMode == KFbxLink::eADDITIVE && pCluster->GetAssociateModel())
					{
						pCluster->GetTransformAssociateModelMatrix(meshBindMatrix);
					}
					else
					{
						pCluster->GetTransformMatrix(meshBindMatrix);
					}
					
					pCluster->GetTransformLinkMatrix( clusterBindMatrix );

					for(int j=0; j<objectList.Size(); j++)
					{
						if(lLinkNode == olist[j].node)
						{
							olist[j].cluster = pCluster; // flag joint as cluster
							olist[j].matrix = olist[o].matrix * (meshBindMatrix.Inverse() * clusterBindMatrix); // store cluster bind matrix
							
							break;
						}
					}
				}
			}
			
			// set skin ref
			BaseTag *skinRef = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
			if(!skinRef) continue;
			
			mesh->InsertTag(skinRef,NULL);
			mesh->Message(MSG_UPDATE);
			DescriptionCommand skrdc;
			skrdc.id = DescID(CDSKR_SET_REFERENCE);
			skinRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
			
			// set skin
			BaseTag *skin = BaseTag::Alloc(ID_CDSKINPLUGIN);
			if(!skin) continue;
			
			mesh->InsertTag(skin,skinRef);
			BaseContainer *tData = skin->GetDataInstance();
			if(!tData) continue;
			
			tData->SetLink(DEST_ID,mesh);
			tData->SetLong(J_COUNT,lClusterCount);

			// Initialize skin tag's weight array
			skin->Message(MSG_MENUPREPARE);
			
			CDSkinVertex *skinWeight = CDGetSkinWeights(skin);
			if(!skinWeight) continue;
			
			// set the bind pose of the joints that are clusters
			LONG jlnkID = 0, pCnt = ToPoint(mesh)->GetPointCount();
			for(int j=0; j<objectList.Size(); j++)
			{
				StatusSetSpin();
				BaseObject *joint = olist[j].object;
				if(!joint) continue;
				
				pCluster = olist[j].cluster;
				if(pCluster)
				{
					Matrix jntM = KFbxXMatrixToMatrix(olist[j].matrix,handed);
					if(handed == 1) jntM = jntM * jRotM;
					
					if(freezeScale) jntM.off = scaM * jntM.off;
					joint->SetMg(jntM); // set joint's global matrix
					tData->SetLong(J_LINK_ID,jlnkID);
					
					CDArray<Real> tWeight;
					if(tWeight.Alloc(pCnt))
					{
						tWeight.Fill(0.0);
						tData->SetLink(J_LINK+jlnkID,joint);
						
						int k, lVertexIndexCount = pCluster->GetControlPointIndicesCount();
						for (k = 0; k < lVertexIndexCount; ++k)
						{
							LONG pInd = pCluster->GetControlPointIndices()[k];
							Real wt = (Real)pCluster->GetControlPointWeights()[k];
							tWeight[pInd] = wt;
						}
						
						SetWeight(doc,tData,skinWeight,tWeight.Array(),pCnt);
						jlnkID++;
						
						tWeight.Free();
					}
					olist[j].cluster = NULL;
				}
			}
			
			// Set accumulated weights
			DescriptionCommand dc;
			dc.id = DescID(ACCUMULATE);
			skin->Message(MSG_DESCRIPTION_COMMAND,&dc);
			
			//Bind skin
			dc.id = DescID(AUTO_BIND_SKIN);
			skin->Message(MSG_DESCRIPTION_COMMAND,&dc);
			skin->Message(MSG_UPDATE);
			
			if(skndMeshes) skndMeshes->Append(mesh);
		}
	}
}

LONG CDImportFBXCommand::GetValidPolygonCount(KFbxMesh *pMesh, LONG pCnt)
{
	LONG cnt = 0;
	
	for(int i=0; i<pCnt; i++)
	{
		int pSize = pMesh->GetPolygonSize(i);
		if(pSize < 5) cnt++;
	}
	
	return cnt;
}

Vector CDImportFBXCommand::GetRotationalOffset(KFbxNode *kNode, Matrix m)
{
	Vector offset1, offset2;
    Matrix nM = GetNormalizedMatrix(m);
	
	KFbxVector4 lTmpVector = kNode->GetRotationPivot(KFbxNode::eSOURCE_SET);
	offset1.x = Real(lTmpVector[2]);
	offset1.y = Real(lTmpVector[1]);
	offset1.z = Real(lTmpVector[0]);
	
	offset2.x = Real(lTmpVector[0])*-1;
	offset2.y = Real(lTmpVector[1])*-1;
	offset2.z = Real(lTmpVector[2]);
	
	return nM * (offset1 + offset2);
}

Bool CDImportFBXCommand::HasBindPose(KFbxNode *kNode)
{
	int i, poseCnt = pScene->GetPoseCount();
	if(poseCnt > 0)
	{
		for(i=0; i<poseCnt; i++)
		{
			KFbxPose *pose = pScene->GetPose(i);
			if(pose->IsBindPose())
			{
				int nodeInd = pose->Find(kNode);
				if(nodeInd > -1) return true;
			}
		}
	}

	return false;
}

KFbxXMatrix CDImportFBXCommand::GetImportMatrix(KFbxNode *kNode)
{
	KFbxXMatrix kM = kNode->GetGlobalFromDefaultTake();
	
	int i, poseCnt = pScene->GetPoseCount();
	if(poseCnt > 0)
	{
		for(i=0; i<poseCnt; i++)
		{
			KFbxPose *pose = pScene->GetPose(i);
			if(pose && pose->IsBindPose())
			{
				int nodeInd = pose->Find(kNode);
				if(nodeInd > -1)
				{
					KFbxMatrix lMatrix = pose->GetMatrix(nodeInd);
					kM = *(KFbxXMatrix*)(double*)&lMatrix;
				}
			}
		}
	}
	
	return kM;
}

BaseObject* CDImportFBXCommand::GetObjectFromNode(KFbxNode *kNode)
{
	BaseObject *op = NULL;
	
	ObjectNode *olist = objectList.Array();
	LONG ind = objectList.GetIndex(kNode);
	if(ind >= 0) op = olist[ind].object;
	
	return op;
}

KFbxNode* CDImportFBXCommand::GetNodeFromObject(BaseObject *op)
{
	KFbxNode *node = NULL;
	
	ObjectNode *olist = objectList.Array();
	LONG ind = objectList.GetIndex(op);
	if(ind >= 0) node = olist[ind].node;
	
	return node;
}

LONG CDImportFBXCommand::GetUserPropertyCount(KFbxNode *kNode)
{
    LONG lCount = 0;
	
	KFbxProperty lProperty = kNode->GetFirstProperty();
    while (lProperty.IsValid())
    {
        if(lProperty.GetFlag(KFbxUserProperty::eUSER))
            lCount++;
		
        lProperty = kNode->GetNextProperty(lProperty);
    }
	
	return lCount;
}

void CDImportFBXCommand::AddUserData(BaseObject *op, KFbxNode *kNode)
{
	DynamicDescription *opDD = op->GetDynamicDescription();
	if(opDD)
	{
		KFbxProperty lProperty = kNode->GetFirstProperty();
		
		while (lProperty.IsValid())
		{
			if(lProperty.GetFlag(KFbxUserProperty::eUSER))
			{
				KFbxDataType lPropertyDataType = lProperty.GetPropertyDataType();
				String propertyName = CharToString(lProperty.GetName());
				
				// bool
				if(lPropertyDataType.GetType() == eBOOL1)
				{
					BaseContainer bc = GetCustomDataTypeDefault(DTYPE_BOOL);
					bc.SetString(DESC_NAME,propertyName);
					bc.SetString(DESC_SHORT_NAME,propertyName);
					
					DescID dscID = opDD->Alloc(bc);
				#if API_VERSION < 9800
					opDD->Set(dscID, bc);
				#else
					opDD->Set(dscID, bc, op);
				#endif
					GeData gData = GeData(Bool(KFbxGet<bool>(lProperty)));
					CDSetParameter(op, dscID, gData);
				}
				// long
				else if(lPropertyDataType.GetType() == eINTEGER1)
				{
					BaseContainer bc = GetCustomDataTypeDefault(DTYPE_LONG);
					bc.SetString(DESC_NAME,propertyName);
					bc.SetString(DESC_SHORT_NAME,propertyName);
					if(lProperty.HasMinLimit() && lProperty.HasMaxLimit() && lProperty.GetMinLimit() != lProperty.GetMaxLimit())
					{
						if(lProperty.HasMinLimit()) bc.SetLong(DESC_MIN,LONG(lProperty.GetMinLimit()));
						if(lProperty.HasMaxLimit()) bc.SetLong(DESC_MAX,LONG(lProperty.GetMaxLimit()));
					}
					
					DescID dscID = opDD->Alloc(bc);
				#if API_VERSION < 9800
					opDD->Set(dscID, bc);
				#else
					opDD->Set(dscID, bc, op);
				#endif
					GeData gData = GeData(LONG(KFbxGet<int>(lProperty)));
					CDSetParameter(op, dscID, gData);
				}
				// real
				else if(lPropertyDataType.GetType() == eDOUBLE1 || lPropertyDataType.GetType() == eFLOAT1)
				{
					BaseContainer bc = GetCustomDataTypeDefault(DTYPE_REAL);
					bc.SetString(DESC_NAME,propertyName);
					bc.SetString(DESC_SHORT_NAME,propertyName);
					bc.SetReal(DESC_STEP,0.01);
					if(lProperty.HasMinLimit() && lProperty.HasMaxLimit() && lProperty.GetMinLimit() !=lProperty.GetMaxLimit())
					{
						if(lProperty.HasMinLimit()) bc.SetReal(DESC_MIN,Real(lProperty.GetMinLimit()));
						if(lProperty.HasMaxLimit()) bc.SetReal(DESC_MAX,Real(lProperty.GetMaxLimit()));
					}
					
					DescID dscID = opDD->Alloc(bc);
				#if API_VERSION < 9800
					opDD->Set(dscID, bc);
				#else
					opDD->Set(dscID, bc, op);
				#endif
					GeData gData = GeData(Real(KFbxGet<double>(lProperty)));
					CDSetParameter(op, dscID, gData);
				}
				// vector
				else if(lPropertyDataType.GetType() == eDOUBLE3 || lPropertyDataType.GetType() == eDOUBLE4)
				{
					BaseContainer bc = GetCustomDataTypeDefault(DTYPE_VECTOR);
					bc.SetString(DESC_NAME,propertyName);
					bc.SetString(DESC_SHORT_NAME,propertyName);
					
					DescID dscID = opDD->Alloc(bc);
					fbxDouble3 vecValues = KFbxGet<fbxDouble3>(lProperty);
					GeData gData = GeData(Vector(Real(vecValues[0]),Real(vecValues[1]),Real(vecValues[2])));
					CDSetParameter(op, dscID, gData);
					
				}
				// popup menu (long)
				else if(lPropertyDataType.GetType() == eENUM)
				{
					BaseContainer bc = GetCustomDataTypeDefault(DTYPE_LONG);
					bc.SetString(DESC_NAME,propertyName);
					bc.SetString(DESC_SHORT_NAME,propertyName);
					
					BaseContainer cycle;
					int eCnt = lProperty.GetEnumCount();
					for(int i=0; i<eCnt; i++)
					{
						String cycleName = CharToString(lProperty.GetEnumValue(i));
						cycle.SetString(i,cycleName);
					}
					bc.SetContainer(DESC_CYCLE, cycle);
					
					DescID dscID = opDD->Alloc(bc);
				#if API_VERSION < 9800
					opDD->Set(dscID, bc);
				#else
					opDD->Set(dscID, bc, op);
				#endif
					GeData gData = GeData(LONG(KFbxGet<int>(lProperty)));
					CDSetParameter(op, dscID, gData);
				}
			}
			
			lProperty = kNode->GetNextProperty(lProperty);
		}
	}
}

void CDImportFBXCommand::SelectEdgesFromPoints(BaseSelect *edgeS, BaseObject *op, Vector *padr, CPolygon *vadr, int aInd, int bInd)
{
	LONG vCnt = ToPoly(op)->GetPolygonCount(), pCnt = ToPoint(op)->GetPointCount();;
	LONG *dadr = NULL, dcnt = 0, p, side;

	Neighbor n;
	if(n.Init(pCnt,vadr,vCnt,NULL))
	{
		n.GetPointPolys(aInd, &dadr, &dcnt);
		for (p=0; p<dcnt; p++)
		{
			CPolygon ply = vadr[dadr[p]];
			
			for(side=0; side<4; side++)
			{
				StatusSetSpin();
				
				switch (side)
				{
					case 0:
					{
						if(aInd == ply.a && bInd == ply.b || bInd == ply.a && aInd == ply.b)
							edgeS->Select(4*dadr[p]+side);
						break;
					}
					case 1:
					{
						if(aInd == ply.b && bInd == ply.c || bInd == ply.b && aInd == ply.c)
							edgeS->Select(4*dadr[p]+side);
						break;
					}
					case 2:
					{
						if(aInd == ply.c && bInd == ply.d || bInd == ply.c && aInd == ply.d)
							edgeS->Select(4*dadr[p]+side);
						break;
					}
					case 3:
					{
						if(aInd == ply.d && bInd == ply.a || bInd == ply.d && aInd == ply.a)
							edgeS->Select(4*dadr[p]+side);
						break;
					}
				}
			}
		}
	}
}

void CDImportFBXCommand::GetMorphShapes(BaseDocument *doc, BaseObject *op, KFbxGeometry *pGoemetry, LONG shpCnt, LONG ptCnt)
{
	BaseTag *refTag = op->GetTag(ID_CDMORPHREFPLUGIN);
	if(!refTag)
	{
		refTag = BaseTag::Alloc(ID_CDMORPHREFPLUGIN);
		op->InsertTag(refTag,NULL);
		refTag->Message(MSG_MENUPREPARE);
		op->Message(MSG_UPDATE);
	}
	
	String oName = CharToString(pGoemetry->GetName());
	
	int i,j;
	for(i=0; i<shpCnt; i++)
	{
		KFbxShape* pShape = pGoemetry->GetShape(i);
		if(pShape)
		{
			String tName = CharToString(pGoemetry->GetShapeName(i));
			BaseObject *mop = BaseObject::Alloc(Opolygon);
			if(mop)
			{
				LONG plyCnt = ToPoly(op)->GetPolygonCount();
				ToPoly(mop)->ResizeObject(ptCnt,plyCnt);
				
				BaseSelect *bs = ToPoint(op)->GetPointS();
				if(bs) bs->DeselectAll();
				
				Vector *padr = GetPointArray(op), *mpadr = GetPointArray(mop);
				KFbxVector4 *shpControlPoints = pShape->GetControlPoints();
				if(!shpControlPoints) continue;
				
				LONG status = 0;
				StatusSetText("Loading mesh - "+oName+" - import blend shape - "+tName);
				for(j=0; j<ptCnt; j++)
				{
					LONG shCnt = shapeList.Size();
					if(shCnt > 0) status = LONG(Real((i*ptCnt)+j)/Real(shCnt*ptCnt)*100.0);
					StatusSetBar(status);
					
					KFbxVector4 shpPt = shpControlPoints[j];
					Vector pt = KFbxVector4ToVector(shpPt);
					if(handed == 1) pt.z *= -1;
					if(freezeScale) mpadr[j] = scaM * pt;
					else mpadr[j] = pt;
					
					if(!VEqual(padr[j],mpadr[j],0.001)) bs->Select(j);
				}
				
				if(bs->GetCount() > 0)
				{
					AutoAlloc<BaseLink> link;
					if(link)
					{
						link->SetLink(op);
						
						BaseTag *mTag = NULL;
						mTag = BaseTag::Alloc(ID_CDMORPHTAGPLUGIN);
						op->InsertTag(mTag,refTag);
						mTag->SetName(tName);
						
						BaseContainer *tData = mTag->GetDataInstance();
						if(tData)
						{
							tData->SetLink(M_DEST_LINK,op);
							mTag->Message(MSG_MENUPREPARE);
						}
						
						DescriptionCommand dc;
						dc.id = DescID(M_SET_SELECTION);
						mTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
						
						LONG seg=0,a,b,si;
						while(CDGetRange(bs,seg++,&a,&b))
						{
							for (si=a; si<=b; ++si)
							{
								StatusSetSpin();
								padr[si] = mpadr[si];
							}
						}
						dc.id = DescID(M_SET_OFFSET);
						mTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
						
						ShapeNode snode = ShapeNode(mTag,pShape,mTag->GetName());
						shapeList.Append(snode);
					}
				}
				
				BaseObject::Free(mop);
			}
		}
	}
}

LONG CDImportFBXCommand::GetNgonID(LONG pind)
{
	NgonUVW *nlist = ngonList.Array();
	for(int n=0; n<ngonList.Size(); n++)
	{
		StatusSetSpin();
		LONG kply = nlist[n].kPolyID;
		if(kply == pind) return nlist[n].ngonID;
	}
	
	return 0;
}

void CDImportFBXCommand::LoadChannelTexture(BaseChannel *channel, KFbxTexture* texture)
{
	String txFileName, txFilePath = CharToString(texture->GetFileName());
	LONG pos,strLen = txFilePath.GetLength();
	txFilePath.FindLast("/",&pos);
	LONG dLen = strLen - (pos+1);
	txFileName = txFilePath.SubStr(pos+1,dLen);
	txFilePath.Delete(pos+1,dLen);
	
	if(!txFilePath.Content())
	{
		txFilePath = fPath;
		txFilePath.FindLast("/",&pos);
		strLen = txFilePath.GetLength();
		dLen = strLen - (pos+1);
		txFilePath.Delete(pos+1,dLen);
	}
	
	BaseContainer data = channel->GetData();
	data.SetString(BASECHANNEL_TEXTURE, txFileName);
	data.SetFilename(BASECHANNEL_SUGGESTEDFOLDER, txFilePath);
	channel->SetData(data);
}

void CDImportFBXCommand::SetMaterialMapping(KFbxNode *kNode, BaseObject *op)
{
	if(!kNode) return;
	
	KFbxMesh *pMesh = (KFbxMesh*)kNode->GetNodeAttribute();
	if(pMesh)
	{
		KFbxLayer *kLayer = pMesh->GetLayer(0);
		if(kLayer)
		{
			KFbxLayerElementMaterial *lMaterialLayer = kLayer->GetMaterials();
			if(lMaterialLayer)
			{
				if(lMaterialLayer->GetMappingMode() == KFbxLayerElement::eBY_POLYGON)
				{
					if(lMaterialLayer->GetReferenceMode() == KFbxLayerElement::eINDEX || lMaterialLayer->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT)
					{
						AutoAlloc<AtomArray> slTags;
						if(slTags)
						{
							BaseTag *tTag = NULL;
							LONG tCnt = 0;
							GeData dSet;
							for(tTag = op->GetFirstTag(); tTag; tTag = tTag->GetNext())
							{
								StatusSetSpin();
								if(tTag && tTag->GetType() == Ttexture)
								{
									SelectionTag *sTag = SelectionTag::Alloc(Tpolygonselection);
									if(sTag)
									{
										op->InsertTag(sTag,tTag);
										
										String sTagName = "Polygon Selection."+CDLongToString(tCnt);
										sTag->SetName(sTagName);
										dSet = sTagName;
										CDSetParameter(tTag,DescID(TEXTURETAG_RESTRICTION),dSet);
										slTags->Append(sTag);
									}
									tCnt++;
								}
							}
							
							LONG ngoncount, *polymap = NULL;
							LONG **ngons = NULL;
							
							Bool mapping = true;
							if(!ToPoly(op)->GetPolygonTranslationMap(ngoncount, polymap))
							{
								CDFree(polymap);
								mapping =  false;
							}
							
							if(!ToPoly(op)->GetNGonTranslationMap(ngoncount, polymap, ngons))
							{
								CDFree(polymap);
								CDFree(ngons);
								mapping =  false;
							}
							
							LONG plyCnt = ToPoly(op)->GetPolygonCount();
							int iaCnt = lMaterialLayer->GetIndexArray().GetCount();
							int i;
							for(i=0; i<iaCnt; i++)
							{
								StatusSetSpin();
								int stInd = lMaterialLayer->GetIndexArray().GetAt(i);
								if(stInd > -1 && stInd < slTags->GetCount())
								{
									SelectionTag *sTag = static_cast<SelectionTag*>(slTags->GetIndex(stInd));
									if(sTag)
									{
										BaseSelect *bs = sTag->GetBaseSelect();
										if(bs)
										{
											LONG pSize = pMesh->GetPolygonSize(i);
											if(mapping && pSize > 4)
											{
												LONG nid = GetNgonID(i);
												LONG p;
												for(p=1; p<=ngons[nid][0]; p++)
												{
													LONG plyID = ngons[nid][p];
													if(plyID < plyCnt)
													{
														bs->Select(plyID);
													}
												}
											}
											else
											{
												LONG index = polygonList[i];
												if(index < plyCnt && index > -1)
												{
													bs->Select(index);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void CDImportFBXCommand::LoadMaterials(BaseDocument *doc, KFbxNode *kNode, BaseObject *op)
{
	GeData dSet;
	int m;
	
	BaseTag *tPred = NULL;
	
	int mtrCnt = kNode->GetSrcObjectCount(KFbxSurfaceMaterial::ClassId);
	for(m=0; m<mtrCnt; m++)
	{
		StatusSetSpin();
		KFbxSurfaceMaterial *lMaterial = KFbxCast <KFbxSurfaceMaterial>(kNode->GetSrcObject(KFbxSurfaceMaterial::ClassId, m));
		if(lMaterial)
		{
			BaseMaterial *mat = BaseMaterial::Alloc(Mmaterial);
			if(mat)
			{
				String mName = CharToString(lMaterial->GetName());
				mat->SetName(mName);
				doc->InsertMaterial(mat);
				
				int txCnt = 0;
				KFbxTexture* lTexture;
				Bool colorHasAlpha = false;

				// color channel
				KFbxProperty lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sDiffuse);
				if(lProperty.IsValid())
				{
					BaseChannel *chColor = mat->GetChannel(CHANNEL_COLOR);
					if(chColor)
					{
						Bool useDif = false;
						
						if(importTexture) txCnt = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
						if(txCnt > 0)
						{
							lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, 0));
							if(lTexture)
							{
								LoadChannelTexture(chColor,lTexture);
								useDif = true;
								
								int alphaType = lTexture->GetAlphaSource();
								if(alphaType == KFbxTexture::eBLACK)
								{
									colorHasAlpha = true;
								}
							}
						}
						else
						{
							if(!lProperty.HasDefaultValue(lProperty))
							{
								KFbxPropertyDouble3 kDiffuseColor;
								KFbxPropertyDouble1 kDiffuseFactor;
								
								if(lMaterial->GetClassId().Is(KFbxSurfaceLambert::ClassId))
								{
									kDiffuseColor = ((KFbxSurfaceLambert*) lMaterial)->GetDiffuseColor();
									kDiffuseFactor = ((KFbxSurfaceLambert*) lMaterial)->GetDiffuseFactor();
								}
								else if(lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId))
								{
									kDiffuseColor = ((KFbxSurfacePhong*) lMaterial)->GetDiffuseColor();
									kDiffuseFactor = ((KFbxSurfacePhong*) lMaterial)->GetDiffuseFactor();
								}
								
								dSet = Vector(Real(kDiffuseColor.Get()[0]),Real(kDiffuseColor.Get()[1]),Real(kDiffuseColor.Get()[2]));
								CDSetParameter(mat,DescID(MATERIAL_COLOR_COLOR), dSet);
								
								Real dfBrightness = Real(kDiffuseFactor.Get());
								if(dfBrightness > 0.0)
								{
									useDif = true;
									dSet = dfBrightness;
									CDSetParameter(mat,DescID(MATERIAL_COLOR_BRIGHTNESS), dSet);
								}
							}
						}
						if(useDif) dSet = true;
						else dSet = false;
						
						CDSetParameter(mat,DescID(MATERIAL_USE_COLOR), dSet);
					}
				}
				 
				// lunimance channel
				lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sEmissive);
				if(lProperty.IsValid())
				{
					BaseChannel *chLuminance = mat->GetChannel(CHANNEL_LUMINANCE);
					if(chLuminance)
					{
						Bool useLum = false;
						
						if(importTexture) txCnt = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
						if(txCnt > 0)
						{
							lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, 0));
							if(lTexture)
							{
								LoadChannelTexture(chLuminance,lTexture);
								useLum = true;
							}
						}
						else
						{
							if(!lProperty.HasDefaultValue(lProperty))
							{
								KFbxPropertyDouble3 kEmissiveColor;
								KFbxPropertyDouble1 kEmissiveFactor;
								
								if(lMaterial->GetClassId().Is(KFbxSurfaceLambert::ClassId))
								{
									kEmissiveColor = ((KFbxSurfaceLambert*) lMaterial)->GetEmissiveColor();
									kEmissiveFactor = ((KFbxSurfaceLambert*) lMaterial)->GetEmissiveFactor();
								}
								else if(lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId))
								{
									kEmissiveColor = ((KFbxSurfacePhong*) lMaterial)->GetEmissiveColor();
									kEmissiveFactor = ((KFbxSurfacePhong*) lMaterial)->GetEmissiveFactor();
								}
								
								dSet = Vector(Real(kEmissiveColor.Get()[0]),Real(kEmissiveColor.Get()[1]),Real(kEmissiveColor.Get()[2]));
								CDSetParameter(mat,DescID(MATERIAL_LUMINANCE_COLOR), dSet);
								
								Real lmBrightness = Real(kEmissiveFactor.Get());
								if(lmBrightness > 0.0)
								{
									useLum = true;
									dSet = lmBrightness;
									CDSetParameter(mat,DescID(MATERIAL_LUMINANCE_BRIGHTNESS), dSet);
								}
							}
						}
						if(useLum)
						{
							dSet = true;
							CDSetParameter(mat,DescID(MATERIAL_USE_LUMINANCE), dSet);
						}
					}
				}
				
				// transparency channel
				lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sTransparentColor);
				if(lProperty.IsValid())
				{
					BaseChannel *chTransparency = NULL;
					
					if(colorHasAlpha) chTransparency = mat->GetChannel(CHANNEL_ALPHA);
					else chTransparency = mat->GetChannel(CHANNEL_TRANSPARENCY);
					
					if(chTransparency)
					{
						Bool useTrns = false;
						
						if(importTexture) txCnt = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
						if(txCnt > 0)
						{
							lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, 0));
							if(lTexture)
							{
								LoadChannelTexture(chTransparency,lTexture);
								useTrns = true;
							}
						}
						else if(!colorHasAlpha)
						{
							if(!lProperty.HasDefaultValue(lProperty))
							{
								KFbxPropertyDouble3 kTransparentColor;
								KFbxPropertyDouble1 kTransparentFactor;
								
								if(lMaterial->GetClassId().Is(KFbxSurfaceLambert::ClassId))
								{
									kTransparentColor = ((KFbxSurfaceLambert*) lMaterial)->GetTransparentColor();
									kTransparentFactor = ((KFbxSurfaceLambert*) lMaterial)->GetTransparencyFactor();
								}
								else if(lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId))
								{
									kTransparentColor = ((KFbxSurfacePhong*) lMaterial)->GetTransparentColor();
									kTransparentFactor = ((KFbxSurfacePhong*) lMaterial)->GetTransparencyFactor();
								}
								
								dSet = Vector(Real(kTransparentColor.Get()[0]),Real(kTransparentColor.Get()[1]),Real(kTransparentColor.Get()[2]));
								CDSetParameter(mat,DescID(MATERIAL_TRANSPARENCY_COLOR), dSet);
								
								Real trBrightness = Real(kTransparentFactor.Get());
								if(trBrightness > 0.0)
								{
									useTrns = true;
									dSet = trBrightness;
									CDSetParameter(mat,DescID(MATERIAL_TRANSPARENCY_BRIGHTNESS), dSet);
								}
							}
						}
						if(useTrns)
						{
							dSet = true;
							if(colorHasAlpha) CDSetParameter(mat,DescID(MATERIAL_USE_ALPHA), dSet);
							else CDSetParameter(mat,DescID(MATERIAL_USE_TRANSPARENCY), dSet);
						}
					}
				}
				
				// reflection channel
				lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sReflection);
				if(lProperty.IsValid())
				{
					BaseChannel *chReflection = mat->GetChannel(CHANNEL_REFLECTION);
					if(chReflection)
					{
						Bool useRefl = false;
						
						if(importTexture) txCnt = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
						if(txCnt > 0)
						{
							lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, 0));
							if(lTexture)
							{
								LoadChannelTexture(chReflection,lTexture);
								useRefl = true;
							}
						}
						else
						{
							if(!lProperty.HasDefaultValue(lProperty))
							{
								if(lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId))
								{
									KFbxPropertyDouble3 kReflectionColor = ((KFbxSurfacePhong*) lMaterial)->GetReflectionColor();
									KFbxPropertyDouble1 kReflectionFactor = ((KFbxSurfacePhong*) lMaterial)->GetReflectionFactor();

									dSet = Vector(Real(kReflectionColor.Get()[0]),Real(kReflectionColor.Get()[1]),Real(kReflectionColor.Get()[2]));
									CDSetParameter(mat,DescID(MATERIAL_REFLECTION_COLOR), dSet);
									
									Real rflBrightness = Real(kReflectionFactor.Get());
									if(rflBrightness > 0.0)
									{
										useRefl = true;
										dSet = rflBrightness;
										CDSetParameter(mat,DescID(MATERIAL_REFLECTION_BRIGHTNESS), dSet);
									}
								}
							}
						}
						
						if(useRefl)
						{
							dSet = true;
							CDSetParameter(mat,DescID(MATERIAL_USE_REFLECTION), dSet);
						}
					}
				}
				
				// bump channel
				lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sBump);
				if(lProperty.IsValid())
				{
					if(importTexture) txCnt = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
					if(txCnt > 0)
					{
						lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, 0));
						if(lTexture)
						{
							BaseChannel *chBump = mat->GetChannel(CHANNEL_BUMP);
							if(chBump)
							{
								dSet = true;
								CDSetParameter(mat,DescID(MATERIAL_USE_BUMP), dSet);
								
								LoadChannelTexture(chBump,lTexture);
								KFbxPropertyDouble1 kBumpFactor;
								
								if(lMaterial->GetClassId().Is(KFbxSurfaceLambert::ClassId))
									kBumpFactor = ((KFbxSurfaceLambert*) lMaterial)->GetBumpFactor();
								
								else if(lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId))
									kBumpFactor = ((KFbxSurfacePhong*) lMaterial)->GetBumpFactor();
								
								dSet = Real(kBumpFactor.Get());
								CDSetParameter(mat,DescID(MATERIAL_BUMP_STRENGTH), dSet);
							}
						}
					}
				}
				
				// specular channel
				lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sShininess);
				if(lProperty.IsValid() && !lProperty.HasDefaultValue(lProperty))
				{
					BaseChannel *chSpecular = mat->GetChannel(CHANNEL_SPECULAR);
					if(chSpecular)
					{
						Bool useSpec = false;
						
						if(!lProperty.HasDefaultValue(lProperty))
						{
							if(lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId))
							{
								useSpec = true;
								
								KFbxPropertyDouble1 kShininess = ((KFbxSurfacePhong*) lMaterial)->GetShininess();
								Real mix = (Real(kShininess.Get())-2.0)/(100.0-2.0);
								dSet = CDBlend(1.0,0.3,mix);
								CDSetParameter(mat,DescID(MATERIAL_SPECULAR_WIDTH), dSet);
							}
						}
						if(useSpec) dSet = true;
						else dSet = false;
						
						CDSetParameter(mat,DescID(MATERIAL_USE_SPECULAR), dSet);
					}
				}
				
				// specular color channel
				lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sSpecular);
				if(lProperty.IsValid())
				{
					BaseChannel *chSpecularColor = mat->GetChannel(CHANNEL_SPECULARCOLOR);
					if(chSpecularColor)
					{
						Bool useSpClr = false;
						
						if(importTexture) txCnt = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
						if(txCnt > 0)
						{
							lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, 0));
							if(lTexture)
							{
								LoadChannelTexture(chSpecularColor,lTexture);
								useSpClr = true;
							}
						}
						else
						{
							if(!lProperty.HasDefaultValue(lProperty))
							{
								if(lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId))
								{
									KFbxPropertyDouble3 kSpecularColor = ((KFbxSurfacePhong*) lMaterial)->GetSpecularColor();
									KFbxPropertyDouble1 kSpecularFactor = ((KFbxSurfacePhong*) lMaterial)->GetSpecularFactor();
									
									dSet = Vector(Real(kSpecularColor.Get()[0]),Real(kSpecularColor.Get()[1]),Real(kSpecularColor.Get()[2]));
									CDSetParameter(mat,DescID(MATERIAL_SPECULAR_COLOR), dSet);
									
									Real spBrightness = Real(kSpecularFactor.Get());
									if(spBrightness > 0.0)
									{
										useSpClr = true;
										dSet = spBrightness;
										CDSetParameter(mat,DescID(MATERIAL_SPECULAR_BRIGHTNESS), dSet);
									}
								}
							}
						}
						
						if(useSpClr)
						{
							dSet = true;
							CDSetParameter(mat,DescID(MATERIAL_USE_SPECULARCOLOR), dSet);
						}
					}
				}
				
				// normal channel
				lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sNormalMap);
				if(lProperty.IsValid() && !lProperty.HasDefaultValue(lProperty))
				{
					if(importTexture) txCnt = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
					if(txCnt > 0)
					{
						lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, 0));
						if(lTexture)
						{
							BaseChannel *chNormal = mat->GetChannel(CHANNEL_NORMAL);
							if(chNormal)
							{
								dSet = true;
								CDSetParameter(mat,DescID(MATERIAL_USE_NORMAL), dSet);
								
								LoadChannelTexture(chNormal,lTexture);
							}
						}
					}
				}

				TextureTag *txTag = TextureTag::Alloc();
				if(txTag)
				{
					op->InsertTag(txTag,tPred);
					txTag->SetMaterial(mat);
					
					GeData d;
					d = TEXTURETAG_PROJECTION_UVW;
					CDSetParameter(txTag,DescLevel(TEXTURETAG_PROJECTION),d);
					tPred = txTag;
				}
			}
		}
	}
}

BaseObject* CDImportFBXCommand::KFbxNodeToJoint(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode)
{
	BaseObject *jnt = BaseObject::Alloc(ID_CDJOINTOBJECT);
	if(jnt)
	{
		String oName = CharToString(kNode->GetName());
		StatusSetText("Loading joint - "+oName);
		StatusSetSpin();
		
		jnt->SetName(oName);
		jnt->InsertUnderLast(pr);
		jnt->Message(MSG_MENUPREPARE);
		
		KFbxNodeAttribute *lAttribute =  kNode->GetNodeAttribute();
		if(lAttribute)
		{
			fbxDouble3 clr = lAttribute->Color.Get();
			
			ObjectColorProperties prop;
			jnt->GetColorProperties(&prop);
			prop.xray = false;
			prop.color = Vector(clr[0],clr[1],clr[2]);
			prop.usecolor = 2;
			jnt->SetColorProperties(&prop);
		}
		
		KFbxXMatrix kM = GetImportMatrix(kNode);
		
		ObjectNode opNode = ObjectNode(NODE_TYPE_JOINT,jnt,kNode,kM,false);
		objectList.Append(opNode);
		
		Matrix opM = KFbxXMatrixToMatrix(kM,handed);
		if(handed == 1) opM = opM * jRotM;
		
		Vector offsetT = GetTranlationOffset(kNode,handed);
		if(freezeScale) opM.off = scaM * (opM.off + offsetT);
		else opM.off += offsetT;
		
		jnt->SetMg(opM);
		
		if(!HasBindPose(kNode))
		{
			KFbxNode *prNode = kNode->GetParent();
			if(prNode)
			{
				Vector sca = CDGetScale(jnt);
				
				KFbxXMatrix prkM = prNode->GetGlobalFromDefaultTake();
				Matrix prM = KFbxXMatrixToMatrix(prkM,handed);
				
				Vector prOffsetT = GetTranlationOffset(prNode,handed);
				if(freezeScale) prM.off = scaM * (prM.off + prOffsetT);
				else prM.off += prOffsetT;
				if(handed == 1 && pr->GetType() == ID_CDJOINTOBJECT) prM = prM * jRotM;
				
				Matrix jLocalM = MInv(prM) * opM;
				jnt->SetMl(jLocalM);
				CDSetScale(jnt,sca);
				opM = jnt->GetMg();
			}
		}
		
		if(jnt->GetUp())
		{
			if(pr->GetType() == ID_CDJOINTOBJECT)
			{
				BaseContainer *jData = jnt->GetDataInstance();
				if(jData)
				{
					if(!jnt->GetPred()) jData->SetBool(CDJ_CONNECTED,true);
					else jData->SetBool(CDJ_CONNECTED,false);
					
					BaseContainer *prData = pr->GetDataInstance();
					if(prData)
					{
						Vector prPos = GetGlobalPosition((BaseObject*)pr);
						Real jLen = Len(opM.off - prPos);
						
						prData->SetReal(CDJ_JOINT_RADIUS,jLen*0.1);
						jData->SetReal(CDJ_JOINT_RADIUS,jLen*0.1);
					}
				}
			}
		}
	}
	
	return jnt;
}

BaseObject* CDImportFBXCommand::KFbxNodeToMesh(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode)
{
	BaseObject *op = BaseObject::Alloc(Opolygon);
	if(op)
	{
		String oName = CharToString(kNode->GetName());
		StatusSetText("Loading mesh - "+oName);
		
		op->SetName(oName);
		op->InsertUnderLast(pr);
		
		KFbxXMatrix kM = kNode->GetGlobalFromDefaultTake();
		KFbxMesh *pMesh = (KFbxMesh*)kNode->GetNodeAttribute();
		if(pMesh)
		{
			Bool skinned = false;
			KFbxSkin* lSkinDeformer = reinterpret_cast<KFbxSkin*>(pMesh->GetDeformer(0, KFbxDeformer::eSKIN));
			if(lSkinDeformer) skinned = true;
			
			ObjectNode opNode = ObjectNode(NODE_TYPE_MESH,op,kNode,kM,skinned);
			objectList.Append(opNode);
			
			Matrix opM = KFbxXMatrixToMatrix(kM,handed);
			
			Vector offsetT = GetTranlationOffset(kNode,handed);
			if(freezeScale) opM.off = scaM * (opM.off + offsetT);
			else opM.off += offsetT;
			
			op->SetMg(opM);
			
			if(!HasBindPose(kNode))
			{
				KFbxNode *prNode = kNode->GetParent();
				if(prNode)
				{
					Vector sca = CDGetScale(op);
					
					KFbxXMatrix prkM = prNode->GetGlobalFromDefaultTake();
					Matrix prM = KFbxXMatrixToMatrix(prkM,handed);
					
					Vector prOffsetT = GetTranlationOffset(prNode,handed);
					if(freezeScale) prM.off = scaM * (prM.off + prOffsetT);
					else prM.off += prOffsetT;
					if(handed == 1 && pr->GetType() == ID_CDJOINTOBJECT) prM = prM * jRotM;
					
					Matrix localM = MInv(prM) * opM;
					op->SetMl(localM);
					CDSetScale(op,sca);
					opM = op->GetMg();
				}
			}
			
			int kvCnt = pMesh->GetControlPointsCount();
			int kpCnt = pMesh->GetPolygonCount();
			
			LONG cpCnt = GetValidPolygonCount(pMesh,kpCnt);
			ToPoly(op)->ResizeObject(kvCnt,cpCnt);
			
			int i;
			
			KFbxXMatrix offKM = GetGeometricPivotMatrix(kNode);
			Vector *padr = GetPointArray(op);
			if(padr)
			{
				KFbxVector4 *lControlPoints = pMesh->GetControlPoints();
				if(lControlPoints)
				{
					for(i=0; i<kvCnt; i++)
					{
						StatusSetSpin();
						KFbxVector4 kPt = offKM.MultT(lControlPoints[i]);
						Vector pt = KFbxVector4ToVector(kPt);
						if(handed == 1) pt.z *= -1;
						if(freezeScale) padr[i] = scaM * (pt - offsetT);
						else padr[i] = pt - offsetT;
					}
				}
			}
			
			UVWTag *uvTag = NULL;
			KFbxLayerElementArrayTemplate<KFbxVector2> *UVs = NULL;
			KFbxLayerElement::EMappingMode lMappingMode = KFbxLayerElement::eNONE;
			
			if(pMesh->GetTextureUV(&UVs,KFbxLayerElement::eDIFFUSE_TEXTURES))
			{
				KFbxLayer *kLayer = pMesh->GetLayer(0);
				if(kLayer)
				{
					KFbxLayerElementUV *kuv = kLayer->GetUVs();
					if(kuv)
					{
						lMappingMode = kuv->GetMappingMode();
						uvTag = UVWTag::Alloc(kpCnt);
						op->InsertTag(uvTag,NULL);
					}
				}
			}
			
			CPolygon *vadr = GetPolygonArray(op);
			if(vadr)
			{
				AutoAlloc<Modeling> mod;
				if(!mod || !mod->InitObject(op)) return op;
				
				ngonList.Init();
				
				LONG plyInd = 0;
				LONG ngnCnt = 0;
				if(polygonList.Alloc(kpCnt))
				{
					polygonList.Fill(-1);
					
					for(i=0; i<kpCnt; i++)
					{
						StatusSetSpin();
						LONG pSize = pMesh->GetPolygonSize(i);
						
						UVWStruct uv;
						if(uvTag) CDGetUVWStruct(uvTag,uv,i);
						
						LONG p;
						CDArray<LONG> pInd;
						if(pInd.Alloc(pSize))
						{
							if(pSize > 4) // handle ngons
							{
								NgonUVW ngnUv = NgonUVW(ngnCnt,i);
								for(p=0; p<pSize; p++)
								{
									if(handed == 1) pInd[pSize-1-p] = pMesh->GetPolygonVertex(i,p);
									else pInd[p] = pMesh->GetPolygonVertex(i,p);
								}
								ngonList.Append(ngnUv);
								ngnCnt++;
								
								mod->CreateNgon(op,pInd.Array(),pSize);
							}
							else
							{
								CDArray<Vector> uvPt;
								if(uvPt.Alloc(pSize))
								{
									for(p=0; p<pSize; p++)
									{
										LONG uvInd;
										pInd[p] = pMesh->GetPolygonVertex(i,p);
										
										if(uvTag)
										{
											if(lMappingMode == KFbxLayerElement::eBY_POLYGON_VERTEX)
											{
												uvInd = pMesh->GetTextureUVIndex(i,p);
												uvPt[p] = Vector((Real)UVs->GetAt(uvInd).mData[0], 1.0 - (Real)UVs->GetAt(uvInd).mData[1], 1.0);
											}
											else
											{
												uvInd = pMesh->GetPolygonVertex(i,p);
												uvPt[p] = Vector((Real)UVs->GetAt(uvInd).mData[0], 1.0 - (Real)UVs->GetAt(uvInd).mData[1], 1.0);
											}
										}
									}
									
									if(pSize == 4 && plyInd < cpCnt)
									{
										if(handed == 1) vadr[plyInd] = CPolygon(pInd[3], pInd[2], pInd[1], pInd[0]);
										else vadr[plyInd] = CPolygon(pInd[0], pInd[1], pInd[2], pInd[3]);
										
										if(uvTag)
										{
											if(handed == 1) uv = UVWStruct(uvPt[3], uvPt[2], uvPt[1], uvPt[0]);
											else uv = UVWStruct(uvPt[0], uvPt[1], uvPt[2], uvPt[3]);
											CDSetUVWStruct(uvTag,uv,plyInd);
										}
									}
									else if(pSize == 3 && plyInd < cpCnt)
									{
										if(handed == 1) vadr[plyInd] = CPolygon(pInd[2], pInd[1], pInd[0]);
										else vadr[plyInd] = CPolygon(pInd[0], pInd[1], pInd[2]);
										
										if(uvTag)
										{
											if(handed == 1) uv = UVWStruct(uvPt[2], uvPt[1], uvPt[0]);
											else uv = UVWStruct(uvPt[0], uvPt[1], uvPt[2]);
											CDSetUVWStruct(uvTag,uv,plyInd);
										}
									}
									polygonList[i] = plyInd;
									plyInd++;
									uvPt.Free();
								}
							}
							pInd.Free();
						}
					}
					if(mod) mod->Commit(op);
					
					LONG ngoncount, *polymap = NULL;
					LONG **ngons = NULL;
					
					Bool mapping = true;
					if(!ToPoly(op)->GetPolygonTranslationMap(ngoncount, polymap))
					{
						CDFree(polymap);
						mapping =  false;
					}
					
					if(!ToPoly(op)->GetNGonTranslationMap(ngoncount, polymap, ngons))
					{
						CDFree(polymap);
						CDFree(ngons);
						mapping =  false;
					}
					
					if(mapping)
					{
						vadr = GetPolygonArray(op);
						uvTag = (UVWTag*)op->GetTag(Tuvw);
						if(uvTag && lMappingMode == KFbxLayerElement::eBY_POLYGON_VERTEX)
						{
							LONG kpCnt = ToPoly(op)->GetPolygonCount();
							NgonUVW *nlist = ngonList.Array();
							for(int n=0; n<ngonList.Size(); n++)
							{
								if(nlist)
								{
									LONG kply = nlist[n].kPolyID;
									LONG nid = nlist[n].ngonID;
									
									LONG pSize = pMesh->GetPolygonSize(kply);
									
									LONG p, k;
									for(p=1; p<=ngons[nid][0]; p++)
									{
										StatusSetSpin();
										LONG plyID = ngons[nid][p];
										if(plyID < kpCnt)
										{
											CPolygon ply = vadr[plyID];
											
											UVWStruct uv;
											if(uvTag) CDGetUVWStruct(uvTag, uv,plyID);
											
											for(k=0; k<pSize; k++)
											{
												LONG kpInd = pMesh->GetPolygonVertex(kply,k);
												LONG uvInd = pMesh->GetTextureUVIndex(kply,k);
												
												if(ply.a == kpInd)
												{
													uv.a = Vector((Real)UVs->GetAt(uvInd).mData[0], 1.0 - (Real)UVs->GetAt(uvInd).mData[1], 1.0);
												}
												else if(ply.b == kpInd)
												{
													uv.b = Vector((Real)UVs->GetAt(uvInd).mData[0], 1.0 - (Real)UVs->GetAt(uvInd).mData[1], 1.0);
												}
												else if(ply.c == kpInd)
												{
													uv.c = Vector((Real)UVs->GetAt(uvInd).mData[0], 1.0 - (Real)UVs->GetAt(uvInd).mData[1], 1.0);
												}
												else if(ply.d == kpInd)
												{
													uv.d = Vector((Real)UVs->GetAt(uvInd).mData[0], 1.0 - (Real)UVs->GetAt(uvInd).mData[1], 1.0);
												}
											}
											CDSetUVWStruct(uvTag,uv,plyID);
										}
									}
								}
							}
						}
					}
				}
			}
			
			op->Message(MSG_UPDATE);
			
			BaseTag *phTag = BaseTag::Alloc(Tphong);
			if(phTag)
			{
				op->InsertTag(phTag,NULL);
				BaseContainer *tData = phTag->GetDataInstance();
				if(tData)
				{
					tData->SetBool(PHONGTAG_PHONG_ANGLELIMIT,true);
					tData->SetReal(PHONGTAG_PHONG_ANGLE,1.396);
					tData->SetBool(PHONGTAG_PHONG_USEEDGES,false);
				}
			}
			
			// get smoothing
			if(importSmoothing)
			{
				KFbxLayer* lLayer = pMesh->GetLayer(0);
				if(lLayer)
				{
					KFbxLayerElementSmoothing* lSmoothingElement = lLayer->GetSmoothing();
					if(lSmoothingElement)
					{
						BaseSelect *edgeSel = ToPoly(op)->GetEdgeS();
						if(edgeSel)
						{
							edgeSel->DeselectAll();
							
							StatusSetText("Loading mesh - "+oName+" - import smoothing data");
							if(lSmoothingElement->GetMappingMode() == KFbxLayerElement::eBY_EDGE)
							{
								for(int lEdgeIndex = 0; lEdgeIndex < pMesh->GetMeshEdgeCount(); lEdgeIndex++)
								{
									StatusSetSpin();
									int lSmoothingIndex = -1;
									
									// get smoothing by the index of edge
									if(lSmoothingElement->GetReferenceMode() == KFbxLayerElement::eDIRECT) lSmoothingIndex = lEdgeIndex;
									
									// get smoothing by the index-to-direct
									if(lSmoothingElement->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT)
										lSmoothingIndex = lSmoothingElement->GetIndexArray().GetAt(lEdgeIndex);
									
									if(lSmoothingIndex > -1)
									{
										int lSmoothingFlag = lSmoothingElement->GetDirectArray().GetAt(lSmoothingIndex);
										if(lSmoothingFlag == 0)
										{
											int aInd, bInd;
											pMesh->GetMeshEdgeVertices(lEdgeIndex,aInd,bInd);
											SelectEdgesFromPoints(edgeSel,op,padr,vadr,aInd,bInd);
										}
									}
								}
							}
							else if(lSmoothingElement->GetMappingMode() == KFbxLayerElement::eBY_POLYGON)
							{
								KFbxGeometryConverter lConverter(pSdkManager);
								if(lConverter.ComputeEdgeSmoothingFromPolygonSmoothing(pMesh))
								{
									int eCnt = pMesh->GetMeshEdgeCount();
									for(int lEdgeIndex = 0; lEdgeIndex < eCnt ; lEdgeIndex++)
									{
										StatusSetSpin();
										int lSmoothingIndex = -1;
										
										// get smoothing by the index of edge
										if(lSmoothingElement->GetReferenceMode() == KFbxLayerElement::eDIRECT) lSmoothingIndex = lEdgeIndex;
										
										// get smoothing by the index-to-direct
										if(lSmoothingElement->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT)
											lSmoothingIndex = lSmoothingElement->GetIndexArray().GetAt(lEdgeIndex);
										
										if(lSmoothingIndex > -1)
										{
											int lSmoothingFlag = lSmoothingElement->GetDirectArray().GetAt(lSmoothingIndex);
											if(lSmoothingFlag == 0)
											{
												int aInd, bInd;
												pMesh->GetMeshEdgeVertices(lEdgeIndex,aInd,bInd);
												SelectEdgesFromPoints(edgeSel,op,padr,vadr,aInd,bInd);
											}
										}
									}
								}
							}
							
							if(edgeSel->GetCount() > 0)
							{
								LONG mode = doc->GetMode();
								
								doc->SetMode(Medges);
								doc->SetActiveObject(op);
								BreakPhongShading();
								
								doc->SetMode(mode);
							}
						}
					}
				}
			}
			
			if(importMaterial)
			{
				StatusSetText("Loading mesh - "+oName+" - importing materials");
				LoadMaterials(doc,kNode,op);
				
				StatusSetText("Loading mesh - "+oName+" - Building material mappings");
				SetMaterialMapping(kNode,op);
			}
			
			LONG shapeCnt = pMesh->GetShapeCount();
			if(importMorph && shapeCnt > 0) GetMorphShapes(doc,op,pMesh,shapeCnt,kvCnt);
			
			polygonList.Free();
			ngonList.Free();
		}
	}
	
	return op;
}

BaseObject* CDImportFBXCommand::KFbxNodeToCamera(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode)
{
	BaseObject *retOp = NULL, *cam = NULL, *camPr = NULL;
	TakeNode *tlist = takeList.Array();
		
	KFbxCamera *lCamera = (KFbxCamera*)kNode->GetNodeAttribute();
	if(lCamera)
	{
		if(takeList.Size() > 0)
		{
			char *tName = StringToChar(tlist[curTake].name);
			KFCurve* fc = lCamera->Roll.GetKFCurve(NULL, tName);
			if(fc && fc->KeyGetCount() > 0) camPr = BaseObject::Alloc(Onull);
			CDFree(tName);
		}
		
		cam = BaseObject::Alloc(Ocamera);
		if(cam)
		{
			KFbxXMatrix kM = kNode->GetGlobalFromDefaultTake();
			Matrix opM, fbxM = KFbxXMatrixToMatrix(kM,handed);
			
			opM.v1 = -fbxM.v3;
			opM.v2 = fbxM.v2;
			opM.v3 = fbxM.v1;
			
			fbxM.off = GetRotationalOffset(kNode,Matrix(fbxM.off,opM.v1,opM.v2,opM.v3));
			
			if(freezeScale) opM.off = scaM * fbxM.off;
			else opM.off = fbxM.off;
			
			
			String cName = CharToString(kNode->GetName());
			StatusSetText("Loading Camera - "+cName);
			StatusSetSpin();
			
			cam->SetName(cName);
			if(camPr)
			{
				cName = CharToString(kNode->GetName())+".Parent";
				camPr->SetName(cName);
				camPr->InsertUnderLast(pr);
				cam->InsertUnderLast(camPr);
			}
			else cam->InsertUnderLast(pr);
			
			if(camPr) camPr->SetMg(opM);
			cam->SetMg(opM);
			
			if(camPr)
			{
				ObjectNode prnode = ObjectNode(NODE_TYPE_CAMERA_PARENT,camPr,kNode,kM,false);
				objectList.Append(prnode);
			}
			ObjectNode opNode = ObjectNode(NODE_TYPE_CAMERA,cam,kNode,kM,false);
			objectList.Append(opNode);
			
			BaseContainer *cmData = cam->GetDataInstance();
			if(cmData)
			{
				if(lCamera->ProjectionType.Get() == KFbxCamera::ePERSPECTIVE)
				{
					Real fLen = (Real)lCamera->FocalLength.Get();
					Real aptW = (Real)lCamera->GetApertureWidth(); // value in inches
					Real aptH = (Real)lCamera->GetApertureHeight(); // value in inches
					
					cmData->SetLong(CAMERA_PROJECTION,Pperspective);
					cmData->SetReal(CAMERA_FOCUS,fLen);
					cmData->SetReal(CAMERAOBJECT_APERTURE,InchToMillimeter(aptW));
					
				#ifdef __PC
					fbxDouble2 filmOffset = lCamera->FilmOffset.Get();
					Real fxOffset = InchToMillimeter(Real(filmOffset.mData[0]));
					Real fyOffset = InchToMillimeter(Real(filmOffset.mData[1]));
				#else
					Real fxOffset = InchToMillimeter(Real(lCamera->FilmOffsetX.Get()));
					Real fyOffset = InchToMillimeter(Real(lCamera->FilmOffsetY.Get()));
				#endif
					
					if(fxOffset != 0.0) cmData->SetReal(CAMERAOBJECT_FILM_OFFSET_X,aptW/fxOffset);
					if(fyOffset != 0.0) cmData->SetReal(CAMERAOBJECT_FILM_OFFSET_Y,aptH/fyOffset);

					cmData->SetReal(CAMERAOBJECT_TARGETDISTANCE,Real(lCamera->FocusDistance.Get()));
				}
			}
			
			RenderData *rData = doc->GetActiveRenderData();
			if(rData)
			{
				BaseContainer *rSettings = rData->GetDataInstance();
				if(rSettings)
				{
					Real fxSize = Real(lCamera->GetApertureWidth());
					Real fySize = Real(lCamera->GetApertureHeight());
					
				#if API_VERSION < 11000
					rSettings->SetLong(RDATA_RESOLUTION,0);
				#else
					rSettings->SetLong(RDATA_RESOLUTION_EX,0);
				#endif
					rSettings->SetReal(RDATA_XRES, InchToMillimeter(fxSize));
					rSettings->SetReal(RDATA_YRES, InchToMillimeter(fySize));
				}
			}
		}
	}
	if(camPr) retOp = camPr;
	else retOp = cam;
	
	return retOp;
}

BaseObject* CDImportFBXCommand::KFbxNodeToLight(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode)
{
	BaseObject *lt = BaseObject::Alloc(Olight);
	if(lt)
	{
		KFbxLight *pLight = (KFbxLight*)kNode->GetNodeAttribute();
		if(pLight)
		{
			String lName = CharToString(kNode->GetName());
			StatusSetText("Loading Light - "+lName);
			StatusSetSpin();
			
			lt->SetName(lName);
			lt->InsertUnderLast(pr);
			
			KFbxXMatrix kM = kNode->GetGlobalFromDefaultTake();
			Matrix opM, fbxM = KFbxXMatrixToMatrix(kM,handed);
			opM = fbxM;
			
			Vector offsetT = GetTranlationOffset(kNode,handed);
			
			if(freezeScale) opM.off = scaM * (fbxM.off + offsetT);
			else opM.off = fbxM.off + offsetT;
			
			if(pLight->LightType.Get() == KFbxLight::eSPOT)
			{
				opM.v1 = fbxM.v3;
				opM.v2 = -fbxM.v1;
				opM.v3 = -fbxM.v2;
			}
			
			lt->SetMg(opM);
			
			KFbxNode *prNode = kNode->GetParent();
			if(prNode)
			{
				Vector sca = CDGetScale(lt);
				
				KFbxXMatrix prkM = prNode->GetGlobalFromDefaultTake();
				Matrix prM = KFbxXMatrixToMatrix(prkM,handed);
				
				Vector prOffsetT = GetTranlationOffset(prNode,handed);
				if(freezeScale) prM.off = scaM * (prM.off + prOffsetT);
				else prM.off += prOffsetT;
				if(handed == 1 && pr->GetType() == ID_CDJOINTOBJECT) prM = prM * jRotM;
				
				Matrix localM = MInv(prM) * opM;
				lt->SetMl(localM);
				CDSetScale(lt,sca);
				opM = lt->GetMg();
			}

			ObjectNode opNode = ObjectNode(NODE_TYPE_LIGHT,lt,kNode,kM,false);
			objectList.Append(opNode);

			GeData d;
			if(pLight->LightType.Get() == KFbxLight::ePOINT)
			{	
				d = LIGHT_TYPE_OMNI;
				CDSetParameter(lt,DescLevel(LIGHT_TYPE),d);
			}
			else if(pLight->LightType.Get() == KFbxLight::eDIRECTIONAL)
			{	
				d = LIGHT_TYPE_PARALLEL;
				CDSetParameter(lt,DescLevel(LIGHT_TYPE),d);
			}
			else if(pLight->LightType.Get() == KFbxLight::eSPOT)
			{	
				d = LIGHT_TYPE_SPOT;
				CDSetParameter(lt,DescLevel(LIGHT_TYPE),d);
				
				d = Rad((Real)pLight->HotSpot.Get());
				CDSetParameter(lt,DescLevel(LIGHT_DETAILS_INNERANGLE),d);

				d = Rad((Real)pLight->ConeAngle.Get());
				CDSetParameter(lt,DescLevel(LIGHT_DETAILS_OUTERANGLE),d);
			}
			
			Real intensity = pLight->Intensity.Get();
			d = intensity * 0.01;
			CDSetParameter(lt,DescLevel(LIGHT_BRIGHTNESS),d);

			KFbxColor lightColor;
			lightColor.mRed = pLight->Color.Get()[0];
			lightColor.mGreen = pLight->Color.Get()[1];
			lightColor.mBlue = pLight->Color.Get()[2];
			
			d = Vector((Real)lightColor.mRed, (Real)lightColor.mGreen, (Real)lightColor.mBlue);
			CDSetParameter(lt,DescLevel(LIGHT_COLOR),d);
			
			if(pLight->DecayType.Get() == KFbxLight::eNONE)
			{
				d = LIGHT_DETAILS_FALLOFF_NONE;
				CDSetParameter(lt,DescLevel(LIGHT_DETAILS_FALLOFF),d);
			}
			else
			{
				if(pLight->DecayType.Get() == KFbxLight::eLINEAR)
				{
					d = LIGHT_DETAILS_FALLOFF_LINEAR;
					CDSetParameter(lt,DescLevel(LIGHT_DETAILS_FALLOFF),d);
				}
				else if(pLight->DecayType.Get() == KFbxLight::eQUADRATIC)
				{
					d = LIGHT_DETAILS_FALLOFF_INVERSESQUARE;
					CDSetParameter(lt,DescLevel(LIGHT_DETAILS_FALLOFF),d);
				}
			}
		}
	}
	
	return lt;
}


BaseObject* CDImportFBXCommand::KFbxNodeToNull(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode)
{
	BaseObject *op = BaseObject::Alloc(Onull);
	if(op)
	{
		String oName = CharToString(kNode->GetName());
		StatusSetText("Loading Null - "+oName);
		StatusSetSpin();
		
		op->SetName(oName);
		op->InsertUnderLast(pr);
		
		KFbxNodeAttribute *lAttribute =  kNode->GetNodeAttribute();
		if(lAttribute)
		{
			fbxDouble3 clr = lAttribute->Color.Get();
			
			ObjectColorProperties prop;
			op->GetColorProperties(&prop);
			prop.xray = false;
			prop.color = Vector(clr[0],clr[1],clr[2]);
			prop.usecolor = 2;
			op->SetColorProperties(&prop);
		}

		KFbxXMatrix kM = kNode->GetGlobalFromDefaultTake();
		Matrix opM = KFbxXMatrixToMatrix(kM,handed);
		
		Vector offsetT = GetTranlationOffset(kNode,handed);
		
		if(freezeScale) opM.off = scaM * (opM.off + offsetT);
		else opM.off += offsetT;
		
		op->SetMg(opM);
		
		KFbxNode *prNode = kNode->GetParent();
		if(prNode)
		{
			Vector sca = CDGetScale(op);
			
			KFbxXMatrix prkM = prNode->GetGlobalFromDefaultTake();
			Matrix prM = KFbxXMatrixToMatrix(prkM,handed);
			
			Vector prOffsetT = GetTranlationOffset(prNode,handed);
			if(freezeScale) prM.off = scaM * (prM.off + prOffsetT);
			else prM.off += prOffsetT;
			if(handed == 1 && pr->GetType() == ID_CDJOINTOBJECT) prM = prM * jRotM;
			
			Matrix localM = MInv(prM) * opM;
			op->SetMl(localM);
			CDSetScale(op,sca);
			opM = op->GetMg();
		}
		
		ObjectNode opNode = ObjectNode(NODE_TYPE_NULL,op,kNode,kM,false);
		objectList.Append(opNode);
	}
	
	return op;
}

BaseObject* CDImportFBXCommand::KFbxNodeToSpline(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode)
{
	BaseObject *op = BaseObject::Alloc(Ospline);
	if(op)
	{
		BaseContainer *oData = op->GetDataInstance();
		
		KFbxNurbsCurve *pNCurve = (KFbxNurbsCurve*)kNode->GetNodeAttribute();
		if(pNCurve)
		{
			String lName = CharToString(kNode->GetName());
			StatusSetText("Loading Spline - "+lName);
			StatusSetSpin();
			
			op->SetName(lName);
			op->InsertUnderLast(pr);
			
			KFbxNodeAttribute *lAttribute =  kNode->GetNodeAttribute();
			if(lAttribute)
			{
				fbxDouble3 clr = lAttribute->Color.Get();
				
				ObjectColorProperties prop;
				op->GetColorProperties(&prop);
				prop.xray = false;
				prop.color = Vector(clr[0],clr[1],clr[2]);
				prop.usecolor = 2;
				op->SetColorProperties(&prop);
			}
			
			KFbxXMatrix kM = kNode->GetGlobalFromDefaultTake();
			Matrix opM = KFbxXMatrixToMatrix(kM,handed);
			
			Vector offsetT = GetTranlationOffset(kNode,handed);
			
			if(freezeScale) opM.off = scaM * (opM.off + offsetT);
			else opM.off += offsetT;
			
			op->SetMg(opM);

			KFbxNode *prNode = kNode->GetParent();
			if(prNode)
			{
				Vector sca = CDGetScale(op);
				
				KFbxXMatrix prkM = prNode->GetGlobalFromDefaultTake();
				Matrix prM = KFbxXMatrixToMatrix(prkM,handed);
				
				Vector prOffsetT = GetTranlationOffset(prNode,handed);
				if(freezeScale) prM.off = scaM * (prM.off + prOffsetT);
				else prM.off += prOffsetT;
				if(handed == 1 && pr->GetType() == ID_CDJOINTOBJECT) prM = prM * jRotM;
				
				Matrix localM = MInv(prM) * opM;
				op->SetMl(localM);
				CDSetScale(op,sca);
				opM = op->GetMg();
			}
			
			ObjectNode opNode = ObjectNode(NODE_TYPE_SPLINE,op,kNode,kM,false);
			objectList.Append(opNode);
			
			switch(pNCurve->GetType())
			{
				case KFbxNurbsCurve::eOPEN:
					oData->SetBool(SPLINEOBJECT_CLOSED,false);
					break;
				case KFbxNurbsCurve::eCLOSED:
					oData->SetBool(SPLINEOBJECT_CLOSED,true);
					break;
				case KFbxNurbsCurve::ePERIODIC:
					oData->SetBool(SPLINEOBJECT_CLOSED,true);
					oData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_BSPLINE);
					break;
			}
			
			
			if(pNCurve->IsBezier())
			{
				if(pNCurve->IsPolyline())
				{
					oData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_LINEAR);
				}
				else
				{
					oData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_BEZIER);
				}
			}
			if(pNCurve->IsRational())
			{
				oData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_BSPLINE);
			}
			
			oData->SetLong(SPLINEOBJECT_INTERPOLATION,SPLINEOBJECT_INTERPOLATION_ADAPTIVE);
			
			LONG pCnt = pNCurve->GetControlPointsCount();
			SplineObject *spl = op->GetRealSpline();
			spl->ResizeObject(pCnt,0);
			
			Vector *padr = GetPointArray(op);
			if(padr)
			{
				KFbxVector4 *ncrvPts = pNCurve->GetControlPoints();
				if(ncrvPts)
				{
					for(int i=0; i<pCnt; i++)
					{
						Vector pt = KFbxVector4ToVector(ncrvPts[i]);
						if(handed == 1) pt.z *= -1;
						if(freezeScale) padr[i] = scaM * (pt - offsetT);
						else padr[i] = pt - offsetT;
					}
				}
			}
			spl->SetDefaultCoeff();
			op->Message(MSG_UPDATE);
		}
	}
	
	return op;
}

void CDImportFBXCommand::SetCameraTargets(void)
{
	TakeNode *tlist = takeList.Array();
	int i, j;
	ObjectNode *oList = objectList.Array();
	for(i=0; i<objectList.Size(); i++)
	{
		KFbxNode *kNode = oList[i].node;
		BaseObject *cam = oList[i].object;
		
		if(oList[i].nodeType != NODE_TYPE_CAMERA) continue;
		
		KFbxCamera *lCamera = (KFbxCamera*)kNode->GetNodeAttribute();
		if(lCamera)
		{
			KFbxNode *camTarg = kNode->GetTarget();
			if(camTarg)
			{
				for(j=0; j<objectList.Size(); j++)
				{
					StatusSetSpin();
					if(camTarg == oList[j].node)
					{
						BaseObject *trg = oList[j].object;
						if(trg)
						{
							BaseTag *trgTag = BaseTag::Alloc(Ttargetexpression);
							if(trgTag)
							{
								char *tName = StringToChar(tlist[curTake].name);
								KFCurve* fc = lCamera->Roll.GetKFCurve(NULL,tName);
								if(fc && fc->KeyGetCount() > 0) cam->GetUp()->InsertTag(trgTag);
								else cam->InsertTag(trgTag);
								CDFree(tName);
								
								BaseContainer *tData = trgTag->GetDataInstance();
								if(tData) tData->SetLink(TARGETEXPRESSIONTAG_LINK,trg);
							}
							break;
						}
					}
				}
			}
		}
	}
}

void CDImportFBXCommand::SetLightTargets(void)
{
	int i, j;
	ObjectNode *oList = objectList.Array();
	for(i=0; i<objectList.Size(); i++)
	{
		KFbxNode *kNode = oList[i].node;
		BaseObject *cam = oList[i].object;
		
		if(oList[i].nodeType != NODE_TYPE_LIGHT) continue;
		
		KFbxNode *ltTarg = kNode->GetTarget();
		if(ltTarg)
		{
			for(j=0; j<objectList.Size(); j++)
			{
				StatusSetSpin();
				if(ltTarg == oList[j].node)
				{
					BaseObject *trg = oList[j].object;
					if(trg)
					{
						BaseTag *trgTag = BaseTag::Alloc(Ttargetexpression);
						if(trgTag)
						{
							cam->InsertTag(trgTag);
							BaseContainer *tData = trgTag->GetDataInstance();
							if(tData) tData->SetLink(TARGETEXPRESSIONTAG_LINK,trg);
						}
						break;
					}
				}
			}
		}
	}
}

kFCurveIndex CDImportFBXCommand::GetKeyIndex(KTime time, KFCurve *fc)
{
	kFCurveIndex keyIndex = fc->KeyFind(time);
	if(keyIndex > -1)
	{
		KTime keyTime = fc->KeyGet(keyIndex).GetTime();
		
		if(keyTime.GetFramedTime() != time.GetFramedTime())
		{
			int kCount = fc->KeyGetCount();
			if(keyIndex < kCount-1)
			{
				KTime nkeyTime = fc->KeyGet(keyIndex+1).GetTime();
				if(nkeyTime.GetFramedTime() == time.GetFramedTime()) keyIndex++;
			}
		}
	}
	
	return keyIndex;
}

Bool CDImportFBXCommand::HasKeyAtTime(KTime time, KFCurve *fc)
{
	kFCurveIndex keyIndex = GetKeyIndex(time,fc);
	if(keyIndex < 0) return false;
	
	KFCurveKey key = fc->KeyGet(keyIndex);
	KTime keyTime = key.GetTime();
	
	if(keyTime.GetFramedTime() == time.GetFramedTime()) return true;
	
	return false;
}

LONG CDImportFBXCommand::GetKeyInterpolation(KTime time, KFCurve *fc)
{
	kFCurveIndex kInd = GetKeyIndex(time,fc);
	if(kInd >= 0)
	{
		kFCurveInterpolation lInterpolationType = fc->KeyGetInterpolation(kInd);
		switch(lInterpolationType)
		{
			case KFCURVE_INTERPOLATION_CUBIC:
				return CD_SPLINE_INTERPOLATION;
			case KFCURVE_INTERPOLATION_LINEAR:
				return CD_LINEAR_INTERPOLATION;
			case KFCURVE_INTERPOLATION_CONSTANT:
				return CD_STEP_INTERPOLATION;
		}
	}

	return CD_SPLINE_INTERPOLATION;
}

Bool CDImportFBXCommand::SetKeyTangents(BaseDocument *doc, BaseList2D *op, const BaseTime &docTime, DescID dscID, KTime time, KFCurve *fc, Bool rad, Real scale)
{
	if(!op) return false;
	
	Real fps = doc->GetFps();
	LONG frm = docTime.GetFrame(fps);
	
	kFCurveIndex keyIndex = GetKeyIndex(time,fc);
	int kCount = fc->KeyGetCount();
	KTime::ETimeMode tMode = pScene->GetGlobalTimeSettings().GetTimeMode();

#if API_VERSION < 9800
	BaseTrack *track = op->FindTrack(dscID); if(!track) return false;
	BaseSequence *seq = NULL;
	for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
	{
		if(docTime >= seq->GetT1() && docTime <= seq->GetT2()) break;
	}
	if(!seq) return false;
	BaseKey *key = seq->FindKey(docTime); if(!key) return false;
	AnimValue *av = GetKeyValue(key); if(!av) return false;
#else
	CTrack *track = op->FindCTrack(dscID); if(!track) return false;
	CCurve *crv = track->GetCurve(); if(!crv) return false;
	CKey *key = crv->FindKey(docTime); if(!key) return false;
#endif	
	
	GeData dT = GeData(true);
	GeData dF = GeData(false);
	
#if API_VERSION > 9799
	CDSetParameter(key,DescLevel(ID_CKEY_AUTO),dF);
	CDSetParameter(key,DescLevel(ID_CKEY_CLAMP),dF);
	CDSetParameter(key,DescLevel(ID_CKEY_LOCK_O),dF);
	CDSetParameter(key,DescLevel(ID_CKEY_LOCK_L),dF);
	#if API_VERSION < 13000
		CDSetParameter(key,DescLevel(ID_CKEY_ZERO_O),dF);
		CDSetParameter(key,DescLevel(ID_CKEY_ZERO_L),dF);
	#endif
#endif
	
	KFCurveTangeantInfo lTInfo = fc->KeyGetLeftDerivativeInfo(keyIndex);
	KFCurveTangeantInfo rTInfo = fc->KeyGetRightDerivativeInfo(keyIndex);
	
#if API_VERSION > 9799	
	if(lTInfo.mDerivative != rTInfo.mDerivative || lTInfo.mWeight != rTInfo.mWeight)
		CDSetParameter(key,DescLevel(ID_CKEY_BREAK),dT);
#endif
	
	// calculate left tangent
	if(keyIndex > 0)
	{
		
		KFCurveKey prvKey = fc->KeyGet(keyIndex-1);
		KTime prvKTime = prvKey.GetTime();
		LONG prvFrm = prvKTime.GetFrame(true,tMode,fps);
		
		Real ltWt = lTInfo.mWeight;
		Real lSeconds = 0.0;
		if(fps != 0) lSeconds = Real(frm-prvFrm) * ltWt/fps;
		BaseTime lTime = BaseTime(-lSeconds);
		Real lSlope = Real(lTInfo.mDerivative);
		if(handed == 1)
		{
			if(dscID == CDGetPSRTrackDescriptionID(CD_ROT_B)) lSlope *= -1;
			if(dscID == CDGetPSRTrackDescriptionID(CD_POS_Z)) lSlope *= -1;
		}
		Real lValue = (-lSeconds * Tan(ATan(lSlope))) * scale;
	#if API_VERSION < 9800
		if(rad) av->value_left = Rad(lValue);
		else av->value_left = lValue;
		av->time_left = -lSeconds;
	#else
		if(rad) key->SetValueLeft(crv,Rad(lValue));
		else key->SetValueLeft(crv,lValue);
		key->SetTimeLeft(crv,lTime);
	#endif
	}
	
	// calculate right tangent
	if(keyIndex < kCount-1)
	{
		KFCurveKey nxtKey = fc->KeyGet(keyIndex+1);
		KTime nxtKTime = nxtKey.GetTime();
		LONG nxtFrm = nxtKTime.GetFrame(true,tMode,fps);
		
		Real rtWt = rTInfo.mWeight;
		Real rSeconds = 0.0;
		if(fps != 0) rSeconds = Real(nxtFrm-frm) * rtWt/fps;
		BaseTime rTime = BaseTime(rSeconds);
		Real rSlope = Real(rTInfo.mDerivative);
		if(handed == 1)
		{
			if(dscID == CDGetPSRTrackDescriptionID(CD_ROT_B)) rSlope *= -1;
			if(dscID == CDGetPSRTrackDescriptionID(CD_POS_Z)) rSlope *= -1;
		}
		Real rValue = (rSeconds * Tan(ATan(rSlope))) * scale;
	#if API_VERSION < 9800
		if(rad) av->value_right = Rad(rValue);
		else av->value_right = rValue;
		av->time_right = rSeconds;
	#else
		if(rad) key->SetValueRight(crv,Rad(rValue));
		else key->SetValueRight(crv,rValue);
		key->SetTimeRight(crv,rTime);
	#endif
	}
	
	return true;
}

void CDImportFBXCommand::SetPositionCurveKeys(BaseDocument *doc, BaseObject *op, BaseTime docTime, KFbxNode *kNode, KTime curTime, char* takeName, Vector pos)
{
	DescID dscID;
	
	KFCurve *fc = kNode->LclTranslation.GetKFCurve(KFCURVENODE_T_X, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_POS_X);
			CDSetKey(doc,op,docTime,dscID,pos.x,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,false,imScale);
		}
	}
	
	fc = kNode->LclTranslation.GetKFCurve(KFCURVENODE_T_Y, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_POS_Y);
			CDSetKey(doc,op,docTime,dscID,pos.y,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,false,imScale);
		}
	}
	
	fc = kNode->LclTranslation.GetKFCurve(KFCURVENODE_T_Z, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_POS_Z);
			CDSetKey(doc,op,docTime,dscID,pos.z,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,false,imScale);
		}
	}
}

void CDImportFBXCommand::SetRotationCurveKeys(BaseDocument *doc, BaseObject *op, BaseTime docTime, KFbxNode *kNode, KTime curTime, char* takeName, Vector rot)
{
	DescID dscID;
	
	KFCurve *fc = kNode->LclRotation.GetKFCurve(KFCURVENODE_R_X, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_ROT_P);
			CDSetKey(doc,op,docTime,dscID,rot.y,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,true);
		}
	}
	
	fc = kNode->LclRotation.GetKFCurve(KFCURVENODE_R_Y, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_ROT_H);
			CDSetKey(doc,op,docTime,dscID,rot.x,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,true);
		}
	}
	
	fc = kNode->LclRotation.GetKFCurve(KFCURVENODE_R_Z, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_ROT_B);
			CDSetKey(doc,op,docTime,dscID,rot.z,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,true);
		}
	}
}

void CDImportFBXCommand::SetScaleCurveKeys(BaseDocument *doc, BaseObject *op, BaseTime docTime, KFbxNode *kNode, KTime curTime, char* takeName, Vector sca)
{
	DescID dscID;
	
	KFCurve *fc = kNode->LclScaling.GetKFCurve(KFCURVENODE_S_X, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_SCA_X);
			CDSetKey(doc,op,docTime,dscID,sca.x,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,false);
		}
	}
	
	fc = kNode->LclScaling.GetKFCurve(KFCURVENODE_S_Y, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_SCA_Y);
			CDSetKey(doc,op,docTime,dscID,sca.y,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,false);
		}
	}
	
	fc = kNode->LclScaling.GetKFCurve(KFCURVENODE_S_Z, takeName);
	if(fc && fc->KeyGetCount() > 0)
	{
		if(HasKeyAtTime(curTime,fc))
		{
			LONG interp = GetKeyInterpolation(curTime, fc);
			dscID = CDGetPSRTrackDescriptionID(CD_SCA_Z);
			CDSetKey(doc,op,docTime,dscID,sca.z,interp);
			
			if(interp == CD_SPLINE_INTERPOLATION)
				SetKeyTangents(doc,op,docTime,dscID,curTime,fc,false);
		}
	}
}

void CDImportFBXCommand::LoadAnimation(BaseDocument *doc, BaseObject *root)
{
	KTime::ETimeMode tMode = pScene->GetGlobalTimeSettings().GetTimeMode();
	
	Real frameRate = KTime::GetFrameRate(tMode);
	doc->SetFps(frameRate);
	
	TakeNode *tlist = takeList.Array();
	
	LONG frmStart = tlist[curTake].start;
	LONG frmEnd = tlist[curTake].end;
	int numFrames = frmEnd - frmStart;
	
	KTime gPeriod, currentTime;
	gPeriod.SetTime(0,0,0,1,0,tMode,frameRate);
	currentTime.SetTime(0,0,0,frmStart,0,tMode,frameRate);
	
	char *tName = StringToChar(tlist[curTake].name);
	KFbxTakeInfo *lCurrentTakeInfo = pScene->GetTakeInfo(tName);
	if(lCurrentTakeInfo) pScene->SetCurrentTake(tName);

	doc->SetMaxTime(BaseTime(Real(frameEnd),Real(frameRate)));
	doc->SetLoopMaxTime(BaseTime(Real(frameEnd),Real(frameRate)));
	
	StatusSetText("Loading Animation Take #"+CDLongToString(curTake));
	LONG frm = frameStart, status = 0;
	for(int f=0; f<=numFrames; f++)
	{
		if(numFrames > 0) status = LONG(Real(f)/Real(numFrames)*100.0);
		StatusSetBar(status);
		
		if(f >= frameOffset && frm <= frameEnd)
		{
			BaseTime docTime = BaseTime(Real(frm),Real(frameRate));
			
			ObjectNode *oList = objectList.Array();
			int i;
			for(i=0; i<objectList.Size(); i++)
			{
				KFbxNode *kNode = oList[i].node;
				BaseObject *op = oList[i].object;
				LONG type = oList[i].nodeType;
				
				if(lCurrentTakeInfo)
				{
					KFCurveNode *fcn = NULL;
					char* takeName = kNode->GetCurrentTakeNodeName();
					if(frm == frameStart)
					{
						// check for translation track
						fcn = kNode->LclTranslation.GetKFCurveNode(false, takeName);
						if(fcn && fcn->KeyGetCount() > 0) oList[i].prop.pos = true;
						
						// check for scale track
						fcn = kNode->LclScaling.GetKFCurveNode(false, takeName);
						if(fcn && fcn->KeyGetCount() > 0) oList[i].prop.sca = true;
						
						// check for rotation track
						fcn = kNode->LclRotation.GetKFCurveNode(false, takeName);
						if(fcn && fcn->KeyGetCount() > 0) oList[i].prop.rot = true;
						
					}
					
					// PSR tracks
					if(oList[i].prop.pos || oList[i].prop.rot || oList[i].prop.sca)
					{
						if(type == NODE_TYPE_CAMERA_PARENT)
						{
							Matrix opM, kM = KFbxXMatrixToMatrix(kNode->GetGlobalFromCurrentTake(currentTime),handed);
							if(freezeScale) opM.off = scaM * kM.off;
							else opM.off = kM.off;
							opM.v1 = -kM.v3;
							opM.v2 = kM.v2;
							opM.v3 = kM.v1;
							
							Matrix prM = Matrix();
							if(op->GetUp()) prM = op->GetUp()->GetMg();
							
							Matrix localM = MInv(prM) * opM;
							if(oList[i].prop.pos)
							{
								CDSetPos(op,localM.off);
								if(importCurves) SetPositionCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetPos(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_POSITION);
							}
							if(oList[i].prop.rot)
							{
								Vector oldRot = CDGetRot(op);
								Vector newRot = CDMatrixToHPB(localM);
								CDSetRot(op,CDGetOptimalAngle(oldRot,newRot));
								
								if(importCurves) SetRotationCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetRot(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_ROTATION);
							}
							if(oList[i].prop.sca)
							{
								CDSetScale(op,GetMatrixScale(localM));
								if(importCurves) SetScaleCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetScale(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_SCALE);
							}
						}
						else if(type == NODE_TYPE_CAMERA)
						{
							KFbxCamera *pCamera = (KFbxCamera*)kNode->GetNodeAttribute();
							if(pCamera)
							{
								KFCurve* fc = pCamera->Roll.GetKFCurve(NULL, takeName);
								if(fc && fc->KeyGetCount() > 0)
								{
									Vector rot;
									rot.z = Rad(fc->Evaluate(currentTime));
									CDSetRot(op,rot);
									if(importCurves) SetRotationCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetRot(op));
									else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_ROTATION);
								}
								else
								{
									Matrix opM, kM = KFbxXMatrixToMatrix(kNode->GetGlobalFromCurrentTake(currentTime),handed);
									opM.v1 = -kM.v3;
									opM.v2 = kM.v2;
									opM.v3 = kM.v1;
									
									kM.off = GetRotationalOffset(kNode,Matrix(kM.off,opM.v1,opM.v2,opM.v3));
									
									if(freezeScale) opM.off = scaM * kM.off;
									else opM.off = kM.off;
									
									Matrix prM = Matrix();
									if(op->GetUp()) prM = op->GetUp()->GetMg();
									
									Matrix localM = MInv(prM) * opM;
									if(oList[i].prop.pos)
									{
										CDSetPos(op,localM.off);
										if(importCurves) SetPositionCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetPos(op));
										else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_POSITION);
									}
									if(oList[i].prop.rot)
									{
										Vector oldRot = CDGetRot(op);
										Vector newRot = CDMatrixToHPB(localM);
										CDSetRot(op, CDGetOptimalAngle(oldRot,newRot));

										if(importCurves) SetRotationCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetRot(op));
										else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_ROTATION);
									}
									if(oList[i].prop.sca)
									{
										CDSetScale(op,GetMatrixScale(localM));
										if(importCurves) SetScaleCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetScale(op));
										else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_SCALE);
									}
								}
							}
						}
						else if(type == NODE_TYPE_LIGHT)
						{
							KFbxLight *pLight = (KFbxLight*)kNode->GetNodeAttribute();
							if(pLight)
							{
								Matrix opM, kM = KFbxXMatrixToMatrix(kNode->GetGlobalFromCurrentTake(currentTime),handed);
								opM = kM;
								
								if(freezeScale) opM.off = scaM * kM.off;
								else opM.off = kM.off;
								if(pLight->LightType.Get() == KFbxLight::eSPOT)
								{
									opM.v1 = kM.v3;
									opM.v2 = -kM.v1;
									opM.v3 = -kM.v2;
								}
								
								op->SetMg(opM);
								
								if(oList[i].prop.pos)
								{
									if(importCurves) SetPositionCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetPos(op));
									else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_POSITION);
								}
								
								if(oList[i].prop.rot)
								{
									if(importCurves) SetRotationCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetRot(op));
									else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_ROTATION);
								}
								
								if(oList[i].prop.sca)
								{
									if(importCurves) SetScaleCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetScale(op));
									else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_SCALE);
								}
							}
						}
						else if(type == NODE_TYPE_JOINT && handed == 1)
						{
							Matrix opM = KFbxXMatrixToMatrix(kNode->GetGlobalFromCurrentTake(currentTime),handed);
							if(freezeScale) opM.off = scaM * opM.off;
							opM = opM * jRotM;
							op->SetMg(opM);
							
							if(oList[i].prop.pos)
							{
								if(importCurves) SetPositionCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetPos(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_POSITION);
							}
							
							if(oList[i].prop.rot)
							{
								if(importCurves) SetRotationCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetRot(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_ROTATION);
							}
							
							if(oList[i].prop.sca)
							{	
								if(importCurves) SetScaleCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetScale(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_SCALE);
							}
							else
							{
								if(i == 0 && f == 0)
								{
									if(!VEqual(CDGetScale(op),Vector(1.0,1.0,1.0)))
									{
										if(importCurves) SetScaleCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetScale(op));
										else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_SCALE);
									}
								}
							}
						}
						else
						{
							Matrix opM = KFbxXMatrixToMatrix(kNode->GetGlobalFromCurrentTake(currentTime),handed);
							if(freezeScale) opM.off = scaM * opM.off;
							op->SetMg(opM);
							
							if(oList[i].prop.pos)
							{
								if(importCurves) SetPositionCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetPos(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_POSITION);
							}
							
							if(oList[i].prop.rot)
							{
								if(importCurves) SetRotationCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetRot(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_ROTATION);
							}
							
							if(oList[i].prop.sca)
							{	
								if(importCurves) SetScaleCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetScale(op));
								else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_SCALE);
							}
							else
							{
								if(i == 0 && f == 0)
								{
									if(!VEqual(CDGetScale(op),Vector(1.0,1.0,1.0)))
									{
										if(importCurves) SetScaleCurveKeys(doc,op,docTime,kNode,currentTime,takeName,CDGetScale(op));
										else CDSetVectorKeys(doc,op,docTime,CD_ID_BASEOBJECT_SCALE);
									}
								}
							}
							
						}
					}
					
					// parameter tracks
					if(type == NODE_TYPE_CAMERA)
					{
						KFbxCamera *pCamera = (KFbxCamera*)kNode->GetNodeAttribute();
						if(pCamera)
						{
							DescID dscID;
							
							// focal length
							KFCurve *fc = pCamera->FocalLength.GetKFCurve(NULL, takeName);
							if(fc && fc->KeyGetCount() > 0)
							{
								Real fLen = (Real)fc->Evaluate(currentTime);
								dscID = DescLevel(CAMERA_FOCUS);
								if(importCurves)
								{
									if(HasKeyAtTime(currentTime,fc))
									{
										LONG interp = GetKeyInterpolation(currentTime, fc);
										CDSetKey(doc,op,docTime,dscID,fLen,interp);

										if(interp == CD_SPLINE_INTERPOLATION)
											SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
									}
								}
								else CDSetKey(doc,op,docTime,dscID,fLen);
							}
							// target distance
							fc = pCamera->FocusDistance.GetKFCurve(NULL, takeName);
							if(fc && fc->KeyGetCount() > 0)
							{
								Real fDistance = (Real)fc->Evaluate(currentTime);
								dscID = DescLevel(CAMERAOBJECT_TARGETDISTANCE);
								if(importCurves)
								{
									if(HasKeyAtTime(currentTime,fc))
									{
										LONG interp = GetKeyInterpolation(currentTime, fc);
										CDSetKey(doc,op,docTime,dscID,fDistance,interp);
										
										if(interp == CD_SPLINE_INTERPOLATION)
											SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
									}
								}
								else CDSetKey(doc,op,docTime,dscID,fDistance);
							}
						}
					}
					
					if(type == NODE_TYPE_LIGHT)
					{
						KFbxLight *pLight = (KFbxLight*)kNode->GetNodeAttribute();
						if(pLight)
						{
							KFCurve *fc;
							DescID dscID;
							
							// color
							Vector ltClr;
							fc = pLight->Color.GetKFCurve(KFCURVENODE_COLOR_RED, takeName);
							if(fc && fc->KeyGetCount() > 0)
							{
								ltClr.x = fc->Evaluate(currentTime);
								dscID = DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
								if(importCurves)
								{
									if(HasKeyAtTime(currentTime,fc))
									{
										LONG interp = GetKeyInterpolation(currentTime, fc);
										CDSetKey(doc,op,docTime,dscID,ltClr.x,interp);
										
										if(interp == CD_SPLINE_INTERPOLATION)
											SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
									}
								}
								else CDSetKey(doc,op,docTime,dscID,ltClr.x);
							}
							fc = pLight->Color.GetKFCurve(KFCURVENODE_COLOR_GREEN, takeName);
							if(fc && fc->KeyGetCount() > 0)
							{
								ltClr.y = fc->Evaluate(currentTime);
								dscID = DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
								if(importCurves)
								{
									if(HasKeyAtTime(currentTime,fc))
									{
										LONG interp = GetKeyInterpolation(currentTime, fc);
										CDSetKey(doc,op,docTime,dscID,ltClr.y,interp);
										
										if(interp == CD_SPLINE_INTERPOLATION)
											SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
									}
								}
								else CDSetKey(doc,op,docTime,dscID,ltClr.y);
							}
							fc = pLight->Color.GetKFCurve(KFCURVENODE_COLOR_BLUE, takeName);
							if(fc && fc->KeyGetCount() > 0)
							{
								ltClr.z = fc->Evaluate(currentTime);
								dscID = DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
								if(importCurves)
								{
									if(HasKeyAtTime(currentTime,fc))
									{
										LONG interp = GetKeyInterpolation(currentTime, fc);
										CDSetKey(doc,op,docTime,dscID,ltClr.z,interp);
										
										if(interp == CD_SPLINE_INTERPOLATION)
											SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
									}
								}
								else CDSetKey(doc,op,docTime,dscID,ltClr.z);
							}
							
							// intensity
							fc = pLight->Intensity.GetKFCurve(NULL, takeName);
							if(fc && fc->KeyGetCount() > 0)
							{
								Real intensity = fc->Evaluate(currentTime) * 0.01;
								dscID = DescID(DescLevel(LIGHT_BRIGHTNESS));
								if(importCurves)
								{
									if(HasKeyAtTime(currentTime,fc))
									{
										LONG interp = GetKeyInterpolation(currentTime, fc);
										CDSetKey(doc,op,docTime,dscID,intensity,interp);
										
										if(interp == CD_SPLINE_INTERPOLATION)
											SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
									}
								}
								else CDSetKey(doc,op,docTime,dscID,intensity);
							}
							
							// spot light
							if(pLight->LightType.Get() == KFbxLight::eSPOT)
							{
								fc = pLight->HotSpot.GetKFCurve(NULL, takeName);
								if(fc && fc->KeyGetCount() > 0)
								{
									Real innerAngle = Rad(fc->Evaluate(currentTime));
									dscID = DescID(DescLevel(LIGHT_DETAILS_INNERANGLE));
									if(importCurves)
									{
										if(HasKeyAtTime(currentTime,fc))
										{
											LONG interp = GetKeyInterpolation(currentTime, fc);
											CDSetKey(doc,op,docTime,dscID,innerAngle,interp);
											
											if(interp == CD_SPLINE_INTERPOLATION)
												SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,true);
										}
									}
									else CDSetKey(doc,op,docTime,dscID,innerAngle);
								}
								
								fc = pLight->ConeAngle.GetKFCurve(NULL, takeName);
								if(fc && fc->KeyGetCount() > 0)
								{
									Real outerAngle = Rad(fc->Evaluate(currentTime));
									dscID = DescID(DescLevel(LIGHT_DETAILS_OUTERANGLE));
									if(importCurves)
									{
										if(HasKeyAtTime(currentTime,fc))
										{
											LONG interp = GetKeyInterpolation(currentTime, fc);
											CDSetKey(doc,op,docTime,dscID,outerAngle,interp);
											
											if(interp == CD_SPLINE_INTERPOLATION)
												SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,true);
										}
									}
									else CDSetKey(doc,op,docTime,dscID,outerAngle);
								}
							}
						}
					}
					
					// user property tracks
					if(importUserData)
					{
						if(GetUserPropertyCount(kNode) > 0)
						{
							DynamicDescription *opDD = op->GetDynamicDescription();
							if(opDD)
							{
								DescID dscID;
								const BaseContainer *dscBC;
								void *usrHnd = opDD->BrowseInit();
								
								KFbxProperty lProperty = kNode->GetFirstProperty();
								while (lProperty.IsValid())
								{
									if(lProperty.GetFlag(KFbxUserProperty::eUSER))
									{
										if(opDD->BrowseGetNext(usrHnd, &dscID, &dscBC))
										{
											if(lProperty.GetFlag(KFbxUserProperty::eANIMATABLE))
											{
												KFbxDataType lPropertyDataType = lProperty.GetPropertyDataType();
												if(lPropertyDataType.GetType() == eDOUBLE3 || lPropertyDataType.GetType() == eDOUBLE4)
												{
													DescLevel dscLevel = dscID.operator[](1);
													
													KFCurve *fc = lProperty.GetKFCurve("X", takeName);
													if(fc && fc->KeyGetCount() > 0)
													{
														DescID xdscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0),DescLevel(dscLevel.id,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
														Real value = (Real)fc->Evaluate(currentTime);
														if(importCurves)
														{
															if(HasKeyAtTime(currentTime,fc))
															{
																LONG interp = GetKeyInterpolation(currentTime, fc);
																CDSetKey(doc,op,docTime,xdscID,value,interp);
																
																if(interp == CD_SPLINE_INTERPOLATION)
																	SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
															}
														}
														else CDSetKey(doc,op,docTime,xdscID,value);
													}
													
													fc = lProperty.GetKFCurve("Y", takeName);
													if(fc && fc->KeyGetCount() > 0)
													{
														DescID ydscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0),DescLevel(dscLevel.id,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
														Real value = (Real)fc->Evaluate(currentTime);
														if(importCurves)
														{
															if(HasKeyAtTime(currentTime,fc))
															{
																LONG interp = GetKeyInterpolation(currentTime, fc);
																CDSetKey(doc,op,docTime,ydscID,value,interp);
																
																if(interp == CD_SPLINE_INTERPOLATION)
																	SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
															}
														}
														else CDSetKey(doc,op,docTime,ydscID,value);
													}
													
													fc = lProperty.GetKFCurve("Z", takeName);
													if(fc && fc->KeyGetCount() > 0)
													{
														DescID zdscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0),DescLevel(dscLevel.id,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
														Real value = (Real)fc->Evaluate(currentTime);
														if(importCurves)
														{
															if(HasKeyAtTime(currentTime,fc))
															{
																LONG interp = GetKeyInterpolation(currentTime, fc);
																CDSetKey(doc,op,docTime,zdscID,value,interp);
																
																if(interp == CD_SPLINE_INTERPOLATION)
																	SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
															}
														}
														else CDSetKey(doc,op,docTime,zdscID,value);
													}
												}
												else
												{
													KFCurve *fc = lProperty.GetKFCurve(NULL, takeName);
													if(fc && fc->KeyGetCount() > 0)
													{
														if(lPropertyDataType.GetType() == eBOOL1)
														{
															Real value = (Real)fc->Evaluate(currentTime);
															if(importCurves)
															{
																if(HasKeyAtTime(currentTime,fc))
																{
																	LONG interp = GetKeyInterpolation(currentTime, fc);
																	CDSetKey(doc,op,docTime,dscID,value,interp);
																	
																	if(interp == CD_SPLINE_INTERPOLATION)
																		SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
																}
															}
															else CDSetKey(doc,op,docTime,dscID,value);
														}
														else if(lPropertyDataType.GetType() == eINTEGER1)
														{
															Real value = (Real)fc->Evaluate(currentTime);
															if(importCurves)
															{
																if(HasKeyAtTime(currentTime,fc))
																{
																	LONG interp = GetKeyInterpolation(currentTime, fc);
																	CDSetKey(doc,op,docTime,dscID,value,interp);
																	
																	if(interp == CD_SPLINE_INTERPOLATION)
																		SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
																}
															}
															else CDSetKey(doc,op,docTime,dscID,value);
														}
														else if(lPropertyDataType.GetType() == eDOUBLE1 || lPropertyDataType.GetType() == eFLOAT1)
														{
															Real value = (Real)fc->Evaluate(currentTime);
															if(importCurves)
															{
																if(HasKeyAtTime(currentTime,fc))
																{
																	LONG interp = GetKeyInterpolation(currentTime, fc);
																	CDSetKey(doc,op,docTime,dscID,value,interp);
																	
																	if(interp == CD_SPLINE_INTERPOLATION)
																		SetKeyTangents(doc,op,docTime,dscID,currentTime,fc,false);
																}
															}
															else CDSetKey(doc,op,docTime,dscID,value);
														}
													}
												}
											}
										}
									}
									lProperty = kNode->GetNextProperty(lProperty);
								}
								opDD->BrowseFree(usrHnd);
							}
						}
					}
				}
				
				if(type == NODE_TYPE_MESH || type == NODE_TYPE_SPLINE)
				{
					KFbxGeometry *pGoemetry = (KFbxGeometry*)kNode->GetNodeAttribute();
					if(pGoemetry)
					{
						if(importPLA && HasVertexCacheDeformer(pGoemetry))
						{
							// vertex cache animation
							int vCnt = pGoemetry->GetControlPointsCount();
							KFbxVector4* lVertexArray = new KFbxVector4[vCnt];
							memcpy(lVertexArray, pGoemetry->GetControlPoints(), vCnt * sizeof(KFbxVector4));
							
							if(ReadVertexCacheData(pGoemetry, currentTime, lVertexArray))
							{
								AutoAlloc<AliasTrans> trans;
								BaseObject *opClone = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,trans);
								if(opClone)
								{
									Vector *padr = GetPointArray(op);
									if(padr)
									{
										Bool autoKeyOn = CDIsCommandChecked(IDM_AUTOKEYS);
										if(!autoKeyOn) CallCommand(IDM_AUTOKEYS);
										
										Bool plaKeyOn = CDIsCommandChecked(IDM_A_PLA);
										if(!plaKeyOn) CallCommand(IDM_A_PLA);
										
										int c;
										for(c=0; c<vCnt; c++)
										{
											KFbxVector4 cchPt = lVertexArray[c];
											Vector pt = KFbxVector4ToVector(cchPt);
											if(handed == 1) pt.z *= -1;
											if(freezeScale) padr[c] = scaM * pt;
											else padr[c] = pt;
										}
										op->Message(MSG_UPDATE);
										
										doc->SetTime(docTime);
										doc->AutoKey(opClone,op,false,false,false,false,false,true);
										
										if(!autoKeyOn) CallCommand(IDM_AUTOKEYS);
										if(!plaKeyOn) CallCommand(IDM_A_PLA);
										
										BaseObject::Free(opClone);
									}
								}
							}
							delete[] lVertexArray;
						}
						else
						{
							// shape animation
							int s, sn, shpCnt = pGoemetry->GetShapeCount();
							if(importMorph && shpCnt > 0)
							{
								for(s=0; s<shpCnt; s++)
								{
									KFbxShape* pShape = pGoemetry->GetShape(s);
									if(pShape)
									{
										KFCurve* pFCurve = pGoemetry->GetShapeChannel(s);
										if(!pFCurve) continue;
										
										Real sWt = Real(pFCurve->Evaluate(currentTime) / 100.0);
										
										ShapeNode *slist = shapeList.Array();
										for(sn=0; sn<shapeList.Size(); sn++)
										{
											KFbxShape *shp = slist[sn].shape;
											
											if(pShape == shp)
											{
												BaseTag *tag = slist[sn].mtag;
												if(tag)
												{
													DescID dscID = DescID(M_MIX_SLIDER);
													if(importCurves)
													{
														if(HasKeyAtTime(currentTime,pFCurve))
														{
															LONG interp = GetKeyInterpolation(currentTime, pFCurve);
															CDSetKey(doc,tag,docTime,dscID,sWt,interp);
															
															if(interp == CD_SPLINE_INTERPOLATION)
																SetKeyTangents(doc,op,docTime,dscID,currentTime,pFCurve,false);
														}
													}
													else CDSetKey(doc,tag,docTime,dscID,sWt);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			frm++;
		}
		currentTime += gPeriod;
	}
	
	CDAnimateObject(doc,root,doc->GetMinTime(),CD_ANIMATEFLAGS_QUICK|CD_ANIMATEFLAGS_NO_CHILDREN);
	
	CDFree(tName);
	
	StatusClear();
}

Bool CDImportFBXCommand::HasVertexCacheDeformer(KFbxGeometry *pGoemetry)
{
	return (pGoemetry->GetDeformerCount(KFbxDeformer::eVERTEX_CACHE) &&
			(static_cast<KFbxVertexCacheDeformer*>(pGoemetry->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE)))->IsActive());
}

void CDImportFBXCommand::LoadContent(BaseDocument *doc, BaseList2D *pr, KFbxNode *kNode)
{	
    if(!kNode) return;
	
	KFbxNodeAttribute::EAttributeType lAttributeType;
	
    BaseObject *op = NULL;
	if(kNode->GetNodeAttribute() != NULL)
    {
        lAttributeType = (kNode->GetNodeAttribute()->GetAttributeType());
		
        switch(lAttributeType)
        {
			case KFbxNodeAttribute::eUNIDENTIFIED:
			{
				if(importNull) op = KFbxNodeToNull(doc,pr,kNode);
				break;
			}
			case KFbxNodeAttribute::eNULL:
			{
				if(importNull) op = KFbxNodeToNull(doc,pr,kNode);
				break;
			}
			case KFbxNodeAttribute::eMARKER:
			{
				if(importMarker) op = KFbxNodeToNull(doc,pr,kNode);
				break;
			}
			case KFbxNodeAttribute::eSKELETON:
			{
				if(importJoint) op = KFbxNodeToJoint(doc,pr,kNode);
				break;
			}
			case KFbxNodeAttribute::eMESH:
			{
				if(importMesh) op = KFbxNodeToMesh(doc,pr,kNode);
				break;
			}
			case KFbxNodeAttribute::eCAMERA:
			{
				if(importCamera) op = KFbxNodeToCamera(doc,pr,kNode);
				break;
			}
			case KFbxNodeAttribute::eLIGHT:
			{
				if(importLight) op = KFbxNodeToLight(doc,pr,kNode);
				break;
			}
			case KFbxNodeAttribute::eNURBS_CURVE:
			{
				if(importSpline) op = KFbxNodeToSpline(doc,pr,kNode);
				break;
			}
			default:
			{
				if(importNull) op = KFbxNodeToNull(doc,pr,kNode);
				break;
			}
        }   
    }
	else 
	{
		if(importNull) op = KFbxNodeToNull(doc,pr,kNode);
	}
	
	if(op)
	{
		if(importUserData)
		{
			if(GetUserPropertyCount(kNode) > 0)
				AddUserData(op,kNode);
		}
		pr = op;
	}
	
	int i;
	for(i=0; i<kNode->GetChildCount(); i++)
	{
		LoadContent(doc,pr,kNode->GetChild(i));
	}
}

BaseTag* CDImportFBXCommand::GetPreviousConstraintTag(BaseObject *op)
{
	BaseTag *prev = NULL;
	
	BaseTag *tag = op->GetFirstTag();
	while(tag)
	{
		switch(tag->GetType())
		{
			case ID_CDPCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDRCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDSCONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDACONSTRAINTPLUGIN:
				prev = tag;
				break;
			case ID_CDPRCONSTRAINTPLUGIN:
				prev = tag;
				break;
		}
		tag = tag->GetNext();
	}
	
	return prev;
}


void CDImportFBXCommand::ConvertConstriants(BaseDocument *doc, KFbxScene *pScene)
{
	int numConstraints = KFbxGetSrcCount<KFbxConstraint>(pScene);
	
	for(int contraintIndex = 0; contraintIndex < numConstraints; contraintIndex++)
	{
		KFbxConstraint *lConstraint = KFbxGetSrc<KFbxConstraint>(pScene, contraintIndex);
		if(!lConstraint) continue;
		
		const char *name = lConstraint->GetName();
		
		KFbxConstraint::EConstraintType constraintType = lConstraint->GetConstraintType();
		// position constraint
		if(constraintType == KFbxConstraint::ePOSITION)
		{
			KFbxConstraintPosition *lPositionConstraint = (KFbxConstraintPosition*) lConstraint;
			if(lPositionConstraint)
			{
				KFbxNode *constrainedObject = static_cast<KFbxNode*>(lPositionConstraint->GetConstrainedObject());
				if(constrainedObject)
				{
					BaseObject *cnstrOp = GetObjectFromNode(constrainedObject);
					if(cnstrOp)
					{
						BaseTag *pTag = BaseTag::Alloc(ID_CDPCONSTRAINTPLUGIN);
						if(pTag)
						{
							pTag->SetName(CharToString(name));
							cnstrOp->InsertTag(pTag,GetPreviousConstraintTag(cnstrOp));
							
							BaseContainer *tData = pTag->GetDataInstance();
							if(tData)
							{
								Vector offset = scaM * KFbxVector4ToVector(lPositionConstraint->GetOffset());
								if(handed == 1) offset.z *= -1;
								
								tData->SetBool(P_AXIS_X,lPositionConstraint->GetAffectX());
								tData->SetReal(P_OFFSET_X,offset.x);
								tData->SetBool(P_AXIS_Y,lPositionConstraint->GetAffectY());
								tData->SetReal(P_OFFSET_Y,offset.y);
								tData->SetBool(P_AXIS_Z,lPositionConstraint->GetAffectZ());
								tData->SetReal(P_OFFSET_Z,offset.z);
								
								int srcCnt = lPositionConstraint->GetConstraintSourceCount();
								tData->SetLong(P_TARGET_COUNT,srcCnt);
								for(int i=0; i<srcCnt; i++)
								{
									KFbxNode *srcObject = static_cast<KFbxNode*>(lPositionConstraint->GetConstraintSource(i));
									if(srcObject)
									{
										BaseObject *srcOp = GetObjectFromNode(srcObject);
										if(srcOp)
										{
											tData->SetLink(P_TARGET+i,srcOp);
											tData->SetReal(P_POS_MIX+i,lPositionConstraint->GetSourceWeight(srcObject) * 0.01);
										}
									}
								}
							}
							pTag->Message(MSG_MENUPREPARE);
						}
					}
				}
			}
		}
		
		// rotation constraint
		if(constraintType == KFbxConstraint::eROTATION)
		{
			KFbxConstraintRotation *lRotationConstraint = (KFbxConstraintRotation*) lConstraint;
			if(lRotationConstraint)
			{
				KFbxNode *constrainedObject = static_cast<KFbxNode*>(lRotationConstraint->GetConstrainedObject());
				if(constrainedObject)
				{
					BaseObject *cnstrOp = GetObjectFromNode(constrainedObject);
					if(cnstrOp)
					{
						BaseTag *rTag = BaseTag::Alloc(ID_CDRCONSTRAINTPLUGIN);
						if(rTag)
						{
							rTag->SetName(CharToString(name));
							cnstrOp->InsertTag(rTag,GetPreviousConstraintTag(cnstrOp));
							
							BaseContainer *tData = rTag->GetDataInstance();
							if(tData)
							{
								Vector offset = KFbxVector4ToVector(lRotationConstraint->GetOffset());
								if(handed == 1) offset.z *= -1;
								
								tData->SetBool(R_AXIS_X,lRotationConstraint->GetAffectY());
								tData->SetReal(R_OFFSET_X,Rad(offset.y));
								tData->SetBool(R_AXIS_Y,lRotationConstraint->GetAffectX());
								tData->SetReal(R_OFFSET_Y,Rad(offset.x));
								tData->SetBool(R_AXIS_Z,lRotationConstraint->GetAffectZ());
								tData->SetReal(R_OFFSET_Z,Rad(offset.z));
								
								int srcCnt = lRotationConstraint->GetConstraintSourceCount();
								tData->SetLong(R_TARGET_COUNT,srcCnt);
								for(int i=0; i<srcCnt; i++)
								{
									KFbxNode *srcObject = static_cast<KFbxNode*>(lRotationConstraint->GetConstraintSource(i));
									if(srcObject)
									{
										BaseObject *srcOp = GetObjectFromNode(srcObject);
										if(srcOp)
										{
											tData->SetLink(R_TARGET+i,srcOp);
											tData->SetReal(R_ROT_MIX+i,lRotationConstraint->GetSourceWeight(srcObject) * 0.01);
											
											if(handed == 1)
											{
												if(cnstrOp->GetType() != ID_CDJOINTOBJECT && srcOp->GetType() == ID_CDJOINTOBJECT)
												{
													Matrix oldM = cnstrOp->GetMg();
													Matrix newM = oldM * jRotM;
													
													if(IsValidPointObject(cnstrOp)) RecalculatePoints(cnstrOp,newM,oldM);
													
													BaseObject *ch = cnstrOp->GetDown();
													while(ch)
													{
														Matrix transM = oldM * MInv(newM) * ch->GetMg();
														ch->SetMg(transM);
														ch = ch->GetNext();
													}
													
													cnstrOp->SetMg(newM);
													cnstrOp->Message(MSG_UPDATE);
												}
												else if(cnstrOp->GetType() == ID_CDJOINTOBJECT && srcOp->GetType() != ID_CDJOINTOBJECT)
												{
													Matrix oldM = srcOp->GetMg();
													Matrix newM = oldM * jRotM;
													
													if(IsValidPointObject(srcOp)) RecalculatePoints(srcOp,newM,oldM);
													
													BaseObject *ch = srcOp->GetDown();
													while(ch)
													{
														Matrix transM = oldM * MInv(newM) * ch->GetMg();
														ch->SetMg(transM);
														ch = ch->GetNext();
													}
													
													srcOp->SetMg(newM);
													srcOp->Message(MSG_UPDATE);
												}
											}
										}
									}
								}
							}
							rTag->Message(MSG_MENUPREPARE);
						}
					}
				}
			}
		}
		
		// scale constraint
		if(constraintType == KFbxConstraint::eSCALE)
		{
			KFbxConstraintScale *lScaleConstraint = (KFbxConstraintScale*) lConstraint;
			if(lScaleConstraint)
			{
				KFbxNode *constrainedObject = static_cast<KFbxNode*>(lScaleConstraint->GetConstrainedObject());
				if(constrainedObject)
				{
					BaseObject *cnstrOp = GetObjectFromNode(constrainedObject);
					if(cnstrOp)
					{
						BaseTag *sTag = BaseTag::Alloc(ID_CDSCONSTRAINTPLUGIN);
						if(sTag)
						{
							sTag->SetName(CharToString(name));
							cnstrOp->InsertTag(sTag,GetPreviousConstraintTag(cnstrOp));
							
							BaseContainer *tData = sTag->GetDataInstance();
							if(tData)
							{
								Vector offset = KFbxVector4ToVector(lScaleConstraint->GetOffset());
								tData->SetBool(S_AXIS_X,lScaleConstraint->GetAffectX());
								tData->SetReal(S_OFFSET_X,offset.x-1);
								tData->SetBool(S_AXIS_Y,lScaleConstraint->GetAffectY());
								tData->SetReal(S_OFFSET_Y,offset.y-1);
								tData->SetBool(S_AXIS_Z,lScaleConstraint->GetAffectZ());
								tData->SetReal(S_OFFSET_Z,offset.z-1);
								
								int srcCnt = lScaleConstraint->GetConstraintSourceCount();
								tData->SetLong(S_TARGET_COUNT,srcCnt);
								for(int i=0; i<srcCnt; i++)
								{
									KFbxNode *srcObject = static_cast<KFbxNode*>(lScaleConstraint->GetConstraintSource(i));
									if(srcObject)
									{
										BaseObject *srcOp = GetObjectFromNode(srcObject);
										if(srcOp)
										{
											tData->SetLink(S_TARGET+i,srcOp);
											tData->SetReal(S_SCA_MIX+i,lScaleConstraint->GetSourceWeight(srcObject) * 0.01);
										}
									}
								}
							}
							sTag->Message(MSG_MENUPREPARE);
						}
					}
				}
			}
		}
		
		// aim constraint
		if(constraintType == KFbxConstraint::eAIM)
		{
			KFbxConstraintAim *lAimConstraint = (KFbxConstraintAim*) lConstraint;
			if(lAimConstraint)
			{
				KFbxNode *constrainedObject = static_cast<KFbxNode*>(lAimConstraint->GetConstrainedObject());
				if(constrainedObject)
				{
					BaseObject *cnstrOp = GetObjectFromNode(constrainedObject);
					if(cnstrOp)
					{
						Matrix opM = cnstrOp->GetMg();
						
						BaseTag *aTag = BaseTag::Alloc(ID_CDACONSTRAINTPLUGIN);
						if(aTag)
						{
							aTag->SetName(CharToString(name));
							cnstrOp->InsertTag(aTag,GetPreviousConstraintTag(cnstrOp));
							
							BaseContainer *tData = aTag->GetDataInstance();
							if(tData)
							{
								KFbxConstraintAim::EAimConstraintWoldUpType upType = lAimConstraint->GetWorldUpType();
								
								switch(upType)
								{
									case KFbxConstraintAim::eAimAtSceneUp:
									{
										Vector wupV = KFbxVector4ToVector(lAimConstraint->GetUpVector());
										if(wupV == Vector(1,0,0)) tData->SetLong(A_POLE_AXIS,A_POLE_X);
										else if(wupV == Vector(0,1,0)) tData->SetLong(A_POLE_AXIS,A_POLE_Y);
										else if(wupV == Vector(-1,0,0)) tData->SetLong(A_POLE_AXIS,A_POLE_NX);
										else if(wupV == Vector(0,-1,0)) tData->SetLong(A_POLE_AXIS,A_POLE_NY);
										break;
									}
									case KFbxConstraintAim::eAimAtObjectUp:
									{
										KFbxNode *upObject = static_cast<KFbxNode*>(lAimConstraint->GetWorldUpObject());
										if(upObject)
										{
											BaseObject *upOp = GetObjectFromNode(upObject);
											if(upOp)
											{
												tData->SetLink(A_UP_VECTOR,upOp);
												tData->SetReal(A_UP_MIX,1.0);
												Vector wupV = KFbxVector4ToVector(lAimConstraint->GetUpVector());
												if(wupV == Vector(1,0,0)) tData->SetLong(A_POLE_AXIS,A_POLE_X);
												else if(wupV == Vector(0,1,0)) tData->SetLong(A_POLE_AXIS,A_POLE_Y);
												else if(wupV == Vector(-1,0,0)) tData->SetLong(A_POLE_AXIS,A_POLE_NX);
												else if(wupV == Vector(0,-1,0)) tData->SetLong(A_POLE_AXIS,A_POLE_NY);
											}
										}
										break;
									}
									case KFbxConstraintAim::eAimAtObjectRotationUp:
									{
										KFbxNode *upObject = static_cast<KFbxNode*>(lAimConstraint->GetWorldUpObject());
										if(upObject)
										{
											BaseObject *upOp = GetObjectFromNode(upObject);
											if(upOp)
											{
												tData->SetLink(A_UP_VECTOR,upOp);
												tData->SetReal(A_AIM_MIX,1.0);
												Vector wupV = KFbxVector4ToVector(lAimConstraint->GetUpVector());
												if(wupV == Vector(1,0,0)) tData->SetLong(A_POLE_AXIS,A_POLE_X);
												else if(wupV == Vector(0,1,0)) tData->SetLong(A_POLE_AXIS,A_POLE_Y);
												else if(wupV == Vector(-1,0,0)) tData->SetLong(A_POLE_AXIS,A_POLE_NX);
												else if(wupV == Vector(0,-1,0)) tData->SetLong(A_POLE_AXIS,A_POLE_NY);
												
												Vector upV = KFbxVector4ToVector(lAimConstraint->GetWorldUpVector());
												if(upV == Vector(1,0,0)) tData->SetLong(A_ALIGN_AXIS,A_ALIGN_X);
												else if(upV == Vector(0,1,0)) tData->SetLong(A_ALIGN_AXIS,A_ALIGN_Y);
												else if(upV == Vector(-1,0,0)) tData->SetLong(A_ALIGN_AXIS,A_ALIGN_NX);
												else if(upV == Vector(0,-1,0)) tData->SetLong(A_ALIGN_AXIS,A_ALIGN_NY);
											}
										}
										break;
									}
									case KFbxConstraintAim::eAimAtVector:
									{
										break;
									}
									case KFbxConstraintAim::eAimAtNone:
									{
										tData->SetLong(A_POLE_AXIS,A_POLE_OFF);
										tData->SetLong(A_ALIGN_AXIS,A_ALIGN_OFF);
										break;
									}
								}
								
								int srcCnt = lAimConstraint->GetConstraintSourceCount();
								tData->SetLong(A_TARGET_COUNT,srcCnt);
								for(int i=0; i<srcCnt; i++)
								{
									KFbxNode *srcObject = static_cast<KFbxNode*>(lAimConstraint->GetConstraintSource(i));
									if(srcObject)
									{
										BaseObject *srcOp = GetObjectFromNode(srcObject);
										if(srcOp)
										{
											tData->SetLink(A_TARGET+i,srcOp);
											tData->SetReal(A_AIM_MIX+i,lAimConstraint->GetSourceWeight(srcObject) * 0.01);
											
											Matrix opM = cnstrOp->GetMg();
											Vector aimVector = srcOp->GetMg().off - opM.off;
											
											if(handed == 1 && cnstrOp->GetType() != ID_CDJOINTOBJECT)
											{
												Matrix oldM, newM, rotM = Matrix();
												
												if(VEqual(aimVector,Vector(1.0,0.0,0.0),0.001))
													rotM = CDHPBToMatrix(Vector(-pi05,0,0));
												else if(VEqual(aimVector,Vector(-1.0,0.0,0.0),0.001))
													rotM = CDHPBToMatrix(Vector(pi05,0,0));
												else if(VEqual(aimVector,Vector(0.0,1.0,0.0),0.001))
													rotM = CDHPBToMatrix(Vector(0,pi05,pi05));
												
												oldM = cnstrOp->GetMg();
												newM = oldM * rotM;
												
												if(IsValidPointObject(cnstrOp)) RecalculatePoints(cnstrOp,newM,oldM);
												
												BaseObject *ch = cnstrOp->GetDown();
												while(ch)
												{
													Matrix transM = oldM * MInv(newM) * ch->GetMg();
													ch->SetMg(transM);
													ch = ch->GetNext();
												}
												
												cnstrOp->SetMg(newM);
												cnstrOp->Message(MSG_UPDATE);
											}
											
											opM = cnstrOp->GetMg();
											
											Real zDot = VDot(VNorm(opM.v3), VNorm(aimVector));
											if(zDot < 0) tData->SetBool(A_Z_DIRECTION,true);
											else tData->SetBool(A_Z_DIRECTION,false);
										}
									}
								}
							}
							aTag->Message(MSG_MENUPREPARE);
						}
					}
				}
			}
		}
		
		// parent constraint
		if(constraintType == KFbxConstraint::ePARENT)
		{
			KFbxConstraintParent *lParentConstraint = (KFbxConstraintParent*) lConstraint;
			if(lParentConstraint)
			{
				KFbxNode *constrainedObject = static_cast<KFbxNode*>(lParentConstraint->GetConstrainedObject());
				if(constrainedObject)
				{
					BaseObject *cnstrOp = GetObjectFromNode(constrainedObject);
					if(cnstrOp)
					{
						BaseTag *prTag = BaseTag::Alloc(ID_CDPRCONSTRAINTPLUGIN);
						if(prTag)
						{
							prTag->SetName(CharToString(name));
							cnstrOp->InsertTag(prTag,GetPreviousConstraintTag(cnstrOp));
							
							BaseContainer *tData = prTag->GetDataInstance();
							if(tData)
							{
								KFbxNode *srcObject = static_cast<KFbxNode*>(lParentConstraint->GetConstraintSource(0));
								if(srcObject)
								{
									BaseObject *srcOp = GetObjectFromNode(srcObject);
									if(srcOp)
									{
										tData->SetLink(PR_TARGET,srcOp);
										tData->SetReal(PR_STRENGTH,lParentConstraint->GetSourceWeight(srcObject) * 0.01);
										tData->SetBool(PR_TARGET_SCALE,false);
									}
								}
							}
							prTag->Message(MSG_MENUPREPARE);
						}
					}
				}
			}
		}
		
		// ik constraint
		if(constraintType == KFbxConstraint::eSINGLECHAIN_IK)
		{
			KFbxConstraintSingleChainIK *lIKConstraint = (KFbxConstraintSingleChainIK*) lConstraint;
			if(lIKConstraint)
			{
				KFbxNode *firstJoint = static_cast<KFbxNode*>(lIKConstraint->GetFirstJointObject());
				if(firstJoint)
				{
					BaseObject *rootJoint = GetObjectFromNode(firstJoint);
					if(rootJoint)
					{
						Matrix jntM = rootJoint->GetMg();
						
						BaseTag *ikTag = BaseTag::Alloc(ID_CDIKHANDLEPLUGIN);
						if(ikTag)
						{
							ikTag->SetName(CharToString(name));
							rootJoint->InsertTag(ikTag);
							
							BaseContainer *tData = ikTag->GetDataInstance();
							if(tData)
							{
								KFbxNode *endJointNode = static_cast<KFbxNode*>(lIKConstraint->GetEndJointObject());
								if(endJointNode)
								{
									BaseObject *endJnt = GetObjectFromNode(endJointNode);
									if(endJnt)
									{
										LONG jCnt = 0;
										BaseObject *jnt = endJnt->GetUp();
										while(jnt)
										{
											jCnt++;
											if(jnt == rootJoint) break;
											jnt = jnt->GetUp();
										}
										tData->SetLong(HIK_BONES_IN_GROUP,jCnt);
									}
								}
								
								KFbxConstraintSingleChainIK::ESolverType pType = lIKConstraint->GetSolverType();
								switch(pType)
								{
									case KFbxConstraintSingleChainIK::eRP_SOLVER:
										tData->SetLong(HIK_SOLVER,HIKRP);
										break;
									case KFbxConstraintSingleChainIK::eSC_SOLVER:
										tData->SetLong(HIK_SOLVER,HIKSC);
										break;
								}
								
								Vector goalV;
								BaseObject *effOp = NULL;
								KFbxNode *effectorNode = static_cast<KFbxNode*>(lIKConstraint->GetEffectorObject());
								if(effectorNode)
								{
									effOp = GetObjectFromNode(effectorNode);
									if(effOp)
									{
										tData->SetLink(HIK_GOAL_LINK,effOp);
										Matrix effM = effOp->GetMg();
										goalV = effM.off - rootJoint->GetMg().off;
										if(handed == 1 && tData->GetLong(HIK_SOLVER) == HIKSC) effOp->SetMg(effM * jRotM);
										
										if(effOp->IsInstanceOf(Onull))
										{
											// Set the IK handle NULL object parameters
											BaseContainer *gData = effOp->GetDataInstance();
											if(gData)
											{
												gData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_AXIS);
												gData->SetReal(NULLOBJECT_RADIUS,20.0);
												gData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
												ObjectColorProperties gProp;
												gProp.color = Vector(0,1,1);
												gProp.usecolor = 2;
												effOp->SetColorProperties(&gProp);
											}
										}
									}
								}
								
								Vector poleV = KFbxVector4ToVector(lIKConstraint->GetPoleVector());
								if(handed == 1)
								{
									Real x = poleV.z;
									Real z = poleV.x;
									poleV.x = x;
									poleV.z = z;
								}
								tData->SetVector(HIKSC_POLE_VECTOR,poleV);
								
								int plCnt = lIKConstraint->GetConstraintPoleVectorCount();
								if(plCnt > 0)
								{
									KFbxNode *poleNode = static_cast<KFbxNode*>(lIKConstraint->GetPoleVectorObject(0));
									if(poleNode)
									{
										BaseObject *poleOp = GetObjectFromNode(poleNode);
										if(poleOp)
										{
											tData->SetLink(HIK_POLE_LINK,poleOp);
											poleV = poleOp->GetMg().off - rootJoint->GetMg().off;
											
											if(poleOp->IsInstanceOf(Onull))
											{
												// Set the pole vector NULL object parameters
												BaseContainer *pData = poleOp->GetDataInstance();
												if(pData)
												{
													pData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
													pData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
													ObjectColorProperties pProp;
													pProp.color = Vector(0,1,1);
													pProp.usecolor = 2;
													poleOp->SetColorProperties(&pProp);
												}
											}
										}
									}
								}
								
								if(tData->GetLong(HIK_SOLVER) == HIKSC)
								{
									if(poleV == Vector(1,0,0)) tData->SetLong(HIK_POLE_AXIS, HIK_POLE_X);
									else if(poleV == Vector(-1,0,0)) tData->SetLong(HIK_POLE_AXIS, HIK_POLE_NX);
									else if(poleV == Vector(0,-1,0)) tData->SetLong(HIK_POLE_AXIS, HIK_POLE_NY);
									else tData->SetLong(HIK_POLE_AXIS, HIK_POLE_Y);
								}
								else
								{
									Real xDot = VDot(VNorm(poleV), VNorm(jntM.v1));
									Real yDot = VDot(VNorm(poleV), VNorm(jntM.v2));
									if(Abs(xDot) > Abs(yDot))
									{
										if(xDot > 0) tData->SetLong(HIK_POLE_AXIS, HIK_POLE_X);
										else tData->SetLong(HIK_POLE_AXIS, HIK_POLE_NX);
									}
									else
									{
										if(yDot > 0) tData->SetLong(HIK_POLE_AXIS, HIK_POLE_Y);
										else tData->SetLong(HIK_POLE_AXIS, HIK_POLE_NY);
									}
								}
								tData->SetReal(HIK_TWIST,Rad(Real(lIKConstraint->GetTwist())));
							}
							ikTag->Message(MSG_MENUPREPARE);
						}
					}
				}
			}
		}
	}
}

void CDImportFBXCommand::MoveSkinnedMeshes(BaseDocument *doc, BaseObject *root)
{
	LONG i, cnt = skndMeshes->GetCount();
	if(cnt > 0)
	{
		BaseObject *last = root->GetDownLast();
		if(skndMeshes->Find(last) == NOTOK)
		{
			for(i=0; i<cnt; i++)
			{
				BaseObject *mesh = static_cast<BaseObject*>(skndMeshes->GetIndex(i));
				if(mesh)
				{
					mesh->Remove();
					doc->InsertObject(mesh,root,last,false);
				}
			}
		}
	}
}

Bool CDImportFBXCommand::ImportFBX(BaseDocument *doc)
{
	Vector iScale = Vector(imScale, imScale, imScale);
	scaM = MatrixScale(iScale);
	
	if(importNURBS) ConvertNurbsAndPatch(pSdkManager, pScene, nurbsLOD);
	
	// Get the scene Axis System
	handed = 0;
	KFbxAxisSystem SceneAxisSystem = pScene->GetGlobalSettings().GetAxisSystem();
	switch(SceneAxisSystem.GetCoorSystem())
	{
		case KFbxAxisSystem::LeftHanded:
		{
			handed = 0;
			break;
		}
		case KFbxAxisSystem::RightHanded:
		{
			handed = 1;
			break;
		}
	}
	
	// convert fbx scene to c4d scene
	BaseObject *root = BaseObject::Alloc(Onull);
	if(root)
	{
		KString lString = lRootNode->GetName();
		String rName = CharToString(lString);
		root->SetName(rName);
		doc->InsertObject(root,NULL,NULL,false);
		
		int kSign;
		Vector rootRotation = Vector(0,0,0);
		switch(SceneAxisSystem.GetUpVector(kSign))
		{
			case KFbxAxisSystem::XAxis:
			{
				break;
			}
			case KFbxAxisSystem::YAxis:
			{
				if(kSign > 0) rootRotation = Vector(0,0,0);
				else rootRotation = Vector(0,pi,0);
				break;
			}
			case KFbxAxisSystem::ZAxis:
			{
				if(kSign > 0) rootRotation = Vector(0,-pi05,0);
				else rootRotation = Vector(0,pi05,0);
				break;
			}
		}
		
		StatusSetSpin();
		
		objectList.Init();
		for(int i=0; i<lRootNode->GetChildCount(); i++)
		{
			LoadContent(doc,root,lRootNode->GetChild(i));
		}
		
		StatusSetText("Transfering data");
		if(importMesh) BuildSkinning(doc);
		
		if(importCamera) SetCameraTargets();
		if(importLight) SetLightTargets();
		
		if(importConstraint) ConvertConstriants(doc,pScene);
		
		if(importAnimation && takeList.Size() > 0) LoadAnimation(doc,root);
		
		if(!freezeScale) CDSetScale(root,iScale);
		
		CallCommand(12211); // command to remove duplicate materials
		
		CDSetRot(root,rootRotation);
		MoveSkinnedMeshes(doc, root);
		
		doc->SetActiveObject(root);
		CallCommand(13038); // command to frame the view to the imported object
	}
	
	StatusClear();
	
	doc->SetTime(doc->GetMinTime());
	CDAnimateDocument(doc);
	
	return true;
}

Bool CDImportFBXCommand::LoadScene(KFbxSdkManager *pSdkManager, KFbxScene *pScene, const char *pFilename)
{
    int lFileMajor, lFileMinor, lFileRevision;
    KString lCurrentTakeName;
    bool status;
	
    // Create an importer.
    KFbxImporter* importer = KFbxImporter::Create(pSdkManager,"");
	if(!importer) return false;
	
    // Initialize the importer by providing a filename.
    const bool importStatus = importer->Initialize(pFilename);
    if(!importStatus)
	{
		importError = CharToString(importer->GetLastErrorString());
		importer->Destroy();
		return false;
	}
	
	// check the file version
    importer->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);
	LONG fileVersion = lFileMajor + lFileMinor + lFileRevision;
	if(fileVersion > 7)
	{
		String ver = CDLongToString(lFileMajor)+"."+CDLongToString(lFileMinor)+"."+CDLongToString(lFileRevision);
		importError = GeLoadString(MD_VERSION_NO_IMPORT)+ver+GeLoadString(MD_RETURN)+GeLoadString(MD_EARLIER_VERSION);
		importer->Destroy();
		return false;
	}
	
	IOSREF.SetBoolProp(IMP_FBX_CONSTRAINT,true);
	IOSREF.SetBoolProp(IMP_FBX_CHARACTER,true);
	IOSREF.SetBoolProp(IMP_FBX_CONSTRAINT_COUNT,true);
	IOSREF.SetBoolProp(IMP_FBX_CHARACTER_COUNT,true);

    // Import the scene.
    status = importer->Import(pScene);
	
    // Destroy the importer.
    importer->Destroy();

    return status;
}

void CDImportFBXCommand::GetVertexCacheRange(KFbxScene *pScene, KFbxNode *kNode, int &start, int &end)
{
    if(!kNode) return;
	
	KFbxNodeAttribute::EAttributeType lAttributeType;
	
	if(kNode->GetNodeAttribute() != NULL)
    {
        lAttributeType = kNode->GetNodeAttribute()->GetAttributeType();
		
        switch(lAttributeType)
        {
			case KFbxNodeAttribute::eNURBS_CURVE:
			case KFbxNodeAttribute::eMESH:
			{
				KFbxGeometry *pGoemetry = (KFbxGeometry*)kNode->GetNodeAttribute();
				if(pGoemetry)
				{
					if(HasVertexCacheDeformer(pGoemetry))
					{
						KFbxVertexCacheDeformer *lDeformer = static_cast<KFbxVertexCacheDeformer*>(pGoemetry->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE));
						if(lDeformer)
						{
							KFbxCache *lCache = lDeformer->GetCache();
							if(lCache && lCache->OpenFileForRead())
							{
								int lChannelIndex = lChannelIndex = lCache->GetChannelIndex(lDeformer->GetCacheChannel());
								
								KTime tStart,tStop;
								if(!lCache->GetAnimationRange(lChannelIndex,tStart,tStop))
									GePrint("Failed to get cache range");
								
								KTime::ETimeMode tMode = pScene->GetGlobalTimeSettings().GetTimeMode();
								Real frameRate = KTime::GetFrameRate(tMode);
								
								int firstFrm = (int)(tStart.GetSecondDouble() * frameRate);
								int lastFrm = (int)(tStop.GetSecondDouble() * frameRate);
								
								if(firstFrm < start) start = firstFrm;
								if(lastFrm > end) end = lastFrm;
							}
						}
					}
				}
				break;
			}
        }   
    }
	
	for(int i=0; i<kNode->GetChildCount(); i++)
	{
		GetVertexCacheRange(pScene,kNode->GetChild(i),start,end);
	}
	
}

void CDImportFBXCommand::BuildTakeList(KFbxScene *pScene)
{
	if(!pScene) return;
	
	int vcStart=0, vcEnd=0;
	takeList.Init();
	
	for(int i=0; i<lRootNode->GetChildCount(); i++)
	{
		GetVertexCacheRange(pScene,lRootNode->GetChild(i),vcStart,vcEnd);
	}
	LONG cacheRange = vcEnd - vcStart;
	
	pScene->FillTakeNameArray(gTakeNameArray);
	
	KTime::ETimeMode tMode = pScene->GetGlobalTimeSettings().GetTimeMode();
	Real frameRate = KTime::GetFrameRate(tMode);
	
	for(int i=0; i<gTakeNameArray.GetCount(); i++)
	{
		LONG numFrames = 0;
		KTime gStart, gStop, timeDiff;
		String takeName = CharToString(gTakeNameArray[i]->Buffer());
		
		KFbxTakeInfo *takeInfo = pScene->GetTakeInfo(*gTakeNameArray[i]);
		if(takeInfo)
		{
			gStart = takeInfo->mLocalTimeSpan.GetStart();
			gStop = takeInfo->mLocalTimeSpan.GetStop();
			
			timeDiff = gStop - gStart;
			double diff = timeDiff.GetSecondDouble();
			
			numFrames = (LONG)(diff * frameRate);
		}
		if(numFrames < cacheRange) numFrames = cacheRange;
		
		LONG takeStart = gStart.GetFrame(true,tMode,frameRate);
		LONG takeEnd = gStart.GetFrame(true,tMode,frameRate)+numFrames;
		
		TakeNode tnode = TakeNode(takeName,takeStart,takeEnd);
		takeList.Append(tnode);
	}
	
	DeleteAndClear(gTakeNameArray);
}

void CDImportFBXCommand::DestroySdkObjects(void)
{
	if(pScene) pScene->Destroy();
	pScene = NULL;
	
	if(pSdkManager) pSdkManager->Destroy();
	pSdkManager = NULL;
}

void CDImportFBXCommand::FreeListMemory(void)
{
	objectList.Free();
	shapeList.Free();
	takeList.Free();
	
	AtomArray::Free(skndMeshes);
}

Bool CDImportFBXCommand::SetImportOptions(void)
{
	BaseContainer *bc = GetWorldPluginData(ID_CDFBXIMPORTER);
	if(!bc) return false;
	
	skndMeshes = AtomArray::Alloc();
	
	importJoint = bc->GetBool(IDC_IMPORT_JOINT);
	importMesh = bc->GetBool(IDC_IMPORT_MESH);
	importMorph = bc->GetBool(IDC_IMPORT_MORPH);
	importNURBS = bc->GetBool(IDC_IMPORT_NURBS);
	
	
	LONG lod = bc->GetLong(IDC_NURBS_SURFACE_LOD);
	switch(lod)
	{
		case 1:
			nurbsLOD = 2;
			break;
		case 2:
			nurbsLOD = 4;
			break;
		default:
			nurbsLOD = 1;
			break;
	}
	
	importCamera = bc->GetBool(IDC_IMPORT_CAMERA);
	importLight = bc->GetBool(IDC_IMPORT_LIGHT);
	importMarker = bc->GetBool(IDC_IMPORT_MARKER);
	importNull = bc->GetBool(IDC_IMPORT_NULL);
	importSpline = bc->GetBool(IDS_IMPORT_SPLINE);
	
	importSmoothing = bc->GetBool(IDS_IMPORT_SMOOTHING);
	importMaterial = bc->GetBool(IDS_IMPORT_MATERIALS);
	importTexture = bc->GetBool(IDS_IMPORT_TEXTURES);
	importUserData = bc->GetBool(IDS_IMPORT_USER_DATA);
	importConstraint = bc->GetBool(IDS_IMPORT_CONSTRAINTS);
	
	importAnimation = bc->GetBool(IDC_IMPORT_ANIMATION);
	importCurves = bc->GetBool(IDS_IMPORT_CURVES);
	importPLA = bc->GetBool(IDC_IMPORT_PLA);
	
	imScale = bc->GetReal(IDC_IMPORT_SCALE);
	freezeScale = bc->GetBool(IDS_FREEZE_SCALE);
	
	return true;
}

Bool CDImportFBXCommand::LoadFBX(Filename *fName)
{
	StatusSetSpin();
	
	char *pFilename = NULL;
	
	pFilename = ConvertPathname(fName);
	
	// initialize fbx sdk
    pSdkManager = KFbxSdkManager::Create(); if(!pSdkManager) return false;
    pScene = KFbxScene::Create(pSdkManager,"");
	if(!pScene)
	{
		// free memory
		CDFree(pFilename);
		DestroySdkObjects();
		return false;
	}	
	
	fPath = fName->GetString();
	
	importError = "";
	
	if(!LoadScene(pSdkManager,pScene,pFilename))
	{
		// free memory
		CDFree(pFilename);
		DestroySdkObjects();
		if(importError != "") MessageDialog(importError);
		return false;
	}
	CDFree(pFilename);
	
	lRootNode = pScene->GetRootNode();
	
	takeList.Init();
	BuildTakeList(pScene);
	
	StatusClear();
	
	return true;
}

Bool CDImportFBXCommand::ExecuteFileImport(String filename, LONG t)
{
	Filename inFileName;
	const String inPath = GetBatchInputPath() + filename;
	inFileName.SetString(inPath);
	if(!inFileName.CheckSuffix("FBX")) return false;
	
	BaseDocument *doc = GetActiveDocument();
	if(!doc) return false;

	if(!LoadFBX(&inFileName)) return false;

	if(!SetImportOptions()) return false;

	TakeNode *tlist = takeList.Array();
	
	curTake = t;
	LONG tCnt = takeList.Size();
	if(curTake >= tCnt) curTake = tCnt-1;
	
	frameStart = tlist[curTake].start;
	frameOffset = 0;
	frameEnd = tlist[curTake].end;
	
	shapeList.Init();
	
	if(!ImportFBX(doc))
	{
		// free memory
		FreeListMemory();
		DestroySdkObjects();
		
		return false;
	}
	
	// free memory
	FreeListMemory();	
	DestroySdkObjects();
	
	StatusClear();
	
	return true;
}

Bool CDImportFBXCommand::Execute(BaseDocument *doc)
{
	// open file dialog
	Filename fName;
#if API_VERSION < 12000
	String title = GeLoadString(IDS_CDFBXIMPRT);
	if(!fName.FileSelect(FSTYPE_SCENES,0,&title)) return false;
#else
	if(!fName.FileSelect(FILESELECTTYPE_SCENES,FILESELECT_LOAD,GeLoadString(IDS_CDFBXIMPRT))) return false;
#endif
	
	if(!fName.CheckSuffix("FBX"))
	{
		MessageDialog(GeLoadString(MD_WRONG_FILE_TYPE));
		return false;
	}
	
	jRotM = CDHPBToMatrix(Vector(-pi05,0,0));
	
	if(!LoadFBX(&fName)) return false;
	
	StopAllThreads(); // so the document can be safely modified
	
	KFbxDocumentInfo *sceneInfo = pScene->GetSceneInfo();
	
	if(sceneInfo)
	{
		dlg.vendorName = CharToString(sceneInfo->LastSaved_ApplicationVendor.Get().Buffer());
		dlg.appName = CharToString(sceneInfo->LastSaved_ApplicationName.Get().Buffer());
		dlg.versionName = CharToString(sceneInfo->LastSaved_ApplicationVersion.Get().Buffer());
		
		if(dlg.appName == "Maya") exportApp = APP_MAYA;
		
		if(dlg.vendorName == "") dlg.vendorName = GeLoadString(IDS_UNKNOWN_INFO);
		if(dlg.appName == "") dlg.appName = GeLoadString(IDS_UNKNOWN_INFO);
		if(dlg.versionName == "") dlg.versionName = GeLoadString(IDS_UNKNOWN_INFO);
	}
	
	if(!dlg.Open()) return false;
	
	if(!SetImportOptions()) return false;
	
	curTake = dlg.take;
	frameStart = dlg.start;
	frameOffset = dlg.offset;
	frameEnd = dlg.end;
	
	if(!ImportFBX(doc))
	{
		// free memory
		FreeListMemory();
		DestroySdkObjects();
		
		return false;
	}
	doc->SetActiveObject(NULL);
		
	// free memory
	FreeListMemory();	
	DestroySdkObjects();
	
	StatusClear();
	
	EventAdd(EVENT_FORCEREDRAW);
	return true;
}

class CDImportFBXCommandR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		return true;
	}
};

Bool RegisterCDImportFBXCommand(void)
{
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
	String name = GeLoadString(IDS_CDFBXIMPRT); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDFBXIMPORTER,name,PLUGINFLAG_HIDE,"CDFBXImp.tif","CD Import FBX",CDDataAllocator(CDImportFBXCommandR));
	else return CDRegisterCommandPlugin(ID_CDFBXIMPORTER,name,0,"CDFBXImp.tif","CD Import FBX",CDDataAllocator(CDImportFBXCommand));
}


// pointer to CDImportFBXCommand
CDImportFBXCommand *cdfbxIm = NULL;

void CDStoreImportCommandPointer(void)
{
	BasePlugin *cmdPtr = CDFindPlugin(ID_CDFBXIMPORTER, CD_COMMAND_PLUGIN);
	if(cmdPtr)
	{
		COMMANDPLUGIN *cp = static_cast<COMMANDPLUGIN*>(cmdPtr->GetPluginStructure());
		if(cp) cdfbxIm = static_cast<CDImportFBXCommand*>(cp->adr);
	}
}

// Coffee import interface functions
void CDFbxImportFile(Coffee* Co, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	if(cdfbxIm && argc > 0)
	{
		String fname = sp[argc-1].GetString();
		LONG tnum;
		if(argc > 1) tnum = sp[argc-2].GetLong();
		else tnum = 1;
		res = cdfbxIm->ExecuteFileImport(fname,tnum);
	}
	
	GeCoffeeGeData2Value(Co, res, &sp[argc]);
	sp += argc;
}



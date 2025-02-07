//	Cactus Dan's FBX Import/Export plugin
//	Copyright 2011 by Cactus Dan Libisch
	
// c4d includes
#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"
#include "c4d_graphview.h"
#include "c4d_operatorplugin.h"
#include "c4d_operatordata.h"
#include "gvobject.h"

// fbx sdk includes
#include "CommonFbx.h"

// CD includes
#include "CDDebug.h"
#include "CDFBX.h"
#include "ObjectNode.h"
#include "TakeNode.h"
#include "ShapeNode.h"
#include "MaterialNode.h"

Real MillimeterToInch(Real mm)
{
	return mm * 0.0393700787;
}

enum // CD Morph
{
	M_OFFSET_EXISTS			= 1016,
	M_BS_POINT_COUNT			= 2000
};

static ObjectNodeArray				objectList;
static ObjectNodeArray				jointList;
static CDArray<ShapeNode>			shapeList;
static CDArray<TakeNode>			takeList;
static CDArray<MaterialNode>		materialList;

static AtomArray *jntTarget = NULL;

static void GetEdgePointIndices(Vector *padr, CPolygon *vadr, LONG ind, int &a, int &b)
{
	LONG side;
	side = ind-((ind/4)*4);
	switch (side)
	{
		case 0:
			a = (int)vadr[ind/4].a;
			b = (int)vadr[ind/4].b;
			break;
		case 1:
			a = (int)vadr[ind/4].b;
			b = (int)vadr[ind/4].c;
			break;
		case 2:
			a = (int)vadr[ind/4].c;
			b = (int)vadr[ind/4].d;
			break;
		case 3:
			a = (int)vadr[ind/4].d;
			b = (int)vadr[ind/4].a;
			break;
	}
}

static Bool HasGeometry(BaseObject *op)
{
	if(!op) return false;
	
	if(op->GetType() != Osds)
	{
		BaseObject *cacheOp = op->GetCache();
		if(cacheOp)
		{
			if(IsValidPolygonObject(cacheOp)) return true;
		}
	}
	
	return false;
}

static void ResolveMaterialNames(BaseDocument *doc)
{
	BaseMaterial *mat = NULL;
	
	materialList.Init();
	for(mat = doc->GetFirstMaterial(); mat; mat = mat->GetNext())
	{
		StatusSetSpin();
		MaterialNode mnode = MaterialNode(mat,mat->GetName());
		materialList.Append(mnode);
	}
	
	int i, j, cnt = materialList.Size();
	MaterialNode *mlist = materialList.Array();
	for(i=0; i<cnt; i++)
	{
		String matName = mlist[i].name;
		for(j=0; j<cnt; j++)
		{
			StatusSetSpin();
			String dupName = mlist[j].name;
			if(dupName == matName && mlist[i].mat != mlist[j].mat)
				mlist[i].name = matName+CDLongToString(cnt+i);
		}
	}
}

static String GetMaterialName(BaseMaterial *mat)
{
	String mName = mat->GetName();
	
	int i;
	MaterialNode *mlist = materialList.Array();
	for(i=0; i<materialList.Size(); i++)
	{
		if(mlist[i].mat == mat) mName = mlist[i].name;
	}
	
	return mName;
}

static String ResolveNodeNames(BaseObject *op)
{
	String opName = op->GetName();
	
	int i, cnt = objectList.Size();
	ObjectNode *olist = objectList.Array();
	for(i=0; i<cnt; i++)
	{
		BaseObject *obj = olist[i].object;
		String nm = obj->GetName();
		if(nm == opName && obj != op)
		{
			opName = opName+"_"+CDLongToString(cnt+i);
		}
	}
	
	return opName;
}

static String ResolveShapeNames(BaseTag *tag)
{
	String tName = tag->GetName();
	
	int i, cnt = shapeList.Size();
	ShapeNode *slist = shapeList.Array();
	for(i=0; i<cnt; i++)
	{
		BaseTag *mTag = slist[i].mtag;
		String nm = slist[i].name;
		if(nm == tName && mTag != tag)
		{
			tName = tName+"_"+CDLongToString(cnt+i);
		}
	}
	
	return tName;
}



class CDFbxExportTakeManager : public GeModalDialog
{
private:
	CDFbxOptionsUA 			ua;
	
	void CreateDynamicGroup(void);
	void ReLayout(void);
	
public:
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
	
};

Bool CDFbxExportTakeManager::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_EXPORT_TAKE_MANAGER));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDFBX_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDFBX_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupBorderSpace(4,4,4,4);
			AddButton(IDC_ADD_EXPORT_TAKE,BFH_FIT,0,0,GeLoadString(IDC_ADD_EXPORT_TAKE));
			AddButton(IDC_DELETE_EXPORT_TAKE,BFH_FIT,0,0,GeLoadString(IDC_DELETE_EXPORT_TAKE));
		}
		GroupEnd();
		
		GroupBegin(0,BFV_SCALEFIT|BFH_SCALEFIT,0,1,GeLoadString(IDS_EXPORT_TAKES),0);
		{
			GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
			GroupBorderSpace(4,4,4,4);
			
			ScrollGroupBegin(GROUP_SCROLL,BFH_SCALEFIT|BFV_SCALEFIT,SCROLLGROUP_VERT,0,100);
			{
				GroupBegin(GROUP_DYNAMIC,BFV_TOP|BFH_SCALEFIT,6,0,"",0);
				{
					CreateDynamicGroup();
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

void CDFbxExportTakeManager::CreateDynamicGroup(void)
{
	int i, tCnt = takeList.Size();
	for(i=0; i<tCnt; i++)
	{
		AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_TAKE_NAME),0);
		AddEditText(IDC_EXPORT_TAKE_NAME+i,BFH_LEFT,150);
		
		AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_TAKE_START),0);
		AddEditNumberArrows(IDC_EXPORT_TAKE_START+i,BFH_LEFT);
		
		AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_TAKE_END),0);
		AddEditNumberArrows(IDC_EXPORT_TAKE_END+i,BFH_LEFT);
	}
}

void CDFbxExportTakeManager::ReLayout(void)
{
	LayoutFlushGroup(GROUP_DYNAMIC);
	CreateDynamicGroup();
	LayoutChanged(GROUP_DYNAMIC);
	
	int i;
	TakeNode *tlist = takeList.Array();
	for(i=0; i<takeList.Size(); i++)
	{
		SetString(IDC_EXPORT_TAKE_NAME+i,tlist[i].name);
		SetLong(IDC_EXPORT_TAKE_START+i,tlist[i].start);
		SetLong(IDC_EXPORT_TAKE_END+i,tlist[i].end);
	}
}

Bool CDFbxExportTakeManager::InitValues(void)
{
	int i;
	TakeNode *tlist = takeList.Array();
	for(i=0; i<takeList.Size(); i++)
	{
		SetString(IDC_EXPORT_TAKE_NAME+i,tlist[i].name);
		SetLong(IDC_EXPORT_TAKE_START+i,tlist[i].start);
		SetLong(IDC_EXPORT_TAKE_END+i,tlist[i].end);
	}
	
	return true;
}

Bool CDFbxExportTakeManager::Command(LONG id,const BaseContainer &msg)
{
	String		tName;
	LONG		tStart, tEnd;
	
	int i;
	TakeNode *tlist = takeList.Array();
	for(i=0; i<takeList.Size(); i++)
	{
		GetString(IDC_EXPORT_TAKE_NAME+i,tName);
		GetLong(IDC_EXPORT_TAKE_START+i,tStart);
		GetLong(IDC_EXPORT_TAKE_END+i,tEnd);
		
		if(id == IDC_EXPORT_TAKE_NAME+i)
		{
			tlist[i].name = tName;
		}
		else if(id == IDC_EXPORT_TAKE_START+i)
		{
			tlist[i].start = tStart;
		}
		else if(id == IDC_EXPORT_TAKE_END+i)
		{
			tlist[i].end = tEnd;
		}
	}
	
	switch(id)
	{
		case IDC_ADD_EXPORT_TAKE:
		{
			LONG tCnt = LONG(takeList.Size());
			BaseDocument *doc = GetActiveDocument();
			
			LONG fps = doc->GetFps();
			LONG frmStart = (LONG)doc->GetUsedMinTime(NULL).GetFrame(Real(fps));
			LONG frmEnd = (LONG)doc->GetUsedMaxTime(NULL).GetFrame(Real(fps));
			
			TakeNode tn = TakeNode("Take."+CDLongToString(tCnt+1),frmStart,frmEnd);
			takeList.Append(tn);
			
			ReLayout();
			break;
		}
		case IDC_DELETE_EXPORT_TAKE:
		{
			if(takeList.Size() > 1)
			{
				takeList.RemoveEnd();
				ReLayout();
			}
			break;
		}
	}
	
	return true;
}


class CDFbxExportDialog : public GeModalDialog
{
private:
	CDFbxOptionsUA 			ua;
	
	void DoEnable(void);
	
public:
	LONG	handed;
	Real	scale;
	Bool	joints, meshes, morphs, cameras, lights, nulls, embed, hierarchy, selected;
	Bool	animations, pla ,bake, pos, sca, rot, bakemesh, bakecloth, bakeskin;
	Bool	smoothing, userdata, curves, splines, constraints, alphanumeric;
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
	
};

Bool CDFbxExportDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDFBXEXPRT));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDC_EXPORT_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					
					AddCheckbox(IDC_EXPORT_JOINT,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_JOINT));
					AddCheckbox(IDC_EXPORT_MESH,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_MESH));
					AddCheckbox(IDC_EXPORT_MORPHS,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_MORPHS));
					AddCheckbox(IDC_EXPORT_CAMERA,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_CAMERA));
					AddCheckbox(IDC_EXPORT_LIGHT,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_LIGHT));
					AddCheckbox(IDS_EXPORT_SPLINE,BFH_LEFT,0,0,GeLoadString(IDS_EXPORT_SPLINE));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_LEFT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(0,8,0,0);
					
					AddCheckbox(IDS_EXPORT_SMOOTHING,BFH_LEFT,0,0,GeLoadString(IDS_EXPORT_SMOOTHING));
					AddCheckbox(IDS_EXPORT_USER_DATA,BFH_LEFT,0,0,GeLoadString(IDS_EXPORT_USER_DATA));
					AddCheckbox(IDS_EXPORT_CONSTRAINTS,BFH_LEFT,0,0,GeLoadString(IDS_EXPORT_CONSTRAINTS));
				}
				GroupEnd();
				
				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(0,8,0,0);
					
					AddCheckbox(IDC_EXPORT_ANIMATION,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_ANIMATION));
					AddCheckbox(IDS_EXPORT_CURVES,BFH_LEFT,0,0,GeLoadString(IDS_EXPORT_CURVES));
					AddCheckbox(IDC_BAKE_JOINT_ANIMATION,BFH_LEFT,0,0,GeLoadString(IDC_BAKE_JOINT_ANIMATION));
					
					GroupBegin(0,BFH_LEFT,3,0,"",0);
					{
						AddCheckbox(IDC_BAKE_POSITION,BFH_LEFT,0,0,GeLoadString(IDC_BAKE_POSITION));
						AddCheckbox(IDC_BAKE_SCALE,BFH_LEFT,0,0,GeLoadString(IDC_BAKE_SCALE));
						AddCheckbox(IDC_BAKE_ROTATION,BFH_LEFT,0,0,GeLoadString(IDC_BAKE_ROTATION));
					}
					GroupEnd();
					
					GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
					{
						GroupBorderNoTitle(BORDER_NONE);
						GroupBorderSpace(0,4,0,0);
						
						AddCheckbox(IDC_EXPORT_PLA,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_PLA));
						AddCheckbox(IDC_BAKE_CDSKIN_TO_PLA,BFH_LEFT,0,0,GeLoadString(IDC_BAKE_CDSKIN_TO_PLA));
						AddCheckbox(IDC_BAKE_MESH_TO_PLA,BFH_LEFT,0,0,GeLoadString(IDC_BAKE_MESH_TO_PLA));
						AddCheckbox(IDC_BAKE_CLOTH_TO_PLA,BFH_LEFT,0,0,GeLoadString(IDC_BAKE_CLOTH_TO_PLA));
					}
					GroupEnd();
				}
				GroupEnd();
				
				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(0,8,0,0);
					
					AddCheckbox(IDC_EMBED_TEXTURES,BFH_LEFT,0,0,GeLoadString(IDC_EMBED_TEXTURES));
					AddCheckbox(IDC_PRESERVE_HIERARCHY,BFH_LEFT,0,0,GeLoadString(IDC_PRESERVE_HIERARCHY));
					AddCheckbox(IDC_ONLY_SELECTED,BFH_LEFT,0,0,GeLoadString(IDC_ONLY_SELECTED));
					AddCheckbox(IDS_ALPHA_NUMERIC_ONLY,BFH_LEFT,0,0,GeLoadString(IDS_ALPHA_NUMERIC_ONLY));

				}
				GroupEnd();
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(0,8,0,0);
					
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_HANDED),0);
					AddComboBox(IDC_HANDED,BFH_CENTER);
					AddChild(IDC_HANDED, 0, GeLoadString(IDS_LEFT_HANDED));					
					AddChild(IDC_HANDED, 1, GeLoadString(IDS_RIGHT_HANDED));					
				}
				GroupEnd();
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
			{
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_EXPORT_SCALE),0);
				AddEditNumberArrows(IDC_EXPORT_SCALE,BFH_LEFT);
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}
	
	return res;
}

Bool CDFbxExportDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeDialog::InitValues()) return false;
		
	BaseContainer *bc = GetWorldPluginData(ID_CDFBXEXPORTER);
	if(!bc)
	{
		SetBool(IDC_EXPORT_JOINT,true);
		SetBool(IDC_EXPORT_MESH,true);
		SetBool(IDC_EXPORT_MORPHS,false);
		SetBool(IDC_EXPORT_CAMERA,true);
		SetBool(IDC_EXPORT_LIGHT,true);
		SetBool(IDS_EXPORT_SPLINE,false);
		
		SetBool(IDS_EXPORT_SMOOTHING,true);
		SetBool(IDS_EXPORT_USER_DATA,false);
		SetBool(IDS_EXPORT_CONSTRAINTS,false);
		
		SetBool(IDC_EXPORT_ANIMATION,true);
		SetBool(IDS_EXPORT_CURVES,false);
		SetBool(IDC_BAKE_JOINT_ANIMATION,false);
		SetBool(IDC_BAKE_POSITION,false);
		SetBool(IDC_BAKE_SCALE,false);
		SetBool(IDC_BAKE_ROTATION,false);
		
		SetBool(IDC_EXPORT_PLA,false);
		SetBool(IDC_BAKE_CDSKIN_TO_PLA,false);
		SetBool(IDC_BAKE_MESH_TO_PLA,false);
		SetBool(IDC_BAKE_CLOTH_TO_PLA,false);
		
		SetBool(IDC_EMBED_TEXTURES,true);
		SetBool(IDC_PRESERVE_HIERARCHY,false);
		SetBool(IDC_ONLY_SELECTED,false);
		SetBool(IDS_ALPHA_NUMERIC_ONLY,true);

		
		SetLong(IDC_HANDED,0);
		SetReal(IDC_EXPORT_SCALE,1.0,0.001,1000.0,1.0,FORMAT_REAL);
	}
	else
	{
		SetBool(IDC_EXPORT_JOINT,bc->GetBool(IDC_EXPORT_JOINT));
		SetBool(IDC_EXPORT_MESH,bc->GetBool(IDC_EXPORT_MESH));
		SetBool(IDC_EXPORT_MORPHS,bc->GetBool(IDC_EXPORT_MORPHS));
		SetBool(IDC_EXPORT_CAMERA,bc->GetBool(IDC_EXPORT_CAMERA));
		SetBool(IDC_EXPORT_LIGHT,bc->GetBool(IDC_EXPORT_LIGHT));
		SetBool(IDS_EXPORT_SPLINE,bc->GetBool(IDS_EXPORT_SPLINE));
		
		SetBool(IDS_EXPORT_SMOOTHING,bc->GetBool(IDS_EXPORT_SMOOTHING));
		SetBool(IDS_EXPORT_USER_DATA,bc->GetBool(IDS_EXPORT_USER_DATA));
		SetBool(IDS_EXPORT_CONSTRAINTS,bc->GetBool(IDS_EXPORT_CONSTRAINTS));
		
		SetBool(IDC_EXPORT_ANIMATION,bc->GetBool(IDC_EXPORT_ANIMATION));
		SetBool(IDS_EXPORT_CURVES,bc->GetBool(IDS_EXPORT_CURVES));
		SetBool(IDC_BAKE_JOINT_ANIMATION,bc->GetBool(IDC_BAKE_JOINT_ANIMATION));
		SetBool(IDC_BAKE_POSITION,bc->GetBool(IDC_BAKE_POSITION));
		SetBool(IDC_BAKE_SCALE,bc->GetBool(IDC_BAKE_SCALE));
		SetBool(IDC_BAKE_ROTATION,bc->GetBool(IDC_BAKE_ROTATION));
		
		SetBool(IDC_EXPORT_PLA,bc->GetBool(IDC_EXPORT_PLA));
		SetBool(IDC_BAKE_CDSKIN_TO_PLA,bc->GetBool(IDC_BAKE_CDSKIN_TO_PLA));
		SetBool(IDC_BAKE_MESH_TO_PLA,bc->GetBool(IDC_BAKE_MESH_TO_PLA));
		SetBool(IDC_BAKE_CLOTH_TO_PLA,bc->GetBool(IDC_BAKE_CLOTH_TO_PLA));
		
		SetBool(IDC_EMBED_TEXTURES,bc->GetBool(IDC_EMBED_TEXTURES));
		SetBool(IDC_PRESERVE_HIERARCHY,bc->GetBool(IDC_PRESERVE_HIERARCHY));
		SetBool(IDC_ONLY_SELECTED,bc->GetBool(IDC_ONLY_SELECTED));
		SetBool(IDS_ALPHA_NUMERIC_ONLY,bc->GetBool(IDS_ALPHA_NUMERIC_ONLY));

		
		SetLong(IDC_HANDED,bc->GetLong(IDC_HANDED));
		SetReal(IDC_EXPORT_SCALE,bc->GetReal(IDC_EXPORT_SCALE),0.001,1000.0,1.0,FORMAT_REAL);
	}
	
	DoEnable();
	
	return true;
}

Bool CDFbxExportDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(IDC_EXPORT_JOINT,joints);
	GetBool(IDC_EXPORT_MESH,meshes);
	GetBool(IDC_EXPORT_MORPHS,morphs);
	GetBool(IDC_EXPORT_CAMERA,cameras);
	GetBool(IDC_EXPORT_LIGHT,lights);
	GetBool(IDS_EXPORT_SPLINE,splines);
	
	GetBool(IDS_EXPORT_SMOOTHING,smoothing);
	GetBool(IDS_EXPORT_USER_DATA,userdata);
	GetBool(IDS_EXPORT_CONSTRAINTS,constraints);
	
	GetBool(IDC_EXPORT_ANIMATION,animations);
	GetBool(IDS_EXPORT_CURVES,curves);
	GetBool(IDC_BAKE_JOINT_ANIMATION,bake);
	GetBool(IDC_BAKE_POSITION,pos);
	GetBool(IDC_BAKE_SCALE,sca);
	GetBool(IDC_BAKE_ROTATION,rot);
	
	GetBool(IDC_EXPORT_PLA,pla);
	GetBool(IDC_BAKE_CDSKIN_TO_PLA,bakeskin);
	GetBool(IDC_BAKE_MESH_TO_PLA,bakemesh);
	GetBool(IDC_BAKE_CLOTH_TO_PLA,bakecloth);
	
	GetBool(IDC_EMBED_TEXTURES,embed);
	GetBool(IDC_PRESERVE_HIERARCHY,hierarchy);
	GetBool(IDC_ONLY_SELECTED,selected);
	GetBool(IDS_ALPHA_NUMERIC_ONLY,alphanumeric);

	
	GetLong(IDC_HANDED,handed);
	GetReal(IDC_EXPORT_SCALE,scale);
	
	BaseContainer bc;
	bc.SetBool(IDC_EXPORT_JOINT,joints);
	bc.SetBool(IDC_EXPORT_MESH,meshes);
	bc.SetBool(IDC_EXPORT_MORPHS,morphs);
	bc.SetBool(IDC_EXPORT_CAMERA,cameras);
	bc.SetBool(IDC_EXPORT_LIGHT,lights);
	bc.SetBool(IDS_EXPORT_SPLINE,splines);
	
	bc.SetBool(IDS_EXPORT_SMOOTHING,smoothing);
	bc.SetBool(IDS_EXPORT_USER_DATA,userdata);
	bc.SetBool(IDS_EXPORT_CONSTRAINTS,constraints);
	
	bc.SetBool(IDC_EXPORT_ANIMATION,animations);
	bc.SetBool(IDS_EXPORT_CURVES,curves);
	bc.SetBool(IDC_BAKE_JOINT_ANIMATION,bake);
	bc.SetBool(IDC_BAKE_POSITION,pos);
	bc.SetBool(IDC_BAKE_SCALE,sca);
	bc.SetBool(IDC_BAKE_ROTATION,rot);
	
	bc.SetBool(IDC_EXPORT_PLA,pla);
	bc.SetBool(IDC_BAKE_CDSKIN_TO_PLA,bakeskin);
	bc.SetBool(IDC_BAKE_MESH_TO_PLA,bakemesh);
	bc.SetBool(IDC_BAKE_CLOTH_TO_PLA,bakecloth);
	
	bc.SetBool(IDC_EMBED_TEXTURES,embed);
	bc.SetBool(IDC_PRESERVE_HIERARCHY,hierarchy);
	bc.SetBool(IDC_ONLY_SELECTED,selected);
	bc.SetBool(IDS_ALPHA_NUMERIC_ONLY,alphanumeric);
	
	bc.SetLong(IDC_HANDED,handed);
	bc.SetReal(IDC_EXPORT_SCALE,scale);
	SetWorldPluginData(ID_CDFBXEXPORTER,bc,false);
	
	DoEnable();
	
	return true;
}

void CDFbxExportDialog::DoEnable(void)
{
	Bool anim, msh, bkJnt, jnts;
	
	
	GetBool(IDC_EXPORT_ANIMATION,anim);
	GetBool(IDC_EXPORT_JOINT,jnts);
	if(!jnts || !anim)
	{
		Enable(IDC_BAKE_JOINT_ANIMATION,false);
		SetBool(IDC_BAKE_JOINT_ANIMATION,false);
	}
	else
	{
		Enable(IDC_BAKE_JOINT_ANIMATION,true);
		GetBool(IDC_BAKE_JOINT_ANIMATION,bkJnt);
	}
	
	GetBool(IDC_BAKE_JOINT_ANIMATION,bkJnt);
	if(!bkJnt)
	{
		Enable(IDC_BAKE_POSITION,false);
		Enable(IDC_BAKE_SCALE,false);
		Enable(IDC_BAKE_ROTATION,false);
	}
	else
	{
		Enable(IDC_BAKE_POSITION,true);
		Enable(IDC_BAKE_SCALE,true);
		Enable(IDC_BAKE_ROTATION,true);
	}
	
	if(!anim)
	{
		Enable(IDS_EXPORT_CONSTRAINTS,false);
		SetBool(IDS_EXPORT_CONSTRAINTS,false);
		Enable(IDS_EXPORT_CURVES,false);
		SetBool(IDS_EXPORT_CURVES,false);
	}
	else
	{
		Enable(IDS_EXPORT_CONSTRAINTS,true);
		Enable(IDS_EXPORT_CURVES,true);
	}
	
	GetBool(IDC_EXPORT_MESH,msh);
	if(!msh)
	{
		Enable(IDC_EXPORT_MORPHS,false);
		SetBool(IDC_EXPORT_MORPHS,false);
		
		Enable(IDS_EXPORT_SMOOTHING,false);
		SetBool(IDS_EXPORT_SMOOTHING,false);
		
		Enable(IDC_EXPORT_PLA,false);
		SetBool(IDC_EXPORT_PLA,false);
		
		Enable(IDC_BAKE_CDSKIN_TO_PLA,false);
		SetBool(IDC_BAKE_CDSKIN_TO_PLA,false);
		
		Enable(IDC_BAKE_MESH_TO_PLA,false);
		SetBool(IDC_BAKE_MESH_TO_PLA,false);
		
		Enable(IDC_BAKE_CLOTH_TO_PLA,false);
		SetBool(IDC_BAKE_CLOTH_TO_PLA,false);
	}
	else
	{
		Enable(IDC_EXPORT_MORPHS,true);
		Enable(IDS_EXPORT_SMOOTHING,true);
		if(!anim)
		{
			Enable(IDC_EXPORT_PLA,false);
			SetBool(IDC_EXPORT_PLA,false);
			
			Enable(IDC_BAKE_CDSKIN_TO_PLA,false);
			SetBool(IDC_BAKE_CDSKIN_TO_PLA,false);
			
			Enable(IDC_BAKE_MESH_TO_PLA,false);
			SetBool(IDC_BAKE_MESH_TO_PLA,false);
			
			Enable(IDC_BAKE_CLOTH_TO_PLA,false);
			SetBool(IDC_BAKE_CLOTH_TO_PLA,false);
		}
		else
		{
			Enable(IDC_EXPORT_PLA,true);
			Enable(IDC_BAKE_MESH_TO_PLA,true);
			Enable(IDC_BAKE_CLOTH_TO_PLA,true);
			
			if(!jnts) Enable(IDC_BAKE_CDSKIN_TO_PLA,true);
			else
			{
				Enable(IDC_BAKE_CDSKIN_TO_PLA,false);
				SetBool(IDC_BAKE_CDSKIN_TO_PLA,false);
			}
		}
	}
}


class CDExportFBXCommand : public CommandData
{
public:
	CDFbxExportDialog				dlg;
	CDFbxExportTakeManager			tkMngr;
	
	KFbxSdkManager					*pSdkManager;
	KFbxScene						*pScene;
	KFbxNode						*lRootNode;
	KString							lFBXAbsolutePath, lFPCAbsoluteDirectory;
	char							*pFilename;
	
	String							fPath, txPath, exportError, missingTextures;	
	Real							exScale;
	LONG							handed, frameCount;
	Matrix							jRotM;
	
	Bool	exportJoint, exportMesh, exportMorph, exportCamera, exportLight, exportNull, embedTextures, onlySelected;
	Bool	preserveHierarchy, exportAnimation, exportPLA, bakeJoints, bakePosition, bakeScale, bakeRotation, exportConstraint;
	Bool	bakeMeshToPLA, bakeClothToPLA, bakeCDSkinToPLA, exportCurves, exportUserData, exportSpline, exportSmoothing;
	Bool	txEmbedFailed, txCopyFailed, exportAlphanumeric;
	

public:
	Bool HasPositionTrack(BaseObject *op);
	Bool HasScaleTrack(BaseObject *op);
	Bool HasRotationTrack(BaseObject *op);
	Bool HasAnimationTracks(BaseObject *op);
	Bool HasUserDataTracks(BaseObject *op);
	Bool HasParameterTracks(BaseObject *op);
	Bool HasKeyAtTime(BaseDocument *doc, BaseList2D *op, const BaseTime &docTime, DescID dscID);
	Bool IsSupportedProperty(const BaseContainer *bc);
	kFCurveInterpolation GetKeyInterpolation(BaseList2D *op, DescID dscID, LONG ind);
	Bool SetKeyTangents(BaseList2D *op, DescID dscID, LONG cKeyIndex, KFCurve *fc, int pKeyIndex, Real fps, Real scale=1.0);
	
	Bool BakeMeshToPLA(BaseObject *op);
	Bool HasPortConnection(GvNode *gvN, LONG portID);
	Bool HasPSRInports(BaseDocument *doc, BaseTag *tag, BaseObject *op);
	Bool HasConstrainedAnimation(BaseDocument *doc, BaseObject *op);
	Bool BakeJointAnimation(BaseObject *op, LONG psr);
	void AnimateScene(BaseDocument *doc, KFbxSdkManager *pSdkManager, KFbxScene *pScene);

	LONG GetFirstFrame(BaseDocument *doc);
	LONG GetLastFrame(BaseDocument *doc);
	Bool DocumentHasAnimation(BaseDocument *doc);
	
	BaseObject* GetObjectFromNode(KFbxNode *kNode);
	KFbxNode* GetNodeFromObject(BaseObject *op);
	KFbxXMatrix GetLocalMatrix(BaseObject *op, KFbxXMatrix kM, BaseObject *pr=NULL);

	KFbxNode* ConvertJoint(BaseObject *jnt, KFbxSdkManager *pSdkManager, Bool root, KFbxNode *parentNode);
	KFbxNode* ConvertMesh(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, BaseObject *gen, KFbxNode *parentNode);
	KFbxNode* ConvertLight(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode);
	KFbxNode* ConvertCamera(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode);
	KFbxNode* ConvertNull(BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode);
	KFbxNode* ConvertSpline(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode);
	
	Bool CreateSkeletons(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode);
	void GetRootJoints(BaseObject *op, AtomArray *sklRoots);
	void CreateLimbNodes(KFbxNode *pSklt, BaseObject *jnt, KFbxSdkManager *pSdkManager);
	Bool IsRootJoint(BaseObject *op);
	void FindConstrainedJointTargets(BaseDocument *doc, BaseObject *op);
	
	Bool CreateMeshes(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode);
	Bool CreateClusters(BaseDocument *doc, BaseObject *op, BaseTag *skin, KFbxSdkManager *pSdkManager, KFbxNode* kNode);
	Bool GetWeight(BaseDocument *doc, BaseTag *skin, BaseContainer *tData, Real *tWeight, LONG pCnt, LONG jInd);
	
	Bool CreateShapes(BaseDocument *doc, BaseObject *op, BaseTag *mRef, KFbxSdkManager *pSdkManager, KFbxNode* kNode, LONG ptCnt);
	void BuildShapeNodeList(BaseObject *op);
	Real GetMorphTagMix(BaseTag *mTag);
	
	Bool CreateVertexCache(KFbxSdkManager* pSdkManager, KFbxNode* kNode);
	Bool DocumentHasPLA(BaseObject *op);
	
	BaseObject* GetTexturedParent(BaseObject *op);
	Bool UsesParentTexture(BaseObject *op);
	void CreateMaterials(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxMesh *pMesh);
	int GetPolygonMaterial(BaseDocument *doc, BaseObject *op, LONG pInd);
	
	Bool CameraHasAnimatedParameters(BaseObject *op);
	Bool LightHasAnimatedParameters(BaseObject *op);

	Bool CreateLights(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode);
	Bool CreateCameras(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode);
	Bool CreateSplines(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode);
	void CreateConstraints(BaseDocument *doc, KFbxSdkManager *pSdkManager, KFbxScene *pScene);
	
	void AddUserProperties(BaseObject *op, KFbxNode *kNode);

	void BuildContent(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode);
	void CreateScene(BaseDocument *doc, KFbxSdkManager *pSdkManager, KFbxScene *pScene);
	void StoreBindPose(KFbxSdkManager *pSdkManager, KFbxScene *pScene);
	
	void DestroySdkObjects(void);
	void CreateFbmDirectory(Filename fName);
	Bool SaveScene(KFbxSdkManager *pSdkManager, KFbxDocument *pScene, int pFileFormat, bool pEmbedMedia);
	void InitTakeListData(BaseDocument *doc);
	
	Bool SetExportOptions(void);
	Bool ExportFBX(BaseDocument *doc, Filename fName);
	
	virtual Bool Execute(BaseDocument *doc);
	
	// Coffee interface functions
	Bool ExecuteFileExport(String filename);
};

// CDExportFBXCommand class functions
Bool CDExportFBXCommand::HasAnimationTracks(BaseObject *op)
{
	if(!op) return false;
	
	if(HasPositionTrack(op)) return true;
	if(HasScaleTrack(op)) return true;
	if(HasRotationTrack(op)) return true;
	if(HasUserDataTracks(op)) return true;
	
	return false;
}

Bool CDExportFBXCommand::IsSupportedProperty(const BaseContainer *bc)
{
	if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_BOOL) return true;
	if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_LONG) return true;
	if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_LONGSLIDER) return true;
	if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_REAL) return true;
	if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_REALSLIDER) return true;
	if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_REALSLIDERONLY) return true;
	if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_VECTOR) return true;
	
	return false;
}

Bool CDExportFBXCommand::HasParameterTracks(BaseObject *op)
{
	if(!op) return false;
	
	Bool prmTrack = false;
	
	AutoAlloc<Description> desc;
	CDGetDescription(op, desc, CD_DESCFLAGS_DESC_0);
	void* h = desc->BrowseInit();
	const BaseContainer *bc = NULL;
	DescID id, groupid, srcID;
	
	Bool skip = true;
	while(desc->GetNext(h, &bc, id, groupid))
	{
		if(skip && id == DescID(ID_OBJECTPROPERTIES)) skip = false;
		
		if(!skip)
		{
			if(id[0].dtype == DTYPE_VECTOR)
			{
				srcID = DescID(DescLevel(id[0].id,id[0].dtype,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
				if(CDHasAnimationTrack(op,srcID)) prmTrack = true;
				
				srcID = DescID(DescLevel(id[0].id,id[0].dtype,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
				if(CDHasAnimationTrack(op,srcID)) prmTrack = true;
				
				srcID = DescID(DescLevel(id[0].id,id[0].dtype,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
				if(CDHasAnimationTrack(op,srcID)) prmTrack = true;
			}
			else if(CDHasAnimationTrack(op,id)) prmTrack = true;
		}
	}
	desc->BrowseFree(h);
	
	return prmTrack;
}

Bool CDExportFBXCommand::HasUserDataTracks(BaseObject *op)
{
	if(!op) return false;
	
	Bool udTrack = false;
	DynamicDescription *dd = op->GetDynamicDescription();
	if(dd)
	{
		DescID dscID;
		const BaseContainer *bc;
		
		void *hnd = dd->BrowseInit();
		while(dd->BrowseGetNext(hnd, &dscID, &bc))
		{
			GeData data;
			CDGetParameter(op,dscID,data);
			if(CDHasAnimationTrack(op,dscID) && IsSupportedProperty(bc)) udTrack = true;
		}
		dd->BrowseFree(hnd);
	}
	
	return udTrack;
}

Bool CDExportFBXCommand::HasPositionTrack(BaseObject *op)
{
	if(!op) return false;
	
	return CDHasVectorTrack(op,CD_ID_BASEOBJECT_POSITION);
}

Bool CDExportFBXCommand::HasScaleTrack(BaseObject *op)
{
	if(!op) return false;
	
	return CDHasVectorTrack(op,CD_ID_BASEOBJECT_SCALE);
}

Bool CDExportFBXCommand::HasRotationTrack(BaseObject *op)
{
	if(!op) return false;
	
	return CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION);
}

BaseObject* CDExportFBXCommand::GetObjectFromNode(KFbxNode *kNode)
{
	BaseObject *op = NULL;
	
	ObjectNode *olist = objectList.Array();
	LONG ind = objectList.GetIndex(kNode);
	if(ind >= 0) op = olist[ind].object;
	
	return op;
}

KFbxNode* CDExportFBXCommand::GetNodeFromObject(BaseObject *op)
{
	KFbxNode *node = NULL;
	
	ObjectNode *olist = objectList.Array();
	LONG ind = objectList.GetIndex(op);
	if(ind >= 0) node = olist[ind].node;
	
	return node;
}

KFbxXMatrix CDExportFBXCommand::GetLocalMatrix(BaseObject *op, KFbxXMatrix kM, BaseObject *pr)
{
	KFbxXMatrix localM = kM;
	
	if(pr)
	{
		Matrix prM = pr->GetMg();
		if(handed == 1 && (pr->GetType() == ID_CDJOINTOBJECT || jntTarget->Find(pr) > NOTOK)) prM = pr->GetMg() * jRotM;
		
		KFbxXMatrix prKM = MatrixToKFbxXMatrix(prM,handed,exScale);
		localM = prKM.Inverse() * kM;
	}
	
	return localM;
}

Bool CDExportFBXCommand::IsRootJoint(BaseObject *op)
{
	BaseObject *pr = op->GetUp();
	
	while(pr)
	{
		StatusSetSpin();
		if(pr->GetType() == ID_CDJOINTOBJECT) return false;
		
		pr = pr->GetUp();
	}
	
	return true;
}

void CDExportFBXCommand::GetRootJoints(BaseObject *op, AtomArray *sklRoots)
{
	while(op)
	{
		StatusSetSpin();
		Bool root = false;
		
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			if(IsRootJoint(op))
			{
				root = true;
				sklRoots->Append(op);
			}
		}
		if(!root) GetRootJoints(op->GetDown(),sklRoots);
		
		op = op->GetNext();
	}
}

LONG CDExportFBXCommand::GetFirstFrame(BaseDocument *doc)
{
	if(!doc) return 0;
	
	Real fps = doc->GetFps();
	BaseTime time = doc->GetUsedMinTime(NULL);
	return time.GetFrame(fps);
}

LONG CDExportFBXCommand::GetLastFrame(BaseDocument *doc)
{
	if(!doc) return 0;
	
	Real fps = doc->GetFps();
	BaseTime time = doc->GetUsedMaxTime(NULL);
	return time.GetFrame(fps);
}

Bool CDExportFBXCommand::DocumentHasAnimation(BaseDocument *doc)
{
	LONG firstFrame = GetFirstFrame(doc);
	LONG lastFrame = GetLastFrame(doc);
	
	LONG animationLength = lastFrame - firstFrame;
	
	if(animationLength > 0) return true;
	else return false;
}

Bool CDExportFBXCommand::BakeJointAnimation(BaseObject *op, LONG psr)
{
	if(!op) return false;
	
	if(op->GetType() != ID_CDJOINTOBJECT) return false;
	
	if(!bakeJoints) return false;
	
	switch(psr)
	{
		case 0:
			if(bakePosition) return true;
			else return false;
		case 1:
			if(bakeScale) return true;
			else return false;
		case 2:
			if(bakeRotation) return true;
			else return false;
	}
	
	return false;
}

Bool CDExportFBXCommand::BakeMeshToPLA(BaseObject *op)
{
	if(exportPLA && CDHasAnimationTrack(op,DescID(ID_PLA_TRACK))) return true;
	if(exportPLA && op->GetTag(ID_CDPTCONSTRAINTPLUGIN)) return true;
	
	if(bakeMeshToPLA && op->GetDeformCache())
	{
		return true;
	}
	if(bakeClothToPLA && op->GetTag(ID_CLOTH_TAG)) return true;
	if(bakeCDSkinToPLA && op->GetTag(ID_CDSKINPLUGIN)) return true;
	
	return false;
}

Bool CDExportFBXCommand::HasPortConnection(GvNode *gvN, LONG portID)
{
	GvPort *inPort = gvN->GetInPortFirstMainID(portID);
	if(inPort && inPort->IsIncomingConnected()) return true;
	
	return false;
}

Bool CDExportFBXCommand::HasPSRInports(BaseDocument *doc, BaseTag *tag, BaseObject *op)
{
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
					
					LONG portID;
					DescID dscID;
					
					// local matrix in port
					if(HasPortConnection(gvN,GV_OBJECT_OPERATOR_LOCAL_IN)) return true;
					
					//global matrix in port
					if(HasPortConnection(gvN,GV_OBJECT_OPERATOR_GLOBAL_IN)) return true;
					
					// global position & rotation ports
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_POSITION));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_ROTATION));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_GLOBAL_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					// local PSR ports
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;

					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_SCALE));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_ROTATION));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(CD_ID_BASEOBJECT_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
							
				#if API_VERSION >= 12000
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_POSITION));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_ROTATION));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
										
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_SCALE));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
					
					dscID = DescID(DescLevel(ID_BASEOBJECT_FROZEN_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
					portID = GvCall(gvOp,GetMainID) (gvN,GV_PORT_INPUT,dscID);
					if(HasPortConnection(gvN,portID)) return true;
				#endif
				}
			}
			gvN = gvN->GetNext();
		}
	}
	
	return false;
}

Bool CDExportFBXCommand::HasConstrainedAnimation(BaseDocument *doc, BaseObject *op)
{
	while(op)
	{
		if(op->GetTag(Ttargetexpression)) return true;
		if(op->GetTag(Taligntospline)) return true;
		if(op->GetTag(Taligntopath)) return true;
		if(op->GetTag(Tvibrate)) return true;
		if(op->GetTag(ID_MOCCA_CONSTRAINT)) return true;

		if(!exportConstraint)
		{
			if(op->GetTag(ID_CDPCONSTRAINTPLUGIN)) return true;
			if(op->GetTag(ID_CDRCONSTRAINTPLUGIN)) return true;
			if(op->GetTag(ID_CDSCONSTRAINTPLUGIN)) return true;
			if(op->GetTag(ID_CDPSRCONSTRAINTPLUGIN)) return true;
			if(op->GetTag(ID_CDACONSTRAINTPLUGIN)) return true;
			if(op->GetTag(ID_CDPRCONSTRAINTPLUGIN)) return true;
		}
		
		if(op->GetTag(ID_CDMCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDNCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDDCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDCCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDLCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDSFCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDSPRCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDTACONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDSPLCONSTRAINTPLUGIN)) return true;
		if(op->GetTag(ID_CDSWCONSTRAINTPLUGIN)) return true;
		
		BaseTag *xTag = op->GetTag(Texpresso);
		while(xTag)
		{
			if(xTag->GetType() == Texpresso)
			{
				if(HasPSRInports(doc,xTag,op)) return true;
			}
			xTag = xTag->GetNext();
		}
		
		op = op->GetUp();
	}
	
	return false;
}

Bool CDExportFBXCommand::CameraHasAnimatedParameters(BaseObject *op)
{
	if(CDHasAnimationTrack(op,DescLevel(CAMERA_FOCUS))) return true;
	if(CDHasAnimationTrack(op,DescLevel(CAMERAOBJECT_TARGETDISTANCE))) return true;
	
	return false;
}

Bool CDExportFBXCommand::LightHasAnimatedParameters(BaseObject *op)
{
	if(CDHasAnimationTrack(op,DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0)))) return true;
	if(CDHasAnimationTrack(op,DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0)))) return true;
	if(CDHasAnimationTrack(op,DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0)))) return true;
	if(CDHasAnimationTrack(op,DescLevel(LIGHT_BRIGHTNESS))) return true;
	if(CDHasAnimationTrack(op,DescLevel(LIGHT_DETAILS_INNERANGLE))) return true;
	if(CDHasAnimationTrack(op,DescLevel(LIGHT_DETAILS_OUTERANGLE))) return true;
	
	return false;
}

kFCurveInterpolation CDExportFBXCommand::GetKeyInterpolation(BaseList2D *op, DescID dscID, LONG ind)
{
	int interp = CDGetKeyInterpolation(op,dscID,ind);
	
	switch(interp)
	{
		case CD_LINEAR_INTERPOLATION:
			return KFCURVE_INTERPOLATION_LINEAR;
		case CD_STEP_INTERPOLATION:
			return KFCURVE_INTERPOLATION_CONSTANT;
	}
	
	return KFCURVE_INTERPOLATION_CUBIC;
}

Bool CDExportFBXCommand::SetKeyTangents(BaseList2D *op, DescID dscID, LONG cKeyIndex, KFCurve *fc, int pKeyIndex, Real fps, Real scale)
{
#if API_VERSION < 9800
	BaseTrack *track = op->FindTrack(dscID); if(!track) return false;
	BaseSequence *seq = NULL;
	BaseKey *key = NULL;
	for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
	{
		if(seq->GetKeyCount() > 0)
		{
			LONG kInd = 0;
			key = seq->GetFirstKey();
			if(key)
			{
				while(key && kInd <= cKeyIndex)
				{
					if(kInd == cKeyIndex) break;
					key = key->GetNext();
					if(key) kInd++;
				}
			}
		}
	}
	if(!key) return false;
	AnimValue *av = GetKeyValue(key); if(!av) return false;
#else
	CTrack *track = op->FindCTrack(dscID); if(!track) return false;
	CCurve *crv = track->GetCurve(); if(!crv) return false;
	CKey *key = crv->GetKey(cKeyIndex); if(!key) return false;
#endif	
	LONG curFrm = CDGetKeyFrame(op,dscID,cKeyIndex,fps);
	
	fc->KeySetTangeantMode(pKeyIndex,KFCURVE_TANGEANT_BREAK);
	fc->KeySetTangeantWeightMode(pKeyIndex,KFCURVE_WEIGHTED_ALL);
	
	if(cKeyIndex > 0)
	{	
		LONG prvFrm = CDGetKeyFrame(op,dscID,cKeyIndex-1,fps);
		if(prvFrm < curFrm)
		{
			// calculate right tangent
		#if API_VERSION < 9800
			BaseKey *prvKey = key->GetPred(); if(!prvKey) return false;
			AnimValue *avPrv = GetKeyValue(prvKey); if(!avPrv) return false;
			Real rValue = avPrv->value_right * scale;
			Real rSeconds = avPrv->time_right;
		#else
			CKey *prvKey = crv->GetKey(cKeyIndex-1); if(!prvKey) return false;
			Real rValue = prvKey->GetValueRight() * scale;
			Real rSeconds = prvKey->GetTimeRight().Get();
		#endif	
			if(rSeconds < CDMINREAL) rSeconds = CDMINREAL;
			
			Real rSlope = rValue/rSeconds;
			if(handed == 1)
			{
				if(dscID == CDGetPSRTrackDescriptionID(CD_ROT_B)) rSlope *= -1;
				if(dscID == CDGetPSRTrackDescriptionID(CD_POS_Z)) rSlope *= -1;
			}
			fc->KeySetRightDerivative(pKeyIndex-1,double(rSlope));
			
			Real rtWt = fps * rSeconds/(curFrm-prvFrm);
			fc->KeySetRightTangeantWeight(pKeyIndex-1,double(rtWt));
			
			// calculate left tangent
		#if API_VERSION < 9800
			Real lValue = av->value_left * scale;
			Real lSeconds = av->time_left;
		#else
			Real lValue = key->GetValueLeft() * scale;
			Real lSeconds = key->GetTimeLeft().Get();
		#endif	
			if(lSeconds < CDMINREAL) lSeconds = CDMINREAL;
			
			Real lSlope = lValue/lSeconds;
			if(handed == 1)
			{
				if(dscID == CDGetPSRTrackDescriptionID(CD_ROT_B)) lSlope *= -1;
				if(dscID == CDGetPSRTrackDescriptionID(CD_POS_Z)) lSlope *= -1;
			}
			fc->KeySetLeftDerivative(pKeyIndex,double(lSlope));
			
			Real ltWt = fps * Abs(lSeconds)/(curFrm-prvFrm);
			fc->KeySetLeftTangeantWeight(pKeyIndex,double(ltWt));
		}
	}
	
	return true;
}

Bool CDExportFBXCommand::HasKeyAtTime(BaseDocument *doc, BaseList2D *op, const BaseTime &docTime, DescID dscID)
{
	if(!op || !doc) return false;
	
#if API_VERSION < 9800
	BaseTrack *track = op->FindTrack(dscID); if(!track) return false;
	BaseSequence *seq = NULL;
	for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
	{
		if(docTime >= seq->GetT1() && docTime <= seq->GetT2()) break;
	}
	if(!seq) return false;
	BaseKey *key = seq->FindKey(docTime); if(!key) return false;
#else
	CTrack *track = op->FindCTrack(dscID); if(!track) return false;
	CCurve *crv = track->GetCurve(); if(!crv) return false;
	CKey *key = crv->FindKey(docTime); if(!key) return false;
#endif
	
	return true;
}

void CDExportFBXCommand::AnimateScene(BaseDocument *doc, KFbxSdkManager *pSdkManager, KFbxScene *pScene)
{
	if(!DocumentHasAnimation(doc)) return;
		
	KTime pTime, pStart, pStop;
    int pKeyIndex = 0;
	double kfps;
	Real fps = doc->GetFps();
	
	KTime_GetNearestCustomFramerate(double(fps), kfps);
	KTime_SetGlobalTimeMode(KTime::eCUSTOM, kfps);
	pScene->GetGlobalTimeSettings().SetTimeMode(KTime::eCUSTOM);

	int t;
	TakeNode *tlist = takeList.Array();
	for(t=0; t<takeList.Size(); t++)
	{
		StatusSetText("Building FBX Animation Take - "+tlist[t].name);
		
		char *tName = StringToChar(tlist[t].name);
		KString pTakeName = KString(tName);
		pScene->CreateTake(pTakeName.Buffer());
		
		int i;
		ObjectNode *olist = objectList.Array();
		for(i=0; i<objectList.Size(); i++)
		{
			KFbxNode *kNode = olist[i].node;
			BaseObject *op = olist[i].object;
			LONG type = olist[i].nodeType;
			
			if(type == NODE_TYPE_MESH || type == NODE_TYPE_SPLINE)
			{
				if(BakeMeshToPLA(op)) olist[i].pla = true;
			}
			
			if(type == NODE_TYPE_CAMERA)
			{
				if(CameraHasAnimatedParameters(op))
				{
					KFbxCamera *pCamera = (KFbxCamera*)kNode->GetNodeAttribute();
					if(pCamera)
					{
						pCamera->CreateTakeNode(pTakeName.Buffer());
						pCamera->SetCurrentTakeNode(pTakeName.Buffer());
						
						if(CDHasAnimationTrack(op,DescLevel(CAMERA_FOCUS)))
							pCamera->FocalLength.GetKFCurveNode(true, pTakeName.Buffer());
						
						if(CDHasAnimationTrack(op,DescLevel(CAMERAOBJECT_TARGETDISTANCE)))
						{
							pCamera->FocusDistance.ModifyFlag(FbxPropertyFlags::eANIMATABLE, true);
							pCamera->FocusDistance.GetKFCurveNode(true, pTakeName.Buffer());
						}
					}
				}
			}
			if(type == NODE_TYPE_LIGHT)
			{
				if(LightHasAnimatedParameters(op))
				{
					KFbxLight *pLight = (KFbxLight*)kNode->GetNodeAttribute();
					if(pLight)
					{
						pLight->CreateTakeNode(pTakeName.Buffer());
						pLight->SetCurrentTakeNode(pTakeName.Buffer());
						
						if(CDHasAnimationTrack(op,DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0))) || 
						   CDHasAnimationTrack(op,DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0))) || 
						   CDHasAnimationTrack(op,DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0))))
							pLight->Color.GetKFCurveNode(true, pTakeName.Buffer());
							
						if(CDHasAnimationTrack(op,DescLevel(LIGHT_BRIGHTNESS)))
							pLight->Intensity.GetKFCurveNode(true, pTakeName.Buffer());
							
						if(CDHasAnimationTrack(op,DescLevel(LIGHT_DETAILS_INNERANGLE)))
							pLight->HotSpot.GetKFCurveNode(true, pTakeName.Buffer());
							
						if(CDHasAnimationTrack(op,DescLevel(LIGHT_DETAILS_OUTERANGLE)))
							pLight->ConeAngle.GetKFCurveNode(true, pTakeName.Buffer());
					}
				}
			}
			if(HasAnimationTracks(op))
			{
				kNode->CreateTakeNode(pTakeName.Buffer());
				kNode->SetCurrentTakeNode(pTakeName.Buffer());
				
				if(HasPositionTrack(op) || BakeJointAnimation(op,0))
					kNode->LclTranslation.GetKFCurveNode(true, pTakeName.Buffer());
				
				if(HasScaleTrack(op) || BakeJointAnimation(op,1))
					kNode->LclScaling.GetKFCurveNode(true, pTakeName.Buffer());
				
				if(HasRotationTrack(op) || BakeJointAnimation(op,2))
					kNode->LclRotation.GetKFCurveNode(true, pTakeName.Buffer());
				
				if(HasUserDataTracks(op))
				{
					DynamicDescription *dd = op->GetDynamicDescription();
					if(dd)
					{
						DescID dscID;
						const BaseContainer *bc;
						
						void *hnd = dd->BrowseInit();
						KFbxProperty lProperty = kNode->GetFirstProperty();
						while(lProperty.IsValid())
						{
							if(lProperty.GetFlag(KFbxUserProperty::eUSER))
							{
								if(dd->BrowseGetNext(hnd, &dscID, &bc))
								{
									GeData data;
									CDGetParameter(op,dscID,data);
									
									if(CDHasAnimationTrack(op,dscID) && IsSupportedProperty(bc))
									{
										lProperty.ModifyFlag(FbxPropertyFlags::eANIMATABLE, true);
										KFCurveNode *fCurveNode = lProperty.GetKFCurveNode(true, pTakeName.Buffer());
										if(fCurveNode)
										{
											if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_VECTOR)
											{
												fCurveNode->FindOrCreate("X", true);
												fCurveNode->FindOrCreate("Y", true);
												fCurveNode->FindOrCreate("Z", true);
											}
										}
									}
								}
							}
							lProperty = kNode->GetNextProperty(lProperty);
						}
						dd->BrowseFree(hnd);
					}
				}
			}
			else
			{
				if(type == NODE_TYPE_JOINT && bakeJoints)
				{
					kNode->CreateTakeNode(pTakeName.Buffer());
					kNode->SetCurrentTakeNode(pTakeName.Buffer());
					
					if(HasPositionTrack(op) || BakeJointAnimation(op,0))
						kNode->LclTranslation.GetKFCurveNode(true, pTakeName.Buffer());
					
					if(HasScaleTrack(op) || BakeJointAnimation(op,1))
						kNode->LclScaling.GetKFCurveNode(true, pTakeName.Buffer());
					
					if(HasRotationTrack(op) || BakeJointAnimation(op,2))
						kNode->LclRotation.GetKFCurveNode(true, pTakeName.Buffer());
				}
				else
				{
					if(HasConstrainedAnimation(doc,op))
					{
						if(type == NODE_TYPE_MESH)
						{
							Bool setPSR = true;
							if(exportJoint && op->GetTag(ID_CDSKINPLUGIN)) setPSR = false;
							
							if(setPSR)
							{
								kNode->CreateTakeNode(pTakeName.Buffer());
								kNode->SetCurrentTakeNode(pTakeName.Buffer());
								
								kNode->LclTranslation.GetKFCurveNode(true, pTakeName.Buffer());
								kNode->LclScaling.GetKFCurveNode(true, pTakeName.Buffer());
								kNode->LclRotation.GetKFCurveNode(true, pTakeName.Buffer());
							}
						}
						else
						{
							kNode->CreateTakeNode(pTakeName.Buffer());
							kNode->SetCurrentTakeNode(pTakeName.Buffer());
							
							kNode->LclTranslation.GetKFCurveNode(true, pTakeName.Buffer());
							kNode->LclScaling.GetKFCurveNode(true, pTakeName.Buffer());
							kNode->LclRotation.GetKFCurveNode(true, pTakeName.Buffer());
						}
					}
				}
			}
		}
		
		LONG firstFrame = tlist[t].start;
		LONG lastFrame = tlist[t].end;
		LONG f, numFrames = lastFrame - firstFrame;
		
		for(f=0; f<=numFrames; f++)
		{
			LONG status = LONG(Real(f)/Real(numFrames)*100.0);
			StatusSetBar(status);
			
			LONG curFrm = firstFrame+f;
			BaseTime docTime = BaseTime(curFrm,fps);
			doc->SetTime(docTime);
			CDAnimateDocument(doc);
			
			if(numFrames == 0) continue;
			
			pTime.SetSecondDouble(double(Real(f)/fps));
			for(i=0; i<objectList.Size(); i++)
			{
				KFbxXMatrix localM;
				
				KFbxNode *kNode = olist[i].node;
				BaseObject *op = olist[i].object;
				LONG type = olist[i].nodeType;
				Bool vCache = olist[i].pla;
				
				Matrix fbxM, opM = op->GetMg();
				if(type == NODE_TYPE_CAMERA)
				{
					fbxM.off = opM.off;
					fbxM.v1 = opM.v3;
					fbxM.v2 = opM.v2;
					fbxM.v3 = -opM.v1;
				}
				else if(type == NODE_TYPE_LIGHT)
				{
					fbxM.off = opM.off;
					fbxM.v1 = -opM.v2;
					fbxM.v2 = -opM.v3;
					fbxM.v3 = opM.v1;
				}
				if(type == NODE_TYPE_JOINT || jntTarget->Find(op) > NOTOK)
				{
					if(handed == 1) fbxM = opM * jRotM;
					else fbxM = opM;
				}
				else fbxM = opM;
				
				
				KFbxNode *pParent = kNode->GetParent();
				if(pParent && pParent != lRootNode)
					localM = GetLocalMatrix(op,MatrixToKFbxXMatrix(fbxM,handed,exScale),GetObjectFromNode(pParent));
				else
					localM = MatrixToKFbxXMatrix(fbxM,handed,exScale);
				
				// psr tracks
				if(HasPositionTrack(op) || BakeJointAnimation(op,0) || HasConstrainedAnimation(doc,op))
				{
					KFbxVector4 pos = localM.GetT();
					
					KFCurve *xCurve = kNode->LclTranslation.GetKFCurve(KFCURVENODE_T_X, pTakeName.Buffer());
					if(xCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_POS_X);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								xCurve->KeyModifyBegin();
								pKeyIndex = xCurve->KeyAdd(pTime);
								xCurve->KeySetValue(pKeyIndex, pos.GetAt(0));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								xCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,xCurve,pKeyIndex,fps,exScale);
								xCurve->KeyModifyEnd();
							}
						}
						else
						{
							xCurve->KeyModifyBegin();
							pKeyIndex = xCurve->KeyAdd(pTime);
							xCurve->KeySetValue(pKeyIndex, pos.GetAt(0));
							xCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							xCurve->KeyModifyEnd();
						}
					}
					
					KFCurve *yCurve = kNode->LclTranslation.GetKFCurve(KFCURVENODE_T_Y, pTakeName.Buffer());
					if(yCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_POS_Y);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								yCurve->KeyModifyBegin();
								pKeyIndex = yCurve->KeyAdd(pTime);
								yCurve->KeySetValue(pKeyIndex, pos.GetAt(1));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								yCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,yCurve,pKeyIndex,fps,exScale);
								yCurve->KeyModifyEnd();
							}
						}
						else
						{
							yCurve->KeyModifyBegin();
							pKeyIndex = yCurve->KeyAdd(pTime);
							yCurve->KeySetValue(pKeyIndex, pos.GetAt(1));
							yCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							yCurve->KeyModifyEnd();
						}
					}
					
					KFCurve *zCurve = kNode->LclTranslation.GetKFCurve(KFCURVENODE_T_Z, pTakeName.Buffer());
					if(zCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_POS_Z);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								zCurve->KeyModifyBegin();
								pKeyIndex = zCurve->KeyAdd(pTime);
								zCurve->KeySetValue(pKeyIndex, pos.GetAt(2));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								zCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,zCurve,pKeyIndex,fps,exScale);
								zCurve->KeyModifyEnd();
							}
						}
						else
						{
							zCurve->KeyModifyBegin();
							pKeyIndex = zCurve->KeyAdd(pTime);
							zCurve->KeySetValue(pKeyIndex, pos.GetAt(2));
							zCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							zCurve->KeyModifyEnd();
						}
					}
				}
				
				if(HasScaleTrack(op) || BakeJointAnimation(op,1) || HasConstrainedAnimation(doc,op))
				{
					KFbxVector4 sca = localM.GetS();
					
					KFCurve *xCurve = kNode->LclScaling.GetKFCurve(KFCURVENODE_S_X, pTakeName.Buffer());
					if(xCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_SCA_X);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								xCurve->KeyModifyBegin();
								pKeyIndex = xCurve->KeyAdd(pTime);
								xCurve->KeySetValue(pKeyIndex, sca.GetAt(0));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								xCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,xCurve,pKeyIndex,fps);
								xCurve->KeyModifyEnd();
							}
						}
						else
						{
							xCurve->KeyModifyBegin();
							pKeyIndex = xCurve->KeyAdd(pTime);
							xCurve->KeySetValue(pKeyIndex, sca.GetAt(0));
							xCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							xCurve->KeyModifyEnd();
						}
					}
					
					KFCurve *yCurve = kNode->LclScaling.GetKFCurve(KFCURVENODE_S_Y, pTakeName.Buffer());
					if(yCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_SCA_Y);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								yCurve->KeyModifyBegin();
								pKeyIndex = yCurve->KeyAdd(pTime);
								yCurve->KeySetValue(pKeyIndex, sca.GetAt(1));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								yCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,yCurve,pKeyIndex,fps);
								yCurve->KeyModifyEnd();
							}
						}
						else
						{
							yCurve->KeyModifyBegin();
							pKeyIndex = yCurve->KeyAdd(pTime);
							yCurve->KeySetValue(pKeyIndex, sca.GetAt(1));
							yCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							yCurve->KeyModifyEnd();
						}
					}
					
					KFCurve *zCurve = kNode->LclScaling.GetKFCurve(KFCURVENODE_S_Z, pTakeName.Buffer());
					if(zCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_SCA_Z);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								zCurve->KeyModifyBegin();
								pKeyIndex = zCurve->KeyAdd(pTime);
								zCurve->KeySetValue(pKeyIndex, sca.GetAt(2));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								zCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,zCurve,pKeyIndex,fps);
								zCurve->KeyModifyEnd();
							}
						}
						else
						{
							zCurve->KeyModifyBegin();
							pKeyIndex = zCurve->KeyAdd(pTime);
							zCurve->KeySetValue(pKeyIndex, sca.GetAt(2));
							zCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							zCurve->KeyModifyEnd();
						}
					}
				}
				
				if(HasRotationTrack(op) || BakeJointAnimation(op,2) || HasConstrainedAnimation(doc,op))
				{
					KFbxVector4 rot = GetOptimalRotation(olist[i].oldRot,localM.GetR());
					
					KFCurve *xCurve = kNode->LclRotation.GetKFCurve(KFCURVENODE_R_X, pTakeName.Buffer());
					if(xCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_ROT_P);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								xCurve->KeyModifyBegin();
								pKeyIndex = xCurve->KeyAdd(pTime);
								xCurve->KeySetValue(pKeyIndex, rot.GetAt(0));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								xCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,xCurve,pKeyIndex,fps);
								xCurve->KeyModifyEnd();
							}
						}
						else
						{
							xCurve->KeyModifyBegin();
							pKeyIndex = xCurve->KeyAdd(pTime);
							xCurve->KeySetValue(pKeyIndex, rot.GetAt(0));
							xCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							xCurve->KeyModifyEnd();
						}
					}
					
					KFCurve *yCurve = kNode->LclRotation.GetKFCurve(KFCURVENODE_R_Y, pTakeName.Buffer());
					if(yCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_ROT_H);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								yCurve->KeyModifyBegin();
								pKeyIndex = yCurve->KeyAdd(pTime);
								yCurve->KeySetValue(pKeyIndex, rot.GetAt(1));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								yCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,yCurve,pKeyIndex,fps);
								yCurve->KeyModifyEnd();
							}
						}
						else
						{
							yCurve->KeyModifyBegin();
							pKeyIndex = yCurve->KeyAdd(pTime);
							yCurve->KeySetValue(pKeyIndex, rot.GetAt(1));
							yCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							yCurve->KeyModifyEnd();
						}
					}
					
					KFCurve *zCurve = kNode->LclRotation.GetKFCurve(KFCURVENODE_R_Z, pTakeName.Buffer());
					if(zCurve)
					{
						DescID dscID = CDGetPSRTrackDescriptionID(CD_ROT_B);
						if(exportCurves)
						{
							if(HasKeyAtTime(doc,op,docTime,dscID))
							{
								zCurve->KeyModifyBegin();
								pKeyIndex = zCurve->KeyAdd(pTime);
								zCurve->KeySetValue(pKeyIndex, rot.GetAt(2));
								
								LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
								kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
								zCurve->KeySetInterpolation(pKeyIndex, kIntrp);
								
								if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,zCurve,pKeyIndex,fps);
								zCurve->KeyModifyEnd();
							}
						}
						else
						{
							zCurve->KeyModifyBegin();
							pKeyIndex = zCurve->KeyAdd(pTime);
							zCurve->KeySetValue(pKeyIndex, rot.GetAt(2));
							zCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
							zCurve->KeyModifyEnd();
						}
					}
					
					olist[i].oldRot = rot;
				}
				
				// user data tracks
				if(HasUserDataTracks(op))
				{
					DynamicDescription *dd = op->GetDynamicDescription();
					if(dd)
					{
						DescID dscID;
						const BaseContainer *bc;
						
						void *hnd = dd->BrowseInit();
						LONG ddCnt = 0;
						
						KFbxProperty lProperty = kNode->GetFirstProperty();
						while(lProperty.IsValid())
						{
							if(lProperty.GetFlag(KFbxUserProperty::eUSER))
							{
								if(dd->BrowseGetNext(hnd, &dscID, &bc))
								{
									DescLevel dscLevel = dscID.operator[](1);
										
									GeData data;
									CDGetParameter(op,dscID,data);
									if(CDHasAnimationTrack(op,dscID) && IsSupportedProperty(bc))
									{
										if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_VECTOR)
										{
											Vector vData = data.GetVector();
											
											KFCurve *fCurve = lProperty.GetKFCurve("X", pTakeName.Buffer());
											if(fCurve)
											{
												DescID xdscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0),DescLevel(dscLevel.id,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
												if(exportCurves)
												{
													if(HasKeyAtTime(doc,op,docTime,xdscID))
													{
														fCurve->KeyModifyBegin();
														pKeyIndex = fCurve->KeyAdd(pTime);
														if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
														{
															fCurve->KeySetValue(pKeyIndex,double(Deg(vData.x)));
														}
														else fCurve->KeySetValue(pKeyIndex,double(vData.x));
														
														LONG cKeyIndex = CDGetNearestKey(op,xdscID,curFrm,fps);
														kFCurveInterpolation kIntrp = GetKeyInterpolation(op,xdscID,cKeyIndex);
														fCurve->KeySetInterpolation(pKeyIndex, kIntrp);
														
														if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,xdscID,cKeyIndex,fCurve,pKeyIndex,fps);
														fCurve->KeyModifyEnd();
													}
												}
												else
												{
													fCurve->KeyModifyBegin();
													int lIndex = fCurve->KeyAdd(pTime);
													if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
													{
														fCurve->KeySetValue(lIndex,double(Deg(vData.x)));
													}
													else fCurve->KeySetValue(lIndex,double(vData.x));
													fCurve->KeyModifyEnd();
												}
											}
											fCurve = lProperty.GetKFCurve("Y", pTakeName.Buffer());
											if(fCurve)
											{
												DescID ydscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0),DescLevel(dscLevel.id,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
												if(exportCurves)
												{
													if(HasKeyAtTime(doc,op,docTime,ydscID))
													{
														fCurve->KeyModifyBegin();
														pKeyIndex = fCurve->KeyAdd(pTime);
														if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
														{
															fCurve->KeySetValue(pKeyIndex,double(Deg(vData.y)));
														}
														else fCurve->KeySetValue(pKeyIndex,double(vData.y));
														
														LONG cKeyIndex = CDGetNearestKey(op,ydscID,curFrm,fps);
														kFCurveInterpolation kIntrp = GetKeyInterpolation(op,ydscID,cKeyIndex);
														fCurve->KeySetInterpolation(pKeyIndex, kIntrp);
														
														if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,ydscID,cKeyIndex,fCurve,pKeyIndex,fps);
														fCurve->KeyModifyEnd();
													}
												}
												else
												{
													fCurve->KeyModifyBegin();
													int lIndex = fCurve->KeyAdd(pTime);
													if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
													{
														fCurve->KeySetValue(lIndex,double(Deg(vData.y)));
													}
													else fCurve->KeySetValue(lIndex,double(vData.y));
													fCurve->KeyModifyEnd();
												}
											}
											fCurve = lProperty.GetKFCurve("Z", pTakeName.Buffer());
											if(fCurve)
											{
												DescID zdscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0),DescLevel(dscLevel.id,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
												if(exportCurves)
												{
													if(HasKeyAtTime(doc,op,docTime,zdscID))
													{
														fCurve->KeyModifyBegin();
														pKeyIndex = fCurve->KeyAdd(pTime);
														if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
														{
															fCurve->KeySetValue(pKeyIndex,double(Deg(vData.z)));
														}
														else fCurve->KeySetValue(pKeyIndex,double(vData.z));
														
														LONG cKeyIndex = CDGetNearestKey(op,zdscID,curFrm,fps);
														kFCurveInterpolation kIntrp = GetKeyInterpolation(op,zdscID,cKeyIndex);
														fCurve->KeySetInterpolation(pKeyIndex, kIntrp);
														
														if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,zdscID,cKeyIndex,fCurve,pKeyIndex,fps);
														fCurve->KeyModifyEnd();
													}
												}
												else
												{
													fCurve->KeyModifyBegin();
													int lIndex = fCurve->KeyAdd(pTime);
													if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
													{
														fCurve->KeySetValue(lIndex,double(Deg(vData.z)));
													}
													else fCurve->KeySetValue(lIndex,double(vData.z));
													fCurve->KeyModifyEnd();
												}
											}
										}
										else
										{
											KFCurve *fCurve = lProperty.GetKFCurve(NULL, pTakeName.Buffer());
											if(fCurve)
											{
												if(exportCurves)
												{
													if(HasKeyAtTime(doc,op,docTime,dscID))
													{
														fCurve->KeyModifyBegin();
														pKeyIndex = fCurve->KeyAdd(pTime);
														if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
														{
															fCurve->KeySetValue(pKeyIndex, double(Deg(data.GetReal())));
														}
														else fCurve->KeySetValue(pKeyIndex, double(data.GetReal()));
														
														LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
														kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
														fCurve->KeySetInterpolation(pKeyIndex, kIntrp);
														
														if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,fCurve,pKeyIndex,fps);
														fCurve->KeyModifyEnd();
													}
												}
												else
												{
													fCurve->KeyModifyBegin();
													pKeyIndex = fCurve->KeyAdd(pTime);
													if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
													{
														fCurve->KeySetValue(pKeyIndex, double(Deg(data.GetReal())));
													}
													else fCurve->KeySetValue(pKeyIndex, double(data.GetReal()));
													fCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
													fCurve->KeyModifyEnd();
												}
											}
										}
									}
									ddCnt++;
								}
							}
							lProperty = kNode->GetNextProperty(lProperty);
						}
						dd->BrowseFree(hnd);
					}
				}
				
				// blend shapes
				if(type == NODE_TYPE_MESH)
				{
					if(exportMorph && !vCache)
					{
						BaseTag *mrTag = op->GetTag(ID_CDMORPHREFPLUGIN);
						if(mrTag)
						{
							LONG m, shpInd = 0;
							ShapeNode *slist = shapeList.Array();
							for(m=0; m<shapeList.Size(); m++)
							{
								BaseTag *tag = slist[m].mtag;
								if(tag)
								{
									KFbxGeometry* lGeometry = (KFbxGeometry*)kNode->GetNodeAttribute();
									if(lGeometry)
									{
										KFCurve *lCurve = lGeometry->GetShapeChannel(shpInd,true,pTakeName.Buffer());
										if(lCurve)
										{
											DescID dscID = DescID(DescLevel(M_MIX_SLIDER));
											if(exportCurves)
											{
												BaseContainer *tData = tag->GetDataInstance();
												if(!tData->GetBool(M_USE_BONE_ROTATION))
												{
													if(!HasKeyAtTime(doc,tag,docTime,dscID))
													{
														lCurve->KeyModifyBegin();
														pKeyIndex = lCurve->KeyAdd(pTime);
														lCurve->KeySetValue(pKeyIndex,GetMorphTagMix(tag)*100.0);
														
														LONG cKeyIndex = CDGetNearestKey(tag,dscID,curFrm,fps);
														kFCurveInterpolation kIntrp = GetKeyInterpolation(tag,dscID,cKeyIndex);
														lCurve->KeySetInterpolation(pKeyIndex, kIntrp);
														
														if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(tag,dscID,cKeyIndex,lCurve,pKeyIndex,fps,100.0);
														lCurve->KeyModifyEnd();
													}
												}
											}
											else
											{
												lCurve->KeyModifyBegin();
												pKeyIndex = lCurve->KeyAdd(pTime);
												lCurve->KeySetValue(pKeyIndex,GetMorphTagMix(tag)*100.0);
												lCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
												lCurve->KeyModifyEnd();
											}
										}
										shpInd++;
									}
								}
							}
						}
					}
				}
				
				if(type == NODE_TYPE_CAMERA)
				{
					if(CameraHasAnimatedParameters(op))
					{
						BaseContainer *cmData = op->GetDataInstance();
						if(cmData)
						{
							KFbxCamera *pCamera = (KFbxCamera*)kNode->GetNodeAttribute();
							if(pCamera)
							{
								if(CDHasAnimationTrack(op,DescLevel(CAMERA_FOCUS)))
								{
									Real fLen = cmData->GetReal(CAMERA_FOCUS);
									
									KFCurve *flCurve = pCamera->FocalLength.GetKFCurve(NULL, pTakeName.Buffer());
									if(flCurve)
									{
										DescID dscID = DescID(DescLevel(CAMERA_FOCUS));
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												flCurve->KeyModifyBegin();
												pKeyIndex = flCurve->KeyAdd(pTime);
												flCurve->KeySetValue(pKeyIndex, double(fLen));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												flCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,flCurve,pKeyIndex,fps);
												flCurve->KeyModifyEnd();
											}
										}
										else
										{
											flCurve->KeyModifyBegin();
											pKeyIndex = flCurve->KeyAdd(pTime);
											flCurve->KeySetValue(pKeyIndex, double(fLen));
											flCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											flCurve->KeyModifyEnd();
										}
									}
								}
								
								if(CDHasAnimationTrack(op,DescLevel(CAMERAOBJECT_TARGETDISTANCE)))
								{
									Real fDistance = cmData->GetReal(CAMERAOBJECT_TARGETDISTANCE);
									
									KFCurve *fdCurve = pCamera->FocusDistance.GetKFCurve(NULL, pTakeName.Buffer());
									if(fdCurve)
									{
										DescID dscID = DescID(DescLevel(CAMERAOBJECT_TARGETDISTANCE));
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												fdCurve->KeyModifyBegin();
												pKeyIndex = fdCurve->KeyAdd(pTime);
												fdCurve->KeySetValue(pKeyIndex, double(fDistance));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												fdCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,fdCurve,pKeyIndex,fps);
												fdCurve->KeyModifyEnd();
											}
										}
										else
										{
											fdCurve->KeyModifyBegin();
											pKeyIndex = fdCurve->KeyAdd(pTime);
											fdCurve->KeySetValue(pKeyIndex, double(fDistance));
											fdCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											fdCurve->KeyModifyEnd();
										}
									}
								}
							}
						}
					}
				}
				
				if(type == NODE_TYPE_LIGHT)
				{
					if(LightHasAnimatedParameters(op))
					{
						BaseContainer *ltData = op->GetDataInstance();
						if(ltData)
						{
							KFbxLight *pLight = (KFbxLight*)kNode->GetNodeAttribute();
							if(pLight)
							{
								// color
								DescID dscID = DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
								if(CDHasAnimationTrack(op,dscID))
								{
									Real xColor = ltData->GetVector(LIGHT_COLOR).x;
									
									KFCurve *flCurve = pLight->Color.GetKFCurve(KFCURVENODE_COLOR_RED, pTakeName.Buffer());
									if(flCurve)
									{
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												flCurve->KeyModifyBegin();
												pKeyIndex = flCurve->KeyAdd(pTime);
												flCurve->KeySetValue(pKeyIndex, double(xColor));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												flCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,flCurve,pKeyIndex,fps);
												flCurve->KeyModifyEnd();
											}
										}
										else
										{
											flCurve->KeyModifyBegin();
											pKeyIndex = flCurve->KeyAdd(pTime);
											flCurve->KeySetValue(pKeyIndex, double(xColor));
											flCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											flCurve->KeyModifyEnd();
										}
									}
								}
								dscID = DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
								if(CDHasAnimationTrack(op,dscID))
								{
									Real yColor = ltData->GetVector(LIGHT_COLOR).y;
									
									KFCurve *flCurve = pLight->Color.GetKFCurve(KFCURVENODE_COLOR_GREEN, pTakeName.Buffer());
									if(flCurve)
									{
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												flCurve->KeyModifyBegin();
												pKeyIndex = flCurve->KeyAdd(pTime);
												flCurve->KeySetValue(pKeyIndex, double(yColor));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												flCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,flCurve,pKeyIndex,fps);
												flCurve->KeyModifyEnd();
											}
										}
										else
										{
											flCurve->KeyModifyBegin();
											pKeyIndex = flCurve->KeyAdd(pTime);
											flCurve->KeySetValue(pKeyIndex, double(yColor));
											flCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											flCurve->KeyModifyEnd();
										}
									}
								}
								dscID = DescID(DescLevel(LIGHT_COLOR,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
								if(CDHasAnimationTrack(op,dscID))
								{
									Real zColor = ltData->GetVector(LIGHT_COLOR).z;
									
									KFCurve *flCurve = pLight->Color.GetKFCurve(KFCURVENODE_COLOR_BLUE, pTakeName.Buffer());
									if(flCurve)
									{
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												flCurve->KeyModifyBegin();
												pKeyIndex = flCurve->KeyAdd(pTime);
												flCurve->KeySetValue(pKeyIndex, double(zColor));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												flCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,flCurve,pKeyIndex,fps);
												flCurve->KeyModifyEnd();
											}
										}
										else
										{
											flCurve->KeyModifyBegin();
											pKeyIndex = flCurve->KeyAdd(pTime);
											flCurve->KeySetValue(pKeyIndex, double(zColor));
											flCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											flCurve->KeyModifyEnd();
										}
									}
								}
								
								// intensity
								if(CDHasAnimationTrack(op,DescLevel(LIGHT_BRIGHTNESS)))
								{
									Real intensity = ltData->GetReal(LIGHT_BRIGHTNESS);
									
									KFCurve *flCurve = pLight->Intensity.GetKFCurve(NULL, pTakeName.Buffer());
									if(flCurve)
									{
										DescID dscID = DescLevel(LIGHT_BRIGHTNESS);
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												flCurve->KeyModifyBegin();
												pKeyIndex = flCurve->KeyAdd(pTime);
												flCurve->KeySetValue(pKeyIndex, double(intensity));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												flCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,flCurve,pKeyIndex,fps);
												flCurve->KeyModifyEnd();
											}
										}
										else
										{
											flCurve->KeyModifyBegin();
											pKeyIndex = flCurve->KeyAdd(pTime);
											flCurve->KeySetValue(pKeyIndex, double(intensity));
											flCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											flCurve->KeyModifyEnd();
										}
									}
								}
								
								// spot light
								if(CDHasAnimationTrack(op,DescLevel(LIGHT_DETAILS_INNERANGLE)))
								{
									Real innerAngle = ltData->GetReal(LIGHT_DETAILS_INNERANGLE);
									
									KFCurve *flCurve = pLight->HotSpot.GetKFCurve(NULL, pTakeName.Buffer());
									if(flCurve)
									{
										DescID dscID = DescLevel(LIGHT_DETAILS_INNERANGLE);
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												flCurve->KeyModifyBegin();
												pKeyIndex = flCurve->KeyAdd(pTime);
												flCurve->KeySetValue(pKeyIndex, double(innerAngle));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												flCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,flCurve,pKeyIndex,fps);
												flCurve->KeyModifyEnd();
											}
										}
										else
										{
											flCurve->KeyModifyBegin();
											pKeyIndex = flCurve->KeyAdd(pTime);
											flCurve->KeySetValue(pKeyIndex, double(innerAngle));
											flCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											flCurve->KeyModifyEnd();
										}
									}
								}
								if(CDHasAnimationTrack(op,DescLevel(LIGHT_DETAILS_OUTERANGLE)))
								{
									Real outerAngle = ltData->GetReal(LIGHT_DETAILS_OUTERANGLE);
									
									KFCurve *flCurve = pLight->ConeAngle.GetKFCurve(NULL, pTakeName.Buffer());
									if(flCurve)
									{
										DescID dscID = DescLevel(LIGHT_DETAILS_OUTERANGLE);
										if(exportCurves)
										{
											if(HasKeyAtTime(doc,op,docTime,dscID))
											{
												flCurve->KeyModifyBegin();
												pKeyIndex = flCurve->KeyAdd(pTime);
												flCurve->KeySetValue(pKeyIndex, double(outerAngle));
												
												LONG cKeyIndex = CDGetNearestKey(op,dscID,curFrm,fps);
												kFCurveInterpolation kIntrp = GetKeyInterpolation(op,dscID,cKeyIndex);
												flCurve->KeySetInterpolation(pKeyIndex, kIntrp);
												
												if(kIntrp == KFCURVE_INTERPOLATION_CUBIC) SetKeyTangents(op,dscID,cKeyIndex,flCurve,pKeyIndex,fps);
												flCurve->KeyModifyEnd();
											}
										}
										else
										{
											flCurve->KeyModifyBegin();
											pKeyIndex = flCurve->KeyAdd(pTime);
											flCurve->KeySetValue(pKeyIndex, double(outerAngle));
											flCurve->KeySetInterpolation(pKeyIndex, KFCURVE_INTERPOLATION_CUBIC);
											flCurve->KeyModifyEnd();
										}
									}
								}
							}
						}
					}
				}
			}
		}
		
		// export PLA animations
		if(exportPLA || bakeMeshToPLA || bakeClothToPLA || bakeCDSkinToPLA)
		{
			for(i=0; i<objectList.Size(); i++)
			{
				KFbxNode *kNode = olist[i].node;
				BaseObject *op = olist[i].object;
				LONG type = olist[i].nodeType;
				Bool vCache = olist[i].pla;
				
				if(type == NODE_TYPE_MESH || type == NODE_TYPE_SPLINE)
				{
					if(vCache)
					{
						KFbxVertexCacheDeformer *lDeformer = static_cast<KFbxVertexCacheDeformer*>(kNode->GetGeometry()->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE));
						if(!lDeformer) continue;
						KFbxCache *lCache = lDeformer->GetCache();
						if(!lCache) continue;
						
						LONG pCnt = ToPoint(op)->GetPointCount();
						Vector *padr = GetPointArray(op);
						if(padr)
						{
							if(lCache->OpenFileForWrite(KFbxCache::eMC_ONE_FILE, KTime::GetFrameRate(pScene->GetGlobalTimeSettings().GetTimeMode()), kNode->GetName()))
							{
								StatusSetText("Building FBX Vertex Cache - "+op->GetName());
								
								if(bakeMeshToPLA || bakeClothToPLA || bakeCDSkinToPLA)
								{
									Vector *verts = NULL;
									
									LONG f;
									for(f=0; f<=numFrames; f++)
									{
										LONG status = LONG(Real(f)/Real(numFrames)*100.0);
										StatusSetBar(status);
										
										doc->SetTime(BaseTime(firstFrame+f,fps));
										CDAnimateDocument(doc);
										
										pTime.SetSecondDouble(double(Real(f)/fps));
										
										int lChannelIndex = lCache->GetChannelIndex(kNode->GetName());
										CDArray<fbxDouble3> lVertices;
										if(lVertices.Alloc(pCnt))
										{
											if(op->GetDeformCache()) ForceDeformCacheRebuild(doc,op);
												
											BaseObject *defOp = op->GetDeformCache();
											if(defOp) verts = GetPointArray(defOp);
											else verts = GetPointArray(op);
											
											for(int p=0; p<pCnt; p++)
											{
												Vector pt = verts[p] * exScale;;
												if(handed == 1) pt.z *= -1;
												
												lVertices[p] = fbxDouble3(double(pt.x),double(pt.y),double(pt.z));
											}
											
											if(!lCache->Write(lChannelIndex,pTime,&lVertices[0][0],int(pCnt)))
												exportError = CharToString(lCache->GetLastErrorString());
											
											lVertices.Free();
										}
									}
								}
								else
								{
									LONG k, kCnt = CDGetTrackKeyCount(op,DescID(ID_PLA_TRACK));
									for(k=0; k<kCnt; k++)
									{
										LONG status = LONG(Real(k)/Real(kCnt)*100.0);
										StatusSetBar(status);
										
										LONG f = CDGetKeyFrame(op,DescID(ID_PLA_TRACK),k,fps);
										if(f >= firstFrame && f <= numFrames)
										{
											doc->SetTime(BaseTime(firstFrame+f,fps));
											CDAnimateDocument(doc);
											
											pTime.SetSecondDouble(double(Real(f)/fps));
											
											int lChannelIndex = lCache->GetChannelIndex(kNode->GetName());
											CDArray<fbxDouble3> lVertices;
											if(lVertices.Alloc(pCnt))
											{
												for(int p=0; p<pCnt; p++)
												{
													Vector pt = padr[p] * exScale;;
													if(handed == 1) pt.z *= -1;
													
													lVertices[p] = fbxDouble3(double(pt.x),double(pt.y),double(pt.z));
												}
												
												if(!lCache->Write(lChannelIndex,pTime,&lVertices[0][0],int(pCnt)))
													exportError = CharToString(lCache->GetLastErrorString());
												
												lVertices.Free();
											}
										}
									}
								}
								
								if(!lCache->CloseFile())
									exportError = CharToString(lCache->GetLastErrorString());
							}
							else
								exportError = CharToString(lCache->GetLastErrorString());
						}
					}
				}
			}
		}
		CDFree(tName);
	}
	
	StatusClear();
}

Real CDExportFBXCommand::GetMorphTagMix(BaseTag *mTag)
{
	Real mix = 0.0;
	
	if(mTag)
	{
		BaseContainer *tData = mTag->GetDataInstance();
		if(tData)
		{
			if(tData->GetBool(M_USE_BONE_ROTATION))
			{
				Real value=0;
				BaseObject *op = mTag->GetObject();
				if(op)
				{
					BaseContainer *oData = op->GetDataInstance();
					if(oData)
					{
						if(op->GetType() == ID_CDJOINTOBJECT && oData)
						{
							Real minVal = tData->GetReal(M_MIN_VALUE);
							Real maxVal = tData->GetReal(M_MAX_VALUE);
							
							Vector opRot = oData->GetVector(CDJ_TRANS_ROT);
							
							switch (tData->GetLong(M_ROTATION_AXIS))
							{
								case M_ROTATION_H:	
									value = opRot.x;	// Get the H Rotation
									break;
								case M_ROTATION_P:
									value = opRot.y;	// Get the P Rotation
									break;
								case M_ROTATION_B:	
									value = opRot.z;	// Get the B Rotation
									break;
									break;
							}
							if(minVal < maxVal)
							{
								if(tData->GetBool(M_CLAMP_MIN) && value < minVal) value = minVal;
								if(tData->GetBool(M_CLAMP_MAX) && value > maxVal) value = maxVal;
							}
							else if(minVal > maxVal)
							{
								if(tData->GetBool(M_CLAMP_MIN) && value > minVal) value = minVal;
								if(tData->GetBool(M_CLAMP_MAX) && value < maxVal) value = maxVal;
							}
							
							if((maxVal-minVal) != 0) //Check for division by zero
							{
								mix = (value-minVal)/(maxVal-minVal);
							}
							else
							{
								mix = 0;
							}
						}
						else
						{
							Real minVal = tData->GetReal(M_MIN_VALUE);
							Real maxVal = tData->GetReal(M_MAX_VALUE);
							Vector opRot;
							
							BaseTag *ztTag = op->GetTag(ID_CDZEROTRANSTAG);
							if(ztTag)
							{
								BaseContainer *ztData = ztTag->GetDataInstance();
								if(ztData) opRot = ztData->GetVector(ZT_TRANS_ROT);
							}
							else opRot = CDGetRot(op);
							
							switch (tData->GetLong(M_ROTATION_AXIS))
							{
								case M_ROTATION_H:	
									value = opRot.x;	// Get the H Rotation
									break;
								case M_ROTATION_P:
									value = opRot.y;	// Get the P Rotation
									break;
								case M_ROTATION_B:	
									value = opRot.z;	// Get the B Rotation
									break;
									break;
							}
							if(minVal < maxVal)
							{
								if(tData->GetBool(M_CLAMP_MIN) && value < minVal) value = minVal;
								if(tData->GetBool(M_CLAMP_MAX) && value > maxVal) value = maxVal;
							}
							else if(minVal > maxVal)
							{
								if(tData->GetBool(M_CLAMP_MIN) && value > minVal) value = minVal;
								if(tData->GetBool(M_CLAMP_MAX) && value < maxVal) value = maxVal;
							}
							
							if((maxVal-minVal) != 0) //Check for division by zero
							{
								mix = (value-minVal)/(maxVal-minVal);
							}
							else
							{
								mix = 0;
							}
						}
					}
				}
			}
			else
			{
				mix = tData->GetReal(M_MIX_SLIDER);
			}
			
		}
	}
	
	return mix;
}

Bool CDExportFBXCommand::GetWeight(BaseDocument *doc, BaseTag *skin, BaseContainer *tData, Real *tWeight, LONG pCnt, LONG jInd)
{
	CDSkinVertex *skinWeight = CDGetSkinWeights(skin);
	if(!skinWeight) return false;
	
	LONG i;
	for(i=0; i<pCnt; i++)
	{
		if(skinWeight[i].taw > 0)
			tWeight[i] = skinWeight[i].jw[jInd] / skinWeight[i].taw;
		else
			tWeight[i] = 0.0;
	}
	
	return true;
}

int CDExportFBXCommand::GetPolygonMaterial(BaseDocument *doc, BaseObject *op, LONG pInd)
{
	BaseTag *tTag = NULL, *sTag = NULL;
	
	int mInd = -1;
	for(tTag = op->GetFirstTag(); tTag; tTag = tTag->GetNext())
	{	
		if(tTag && tTag->GetType() == Ttexture)
		{
			mInd++;
			GeData dGet;
			CDGetParameter(tTag,DescID(TEXTURETAG_RESTRICTION),dGet);
			String selectionName = dGet.GetString();
			
			for(sTag = op->GetFirstTag(); sTag; sTag = sTag->GetNext())
			{
				if(sTag->GetName() == selectionName)
				{
					BaseSelect *bs = static_cast<SelectionTag*>(sTag)->GetBaseSelect();
					if(bs)
					{
						LONG seg;
						if(bs->FindSegment(pInd,&seg)) return mInd;
					}
				}
			}
		}
	}
	
	// no restriction was found: Get the left most unrestricted texture tag
	mInd = 0;
	int tCnt = 0;
	for(tTag = op->GetFirstTag(); tTag; tTag = tTag->GetNext())
	{	
		if(tTag && tTag->GetType() == Ttexture)
		{
			GeData dGet;
			CDGetParameter(tTag,DescID(TEXTURETAG_RESTRICTION),dGet);
			String selectionName = dGet.GetString();
			if(!selectionName.Content()) mInd += tCnt;
			tCnt++;
		}
	}
		
	return mInd;
}

KFbxNode* CDExportFBXCommand::ConvertJoint(BaseObject *op, KFbxSdkManager *pSdkManager, Bool root, KFbxNode *parentNode)
{
	if(onlySelected && !op->GetBit(BIT_ACTIVE)) return NULL;
	
	KFbxNode *pSklt = NULL;
	
	char *oName = StringToChar("");
	if(exportAlphanumeric)
		oName = StringToChar(FixNameCharacters(ResolveNodeNames(op)));
	else
		oName = StringToChar(ResolveNodeNames(op));

	KString pSkltName = KString(oName);
	
	pSklt = KFbxNode::Create(pSdkManager,pSkltName.Buffer());
	if(pSklt)
	{
		Matrix opM = op->GetMg();
		if(handed == 1) opM = op->GetMg() * jRotM;
		
		KFbxXMatrix kM = MatrixToKFbxXMatrix(opM,handed,exScale);
		
		ObjectNode onode = ObjectNode(NODE_TYPE_JOINT,op,pSklt,kM,false);
		objectList.Append(onode);
		jointList.Append(onode);
		
		KFbxSkeleton *pSkltAttribute = KFbxSkeleton::Create(pSdkManager, pSkltName);
		if(pSkltAttribute)
		{
			if(root) pSkltAttribute->SetSkeletonType(KFbxSkeleton::eROOT);
			else pSkltAttribute->SetSkeletonType(KFbxSkeleton::eLIMB_NODE);
			
			ObjectColorProperties prop;
			op->GetColorProperties(&prop);
			pSkltAttribute->Color.Set(fbxDouble3(prop.color.x, prop.color.y, prop.color.z));
			
			pSklt->SetNodeAttribute(pSkltAttribute);
		}
		
		KFbxXMatrix localM = GetLocalMatrix(op,kM,GetObjectFromNode(parentNode));
		pSklt->SetDefaultT(localM.GetT());
		pSklt->SetDefaultR(localM.GetR());
		pSklt->SetPreferedAngle(localM.GetR());
				
		if(exportUserData) AddUserProperties(op,pSklt);
	}
	CDFree(oName);
	
	return pSklt;
}

KFbxNode* CDExportFBXCommand::ConvertMesh(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, BaseObject *gen, KFbxNode *parentNode)
{
	if(onlySelected && !op->GetBit(BIT_ACTIVE)) return NULL;
	
	KFbxNode *kNode = NULL;
	BaseObject *trg = NULL;
	
	if(!gen) trg = op;
	else trg = gen;
	
	Vector *padr = NULL;
	
	Bool dfrmAnim = false;
	BaseObject *dCacheOp = op->GetDeformCache();
	if(dCacheOp) dfrmAnim = HasParameterTracks(GetObjectDeformer(op));
	
	if(dCacheOp && !dfrmAnim) padr = GetPointArray(dCacheOp);
	else padr = GetPointArray(op);
	if(!padr) return NULL;
	
	CPolygon *vadr = GetPolygonArray(op); if(!vadr) return NULL;
	
	char *oName = StringToChar("");
	if(exportAlphanumeric)
		oName = StringToChar(FixNameCharacters(ResolveNodeNames(op)));
	else
		oName = StringToChar(ResolveNodeNames(op));
	
	KString pMeshName = KString(oName);
	
	KFbxMesh *pMesh = KFbxMesh::Create(pSdkManager,pMeshName.Buffer());
	if(pMesh)
	{
		StatusSetText("Building FBX Mesh "+op->GetName());
		KFbxXMatrix kM = MatrixToKFbxXMatrix(op->GetMg(),handed,exScale);
		
		LONG ptCnt = ToPoint(op)->GetPointCount();
		
		pMesh->InitControlPoints(ptCnt);
		KFbxVector4 *controlPoints = pMesh->GetControlPoints();

		CDMRefPoint *skinRef = NULL;
		BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN);
		if(skrTag)
		{
			skinRef = CDGetSkinReference(skrTag);
		}
		
		// create vertices
		for(int i=0; i<ptCnt; i++)
		{
			StatusSetSpin();
			Vector pt;
			if(!skinRef) pt = padr[i] * exScale;
			else pt = skinRef[i].GetVector() * exScale;
			
			if(handed == 1) pt.z *= -1;
			controlPoints[i] = VectorToKFbxVector4(pt);
		}
		
		LONG plyCnt = ToPoly(op)->GetPolygonCount();
		
		// create layer 0
		KFbxLayer *lLayer = pMesh->GetLayer(0);
		if(!lLayer)
		{
			pMesh->CreateLayer();
			lLayer = pMesh->GetLayer(0);
		}
		
		// Set material mapping.
		KFbxLayerElementMaterial *lMaterialLayer = KFbxLayerElementMaterial::Create(pMesh, "");
		if(lMaterialLayer)
		{
			lMaterialLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
			lMaterialLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
			lLayer->SetMaterials(lMaterialLayer);
		}
		
		// Create UV for Diffuse channel
		KFbxLayerElementUV *lUVDiffuseLayer = NULL;
		UVWTag *uvTag = (UVWTag*)op->GetTag(Tuvw);
		if(uvTag)
		{
			lUVDiffuseLayer = KFbxLayerElementUV::Create(pMesh, "DiffuseUV");
			if(lUVDiffuseLayer)
			{
				lUVDiffuseLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON_VERTEX);
				lUVDiffuseLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
				lLayer->SetUVs(lUVDiffuseLayer, KFbxLayerElement::eDIFFUSE_TEXTURES);

				LONG uvCnt = 0;
				for(int i=0; i<plyCnt; i++)
				{
					StatusSetSpin();
					UVWStruct uv;
					CDGetUVWStruct(uvTag,uv,i);
					
					CPolygon ply = vadr[i];
					if(handed == 1)
					{
						lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.c.x),1.0-double(uv.c.y)));
						uvCnt++;
						
						lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.b.x),1.0-double(uv.b.y)));
						uvCnt++;
						
						lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.a.x),1.0-double(uv.a.y)));
						uvCnt++;
						
						if(ply.c != ply.d)
						{
							lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.d.x),1.0-double(uv.d.y)));
							uvCnt++;
						}
					}
					else
					{
						lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.a.x),1.0-double(uv.a.y)));
						uvCnt++;
						
						lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.b.x),1.0-double(uv.b.y)));
						uvCnt++;
						
						lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.c.x),1.0-double(uv.c.y)));
						uvCnt++;
						
						if(ply.c != ply.d)
						{
							lUVDiffuseLayer->GetDirectArray().Add(KFbxVector2(double(uv.d.x),1.0-double(uv.d.y)));
							uvCnt++;
						}
					}
				}
				lUVDiffuseLayer->GetIndexArray().SetCount(uvCnt);
			}
		}

		// create polygons
		LONG uvInd = 0;
		for(int i=0; i<plyCnt; i++)
		{
			StatusSetSpin();
			pMesh->BeginPolygon(GetPolygonMaterial(doc,op,i), -1, -1, false);
			
			CPolygon ply = vadr[i];
			if(handed == 1)
			{
				pMesh->AddPolygon(ply.c);
				if(uvTag) pMesh->SetTextureUVIndex(i,0,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
				uvInd++;
				
				pMesh->AddPolygon(ply.b);
				if(uvTag) pMesh->SetTextureUVIndex(i,1,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
				uvInd++;
				
				pMesh->AddPolygon(ply.a);
				if(uvTag) pMesh->SetTextureUVIndex(i,2,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
				uvInd++;
				
				if(ply.c != ply.d)
				{
					pMesh->AddPolygon(ply.d);
					if(uvTag) pMesh->SetTextureUVIndex(i,3,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
					uvInd++;
				}
			}
			else
			{
				pMesh->AddPolygon(ply.a);
				if(uvTag) pMesh->SetTextureUVIndex(i,0,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
				uvInd++;
				
				pMesh->AddPolygon(ply.b);
				if(uvTag) pMesh->SetTextureUVIndex(i,1,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
				uvInd++;
				
				pMesh->AddPolygon(ply.c);
				if(uvTag) pMesh->SetTextureUVIndex(i,2,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
				uvInd++;
				
				if(ply.c != ply.d)
				{
					pMesh->AddPolygon(ply.d);
					if(uvTag) pMesh->SetTextureUVIndex(i,3,uvInd,KFbxLayerElement::eDIFFUSE_TEXTURES);
					uvInd++;
				}
			}
			
			pMesh->EndPolygon();
		}
		
		kNode = KFbxNode::Create(pSdkManager,pMeshName);
		kNode->SetNodeAttribute(pMesh);
		
		KFbxXMatrix localM = GetLocalMatrix(op,kM,GetObjectFromNode(parentNode));
		kNode->SetDefaultT(localM.GetT());
		kNode->SetDefaultR(localM.GetR());
		
		ObjectNode onode = ObjectNode(NODE_TYPE_MESH,trg,kNode,kM,false);
		objectList.Append(onode);
		
		// build normal layer element
		KFbxLayerElementNormal* lNormalLayer= KFbxLayerElementNormal::Create(pMesh, "");
		if(lNormalLayer)
		{
			lNormalLayer->SetReferenceMode(KFbxLayerElement::eDIRECT);
			lNormalLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON_VERTEX);
			
			if(op->GetTag(Tphong))
			{
			#if API_VERSION < 12000
				Vector *phN = ToPoly(op)->CreatePhongNormals();
			#else
				SVector *phN = ToPoly(op)->CreatePhongNormals();
			#endif
				Matrix rhM = Matrix();
				rhM.v3 *= -1;
				
				for(int i=0; i<plyCnt; i++)
				{
					StatusSetSpin();
					CPolygon ply = vadr[i];
					Vector aN, bN, cN, dN;
					
				#if API_VERSION < 12000
					aN = VNorm(phN[i*4]);
					bN = VNorm(phN[i*4+1]);
					cN = VNorm(phN[i*4+2]);
					dN = VNorm(phN[i*4+3]);
				#else
                    #if API_VERSION < 15000
                        aN = VNorm(phN[i*4].ToRV());
                        bN = VNorm(phN[i*4+1].ToRV());
                        cN = VNorm(phN[i*4+2].ToRV());
                        dN = VNorm(phN[i*4+3].ToRV());
                    #else
                        aN = VNorm((Vector)phN[i*4]);
                        bN = VNorm((Vector)phN[i*4+1]);
                        cN = VNorm((Vector)phN[i*4+2]);
                        dN = VNorm((Vector)phN[i*4+3]);
                    #endif
				#endif
					
					if(handed == 1)
					{
						lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(rhM * cN));
						lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(rhM * bN));
						lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(rhM * aN));
						if(ply.c != ply.d) lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(rhM * dN));
					}
					else
					{
						lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(aN));
						lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(bN));
						lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(cN));
						if(ply.c != ply.d) lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(dN));
					}
				}
				if(phN) CDFree(phN);
			}
			else
			{
				for(int i=0; i<plyCnt; i++)
				{
					StatusSetSpin();
					
					CPolygon ply = vadr[i];
					Vector normal = CalcFaceNormal(padr,ply);
					if(handed == 1) normal.z *= -1;
					
					lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(VNorm(normal)));
					lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(VNorm(normal)));
					lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(VNorm(normal)));
					if(ply.c != ply.d)
						lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(VNorm(normal)));
				}
			}
			
			lLayer->SetNormals(lNormalLayer);
		}
		
		if(exportSmoothing)
		{
			// build edge smoothing
			pMesh->BuildMeshEdgeArray();
			
			KFbxLayerElementSmoothing* lSmoothingElement = KFbxLayerElementSmoothing::Create(pMesh, "");
			if(lSmoothingElement)
			{
				lSmoothingElement->SetMappingMode(KFbxLayerElement::eBY_EDGE);
				lSmoothingElement->SetReferenceMode(KFbxLayerElement::eDIRECT);
				
				if(op->GetTag(Tphong))
				{
					for(int i = 0; i < pMesh->GetMeshEdgeCount(); i++)
					{
						StatusSetSpin();
						lSmoothingElement->GetDirectArray().Add(1);
					}
					
					BaseSelect *bs = ToPoly(op)->GetPhongBreak();
					if(bs)
					{
						LONG seg=0,a,b;
						while(CDGetRange(bs,seg++,&a,&b))
						{
							for(int i=a; i<=b; ++i)
							{
								StatusSetSpin();
								int aInd,bInd;
								GetEdgePointIndices(padr,vadr,i,aInd,bInd);
								bool rev;
								int edgeIndex = pMesh->GetMeshEdgeIndex(aInd,bInd,rev);
								lSmoothingElement->GetDirectArray().SetAt(edgeIndex,0);
							}
						}
					}
				}
				else
				{
					for(int i = 0; i < pMesh->GetMeshEdgeCount(); i++)
					{
						StatusSetSpin();
						lSmoothingElement->GetDirectArray().Add(0);
					}
				}
				
				lLayer->SetSmoothing(lSmoothingElement);
			}
		}
		
		CreateMaterials(doc,trg,pSdkManager,pMesh);
		
		if(BakeMeshToPLA(trg))
		{
			CreateVertexCache(pSdkManager,kNode);
		}
		else
		{
			BaseTag *skin = op->GetTag(ID_CDSKINPLUGIN);
			if(skin) CreateClusters(doc,op,skin,pSdkManager,kNode);
			
			BaseTag *mRef = op->GetTag(ID_CDMORPHREFPLUGIN);
			if(exportMorph && mRef) CreateShapes(doc,op,mRef,pSdkManager,kNode,ptCnt);
		}
	}
	
	CDFree(oName);
	
	return kNode;
}

KFbxNode* CDExportFBXCommand::ConvertLight(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode)
{
	if(onlySelected && !op->GetBit(BIT_ACTIVE)) return NULL;
	
	KFbxNode *kNode = NULL;
	
	char *oName = StringToChar("");
	if(exportAlphanumeric)
		oName = StringToChar(FixNameCharacters(ResolveNodeNames(op)));
	else
		oName = StringToChar(ResolveNodeNames(op));

	KString pLightName = KString(oName);

	KFbxLight *pLight = KFbxLight::Create(pSdkManager,pLightName.Buffer());
	if(pLight)
	{
		Matrix fbxM, opM = op->GetMg();
		fbxM.off = opM.off;
		fbxM.v1 = -opM.v2;
		fbxM.v2 = -opM.v3;
		fbxM.v3 = opM.v1;
		
		KFbxXMatrix kM = MatrixToKFbxXMatrix(fbxM,handed,exScale);
		
		GeData d;
		if(CDGetParameter(op,DescLevel(LIGHT_TYPE),d))
		{
			if(d == LIGHT_TYPE_PARALLEL)
			{
				pLight->LightType.Set(KFbxLight::eDIRECTIONAL);
			}
			else if(d == LIGHT_TYPE_SPOT)
			{
				pLight->LightType.Set(KFbxLight::eSPOT);
				
				if(CDGetParameter(op,DescLevel(LIGHT_DETAILS_INNERANGLE),d))
					pLight->HotSpot.Set(double(Deg(d.GetReal())));
				
				if(CDGetParameter(op,DescLevel(LIGHT_DETAILS_OUTERANGLE),d))
					pLight->ConeAngle.Set(double(Deg(d.GetReal())));
			}
			else
			{
				pLight->LightType.Set(KFbxLight::ePOINT);
			}
		}
		
		if(CDGetParameter(op,DescLevel(LIGHT_BRIGHTNESS),d))
			pLight->Intensity.Set(double(d.GetReal()*100.0));
		
		if(CDGetParameter(op,DescLevel(LIGHT_COLOR),d))
		{
			Vector color = d.GetVector();
			pLight->Color.Set(fbxDouble3(double(color.x),double(color.y),double(color.z)));
		}
		
		kNode = KFbxNode::Create(pSdkManager,pLightName);
		kNode->SetNodeAttribute(pLight);
		kNode->SetTarget(NULL);
		
		KFbxXMatrix localM = GetLocalMatrix(op,kM,GetObjectFromNode(parentNode));
		kNode->SetDefaultT(localM.GetT());
		kNode->SetDefaultR(localM.GetR());
		
		ObjectNode onode = ObjectNode(NODE_TYPE_LIGHT,op,kNode,kM,false);
		objectList.Append(onode);
	}
	CDFree(oName);
	return kNode;
}

KFbxNode* CDExportFBXCommand::ConvertCamera(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode)
{
	if(onlySelected && !op->GetBit(BIT_ACTIVE)) return NULL;
	
	KFbxNode *kNode = NULL;
	
	char *oName = StringToChar("");
	if(exportAlphanumeric)
		oName = StringToChar(FixNameCharacters(ResolveNodeNames(op)));
	else
		oName = StringToChar(ResolveNodeNames(op));

	KString pCameraName = KString(oName);
	
	KFbxCamera *pCamera = KFbxCamera::Create(pSdkManager,pCameraName.Buffer());
	if(pCamera)
	{
		Matrix fbxM, opM = op->GetMg();
		fbxM.off = opM.off;
		fbxM.v1 = opM.v3;
		fbxM.v2 = opM.v2;
		fbxM.v3 = -opM.v1;
		
		KFbxXMatrix kM = MatrixToKFbxXMatrix(fbxM,handed,exScale);
		
		BaseContainer *cmData = op->GetDataInstance();
		if(cmData)
		{
			Real aspectRatio = 4.0/3.0; // default value;
			
			RenderData *rData = doc->GetActiveRenderData();
			if(rData)
			{
				BaseContainer *rSettings = rData->GetDataInstance();
				if(rSettings)
				{
					Real fxSize = rSettings->GetReal(RDATA_XRES, 0.f);
					Real fySize = rSettings->GetReal(RDATA_YRES, 0.f);
					
					if(fySize != 0.0) aspectRatio = fxSize/fySize; // check for division by 0
				}
			}
			
			pCamera->ProjectionType.Set(KFbxCamera::ePERSPECTIVE);
			pCamera->SetApertureMode(KFbxCamera::eFOCAL_LENGTH);
			
			Real aHeight, aWidth = cmData->GetReal(CAMERAOBJECT_APERTURE);
			aHeight = aWidth/aspectRatio;
			pCamera->SetApertureWidth(double(MillimeterToInch(aWidth)));
			pCamera->SetApertureHeight(double(MillimeterToInch(aHeight)));
			
			Real fLen = cmData->GetReal(CAMERA_FOCUS);
			pCamera->FocalLength.Set(double(fLen));
			
			Real fxOffset = cmData->GetReal(CAMERAOBJECT_FILM_OFFSET_X);
			Real fyOffset = cmData->GetReal(CAMERAOBJECT_FILM_OFFSET_Y);
		#ifdef __PC
			pCamera->FilmOffset.Set(fbxDouble2(double(MillimeterToInch(aWidth*fxOffset)),double(MillimeterToInch(aHeight*fyOffset))));
		#else
			pCamera->FilmOffsetX.Set(double(MillimeterToInch(aWidth*fxOffset)));
			pCamera->FilmOffsetY.Set(double(MillimeterToInch(aHeight*fyOffset)));
		#endif
			
			Real trgDist = cmData->GetReal(CAMERAOBJECT_TARGETDISTANCE);
			pCamera->FocusSource.Set(KFbxCamera::eSPECIFIC_DISTANCE);
			
			pCamera->UseDepthOfField.Set(true);
			pCamera->FocusDistance.Set(double(trgDist));
		}
		
		kNode = KFbxNode::Create(pSdkManager,pCameraName);
		kNode->SetNodeAttribute(pCamera);
		kNode->SetTarget(NULL);
		
		KFbxXMatrix localM = GetLocalMatrix(op,kM,GetObjectFromNode(parentNode));
		kNode->SetDefaultT(localM.GetT());
		kNode->SetDefaultR(localM.GetR());
		
		ObjectNode onode = ObjectNode(NODE_TYPE_CAMERA,op,kNode,kM,false);
		objectList.Append(onode);
	}
	CDFree(oName);
	return kNode;
}

KFbxNode* CDExportFBXCommand::ConvertNull(BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode)
{
	if(onlySelected && !op->GetBit(BIT_ACTIVE)) return NULL;
	
	KFbxNode *kNode = NULL;
	
	char *oName = StringToChar("");
	if(exportAlphanumeric)
		oName = StringToChar(FixNameCharacters(ResolveNodeNames(op)));
	else
		oName = StringToChar(ResolveNodeNames(op));

	KString pNullName = KString(oName);
	
	KFbxNull *pNull = KFbxNull::Create(pSdkManager,pNullName.Buffer());
	if(pNull)
	{
		kNode = KFbxNode::Create(pSdkManager,pNullName);
		
		ObjectColorProperties prop;
		op->GetColorProperties(&prop);
		pNull->Color.Set(fbxDouble3(prop.color.x, prop.color.y, prop.color.z));
		
		kNode->SetNodeAttribute(pNull);
		
		Matrix opM = op->GetMg();
		if(exportConstraint && handed == 1 && jntTarget->Find(op) > NOTOK) opM = op->GetMg() * jRotM;
		
		KFbxXMatrix kM = MatrixToKFbxXMatrix(opM,handed,exScale);
		
		ObjectNode onode = ObjectNode(NODE_TYPE_NULL,op,kNode,kM,false);
		objectList.Append(onode);
		
		KFbxXMatrix localM = GetLocalMatrix(op,kM,GetObjectFromNode(parentNode));
		kNode->SetDefaultT(localM.GetT());
		kNode->SetDefaultR(localM.GetR());
	}
	CDFree(oName);
	return kNode;
}

KFbxNode* CDExportFBXCommand::ConvertSpline(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *parentNode)
{
	if(onlySelected && !op->GetBit(BIT_ACTIVE)) return NULL;
	
	KFbxNode *kNode = NULL;
	
	char *oName = StringToChar("");
	if(exportAlphanumeric)
		oName = StringToChar(FixNameCharacters(ResolveNodeNames(op)));
	else
		oName = StringToChar(ResolveNodeNames(op));

	KString pNodeName = KString(oName);
	
	SplineObject *spline = op->GetRealSpline(); if(!spline) return NULL;
	Vector *padr = GetPointArray(spline); if(!padr) return NULL;
	LONG ptCnt = ToPoint(spline)->GetPointCount();
	
	KFbxNurbsCurve *pNCurve = KFbxNurbsCurve::Create(pSdkManager,pNodeName.Buffer());
	if(pNCurve)
	{
		StatusSetText("Building FBX Nurbs Curve "+op->GetName());
		
		kNode = KFbxNode::Create(pSdkManager,pNodeName);
		
		ObjectColorProperties prop;
		op->GetColorProperties(&prop);
		pNCurve->Color.Set(fbxDouble3(prop.color.x, prop.color.y, prop.color.z));
		
		kNode->SetNodeAttribute(pNCurve);
		
		Matrix opM = op->GetMg();
		if(exportConstraint && handed == 1 && jntTarget->Find(op) > NOTOK) opM = op->GetMg() * jRotM;
		
		KFbxXMatrix kM = MatrixToKFbxXMatrix(opM,handed,exScale);
		
		BaseContainer *oData = op->GetDataInstance();
		if(oData)
		{
			ObjectNode onode = ObjectNode(NODE_TYPE_SPLINE,op,kNode,kM,false);
			objectList.Append(onode);
			
			KFbxXMatrix localM = GetLocalMatrix(op,kM,GetObjectFromNode(parentNode));
			kNode->SetDefaultT(localM.GetT());
			kNode->SetDefaultR(localM.GetR());
			
			pNCurve->SetDimension(KFbxNurbsCurve::e3D);
			
			KFbxNurbsCurve::EType pVType;
			if(op->GetType() != Ospline) pVType = KFbxNurbsCurve::ePERIODIC;
			else
			{
				if(!oData->GetBool(SPLINEOBJECT_CLOSED)) pVType = KFbxNurbsCurve::eOPEN;
				else pVType = KFbxNurbsCurve::ePERIODIC;
			}
			
			int pOrder = 2;
			switch(oData->GetLong(SPLINEOBJECT_TYPE))
			{
				case SPLINEOBJECT_TYPE_LINEAR:
					pOrder = 2;
					break;
				case SPLINEOBJECT_TYPE_CUBIC:
					pOrder = 3;
					break;
				case SPLINEOBJECT_TYPE_AKIMA:
					pOrder = 3;
					break;
				case SPLINEOBJECT_TYPE_BSPLINE:
					pOrder = 3;
					break;
				case SPLINEOBJECT_TYPE_BEZIER:
					pOrder = 3;
					break;
			}
			pNCurve->SetOrder(pOrder);
			pNCurve->InitControlPoints(ptCnt,pVType);
			
			int knotCnt = pNCurve->GetKnotCount();
			double *pKnotVector = new double[sizeof(double)*knotCnt];
			
			if(pVType == KFbxNurbsCurve::eOPEN)
			{
				int knt = 0;
				for(int i=0; i<knotCnt; i++)
				{
					if(i > (pOrder-1) && i < (knotCnt - (pOrder-1))) knt++;
					pKnotVector[i] = knt;
				}
			}
			else
			{
				int knt = 0 - (pOrder-1);
				for(int i=0; i<knotCnt; i++)
				{
					pKnotVector[i] = knt;
					knt++;
				}
			}
			   
			memcpy(pNCurve->GetKnotVector(), pKnotVector, knotCnt*sizeof(double));
			
			KFbxVector4 *controlPoints = pNCurve->GetControlPoints();
			if(controlPoints)
			{
				// create vertices
				for(int i=0; i<ptCnt; i++)
				{
					StatusSetSpin();
					Vector pt = MInv(opM) * (op->GetMg() * (padr[i] * exScale));
					if(handed == 1) pt.z *= -1;
					controlPoints[i] = VectorToKFbxVector4(pt);
				}
			}
		}
		if(BakeMeshToPLA(op))
		{
			CreateVertexCache(pSdkManager,kNode);
		}
	}
	CDFree(oName);
	return kNode;
}

void CDExportFBXCommand::AddUserProperties(BaseObject *op, KFbxNode *kNode)
{
	DynamicDescription *opDD = op->GetDynamicDescription();
	if(opDD)
	{
		DescID dscID;
		const BaseContainer *bc;
		void *dscHnd = opDD->BrowseInit();
		
		while(opDD->BrowseGetNext(dscHnd, &dscID, &bc))
		{	
			GeData gData;
			CDGetParameter(op,dscID,gData);
			
			char *propertyName = StringToChar(""); 
			if(exportAlphanumeric)
				propertyName = StringToChar(FixNameCharacters(bc->GetString(DESC_NAME)));
			else
				propertyName = StringToChar(bc->GetString(DESC_NAME));

			
			// bool
			if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_BOOL)
			{
				KFbxProperty uProp = KFbxProperty::Create(kNode, propertyName, DTBool, propertyName);
				uProp.ModifyFlag(KFbxUserProperty::eUSER, true);
				uProp.ModifyFlag(KFbxUserProperty::eANIMATABLE, true);
				uProp.Set(int(gData.GetLong()));
			}
			// long
			else if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_LONG || bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_LONGSLIDER)
			{
				BaseContainer *cycle = bc->GetContainerInstance(DESC_CYCLE);
				if(cycle)
				{
					KFbxProperty uProp = KFbxProperty::Create(kNode, propertyName, DTStringList, propertyName);
					uProp.ModifyFlag(KFbxUserProperty::eUSER, true);
					uProp.ModifyFlag(KFbxUserProperty::eANIMATABLE, true);
					
					LONG i=0;
					char *menuName = NULL;
					while(cycle->GetString(i) != "")
					{
						menuName = StringToChar(cycle->GetString(i));
						uProp.AddEnumValue(menuName);
						i++;
					}
					uProp.Set(int(gData.GetLong()));
					CDFree(menuName);
				}
				else
				{
					KFbxProperty uProp = KFbxProperty::Create(kNode, propertyName, DTInteger, propertyName);
					uProp.ModifyFlag(KFbxUserProperty::eUSER, true);
					uProp.ModifyFlag(KFbxUserProperty::eANIMATABLE, true);
					uProp.Set(int(gData.GetLong()));
					uProp.SetLimits(double(bc->GetLong(DESC_MIN)),double(bc->GetLong(DESC_MAX)));
				}
			}
			// real
			else if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_REAL || bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_REALSLIDER || bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_REALSLIDERONLY)
			{
				KFbxProperty uProp = KFbxProperty::Create(kNode, propertyName, DTDouble, propertyName);
				uProp.ModifyFlag(KFbxUserProperty::eUSER, true);
				uProp.ModifyFlag(KFbxUserProperty::eANIMATABLE, true);
				uProp.Set(double(gData.GetReal()));
				
				if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
				{
					uProp.SetLimits(double(Deg(bc->GetReal(DESC_MIN))),double(Deg(bc->GetReal(DESC_MAX))));
				}
				else uProp.SetLimits(double(bc->GetReal(DESC_MIN)),double(bc->GetReal(DESC_MAX)));
			}
			// vector
			else if(bc->GetLong(DESC_CUSTOMGUI) == CUSTOMGUI_VECTOR)
			{
				KFbxProperty uProp = KFbxProperty::Create(kNode, propertyName, DTDouble4, propertyName);
				uProp.ModifyFlag(KFbxUserProperty::eUSER, true);
				uProp.ModifyFlag(KFbxUserProperty::eANIMATABLE, true);
				Vector v = gData.GetVector();
				uProp.Set(fbxDouble3(v.x,v.y,v.z));
				
				if(bc->GetLong(DESC_UNIT) == DESC_UNIT_DEGREE)
				{
					uProp.SetLimits(double(Deg(bc->GetReal(DESC_MIN))),double(Deg(bc->GetReal(DESC_MAX))));
				}
				else uProp.SetLimits(double(bc->GetReal(DESC_MIN)),double(bc->GetReal(DESC_MAX)));
			}
			CDFree(propertyName);
		}
		opDD->BrowseFree(dscHnd);
	}
}

void CDExportFBXCommand::StoreBindPose(KFbxSdkManager *pSdkManager, KFbxScene *pScene)
{
	KFbxPose *bindPose = KFbxPose::Create(pSdkManager,"Bind Pose");
	if(bindPose)
	{
		int i;
		ObjectNode *jlist = jointList.Array();
		for(i=0; i<jointList.Size(); i++)
		{
			KFbxNode *kNode = jlist[i].node;
			BaseObject *op = jlist[i].object;
			if(op && kNode)
			{
				KFbxMatrix bindMatrix = jlist[i].matrix;
				if(!jlist[i].cluster)
				{
					Matrix opM = op->GetMl();
					if(handed == 1)
					{
						BaseObject *pr = op->GetUp();
						if(pr && pr->GetType() == ID_CDJOINTOBJECT)
						{
							Matrix prM = pr->GetMg() * jRotM;
							opM = MInv(prM) * (op->GetMg() * jRotM);
						}
					}
					KFbxMatrix lclM = MatrixToKFbxXMatrix(opM,handed,exScale);
					LONG ind = jointList.GetIndex(op->GetUp());
					if(ind > -1)
					{
						KFbxMatrix prkM = jlist[ind].matrix;
						bindMatrix = prkM * lclM;
					}
				}
				bindPose->Add(kNode,bindMatrix);
			}
		}
			
		pScene->AddPose(bindPose);
		bindPose->SetIsBindPose(true);
	}
}

void CDExportFBXCommand::BuildShapeNodeList(BaseObject *op)
{
	while(op)
	{
		StatusSetSpin();
		BaseTag *tag = NULL;
		for(tag = op->GetFirstTag(); tag; tag = tag->GetNext())
		{
			if(tag && tag->GetType() == ID_CDMORPHTAGPLUGIN)
			{
				ShapeNode sn = ShapeNode(tag,NULL,ResolveShapeNames(tag));
				shapeList.Append(sn);
			}
		}
		BuildShapeNodeList(op->GetDown());
		
		op = op->GetNext();
	}
}

Bool CDExportFBXCommand::DocumentHasPLA(BaseObject *op)
{
	while(op)
	{
		if(CDHasAnimationTrack(op,DescID(ID_PLA_TRACK))) return true;
		
		if(DocumentHasPLA(op->GetDown())) return true;
		
		op = op->GetNext();
	}
	
	return false;
}

void CDExportFBXCommand::BuildContent(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager,  KFbxNode *rootNode)
{
	KFbxNode *parentNode = rootNode;
	
	while(op)
	{
		KFbxNode *kNode = NULL;
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			if(exportJoint)
			{
				if(IsRootJoint(op)) kNode = ConvertJoint(op,pSdkManager,true,parentNode);
				else kNode = ConvertJoint(op,pSdkManager,false,parentNode);
			}
		}
		else if(op->GetType() == Opolygon)
		{
			if(exportMesh) kNode = ConvertMesh(doc,op,pSdkManager,NULL,parentNode);
		}
		else if(op->GetType() == Olight)
		{
			if(exportLight) kNode = ConvertLight(doc,op,pSdkManager,parentNode);
		}
		else if(op->GetType() == Ocamera)
		{
			if(exportCamera) kNode = ConvertCamera(doc,op,pSdkManager,parentNode);
		}
		else if(op->GetRealSpline())
		{
			if(exportSpline) kNode = ConvertSpline(doc,op,pSdkManager,parentNode);
		}
		else if(HasGeometry(op))
		{
			if(exportMesh)
			{
				BaseObject *cacheOp = op->GetCache();
				if(cacheOp) kNode = ConvertMesh(doc,cacheOp,pSdkManager,op,parentNode);
			}
		}
		else kNode = ConvertNull(op,pSdkManager,parentNode);
			
		if(kNode)
		{
			if(exportUserData) AddUserProperties(op,kNode);
			
			parentNode->AddChild(kNode);
			BuildContent(doc,op->GetDown(),pSdkManager,kNode);
		}
		
		op = op->GetNext();
	}
}

BaseObject* CDExportFBXCommand::GetTexturedParent(BaseObject *op)
{
	while(op)
	{
		if(op->GetTag(Ttexture)) return op;
		op = op->GetUp();
	}
	return NULL;
}

Bool CDExportFBXCommand::UsesParentTexture(BaseObject *op)
{
	if(!op->GetTag(Ttexture))
	{
		if(GetTexturedParent(op)) return true;
	}
	
	return false;
}

void CDExportFBXCommand::CreateMaterials(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxMesh *pMesh)
{
	BaseTag *tag = NULL;
	BaseObject *trg = NULL;
	
	if(UsesParentTexture(op)) trg = GetTexturedParent(op->GetUp());
	else trg = op;
	
	for(tag = trg->GetFirstTag(); tag; tag = tag->GetNext())
	{
		StatusSetSpin();
		if(tag && tag->GetType() == Ttexture)
		{
			BaseMaterial *mat = static_cast<TextureTag*>(tag)->GetMaterial();
			if(mat)
			{
				char *mName = StringToChar(GetMaterialName(mat));
				KString lMaterialName = KString(mName);
				KString lShadingName = "Phong";
				
				KFbxLayer *lLayer = pMesh->GetLayer(0);
				if(lLayer)
				{
					// create material
					KFbxSurfacePhong *lMaterial = KFbxSurfacePhong::Create(pSdkManager, lMaterialName.Buffer());
					if(lMaterial)
					{
						lMaterial->GetShadingModel().Set(lShadingName);
						
						Vector chColor;
						Real chBrightness;
						
						LONG pos;
						Bool colorHasAlpha = false;
						
						GeData dGet;
						if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_COLOR)))
						{
							BaseChannel *channel = mat->GetChannel(CHANNEL_COLOR);
							if(channel)
							{
								Filename path, destPath;
								char *txName = NULL;
								
								BaseContainer chData = channel->GetData();
								Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
								if(texName.Content())
								{
									// create texture
									KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"DiffuseTexture");
									if(lTexture)
									{
										path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
										if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
										else path = path + texName;
										
										if(!embedTextures)
										{
											if(!GeFExist(path))
											{
												txCopyFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											destPath.SetString(txPath+texName.GetString());
											GeFCopyFile(path,destPath,false);
											txName = StringToChar(destPath.GetString());
										}
										else
										{
											if(!GeFExist(path))
											{
												txEmbedFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											txName = StringToChar(path.GetString());
										}
										lTexture->SetFileName(txName);
										
										// create diffuse texture layer
										KFbxLayerElementTexture* lTextureDiffuseLayer = KFbxLayerElementTexture::Create(pMesh, "DiffuseTextureLayer");
										if(lTextureDiffuseLayer)
										{
											lTextureDiffuseLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
											lTextureDiffuseLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
											
											lLayer->SetTextures(KFbxLayerElement::eDIFFUSE_TEXTURES, lTextureDiffuseLayer);
											lTextureDiffuseLayer->GetDirectArray().Add(lTexture);
											lMaterial->GetDiffuseColor().ConnectSrcObject(lTexture);
										}
										
										BaseChannel *alphChnl = mat->GetChannel(CHANNEL_ALPHA);
										if(alphChnl)
										{
											BaseContainer achData = alphChnl->GetData();
											Filename alphaName = achData.GetString(BASECHANNEL_TEXTURE);
											if(alphaName == texName)
											{
												// create alpha texture layer
												KFbxLayerElementTexture* lTextureAlphaLayer = KFbxLayerElementTexture::Create(pMesh, "AlphaTextureLayer");
												if(lTextureAlphaLayer)
												{
													lTextureAlphaLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
													lTextureAlphaLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
													lTexture->SetAlphaSource(KFbxTexture::eBLACK);
													
													lLayer->SetTextures(KFbxLayerElement::eTRANSPARENT_TEXTURES, lTextureAlphaLayer);
													lTextureAlphaLayer->GetDirectArray().Add(lTexture);
													lMaterial->GetTransparentColor().ConnectSrcObject(lTexture);
													
													colorHasAlpha = true;
												}
											}
										}
										CDFree(txName);
									}
								}
							}
							
							CDGetParameter(mat,DescID(MATERIAL_COLOR_COLOR),dGet);
							chColor = dGet.GetVector();
							
							CDGetParameter(mat,DescID(MATERIAL_COLOR_BRIGHTNESS),dGet);
							chBrightness = dGet.GetReal();
							
							fbxDouble3 lColor(double(chColor.x*chBrightness),double(chColor.y*chBrightness),double(chColor.z*chBrightness));
							lMaterial->GetDiffuseColor().Set(lColor);
							lMaterial->GetDiffuseFactor().Set(1.0);
						}
						
						if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_LUMINANCE)))
						{
							BaseChannel *channel = mat->GetChannel(CHANNEL_LUMINANCE);
							if(channel)
							{
								Filename path, destPath;
								char *txName = NULL;
								
								BaseContainer chData = channel->GetData();
								Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
								if(texName.Content())
								{
									KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"EmissiveTexture");
									if(lTexture)
									{
										path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
										if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
										else path = path + texName;
										
										if(!embedTextures)
										{
											if(!GeFExist(path))
											{
												txCopyFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											destPath.SetString(txPath+texName.GetString());
											GeFCopyFile(path,destPath,false);
											txName = StringToChar(destPath.GetString());
										}
										else
										{
											if(!GeFExist(path))
											{
												txEmbedFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											txName = StringToChar(path.GetString());
										}
										
										KFbxLayerElementTexture* lTextureEmissiveLayer = KFbxLayerElementTexture::Create(pMesh, "EmissiveTextureLayer");
										if(lTextureEmissiveLayer)
										{
											lTextureEmissiveLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
											lTextureEmissiveLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
											lLayer->SetTextures(KFbxLayerElement::eEMISSIVE_TEXTURES, lTextureEmissiveLayer);
											
											lTexture->SetFileName(txName);
											lTextureEmissiveLayer->GetDirectArray().Add(lTexture);
											
											lMaterial->GetEmissiveColor().ConnectSrcObject(lTexture);
										}
										CDFree(txName);
									}
								}
							}
							
							CDGetParameter(mat,DescID(MATERIAL_LUMINANCE_COLOR),dGet);
							chColor = dGet.GetVector();
							
							CDGetParameter(mat,DescID(MATERIAL_LUMINANCE_BRIGHTNESS),dGet);
							chBrightness = dGet.GetReal();
							
							fbxDouble3 lColor(double(chColor.x*chBrightness),double(chColor.y*chBrightness),double(chColor.z*chBrightness));
							lMaterial->GetEmissiveColor().Set(lColor);
							lMaterial->GetEmissiveFactor().Set(1.0);
						}

						if(!colorHasAlpha)
						{
							if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_TRANSPARENCY)))
							{
								BaseChannel *channel = mat->GetChannel(CHANNEL_TRANSPARENCY);
								if(channel)
								{
									Filename path, destPath;
									char *txName = NULL;
									
									BaseContainer chData = channel->GetData();
									Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
									if(texName.Content())
									{
										KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"TransparencyTexture");
										if(lTexture)
										{
											path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
											if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
											else path = path + texName;
											
											if(!embedTextures)
											{
												if(!GeFExist(path))
												{
													txCopyFailed = true;
													if(!missingTextures.FindFirst(texName.GetString(),&pos))
														missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
												}
												destPath.SetString(txPath+texName.GetString());
												GeFCopyFile(path,destPath,false);
												txName = StringToChar(destPath.GetString());
											}
											else
											{
												if(!GeFExist(path))
												{
													txEmbedFailed = true;
													if(!missingTextures.FindFirst(texName.GetString(),&pos))
														missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
												}
												txName = StringToChar(path.GetString());
											}
											
											KFbxLayerElementTexture* lTextureTransparencyLayer = KFbxLayerElementTexture::Create(pMesh, "TransparencyTextureLayer");
											if(lTextureTransparencyLayer)
											{
												lTextureTransparencyLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
												lTextureTransparencyLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
												lLayer->SetTextures(KFbxLayerElement::eTRANSPARENT_TEXTURES, lTextureTransparencyLayer);
												
												lTexture->SetFileName(txName);
												lTextureTransparencyLayer->GetDirectArray().Add(lTexture);
												
												lMaterial->GetTransparentColor().ConnectSrcObject(lTexture);
											}
											CDFree(txName);
										}
									}
								}
								
								CDGetParameter(mat,DescID(MATERIAL_TRANSPARENCY_COLOR),dGet);
								chColor = dGet.GetVector();
								
								CDGetParameter(mat,DescID(MATERIAL_TRANSPARENCY_BRIGHTNESS),dGet);
								chBrightness = dGet.GetReal();
								
								fbxDouble3 lColor(double(chColor.x*chBrightness),double(chColor.y*chBrightness),double(chColor.z*chBrightness));
								lMaterial->GetTransparentColor().Set(lColor);
								lMaterial->GetTransparencyFactor().Set(1.0);
							}
							else if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_ALPHA)))
							{
								BaseChannel *channel = mat->GetChannel(CHANNEL_ALPHA);
								if(channel)
								{
									Filename path, destPath;
									char *txName = NULL;
									
									BaseContainer chData = channel->GetData();
									Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
									if(texName.Content())
									{
										KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"Transparency Texture");
										if(lTexture)
										{
											path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
											if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
											else path = path + texName;
											
											if(!embedTextures)
											{
												if(!GeFExist(path))
												{
													txCopyFailed = true;
													if(!missingTextures.FindFirst(texName.GetString(),&pos))
														missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
												}
												destPath.SetString(txPath+texName.GetString());
												GeFCopyFile(path,destPath,false);
												txName = StringToChar(destPath.GetString());
											}
											else
											{
												if(!GeFExist(path))
												{
													txEmbedFailed = true;
													if(!missingTextures.FindFirst(texName.GetString(),&pos))
														missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
												}
												txName = StringToChar(path.GetString());
											}
											
											KFbxLayerElementTexture* lTextureTransparencyLayer = KFbxLayerElementTexture::Create(pMesh, "Transparency Texture");
											if(lTextureTransparencyLayer)
											{
												lTextureTransparencyLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
												lTextureTransparencyLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
												lLayer->SetTextures(KFbxLayerElement::eTRANSPARENT_TEXTURES, lTextureTransparencyLayer);
												
												lTexture->SetAlphaSource(KFbxTexture::eBLACK);
												
												lTexture->SetFileName(txName);
												lTextureTransparencyLayer->GetDirectArray().Add(lTexture);
												
												lMaterial->GetTransparentColor().ConnectSrcObject(lTexture);
											}
											CDFree(txName);
										}
									}
								}
							}
						}
						
						if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_REFLECTION)))
						{
							BaseChannel *channel = mat->GetChannel(CHANNEL_REFLECTION);
							if(channel)
							{
								Filename path, destPath;
								char *txName = NULL;
								
								BaseContainer chData = channel->GetData();
								Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
								if(texName.Content())
								{
									KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"ReflectionTexture");
									if(lTexture)
									{
										path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
										if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
										else path = path + texName;
										
										if(!embedTextures)
										{
											if(!GeFExist(path))
											{
												txCopyFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											destPath.SetString(txPath+texName.GetString());
											GeFCopyFile(path,destPath,false);
											txName = StringToChar(destPath.GetString());
										}
										else
										{
											if(!GeFExist(path))
											{
												txEmbedFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											txName = StringToChar(path.GetString());
										}
										
										KFbxLayerElementTexture* lTextureReflectionLayer = KFbxLayerElementTexture::Create(pMesh, "ReflectionTextureLayer");
										if(lTextureReflectionLayer)
										{
											lTextureReflectionLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
											lTextureReflectionLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
											lLayer->SetTextures(KFbxLayerElement::eREFLECTION_TEXTURES, lTextureReflectionLayer);
											
											lTexture->SetFileName(txName);
											lTextureReflectionLayer->GetDirectArray().Add(lTexture);
											
											lMaterial->GetReflectionColor().ConnectSrcObject(lTexture);
										}
										CDFree(txName);
									}
								}
							}
							
							CDGetParameter(mat,DescID(MATERIAL_REFLECTION_COLOR),dGet);
							chColor = dGet.GetVector();
							
							CDGetParameter(mat,DescID(MATERIAL_REFLECTION_BRIGHTNESS),dGet);
							chBrightness = dGet.GetReal();
							
							fbxDouble3 lColor(double(chColor.x*chBrightness),double(chColor.y*chBrightness),double(chColor.z*chBrightness));
							lMaterial->GetReflectionColor().Set(lColor);
							lMaterial->GetReflectionFactor().Set(1.0);
						}
						
						if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_BUMP)))
						{
							BaseChannel *channel = mat->GetChannel(CHANNEL_BUMP);
							if(channel)
							{
								Filename path, destPath;
								char *txName = NULL;
								
								BaseContainer chData = channel->GetData();
								Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
								if(texName.Content())
								{
									KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"BumpTexture");
									if(lTexture)
									{
										path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
										if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
										else path = path + texName;
										
										if(!embedTextures)
										{
											if(!GeFExist(path))
											{
												txCopyFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											destPath.SetString(txPath+texName.GetString());
											GeFCopyFile(path,destPath,false);
											txName = StringToChar(destPath.GetString());
										}
										else
										{
											if(!GeFExist(path))
											{
												txEmbedFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											txName = StringToChar(path.GetString());
										}
										
										KFbxLayerElementTexture* lTextureBumpLayer = KFbxLayerElementTexture::Create(pMesh, "BumpTextureLayer");
										if(lTextureBumpLayer)
										{
											lTextureBumpLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
											lTextureBumpLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
											lLayer->SetTextures(KFbxLayerElement::eBUMP_TEXTURES, lTextureBumpLayer);
											
											lTexture->SetFileName(txName);
											lTextureBumpLayer->GetDirectArray().Add(lTexture);
											
											lMaterial->GetBump().ConnectSrcObject(lTexture);
										}
										CDFree(txName);
									}
								}
							}
							
							CDGetParameter(mat,DescID(MATERIAL_BUMP_STRENGTH),dGet);
							Real bumpStrength = dGet.GetReal();
							lMaterial->GetBumpFactor().Set(double(bumpStrength));
						}
						
						if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_SPECULAR)))
						{
							CDGetParameter(mat,DescID(MATERIAL_SPECULAR_WIDTH),dGet);
							Real sWidth = dGet.GetReal();
							if(sWidth < 0.3) sWidth = 0.3;
							Real mix = (sWidth-0.3)/(1.0-0.3);
							Real shinyness = CDBlend(100.0,2.0,mix);
							lMaterial->GetShininess().Set(double(shinyness));
						}
							
						if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_SPECULARCOLOR)))
						{
							BaseChannel *channel = mat->GetChannel(CHANNEL_SPECULARCOLOR);
							if(channel)
							{
								Filename path, destPath;
								char *txName = NULL;
								
								BaseContainer chData = channel->GetData();
								Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
								if(texName.Content())
								{
									KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"SpecularTexture");
									if(lTexture)
									{
										path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
										if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
										else path = path + texName;
										
										if(!embedTextures)
										{
											if(!GeFExist(path))
											{
												txCopyFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											destPath.SetString(txPath+texName.GetString());
											GeFCopyFile(path,destPath,false);
											txName = StringToChar(destPath.GetString());
										}
										else
										{
											if(!GeFExist(path))
											{
												txEmbedFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											txName = StringToChar(path.GetString());
										}
										
										KFbxLayerElementTexture* lTextureSpecularLayer = KFbxLayerElementTexture::Create(pMesh, "SpecularTextureLayer");
										if(lTextureSpecularLayer)
										{
											lTextureSpecularLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
											lTextureSpecularLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
											lLayer->SetTextures(KFbxLayerElement::eSPECULAR_TEXTURES, lTextureSpecularLayer);
											
											lTexture->SetFileName(txName);
											lTextureSpecularLayer->GetDirectArray().Add(lTexture);
											
											lMaterial->GetSpecularColor().ConnectSrcObject(lTexture);
										}
										CDFree(txName);
									}
								}
							}
							
							CDGetParameter(mat,DescID(MATERIAL_SPECULAR_COLOR),dGet);
							chColor = dGet.GetVector();
							
							CDGetParameter(mat,DescID(MATERIAL_SPECULAR_BRIGHTNESS),dGet);
							chBrightness = dGet.GetReal();
							
							fbxDouble3 lColor(double(chColor.x*chBrightness),double(chColor.y*chBrightness),double(chColor.z*chBrightness));
							lMaterial->GetSpecularColor().Set(lColor);
							lMaterial->GetSpecularFactor().Set(1.0);
						}
						
						if(MaterialChannelEnabled(mat,DescID(MATERIAL_USE_NORMAL)))
						{
							BaseChannel *channel = mat->GetChannel(CHANNEL_NORMAL);
							if(channel)
							{
								Filename path, destPath;
								char *txName = NULL;
								
								BaseContainer chData = channel->GetData();
								Filename texName = chData.GetString(BASECHANNEL_TEXTURE);
								if(texName.Content())
								{
									KFbxTexture* lTexture = KFbxTexture::Create(pSdkManager,"NormalTexture");
									if(lTexture)
									{
										path = chData.GetFilename(BASECHANNEL_SUGGESTEDFOLDER);
										if(!path.Content()) GenerateTexturePath(doc->GetDocumentPath(),texName,Filename(),&path);
										else path = path + texName;
										
										if(!embedTextures)
										{
											if(!GeFExist(path))
											{
												txCopyFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											destPath.SetString(txPath+texName.GetString());
											GeFCopyFile(path,destPath,false);
											txName = StringToChar(destPath.GetString());
										}
										else
										{
											if(!GeFExist(path))
											{
												txEmbedFailed = true;
												if(!missingTextures.FindFirst(texName.GetString(),&pos))
													missingTextures += texName.GetString()+ GeLoadString(MD_RETURN);
											}
											txName = StringToChar(path.GetString());
										}
										
										KFbxLayerElementTexture* lTextureNormalLayer = KFbxLayerElementTexture::Create(pMesh, "NormalTextureLayer");
										if(lTextureNormalLayer)
										{
											lTextureNormalLayer->SetMappingMode(KFbxLayerElement::eBY_POLYGON);
											lTextureNormalLayer->SetReferenceMode(KFbxLayerElement::eINDEX_TO_DIRECT);
											lLayer->SetTextures(KFbxLayerElement::eNORMALMAP_TEXTURES, lTextureNormalLayer);
											
											lTexture->SetFileName(txName);
											lTextureNormalLayer->GetDirectArray().Add(lTexture);
											
											lMaterial->GetBump().ConnectSrcObject(lTexture);
										}
										CDFree(txName);
									}
								}
							}
							
							CDGetParameter(mat,DescID(MATERIAL_NORMAL_STRENGTH),dGet);
							Real bumpStrength = dGet.GetReal();
							lMaterial->GetBumpFactor().Set(double(bumpStrength));
						}
						
						KFbxNode *lNode = pMesh->GetNode();
						if(lNode) lNode->AddMaterial(lMaterial);
					}
				}
				CDFree(mName);
			}
		}
	}
}

Bool CDExportFBXCommand::CreateVertexCache(KFbxSdkManager* pSdkManager, KFbxNode* kNode)
{
    // Get the point cache absolute and relative file name
    KString lAbsolutePCFileName = lFPCAbsoluteDirectory + KString("/") + kNode->GetName();
    lAbsolutePCFileName += ".xml";
	
    KString lRelativePCFileName = KFbxGetRelativeFilePath(KFbxExtractDirectory(lFBXAbsolutePath)+"/", lAbsolutePCFileName);
	
    // Make sure the direcotry exist.
    if(!KFbxEnsureDirectoryExistance(lAbsolutePCFileName))
    {
        MessageDialog(GeLoadString(MD_VERTEX_CACHE_FAILED));
        return false;
    }
	
    // Create the cache file
    KFbxCache* lCache = KFbxCache::Create(pSdkManager, kNode->GetName());
	if(lCache)
	{
		lCache->SetCacheFileName(lRelativePCFileName, lAbsolutePCFileName);
		lCache->SetCacheFileFormat(KFbxCache::eMC);
		
		// Create the vertex deformer
		KFbxVertexCacheDeformer* lDeformer = KFbxVertexCacheDeformer::Create(pSdkManager, kNode->GetName());
		if(lDeformer)
		{
			lDeformer->SetCache(lCache);
			lDeformer->SetCacheChannel(kNode->GetName());
			lDeformer->SetActive(true);
			
			// Apply the deformer on the mesh
			kNode->GetGeometry()->AddDeformer(lDeformer);
		}
	}
	
	return true;
}

Bool CDExportFBXCommand::CreateShapes(BaseDocument *doc, BaseObject *op, BaseTag *mRef, KFbxSdkManager *pSdkManager, KFbxNode* kNode, LONG ptCnt)
{
	CDMRefPoint *rpadr = CDGetMorphReference(mRef); if(!rpadr) return false;
	
	StatusSetText("Building FBX Blend Shapes for "+op->GetName());
	StatusSetBar(0);
	
	Vector *padr = GetPointArray(op); if(!padr) return false;
	CDArray<Vector> radr;
	if(!radr.Alloc(ptCnt)) return false;
	
	for(int j=0; j<ptCnt; j++)
	{
		Vector pt = rpadr[j].GetVector();
		radr[j] = pt;
	}
	
	ShapeNode *slist = shapeList.Array();
	for(int i=0; i<shapeList.Size(); i++)
	{
		BaseTag *tag = slist[i].mtag;
		if(tag)
		{
			BaseContainer *tData = tag->GetDataInstance();
			if(tData)
			{
				BaseObject *mop = tData->GetObjectLink(M_DEST_LINK,doc);
				if(mop == op)
				{
					LONG bsCnt = tData->GetLong(M_BS_POINT_COUNT);
					if(tData->GetBool(M_OFFSET_EXISTS) && bsCnt > 0)
					{
						CDMPoint *mpadr = CDGetMorphPoints(tag);
						if(!mpadr)
						{
							radr.Free();
							return false;
						}
						
						char *sName = "";
						if(exportAlphanumeric)
							sName = StringToChar(FixNameCharacters(tag->GetName()));
						else
							sName = StringToChar(tag->GetName());

						KString pShapeName = KString(sName);
						KFbxShape *lShape = KFbxShape::Create(pSdkManager,"");
						if(lShape)
						{
							lShape->InitControlPoints(ptCnt);
							KFbxVector4 *lControlPoints = lShape->GetControlPoints();
							if(lControlPoints)
							{
								CopyMem(radr.Array(),padr,sizeof(Vector)*ptCnt);
								
								for(int j=0; j<bsCnt; j++)
								{
									LONG ind = mpadr[j].ind;
									if(ind < ptCnt)
									{
										Vector refPt = rpadr[ind].GetVector();
										Vector mpt = mpadr[j].GetDelta(refPt);
										padr[ind] = mpt;
									}
								}
								
								// specify normals per control point.
								KFbxLayer* lLayer = lShape->GetLayer(0);
								if(lLayer == NULL)
								{
									lShape->CreateLayer();
									lLayer = lShape->GetLayer(0);
								}
								
								KFbxLayerElementNormal* lNormalLayer = KFbxLayerElementNormal::Create(lShape, "");
								if(lNormalLayer)
								{
									lNormalLayer->SetMappingMode(KFbxLayerElement::eBY_CONTROL_POINT);
									lNormalLayer->SetReferenceMode(KFbxLayerElement::eDIRECT);
									
									// create control points and normals
									for(int j=0; j<ptCnt; j++)
									{
										LONG status = LONG(Real((i*ptCnt)+j)/Real(shapeList.Size()*ptCnt)*100.0);
										StatusSetBar(status);
										
										Vector pt = padr[j] * exScale;
										if(handed == 1) pt.z *= -1;
										lControlPoints[j] = VectorToKFbxVector4(pt);
										
										pt = CalcPointNormal(op,padr,j);
										if(handed == 1) pt.z *= -1;
										lNormalLayer->GetDirectArray().Add(VectorToKFbxVector4(pt));
									}
									
									lLayer->SetNormals(lNormalLayer);
								}
								
								int shpIndex = kNode->GetGeometry()->AddShape(lShape,pShapeName.Buffer());
								slist[i].shape = lShape;
							}
						}
						CDFree(sName);
					}
				}
			}
		}
	}
	radr.Free();
	
	int shpCnt = kNode->GetGeometry()->GetShapeCount();
	
	return true;
}

Bool CDExportFBXCommand::CreateClusters(BaseDocument *doc, BaseObject *op, BaseTag *skin, KFbxSdkManager *pSdkManager, KFbxNode* kNode)
{
	BaseContainer *tData = skin->GetDataInstance(); if(!tData) return false;
	
	Matrix opM = op->GetMg(), skM = tData->GetMatrix(SKIN_MATRIX);
	
	KFbxXMatrix meshBindMatrix = MatrixToKFbxXMatrix(skM,handed,exScale);
	
    KFbxGeometry *pNodeAttribute = (KFbxGeometry*) kNode->GetNodeAttribute(); if(!pNodeAttribute) return false;
    KFbxSkin *pSkin = KFbxSkin::Create(pSdkManager, ""); if(!pSkin) return false;
	
	// first normalize all weights
	Real minWt = 0.0;
	skin->Message(CD_MSG_NORMALIZE_WEIGHT,&minWt);
	
	LONG jCnt = tData->GetLong(J_COUNT), pCnt = ToPoint(op)->GetPointCount();
	ObjectNode *jlist = jointList.Array();
	StatusSetText("Building FBX Skin for "+op->GetName());
	for(int i=0; i<jCnt; i++)
	{
		LONG status = LONG(Real(i)/Real(jCnt)*100.0);
		StatusSetBar(status);
		
		BaseObject *jnt = tData->GetObjectLink(J_LINK+i,doc);
		if(jnt)
		{
			for(int j=0; j<jointList.Size(); j++)
			{
				if(jnt == jlist[j].object)
				{
					KFbxNode *linkNode = jlist[j].node;
					KFbxCluster *cluster = KFbxCluster::Create(pSdkManager,"");
					if(linkNode && cluster)
					{
						tData->SetLong(J_LINK_ID,i);
						CDArray<Real> tWeight;
						if(tWeight.Alloc(pCnt))
						{
							if(GetWeight(doc,skin,tData,tWeight.Array(),pCnt,i))
							{
								Matrix jM = tData->GetMatrix(J_MATRIX+i);
								if(handed == 1) jM = tData->GetMatrix(J_MATRIX+i) * jRotM; // flip X and Z axes for right handed
								
								jlist[j].matrix = MatrixToKFbxXMatrix(opM *(MInv(skM) * jM),handed,exScale);
								jlist[j].cluster = cluster;
								
								KFbxXMatrix clusterBindMatrix = MatrixToKFbxXMatrix(jM,handed,exScale);
								
								cluster->SetLink(linkNode);
								cluster->SetLinkMode(KFbxCluster::eTOTAL1);
								
								cluster->SetTransformMatrix(meshBindMatrix);
								cluster->SetTransformLinkMatrix(clusterBindMatrix);
								
								LONG p;
								for(p=0; p<pCnt; p++)
								{
									Real wt = tWeight[p];
									if(wt > 0.0)
									{
										cluster->AddControlPointIndex(p,double(wt));
									}
								}
								pSkin->AddCluster(cluster);
							}
							tWeight.Free();
						}
					}
					break;
				}
			}
		}
	}
	pNodeAttribute->AddDeformer(pSkin);
	
	return true;
}

void CDExportFBXCommand::CreateLimbNodes(KFbxNode *pSklt, BaseObject *op, KFbxSdkManager *pSdkManager)
{
	while(op)
	{
		StatusSetSpin();
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			KFbxNode *pLimb = ConvertJoint(op, pSdkManager,false,pSklt);
			if(pLimb)
			{
				if(exportUserData) AddUserProperties(op,pLimb);
				
				pSklt->AddChild(pLimb);
				CreateLimbNodes(pLimb,op->GetDown(),pSdkManager);
			}
		}
		else CreateLimbNodes(pSklt,op->GetDown(),pSdkManager);
		
		op = op->GetNext();
	}
}

Bool CDExportFBXCommand::CreateSkeletons(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode)
{
	if(!op || !lRootNode) return false;
	
	AutoAlloc<AtomArray> sklRoots; if(!sklRoots) return false;
	
	StatusSetText("Building FBX Skeleton");
	GetRootJoints(op,sklRoots);
	
	LONG i, cnt = sklRoots->GetCount();
	for(i=0; i<cnt; i++)
	{
		BaseObject *rootJoint = static_cast<BaseObject*>(sklRoots->GetIndex(i));
		if(rootJoint)
		{
			KFbxNode *pSkeletonRoot = ConvertJoint(rootJoint,pSdkManager,true,NULL); 
			if(pSkeletonRoot)
			{
				if(exportUserData) AddUserProperties(op,pSkeletonRoot);
				
				CreateLimbNodes(pSkeletonRoot,rootJoint->GetDown(),pSdkManager);
				lRootNode->AddChild(pSkeletonRoot);
			}
		}
	}
	
	return true;
}

Bool CDExportFBXCommand::CreateMeshes(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode)
{
	if(!op) return false;
	
	while(op)
	{
		KFbxNode *pMesh = NULL;
		
		if(op->GetType() == Opolygon)
		{
			pMesh = ConvertMesh(doc,op,pSdkManager,NULL,NULL);
		}
		else
		{
			if(op->GetType() != Osds)
			{
				BaseObject *cacheOp = op->GetCache();
				if(cacheOp)
				{
					pMesh = ConvertMesh(doc,cacheOp,pSdkManager,op,NULL);
				}
			}
		}
		
		if(pMesh)
		{
			if(exportUserData) AddUserProperties(op,pMesh);
			lRootNode->AddChild(pMesh);
		}
		
		CreateMeshes(doc,op->GetDown(),pSdkManager,lRootNode);
		
		op = op->GetNext();
	}
	
	return true;
}

Bool CDExportFBXCommand::CreateCameras(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode)
{
	if(!op) return false;
	
	StatusSetText("Creating FBX cameras");
	while(op)
	{
		StatusSetSpin();
		
		KFbxNode *pCamera = NULL;
		
		if(op->GetType() == Ocamera)
		{
			pCamera = ConvertCamera(doc,op,pSdkManager,NULL);
		}
		if(pCamera)
		{
			if(exportUserData) AddUserProperties(op,pCamera);
			lRootNode->AddChild(pCamera);
		}
		
		CreateCameras(doc,op->GetDown(),pSdkManager,lRootNode);
		
		op = op->GetNext();
	}
	
	return true;
}

Bool CDExportFBXCommand::CreateLights(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode)
{
	if(!op) return false;
	
	StatusSetText("Creating FBX lights");
	while(op)
	{
		StatusSetSpin();
		
		KFbxNode *pLight = NULL;
		
		if(op->GetType() == Olight)
		{
			pLight = ConvertLight(doc,op,pSdkManager,NULL);
		}		
		if(pLight)
		{
			if(exportUserData) AddUserProperties(op,pLight);
			lRootNode->AddChild(pLight);
		}
		
		CreateLights(doc,op->GetDown(),pSdkManager,lRootNode);
		
		op = op->GetNext();
	}
	
	return true;
}

Bool CDExportFBXCommand::CreateSplines(BaseDocument *doc, BaseObject *op, KFbxSdkManager *pSdkManager, KFbxNode *lRootNode)
{
	if(!op) return false;
	
	StatusSetText("Creating FBX nurbs curves");
	while(op)
	{
		StatusSetSpin();
		
		KFbxNode *pNCurve = NULL;
		
		if(op->GetRealSpline())
		{
			pNCurve = ConvertSpline(doc,op,pSdkManager,NULL);
		}		
		if(pNCurve)
		{
			if(exportUserData) AddUserProperties(op,pNCurve);
			lRootNode->AddChild(pNCurve);
		}
		
		CreateSplines(doc,op->GetDown(),pSdkManager,lRootNode);
		
		op = op->GetNext();
	}
	
	return true;
}

void CDExportFBXCommand::CreateConstraints(BaseDocument *doc, KFbxSdkManager *pSdkManager, KFbxScene *pScene)
{
	int i;
	int posCnt = 0, rotCnt = 0, scaCnt = 0, aimCnt = 0, prCnt = 0, ikCnt = 0;
	
	KFbxXMatrix localM;
	Matrix fbxM;
	
	ObjectNode *olist = objectList.Array();
	for(i=0; i<objectList.Size(); i++)
	{
		KFbxNode *kNode = olist[i].node;
		BaseObject *op = olist[i].object;
		if(op && kNode)
		{
			// position constraint
			BaseTag *pTag = op->GetTag(ID_CDPCONSTRAINTPLUGIN);
			if(pTag)
			{
				posCnt++;
				BaseContainer *tData = pTag->GetDataInstance();
				if(tData)
				{
					KFbxConstraintPosition *lPositionConstraint = KFbxConstraintPosition::Create(pSdkManager,"Position");
					if(lPositionConstraint)
					{
						lPositionConstraint->SetConstrainedObject(kNode);
						lPositionConstraint->SetWeight(double(tData->GetReal(P_STRENGTH)));
						
						LONG i, cnt = tData->GetLong(P_TARGET_COUNT);
						for(i=0; i<cnt; i++)
						{
							BaseObject *trg = tData->GetObjectLink(P_TARGET+i,doc);
							if(trg)
							{
								KFbxNode *trgNode = GetNodeFromObject(trg);
								if(trgNode)
								{
									if(cnt > 1 && tData->GetBool(P_USE_AB_MIX))
									{
										Real maxVal = 0.0, minVal = 1.0;
										if(cnt-1 != 0) minVal = 1.0/(Real)(cnt-1);//Check for division by zero
											
										Real mixVal, value = Abs((minVal*i) - tData->GetReal(P_AB_MIX));
										if(value > minVal) value = minVal;
										if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
										else mixVal = 0.0;
										
										lPositionConstraint->AddConstraintSource(trgNode,double(100 * mixVal));
									}
									else lPositionConstraint->AddConstraintSource(trgNode,double(100 * tData->GetReal(P_POS_MIX+i)));
								}
							}
						}

						lPositionConstraint->SetAffectX(tData->GetBool(P_AXIS_X));
						lPositionConstraint->SetAffectY(tData->GetBool(P_AXIS_Y));
						lPositionConstraint->SetAffectZ(tData->GetBool(P_AXIS_Z));
						
						Vector offset = Vector(tData->GetReal(P_OFFSET_X),tData->GetReal(P_OFFSET_Y),tData->GetReal(P_OFFSET_Z)) * exScale;
						if(handed == 1) offset.z *= -1;
						lPositionConstraint->SetOffset(VectorToKFbxVector4(offset));
						KFbxConnectDst(lPositionConstraint,pScene);
					}
				}
			}
			
			// rotation constraint
			BaseTag *rTag = op->GetTag(ID_CDRCONSTRAINTPLUGIN);
			if(rTag)
			{
				rotCnt++;
				BaseContainer *tData = rTag->GetDataInstance();
				if(tData)
				{
					KFbxConstraintRotation *lRotationConstraint = KFbxConstraintRotation::Create(pSdkManager,"Rotation");
					if(lRotationConstraint)
					{
						lRotationConstraint->SetConstrainedObject(kNode);
						lRotationConstraint->SetWeight(double(tData->GetReal(R_STRENGTH)));
						
						LONG i, cnt = tData->GetLong(R_TARGET_COUNT);
						for(i=0; i<cnt; i++)
						{
							BaseObject *trg = tData->GetObjectLink(R_TARGET+i,doc);
							if(trg)
							{
								KFbxNode *trgNode = GetNodeFromObject(trg);
								if(trgNode)
								{
									if(cnt > 1 && tData->GetBool(R_USE_AB_MIX))
									{
										Real maxVal = 0.0, minVal = 1.0;
										if(cnt-1 != 0) minVal = 1.0/(Real)(cnt-1);//Check for division by zero
											
										Real mixVal, value = Abs((minVal*i) - tData->GetReal(R_AB_MIX));
										if(value > minVal) value = minVal;
										if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
										else mixVal = 0.0;
										
										lRotationConstraint->AddConstraintSource(trgNode,double(100 * mixVal));
									}
									else lRotationConstraint->AddConstraintSource(trgNode,double(100 * tData->GetReal(R_ROT_MIX+i)));
								}
							}
						}
						
						lRotationConstraint->SetAffectX(tData->GetBool(R_AXIS_Y));
						lRotationConstraint->SetAffectY(tData->GetBool(R_AXIS_X));
						lRotationConstraint->SetAffectZ(tData->GetBool(R_AXIS_Z));
						
						Vector offset = Vector(Deg(tData->GetReal(R_OFFSET_Y)),Deg(tData->GetReal(R_OFFSET_X)),Deg(tData->GetReal(R_OFFSET_Z)));
						if(op->GetType() == ID_CDJOINTOBJECT && handed == 1)
							offset = Vector(Deg(tData->GetReal(R_OFFSET_Y)),Deg(tData->GetReal(R_OFFSET_Z)),Deg(tData->GetReal(R_OFFSET_X)));
						
						if(handed == 1) offset.z *= -1;
						lRotationConstraint->SetOffset(VectorToKFbxVector4(offset));
						KFbxConnectDst(lRotationConstraint,pScene);
					}
				}
			}
			
			// scale constraint
			BaseTag *sTag = op->GetTag(ID_CDSCONSTRAINTPLUGIN);
			if(sTag)
			{
				scaCnt++;
				BaseContainer *tData = sTag->GetDataInstance();
				if(tData)
				{
					KFbxConstraintScale *lScaleConstraint = KFbxConstraintScale::Create(pSdkManager,"Scale");
					if(lScaleConstraint)
					{
						lScaleConstraint->SetConstrainedObject(kNode);
						lScaleConstraint->SetWeight(double(tData->GetReal(S_STRENGTH)));
						
						LONG i, cnt = tData->GetLong(S_TARGET_COUNT);
						for(i=0; i<cnt; i++)
						{
							BaseObject *trg = tData->GetObjectLink(S_TARGET+i,doc);
							if(trg)
							{
								KFbxNode *trgNode = GetNodeFromObject(trg);
								if(trgNode)
								{
									if(cnt > 1 && tData->GetBool(S_USE_AB_MIX))
									{
										Real maxVal = 0.0, minVal = 1.0;
										if(cnt-1 != 0) minVal = 1.0/(Real)(cnt-1);//Check for division by zero
											
										Real mixVal, value = Abs((minVal*i) - tData->GetReal(S_AB_MIX));
										if(value > minVal) value = minVal;
										if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
										else mixVal = 0.0;
										
										lScaleConstraint->AddConstraintSource(trgNode,double(100 * mixVal));
									}
									else lScaleConstraint->AddConstraintSource(trgNode,double(100 * tData->GetReal(S_SCA_MIX+i)));
								}
							}
						}
						
						lScaleConstraint->SetAffectX(tData->GetBool(S_AXIS_X));
						lScaleConstraint->SetAffectY(tData->GetBool(S_AXIS_Y));
						lScaleConstraint->SetAffectZ(tData->GetBool(S_AXIS_Z));
						
						Vector offset = Vector(tData->GetReal(S_OFFSET_X)+1,tData->GetReal(S_OFFSET_Y)+1,tData->GetReal(S_OFFSET_Z)+1);
						lScaleConstraint->SetOffset(VectorToKFbxVector4(offset));
						KFbxConnectDst(lScaleConstraint,pScene);
					}
				}
			}
			
			// psr constraint
			BaseTag *psrTag = op->GetTag(ID_CDPSRCONSTRAINTPLUGIN);
			if(psrTag)
			{
				BaseContainer *tData = psrTag->GetDataInstance();
				if(tData)
				{
					// position constraint
					if(tData->GetBool(PSR_USE_P))
					{
						posCnt++;
						
						KFbxConstraintPosition *lPositionConstraint = KFbxConstraintPosition::Create(pSdkManager,"Position");
						if(lPositionConstraint)
						{
							lPositionConstraint->SetConstrainedObject(kNode);
							lPositionConstraint->SetWeight(double(tData->GetReal(PSR_STRENGTH)));
							
							LONG i, cnt = tData->GetLong(PSR_TARGET_COUNT);
							for(i=0; i<cnt; i++)
							{
								BaseObject *trg = tData->GetObjectLink(PSR_TARGET+i,doc);
								if(trg)
								{
									KFbxNode *trgNode = GetNodeFromObject(trg);
									if(trgNode)
									{
										if(cnt > 1 && tData->GetBool(PSR_USE_AB_MIX))
										{
											Real maxVal = 0.0, minVal = 1.0;
											if(cnt-1 != 0) minVal = 1.0/(Real)(cnt-1);//Check for division by zero
												
												Real mixVal, value = Abs((minVal*i) - tData->GetReal(PSR_AB_MIX));
												if(value > minVal) value = minVal;
												if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
											else mixVal = 0.0;
											
											lPositionConstraint->AddConstraintSource(trgNode,double(100 * mixVal));
										}
										else lPositionConstraint->AddConstraintSource(trgNode,double(100 * tData->GetReal(PSR_PSR_MIX+i)));
									}
								}
							}
							
							lPositionConstraint->SetAffectX(tData->GetBool(PSR_P_AXIS_X));
							lPositionConstraint->SetAffectY(tData->GetBool(PSR_P_AXIS_Y));
							lPositionConstraint->SetAffectZ(tData->GetBool(PSR_P_AXIS_Z));
							
							Vector offset = Vector(tData->GetReal(PSR_P_OFFSET_X),tData->GetReal(PSR_P_OFFSET_Y),tData->GetReal(PSR_P_OFFSET_Z)) * exScale;
							if(handed == 1) offset.z *= -1;
							lPositionConstraint->SetOffset(VectorToKFbxVector4(offset));
							KFbxConnectDst(lPositionConstraint,pScene);
						}
					}
					
					// rotation constraint
					if(tData->GetBool(PSR_USE_R))
					{
						rotCnt++;

						KFbxConstraintRotation *lRotationConstraint = KFbxConstraintRotation::Create(pSdkManager,"Rotation");
						if(lRotationConstraint)
						{
							lRotationConstraint->SetConstrainedObject(kNode);
							lRotationConstraint->SetWeight(double(tData->GetReal(PSR_STRENGTH)));
							
							LONG i, cnt = tData->GetLong(PSR_TARGET_COUNT);
							for(i=0; i<cnt; i++)
							{
								BaseObject *trg = tData->GetObjectLink(PSR_TARGET+i,doc);
								if(trg)
								{
									KFbxNode *trgNode = GetNodeFromObject(trg);
									if(trgNode)
									{
										if(cnt > 1 && tData->GetBool(PSR_USE_AB_MIX))
										{
											Real maxVal = 0.0, minVal = 1.0;
											if(cnt-1 != 0) minVal = 1.0/(Real)(cnt-1);//Check for division by zero
												
												Real mixVal, value = Abs((minVal*i) - tData->GetReal(PSR_AB_MIX));
												if(value > minVal) value = minVal;
												if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
											else mixVal = 0.0;
											
											lRotationConstraint->AddConstraintSource(trgNode,double(100 * mixVal));
										}
										else lRotationConstraint->AddConstraintSource(trgNode,double(100 * tData->GetReal(PSR_PSR_MIX+i)));
									}
								}
							}
							
							lRotationConstraint->SetAffectX(tData->GetBool(PSR_R_AXIS_Y));
							lRotationConstraint->SetAffectY(tData->GetBool(PSR_R_AXIS_X));
							lRotationConstraint->SetAffectZ(tData->GetBool(PSR_R_AXIS_Z));
							
							Vector offset = Vector(Deg(tData->GetReal(PSR_R_OFFSET_Y)),Deg(tData->GetReal(PSR_R_OFFSET_X)),Deg(tData->GetReal(PSR_R_OFFSET_Z)));
							if(op->GetType() == ID_CDJOINTOBJECT && handed == 1)
								offset = Vector(Deg(tData->GetReal(PSR_R_OFFSET_Y)),Deg(tData->GetReal(PSR_R_OFFSET_Z)),Deg(tData->GetReal(PSR_R_OFFSET_X)));
							
							if(handed == 1) offset.z *= -1;
							lRotationConstraint->SetOffset(VectorToKFbxVector4(offset));
							KFbxConnectDst(lRotationConstraint,pScene);
						}
					}
					
					// scale constraint
					if(tData->GetBool(PSR_USE_S))
					{
						scaCnt++;
						
						KFbxConstraintScale *lScaleConstraint = KFbxConstraintScale::Create(pSdkManager,"Scale");
						if(lScaleConstraint)
						{
							lScaleConstraint->SetConstrainedObject(kNode);
							lScaleConstraint->SetWeight(double(tData->GetReal(PSR_STRENGTH)));
							
							LONG i, cnt = tData->GetLong(PSR_TARGET_COUNT);
							for(i=0; i<cnt; i++)
							{
								BaseObject *trg = tData->GetObjectLink(PSR_TARGET+i,doc);
								if(trg)
								{
									KFbxNode *trgNode = GetNodeFromObject(trg);
									if(trgNode)
									{
										if(cnt > 1 && tData->GetBool(PSR_USE_AB_MIX))
										{
											Real maxVal = 0.0, minVal = 1.0;
											if(cnt-1 != 0) minVal = 1.0/(Real)(cnt-1);//Check for division by zero
												
												Real mixVal, value = Abs((minVal*i) - tData->GetReal(PSR_AB_MIX));
												if(value > minVal) value = minVal;
												if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
											else mixVal = 0.0;
											
											lScaleConstraint->AddConstraintSource(trgNode,double(100 * mixVal));
										}
										else lScaleConstraint->AddConstraintSource(trgNode,double(100 * tData->GetReal(PSR_PSR_MIX+i)));
									}
								}
							}
							
							lScaleConstraint->SetAffectX(tData->GetBool(PSR_S_AXIS_X));
							lScaleConstraint->SetAffectY(tData->GetBool(PSR_S_AXIS_Y));
							lScaleConstraint->SetAffectZ(tData->GetBool(PSR_S_AXIS_Z));
							
							Vector offset = Vector(tData->GetReal(PSR_S_OFFSET_X)+1,tData->GetReal(PSR_S_OFFSET_Y)+1,tData->GetReal(PSR_S_OFFSET_Z)+1);
							lScaleConstraint->SetOffset(VectorToKFbxVector4(offset));
							KFbxConnectDst(lScaleConstraint,pScene);
						}
					}
				}
			}
			
			
			// aim constraint
			BaseTag *aTag = op->GetTag(ID_CDACONSTRAINTPLUGIN);
			if(aTag)
			{
				aimCnt++;
				BaseContainer *tData = aTag->GetDataInstance();
				if(tData)
				{
					KFbxConstraintAim *lAimConstraint = KFbxConstraintAim::Create(pSdkManager,"Aim");
					if(lAimConstraint)
					{
						lAimConstraint->SetConstrainedObject(kNode);
						lAimConstraint->SetWeight(double(tData->GetReal(A_STRENGTH)));
						
						LONG i, cnt = tData->GetLong(A_TARGET_COUNT);
						for(i=0; i<cnt; i++)
						{
							BaseObject *trg = tData->GetObjectLink(A_TARGET+i,doc);
							if(trg)
							{
								KFbxNode *trgNode = GetNodeFromObject(trg);
								if(trgNode)
								{
									if(cnt > 1 && tData->GetBool(A_USE_AB_MIX))
									{
										Real maxVal = 0.0, minVal = 1.0;
										if(cnt-1 != 0) minVal = 1.0/(Real)(cnt-1);//Check for division by zero
											
										Real mixVal, value = Abs((minVal*i) - tData->GetReal(A_AB_MIX));
										if(value > minVal) value = minVal;
										if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
										else mixVal = 0.0;
										
										lAimConstraint->AddConstraintSource(trgNode,double(100 * mixVal));
									}
									else lAimConstraint->AddConstraintSource(trgNode,double(100 * tData->GetReal(A_AIM_MIX+i)));
								}
							}
						}
						
						// set aim vector
						Vector aimVector = Vector(0.0,0.0,1.0);
						if(op->GetType() == ID_CDJOINTOBJECT && handed == 1) aimVector = Vector(1.0,0.0,0.0); // flip X and Z axes for right handed
						
						if(tData->GetBool(A_Z_DIRECTION)) aimVector *= -1;
						if(handed == 1) aimVector.z *= -1;
						lAimConstraint->SetAimVector(VectorToKFbxVector4(aimVector));
						
						// set up vector
						Vector upVector;
						switch(tData->GetLong(A_POLE_AXIS))
						{
							case A_POLE_X:
								if(op->GetType() == ID_CDJOINTOBJECT && handed == 1) upVector = Vector(0.0,0.0,1.0); // flip X and Z axes for right handed
								upVector = Vector(1.0,0.0,0.0);
								break;
							case A_POLE_Y:
								upVector = Vector(0.0,1.0,0.0);
								break;
							case A_POLE_NX:
								if(op->GetType() == ID_CDJOINTOBJECT && handed == 1) upVector = Vector(0.0,0.0,-1.0); // flip X and Z axes for right handed
								upVector = Vector(-1.0,0.0,0.0);
								break;
							case A_POLE_NY:
								upVector = Vector(0.0,-1.0,0.0);
								break;
							default:
								upVector = Vector(0.0,1.0,0.0);
								break;
						}
						lAimConstraint->SetUpVector(VectorToKFbxVector4(upVector));
						
						if(tData->GetLong(A_ALIGN_AXIS) > A_ALIGN_OFF)
						{
							switch(tData->GetLong(A_ALIGN_AXIS))
							{
								case A_ALIGN_X:
									if(op->GetType() == ID_CDJOINTOBJECT && handed == 1) upVector = Vector(0.0,0.0,1.0); // flip X and Z axes for right handed
									upVector = Vector(1.0,0.0,0.0);
									break;
								case A_ALIGN_Y:
									upVector = Vector(0.0,1.0,0.0);
									break;
								case A_ALIGN_NX:
									if(op->GetType() == ID_CDJOINTOBJECT && handed == 1) upVector = Vector(0.0,0.0,-1.0); // flip X and Z axes for right handed
									upVector = Vector(-1.0,0.0,0.0);
									break;
								case A_ALIGN_NY:
									upVector = Vector(0.0,-1.0,0.0);
									break;
								default:
									upVector = Vector(0.0,1.0,0.0);
									break;
							}
							lAimConstraint->SetWorldUpVector(VectorToKFbxVector4(upVector));
						}
						else lAimConstraint->SetWorldUpVector(VectorToKFbxVector4(upVector));
						
						BaseObject *upOp = tData->GetObjectLink(A_UP_VECTOR,doc);
						
						// set up vector type
						KFbxConstraintAim::EAimConstraintWoldUpType upType = KFbxConstraintAim::eAimAtSceneUp;
						if(tData->GetLong(A_POLE_AXIS) == A_POLE_OFF) upType = KFbxConstraintAim::eAimAtNone;
						else
						{
							if(upOp)
							{
								KFbxNode *upNode = GetNodeFromObject(upOp);
								if(upNode)
								{
									if(tData->GetLong(A_ALIGN_AXIS) > A_ALIGN_OFF)
										upType = KFbxConstraintAim::eAimAtObjectRotationUp;
									else
										upType = KFbxConstraintAim::eAimAtObjectUp;
									
									lAimConstraint->SetWorldUpObject(upNode);
								}
							}
							else upType = KFbxConstraintAim::eAimAtSceneUp;
						}
						lAimConstraint->SetWorldUpType(upType);
						KFbxConnectDst(lAimConstraint,pScene);
					}
				}
			}
			
			// parent constraint
			BaseTag *prTag = op->GetTag(ID_CDPRCONSTRAINTPLUGIN);
			if(prTag)
			{
				prCnt++;
				BaseContainer *tData = prTag->GetDataInstance();
				if(tData)
				{
					KFbxConstraintParent *lParentConstraint = KFbxConstraintParent::Create(pSdkManager,"Parent");
					if(lParentConstraint)
					{
						lParentConstraint->SetConstrainedObject(kNode);
						lParentConstraint->SetWeight(double(tData->GetReal(PR_STRENGTH)));
						
						BaseObject *trg = tData->GetObjectLink(PR_TARGET,doc);
						if(trg)
						{
							KFbxNode *trgNode = GetNodeFromObject(trg);
							if(trgNode) lParentConstraint->AddConstraintSource(trgNode);
							
							Vector offsetT = (MInv(trg->GetMg()) * op->GetMg().off) * exScale;
							if(handed == 1) offsetT.z *= -1;
							lParentConstraint->SetTranslationOffset(trgNode,VectorToKFbxVector4(offsetT));

							Vector offsetR = CDMatrixToHPB(MInv(trg->GetMg()) * op->GetMg());
							if(handed == 1) offsetR.z *= -1;
							lParentConstraint->SetRotationOffset(trgNode,VectorToKFbxVector4(Vector(Deg(offsetR.y),Deg(offsetR.x),Deg(offsetR.z))));
						}
						KFbxConnectDst(lParentConstraint,pScene);
					}
				}
			}
			
			// ik constraints
			BaseTag *ikTag = NULL;
			if(op->GetTag(ID_CDLIMBIKPLUGIN))
			{
				ikTag = op->GetTag(ID_CDLIMBIKPLUGIN);
				if(ikTag)
				{
					BaseContainer *tData = ikTag->GetDataInstance();
					if(tData)
					{
						KFbxNode *ikHandleNode = NULL;
						BaseObject *gl = tData->GetObjectLink(LMBIK_GOAL_LINK,doc);
						if(gl) ikHandleNode = GetNodeFromObject(gl);
						
						if(ikHandleNode)
						{
							ikCnt++;
							char *ikConstName = StringToChar(gl->GetName()+"_ncl1_"+CDLongToString(ikCnt));
							
							KFbxConstraintSingleChainIK *lIKConstraint = KFbxConstraintSingleChainIK::Create(pSdkManager,ikConstName);
							if(lIKConstraint)
							{
								lIKConstraint->SetFirstJointObject(kNode);
								lIKConstraint->SetWeight(100);
								
								lIKConstraint->SetSolverType(KFbxConstraintSingleChainIK::eRP_SOLVER);
								lIKConstraint->SetPoleVectorType(KFbxConstraintSingleChainIK::ePOLE_VECTOR_TYPE_VECTOR);//ePOLE_VECTOR_TYPE_OBJECT
								
								lIKConstraint->SetEffectorObject(ikHandleNode);
								
								KFbxNode *poleVectorNode = NULL;
								BaseObject *pl = tData->GetObjectLink(LMBIK_SOLVER_LINK,doc);
								if(pl)
								{
									poleVectorNode = GetNodeFromObject(pl);
									if(poleVectorNode)
									{
										Vector poleVector;
										lIKConstraint->AddPoleVectorObject(poleVectorNode);
										poleVector = (pl->GetMg().off - op->GetMg().off) * exScale;
										if(handed == 1) poleVector.z *= -1;
										lIKConstraint->SetPoleVector(VectorToKFbxVector4(poleVector));
									}
								}
								
								Real twist = tData->GetReal(LMBIK_TWIST);
								lIKConstraint->SetTwist(double(Deg(twist)));
								
								KFbxNode *endJointNode = NULL;
								BaseObject *endJnt = op->GetDown()->GetDown();
								if(endJnt)
								{
									endJointNode = GetNodeFromObject(endJnt);
									if(endJointNode) lIKConstraint->SetEndJointObject(endJointNode);
								}
								
								KFbxConnectDst(lIKConstraint,pScene);
							}
							CDFree(ikConstName);
						}
					}
				}
			}
			else if(op->GetTag(ID_CDFOOTIKPLUGIN))
			{
				ikTag = op->GetTag(ID_CDFOOTIKPLUGIN);
				if(ikTag)
				{
					BaseContainer *tData = ikTag->GetDataInstance();
					if(tData)
					{
						KFbxNode *ikHandleNodeA = NULL;
						BaseObject *glA = tData->GetObjectLink(FTIK_GOAL_A_LINK,doc);
						if(glA) ikHandleNodeA = GetNodeFromObject(glA);
						
						if(ikHandleNodeA)
						{
							ikCnt++;
							char *ikConstNameA = StringToChar(glA->GetName()+"_ncl1_"+CDLongToString(ikCnt));
							
							KFbxConstraintSingleChainIK *lIKConstraint = KFbxConstraintSingleChainIK::Create(pSdkManager,ikConstNameA);
							if(lIKConstraint)
							{
								lIKConstraint->SetFirstJointObject(kNode);
								lIKConstraint->SetWeight(100);
								
								lIKConstraint->SetEffectorObject(ikHandleNodeA);

								KFbxNode *secondJoint = NULL;
								BaseObject *ch = op->GetDown();
								if(ch)
								{
									secondJoint = GetNodeFromObject(ch);
									if(secondJoint) lIKConstraint->SetEndJointObject(secondJoint);
								}
								
								lIKConstraint->SetSolverType(KFbxConstraintSingleChainIK::eSC_SOLVER);
								lIKConstraint->SetPoleVectorType(KFbxConstraintSingleChainIK::ePOLE_VECTOR_TYPE_VECTOR);
								Vector poleV = Vector(1.0,0.0,0.0);
								if(handed == 1) poleV = Vector(0.0,0.0,1.0);
								lIKConstraint->SetPoleVector(VectorToKFbxVector4(poleV));
								
								lIKConstraint->SetActive(true);
								KFbxConnectDst(lIKConstraint,pScene);
								
								KFbxNode *ikHandleNodeB = NULL;
								BaseObject *glB = tData->GetObjectLink(FTIK_GOAL_B_LINK,doc);
								if(glB) ikHandleNodeB = GetNodeFromObject(glB);
									
								if(ikHandleNodeB)
								{
									ikCnt++;
									char *ikConstNameB = StringToChar(glB->GetName()+"_ncl1_"+CDLongToString(ikCnt));
									
									KFbxConstraintSingleChainIK *lIKConstraint = KFbxConstraintSingleChainIK::Create(pSdkManager,ikConstNameB);
									if(lIKConstraint)
									{
										lIKConstraint->SetFirstJointObject(secondJoint);
										lIKConstraint->SetWeight(100);
										
										lIKConstraint->SetEffectorObject(ikHandleNodeB);
										
										KFbxNode *thirdJoint = NULL;
										BaseObject *gch = ch->GetDown();
										if(gch)
										{
											thirdJoint = GetNodeFromObject(gch);
											if(thirdJoint) lIKConstraint->SetEndJointObject(thirdJoint);
										}
										
										lIKConstraint->SetSolverType(KFbxConstraintSingleChainIK::eSC_SOLVER);
										lIKConstraint->SetPoleVectorType(KFbxConstraintSingleChainIK::ePOLE_VECTOR_TYPE_VECTOR);
										lIKConstraint->SetPoleVector(VectorToKFbxVector4(poleV));
										
										lIKConstraint->SetActive(true);
										KFbxConnectDst(lIKConstraint,pScene);
									}
									CDFree(ikConstNameB);
								}
							}
							CDFree(ikConstNameA);
						}
					}
				}
			}
			else if(op->GetTag(ID_CDQUADLEGPLUGIN))
			{
				ikTag = op->GetTag(ID_CDQUADLEGPLUGIN);
				if(ikTag)
				{
					BaseContainer *tData = ikTag->GetDataInstance();
					if(tData)
					{
						KFbxNode *ikHandleNode = NULL;
						BaseObject *gl = tData->GetObjectLink(QLIK_GOAL_LINK,doc);
						if(gl) ikHandleNode = GetNodeFromObject(gl);
						
						if(ikHandleNode)
						{
							ikCnt++;
							char *ikConstName = StringToChar(gl->GetName()+"_ncl1_"+CDLongToString(ikCnt));
							
							KFbxConstraintSingleChainIK *lIKConstraint = KFbxConstraintSingleChainIK::Create(pSdkManager,ikConstName);
							if(lIKConstraint)
							{
								lIKConstraint->SetFirstJointObject(kNode);
								
								lIKConstraint->SetSolverType(KFbxConstraintSingleChainIK::eRP_SOLVER);
								lIKConstraint->SetPoleVectorType(KFbxConstraintSingleChainIK::ePOLE_VECTOR_TYPE_VECTOR);//ePOLE_VECTOR_TYPE_OBJECT
								lIKConstraint->SetWeight(100);
								
								lIKConstraint->SetEffectorObject(ikHandleNode);
								
								KFbxNode *poleVectorNode = NULL;
								BaseObject *pl = tData->GetObjectLink(QLIK_SOLVER_LINK,doc);
								if(pl)
								{
									poleVectorNode = GetNodeFromObject(pl);
									if(poleVectorNode)
									{
										lIKConstraint->AddPoleVectorObject(poleVectorNode);
										Vector poleVector = VNorm(pl->GetMg().off - op->GetMg().off);
										if(handed == 1) poleVector.z *= -1;
										lIKConstraint->SetPoleVector(VectorToKFbxVector4(poleVector));
									}
								}
								
								Real twist = tData->GetReal(QLIK_TWIST);
								lIKConstraint->SetTwist(double(Deg(twist)));
								
								KFbxNode *endJointNode = NULL;
								BaseObject *endJnt = op->GetDown()->GetDown()->GetDown();
								if(endJnt)
								{
									endJointNode = GetNodeFromObject(endJnt);
									if(endJointNode) lIKConstraint->SetEndJointObject(endJointNode);
								}
								
								lIKConstraint->SetActive(true);
								KFbxConnectDst(lIKConstraint,pScene);
							}
							CDFree(ikConstName);
						}
					}
				}
			}
			else if(op->GetTag(ID_CDIKHANDLEPLUGIN))
			{
				ikTag = op->GetTag(ID_CDIKHANDLEPLUGIN);
				if(ikTag)
				{
					BaseContainer *tData = ikTag->GetDataInstance();
					if(tData)
					{
						KFbxNode *ikHandleNode = NULL;
						BaseObject *gl = tData->GetObjectLink(HIK_GOAL_LINK,doc);
						if(gl) ikHandleNode = GetNodeFromObject(gl);
						
						if(ikHandleNode)
						{
							ikCnt++;
							char *ikConstName = StringToChar(gl->GetName()+"_ncl1_"+CDLongToString(ikCnt));
							
							KFbxConstraintSingleChainIK *lIKConstraint = KFbxConstraintSingleChainIK::Create(pSdkManager,ikConstName);
							if(lIKConstraint)
							{
								lIKConstraint->SetFirstJointObject(kNode);
								
								switch(tData->GetLong(HIK_SOLVER))
								{
									case HIKRP:
										lIKConstraint->SetSolverType(KFbxConstraintSingleChainIK::eRP_SOLVER);
										break;
									case HIKSC:
										lIKConstraint->SetSolverType(KFbxConstraintSingleChainIK::eSC_SOLVER);
										break;
									default:
										lIKConstraint->SetSolverType(KFbxConstraintSingleChainIK::eSC_SOLVER);
										break;
								}
								lIKConstraint->SetWeight(100);
								
								lIKConstraint->SetEffectorObject(ikHandleNode);
								
								KFbxNode *poleVectorNode = NULL;
								BaseObject *pl = tData->GetObjectLink(HIK_POLE_LINK,doc);
								if(pl && tData->GetLong(HIK_SOLVER) == HIKRP)
								{
									poleVectorNode = GetNodeFromObject(pl);
									if(poleVectorNode)
									{
										lIKConstraint->SetPoleVectorType(KFbxConstraintSingleChainIK::ePOLE_VECTOR_TYPE_VECTOR);//ePOLE_VECTOR_TYPE_OBJECT

										lIKConstraint->AddPoleVectorObject(poleVectorNode);
										Vector poleVector = VNorm(pl->GetMg().off - op->GetMg().off);
										if(handed == 1) poleVector.z *= -1;
										lIKConstraint->SetPoleVector(VectorToKFbxVector4(poleVector));
									}
								}
								else
								{
									lIKConstraint->SetPoleVectorType(KFbxConstraintSingleChainIK::ePOLE_VECTOR_TYPE_VECTOR);
									Vector poleVector = tData->GetVector(HIKSC_POLE_VECTOR);
									if(handed == 1) poleVector.z *= -1;
									lIKConstraint->SetPoleVector(VectorToKFbxVector4(poleVector));
								}
								
								Real twist = tData->GetReal(HIK_TWIST);
								lIKConstraint->SetTwist(double(Deg(twist)));
								
								KFbxNode *endJointNode = NULL;
								BaseObject *endJnt = op;
								LONG i, jCnt = tData->GetLong(HIK_BONES_IN_GROUP);
								for(i=0; i<jCnt; i++)
								{
									if(endJnt) endJnt = endJnt->GetDown();
								}
								if(endJnt)
								{
									endJointNode = GetNodeFromObject(endJnt);
									if(endJointNode) lIKConstraint->SetEndJointObject(endJointNode);
								}
								
								lIKConstraint->SetActive(true);
								KFbxConnectDst(lIKConstraint,pScene);
							}
							CDFree(ikConstName);
						}
					}
				}
			}
		}
	}
}

void CDExportFBXCommand::FindConstrainedJointTargets(BaseDocument *doc, BaseObject *op)
{
	while(op)
	{
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			BaseTag *rTag = op->GetTag(ID_CDRCONSTRAINTPLUGIN);
			if(rTag)
			{
				BaseContainer *tData = rTag->GetDataInstance();
				if(tData)
				{
					LONG i, cnt = tData->GetLong(R_TARGET_COUNT);
					for(i=0; i<cnt; i++)
					{
						BaseObject *trg = tData->GetObjectLink(R_TARGET+i,doc);
						if(trg && trg->GetType() != ID_CDJOINTOBJECT && jntTarget->Find(trg) == NOTOK) jntTarget->Append(trg);
					}
				}
			}
			
			BaseTag *psrTag = op->GetTag(ID_CDPSRCONSTRAINTPLUGIN);
			if(psrTag)
			{
				BaseContainer *tData = psrTag->GetDataInstance();
				if(tData && tData->GetBool(PSR_USE_R))
				{
					LONG i, cnt = tData->GetLong(PSR_TARGET_COUNT);
					for(i=0; i<cnt; i++)
					{
						BaseObject *trg = tData->GetObjectLink(PSR_TARGET+i,doc);
						if(trg && trg->GetType() != ID_CDJOINTOBJECT && jntTarget->Find(trg) == NOTOK) jntTarget->Append(trg);
					}
				}
			}
			
			BaseTag *aTag = op->GetTag(ID_CDACONSTRAINTPLUGIN);
			if(aTag)
			{
				BaseContainer *tData = aTag->GetDataInstance();
				if(tData)
				{
					LONG i, cnt = tData->GetLong(A_TARGET_COUNT);
					for(i=0; i<cnt; i++)
					{
						BaseObject *upv = tData->GetObjectLink(A_UP_VECTOR+i,doc);
						if(upv && upv->GetType() != ID_CDJOINTOBJECT && jntTarget->Find(upv) == NOTOK) jntTarget->Append(upv);
					}
				}
			}
			
			BaseTag *fTag = op->GetTag(ID_CDFOOTIKPLUGIN);
			if(fTag)
			{
				BaseContainer *tData = fTag->GetDataInstance();
				if(tData)
				{
					BaseObject *trgA = tData->GetObjectLink(FTIK_GOAL_A_LINK,doc);
					if(trgA && jntTarget->Find(trgA) == NOTOK) jntTarget->Append(trgA);
					
					BaseObject *trgB = tData->GetObjectLink(FTIK_GOAL_B_LINK,doc);
					if(trgB && jntTarget->Find(trgB) == NOTOK) jntTarget->Append(trgB);
				}
			}
			BaseTag *ikTag = op->GetTag(ID_CDIKHANDLEPLUGIN);
			if(ikTag)
			{
				BaseContainer *tData = ikTag->GetDataInstance();
				if(tData && tData->GetLong(HIK_SOLVER) == HIKSC)
				{
					BaseObject *trg = tData->GetObjectLink(HIK_GOAL_LINK,doc);
					if(trg && jntTarget->Find(trg) == NOTOK) jntTarget->Append(trg);
				}
			}
		}
		else
		{
			BaseTag *rTag = op->GetTag(ID_CDRCONSTRAINTPLUGIN);
			if(rTag)
			{
				BaseContainer *tData = rTag->GetDataInstance();
				if(tData)
				{
					LONG i, cnt = tData->GetLong(R_TARGET_COUNT);
					for(i=0; i<cnt; i++)
					{
						BaseObject *trg = tData->GetObjectLink(R_TARGET+i,doc);
						if(trg && trg->GetType() == ID_CDJOINTOBJECT && jntTarget->Find(op) == NOTOK) jntTarget->Append(op);
					}
				}
			}
			
			BaseTag *psrTag = op->GetTag(ID_CDPSRCONSTRAINTPLUGIN);
			if(psrTag)
			{
				BaseContainer *tData = psrTag->GetDataInstance();
				if(tData && tData->GetBool(PSR_USE_R))
				{
					LONG i, cnt = tData->GetLong(PSR_TARGET_COUNT);
					for(i=0; i<cnt; i++)
					{
						BaseObject *trg = tData->GetObjectLink(PSR_TARGET+i,doc);
						if(trg && trg->GetType() == ID_CDJOINTOBJECT && jntTarget->Find(op) == NOTOK) jntTarget->Append(op);
					}
				}
			}
			
			BaseTag *aTag = op->GetTag(ID_CDACONSTRAINTPLUGIN);
			if(aTag)
			{
				BaseContainer *tData = aTag->GetDataInstance();
				if(tData)
				{
					LONG i, cnt = tData->GetLong(A_TARGET_COUNT);
					for(i=0; i<cnt; i++)
					{
						BaseObject *upv = tData->GetObjectLink(A_UP_VECTOR+i,doc);
						if(upv && upv->GetType() == ID_CDJOINTOBJECT && jntTarget->Find(op) == NOTOK) jntTarget->Append(op);
					}
				}
			}
		}
		
		FindConstrainedJointTargets(doc, op->GetDown());
		
		op = op->GetNext();
	}
}

void CDExportFBXCommand::CreateScene(BaseDocument *doc, KFbxSdkManager *pSdkManager, KFbxScene *pScene)
{
    // create scene info
	char *dName = StringToChar(doc->GetName());
    KFbxDocumentInfo *sceneInfo = KFbxDocumentInfo::Create(pSdkManager,"SceneInfo");
    sceneInfo->mTitle = dName;
    sceneInfo->mAuthor = "CD FBX Import/Export plugin for Cinema 4D";
	char *revName = StringToChar("Version "+CDRealToString(GetCDPluginVersion(ID_CDFBXPLUGIN),-1,3));
    sceneInfo->mRevision = revName;
	
	sceneInfo->LastSaved_ApplicationVendor.Set(fbxString("CD Plugins"));
	sceneInfo->LastSaved_ApplicationName.Set(fbxString("CD FBX Import/Export"));
	char *cdVersion = StringToChar("Version "+CDRealToString(GetCDPluginVersion(ID_CDFBXPLUGIN),-1,3));
	sceneInfo->LastSaved_ApplicationVersion.Set(fbxString(cdVersion));
	
    pScene->SetSceneInfo(sceneInfo);
	
	if(handed == 0)//XAxis  YAxis  ZAxis  ParityOdd  ParityEven  RightHanded  LeftHanded
	{
		KFbxAxisSystem sys = KFbxAxisSystem(KFbxAxisSystem::YAxis, KFbxAxisSystem::ParityOdd, KFbxAxisSystem::LeftHanded);
		pScene->GetGlobalSettings().SetAxisSystem(sys);
	}
	else
	{
		KFbxAxisSystem sys = KFbxAxisSystem(KFbxAxisSystem::YAxis, KFbxAxisSystem::ParityOdd, KFbxAxisSystem::RightHanded);
		pScene->GetGlobalSettings().SetAxisSystem(sys);
	}
	
	if(exportPLA && DocumentHasPLA(doc->GetFirstObject()) || bakeMeshToPLA  || bakeClothToPLA || bakeCDSkinToPLA)
	{
		// create directory path for vertex cache
		lFBXAbsolutePath = KFbxFullPath(pFilename);

	    // Create a cache directory with the same name as the fbx file
		lFPCAbsoluteDirectory  = KFbxExtractDirectory(lFBXAbsolutePath);
		lFPCAbsoluteDirectory += "/";
		lFPCAbsoluteDirectory += KFbxExtractFileName(pFilename, false);
		lFPCAbsoluteDirectory += "_fpc";
		
		// Make this path the shortest possible
		lFPCAbsoluteDirectory = KFbxCleanPath(lFPCAbsoluteDirectory);
	}
	
    // Build the node tree.
    lRootNode = pScene->GetRootNode();
	
	objectList.Init();
	shapeList.Init();
	jointList.Init();
	ResolveMaterialNames(doc);
	
	jntTarget = AtomArray::Alloc();
	if(exportConstraint && handed == 1) FindConstrainedJointTargets(doc, doc->GetFirstObject());
	
	if(exportMorph) BuildShapeNodeList(doc->GetFirstObject());
	
	if(preserveHierarchy) BuildContent(doc,doc->GetFirstObject(),pSdkManager,lRootNode);
	else
	{
		if(exportSpline) CreateSplines(doc,doc->GetFirstObject(),pSdkManager,lRootNode);
		if(exportJoint) CreateSkeletons(doc,doc->GetFirstObject(),pSdkManager,lRootNode);
		if(exportMesh) CreateMeshes(doc,doc->GetFirstObject(),pSdkManager,lRootNode);
		if(exportLight) CreateLights(doc,doc->GetFirstObject(),pSdkManager,lRootNode);
		if(exportCamera) CreateCameras(doc,doc->GetFirstObject(),pSdkManager,lRootNode);
	}
	
	if(jointList.Size() > 0) StoreBindPose(pSdkManager,pScene);
	
	if(exportConstraint) CreateConstraints(doc,pSdkManager,pScene);
	
	if(exportAnimation) AnimateScene(doc,pSdkManager,pScene);
	
	CDFree(dName);
	CDFree(revName);
	CDFree(cdVersion);
	materialList.Free();
	
	if(jntTarget) AtomArray::Free(jntTarget);
}

Bool CDExportFBXCommand::SaveScene(KFbxSdkManager *pSdkManager, KFbxDocument *pScene, int pFileFormat, bool pEmbedMedia)
{
    Bool lStatus = true;
	
    // Create an exporter.
    KFbxExporter *lExporter = KFbxExporter::Create(pSdkManager, ""); if(!lExporter) return false;
	
    if( pFileFormat < 0 || pFileFormat >= pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount() )
    {
        // Write in fall back format if pEmbedMedia is true
        pFileFormat = pSdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();
		
        if(!pEmbedMedia)
        {
            //Try to export in ASCII if possible
            int lFormatIndex, lFormatCount = pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount();
			
            for (lFormatIndex=0; lFormatIndex<lFormatCount; lFormatIndex++)
            {
                if(pSdkManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
                {
                    KString lDesc =pSdkManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
                    char *lASCII = "ascii";
                    if(lDesc.Find(lASCII)>=0)
                    {
                        pFileFormat = lFormatIndex;
                        break;
                    }
                }
            }
        }
    }
	
    // Set the export states. By default, the export states are always set to 
    // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
    // shows how to change these states.
    IOSREF.SetBoolProp(EXP_FBX_CONSTRAINT,true);
    IOSREF.SetBoolProp(EXP_FBX_CHARACTER,true);
    IOSREF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
	
    // Initialize the exporter by providing a filename.
#ifdef __PC
	if(lExporter->Initialize(pFilename) == false)
	{
		exportError = CharToString(lExporter->GetLastErrorString());
		return false;
	}
#else
	if(lExporter->Initialize(pFilename, pFileFormat) == false)
	{
		exportError = CharToString(lExporter->GetLastErrorString());
		return false;
	}
#endif
	
    // Export the scene.
    lStatus = lExporter->Export(pScene); 
	
    // Destroy the exporter.
    lExporter->Destroy();
    return lStatus;
}

void CDExportFBXCommand::DestroySdkObjects(void)
{
	if(pScene) pScene->Destroy();
	pScene = NULL;
	
	if(pSdkManager) pSdkManager->Destroy();
	pSdkManager = NULL;
}

void CDExportFBXCommand::CreateFbmDirectory(Filename fName)
{
	Filename directory = fName.GetDirectory();
	Filename nameOnly = fName.GetFileString();
	nameOnly.ClearSuffix();
	
	txPath = directory.GetString()+"/"+nameOnly.GetFileString()+".fbm/";
	directory.SetString(txPath);
	GeFCreateDir(directory);
}

void CDExportFBXCommand::InitTakeListData(BaseDocument *doc)
{
	takeList.Init();
	
	BaseContainer *exTakeData = doc->GetDataInstance()->GetContainerInstance(ID_CDFBXEXPORTER);
	if(!exTakeData)
	{
		// no previous take list stored in document, create default take 1
		LONG fps = doc->GetFps();
		LONG startFrame = (LONG)doc->GetUsedMinTime(NULL).GetFrame(Real(fps));
		LONG endFrame = (LONG)doc->GetUsedMaxTime(NULL).GetFrame(Real(fps));
		if(startFrame > endFrame)
		{
			startFrame = 0;
			endFrame = 0;
		}
		frameCount = endFrame - startFrame;
		
		TakeNode tnode = TakeNode("Take.1",startFrame,endFrame);
		takeList.Append(tnode);
	}
	else
	{
		// load take list stored in document
		LONG i, tCnt = exTakeData->GetLong(IDC_EXPORT_TAKE_COUNT);
		for(i=0; i<tCnt; i++)
		{
			String tName = exTakeData->GetString(IDC_EXPORT_TAKE_NAME+i);
			LONG startFrame = exTakeData->GetLong(IDC_EXPORT_TAKE_START+i);
			LONG endFrame = exTakeData->GetLong(IDC_EXPORT_TAKE_END+i);
			if(startFrame > endFrame)
			{
				startFrame = 0;
				endFrame = 0;
			}
			frameCount = endFrame - startFrame;
			
			TakeNode tnode = TakeNode(tName,startFrame,endFrame);
			takeList.Append(tnode);
		}
	}
}

Bool CDExportFBXCommand::SetExportOptions(void)
{
	BaseContainer *bc = GetWorldPluginData(ID_CDFBXEXPORTER);
	if(!bc) return false;
	
	exScale = bc->GetReal(IDC_EXPORT_SCALE);
	handed = bc->GetLong(IDC_HANDED);
	
	exportJoint = bc->GetBool(IDC_EXPORT_JOINT);
	exportMesh = bc->GetBool(IDC_EXPORT_MESH);
	exportMorph = bc->GetBool(IDC_EXPORT_MORPHS);
	exportCamera = bc->GetBool(IDC_EXPORT_CAMERA);
	exportLight = bc->GetBool(IDC_EXPORT_LIGHT);
	exportSpline = bc->GetBool(IDS_EXPORT_SPLINE);
	
	exportSmoothing = bc->GetBool(IDS_EXPORT_SMOOTHING);
	exportUserData = bc->GetBool(IDS_EXPORT_USER_DATA);
	exportConstraint = bc->GetBool(IDS_EXPORT_CONSTRAINTS);
	
	exportAnimation = bc->GetBool(IDC_EXPORT_ANIMATION);
	exportCurves = bc->GetBool(IDS_EXPORT_CURVES);
	bakeJoints = bc->GetBool(IDC_BAKE_JOINT_ANIMATION);
	bakePosition = bc->GetBool(IDC_BAKE_POSITION);
	bakeScale = bc->GetBool(IDC_BAKE_SCALE);
	bakeRotation = bc->GetBool(IDC_BAKE_ROTATION);
	
	exportPLA = bc->GetBool(IDC_EXPORT_PLA);
	bakeCDSkinToPLA = bc->GetBool(IDC_BAKE_CDSKIN_TO_PLA);
	bakeMeshToPLA = bc->GetBool(IDC_BAKE_MESH_TO_PLA);
	bakeClothToPLA = bc->GetBool(IDC_BAKE_CLOTH_TO_PLA);
	
	embedTextures = bc->GetBool(IDC_EMBED_TEXTURES);
	preserveHierarchy = bc->GetBool(IDC_PRESERVE_HIERARCHY);
	onlySelected = bc->GetBool(IDC_ONLY_SELECTED);
	exportAlphanumeric = bc->GetBool(IDS_ALPHA_NUMERIC_ONLY);
	
	return true;
}

Bool CDExportFBXCommand::ExportFBX(BaseDocument *doc, Filename fName)
{
	pSdkManager = NULL;
	pScene = NULL;
	lRootNode = NULL;
	pFilename = NULL;
	
	exportError = "";
	
	fName.SetSuffix("fbx");
	
	bool tEmbed = true;
	if(!embedTextures)
	{
		tEmbed = false;
		CreateFbmDirectory(fName);
	}
	
	pFilename = ConvertPathname(&fName);
	
	fPath = CharToString(pFilename);
	
	// initialize fbx sdk
    pSdkManager = KFbxSdkManager::Create();
	if(!pSdkManager)
	{
		// free memory
		CDFree(pFilename);
		takeList.Free();
		StatusClear();
		return false;
	}
	
    pScene = KFbxScene::Create(pSdkManager,"");
	if(!pScene)
	{
		// free memory
		CDFree(pFilename);
		takeList.Free();
		DestroySdkObjects();
		StatusClear();
		return false;
	}
	
	// set defaults for missing texture test
	txEmbedFailed = false;
	txCopyFailed = false;
	missingTextures = "";
	
	CreateScene(doc,pSdkManager,pScene);
	
	if(!SaveScene(pSdkManager,pScene,-1,tEmbed))
	{
		// free memory
		objectList.Free();
		jointList.Free();
		shapeList.Free();
		takeList.Free();
		
		DestroySdkObjects();
		
		CDFree(pFilename);
		
		if(exportError != "") MessageDialog(exportError);
		StatusClear();
		return false;
	}
	
	if(txEmbedFailed) MessageDialog(GeLoadString(MD_EMBED_FAILED)+missingTextures);
	if(txCopyFailed) MessageDialog(GeLoadString(MD_MISSING_TEXTURES)+missingTextures);
	
	// free memory
	objectList.Free();
	jointList.Free();
	shapeList.Free();
	takeList.Free();
	
	DestroySdkObjects();
	
	CDFree(pFilename);
	
	StatusClear();
		
	return true;
}

Bool CDExportFBXCommand::ExecuteFileExport(String filename)
{
	Filename inFileName, outFileName;
	const String inPath = GetBatchInputPath() + filename;
	inFileName.SetString(inPath);
	if(!inFileName.CheckSuffix("C4D")) return false;
	
	BaseDocument *exDoc = LoadDocument(inFileName,SCENEFILTER_OBJECTS|SCENEFILTER_MATERIALS,NULL);
	if(!exDoc) return false;
	
	const String outPath = GetBatchOutputPath() + filename;
	outFileName.SetString(outPath);
	
	SetExportOptions();
	if(exportAnimation) InitTakeListData(exDoc);

	return ExportFBX(exDoc, outFileName);
}

Bool CDExportFBXCommand::Execute(BaseDocument *doc)
{
	if(!doc->GetFirstObject())
	{
		MessageDialog(GeLoadString(MD_EMPTY_SCENE));
		return false;
	}
	jRotM = CDHPBToMatrix(Vector(pi05,0,0));

	if(!dlg.Open()) return false;
	
	if(!SetExportOptions()) return false;
	
	frameCount = 0;
	if(exportAnimation)
	{
		InitTakeListData(doc);
		
		if(!tkMngr.Open())
		{
			takeList.Free();
			return false;
		}
		
		BaseContainer *docData = doc->GetDataInstance();
		TakeNode *tlist = takeList.Array();
		BaseContainer dbc;
		for(int i=0; i<takeList.Size(); i++)
		{
			dbc.SetString(IDC_EXPORT_TAKE_NAME+i,tlist[i].name);
			dbc.SetLong(IDC_EXPORT_TAKE_START+i,tlist[i].start);
			dbc.SetLong(IDC_EXPORT_TAKE_END+i,tlist[i].end);
		}
		dbc.SetLong(IDC_EXPORT_TAKE_COUNT,takeList.Size());
		docData->SetContainer(ID_CDFBXEXPORTER,dbc);
	}
	
	Filename fName;
	if(onlySelected)
	{
		BaseObject *op = doc->GetActiveObject();
		if(op) fName = Filename(op->GetName());
		else fName = doc->GetDocumentName();
	}
	else fName = doc->GetDocumentName();
	
#if API_VERSION < 12000
	String title = GeLoadString(IDS_CDFBXEXPRT);
	if(!fName.FileSelect(FSTYPE_SCENES,GE_SAVE,&title))
	{
		// free memory
		takeList.Free();
		return false;
	}
#else
	if(!fName.FileSelect(FILESELECTTYPE_SCENES,FILESELECT_SAVE,GeLoadString(IDS_CDFBXEXPRT),"fbx"))
	{
		// free memory
		takeList.Free();
		return false;
	}
#endif
	
	Bool ret = ExportFBX(doc, fName);
	
	EventAdd(EVENT_FORCEREDRAW);
	return ret;
}


class CDExportFBXCommandR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		MessageDialog(GeLoadString(MD_NO_EXPORT_IN_DEMO));
		return true;
	}
};

Bool RegisterCDExportFBXCommand(void)
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
	
	// decide by name if the plugin shall be registered - just for user convenience  
	String name = GeLoadString(IDS_CDFBXEXPRT); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDFBXEXPORTER,name,PLUGINFLAG_HIDE,"CDFBXExp.tif","CD Export FBX",CDDataAllocator(CDExportFBXCommandR));
	else return CDRegisterCommandPlugin(ID_CDFBXEXPORTER,name,0,"CDFBXExp.tif","CD Export FBX",CDDataAllocator(CDExportFBXCommand));
}


// pointer to CDExportFBXCommand
CDExportFBXCommand *cdfbxEx = NULL;

void CDStoreExportCommandPointer(void)
{
	BasePlugin *cmdPtr = CDFindPlugin(ID_CDFBXEXPORTER, CD_COMMAND_PLUGIN);
	if(cmdPtr)
	{
		COMMANDPLUGIN *cp = static_cast<COMMANDPLUGIN*>(cmdPtr->GetPluginStructure());
		if(cp) cdfbxEx = static_cast<CDExportFBXCommand*>(cp->adr);
	}
}

// Coffee export interface functions
void CDFbxExportFile(Coffee* Co, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	if(cdfbxEx && argc == 1)
	{
		String fname = sp[argc-1].GetString();
		res = cdfbxEx->ExecuteFileExport(fname);
	}
	
	GeCoffeeGeData2Value(Co, res, &sp[argc]);
	sp += argc;
}



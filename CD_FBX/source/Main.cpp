//	Cactus Dan's FBX Import/Export plugin
//	Copyright 2011 by Cactus Dan Libisch


#include "c4d.h"
#include "c4d_symbols.h"

#include "CDFBX.h"
#include "CommonFbx.h"

// Starts the plugin registration

// forward declarations
// menu command plugins
Bool RegisterAboutCDFBX(void);
Bool RegisterCDImportFBXCommand(void);
Bool RegisterCDExportFBXCommand(void);
Bool RegisterCDTransAxisCommand(void);
Bool RegisterCDConvertToCDSkin(void);
Bool RegisterCDConvertFromCDSkin(void);
Bool RegisterCDConvertToCDJoints(void);
Bool RegisterCDConvertFromCDJoints(void);

// tag / expression plugins
Bool RegisterCDNormalsDisplay(void);
Bool RegisterCDRetargetTag(void);
Bool RegisterCDKeepPositionPlaner(void);

Bool RegisterCDSkinPlugin(void);
Bool RegisterCDSkinRefPlugin(void);
Bool RegisterCDClusterTagPlugin(void);
Bool RegisterCDMorphRefPlugin(void);
Bool RegisterCDMorphTagPlugin(void);
Bool RegisterCDIKHandlePlugin(void);
Bool RegisterCDAConstraintPlugin(void);
Bool RegisterCDPConstraintPlugin(void);
Bool RegisterCDPRConstraintPlugin(void);
Bool RegisterCDRConstraintPlugin(void);
Bool RegisterCDSConstraintPlugin(void);

// object plugins
Bool RegisterCDJoint(void);

// message plugins
Bool RegisterCDJSMessagePlugin(void);
Bool RegisterCDMorphMessagePlugin(void);

// scene hook plugins
Bool RegisterCDJSSceneHook(void);

// Selection logger
Bool RegisterSelectionLog(void);

// library functions
Bool RegisterCDJSFunctionLib(void);
Bool RegisterCDMorphFunctionLib(void);

static AtomArray *cdsrTagList = NULL;
static AtomArray *cdskTagList = NULL;
static AtomArray *cdJointList = NULL;
static AtomArray *cdscTagList = NULL;
static AtomArray *cdmTagList = NULL;
static AtomArray *cdmrTagList = NULL;

Vector colorH, colorCH;

Bool clusDialogOpen = false;
Real clusDialogRad;
Real clusDialogMax;
Real clusDialogFalloff;

Bool cdsymPlug = true;

void CDStoreExportCommandPointer(void);
void CDStoreImportCommandPointer(void);

CDFbxBatchPaths *bPaths;

static Real version = 1.024;

Bool PluginStart(void)
{
	String pName = "CD FBX";
	//if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDFBXPLUGIN,bc,false);
	
	Bool reg = RegisterCDFBX();
	
	// Register/About
	if(!RegisterAboutCDFBX()) return false;
	
	// Allocate the lists
	cdsrTagList = AtomArray::Alloc();
	cdskTagList = AtomArray::Alloc();
	cdJointList = AtomArray::Alloc();
	cdscTagList = AtomArray::Alloc();
	cdmTagList = AtomArray::Alloc();
	cdmrTagList = AtomArray::Alloc();
	
	// Selection logger
	if(RegisterSelectionLog())
	{
		InitSelectionLog();
	}
	
	// command plugins
	if(!RegisterCDImportFBXCommand()) return false;
	if(!RegisterCDExportFBXCommand()) return false;
	
	// tag plugins
	if(!RegisterCDNormalsDisplay()) return false;
	if(!RegisterCDRetargetTag()) return false;
	if(!RegisterCDKeepPositionPlaner()) return false;

	String license = "";
	if(!reg) license = " - Runtime";
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) license = " - Demo";
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) license = " - Demo";
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) license = " - Runtime";
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) license = " - Demo";
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) license = " - Runtime";
#endif
	
	// duplicate plugins
	if(reg)
	{
		// message plugins
		if(!RegisterCDJSMessagePlugin()) return false;
		if(!RegisterCDMorphMessagePlugin()) return false;
		
		// scene hook plugins
		if(!RegisterCDJSSceneHook()) return false;
		
		// command plugins
		if(!RegisterCDTransAxisCommand()) return false;
		if(!RegisterCDConvertToCDSkin()) return false;
		if(!RegisterCDConvertFromCDSkin()) return false;
		if(!RegisterCDConvertToCDJoints()) return false;
		if(!RegisterCDConvertFromCDJoints()) return false;
		
		// object plugins
		if(!RegisterCDJoint()) return false;
		
		// tag plugins
		if(!RegisterCDSkinPlugin()) return false;
		if(!RegisterCDSkinRefPlugin()) return false;
		if(!RegisterCDClusterTagPlugin()) return false;
		if(!RegisterCDMorphRefPlugin()) return false;
		if(!RegisterCDMorphTagPlugin()) return false;
		if(!RegisterCDIKHandlePlugin()) return false;
		if(!RegisterCDAConstraintPlugin()) return false;
		if(!RegisterCDPConstraintPlugin()) return false;
		if(!RegisterCDPRConstraintPlugin()) return false;
		if(!RegisterCDRConstraintPlugin()) return false;
		if(!RegisterCDSConstraintPlugin()) return false;
		
		// store pointers to import and export commands
		CDStoreExportCommandPointer();
		CDStoreImportCommandPointer();
		
		bPaths = new CDFbxBatchPaths();
		
		// coffee extension functions
		Coffee *cofM = GetCoffeeMaster();
		if(cofM)
		{
			if(CDFbxAddSymbols(cofM))
			{
				cofM->AddGlobalFunction("CDFbxClearExportData",CDFbxClearExportData);
				cofM->AddGlobalFunction("CDFbxSetExportData",CDFbxSetExportData);
				cofM->AddGlobalFunction("CDFbxSetOutputPath",CDFbxSetOutputPath);
				cofM->AddGlobalFunction("CDFbxExportFile",CDFbxExportFile);
				
				cofM->AddGlobalFunction("CDFbxClearImportData",CDFbxClearImportData);
				cofM->AddGlobalFunction("CDFbxSetImportData",CDFbxSetImportData);
				cofM->AddGlobalFunction("CDFbxSetInputPath",CDFbxSetInputPath);
				cofM->AddGlobalFunction("CDFbxImportFile",CDFbxImportFile);
			}
		}
		
		// library functions
		if(!CDFindPlugin(1019259,CD_COMMAND_PLUGIN))
		{
			if(!RegisterCDJSFunctionLib()) return false;
		}
		
		if(!CDFindPlugin(1017238,CD_COMMAND_PLUGIN))
		{
			if(!RegisterCDMorphFunctionLib()) return false;
		}
		
		GePrint(pName+" v"+CDRealToString(version)+license);
	}
	
	return true;
}

void PluginEnd(void)
{
	if(cdsrTagList) AtomArray::Free(cdsrTagList);
	if(cdskTagList) AtomArray::Free(cdskTagList);
	if(cdJointList) AtomArray::Free(cdJointList);
	if(cdscTagList) AtomArray::Free(cdscTagList);
	if(cdmTagList) AtomArray::Free(cdmTagList);
	if(cdmrTagList) AtomArray::Free(cdmrTagList);
	
	if(bPaths) delete bPaths;

	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	//use the following lines to set a plugin priority
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if(!resource.Init())
			{
				// don't start plugin without resource
				GePrint("CD FBX failed to load resources");
				return false; 
			}
			return true;
			
		case C4DMSG_PRIORITY: 
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS-1);
			return true;
	}
	
	if(!CDFindPlugin(ID_ABOUTCDJOINTSKIN,CD_COMMAND_PLUGIN))
	{
		CDJSData *scd, *skd, *srd, *jd; //
		switch (id)
		{
			case ID_CDCLUSTERTAGPLUGIN:
				scd = (CDJSData *)data;
				scd->list = cdscTagList;
				return true;
				
			case ID_CDSKINPLUGIN:
				skd = (CDJSData *)data;
				skd->list = cdskTagList;
				return true;
				
			case ID_CDSKINREFPLUGIN:
				srd = (CDJSData *)data;
				srd->list = cdsrTagList;
				return true;
				
			case ID_CDJOINTSANDSKIN:
				colorH = CDGetActiveViewColor();
				colorCH = CDGetActiveChildViewColor();
				return true;
				
			case ID_CDJOINTOBJECT:
				jd = (CDJSData *)data;
				jd->list = cdJointList;
				return true;
		}
	}
	
	if(!CDFindPlugin(ID_ABOUTCDMORPH,CD_COMMAND_PLUGIN))
	{
		CDMData *mrd, *md;
		switch (id)
		{
			case ID_CDMORPHREFPLUGIN:
				mrd = (CDMData *)data;
				mrd->list = cdmrTagList;
				return true;
				
			case ID_CDMORPHTAGPLUGIN:
				md = (CDMData *)data;
				md->list = cdmTagList;
				return true;
		}
	}
	
	return false;
}

// user area
void CDFbxAboutUserArea::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDFbxAbout.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}

void CDFbxOptionsUA::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDFbxOptions.tif"));
	
	OffScreenOn();
	DrawSetPen(Vector(0.1, 0.1, 0.1)); //grey background
	DrawRectangle(0,0,sx,24);
	
	// draw the bitmap
	DrawBitmap(bm,0,0,256,24,0,0,256,24,0);
}

void CDFbxOptionsUA::Sized(LONG w, LONG h) 
{
	//store the width for drawing 
	sx = w;
}

Bool CDFbxOptionsUA::GetMinSize(LONG& w, LONG& h) 
{
	//the dimensions of the titlebar bitmap
	w = 256;
	h = 24;
	return true;
}


// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 12;
	*a = 169;
	*b = 211;
	*c = 98;
}

char* ConvertPathname(Filename *fName)
{
	String fnStr = fName->GetString();
	
	switch(CDGeGetCurrentOS())
	{
		case CD_MAC:
		{
		#if API_VERSION < 9800
			LONG pos;
			while(fnStr.FindFirst(":",&pos))
			{
				fnStr.Delete(pos,1);
				fnStr.Insert(pos,"/");
			}
			fnStr.Insert(0,"/Volumes/");
		#endif	
			
			break;
		}
		case CD_WIN:
		{
			break;
		}
	}
	
	return StringToChar(fnStr);
}

Bool MaterialChannelEnabled(BaseMaterial *mat, const DescID& id)
{
	GeData channelEnabled;
	CDGetParameter(mat,id,channelEnabled);
	if(channelEnabled == true) return true;
	
	return false;
}

Vector CDMCalcFaceNormal(CDMRefPoint *refadr, const CPolygon &ply)
{
	Vector a,b,c,d;
	
	a = refadr[ply.a].GetVector();
	b = refadr[ply.b].GetVector();
	c = refadr[ply.c].GetVector();
	d = refadr[ply.d].GetVector();
	
	if(ply.c == ply.d)
	{
		return VNorm(VCross((b - a), (c - a)));
	}
	else
	{
		return VNorm(VCross((b - d), (c - a)));
	}
}

Vector CDMCalcPolygonCenter(CDMRefPoint *refadr, const CPolygon &ply)
{
	Vector a,b,c,d;
	
	a = refadr[ply.a].GetVector();
	b = refadr[ply.b].GetVector();
	c = refadr[ply.c].GetVector();
	d = refadr[ply.d].GetVector();
	
	if(ply.c == ply.d)
	{
		return (a+b+c)/3;
	}
	else
	{
		return (a+b+c+d)/4;
	}
}

Vector CDMCalcPointNormal(BaseObject *op, CDMRefPoint *refadr, LONG ind)
{
	Vector pt, norm, ptNorm = Vector(0,0,0);
	
	CPolygon *vadr = GetPolygonArray(op);
	if(vadr)
	{
		LONG pCnt = ToPoint(op)->GetPointCount(), vCnt = ToPoly(op)->GetPolygonCount();
		LONG *dadr = NULL, dcnt = 0, p;
		CPolygon ply;
		
		Neighbor n;
		n.Init(pCnt,vadr,vCnt,NULL);
		n.GetPointPolys(ind, &dadr, &dcnt);
		
		pt = refadr[ind].GetVector();
		Real totalLen = 0.0;
		for(p=0; p<dcnt; p++)
		{
			ply = vadr[dadr[p]];
			totalLen += Len(pt - CDMCalcPolygonCenter(refadr,ply));
		}
		for(p=0; p<dcnt; p++)
		{
			ply = vadr[dadr[p]];
			norm = CDMCalcFaceNormal(refadr,ply);
			ptNorm += norm * Len(pt - CDMCalcPolygonCenter(refadr,ply))/totalLen;
		}
	}
	else
	{
		ptNorm = refadr[ind].GetVector();
	}
	
	return VNorm(ptNorm);
}

Matrix GetRPMatrix(LONG pole, Vector o, Vector g, Vector p)
{
	Matrix m = Matrix();
	
	m.off = o;
	switch(pole)
	{
		case POLE_X:	
			m.v2 = VNorm(VCross(g, p));
			m.v1 = VNorm(VCross(m.v2, g));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case POLE_Y:
			m.v1 = VNorm(VCross(p, g));
			m.v2 = VNorm(VCross(g, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case POLE_NX:	
			m.v2 = VNorm(VCross(p, g));
			m.v1 = VNorm(VCross(m.v2, g));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case POLE_NY:
			m.v1 = VNorm(VCross(g, p));
			m.v2 = VNorm(VCross(g, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
	}
	
	return m;	
}

void RecalculateReference(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM)
{
	CDTransMatrixData tmData;
	tmData.nM = newM;
	tmData.oM = oldM;
	
	BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN);
	if(skrTag)
	{
		skrTag->Message(CD_MSG_RECALC_REFERENCE,&tmData);
		
		BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
		if(skTag)
		{
			BaseContainer *skData = skTag->GetDataInstance();
			if(!skData->GetBool(BOUND))
			{
				DescriptionCommand dc;
				dc.id = DescID(CDSKR_RESTORE_REFERENCE);
				skrTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
			}
		}
	}
	BaseTag *mrTag = op->GetTag(ID_CDMORPHREFPLUGIN);
	if(mrTag) mrTag->Message(CD_MSG_RECALC_REFERENCE,&tmData);
}


// CDFbxBatchPaths class functions for coffee scripts
CDFbxBatchPaths::CDFbxBatchPaths()
{
	batchExportPath = "";
	batchImportPath = "";
}

CDFbxBatchPaths::~CDFbxBatchPaths()
{
}

Bool CDFbxBatchPaths::SetExportPath(void)
{
	char *pathName = NULL;
	
	Filename fName;
#if API_VERSION < 12000
	String title = GeLoadString(IDS_BATCH_EXPORT_PATH);
	if(!fName.FileSelect(FSTYPE_SCENES,GE_DIRECTORY,&title)) return false;
#else
	if(!fName.FileSelect(FILESELECTTYPE_SCENES,FILESELECT_DIRECTORY,GeLoadString(IDS_BATCH_EXPORT_PATH))) return false;
#endif
	
	pathName = ConvertPathname(&fName);
	bPaths->batchExportPath = CharToString(pathName)+"/";
	
	CDFree(pathName);
	
	return true;
}

String CDFbxBatchPaths::GetExportPath(void)
{
	return batchExportPath;
}

Bool CDFbxBatchPaths::SetImportPath(void)
{
	char *pathName = NULL;
	
	Filename fName;
#if API_VERSION < 12000
	String title = GeLoadString(IDS_BATCH_IMPORT_PATH);
	if(!fName.FileSelect(FSTYPE_SCENES,GE_DIRECTORY,&title)) return false;
#else
	if(!fName.FileSelect(FILESELECTTYPE_SCENES,FILESELECT_DIRECTORY,GeLoadString(IDS_BATCH_IMPORT_PATH))) return false;
#endif
	
	pathName = ConvertPathname(&fName);
	bPaths->batchImportPath = CharToString(pathName)+"/";
	
	CDFree(pathName);
	
	return true;
}

String CDFbxBatchPaths::GetImportPath(void)
{
	return batchImportPath;
}

Bool SetBatchOutputPath(void)
{
	if(!bPaths) return false;
	return bPaths->SetExportPath();
}

String GetBatchOutputPath(void)
{
	if(!bPaths) return "";
	return bPaths->GetExportPath();
}

Bool SetBatchInputPath(void)
{
	if(!bPaths) return false;
	return bPaths->SetImportPath();
}

String GetBatchInputPath(void)
{
	if(!bPaths) return "";
	return bPaths->GetImportPath();
}



// coffee extension functions
void CDFbxClearExportData(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	BaseContainer bc;
	bc.SetBool(IDC_EXPORT_JOINT,false);
	bc.SetBool(IDC_EXPORT_MESH,false);
	bc.SetBool(IDC_EXPORT_MORPHS,false);
	bc.SetBool(IDC_EXPORT_CAMERA,false);
	bc.SetBool(IDC_EXPORT_LIGHT,false);
	bc.SetBool(IDS_EXPORT_SPLINE,false);
	
	bc.SetBool(IDS_EXPORT_SMOOTHING,false);
	bc.SetBool(IDS_EXPORT_USER_DATA,false);
	bc.SetBool(IDS_EXPORT_CONSTRAINTS,false);
	
	bc.SetBool(IDC_EXPORT_ANIMATION,false);
	bc.SetBool(IDS_EXPORT_CURVES,false);
	bc.SetBool(IDC_BAKE_JOINT_ANIMATION,false);
	bc.SetBool(IDC_BAKE_POSITION,false);
	bc.SetBool(IDC_BAKE_SCALE,false);
	bc.SetBool(IDC_BAKE_ROTATION,false);
	
	bc.SetBool(IDC_EXPORT_PLA,false);
	bc.SetBool(IDC_BAKE_CDSKIN_TO_PLA,false);
	bc.SetBool(IDC_BAKE_MESH_TO_PLA,false);
	bc.SetBool(IDC_BAKE_CLOTH_TO_PLA,false);
	
	bc.SetBool(IDC_EMBED_TEXTURES,false);
	bc.SetBool(IDC_PRESERVE_HIERARCHY,false);
	bc.SetBool(IDC_ONLY_SELECTED,false);
	
	bc.SetLong(IDC_HANDED,0);
	bc.SetReal(IDC_EXPORT_SCALE,1.0);
	res = SetWorldPluginData(ID_CDFBXEXPORTER,bc,false);

	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}

void CDFbxSetExportData(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	if(argc == 2)
	{
		BaseContainer bc;
		LONG id = sp[argc-1].GetLong();
		
		if(id == IDC_EXPORT_SCALE)
		{
			Real value = sp[argc-2].GetReal();
			bc.SetReal(IDC_EXPORT_SCALE,value);
			res = SetWorldPluginData(ID_CDFBXEXPORTER,bc,true);
		}
		else if(id == IDC_HANDED)
		{
			LONG value = sp[argc-2].GetLong();
			bc.SetLong(IDC_HANDED,value);
			res = SetWorldPluginData(ID_CDFBXEXPORTER,bc,true);
		}
		else
		{
			Bool value = sp[argc-2].GetLong();
			bc.SetBool(id,value);
			res = SetWorldPluginData(ID_CDFBXEXPORTER,bc,true);
		}
	}
	
	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}

void CDFbxSetOutputPath(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = SetBatchOutputPath();
	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}

void CDFbxClearImportData(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	BaseContainer bc;
	bc.SetBool(IDC_IMPORT_JOINT,false);
	bc.SetBool(IDC_IMPORT_MESH,false);
	bc.SetBool(IDC_IMPORT_MORPH,false);
	bc.SetBool(IDC_IMPORT_NURBS,false);
	bc.SetLong(IDC_NURBS_SURFACE_LOD,0);
	
	bc.SetBool(IDC_IMPORT_CAMERA,false);
	bc.SetBool(IDC_IMPORT_LIGHT,false);
	bc.SetBool(IDC_IMPORT_MARKER,false);
	bc.SetBool(IDC_IMPORT_NULL,false);
	bc.SetBool(IDS_IMPORT_SPLINE,false);
	
	bc.SetBool(IDS_IMPORT_SMOOTHING,false);
	bc.SetBool(IDS_IMPORT_MATERIALS,false);
	bc.SetBool(IDS_IMPORT_TEXTURES,false);
	bc.SetBool(IDS_IMPORT_USER_DATA,false);
	bc.SetBool(IDS_IMPORT_CONSTRAINTS,false);
	
	bc.SetBool(IDC_IMPORT_ANIMATION,false);
	bc.SetBool(IDS_IMPORT_CURVES,false);
	bc.SetBool(IDC_IMPORT_PLA,false);
	
	bc.SetReal(IDC_IMPORT_SCALE,1.0);
	bc.SetBool(IDS_FREEZE_SCALE,false);
	res = SetWorldPluginData(ID_CDFBXIMPORTER,bc,false);

	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}

void CDFbxSetImportData(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	if(argc == 2)
	{
		BaseContainer bc;
		LONG id = sp[argc-1].GetLong();
		
		if(id == IDC_IMPORT_SCALE)
		{
			Real value = sp[argc-2].GetReal();
			bc.SetReal(IDC_IMPORT_SCALE,value);
			SetWorldPluginData(ID_CDFBXIMPORTER,bc,true);
		}
		else if(id == IDC_NURBS_SURFACE_LOD)
		{
			LONG value = sp[argc-2].GetLong();
			bc.SetLong(IDC_NURBS_SURFACE_LOD,value);
			SetWorldPluginData(ID_CDFBXIMPORTER,bc,true);
		}
		else if(id == IDC_TAKE)
		{
			LONG value = sp[argc-2].GetLong();
			bc.SetLong(IDC_TAKE,value);
			SetWorldPluginData(ID_CDFBXIMPORTER,bc,true);
		}
		else if(id == IDC_IMPORT_START)
		{
			LONG value = sp[argc-2].GetLong();
			bc.SetLong(IDC_IMPORT_START,value);
			SetWorldPluginData(ID_CDFBXIMPORTER,bc,true);
		}
		else if(id == IDC_IMPORT_OFFSET)
		{
			LONG value = sp[argc-2].GetLong();
			bc.SetLong(IDC_IMPORT_OFFSET,value);
			SetWorldPluginData(ID_CDFBXIMPORTER,bc,true);
		}
		else if(id == IDC_IMPORT_END)
		{
			LONG value = sp[argc-2].GetLong();
			bc.SetLong(IDC_IMPORT_END,value);
			SetWorldPluginData(ID_CDFBXIMPORTER,bc,true);
		}
		else
		{
			Bool value = sp[argc-2].GetLong();
			bc.SetBool(id,value);
			SetWorldPluginData(ID_CDFBXIMPORTER,bc,true);
		}
	}
	
	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}

void CDFbxSetInputPath(Coffee* cof, VALUE *&sp, LONG argc)
{
	String inPath = "";
	if(SetBatchInputPath()) inPath = GetBatchInputPath();
	
	GeCoffeeGeData2Value(cof, inPath, &sp[argc]);
	sp += argc;
}

Bool CDFbxAddSymbols(Coffee *cofM)
{
	// export symbols
	VALUE CDFBX_HANDED;
	CDFBX_HANDED.SetLong(12000);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_HANDED",&CDFBX_HANDED)) return false;

	VALUE CDFBX_LEFT_HANDED;
	CDFBX_LEFT_HANDED.SetLong(0);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_LEFT_HANDED",&CDFBX_LEFT_HANDED)) return false;
	
	VALUE CDFBX_RIGHT_HANDED;
	CDFBX_RIGHT_HANDED.SetLong(1);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_RIGHT_HANDED",&CDFBX_RIGHT_HANDED)) return false;

	VALUE CDFBX_EXPORT_SCALE;
	CDFBX_EXPORT_SCALE.SetLong(12003);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_SCALE",&CDFBX_EXPORT_SCALE)) return false;
	
	VALUE CDFBX_EXPORT_JOINT;
	CDFBX_EXPORT_JOINT.SetLong(12005);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_JOINT",&CDFBX_EXPORT_JOINT)) return false;
	
	VALUE CDFBX_EXPORT_MESH;
	CDFBX_EXPORT_MESH.SetLong(12006);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_MESH",&CDFBX_EXPORT_MESH)) return false;
	
	VALUE CDFBX_EXPORT_CAMERA;
	CDFBX_EXPORT_CAMERA.SetLong(12007);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_CAMERA",&CDFBX_EXPORT_CAMERA)) return false;
	
	VALUE CDFBX_EXPORT_LIGHT;
	CDFBX_EXPORT_LIGHT.SetLong(12008);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_LIGHT",&CDFBX_EXPORT_LIGHT)) return false;
	
	VALUE CDFBX_EXPORT_ANIMATION;
	CDFBX_EXPORT_ANIMATION.SetLong(12009);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_ANIMATION",&CDFBX_EXPORT_ANIMATION)) return false;
	
	VALUE CDFBX_BAKE_JOINT_ANIMATION;
	CDFBX_BAKE_JOINT_ANIMATION.SetLong(12010);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_BAKE_JOINT_ANIMATION",&CDFBX_BAKE_JOINT_ANIMATION)) return false;
	
	VALUE CDFBX_BAKE_POSITION;
	CDFBX_BAKE_POSITION.SetLong(12011);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_BAKE_POSITION",&CDFBX_BAKE_POSITION)) return false;
	
	VALUE CDFBX_BAKE_SCALE;
	CDFBX_BAKE_SCALE.SetLong(12012);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_BAKE_SCALE",&CDFBX_BAKE_SCALE)) return false;
	
	VALUE CDFBX_BAKE_ROTATION;
	CDFBX_BAKE_ROTATION.SetLong(12013);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_BAKE_ROTATION",&CDFBX_BAKE_ROTATION)) return false;
	
	VALUE CDFBX_EXPORT_MORPHS;
	CDFBX_EXPORT_MORPHS.SetLong(12014);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_MORPHS",&CDFBX_EXPORT_MORPHS)) return false;
	
	VALUE CDFBX_PRESERVE_HIERARCHY;
	CDFBX_PRESERVE_HIERARCHY.SetLong(12015);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_PRESERVE_HIERARCHY",&CDFBX_PRESERVE_HIERARCHY)) return false;
	
	VALUE CDFBX_EXPORT_PLA;
	CDFBX_EXPORT_PLA.SetLong(12016);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_PLA",&CDFBX_EXPORT_PLA)) return false;
	
	VALUE CDFBX_BAKE_MESH_TO_PLA;
	CDFBX_BAKE_MESH_TO_PLA.SetLong(12017);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_BAKE_MESH_TO_PLA",&CDFBX_BAKE_MESH_TO_PLA)) return false;
	
	VALUE CDFBX_BAKE_CLOTH_TO_PLA;
	CDFBX_BAKE_CLOTH_TO_PLA.SetLong(12018);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_BAKE_CLOTH_TO_PLA",&CDFBX_BAKE_CLOTH_TO_PLA)) return false;
	
	VALUE CDFBX_BAKE_CDSKIN_TO_PLA;
	CDFBX_BAKE_CDSKIN_TO_PLA.SetLong(12019);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_BAKE_CDSKIN_TO_PLA",&CDFBX_BAKE_CDSKIN_TO_PLA)) return false;
	
	VALUE CDFBX_EMBED_TEXTURES;
	CDFBX_EMBED_TEXTURES.SetLong(12020);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EMBED_TEXTURES",&CDFBX_EMBED_TEXTURES)) return false;
	
	VALUE CDFBX_ONLY_SELECTED;
	CDFBX_ONLY_SELECTED.SetLong(12021);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_ONLY_SELECTED",&CDFBX_ONLY_SELECTED)) return false;
	
	VALUE CDFBX_EXPORT_CURVES;
	CDFBX_EXPORT_CURVES.SetLong(12022);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_CURVES",&CDFBX_EXPORT_CURVES)) return false;
	
	VALUE CDFBX_EXPORT_USER_DATA;
	CDFBX_EXPORT_USER_DATA.SetLong(12023);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_USER_DATA",&CDFBX_EXPORT_USER_DATA)) return false;
	
	VALUE CDFBX_EXPORT_SPLINE;
	CDFBX_EXPORT_SPLINE.SetLong(12024);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_SPLINE",&CDFBX_EXPORT_SPLINE)) return false;
	
	VALUE CDFBX_EXPORT_SMOOTHING;
	CDFBX_EXPORT_SMOOTHING.SetLong(12025);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_SMOOTHING",&CDFBX_EXPORT_SMOOTHING)) return false;
	
	VALUE CDFBX_EXPORT_CONSTRAINTS;
	CDFBX_EXPORT_CONSTRAINTS.SetLong(12026);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_EXPORT_CONSTRAINTS",&CDFBX_EXPORT_CONSTRAINTS)) return false;
	
	VALUE CDFBX_ALPHANUMERIC_ONLY;
	CDFBX_EXPORT_CONSTRAINTS.SetLong(12028);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_ALPHANUMERIC_ONLY",&CDFBX_ALPHANUMERIC_ONLY)) return false;
	
	// import symbols
	VALUE CDFBX_IMPORT_SCALE;
	CDFBX_IMPORT_SCALE.SetLong(11001);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_SCALE",&CDFBX_IMPORT_SCALE)) return false;
	
	VALUE CDFBX_IMPORT_JOINT;
	CDFBX_IMPORT_JOINT.SetLong(11003);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_JOINT",&CDFBX_IMPORT_JOINT)) return false;
	
	VALUE CDFBX_IMPORT_MESH;
	CDFBX_IMPORT_MESH.SetLong(11004);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_MESH",&CDFBX_IMPORT_MESH)) return false;
	
	VALUE CDFBX_IMPORT_CAMERA;
	CDFBX_IMPORT_CAMERA.SetLong(11005);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_CAMERA",&CDFBX_IMPORT_CAMERA)) return false;
	
	VALUE CDFBX_IMPORT_LIGHT;
	CDFBX_IMPORT_LIGHT.SetLong(11006);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_LIGHT",&CDFBX_IMPORT_LIGHT)) return false;
	
	VALUE CDFBX_IMPORT_ANIMATION;
	CDFBX_IMPORT_ANIMATION.SetLong(11007);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_ANIMATION",&CDFBX_IMPORT_ANIMATION)) return false;
	
	VALUE CDFBX_IMPORT_MARKER;
	CDFBX_IMPORT_MARKER.SetLong(11008);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_MARKER",&CDFBX_IMPORT_MARKER)) return false;
	
	VALUE CDFBX_IMPORT_MORPH;
	CDFBX_IMPORT_MORPH.SetLong(11009);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_MORPH",&CDFBX_IMPORT_MORPH)) return false;
	
	VALUE CDFBX_IMPORT_NULL;
	CDFBX_IMPORT_NULL.SetLong(11010);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_NULL",&CDFBX_IMPORT_NULL)) return false;
	
	VALUE CDFBX_IMPORT_PLA;
	CDFBX_IMPORT_PLA.SetLong(11011);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_PLA",&CDFBX_IMPORT_PLA)) return false;
	
	VALUE CDFBX_IMPORT_START;
	CDFBX_IMPORT_START.SetLong(11012);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_START",&CDFBX_IMPORT_START)) return false;
	
	VALUE CDFBX_IMPORT_OFFSET;
	CDFBX_IMPORT_OFFSET.SetLong(11013);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_OFFSET",&CDFBX_IMPORT_OFFSET)) return false;
	
	VALUE CDFBX_IMPORT_END;
	CDFBX_IMPORT_END.SetLong(11014);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_END",&CDFBX_IMPORT_END)) return false;
	
	VALUE CDFBX_IMPORT_NURBS;
	CDFBX_IMPORT_NURBS.SetLong(11015);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_NURBS",&CDFBX_IMPORT_NURBS)) return false;
	
	VALUE CDFBX_NURBS_SURFACE_LOD;
	CDFBX_NURBS_SURFACE_LOD.SetLong(11016);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_NURBS_SURFACE_LOD",&CDFBX_NURBS_SURFACE_LOD)) return false;
	
	VALUE CDFBX_NURBS_LOW;
	CDFBX_NURBS_LOW.SetLong(0);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_NURBS_LOW",&CDFBX_NURBS_LOW)) return false;
	
	VALUE CDFBX_NURBS_MEDIUM;
	CDFBX_NURBS_MEDIUM.SetLong(1);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_NURBS_MEDIUM",&CDFBX_NURBS_MEDIUM)) return false;
	
	VALUE CDFBX_NURBS_HIGH;
	CDFBX_NURBS_HIGH.SetLong(2);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_NURBS_HIGH",&CDFBX_NURBS_HIGH)) return false;
	
	VALUE CDFBX_FREEZE_SCALE;
	CDFBX_FREEZE_SCALE.SetLong(11020);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_FREEZE_SCALE",&CDFBX_FREEZE_SCALE)) return false;
	
	VALUE CDFBX_IMPORT_SMOOTHING;
	CDFBX_IMPORT_SMOOTHING.SetLong(11023);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_SMOOTHING",&CDFBX_IMPORT_SMOOTHING)) return false;
	
	VALUE CDFBX_IMPORT_USER_DATA;
	CDFBX_IMPORT_USER_DATA.SetLong(11024);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_USER_DATA",&CDFBX_IMPORT_USER_DATA)) return false;
	
	VALUE CDFBX_IMPORT_CURVES;
	CDFBX_IMPORT_CURVES.SetLong(11025);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_CURVES",&CDFBX_IMPORT_CURVES)) return false;
	
	VALUE CDFBX_IMPORT_SPLINE;
	CDFBX_IMPORT_SPLINE.SetLong(11026);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_SPLINE",&CDFBX_IMPORT_SPLINE)) return false;
	
	VALUE CDFBX_IMPORT_CONSTRAINTS;
	CDFBX_IMPORT_CONSTRAINTS.SetLong(11027);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_CONSTRAINTS",&CDFBX_IMPORT_CONSTRAINTS)) return false;
	
	VALUE CDFBX_IMPORT_MATERIALS;
	CDFBX_IMPORT_MATERIALS.SetLong(11028);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_MATERIALS",&CDFBX_IMPORT_MATERIALS)) return false;
	
	VALUE CDFBX_IMPORT_TEXTURES;
	CDFBX_IMPORT_TEXTURES.SetLong(11029);
	if(!CDAddGlobalSymbol(cofM,"CDFBX_IMPORT_TEXTURES",&CDFBX_IMPORT_TEXTURES)) return false;
	
	return true;
}



// cdjs library functions
CDJSFuncLib cdjsFLib;

CDMRefPoint* iCDGetSkinReference(BaseTag *tag);
CDMRefPoint* iCDGetDeformReference(BaseTag *tag);
Bool iCDGetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool iCDSetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool iCDRemapClusterWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map);
Bool iCDGetJointWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG pCnt);
Bool iCDSetJointWeight(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt);
Bool iCDGetSkinJointColors(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt);
CDSkinVertex* iCDGetSkinWeights(BaseTag *tag);

Bool RegisterCDJSFunctionLib(void)
{
	// Clear the structure
    ClearMem(&cdjsFLib, sizeof(cdjsFLib));
	
    // Fill in all function pointers
    cdjsFLib.CDGetSkinReference = iCDGetSkinReference;
    cdjsFLib.CDGetDeformReference = iCDGetDeformReference;
    cdjsFLib.CDGetClusterWeight = iCDGetClusterWeight;
    cdjsFLib.CDSetClusterWeight = iCDSetClusterWeight;
    cdjsFLib.CDRemapClusterWeights = iCDRemapClusterWeights;
    cdjsFLib.CDGetJointWeight = iCDGetJointWeight;
    cdjsFLib.CDSetJointWeight = iCDSetJointWeight;
    cdjsFLib.CDGetSkinJointColors = iCDGetSkinJointColors;
    cdjsFLib.CDGetSkinWeights = iCDGetSkinWeights;
 	
    // Install the library
    return InstallLibrary(ID_CDJSFUNCLIB, &cdjsFLib, 1, sizeof(cdjsFLib));
}

// cdm library functions
CDMorphFuncLib cdmFLib;

CDMRefPoint* iCDGetMorphReference(BaseTag *tag);
CDMRefPoint* iCDGetMorphCache(BaseTag *tag);
CDMPoint* iCDGetMorphPoints(BaseTag *tag);
CDMSpPoint* iCDGetMorphSphericalPoints(BaseTag *tag);

Bool RegisterCDMorphFunctionLib(void)
{
    // Clear the structure
    ClearMem(&cdmFLib, sizeof(cdmFLib));
	
    // Fill in all function pointers
    cdmFLib.CDGetMorphReference = iCDGetMorphReference;
    cdmFLib.CDGetMorphCache = iCDGetMorphCache;
    cdmFLib.CDGetMorphPoints = iCDGetMorphPoints;
    cdmFLib.CDGetMorphSphericalPoints = iCDGetMorphSphericalPoints;
 	
    // Install the library
    return InstallLibrary(ID_CDMORPHFUNCLIB, &cdmFLib, 1, sizeof(cdmFLib));
}


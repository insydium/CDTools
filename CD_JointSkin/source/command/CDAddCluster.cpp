//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "tCDCluster.h"
#include "CDClusterTag.h"
#include "CDJointSkin.h"

extern Bool clusDialogOpen;
extern Real clusDialogRad;
extern Real clusDialogMax;
extern Real clusDialogFalloff;

class CDAddSkinClusterDialog : public GeDialog
{
	private:
		CDJSOptionsUA ua;
		
	public:
		Bool		openPaint;
		Real		clusRad, maxWt, falloff;
	
		Bool AddClusterTag(BaseDocument* doc);
		Bool AutoComputeWeights(BaseDocument* doc, BaseTag *tag, BaseObject *mesh, BaseObject *joint, Real *tWeight);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
		virtual void DestroyWindow(void);
};

Bool CDAddSkinClusterDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle("CD Add Skin Cluster");

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
			GroupBorder(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT|BFV_SCALEFIT,1,0,GeLoadString(IDC_INFLUENCE),0);
			{
				GroupBorder(BORDER_GROUP_IN);
				GroupBorderSpace(8,8,8,8);

				GroupBegin(0,BFH_SCALEFIT,4,0,"",0);
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDS_CLUSTER_RADIUS),0);
					AddEditNumberArrows(IDS_CLUSTER_RADIUS,BFH_LEFT);
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDS_MAXWEIGHT),0);
					AddEditNumberArrows(IDS_MAXWEIGHT,BFH_LEFT);
				GroupEnd();
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					GroupSpace(4,1);
					AddStaticText(0,0,0,0,GeLoadString(IDC_FALLOFF),0);
					AddEditSlider(IDC_FALLOFF,BFH_SCALEFIT);
				GroupEnd();

			}
			GroupEnd();

			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				GroupSpace(4,1);
				AddCheckbox (IDC_OPENPAINTTOOL,BFH_LEFT,0,0,GeLoadString(IDC_OPENPAINTTOOL));
			GroupEnd();

			AddDlgGroup(DLG_OK | DLG_CANCEL);
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDAddSkinClusterDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;

	BaseContainer *bc = GetWorldPluginData(ID_CDADDSKINCLUSTER);
	if(!bc)
	{
		SetReal(IDS_CLUSTER_RADIUS,10.0,0.0,CDMAXREAL,1.0,FORMAT_REAL);
		SetReal(IDS_MAXWEIGHT,0.5,0.0,1.0,0.01,FORMAT_PERCENT);
		SetReal(IDC_FALLOFF,0.0,0.0,4.5,0.1,FORMAT_REAL);
		SetBool(IDC_OPENPAINTTOOL,true);
	}
	else
	{
		SetReal(IDS_CLUSTER_RADIUS,bc->GetReal(IDS_CLUSTER_RADIUS),0.0,CDMAXREAL,1.0,FORMAT_REAL);
		SetReal(IDS_MAXWEIGHT,bc->GetReal(IDS_MAXWEIGHT),0.0,1.0,0.01,FORMAT_PERCENT);
		SetReal(IDC_FALLOFF,bc->GetReal(IDC_FALLOFF),0.0,4.5,0.1,FORMAT_REAL);
		SetBool(IDC_OPENPAINTTOOL,bc->GetBool(IDC_OPENPAINTTOOL));
	}

	GetReal(IDS_CLUSTER_RADIUS,clusRad);
	GetReal(IDS_MAXWEIGHT,maxWt);
	GetReal(IDC_FALLOFF,falloff);
	GetBool(IDC_OPENPAINTTOOL,openPaint);

	clusDialogOpen = true;
	clusDialogRad = clusRad * 0.9;
	clusDialogMax = maxWt;
	clusDialogFalloff = clusDialogRad * (0.5 + (0.1 * falloff));

	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);

	return true;
}

Bool CDAddSkinClusterDialog::Command(LONG id,const BaseContainer &msg)
{
	StopAllThreads(); // so the document can be safely modified
	
	GetReal(IDS_CLUSTER_RADIUS,clusRad);
	GetReal(IDS_MAXWEIGHT,maxWt);
	GetReal(IDC_FALLOFF,falloff);
	GetBool(IDC_OPENPAINTTOOL,openPaint);
	
	clusDialogRad = clusRad * 0.9;
	clusDialogMax = maxWt;
	clusDialogFalloff = clusDialogRad * (0.5 + (0.1 * falloff));

	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	
	BaseContainer bc;
	bc.SetReal(IDS_CLUSTER_RADIUS,clusRad);
	bc.SetReal(IDS_MAXWEIGHT,maxWt);
	bc.SetReal(IDC_FALLOFF,falloff);
	bc.SetBool(IDC_OPENPAINTTOOL,openPaint);
	SetWorldPluginData(ID_CDADDSKINCLUSTER,bc,false);
	
	switch (id)
	{
		case DLG_OK:
			if(!AddClusterTag(GetActiveDocument()))
			{
				Close();
				return false;
			}
			Close();
			break;
		case DLG_CANCEL:
			Close();
			break;
	}
	
	return true;
}

void CDAddSkinClusterDialog::DestroyWindow(void)
{
	clusDialogOpen = false;
	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
}

Bool CDAddSkinClusterDialog::AutoComputeWeights(BaseDocument* doc, BaseTag *tag, BaseObject *mesh, BaseObject *joint, Real *tWeight)
{
	Matrix localM = MInv(mesh->GetMg()) * joint->GetMg();
	Vector skinVert, *padr = GetPointArray(mesh);
	
	SplineObject *falloffspline = CDAllocateSplineObject(5,CD_SPLINETYPE_BSPLINE);
	Real f = falloff * 10;
	Vector *fsPts = GetPointArray(falloffspline);
	fsPts[0] = Vector(0.0,0.0,0.0);
	fsPts[1] = Vector(f,0.0,0.0);
	fsPts[2] = Vector(50.0,0.5,0.0);
	fsPts[3] = Vector(100.0-f,1.0,0.0);
	fsPts[4] = Vector(100.0,1.0,0.0);
	
	LONG i, ptCount = ToPoint(mesh)->GetPointCount();
	for(i=0; i<ptCount; i++)
	{
		StatusSetBar(LONG(Real(i)/Real(ptCount)*100.0));
		skinVert = padr[i];
		tWeight[i] = 0.0;
		Real dist = Len(skinVert - localM.off);
		if(dist <= clusRad)
		{
			Vector softPt = falloffspline->GetSplinePoint(CDUniformToNatural(falloffspline,(clusRad - dist)/clusRad));
			Real fWeight = softPt.y;
			if(fWeight < 0.01) fWeight = 0.0;
			tWeight[i] = fWeight * maxWt;
		}
	}
	
	SplineObject::Free(falloffspline);
	
	return true;
}

Bool CDAddSkinClusterDialog::AddClusterTag(BaseDocument* doc)
{
	AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	
	LONG  selCount = opSelectionList->GetCount();
	if(selCount != 2) return false;
	
	BaseObject *mesh = static_cast<BaseObject*>(opSelectionList->GetIndex(0)); 
	if(!mesh) return false;
	if(!IsValidPointObject(mesh)) return false;
	
	BaseObject *dest = static_cast<BaseObject*>(opSelectionList->GetIndex(1)); 
	if(!dest) return false;
		
	doc->SetActiveObject(dest);
	StatusSetSpin();
	doc->StartUndo();

	BaseTag *skinRef = mesh->GetTag(ID_CDSKINREFPLUGIN);
	if(!skinRef)
	{
		skinRef = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
		mesh->InsertTag(skinRef,NULL);
		CDAddUndo(doc,CD_UNDO_NEW,skinRef);
		mesh->Message(MSG_UPDATE);
		
		DescriptionCommand skrdc;
		skrdc.id = DescID(1000);
		skinRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
	}
	
	BaseTag *cTag = BaseTag::Alloc(ID_CDCLUSTERTAGPLUGIN);
	dest->InsertTag(cTag,NULL);
	cTag->Message(MSG_MENUPREPARE);
	CDAddUndo(doc,CD_UNDO_NEW,cTag);
	
	CDAddUndo(doc,CD_UNDO_CHANGE,cTag);
	BaseContainer *tData = cTag->GetDataInstance();
	tData->SetLink(CLST_DEST_LINK,mesh);
	tData->SetLink(CLST_DEST_ID,mesh);
	if(!mesh->GetTag(ID_CDSKINPLUGIN)) tData->SetBool(CLST_MIX_PREVIOUS,false);
	else tData->SetBool(CLST_MIX_PREVIOUS,true);
	cTag->Message(MSG_UPDATE);
	
	LONG ptCnt = ToPoint(mesh)->GetPointCount();
	CDArray<Real> tWeight;
	tWeight.Alloc(ptCnt);
	
	// Auto compute weights
	if(!AutoComputeWeights(doc,cTag,mesh,dest,tWeight.Array()))
	{
		tWeight.Free();
		StatusClear();
		return false;
	}
	CDSetClusterWeight(cTag, tData, tWeight.Array(), ptCnt);
	mesh->Message(MSG_UPDATE);
	
	DescriptionCommand dc;
	dc.id = DescID(CLST_BIND_POINTS);
	cTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	cTag->Message(MSG_UPDATE);

	doc->SetActiveTag(cTag);
	doc->EndUndo();
	
	StatusClear();

	if(openPaint)
	{
		doc->SetAction(ID_SKINWEIGHTTOOL);
		SpecialEventAdd(ID_SKINWEIGHTTOOL); // needed to update tool dialog layout
	}
	
	tWeight.Free();
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;
}

class CDAddSkinCluster : public CommandData
{
	private:
		CDAddSkinClusterDialog dlg;

	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	
			LONG  selCount = opSelectionList->GetCount();
			if(selCount != 2) return false;
			
			BaseObject *mesh = static_cast<BaseObject*>(opSelectionList->GetIndex(0)); 
			if(!mesh) return false;
			if(!IsValidPointObject(mesh)) return false;
			
			BaseObject *dest = static_cast<BaseObject*>(opSelectionList->GetIndex(1)); 
			if(!dest) return false;
			
			BaseContainer *bc = GetWorldPluginData(ID_CDADDSKINCLUSTER);
			if(bc) clusDialogRad = bc->GetLong(IDS_CLUSTER_RADIUS);
			
		#if API_VERSION < 12000
			return dlg.Open(true,ID_CDADDSKINCLUSTER,-1,-1);
		#else
			return dlg.Open(DLG_TYPE_ASYNC,ID_CDADDSKINCLUSTER,-1,-1);
		#endif
		}
		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDADDSKINCLUSTER,0,secret);
		}
};

class CDAddSkinClusterR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddSkinCluster(void)
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
	String name=GeLoadString(IDS_ADDCLUSTER); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDSKINCLUSTER,name,PLUGINFLAG_HIDE,"CDAddCluster.tif","CD Add Skin Cluster",CDDataAllocator(CDAddSkinClusterR));
	else return CDRegisterCommandPlugin(ID_CDADDSKINCLUSTER,name,0,"CDAddCluster.tif","CD Add Skin Cluster",CDDataAllocator(CDAddSkinCluster));
}

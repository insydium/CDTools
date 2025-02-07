//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "CDMorph.h"
#include "CDMorphTag.h"

#include "tCDMorphTag.h"

extern Bool mSplitDialogOpen;

class CDMorphSplitDialog : public GeDialog
{
	private:
		CDMOptionsUA ua;
	
		Bool CreateSplitTags(void);
			
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
		virtual void DestroyWindow(void);
};

Bool CDMorphSplitDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDMSPLIT));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDM_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDM_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorder(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDC_SPLIT_AXIS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_SPLIT_AXIS,BFH_CENTER,3,1);
					AddChild(IDC_SPLIT_AXIS, 0, GeLoadString(M_X));
					AddChild(IDC_SPLIT_AXIS, 1, GeLoadString(M_Y));
					AddChild(IDC_SPLIT_AXIS, 2, GeLoadString(M_Z));
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					GroupSpace(4,1);
					AddStaticText(0,0,0,0,GeLoadString(IDC_FALLOFF),0);
					AddEditSlider(IDC_FALLOFF,BFH_SCALEFIT);
				GroupEnd();
				
			}
			GroupEnd();
			
			AddDlgGroup(DLG_OK | DLG_CANCEL);
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDMorphSplitDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDMORPHSPLIT);
	if(!wpData)
	{
		SetLong(IDC_SPLIT_AXIS,0);
		SetReal(IDC_FALLOFF,0.25,0.0,1.0,0.01,FORMAT_REAL);
	}
	else
	{
		SetLong(IDC_SPLIT_AXIS,wpData->GetLong(IDC_SPLIT_AXIS));
		SetReal(IDC_FALLOFF,wpData->GetReal(IDC_FALLOFF),0.0,1.0,0.01,FORMAT_REAL);
	}
	
	mSplitDialogOpen = true;
	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	
	return true;
}

Bool CDMorphSplitDialog::Command(LONG id,const BaseContainer &msg)
{
	Real		falloff;
	LONG		axis;
	
	GetLong(IDC_SPLIT_AXIS,axis);
	GetReal(IDC_FALLOFF,falloff);
	
	BaseContainer wpData;
	wpData.SetLong(IDC_SPLIT_AXIS,axis);
	wpData.SetReal(IDC_FALLOFF,falloff);
	SetWorldPluginData(ID_CDMORPHSPLIT,wpData,false);
	
	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	
	switch (id)
	{
		case DLG_OK:
			if(!CreateSplitTags())
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

Bool CDMorphSplitDialog::CreateSplitTags(void)
{	
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	LONG mode = doc->GetMode();
	
	BaseTag *tag = doc->GetActiveTag(); if(!tag) return false;
	if(tag->GetType() != ID_CDMORPHTAGPLUGIN) return false;
	
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	BaseObject *op = tData->GetObjectLink(MT_DEST_LINK,doc); if(!op) return false;
	BaseObject *trg = doc->GetActiveObject(); if(!trg) trg = op;
	
	Vector *padr = GetPointArray(op); if(!padr) return false;
	
	Matrix opM = op->GetMg();
	
	CDMPoint *mpadr = CDGetMorphPoints(tag); if(!mpadr) return false;
	
	LONG i, pInd, cnt = tData->GetLong(MT_BS_POINT_COUNT), pCnt = tData->GetLong(MT_POINT_COUNT);
	
	CDAABB bnds;
	bnds.Empty();
	
	for(i=0; i<cnt; i++)
	{
		pInd = mpadr[i].ind;
		if(pInd < pCnt)
		{
			Vector pt = padr[pInd];
			bnds.AddPoint(pt);
		}
	}
	Vector min = bnds.min, max = bnds.max, cen = bnds.GetCenterPoint();
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDMORPHSPLIT); if(!wpData) return false;
	LONG axis = wpData->GetLong(IDC_SPLIT_AXIS);
	Real f = wpData->GetReal(IDC_FALLOFF);
	if(f < 0.01) f = 0.01; // prevent division by 0
	
	BaseTag *cln1 = (BaseTag*)CDGetClone(tag,CD_COPYFLAGS_NO_ANIMATION,NULL); if(!cln1) return false;
	
	CDMPoint *c1padr = CDGetMorphPoints(cln1);
	if(!c1padr)
	{
		if(cln1) BaseTag::Free(cln1);
		return false;
	}
	trg->InsertTag(cln1,NULL);
	
	ModelingCommandData	mcd;
	mcd.doc = doc;
#if API_VERSION < 12000
	mcd.mode = MODIFY_POINTSELECTION;
#else
	mcd.mode = MODELINGCOMMANDMODE_POINTSELECTION;
#endif
	mcd.op = op;
	
	BaseSelect *bs = ToPoint(op)->GetPointS();
	
	DescriptionCommand dc;
	if(bs)
	{
		dc.id = DescID(MT_EDIT_SELECTION);
		cln1->Message(MSG_DESCRIPTION_COMMAND,&dc);
	}
		
	SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
	for(i=0; i<cnt; i++)
	{
		pInd = mpadr[i].ind;
		if(pInd < pCnt)
		{
			Real t;
			Vector pt = padr[pInd];
			switch(axis)
			{
				case 0:
					t = CDBlend(-1.0,1.0,(pt.x-min.x)/(max.x-min.x));
					break;
				case 1:
					t = CDBlend(-1.0,1.0,(pt.y-min.y)/(max.y-min.y));
					break;
				case 2:
					t = CDBlend(-1.0,1.0,(pt.z-min.x)/(max.z-min.z));
					break;
			}
			Real scale = 0.0;
			if(t <= f && t >= -f) scale = CDBlend(0.0,1.0,(t-(-f))/(f-(-f)));
			else
			{
				if(t > f) scale = 1.0;
				else if(t < -f) scale = 0.0;
			}
			if(bs && scale > 0.0) bs->Select(pInd);
			c1padr[i].SetVector(mpadr[i].GetVector()*scale);
		}
	}
	if(bs)
	{
		dc.id = DescID(MT_SET_SELECTION);
		cln1->Message(MSG_DESCRIPTION_COMMAND,&dc);
	}
	
	
	BaseTag *cln2 = (BaseTag*)CDGetClone(tag,CD_COPYFLAGS_NO_ANIMATION,NULL); if(!cln2) return false;
	
	CDMPoint *c2padr = CDGetMorphPoints(cln2);
	if(!c2padr)
	{
		if(cln2) BaseTag::Free(cln2);
		return false;
	}
	trg->InsertTag(cln2,NULL);
	
	if(bs)
	{
		dc.id = DescID(MT_EDIT_SELECTION);
		cln2->Message(MSG_DESCRIPTION_COMMAND,&dc);
	}
	
	SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
	for(i=0; i<cnt; i++)
	{
		pInd = mpadr[i].ind;
		if(pInd < pCnt)
		{
			Real t;
			Vector pt = padr[pInd];
			switch(axis)
			{
				case 0:
					t = CDBlend(-1.0,1.0,(pt.x-min.x)/(max.x-min.x));
					break;
				case 1:
					t = CDBlend(-1.0,1.0,(pt.y-min.y)/(max.y-min.y));
					break;
				case 2:
					t = CDBlend(-1.0,1.0,(pt.z-min.x)/(max.z-min.z));
					break;
			}
			Real scale = 0.0;
			if(t <= f && t >= -f) scale = CDBlend(1.0,0.0,(t-(-f))/(f-(-f)));
			else
			{
				if(t > f) scale = 0.0;
				else if(t < -f) scale = 1.0;
			}
			if(bs && scale > 0.0) bs->Select(pInd);
			c2padr[i].SetVector(mpadr[i].GetVector()*scale);
		}
	}
	if(bs)
	{
		dc.id = DescID(MT_SET_SELECTION);
		cln2->Message(MSG_DESCRIPTION_COMMAND,&dc);
		SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
	}
	
	doc->SetMode(mode);
	doc->SetActiveObject(trg);
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;
}

void CDMorphSplitDialog::DestroyWindow(void)
{
	mSplitDialogOpen = false;
	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
}

class CDMorphSplit : public CommandData
{
	private:
		CDMorphSplitDialog dlg;
		
	public:
		virtual Bool Execute(BaseDocument* doc);
		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDMORPHSPLIT,0,secret);
		}
	
};

Bool CDMorphSplit::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> tags; if (!tags) return false;
	doc->GetActiveTags(tags);
	LONG tCnt = tags->GetCount();
	
	BaseObject *trg = NULL;
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG oCnt = objects->GetCount();
	
	if(tCnt != 1)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return false;
	}
	
	BaseTag *mTag = static_cast<BaseTag*>(tags->GetIndex(0));
	if(!mTag || mTag->GetType() != ID_CDMORPHTAGPLUGIN)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return false;
	}
	
	if(oCnt < 1) trg = mTag->GetObject();
	else trg = static_cast<BaseObject*>(objects->GetIndex(0));
	
	if(!mTag || !trg) return false;

	BaseContainer *tData = mTag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tData->GetObjectLink(MT_DEST_LINK,doc); if(!op) return false;
	
#if API_VERSION < 12000
	if(!dlg.Open(true,ID_CDMORPHSPLIT,-1,-1)) return false;
#else
	if(!dlg.Open(DLG_TYPE_ASYNC,ID_CDMORPHSPLIT,-1,-1)) return false;
#endif
	
	return true;
}

class CDMorphSplitR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMorphSplitPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDM_SERIAL_SIZE];
	String cdmnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDMORPH,data,CDM_SERIAL_SIZE)) reg = false;
	
	cdmnr.SetCString(data,CDM_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdmnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdmnr.FindFirst("-",&pos);
	while(h)
	{
		cdmnr.Delete(pos,1);
		h = cdmnr.FindFirst("-",&pos);
	}
	cdmnr.ToUpper();
	kb = cdmnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDMSPLIT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMORPHSPLIT,name,PLUGINFLAG_HIDE,"CDMorphSplit.tif","CD Morph Split",CDDataAllocator(CDMorphSplitR));
	else return CDRegisterCommandPlugin(ID_CDMORPHSPLIT,name,0,"CDMorphSplit.tif","CD Morph Split",CDDataAllocator(CDMorphSplit));
}

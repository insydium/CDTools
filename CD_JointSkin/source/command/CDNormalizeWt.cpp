//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "oCDJoint.h"
#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "CDClusterTag.h"

#include "tCDCluster.h"

class CDNormalizeAllDialog : public GeModalDialog
{
	private:
		CDJSOptionsUA ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDNormalizeAllDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDNORMWT));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDC_MIN_WEIGHT),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddEditNumberArrows(IDC_MIN_WEIGHT,BFH_LEFT);
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDNormalizeAllDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDNORMALIZEALLWEIGHT);
	if(!wpData)
	{
		SetPercent(IDC_MIN_WEIGHT,0.0,0.0,100,0.01);
	}
	else
	{
		SetPercent(IDC_MIN_WEIGHT,wpData->GetReal(IDC_MIN_WEIGHT),0.0,100,0.01);
	}
		
	return true;
}

Bool CDNormalizeAllDialog::Command(LONG id,const BaseContainer &msg)
{
	Real minWt; 
	
	GetReal(IDC_MIN_WEIGHT,minWt);
	
	BaseContainer bc;
	bc.SetReal(IDC_MIN_WEIGHT,minWt);
	SetWorldPluginData(ID_CDNORMALIZEALLWEIGHT,bc,false);
	
	return true;
}

class CDNormalizeAllWeights : public CommandData
{
	private:
		CDNormalizeAllDialog dlg;

	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDNormalizeAllWeights::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_SKIN));
		return false;
	}
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}

	BaseContainer *wpData = GetWorldPluginData(ID_CDNORMALIZEALLWEIGHT);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetReal(IDC_MIN_WEIGHT,0.0);
		SetWorldPluginData(ID_CDNORMALIZEALLWEIGHT,bc,false);
		wpData = GetWorldPluginData(ID_CDNORMALIZEALLWEIGHT);
	}

	BaseObject *op=NULL;
	LONG i, opCount = objects->GetCount();
	
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op)
		{
			BaseTag *skin = op->GetTag(ID_CDSKINPLUGIN);
			if(skin)
			{
				Real minWt = wpData->GetReal(IDC_MIN_WEIGHT);
				skin->Message(CD_MSG_NORMALIZE_WEIGHT,&minWt);
			}
		}
	}
	
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;
}

class CDNormalizeAllWeightsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDNormalizeAllWeights(void)
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
	String name=GeLoadString(IDS_CDNORMWT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDNORMALIZEALLWEIGHT,name,PLUGINFLAG_HIDE,"CDNormalizeWt.tif","CD Normalize All Weights",CDDataAllocator(CDNormalizeAllWeightsR));
	else return CDRegisterCommandPlugin(ID_CDNORMALIZEALLWEIGHT,name,0,"CDNormalizeWt.tif","CD Normalize All Weights",CDDataAllocator(CDNormalizeAllWeights));
}

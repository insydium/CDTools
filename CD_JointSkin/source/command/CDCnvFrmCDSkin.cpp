//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "stdlib.h"

#include "c4d.h"
#include "c4d_symbols.h"
#include "customgui_inexclude.h"
#if API_VERSION > 9600
	#include "lib_ca.h"
#endif

#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"

#include "tCDSkin.h"
#if API_VERSION > 9600
	#include "tcaweight.h"
#endif

class CDConvertFromCDSkinDialog : public GeModalDialog
{
private:
	CDJSOptionsUA ua;
	
public:	
	LONG opt;
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDConvertFromCDSkinDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDCNVFRMCDSKIN));
		
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
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(DLG_CDJS_OPTIONS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_MIRROR_AXIS,BFH_CENTER,2,1);
				AddChild(IDC_MIRROR_AXIS, 0, GeLoadString(IDS_CAWEIGHTS));
				AddChild(IDC_MIRROR_AXIS, 1, GeLoadString(IDS_CBWEIGHTS));
				
			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}
	
	return res;
}

Bool CDConvertFromCDSkinDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDCONVERTFROMCDSKIN);
	if(!bc)
	{
		SetLong(IDC_MIRROR_AXIS,0);
	}
	else
	{
		SetLong(IDC_MIRROR_AXIS,bc->GetBool(IDC_MIRROR_AXIS));
	}
	
	return true;
}

Bool CDConvertFromCDSkinDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDC_MIRROR_AXIS,opt);
	
	BaseContainer bc;
	bc.SetLong(IDC_MIRROR_AXIS,opt);
	SetWorldPluginData(ID_CDCONVERTFROMCDSKIN,bc,false);
	
	return true;
}

class CDConvertFromCDSkin : public CommandData
{
private:
	CDConvertFromCDSkinDialog dlg;
	
	
#if API_VERSION < 12000
	Bool CDSkinToClaudeBonet(BaseDocument* doc);
#endif
	
#if API_VERSION >= 9800	
	void CDSetCAWeightMap(CAWeightTag *tag, CDSkinVertex *sknWt, LONG jInd, LONG pCnt);
	Bool CDAddJointToCAWeightTag(BaseTag *tag, BaseContainer *tData, CAWeightTag *wTag, BaseObject *jnt);
	Bool CDSkinToCAWeight(BaseDocument* doc);
#endif
	
public:
	virtual Bool Execute(BaseDocument* doc);

};

#if API_VERSION < 12000
Bool CDConvertFromCDSkin::CDSkinToClaudeBonet(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	doc->GetActiveObjects(objects,false);
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_SKIN));
		return false;
	}
	op = static_cast<BaseObject*>(objects->GetIndex(0));
	
	BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
	if(!skTag)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_SKIN));
		return false;
	}
	BaseContainer *skData = skTag->GetDataInstance();
	if(!IsValidPointObject(op)) return false;
	
	StatusSetSpin();
	doc->StartUndo();
	
	// Delete any previous Claude Bonet tags
	BaseTag *cbTag = NULL;
	while(op->GetTag(Tclaudebonet))
	{
		cbTag = op->GetTag(Tclaudebonet);
		BaseTag::Free(cbTag);
	}
	
	CDSkinVertex *skinWeight = CDGetSkinWeights(skTag);
	if(!skinWeight)
	{
		doc->EndUndo();
		StatusClear();
		return false;
	}

	cbTag = NULL;
	LONG i, j;
	LONG ptCount = ToPoint(op)->GetPointCount(), jCount = skData->GetLong(SKN_J_COUNT);
	BaseObject	*joint=NULL;
	VariableTag *cbvTag=NULL;
	BaseContainer *cbData=NULL;
	Real *cbWeight=NULL;
	if(ptCount > 0 && jCount > 0)
	{
		for(j=0; j<jCount; j++)
		{
			joint = skData->GetObjectLink(SKN_J_LINK+j,doc);
			cbvTag = op->MakeVariableTag(Tclaudebonet,ptCount,cbTag);
			if(cbvTag)
			{
				CDAddUndo(doc,CD_UNDO_NEW,cbvTag);
				cbData = cbvTag->GetDataInstance();
				cbData->SetLink(1000,joint);
				
			#if API_VERSION < 9800
				cbWeight = (Real *)cbvTag->GetDataAddress();
			#else
				cbWeight = (Real *)cbvTag->GetDataAddressW();
			#endif
				
				for(i=0; i<ptCount; i++)
				{
					cbWeight[i] = skinWeight[i].jw[j];
				}
				cbTag = static_cast<BaseTag*>(cbvTag);
			}
		}
	}
		
	doc->EndUndo();
	StatusClear();

	EventAdd(EVENT_FORCEREDRAW);

	return true;
}
#endif

#if API_VERSION >= 9800	
void CDConvertFromCDSkin::CDSetCAWeightMap(CAWeightTag *tag, CDSkinVertex *sknWt, LONG jInd, LONG pCnt)
{
	LONG i;
	for(i=0; i<pCnt; i++)
	{
		Real wt = 0.0;
		if(sknWt[i].taw > 0.0)
		{
			wt = TruncatePercentFractions(sknWt[i].jw[jInd] / sknWt[i].taw);
		}
		tag->SetWeight(jInd, i, wt);
	}
}

Bool CDConvertFromCDSkin::CDAddJointToCAWeightTag(BaseTag *tag, BaseContainer *tData, CAWeightTag *wTag, BaseObject *jnt)
{
#if API_VERSION < 12000
	if(!tag || !tData || !wTag || !jnt) return false;
	
	GeData jntData;
	if(CDGetParameter(tag,DescID(ID_CA_WEIGHT_TAG_JOINTS),jntData,0))
	{
		InExcludeData *jntList = (InExcludeData*)jntData.GetCustomDataType(CUSTOMDATATYPE_INEXCLUDE_LIST);
		if(jntList)
		{
			jntList->InsertObject(jnt,0);
			CDSetParameter(tag,DescID(ID_CA_WEIGHT_TAG_JOINTS),jntData,0);
		}
	}
#else
	if(!wTag || !jnt) return false;
	
	wTag->AddJoint(jnt);
#endif
	
	return true;
}

Bool CDConvertFromCDSkin::CDSkinToCAWeight(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
    
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_SKIN));
		return false;
	}
	op = static_cast<BaseObject*>(objects->GetIndex(0));
	
	BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
	if(!skTag)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_SKIN));
		return false;
	}
	if(!IsValidPointObject(op)) return false;
	
	BaseTag *skRef = op->GetTag(ID_CDSKINREFPLUGIN); if(!skRef) return false;
	
	BaseContainer *skData = skTag->GetDataInstance();
	if(skData)
	{
		CDSkinVertex *sknWt = CDGetSkinWeights(skTag);
		if(sknWt)
		{
			StatusSetSpin();
			doc->StartUndo();
			
			BaseTag *caTag = BaseTag::Alloc(Tweights);
			if(caTag)
			{
				op->InsertTag(caTag,NULL);
				CDAddUndo(doc,CD_UNDO_NEW,caTag);
				caTag->Message(MSG_MENUPREPARE);

				CAWeightTag *wTag = static_cast<CAWeightTag*>(caTag);
				if(wTag)
				{
					BaseContainer *caData = caTag->GetDataInstance();
					
					LONG i, pCnt = ToPoint(op)->GetPointCount(), jCnt = skData->GetLong(SKN_J_COUNT);
					for(i=0; i<jCnt; i++)
					{
						BaseObject *jnt = skData->GetObjectLink(SKN_J_LINK+i, doc);
						skData->SetLong(SKN_J_LINK_ID,i);
						
						CDAddJointToCAWeightTag(caTag, caData, wTag, jnt);
						CDSetCAWeightMap(wTag, sknWt, i, pCnt);
					}
					
					CDAddUndo(doc,CD_UNDO_CHANGE,caTag);
					DescriptionCommand wtdc;
					wtdc.id = DescID(ID_CA_WEIGHT_TAG_SET);
					caTag->Message(MSG_DESCRIPTION_COMMAND,&wtdc);
					
					BaseObject *skDef = BaseObject::Alloc(Oskin);
					if(skDef) doc->InsertObject(skDef,op,NULL);
					CDAddUndo(doc,CD_UNDO_NEW,skDef);
					
					CDAddUndo(doc,CD_UNDO_DELETE,skRef);
					BaseTag::Free(skRef);
					CDAddUndo(doc,CD_UNDO_DELETE,skTag);
					BaseTag::Free(skTag);
				}

			}
			doc->EndUndo();
			StatusClear();
			
			EventAdd(EVENT_FORCEREDRAW);
		}
	}
	
	return true;
}
#endif


Bool CDConvertFromCDSkin::Execute(BaseDocument* doc)
{
#if API_VERSION < 12000 && API_VERSION >= 9800
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}
	LONG convert = 0;
	BaseContainer *wpData = GetWorldPluginData(ID_CDCONVERTFROMCDSKIN);
	if(wpData) convert = wpData->GetLong(IDC_MIRROR_AXIS);
	
	switch(convert)
	{
		case 0:
			return CDSkinToCAWeight(doc);
		case 1:
			return CDSkinToClaudeBonet(doc);
	}
#endif	
	
#if API_VERSION < 9800
	return CDSkinToClaudeBonet(doc);
#endif
	
#if API_VERSION >= 12000
	return CDSkinToCAWeight(doc);
#endif
}

class CDConvertFromCDSkinR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDConvertFromCDSkin(void)
{
	if(CDFindPlugin(ID_CDCONVERTFROMCDSKIN,CD_COMMAND_PLUGIN)) return true;

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
	
#if API_VERSION < 12000
	GeGetSerialInfo(SERIAL_CINEMA4D,&si);
#else
	GeGetSerialInfo(SERIALINFO_CINEMA4D,&si);
#endif
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
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDCNVFRMCDSKIN); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCONVERTFROMCDSKIN,name,PLUGINFLAG_HIDE,"CDSkinToCB.tif","CD Convert From CD Skin",CDDataAllocator(CDConvertFromCDSkinR));
	else return CDRegisterCommandPlugin(ID_CDCONVERTFROMCDSKIN,name,0,"CDSkinToCB.tif","CD Convert From CD Skin",CDDataAllocator(CDConvertFromCDSkin));
}

//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "stdlib.h"

#include "c4d.h"
#include "c4d_symbols.h"

#if API_VERSION > 9600
	#include "lib_ca.h"
#endif

#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"

#include "tCDSkin.h"

class CDConvertToCDSkinDialog : public GeModalDialog
{
private:
	CDJSOptionsUA ua;
	
public:	
	LONG opt;
	
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDConvertToCDSkinDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	
	if(res)
	{
		SetTitle(GeLoadString(IDS_CDCNVTOCDSKIN));
		
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

Bool CDConvertToCDSkinDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDCONVERTTOCDSKIN);
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

Bool CDConvertToCDSkinDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDC_MIRROR_AXIS,opt);
	
	BaseContainer bc;
	bc.SetLong(IDC_MIRROR_AXIS,opt);
	SetWorldPluginData(ID_CDCONVERTTOCDSKIN,bc,false);
	
	return true;
}

class CDConvertToCDSkin : public CommandData
{
private:
	CDConvertToCDSkinDialog dlg;
	
#if API_VERSION < 12000
	Bool ClaudeBonetToCDSkin(BaseDocument* doc);
#endif
	
#if API_VERSION >= 9800	
	void CDGetCAWeightMap(CAWeightTag *tag, LONG jInd, Real *wt, LONG pCnt);
	Bool CAWeightToCDSkin(BaseDocument* doc);
#endif
	
public:
	virtual Bool Execute(BaseDocument* doc);

};

#if API_VERSION < 12000
Bool CDConvertToCDSkin::ClaudeBonetToCDSkin(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op=NULL, *joint=NULL;
	BaseTag *opTag=NULL;
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	doc->GetActiveObjects(objects,false);
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_CB));
		return false;
	}
	op = static_cast<BaseObject*>(objects->GetIndex(0)); if(!op) return false;
	if(!IsValidPointObject(op)) return false;

	
	LONG cbCnt = 0;
	for (opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
	{
		if(opTag && opTag->GetType() == Tclaudebonet)
		{
			cbCnt++;
		}
	}
	if(cbCnt > 0)
	{
		CDArray<Bool> addJoint;
		addJoint.Alloc(cbCnt);
		
		//GePrint("Start Conversion");
		StatusSetSpin();
		doc->StartUndo();
		
		BaseTag *skinRef = op->GetTag(ID_CDSKINREFPLUGIN);
		if(skinRef) BaseTag::Free(skinRef);
		skinRef = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
		op->InsertTag(skinRef,NULL);
		CDAddUndo(doc,CD_UNDO_NEW,skinRef);
		op->Message(MSG_UPDATE);
		DescriptionCommand skrdc;
		skrdc.id = DescID(SKR_SET_REFERENCE);
		skinRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);

		BaseTag *cdsTag=NULL;
		cdsTag = BaseTag::Alloc(ID_CDSKINPLUGIN);

		BaseTag *refTag = op->GetTag(ID_CDMORPHREFPLUGIN);
		if(refTag) op->InsertTag(cdsTag,refTag);
		else op->InsertTag(cdsTag,NULL);
		cdsTag->Message(MSG_MENUPREPARE);
		CDAddUndo(doc,CD_UNDO_NEW,cdsTag);
	
		BaseContainer *skData = cdsTag->GetDataInstance();
		if(skData)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,cdsTag);
			skData->SetLink(SKN_DEST_ID,op);
			skData->SetLong(SKN_J_COUNT,cbCnt);
			skData->SetLong(SKN_J_LINK_ID,0);
			
			LONG i, jCnt = 0, jAddCnt = 0, ptCount = ToPoint(op)->GetPointCount();
			BaseContainer *cbData = NULL;
			//Set the joints links from the CB tags
			VariableTag *cbTag = NULL;
			Real *cbWeight=NULL;
			for (opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
			{
				if(opTag && opTag->GetType() == Tclaudebonet && jCnt < cbCnt)
				{
					cbData = opTag->GetDataInstance();
					if(cbData)
					{
						joint = cbData->GetObjectLink(1000,doc);
						if(joint)
						{
							addJoint[jCnt] = false;
							cbTag = static_cast<VariableTag*>(opTag);
							if(cbTag)
							{
							#if API_VERSION < 9800
								cbWeight = (Real *)cbTag->GetDataAddress();
							#else
								cbWeight = (Real *)cbTag->GetDataAddressW();
							#endif
								
								if(cbWeight)
								{
									for(i=0; i<ptCount; i++)
									{
										if(i < cbTag->GetDataCount())
										{
											if(cbWeight[i] > 0.01) addJoint[jCnt] = true;
										}
									}
									if(addJoint[jCnt])
									{
										skData->SetLink(SKN_J_LINK+jAddCnt,joint);
										skData->SetLink(SKN_J_LINK_ID,joint);
										jAddCnt++;
									}
								}
							}
							jCnt++;
						}
					}
				}
			}
			
			// Initialize skin tag's weight array
			cdsTag->Message(MSG_MENUPREPARE);
			
			CDSkinVertex *skinWeight = CDGetSkinWeights(cdsTag);
			if(skinWeight)
			{
				//Set the Skin weights from the CB tags
				jCnt = 0;
				jAddCnt = 0;
				cbTag = NULL;
				cbWeight=NULL;
				for (opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
				{
					if(opTag && opTag->GetType() == Tclaudebonet && jCnt < cbCnt)
					{
						cbData = opTag->GetDataInstance();
						joint = cbData->GetObjectLink(1000,doc);
						if(joint)
						{
							if(addJoint[jCnt])
							{
								cbTag = static_cast<VariableTag*>(opTag);
								if(cbTag)
								{
								#if API_VERSION < 9800
									cbWeight = (Real *)cbTag->GetDataAddress();
								#else
									cbWeight = (Real *)cbTag->GetDataAddressW();
								#endif
									
									if(cbWeight)
									{
										for(i=0; i<ptCount; i++)
										{
											skinWeight[i].jw[jAddCnt] = cbWeight[i];
										}
									}
								}
								jAddCnt++;
							}
							jCnt++;
						}
					}
				}
				DescriptionCommand dc;
				dc.id = DescID(SKN_ACCUMULATE);
				cdsTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
			}
		}
		
		addJoint.Free();
		doc->EndUndo();
		StatusClear();
	}
	else
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_CB));
		return false;
	}

	EventAdd(EVENT_FORCEREDRAW);

	return true;
}
#endif

#if API_VERSION >= 9800	
void CDConvertToCDSkin::CDGetCAWeightMap(CAWeightTag *tag, LONG jInd, Real *wt, LONG pCnt)
{
#if API_VERSION < 12000
	tag->GetWeightMap(jInd, wt, pCnt);
#else
	SReal *map = (SReal*)CDAlloc<SReal>(pCnt);
	
	tag->GetWeightMap(jInd, map, pCnt);
	
	LONG i;
	for(i=0; i<pCnt; i++)
	{
		wt[i] = map[i];
	}
	
	CDFree(map);
#endif
}

Bool CDConvertToCDSkin::CAWeightToCDSkin(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
		
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
    
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_CA));
		return false;
	}
	
	BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(0)); if(!op) return false;
	if(!IsValidPointObject(op)) return false;
	LONG pCnt = ToPoint(op)->GetPointCount();
	
	BaseTag *caTag = op->GetTag(Tweights);
	if(!caTag)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_CA));
		return false;
	}
	
	CAWeightTag *wTag = static_cast<CAWeightTag*>(caTag);
	if(wTag)
	{
		StatusSetSpin();
		doc->StartUndo();
		
		LONG i, jCnt = wTag->GetJointCount();

		BaseTag *skinRef = op->GetTag(ID_CDSKINREFPLUGIN);
		if(skinRef) BaseTag::Free(skinRef);
		skinRef = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
		op->InsertTag(skinRef,NULL);
		CDAddUndo(doc,CD_UNDO_NEW,skinRef);
		
		op->Message(MSG_UPDATE);
		DescriptionCommand skrdc;
		skrdc.id = DescID(SKR_SET_REFERENCE);
		skinRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
		
		BaseTag *cdsTag = BaseTag::Alloc(ID_CDSKINPLUGIN);
		BaseContainer *skData = cdsTag->GetDataInstance();
		if(skData)
		{
			skData->SetLink(SKN_DEST_ID,op);
			skData->SetLong(SKN_J_COUNT,jCnt);
			skData->SetLong(SKN_J_LINK_ID,0);
			
			BaseTag *refTag = op->GetTag(ID_CDMORPHREFPLUGIN);
			if(refTag) op->InsertTag(cdsTag,refTag);
			else op->InsertTag(cdsTag,NULL);
			cdsTag->Message(MSG_MENUPREPARE);
			CDAddUndo(doc,CD_UNDO_NEW,cdsTag);
			
			CDSkinVertex *sknWt = CDGetSkinWeights(cdsTag);
			if(sknWt)
			{
				CDArray<Real> tWeight;
				tWeight.Alloc(pCnt);
				for(i=0; i<jCnt; i++)
				{
					BaseObject *jnt = wTag->GetJoint(i,doc);
					skData->SetLink(SKN_J_LINK+i,jnt);
					skData->SetLong(SKN_J_LINK_ID,i);
					
					CDGetCAWeightMap(wTag, i, tWeight.Array(), pCnt);
					CDSetJointWeight(cdsTag, doc, skData, tWeight.Array(), pCnt);
				}
				tWeight.Free();
				
				CDAddUndo(doc,CD_UNDO_CHANGE,cdsTag);
				DescriptionCommand skdc;
				skdc.id = DescID(SKN_BIND_SKIN);
				cdsTag->Message(MSG_DESCRIPTION_COMMAND,&skdc);
				
				CDAddUndo(doc,CD_UNDO_DELETE,caTag);
				BaseTag::Free(caTag);
				
				BaseObject *ch = op->GetDown();
				while(ch)
				{
					if(ch->GetType() == Oskin)
					{
						CDAddUndo(doc,CD_UNDO_DELETE,ch);
						BaseObject::Free(ch);
						break;
					}
					ch = ch->GetNext();
				}
			}
		}
		else
		{
			BaseTag::Free(skinRef);
			BaseTag::Free(cdsTag);
			return false;
		}
		
		doc->EndUndo();
		StatusClear();

	}
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;
}
#endif

Bool CDConvertToCDSkin::Execute(BaseDocument* doc)
{
#if API_VERSION < 12000 && API_VERSION >= 9800
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}
	LONG convert = 0;
	BaseContainer *wpData = GetWorldPluginData(ID_CDCONVERTTOCDSKIN);
	if(wpData) convert = wpData->GetLong(IDC_MIRROR_AXIS);

	switch(convert)
	{
		case 0:
			return CAWeightToCDSkin(doc);
		case 1:
			return ClaudeBonetToCDSkin(doc);
	}
#endif	
	
#if API_VERSION < 9800
	return ClaudeBonetToCDSkin(doc);
#endif
	
#if API_VERSION >= 12000
	return CAWeightToCDSkin(doc);
#endif
}

class CDConvertToCDSkinR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDConvertToCDSkin(void)
{
	if(CDFindPlugin(ID_CDCONVERTTOCDSKIN,CD_COMMAND_PLUGIN)) return true;

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
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name = GeLoadString(IDS_CDCNVTOCDSKIN); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCONVERTTOCDSKIN,name,PLUGINFLAG_HIDE,"CDCBToSkin.tif","CD Convert To CD Skin",CDDataAllocator(CDConvertToCDSkinR));
	else return CDRegisterCommandPlugin(ID_CDCONVERTTOCDSKIN,name,0,"CDCBToSkin.tif","CD Convert To CD Skin",CDDataAllocator(CDConvertToCDSkin));
}

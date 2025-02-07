//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"
#include "customgui_inexclude.h"

//#include "CDCompatibility.h"
#include "tCDMorphMix.h"
#include "CDMorph.h"
#include "CDTagData.h"

#define MAX_ADD_OFFSET		100

// from CD Morph tag
enum
{
	CDM_MIX_SLIDER				= 1018,
	CDM_USE_BONE_ROTATION		= 1020
	
};
enum
{
	//MM_PURCHASE					= 1000,
	MM_DISK_LEVEL					= 1001,
	MM_TAG_COUNT					= 1002,
	MM_T_LINK_COUNT				= 1003,
	
	//MM_LIST_GROUP				= 1020,
	//MM_MIXER_GROUP				= 1021,
	
	//MM_M_TAG_LIST				= 2000,
	//MM_EDIT_M_TAG				= 2001,
	
	//MM_MIX_SLIDER				= 3000,
	MM_TAG_LINK					= 4000,
	MM_TAG_ID						= 5000
};

class CDMorphMixPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Bool TagInLink(BaseDocument *doc, BaseContainer *tData, BaseTag *mTag);
	Bool TagInList(BaseDocument *doc, BaseTag *mTag, InExcludeData *mTagList);
	
	LONG GetEmptyLinkID(BaseDocument *doc, BaseContainer *tData);
	LONG GetTagSliderID(BaseDocument *doc, BaseContainer *tData, BaseTag *mTag);
	
	void RemoveTagFromLink(BaseTag *mTag, BaseDocument *doc, BaseContainer *tData);
	void AppendTagList(BaseDocument *doc, BaseContainer *tData, InExcludeData *mTagList);
	void RemoveTagsFromList(BaseDocument *doc, BaseContainer *tData, InExcludeData *mTagList);
	void RemoveAnimationTracks(BaseTag *tag, BaseDocument *doc, BaseContainer *tData);
	
	void ConfirmXListItems(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, InExcludeData *mTagList);
	void FlushOldData(BaseTag *tag, BaseContainer *tData);

public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
		
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDMorphMixPlugin); }
};

Bool CDMorphMixPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer		*tData = tag->GetDataInstance();
	
	tData->SetLong(MM_TAG_COUNT,0);
	tData->SetLong(MM_T_LINK_COUNT,0);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
			pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_ANIMATION);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	CDMData mxd;
	PluginMessage(ID_CDMORPHMIXPLUGIN,&mxd);
	if(mxd.list) mxd.list->Append(node);
	
	return true;
}

void CDMorphMixPlugin::Free(GeListNode *node)
{
	CDMData mxd;
	PluginMessage(ID_CDMORPHMIXPLUGIN,&mxd);
	if(mxd.list) mxd.list->Remove(node);
}

Bool CDMorphMixPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(MM_DISK_LEVEL,level);
	
	return true;
}

void CDMorphMixPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdmnr = tData->GetString(T_STR);
	SerialInfo si;
	
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
			tData->SetLink(T_OID,op);
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDMorphMixPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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

	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDMorphMixPlugin::FlushOldData(BaseTag *tag, BaseContainer *tData)
{
	tData->FlushAll();
	
	tData->SetLong(MM_TAG_COUNT,0);
	tData->SetLong(MM_T_LINK_COUNT,0);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if(CDGetParameter(tag,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
			pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_INITIAL);
		}
		CDSetParameter(tag,DescLevel(EXPRESSION_PRIORITY),d);
	}
}

void CDMorphMixPlugin::RemoveTagFromLink(BaseTag *mTag, BaseDocument *doc, BaseContainer *tData)
{
	LONG i, cnt = tData->GetLong(MM_T_LINK_COUNT), tCnt = tData->GetLong(MM_TAG_COUNT);
	for(i=0; i<cnt; i++)
	{
		if(mTag == tData->GetLink(MM_TAG_LINK+i,doc))
		{
			tData->SetLink(MM_TAG_LINK+i,NULL);
			tCnt--;
		}
	}
	tData->SetLong(MM_TAG_COUNT,tCnt);
}

Bool CDMorphMixPlugin::TagInLink(BaseDocument *doc, BaseContainer *tData, BaseTag *mTag)
{
	LONG i, cnt = tData->GetLong(MM_T_LINK_COUNT);
	
	for(i=0; i<cnt; i++)
	{
		if(mTag == tData->GetLink(MM_TAG_LINK+i,doc)) return true;
	}
	
	return false;
}

Bool CDMorphMixPlugin::TagInList(BaseDocument *doc, BaseTag *mTag, InExcludeData *mTagList)
{
	LONG i, cnt = mTagList->GetObjectCount();
	
	for(i=0; i<cnt; i++)
	{
		if(mTag == mTagList->ObjectFromIndex(doc,i)) return true;
	}
	
	return false;
}

LONG CDMorphMixPlugin::GetEmptyLinkID(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, tID, cnt = tData->GetLong(MM_T_LINK_COUNT);
	
	tID = cnt;
	for(i=0; i<cnt; i++)
	{
		if(!tData->GetLink(MM_TAG_LINK+i,doc))
		{
			tID = i;
			break;
		}
	}
	
	return tID;
}

void CDMorphMixPlugin::RemoveAnimationTracks(BaseTag *tag, BaseDocument *doc, BaseContainer *tData)
{
	LONG i, lnkCnt = tData->GetLong(MM_T_LINK_COUNT);
	
	for(i=0; i<lnkCnt; i++)
	{
		BaseTag *mTag = (BaseTag*)tData->GetLink(MM_TAG_LINK+i,doc);
		if(!mTag)
		{
			DescID dscID = DescID(DescLevel(MM_MIX_SLIDER+i,DTYPE_REAL,0));
			CDDeleteAnimationTrack(doc,tag,dscID);
		}
	}
}

void CDMorphMixPlugin::RemoveTagsFromList(BaseDocument *doc, BaseContainer *tData, InExcludeData *mTagList)
{
	LONG i, lnkCnt = tData->GetLong(MM_T_LINK_COUNT);
	LONG tCnt = tData->GetLong(MM_TAG_COUNT);
	
	for(i=0; i<lnkCnt; i++)
	{
		BaseTag *mTag = (BaseTag*)tData->GetLink(MM_TAG_LINK+i,doc);
		if(mTag)
		{
			if(!TagInList(doc,mTag,mTagList))
			{
				tData->SetLink(MM_TAG_LINK+i,NULL);
				tCnt--;
			}
		}
	}
	tData->SetLong(MM_TAG_COUNT,tCnt);
}

void CDMorphMixPlugin::AppendTagList(BaseDocument *doc, BaseContainer *tData, InExcludeData *mTagList)
{
	LONG i, xCnt = mTagList->GetObjectCount();
	LONG lnkCnt = tData->GetLong(MM_T_LINK_COUNT);
	LONG tCnt = tData->GetLong(MM_TAG_COUNT);
	
	for(i=0; i<xCnt; i++)
	{
		BaseTag *mTag = (BaseTag*)mTagList->ObjectFromIndex(doc, i);
		if(mTag)
		{
			if(!TagInLink(doc,tData,mTag))
			{
				LONG tID = GetEmptyLinkID(doc,tData);
				tData->SetLink(MM_TAG_LINK+tID,mTag);
				tCnt++;
				if(tID >= lnkCnt) lnkCnt++;
			}
		}
	}
	tData->SetLong(MM_TAG_COUNT,tCnt);
	tData->SetLong(MM_T_LINK_COUNT,lnkCnt);
}

void CDMorphMixPlugin::ConfirmXListItems(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, InExcludeData *mTagList)
{
	LONG i, xCnt = mTagList->GetObjectCount();
	LONG tCnt = tData->GetLong(MM_TAG_COUNT);
	
	for(i=0; i<xCnt; i++)
	{
		BaseTag *mTag = (BaseTag*)mTagList->ObjectFromIndex(doc, i);
		if(!mTag)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			mTagList->DeleteObject(i);
			tCnt--;
			tData->SetLong(MM_TAG_COUNT,tCnt);
			RemoveAnimationTracks(tag,doc,tData);
		}
		else
		{
			BaseContainer *mtData = mTag->GetDataInstance();
			if(mtData)
			{
				if(mtData->GetBool(CDM_USE_BONE_ROTATION))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					mTagList->DeleteObject(i);
					RemoveTagFromLink(mTag,doc,tData);
					RemoveAnimationTracks(tag,doc,tData);
				}
				else
				{
					if(!TagInLink(doc,tData,mTag))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						AppendTagList(doc,tData,mTagList);
					}
				}
			}
		}
	}
	
}

Bool CDMorphMixPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag	*tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdmnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdmnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdmnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdmnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdmnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdmnr);
			break;
		}
	}
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	CheckTagReg(doc,op,tData);

	InExcludeData *mTagList = (InExcludeData*)tData->GetCustomDataType(MM_M_TAG_LIST,CUSTOMDATATYPE_INEXCLUDE_LIST); 

	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			if(tData->GetLong(MM_DISK_LEVEL) < 1) FlushOldData(tag,tData);
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			if(mTagList) ConfirmXListItems(tag,doc,tData,mTagList);
			break;
		}
		case CD_MSG_UPDATE:
		{
			if(mTagList) ConfirmXListItems(tag,doc,tData,mTagList);
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==MM_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==MM_EDIT_M_TAG)
			{
				if(mTagList)
				{
					LONG i, xCnt = mTagList->GetObjectCount();
					for(i=0; i<xCnt; i++)
					{
						BaseContainer *sData = mTagList->GetData(i);
						if(sData)
						{
							if(sData->GetBool(IN_EXCLUDE_DATA_SELECTION))
							{
								BaseTag *mTag = (BaseTag*)mTagList->ObjectFromIndex(doc, i);
								if(mTag)
								{
									doc->SetActiveTag(mTag);
									break;
								}
							}
						}
					}
				}
			}
			break;
		}
	}

	return true;
}

LONG CDMorphMixPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	InExcludeData *mTagList = (InExcludeData*)tData->GetCustomDataType(MM_M_TAG_LIST,CUSTOMDATATYPE_INEXCLUDE_LIST); 
	if(!mTagList) return false;

	LONG i, xCnt = mTagList->GetObjectCount();
	for (i=0; i<xCnt; i++)
	{
		BaseTag *mTag = (BaseTag*)mTagList->ObjectFromIndex(doc, i);
		if(mTag)
		{
			BaseContainer *mtData = mTag->GetDataInstance();
			if(mtData)
			{
				if(!mtData->GetBool(CDM_USE_BONE_ROTATION))
				{
					LONG sID = GetTagSliderID(doc,tData,mTag);
					mtData->SetReal(CDM_MIX_SLIDER,tData->GetReal(MM_MIX_SLIDER+sID));
				}
			}
		}
	}
	return CD_EXECUTION_RESULT_OK;
}

LONG CDMorphMixPlugin::GetTagSliderID(BaseDocument *doc, BaseContainer *tData, BaseTag *mTag)
{
	LONG i, sID=0, cnt = tData->GetLong(MM_T_LINK_COUNT);
	
	for(i=0; i<cnt; i++)
	{
		if(mTag == tData->GetLink(MM_TAG_LINK+i,doc))
		{
			sID = i;
		}
	}
	
	return sID;
}

Bool CDMorphMixPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;

	if(!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(MM_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	InExcludeData *mTagList = (InExcludeData*)tData->GetCustomDataType(MM_M_TAG_LIST,CUSTOMDATATYPE_INEXCLUDE_LIST); 
	if(mTagList)
	{
		LONG i, xCnt = mTagList->GetObjectCount();
		for (i=0; i<xCnt; i++)
		{
			BaseTag *mTag = (BaseTag*)mTagList->ObjectFromIndex(doc, i);
			if(mTag)
			{
				String mixName = mTag->GetName();
				LONG sID = GetTagSliderID(doc,tData,mTag);

				BaseContainer bcMix = GetCustomDataTypeDefault(DTYPE_REAL);
				bcMix.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bcMix.SetString(DESC_NAME,mixName);
				bcMix.SetReal(DESC_MINSLIDER,0.0);
				bcMix.SetReal(DESC_MAXSLIDER,1.0);
				bcMix.SetReal(DESC_STEP,0.01);
				bcMix.SetLong(DESC_UNIT,DESC_UNIT_PERCENT);
				bcMix.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bcMix.SetBool(DESC_REMOVEABLE,true);
				if(!description->SetParameter(DescLevel(MM_MIX_SLIDER+sID, DTYPE_REAL, 0), bcMix, DescLevel(MM_MIXER_GROUP))) return false;
			}
		}
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDMorphMixPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case MM_M_TAG_LIST:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MM_EDIT_M_TAG:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	
	return true;	
}

Bool RegisterCDMorphMixPlugin(void)
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
	String name=GeLoadString(IDS_CDMORPHMIX); if(!name.Content()) return true;

	return CDRegisterTagPlugin(ID_CDMORPHMIXPLUGIN,name,info,CDMorphMixPlugin::Alloc,"tCDMorphMix","CDMorphMix.tif",1);
}

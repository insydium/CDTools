//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDTransSelTag.h"

Bool CDTransferSelectedTag::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(TRNSEL_TRANSFER_ON,false);
	tData->SetBool(TRNSEL_SEL_TRANS,false);
	tData->SetBool(TRNSEL_TOGGLE_ON,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	tData->SetLong(TRNSEL_BUTTON_SELECT,TRNSEL_BUTTON);
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDTRData trnsD;
	PluginMessage(ID_CDTRANSELECTEDTAG,&trnsD);
	if(trnsD.list) trnsD.list->Append(node);

	return true;
}

void CDTransferSelectedTag::Free(GeListNode *node)
{
	CDTRData trnsD;
	PluginMessage(ID_CDTRANSELECTEDTAG,&trnsD);
	if(trnsD.list) trnsD.list->Remove(node);
}

void CDTransferSelectedTag::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdtnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
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
			
			tData->SetLink(TRNSEL_TARGET+T_LST,tData->GetLink(TRNSEL_TARGET,doc));
			
			tData->SetBool(T_SET,true);
		}
	}
}

LONG CDTransferSelectedTag::GetButtonID(BaseContainer *tData, BaseList2D *trg)
{
	LONG buttonID;
	
	AutoAlloc<Description> desc;
	CDGetDescription(trg,desc,CD_DESCFLAGS_DESC_0);

	void* h = desc->BrowseInit();
	const BaseContainer *descBC = NULL;
	DescID id, groupid, srcID;
	
	Bool skip = true;
	LONG i=0;
	while(desc->GetNext(h, &descBC, id, groupid))
	{
		if(id == DescID(ID_BASELIST_NAME)) skip = false;
		if(skip) continue;
		
		if(id[0].dtype == DTYPE_BUTTON)
		{
			i++;
			if(tData->GetLong(TRNSEL_BUTTON_SELECT) == TRNSEL_BUTTON+i)
			{
				buttonID = id[0].id;
				break;
			}
		}
	}
	desc->BrowseFree(h);
	
	return buttonID;
}

Bool CDTransferSelectedTag::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDTR_SERIAL_SIZE];
			String cdtnr;
			
			CDReadPluginInfo(ID_CDTRANSFERTOOLS,snData,CDTR_SERIAL_SIZE);
			cdtnr.SetCString(snData,CDTR_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdtnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDTR_SERIAL_SIZE];
			String cdtnr;
			
			CDReadPluginInfo(ID_CDTRANSFERTOOLS,snData,CDTR_SERIAL_SIZE);
			cdtnr.SetCString(snData,CDTR_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdtnr);
			break;
		}
	}
	BaseDocument *doc = tag->GetDocument(); if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	BaseList2D *dest = tData->GetLink(TRNSEL_TARGET,doc);
	
	switch(type)
	{
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == TRNSEL_TRANSFER_ON) tData->SetBool(TRNSEL_TOGGLE_ON,true);
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==TRNSEL_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
		case CD_MSG_UPDATE:
		{
			CDTrnSel *trnSel = (CDTrnSel*)data;
			
			if(dest == op) break;
			
			if(tData->GetBool(TRNSEL_TRANSFER_ON) && !tData->GetBool(TRNSEL_TOGGLE_ON))
			{
				tData->SetBool(TRNSEL_SEL_TRANS,true);
				BaseObject *setOp = tData->GetObjectLink(T_OID,doc);
				if(setOp != op)
				{
					doc->SetActiveObject(op,SELECTION_SUB);
					if(dest)
					{
						if(tData->GetBool(TRNSEL_USE_BUTTON))
						{
							LONG button = GetButtonID(tData, dest);
							DescriptionCommand dc;
							dc.id = DescID(button);
							dest->Message(MSG_DESCRIPTION_COMMAND,&dc);
							EventAdd(EVENT_FORCEREDRAW);
						}
						else
						{
							if(dest->IsInstanceOf(Obase))
							{
								if(dest->GetType() != Oselection)
								{
									doc->SetActiveObject(static_cast<BaseObject*>(dest),SELECTION_ADD);
								}
							}
							else if(dest->IsInstanceOf(Tbase))
							{
								doc->SetActiveTag(static_cast<BaseTag*>(dest),SELECTION_ADD);
							}
						}
					}
					tData->SetLink(T_OID,op);
				}
				else
				{
					trnSel->trn = true;
				}
			}
			else if(tData->GetBool(TRNSEL_TRANSFER_ON) && tData->GetBool(TRNSEL_TOGGLE_ON))
			{
				tData->SetBool(TRNSEL_SEL_TRANS,true);
				tData->SetBool(TRNSEL_TOGGLE_ON,false);
				doc->SetActiveObject(op,SELECTION_SUB);
				if(dest)
				{
					if(tData->GetBool(TRNSEL_USE_BUTTON))
					{
						LONG button = GetButtonID(tData, dest);
						DescriptionCommand dc;
						dc.id = DescID(button);
						dest->Message(MSG_DESCRIPTION_COMMAND,&dc);
						EventAdd(EVENT_FORCEREDRAW);
					}
					else
					{
						if(dest->IsInstanceOf(Obase))
						{
							if(dest->GetType() != Oselection)
							{
								doc->SetActiveObject(static_cast<BaseObject*>(dest),SELECTION_ADD);
							}
						}
						else if(dest->IsInstanceOf(Tbase))
						{
							doc->SetActiveTag(static_cast<BaseTag*>(dest),SELECTION_ADD);
						}
					}
				}
			}
			break;
		}
	}
	
	return true;
}

LONG CDTransferSelectedTag::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	if(!tData->GetBool(T_REG))
	{
		if(!tData->GetBool(T_SET)) return false;
		
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
		if(tagMoved || tData->GetBool(T_MOV))
		{
			tData->SetBool(TRNSEL_TRANSFER_ON, false);
			return false;
		}
		
		tData->SetLink(TRNSEL_TARGET,tData->GetLink(TRNSEL_TARGET+T_LST,doc));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDTransferSelectedTag::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	BaseDocument *doc = tag->GetDocument();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(TRNSEL_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	bc = description->GetParameterI(DescLevel(TRNSEL_BUTTON_SELECT), ar);
	
	BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_LONG);
	bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
	
	BaseContainer cycle;
	LONG i = 0;
	cycle.SetString(TRNSEL_BUTTON+i,GeLoadString(CDTR_NONE));
	i++;
	
	BaseList2D *trg = tData->GetLink(TRNSEL_TARGET,doc);
	if(trg)
	{
		AutoAlloc<Description> desc;
		
		CDGetDescription(trg,desc,CD_DESCFLAGS_DESC_0);
		void* h = desc->BrowseInit();
		const BaseContainer *descBC = NULL;
		DescID id, groupid, srcID;
		
		Bool skip = true;
		while(desc->GetNext(h, &descBC, id, groupid))
		{
			if(id == DescID(ID_BASELIST_NAME)) skip = false;
			if(skip) continue;
			
			if(id[0].dtype == DTYPE_BUTTON)
			{
				cycle.SetString(TRNSEL_BUTTON+i,descBC->GetString(DESC_NAME));
				i++;
			}
		}
		desc->BrowseFree(h);
	}
	
	bc1.SetContainer(DESC_CYCLE, cycle);
	if(!description->SetParameter(DescLevel(TRNSEL_BUTTON_SELECT, DTYPE_LONG, 0), bc1, DescLevel(ID_TAGPROPERTIES))) return true;
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDTransferSelectedTag::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = tag->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case TRNSEL_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TRNSEL_USE_BUTTON:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetLink(TRNSEL_TARGET,doc)) return false;
				else return true;
			}
		case TRNSEL_BUTTON_SELECT:
			if(tData->GetBool(TRNSEL_USE_BUTTON)) return true;
			else return false;
	}
	return true;
}

Bool RegisterCDTransferSelectedTag(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDTR_SERIAL_SIZE];
	String cdtnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = false;
	
	cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
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
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_TRANSELTAG); if (!name.Content()) return true;

	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;

	return CDRegisterTagPlugin(ID_CDTRANSELECTEDTAG,name,info,CDTransferSelectedTag::Alloc,"tCDTranSel","CDTranSel.tif",0);
}

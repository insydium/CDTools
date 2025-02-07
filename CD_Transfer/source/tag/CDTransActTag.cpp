//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch


#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "tCDTranAct.h"
#include "CDTransfer.h"

enum
{
	//TRANSFER_ON				= 1000,
	//TARGET					= 1001,
	//PURCHASE					= 1002,

	TOGGLE_ON				= 2001,
	
	//Reserve 500000 - 500099 for runtime
	T_REG					= 500000,
	T_SET					= 500001,
	T_MOV					= 500002,
	T_OID					= 500003,
	T_STR					= 500004
};

class CDTransferActivatedTag : public TagData
{
	private:
		void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
		Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
		
		Bool Execution(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
		
	public:
		virtual Bool Init(GeListNode *node);
		virtual void Free(GeListNode *node);
		virtual Bool Message(GeListNode *node, LONG type, void *data);

	#if API_VERSION < 12000
		virtual LONG Execute(PluginTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
		virtual Bool GetDDescription(GeListNode *node, Description *description, LONG &flags);
		virtual Bool GetDEnabling(GeListNode *node, const DescID &id,GeData &t_data,LONG flags,const BaseContainer *itemdesc);
	#else
		virtual EXECUTIONRESULT Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, LONG priority, EXECUTIONFLAGS flags);
		virtual Bool GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC &flags);
		virtual Bool GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc);
	#endif

		static NodeData *Alloc(void) { return gNew CDTransferActivatedTag; }
};

Bool CDTransferActivatedTag::Init(GeListNode *node)
{
	//GePrint("CDTransferActivatedTag::Init");
	
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(TRANSFER_ON,FALSE);
	tData->SetBool(TOGGLE_ON,FALSE);
	
	tData->SetBool(T_REG,FALSE);
	tData->SetBool(T_SET,FALSE);
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,FALSE);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDTRData trnaD;
	PluginMessage(ID_CDTRANSACTIVATEDTAG,&trnaD);
	if(trnaD.list) trnaD.list->Append(node);

	return TRUE;
}

void CDTransferActivatedTag::Free(GeListNode *node)
{
	CDTRData trnaD;
	PluginMessage(ID_CDTRANSACTIVATEDTAG,&trnaD);
	if(trnaD.list) trnaD.list->Remove(node);
}

void CDTransferActivatedTag::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = TRUE;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdtnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdtnr)) reg = FALSE;
	
#if API_VERSION < 12000
	GeGetSerialInfo(SERIAL_CINEMA4D,&si);
#else
	GeGetSerialInfo(SERIALINFO_CINEMA4D,&si);
#endif
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
	if(kb != s) reg = FALSE;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = TRUE;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & R11_VERSION_SAVABLEDEMO) reg = TRUE;
		if(GeGetVersionType() & R11_VERSION_SAVABLEDEMO_ACTIVE) reg = FALSE;
	}
	
	if(GeGetVersionType() & VERSION_NET) reg = TRUE;
	if(GeGetVersionType() & VERSION_SERVER) reg = TRUE;
#else
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO) reg = TRUE;
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = FALSE;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = TRUE;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = TRUE;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = TRUE;
#endif
	
	tData->SetBool(T_REG,reg);
	if(!reg)
	{
		if(!tData->GetBool(T_SET))
		{
			tData->SetLink(T_OID,op);
			
			tData->SetLink(TARGET+510000,tData->GetLink(TARGET,doc));
			//tData->SetBool(TRANSFER_ON+510000,tData->GetBool(TRANSFER_ON));
			
			tData->SetBool(T_SET,TRUE);
		}
	}
}

Bool CDTransferActivatedTag::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return TRUE;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return TRUE;
	BaseObject *op = tag->GetObject(); if(!op) return TRUE;

	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,FALSE);
			CHAR snData[CDTR_SERIAL_SIZE];
			String cdtnr;
			
			CDReadPluginInfo(ID_CDTRANSFERTOOLS,snData,CDTR_SERIAL_SIZE);
			cdtnr.SetCString(snData,CDTR_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdtnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,FALSE);
			CHAR snData[CDTR_SERIAL_SIZE];
			String cdtnr;
			
			CDReadPluginInfo(ID_CDTRANSFERTOOLS,snData,CDTR_SERIAL_SIZE);
			cdtnr.SetCString(snData,CDTR_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdtnr);
			break;
		}
	}
	BaseDocument *doc = tag->GetDocument(); if(!doc) return TRUE;
	CheckTagReg(doc,op,tData);
	
	//BaseList2D *dest = tData->GetLink(TARGET,doc); if(!dest) return TRUE;
		
	//ListReceivedMessages("CDTransferActivatedTag",type,data);
	
	switch(type)
	{
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == TRANSFER_ON) tData->SetBool(TOGGLE_ON,TRUE);
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
		case CD_MSG_UPDATE:
		{
			//if(dest == op) break;
			
			if(op->GetBit(BIT_ACTIVE))
			{
				if(tData->GetBool(TRANSFER_ON) && !tData->GetBool(TOGGLE_ON))
				{
					tData->SetBool(TOGGLE_ON,TRUE);
					//doc->AnimateObject(dest,doc->GetTime(),ANIMATE_QUICK|ANIMATE_NO_CHILDS);
					EventAdd(EVENT_ANIMATE);
				}
			}
			else
			{
				if(tData->GetBool(TRANSFER_ON) && tData->GetBool(TOGGLE_ON))
				{
					tData->SetBool(TOGGLE_ON,FALSE);
					//doc->AnimateObject(dest,doc->GetTime(),ANIMATE_QUICK|ANIMATE_NO_CHILDS);
					EventAdd(EVENT_ANIMATE);
				}
			}
			break;
		}
	}
	
	return TRUE;
}

Bool CDTransferActivatedTag::Execution(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	//GePrint("CDTransferActivatedTag::Execution");
	
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return FALSE;
	
	if(!tData->GetBool(T_REG))
	{
		if(!tData->GetBool(T_SET)) return FALSE;
		
		Bool tagMoved = FALSE;
		if(op != tData->GetObjectLink(T_OID,doc))
		{
			BaseObject *tOp = tData->GetObjectLink(T_OID,doc);
			if(tOp)
			{
				if(tOp->GetDocument())
				{
					tagMoved = TRUE;
					tData->SetBool(T_MOV,TRUE);
				}
			}
			if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_OID,op);
		}
		else tData->SetBool(T_MOV,FALSE);
		if(tagMoved || tData->GetBool(T_MOV))
		{
			tData->SetBool(TRANSFER_ON, FALSE);
			return FALSE;
		}
		
		tData->SetLink(TARGET,tData->GetLink(TARGET+510000,doc));
		//tData->SetBool(TRANSFER_ON,tData->GetBool(TRANSFER_ON+510000));
	}
	else
	{
		tData->SetBool(T_SET,FALSE);
		tData->SetBool(T_MOV,FALSE);
	}
	
	return TRUE;
}

#if API_VERSION < 12000
LONG CDTransferActivatedTag::Execute(PluginTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	Execution(tag,doc,op,bt,priority,CDGetExecutionFlags(flags));
	return EXECUTION_RESULT_OK;
}
#else
EXECUTIONRESULT CDTransferActivatedTag::Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, LONG priority, EXECUTIONFLAGS flags)
{
	Execution(tag,doc,op,bt,priority,CDGetExecutionFlags(flags));
	return EXECUTIONRESULT_OK;
}
#endif

#if API_VERSION < 12000
Bool CDTransferActivatedTag::GetDDescription(GeListNode *node, Description *description, LONG &flags)
#else
Bool CDTransferActivatedTag::GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC& flags)
#endif
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	//BaseDocument *doc = tag->GetDocument();

	if (!description->LoadDescription(node->GetType())) return FALSE;

	AutoAlloc<AtomArray> ar; if(!ar) return FALSE;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, FALSE);
		else bc->SetBool(DESC_HIDE, TRUE);
	}
	
	
	flags |= DESCFLAGS_DESC_LOADED;
	return TagData::GetDDescription(node,description,flags);
}

#if API_VERSION < 12000
Bool CDTransferActivatedTag::GetDEnabling(GeListNode *node, const DescID &id,GeData &t_data,LONG flags,const BaseContainer *itemdesc)
#else
Bool CDTransferActivatedTag::GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc)
#endif
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return FALSE;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return FALSE;
	BaseDocument *doc = tag->GetDocument(); if(!doc) return FALSE;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return FALSE;

	switch (id[0].id)
	{
		case TARGET:
			if(!tData->GetBool(T_REG)) return FALSE;
			else return TRUE;
	}
	return TRUE;
}

Bool RegisterCDTransferActivatedTag(void)
{
	Bool reg = TRUE;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDTR_SERIAL_SIZE];
	String cdtnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = FALSE;
	
	cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdtnr)) reg = FALSE;
	
#if API_VERSION < 12000
	GeGetSerialInfo(SERIAL_CINEMA4D,&si);
#else
	GeGetSerialInfo(SERIALINFO_CINEMA4D,&si);
#endif
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
	if(kb != s) reg = FALSE;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = TRUE;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & R11_VERSION_SAVABLEDEMO) reg = TRUE;
		if(GeGetVersionType() & R11_VERSION_SAVABLEDEMO_ACTIVE) reg = FALSE;
	}
	
	if(GeGetVersionType() & VERSION_NET) reg = TRUE;
	if(GeGetVersionType() & VERSION_SERVER) reg = TRUE;
#else
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO) reg = TRUE;
	if(GeGetSystemInfo() == SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = FALSE;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = TRUE;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = TRUE;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = TRUE;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_TRANSACTTAG); if (!name.Content()) return TRUE;

	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;

#if API_VERSION < 12000
	return RegisterTagPlugin(ID_CDTRANSACTIVATEDTAG,name,info,CDTransferActivatedTag::Alloc,"tCDTranAct","CDTranSel.tif",0);
#else
	return RegisterTagPlugin(ID_CDTRANSACTIVATEDTAG,name,info,CDTransferActivatedTag::Alloc,"tCDTranAct",AutoBitmap("CDTranSel.tif"),0);
#endif
}

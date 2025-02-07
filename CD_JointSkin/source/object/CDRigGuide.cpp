//	Cactus Dan's Joints & Skin
//	Copyright 2012 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "R12Compatibility.h"
#include "oCDRigGuide.h"
#include "CDJointSkin.h"
#include "CDDebug.h"



enum
{
	//PURCHASE = 1000,
	
	//ADJUST,
	//SET,
	
	DRAG_DETECT			= 10000,
	CHILD_COUNT,
	ADJUST_MODE,


	T_REG					= 500000,
	T_SET					= 500001,
	T_STR					= 500002
};


class CDRigGuide : public ObjectData
{
	INSTANCEOF(CDJoint,ObjectData)
	
private:
	void CheckOpReg(BaseContainer *oData);

	Bool Execution(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags);

public:

	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode* node);
		
	virtual Bool Message(GeListNode *node, LONG type, void *data);

#if API_VERSION < 12000
	virtual Bool Draw(PluginObject *op, LONG type, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool AddToExecution(PluginObject* op, PriorityList* list);
	virtual LONG Execute(PluginObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags);
	virtual Bool GetDDescription(GeListNode *node, Description *description,LONG &flags);
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id,GeData &t_data,LONG flags,const BaseContainer *itemdesc);
#else
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh);
	virtual Bool AddToExecution(BaseObject* op, PriorityList* list);
	virtual EXECUTIONRESULT Execute(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, EXECUTIONFLAGS flags);
	virtual Bool GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc);
#endif
	
	static NodeData *Alloc(void) { return gNew CDRigGuide; }
};

Bool CDRigGuide::Init(GeListNode *node)
{
	BaseObject *op = (BaseObject*)node;
	BaseContainer *oData = op->GetDataInstance();

	oData->SetBool(SHOW_LINES,TRUE);
	oData->SetVector(LINE_COLOR, Vector(1,1,0));
	oData->SetReal(RADIUS,200.0);
	
	oData->SetBool(ADJUST_MODE,TRUE);

	CDJSData rgd;
	PluginMessage(ID_CDRIGGUIDE,&rgd);
	if(rgd.list) rgd.list->Append(node);
	
	return TRUE;
}

void CDRigGuide::Free(GeListNode* node)
{
	CDJSData rgd;
	PluginMessage(ID_CDRIGGUIDE,&rgd);
	if(rgd.list) rgd.list->Remove(node);
}

void CDRigGuide::CheckOpReg(BaseContainer *oData)
{
	Bool reg = TRUE;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdjnr = oData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdjnr)) reg = FALSE;
	
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
	
	oData->SetBool(T_REG,reg);
	if(!reg)
	{
		if(!oData->GetBool(T_SET))
		{
			/*oData->SetBool(CONNECTED+510000,oData->GetBool(CONNECTED));
			oData->SetBool(SHOW_LOCAL_AXIS+510000,oData->GetBool(SHOW_LOCAL_AXIS));
			oData->SetReal(JOINT_RADIUS+510000,oData->GetReal(JOINT_RADIUS));
			
			oData->SetBool(LOCK_ZERO_TRANS+510000,oData->GetBool(LOCK_ZERO_TRANS));
			
			oData->SetBool(SHOW_ENVELOPE+510000,oData->GetBool(SHOW_ENVELOPE));
			oData->SetBool(SHOW_PROXY+510000,oData->GetBool(SHOW_PROXY));
			oData->SetVector(ROOT_OFFSET+510000,oData->GetVector(ROOT_OFFSET));
			oData->SetVector(TIP_OFFSET+510000,oData->GetVector(TIP_OFFSET));
			oData->SetReal(SKEW_RX+510000,oData->GetReal(SKEW_RX));
			oData->SetReal(SKEW_RY+510000,oData->GetReal(SKEW_RY));
			oData->SetReal(SKEW_TX+510000,oData->GetReal(SKEW_TX));
			oData->SetReal(SKEW_TY+510000,oData->GetReal(SKEW_TY));*/
			
			oData->SetBool(T_SET,TRUE);
		}
	}
}

Bool CDRigGuide::Message(GeListNode *node, LONG type, void *data)
{
	BaseObject *op = (BaseObject*)node;
	BaseContainer *oData = op->GetDataInstance(); if(!oData) return TRUE;

	BaseDocument *doc = node->GetDocument(); 
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			oData->SetBool(T_SET,FALSE);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			oData->SetString(T_STR,cdjnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			oData->SetBool(T_SET,FALSE);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			oData->SetString(T_STR,cdjnr);
			break;
		}
			
	}
	if(!doc) return TRUE;
	
	CheckOpReg(oData);
	
	
	//ListReceivedMessages("CDRigGuide",type,data);
	switch (type)
	{
		case MSG_DRAGANDDROP:
		{
			oData->SetBool(DRAG_DETECT,TRUE);
			break;
		}
		case CD_MSG_UPDATE:
		{	
			GePrint("detected change.");
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==ADJUST)
			{
				oData->SetBool(ADJUST_MODE, TRUE);
			}
			else if (dc->id[0].id==SET)
			{
				oData->SetBool(ADJUST_MODE, FALSE);
			}
		}
	}
		
	return TRUE;
}

#if API_VERSION < 12000
Bool CDRigGuide::Draw(PluginObject *op, LONG type, BaseDraw *bd, BaseDrawHelp *bh)
#else
DRAWRESULT CDRigGuide::Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh)
#endif
{
	BaseContainer *oData = op->GetDataInstance();
	
	if(oData->GetBool(SHOW_LINES))
	{
	#if API_VERSION < 12000
		if (type == DRAWPASS_OBJECT)
	#else
		if (drawpass == DRAWPASS_OBJECT)
	#endif
		{
				
		#if API_VERSION > 11999
			bd->SetMatrix_Matrix(NULL, Matrix());
		#endif
			
			bd->SetPen(oData->GetVector(LINE_COLOR));
			
			Matrix opM = op->GetMg();
			Matrix rotM = MatrixRotX(pi05);
			
			Real r = oData->GetReal(RADIUS);
			CDDrawLine(bd,Vector(1*r,0,0),Vector(0.56*r,0,-0.32*r));
			CDDrawLine(bd,Vector(0.56*r,0,-0.32*r),Vector(0.56*r,0,-0.12*r));
			CDDrawLine(bd,Vector(0.56*r,0,-0.12*r),Vector(0.12*r,0,-0.12*r));
			CDDrawLine(bd,Vector(0.12*r,0,-0.12*r),Vector(0.12*r,0,-0.56*r));
			CDDrawLine(bd,Vector(0.12*r,0,-0.56*r),Vector(0.32*r,0,-0.56*r));
			CDDrawLine(bd,Vector(0.32*r,0,-0.56*r),Vector(0,0,-1*r));
			CDDrawLine(bd,Vector(0,0,-1*r),Vector(-0.32*r,0,-0.56*r));
			CDDrawLine(bd,Vector(-0.32*r,0,-0.56*r),Vector(-0.12*r,0,-0.56*r));
			CDDrawLine(bd,Vector(-0.12*r,0,-0.56*r),Vector(-0.12*r,0,-0.12*r));
			CDDrawLine(bd,Vector(-0.12*r,0,-0.12*r),Vector(-0.56*r,0,-0.12*r));
			CDDrawLine(bd,Vector(-0.56*r,0,-0.12*r),Vector(-0.56*r,0,-0.32*r));
			CDDrawLine(bd,Vector(-0.56*r,0,-0.32*r),Vector(-1*r,0,0));
			CDDrawLine(bd,Vector(-1*r,0,0),Vector(-0.56*r,0,0.32*r));
			CDDrawLine(bd,Vector(-0.56*r,0,0.32*r),Vector(-0.56*r,0,0.12*r));
			CDDrawLine(bd,Vector(-0.56*r,0,0.12*r),Vector(-0.12*r,0,0.12*r));
			CDDrawLine(bd,Vector(-0.12*r,0,0.12*r),Vector(-0.12*r,0,0.56*r));
			CDDrawLine(bd,Vector(-0.12*r,0,0.56*r),Vector(-0.32*r,0,0.56*r));
			CDDrawLine(bd,Vector(-0.32*r,0,0.56*r),Vector(0,0,1*r));
			CDDrawLine(bd,Vector(0,0,1*r),Vector(0.32*r,0,0.56*r));
			CDDrawLine(bd,Vector(0.32*r,0,0.56*r),Vector(0.12*r,0,0.56*r));
			CDDrawLine(bd,Vector(0.12*r,0,0.56*r),Vector(0.12*r,0,0.12*r));
			CDDrawLine(bd,Vector(0.12*r,0,0.12*r),Vector(0.56*r,0,0.12*r));
			CDDrawLine(bd,Vector(0.56*r,0,0.12*r),Vector(0.56*r,0,0.32*r));
			CDDrawLine(bd,Vector(0.56*r,0,0.32*r),Vector(1*r,0,0));
		}
	}
		
#if API_VERSION < 12000
	return SUPER::Draw(op, type, bd, bh);
#else
	return DRAWRESULT_OK;
#endif
}

#if API_VERSION < 12000
Bool CDRigGuide::AddToExecution(PluginObject *op, PriorityList *list)
#else
Bool CDRigGuide::AddToExecution(BaseObject *op, PriorityList *list)
#endif
{
#if API_VERSION < 12000
	list->Add(op,EXECUTION_INITIAL, 0);
#else
	list->Add(op,EXECUTIONPRIORITY_INITIAL, EXECUTIONFLAGS_0);
#endif
	
	return TRUE;
}

#if API_VERSION < 12000
LONG CDRigGuide::Execute(PluginObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags)
#else
EXECUTIONRESULT CDRigGuide::Execute(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, EXECUTIONFLAGS flags)
#endif
{
	/*if(op->GetDown())
	{
		Bool folded = op->GetBit(BIT_OFOLD);
		if(folded)
		{
			GePrint("Not Folded");
			op->DelBit(BIT_OFOLD);
			op->Message(MSG_UPDATE);
		}
		else GePrint("Folded");
		
	}*/
	
#if API_VERSION < 12000
	return EXECUTION_RESULT_OK;
#else
	return EXECUTIONRESULT_OK;
#endif
	
}

#if API_VERSION < 12000
Bool CDRigGuide::GetDDescription(GeListNode *node, Description *description, LONG &flags)
#else
Bool CDRigGuide::GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
#endif
{
	BaseObject *op = (BaseObject*)node;
	BaseContainer *data = op->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return FALSE;
	
	AutoAlloc<AtomArray> ar; if(!ar) return FALSE;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(909), ar);
	if(bc) bc->SetBool(DESC_HIDE, FALSE);
	
	bc = description->GetParameterI(DescLevel(PURCHASE), ar);
	if(bc)
	{
		if(!data->GetBool(T_REG)) bc->SetBool(DESC_HIDE, FALSE);
		else bc->SetBool(DESC_HIDE, TRUE);
	}
	
	flags |= DESCFLAGS_DESC_LOADED;
	
	return SUPER::GetDDescription(node,description,flags);
}

#if API_VERSION < 12000
Bool CDRigGuide::GetDEnabling(GeListNode *node, const DescID &id,GeData &t_data,LONG flags,const BaseContainer *itemdesc)
#else
Bool CDRigGuide::GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc)
#endif
{
	//GePrint("CDSkinPlugin::GetDEnabling");
	BaseObject	*op  = (BaseObject*)node; if(!op) return FALSE;
	BaseContainer *oData = op->GetDataInstance(); if(!oData) return FALSE;
	
	switch (id[0].id)
	{
		case ADJUST:
			if(!oData->GetBool(T_REG)) return FALSE;
			else if(oData->GetBool(ADJUST_MODE)) return FALSE;
			else return TRUE;
			
		case SET:
			if(!oData->GetBool(T_REG)) return FALSE;
			else if(!oData->GetBool(ADJUST_MODE)) return FALSE;
			else return TRUE;
	}
	
	return TRUE;
}


Bool RegisterCDRigGuide(void)
{
#if API_VERSION < 12000
	if(!FindPlugin(ID_ABOUTCDIKTOOLS,C4DPL_COMMAND)) return TRUE;
#else
	if(!FindPlugin(ID_ABOUTCDIKTOOLS,PLUGINTYPE_COMMAND)) return TRUE;
#endif

	Bool reg = TRUE;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = FALSE;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = FALSE;
	
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
	
	LONG info;
	if(!reg) info = PLUGINFLAG_HIDE;
	else info = 0;
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDRGUIDE); if (!name.Content()) return TRUE;
#if API_VERSION < 12000
	#if API_VERSION < 9800
		return RegisterObjectPlugin(ID_CDRIGGUIDE,name,info,CDRigGuide::Alloc,"oCDRigGuide","CDRigGuide.tif","CDRigGuide_small.tif",0);
	#else
		return RegisterObjectPlugin(ID_CDRIGGUIDE,name,info,CDRigGuide::Alloc,"oCDRigGuide","CDRigGuide.tif",0);
	#endif
#else
	return RegisterObjectPlugin(ID_CDRIGGUIDE,name,info,CDRigGuide::Alloc,"oCDRigGuide",AutoBitmap("CDRigGuide.tif"),0);
#endif
}

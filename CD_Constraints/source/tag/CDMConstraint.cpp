//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDMConstraint.h"
#include "CDConstraint.h"

/*enum
{
	//MC_TARGET				= 1010,
	//MC_STRENGTH			= 1011,
	
	//MC_MIRROR_AXIS			= 1012,
		//MC_X_AXIS			= 1013,
		//MC_Y_AXIS			= 1014,
		//MC_Z_AXIS			= 1015,

	//MC_SHOW_LINES			= 1016,
	//MC_LINE_COLOR			= 1017,
	//MC_MIR_CENTER			= 1018,
		//MC_GLOBAL			= 1019,
		//MC_LOCAL			= 1020,
		//MC_OBJECT			= 1021,
	
	//MC_OBJECT_LINK			= 1022,
	
	//MC_PURCHASE			= 1030,
	
	//MC_ID_TARGET			= 2000,
};*/

class CDMConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDMConstraintPlugin); }
};

Bool CDMConstraintPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(MC_MIRROR_AXIS,MC_X_AXIS);
	tData->SetLong(MC_MIR_CENTER,MC_GLOBAL);
	tData->SetBool(MC_SHOW_LINES,true);
	tData->SetVector(MC_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(MC_STRENGTH,1.00);

	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
		
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDMConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	if (!tData->GetBool(MC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(MC_LINE_COLOR));
	BaseObject *goal = tData->GetObjectLink(MC_TARGET,doc); 
	if (goal)
	{
		CDDrawLine(bd,op->GetMg().off, goal->GetMg().off);
	}
	
	return true;
}

void CDMConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdcnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
			
			tData->SetLink(MC_TARGET+T_LST,tData->GetLink(MC_TARGET,doc));
			tData->SetLink(MC_OBJECT_LINK+T_LST,tData->GetLink(MC_OBJECT_LINK,doc));
			tData->SetLong(MC_MIRROR_AXIS+T_LST,tData->GetLong(MC_MIRROR_AXIS));
			tData->SetLong(MC_MIR_CENTER+T_LST,tData->GetLong(MC_MIR_CENTER));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDMConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	BaseDocument *doc = node->GetDocument();
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==MC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
		}
	}

	return true;
}

Bool CDMConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(MC_TARGET,tData->GetLink(MC_TARGET+T_LST,doc));
		tData->SetLink(MC_OBJECT_LINK,tData->GetLink(MC_OBJECT_LINK+T_LST,doc));
		tData->SetLong(MC_MIRROR_AXIS,tData->GetLong(MC_MIRROR_AXIS+T_LST));
		tData->SetLong(MC_MIR_CENTER,tData->GetLong(MC_MIR_CENTER+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDMConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseObject *goal = tData->GetObjectLink(MC_TARGET,doc); if (!goal) return false;
	
	Vector opPos, glPos, mirPos, trgPos;
	Vector theScale = CDGetScale(goal);
	Vector oldRot, newRot, rotSet;
		
	Real strength = tData->GetReal(MC_STRENGTH);
	Matrix opM = op->GetMg(), goalM = goal->GetMg(), trgM, rotM, prM = Matrix();
	CDQuaternion opQ, goalQ, trgQ, newQ;
	
	//Check for Parent object
	BaseObject *pr = NULL;
	switch(tData->GetLong(MC_MIR_CENTER))
	{
		case MC_LOCAL:
		{
			pr = op->GetUp();
			if(pr) prM = pr->GetMg();
			break;
		}
		case MC_OBJECT:
		{
			pr = tData->GetObjectLink(MC_OBJECT_LINK,doc);
			if(pr) prM = pr->GetMg();
			break;
		}
	}
	
	opQ.SetMatrix(MInv(prM) * opM);
	goalQ.SetMatrix(MInv(prM) * goalM);
	opPos = MInv(prM) * opM.off;
	glPos = MInv(prM) * goal->GetMg().off;
	switch(tData->GetLong(MC_MIRROR_AXIS))
	{
		case MC_X_AXIS:	
			mirPos.x = -glPos.x;
			mirPos.y = glPos.y;
			mirPos.z = glPos.z;
			trgQ.w = -goalQ.w;
			trgQ.v.x = -goalQ.v.x;
			trgQ.v.y = goalQ.v.y;
			trgQ.v.z = goalQ.v.z;
			break;
		case MC_Y_AXIS:
			mirPos.x = glPos.x;
			mirPos.y = -glPos.y;
			mirPos.z = glPos.z;
			trgQ.w = -goalQ.w;
			trgQ.v.x = goalQ.v.x;
			trgQ.v.y = -goalQ.v.y;
			trgQ.v.z = goalQ.v.z;
			break;
		case MC_Z_AXIS:
			mirPos.x = glPos.x;
			mirPos.y = glPos.y;
			mirPos.z = -glPos.z;
			trgQ.w = -goalQ.w;
			trgQ.v.x = goalQ.v.x;
			trgQ.v.y = goalQ.v.y;
			trgQ.v.z = -goalQ.v.z;
			break;
		default:
		break;
	}
	trgPos = mirPos;

	newQ = CDQSlerp(opQ, trgQ, strength);
	rotM = newQ.GetMatrix();
	rotM.off = CDBlend(opPos, trgPos, strength);
	trgM = prM * rotM;
	
	if(!MatrixEqual(opM,trgM,0.001))
	{
		oldRot = CDGetRot(op);
		op->SetMg(trgM);
		newRot = CDGetRot(op);
		rotSet = CDGetOptimalAngle(oldRot, newRot, op);
		CDSetRot(op,rotSet);
		CDSetScale(op,theScale);
	}

	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDMConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(MC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDMConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case MC_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MC_MIRROR_AXIS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MC_MIR_CENTER:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MC_OBJECT_LINK:
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetLong(MC_MIR_CENTER) == MC_OBJECT) return true;
			else return false;
	}
	return true;
}

Bool RegisterCDMConstraintPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDMCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDMCONSTRAINTPLUGIN,name,info,CDMConstraintPlugin::Alloc,"tCDMConstraint","CDMConstraint.tif",0);
}

//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDPistonIK.h"
#include "CDIKtools.h"

#define MAX_BONES		100

enum
{
	//PSTIK_PURCHASE					= 1000,
	
	//PSTIK_LINE_COLOR					= 1500,
	
	//PSTIK_BONES_IN_GROUP				= 1600,
	
	//PSTIK_SHOW_LINES				= 10001,
	
	//PSTIK_POLE_AXIS					= 10006,
		//PSTIK_POLE_X				= 10007,
		//PSTIK_POLE_Y				= 10008,
	
	//PSTIK_POLE_LINK					= 10009,
	
	//PSTIK_GOAL_LINK					= 10010,
	
	//PSTIK_POLE_GROUP				= 10011,
	//PSTIK_GOAL_GROUP				= 10012,
	
	//PSTIK_POLE_NX					= 10019,
	//PSTIK_POLE_NY					= 10020,
	
	//PSTIK_MIX_GROUP					= 20000
	
	PSTIK_MIX						= 30000	
};

class CDPistonIKPlugin : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	LONG GetPoleAxis(BaseContainer *tData);
	LONG ConfirmJointCount(BaseObject *jnt, BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDPistonIKPlugin); }
};

Bool CDPistonIKPlugin::Init(GeListNode *node)
{
	BaseTag	*tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(PSTIK_POLE_AXIS,PSTIK_POLE_Y);
	tData->SetBool(PSTIK_SHOW_LINES,false);
	tData->SetVector(PSTIK_LINE_COLOR, Vector(1,0,0));
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	return true;
}

Bool CDPistonIKPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!tData->GetBool(PSTIK_SHOW_LINES)) return true;
	
	BaseObject *goal = tData->GetObjectLink(PSTIK_GOAL_LINK,doc); if(!goal) return true;
	BaseObject *pole = tData->GetObjectLink(PSTIK_POLE_LINK,doc); if(!pole) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
		
	bd->SetPen(tData->GetVector(PSTIK_LINE_COLOR));
	CDDrawLine(bd,op->GetMg().off, goal->GetMg().off);
	CDDrawLine(bd,op->GetMg().off, pole->GetMg().off);
	
	return true;
}

Bool CDPistonIKPlugin::Message(GeListNode *node, LONG type, void *data)
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
			CHAR snData[CDIK_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDIKTOOLS,snData,CDIK_SERIAL_SIZE);
			cdknr.SetCString(snData,CDIK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDIK_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDIKTOOLS,snData,CDIK_SERIAL_SIZE);
			cdknr.SetCString(snData,CDIK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	LONG jCnt = ConfirmJointCount(op, tData);

	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==PSTIK_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

LONG CDPistonIKPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(PSTIK_POLE_AXIS))
	{
		case PSTIK_POLE_X:	
			return POLE_X;
		case PSTIK_POLE_Y:
			return POLE_Y;
		case PSTIK_POLE_NX:	
			return POLE_NX;
		case PSTIK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

LONG CDPistonIKPlugin::ConfirmJointCount(BaseObject *jnt, BaseContainer *tData)
{
	LONG jCnt = 0;
	
	while(jnt &&  jCnt < tData->GetLong(PSTIK_BONES_IN_GROUP))
	{
		jnt = jnt->GetDown();
		if(jnt) jCnt++;
	}
	tData->SetLong(PSTIK_BONES_IN_GROUP, jCnt);
	
	return jCnt;
	
}

LONG CDPistonIKPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	BaseObject *goal = tData->GetObjectLink(PSTIK_GOAL_LINK,doc); if(!goal) return false;
	BaseObject *pole = tData->GetObjectLink(PSTIK_POLE_LINK,doc); if(!pole) return false;
	
	LONG i, jCnt = ConfirmJointCount(op, tData);
	
	Matrix opM = op->GetMg(), goalM = goal->GetMg(), poleM = pole->GetMg(), transM, targM;
	Vector opSca = CDGetScale(op), oldRot = CDGetRot(op), newRot, opRot;
		
	//	Set the vectors to the Pole and Goal
	Vector goalV = goalM.off - opM.off;
	Vector poleV = poleM.off - opM.off;
	
	//	Get the Rotation Plane matrix.
	Matrix rpM = GetRPMatrix(GetPoleAxis(tData),opM.off,goalV,poleV);
	
	// set root joint
	targM = rpM;
	op->SetMg(targM);
	
	newRot = CDGetRot(op);
	opRot = CDGetOptimalAngle(oldRot, newRot, op);
	
	CDSetScale(op,opSca);
	CDSetRot(op,opRot);
	
	BaseObject *ch = op->GetDown();
	for(i=0; i<jCnt; i++)
	{
		if(ch)
		{
			Vector chPos = CDGetPos(ch), chSca = CDGetScale(ch), chRot;
			oldRot = CDGetRot(ch);
			
			Real mix = tData->GetReal(PSTIK_MIX+i);
			if(i == jCnt-1) mix = 1.0;
			
			targM.off = CDBlend(opM.off, goalM.off, mix);
			ch->SetMg(targM);
			
			newRot = CDGetRot(ch);
			chRot = CDGetOptimalAngle(oldRot, newRot, ch);
			
			CDSetScale(ch,chSca);
			CDSetRot(ch,chRot);
			
			ch = ch->GetDown();
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDPistonIKPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	BaseObject *ch = op->GetDown(); if(!ch) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(PSTIK_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	LONG i, cnt = tData->GetLong(PSTIK_BONES_IN_GROUP)-1;
	for(i=0; i<cnt; i++)
	{
		if(ch)
		{
			String name = ch->GetName() + ".Mix";
			
			BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_REAL);
			bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
			bc1.SetString(DESC_NAME, name);
			bc1.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
			bc1.SetReal(DESC_MINSLIDER, 0.0);
			bc1.SetReal(DESC_MAXSLIDER, 1.0);
			bc1.SetReal(DESC_STEP, 0.01);
			bc1.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
			bc1.SetBool(DESC_REMOVEABLE,false);
			if(!description->SetParameter(DescLevel(PSTIK_MIX+i, DTYPE_REAL, 0), bc1, DescLevel(PSTIK_MIX_GROUP))) return true;
			
			ch = ch->GetDown();
		}
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

void CDPistonIKPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdknr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
			tData->SetLink(T_PID,op->GetUp());
			
			tData->SetLong(PSTIK_POLE_AXIS+T_LST,tData->GetLong(PSTIK_POLE_AXIS));
			
			tData->SetLink(PSTIK_GOAL_LINK+T_LST,tData->GetLink(PSTIK_GOAL_LINK,doc));
			tData->SetLink(PSTIK_POLE_LINK+T_LST,tData->GetLink(PSTIK_POLE_LINK,doc));
			
			LONG i, cnt = tData->GetLong(PSTIK_BONES_IN_GROUP);
			for(i=0; i<cnt; i++)
			{
				tData->SetReal(PSTIK_MIX+T_LST+i,tData->GetReal(PSTIK_MIX+i));
			}
						
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDPistonIKPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		else
		{
			if(op->GetUp() != tData->GetObjectLink(T_PID,doc))
			{
				BaseObject *pOp = tData->GetObjectLink(T_PID,doc);
				if(pOp)
				{
					if(pOp->GetDocument())
					{
						tagMoved = true;
						tData->SetBool(T_MOV,true);
					}
				}
				if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_PID,op->GetUp());
			}
			else tData->SetBool(T_MOV,false);
		}
		if(tagMoved || tData->GetBool(T_MOV)) enable = false;
		
		tData->SetLong(PSTIK_POLE_AXIS,tData->GetLong(PSTIK_POLE_AXIS+T_LST));
		
		tData->SetLink(PSTIK_GOAL_LINK,tData->GetLink(PSTIK_GOAL_LINK+T_LST,doc));
		tData->SetLink(PSTIK_POLE_LINK,tData->GetLink(PSTIK_POLE_LINK+T_LST,doc));
		
		LONG i, cnt = tData->GetLong(PSTIK_BONES_IN_GROUP);
		for(i=0; i<cnt; i++)
		{
			tData->SetReal(PSTIK_MIX+i,tData->GetReal(PSTIK_MIX+T_LST+i));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDPistonIKPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	LONG i, cnt = tData->GetLong(PSTIK_BONES_IN_GROUP);
	
	switch (id[0].id)
	{
		case PSTIK_GOAL_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSTIK_POLE_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSTIK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	for(i=0; i<cnt; i++)
	{
		if(id[0].id == PSTIK_MIX+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
	}
	
	return true;
}

Bool RegisterCDPistonIKPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDIK_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE)) reg = false;
	
	cdknr.SetCString(data,CDIK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDPSTNIK); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDPISTONIKPLUGIN,name,info,CDPistonIKPlugin::Alloc,"tCDPistonIK","CDPistonIK.tif",1);
}

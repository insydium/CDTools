//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDLinkageIK.h"
#include "CDIKtools.h"

/*enum
{
	//LNKIK_PURCHASE					= 1000,
	
	//LNKIK_LINE_COLOR				= 1500,
	
	//LNKIK_SHOW_LINES				= 10001,
	
	//LNKIK_POLE_AXIS					= 10006,
	//LNKIK_POLE_X				= 10007,
	//LNKIK_POLE_Y				= 10008,
	
	//LNKIK_POLE_LINK					= 10009,
	
	//LNKIK_GOAL_LINK					= 10010,
	
	//LNKIK_POLE_GROUP				= 10011,
	//LNKIK_GOAL_GROUP				= 10012,
	
	//LNKIK_POLE_NX					= 10019,
	//LNKIK_POLE_NY					= 10020,
	
};*/

class CDLinkageIKPlugin : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	LONG GetPoleAxis(BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDLinkageIKPlugin); }
};

Bool CDLinkageIKPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(LNKIK_SHOW_LINES,true);
	tData->SetVector(LNKIK_LINE_COLOR, Vector(1,0,0));
	
	tData->SetLong(LNKIK_POLE_AXIS,LNKIK_POLE_Y);
	tData->SetBool(LNKIK_CONNECT_BONES,false);
	tData->SetBool(LNKIK_CONNECT_NEXT,false);
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	return true;
}

Bool CDLinkageIKPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	
	BaseObject *ch = op->GetDown(); if(!ch) return true;
	BaseObject *gch = ch->GetDown(); if(!gch) return true;
	
	BaseObject *goal = tData->GetObjectLink(LNKIK_GOAL_LINK,doc); if(!goal) return true;
	BaseObject *pole = tData->GetObjectLink(LNKIK_POLE_LINK,doc); if(!pole) return true;
	
	Vector opPos = op->GetMg().off;
	Vector chPos = ch->GetMg().off;
	Vector polePos = pole->GetMg().off;
	Vector goalPos = goal->GetMg().off;
	
	if(!tData->GetBool(LNKIK_SHOW_LINES)) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(LNKIK_LINE_COLOR));
	CDDrawLine(bd,opPos, polePos);
	CDDrawLine(bd,opPos, chPos);
	CDDrawLine(bd,chPos, gch->GetMg().off);
	
	return true;
}

void CDLinkageIKPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(LNKIK_POLE_AXIS+T_LST,tData->GetLong(LNKIK_POLE_AXIS));
			
			tData->SetLink(LNKIK_POLE_LINK+T_LST,tData->GetLink(LNKIK_POLE_LINK,doc));
			tData->SetLink(LNKIK_GOAL_LINK+T_LST,tData->GetLink(LNKIK_GOAL_LINK,doc));
			
			BaseObject *ch = op->GetDown();
			if(ch)
			{
				tData->SetVector(T_CP,CDGetPos(ch));
				
				BaseObject *gch = ch->GetDown();
				if(gch)
				{
					tData->SetVector(T_GCP,CDGetPos(gch));
				}
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDLinkageIKPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(LNKIK_POLE_AXIS,tData->GetLong(LNKIK_POLE_AXIS+T_LST));
		
		tData->SetLink(LNKIK_POLE_LINK,tData->GetLink(LNKIK_POLE_LINK+T_LST,doc));
		tData->SetLink(LNKIK_GOAL_LINK,tData->GetLink(LNKIK_GOAL_LINK+T_LST,doc));
		
		BaseObject *ch = op->GetDown();
		if(ch)
		{
			CDSetPos(ch,tData->GetVector(T_CP));
			
			BaseObject *gch = ch->GetDown();
			if(gch)
			{
				CDSetPos(gch,tData->GetVector(T_GCP));
			}
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDLinkageIKPlugin::Message(GeListNode *node, LONG type, void *data)
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
	
	BaseObject *ch = op->GetDown(); if(!ch) return true;
	BaseObject *gch = ch->GetDown(); if(!gch) return true;
	BaseObject *goal = tData->GetObjectLink(LNKIK_GOAL_LINK,doc); if(!goal) return true;
	
	Vector gScale = GetGlobalScale(op);
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id == LNKIK_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

LONG CDLinkageIKPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(LNKIK_POLE_AXIS))
	{
		case LNKIK_POLE_X:	
			return POLE_X;
		case LNKIK_POLE_Y:
			return POLE_Y;
		case LNKIK_POLE_NX:	
			return POLE_NX;
		case LNKIK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

LONG CDLinkageIKPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	BaseObject *ch = op->GetDown();
	BaseObject *gch = ch->GetDown();
	
	BaseObject *goal = tData->GetObjectLink(LNKIK_GOAL_LINK,doc); if(!goal) return false;
	BaseObject *pole = tData->GetObjectLink(LNKIK_POLE_LINK,doc); if(!pole) return false;
	
	Matrix opM = op->GetMg(), chM = ch->GetMg(), gchM = gch->GetMg();
	Matrix goalM = goal->GetMg(), poleM = pole->GetMg();
	Matrix rpM, twistM, targM, transM;
	
	Vector opPos = CDGetPos(op), opScale = CDGetScale(op), opRot = CDGetRot(op);
	Vector chPos = CDGetPos(ch), chScale = CDGetScale(ch), chRot = CDGetRot(ch);
	Vector gchPos = CDGetPos(gch), gchScale = CDGetScale(gch), gchRot = CDGetRot(gch);
	
	// check plane points for 0 distance
	if(poleM.off == opM.off) return false;
	if(goalM.off == opM.off) return false;
	if(goalM.off == poleM.off) return false;
	if(poleM.off == chM.off) return false;
	if(goalM.off == chM.off) return false;
	
	//	Set the vectors to the Pole and Goal
	Vector poleV = poleM.off - opM.off;
	Vector goalV = goalM.off - opM.off;

	//	Get the Rotation Plane matrix.
	rpM = GetRPMatrix(GetPoleAxis(tData),opM.off,goalV,poleV);

	// set root joint
	targM = op->GetMg();
	switch(tData->GetLong(LNKIK_POLE_AXIS))
	{
		case LNKIK_POLE_X:
			targM.v1 = rpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, targM.v1));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		case LNKIK_POLE_Y:
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		case LNKIK_POLE_NX:
			targM.v1 = rpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, targM.v1));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		case LNKIK_POLE_NY:
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
	}
	
	op->SetMg(targM);
	CDSetScale(op,opScale);
	CDSetPos(op,opPos);
	
	// set the child joint
	Real b = Len(chM.off - opM.off);
	Real c = Len(gchM.off - chM.off);
	Real C = ACos(VDot(VNorm(targM.v3),VNorm(goalV)));
	Real B = ASin((Sin(C)/c) * b);
	Real A = pi - (C + B);
	Real a = Sqrt((b*b) + (c*c) - 2 * b * c* Cos(A));

	targM = ch->GetMg();
	targM.v3 = VNorm((opM.off + VNorm(goalV) * a) - targM.off);
	switch(tData->GetLong(LNKIK_POLE_AXIS))
	{
		case LNKIK_POLE_X:
			targM.v1 = VNorm(rpM.v1);
			targM.v2 = VNorm(VCross(targM.v3, targM.v1));
			break;
		case LNKIK_POLE_Y:
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case LNKIK_POLE_NX:
			targM.v1 = VNorm(rpM.v1);
			targM.v2 = VNorm(VCross(targM.v3, targM.v1));
			break;
		case LNKIK_POLE_NY:
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
	}
	
	ch->SetMg(targM);
	CDSetScale(ch,chScale);
	if(tData->GetBool(LNKIK_CONNECT_BONES))
	{
		chPos.x = 0.0;
		chPos.y = 0.0;
	}
	CDSetPos(ch,chPos);
	
	// set the end joint
	targM = gch->GetMg();
	targM.v3 = VNorm(goalV);
	switch(tData->GetLong(LNKIK_POLE_AXIS))
	{
		case LNKIK_POLE_X:
			targM.v1 = VNorm(rpM.v1);
			targM.v2 = VNorm(VCross(targM.v3, targM.v1));
			break;
		case LNKIK_POLE_Y:
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case LNKIK_POLE_NX:
			targM.v1 = VNorm(rpM.v1);
			targM.v2 = VNorm(VCross(targM.v3, targM.v1));
			break;
		case LNKIK_POLE_NY:
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
	}
	
	gch->SetMg(targM);
	CDSetScale(gch,gchScale);
	if(tData->GetBool(LNKIK_CONNECT_NEXT))
	{
		gchPos.x = 0.0;
		gchPos.y = 0.0;
	}
	CDSetPos(gch,gchPos);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDLinkageIKPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(LNKIK_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDLinkageIKPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case LNKIK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LNKIK_GOAL_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LNKIK_POLE_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDLinkageIKPlugin(void)
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
	String name=GeLoadString(IDS_CDLNKGIK); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDLINKAGEIKPLUGIN,name,info,CDLinkageIKPlugin::Alloc,"tCDLinkageIK","CDLinkageIK.tif",1);
}


//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDMechIK.h"
#include "CDIKtools.h"

enum
{
	//MCHIK_LINE_COLOR			= 1500,
	//MCHIK_TWIST					= 1501,
	
	//MCHIK_SHOW_LINES			= 10001,
	//MCHIK_USE				= 10002,
	
	//MCHIK_ROOT_ANGLE			= 10003,
	//MCHIK_ROOT_FLIP				= 10004,

	//MCHIK_POLE_AXIS			= 10006,
		//MCHIK_POLE_Y			= 10007,
		//MCHIK_POLE_X			= 10008,
	
	//MCHIK_SOLVER_LINK		= 10009,
		
	//MCHIK_GOAL_LINK			= 10010,
	
	//MCHIK_SOLVER_GROUP		= 10011,
	//MCHIK_GOAL_GROUP			= 10012,

	MCHIK_PVR_MATRIX				= 10014,
	MCHIK_DIRTY_IK				= 10015,
	MCHIK_OLD_IK_BLEND			= 10016,

	//MCHIK_DAMPING_ON			= 10017,
	//MCHIK_DAMPING_VALUE			= 10018,
	//MCHIK_POLE_NX			= 10019,
	//MCHIK_POLE_NY			= 10020,
	//MCHIK_GOAL_TO_BONE			= 10021,
	MCHIK_GOAL_DIR_MATRIX			= 10022,
	
	MCHIK_G_VECTOR				= 10030,
	MCHIK_P_VECTOR				= 10031,
	
	MCHIK_REST_ROTATION_SET		= 11000,
	MCHIK_REST_ROTATION			= 11001,
	
	//MCHIK_LOCK_UPPER				= 12000,
	//MCHIK_LOWER_SLIDE				= 12001,
	//MCHIK_SLIDE_MIN				= 12002,
	//MCHIK_SLIDE_MAX				= 12003,
	
	//MCHIK_CONNECT_BONES			= 20000,
	//MCHIK_CONNECT_NEXT			= 20001,

	//MCHIK_BLEND				= 20002,
	MCHIK_P1						= 20003,
	MCHIK_P2						= 20004,
	MCHIK_P3						= 20005
};

class CDMechIKPlugin : public CDTagData
{
private:
	Vector poleV, goalV;
	Matrix goalDirM;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData);
	
	LONG GetPoleAxis(BaseContainer *tData);
	Matrix GetRootRPMatrix(LONG pole, Vector o, Vector g, Vector p);
	void SetFootIKData(BaseObject *op, BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDMechIKPlugin); }
};

Bool CDMechIKPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(MCHIK_USE,true);
	tData->SetReal(MCHIK_BLEND,1.0);
	
	tData->SetBool(MCHIK_SHOW_LINES,true);
	tData->SetVector(MCHIK_LINE_COLOR, Vector(1,0,0));
	
	tData->SetLong(MCHIK_POLE_AXIS,MCHIK_POLE_Y);
	tData->SetBool(MCHIK_DAMPING_ON,false);
	tData->SetReal(MCHIK_DAMPING_VALUE,0);
	tData->SetBool(MCHIK_CONNECT_BONES,false);
	tData->SetBool(MCHIK_CONNECT_NEXT,false);

	tData->SetBool(MCHIK_LOCK_UPPER,false);
	tData->SetReal(MCHIK_ROOT_ANGLE,0.0);
	tData->SetBool(MCHIK_ROOT_FLIP,false);
	
	tData->SetBool(MCHIK_LOWER_SLIDE,false);
	tData->SetReal(MCHIK_SLIDE_MIN,10.0);
	tData->SetReal(MCHIK_SLIDE_MAX,100.0);
	
	tData->SetBool(MCHIK_REST_ROTATION_SET,false);
	tData->SetBool(MCHIK_DIRTY_IK,true);

	goalDirM = Matrix();

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDMechIKPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *ch = op->GetDown(); if(!ch) return true;
	BaseObject *gch = ch->GetDown(); if(!gch) return true;
	BaseObject *eff = gch->GetDown(); if(!eff) return true;
	
	BaseObject *goal = tData->GetObjectLink(MCHIK_GOAL_LINK,doc); if(!goal) return true;
	BaseObject *pole = tData->GetObjectLink(MCHIK_SOLVER_LINK,doc); if(!pole) return true;
	
	Vector opPos = op->GetMg().off;
	Vector chPos = ch->GetMg().off;
	Vector polePos = pole->GetMg().off;
	Vector goalPos = goal->GetMg().off;
	
	if(!tData->GetBool(MCHIK_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(MCHIK_LINE_COLOR));
	CDDrawLine(bd,opPos, polePos);
	CDDrawLine(bd,opPos, chPos);
	CDDrawLine(bd,chPos, gch->GetMg().off);
	CDDrawLine(bd,gch->GetMg().off, eff->GetMg().off);
	
	Vector p1 = tData->GetVector(MCHIK_P1);
	Vector p2 = tData->GetVector(MCHIK_P2);
	Vector p3 = tData->GetVector(MCHIK_P3);
	CDDrawLine(bd,p1, goalPos);
	
	if(tData->GetBool(MCHIK_LOCK_UPPER))
	{
		Real gLen = Len(goalPos - p2);
		if(gLen > tData->GetReal(MCHIK_SLIDE_MAX)) CDDrawLine(bd,p3, goalPos);
	}
	
	ObjectColorProperties prop;
	if(tData->GetBool(MCHIK_USE) && tData->GetReal(MCHIK_BLEND) < 1.0)
	{
		op->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0.25,0));
		CDDrawLine(bd,opPos, p1);
		
		ch->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0,0.5));
		CDDrawLine(bd,p1, p2);
		
		gch->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0,0.5));
		CDDrawLine(bd,p2, p3);
	}
		
	return true;
}

void CDMechIKPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(MCHIK_POLE_AXIS+T_LST,tData->GetLong(MCHIK_POLE_AXIS));
			
			tData->SetLink(MCHIK_SOLVER_LINK+T_LST,tData->GetLink(MCHIK_SOLVER_LINK,doc));
			tData->SetLink(MCHIK_GOAL_LINK+T_LST,tData->GetLink(MCHIK_GOAL_LINK,doc));
			
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

Bool CDMechIKPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(MCHIK_POLE_AXIS,tData->GetLong(MCHIK_POLE_AXIS+T_LST));
			
		tData->SetLink(MCHIK_SOLVER_LINK,tData->GetLink(MCHIK_SOLVER_LINK+T_LST,doc));
		tData->SetLink(MCHIK_GOAL_LINK,tData->GetLink(MCHIK_GOAL_LINK+T_LST,doc));
				
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

Bool CDMechIKPlugin::Message(GeListNode *node, LONG type, void *data)
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
	BaseObject *goal = tData->GetObjectLink(MCHIK_GOAL_LINK,doc); if(!goal) return true;

	if(!tData->GetBool(MCHIK_GOAL_TO_BONE))
	{
		goalDirM = MInv(ch->GetMg()) * goal->GetMg();
		tData->SetMatrix(MCHIK_GOAL_DIR_MATRIX,goalDirM);
	}
	
	Vector gScale = GetGlobalScale(op);
	switch (type)
	{
		case MSG_CHANGE:
		{
			tData->SetBool(MCHIK_DIRTY_IK,true);
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			tData->SetBool(MCHIK_DIRTY_IK,true);
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==MCHIK_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}

	return true;
}

LONG CDMechIKPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(MCHIK_POLE_AXIS))
	{
		case MCHIK_POLE_X:	
			return POLE_X;
		case MCHIK_POLE_Y:
			return POLE_Y;
		case MCHIK_POLE_NX:	
			return POLE_NX;
		case MCHIK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

void CDMechIKPlugin::SetFootIKData(BaseObject *op, BaseContainer *tData)
{
	BaseTag *footIKTag = op->GetTag(ID_CDFOOTIKPLUGIN);
	BaseContainer *ftData = footIKTag->GetDataInstance();
	
	ftData->SetVector(10018,tData->GetVector(MCHIK_P2));
	ftData->SetReal(MCHIK_BLEND,tData->GetReal(MCHIK_BLEND));
	ftData->SetBool(MCHIK_USE,tData->GetBool(MCHIK_USE));
	if(!tData->GetBool(MCHIK_CONNECT_NEXT))
	{
		ftData->SetBool(MCHIK_CONNECT_BONES,false);
	}
	else
	{
		ftData->SetBool(MCHIK_CONNECT_BONES,true);
	}
}

Bool CDMechIKPlugin::CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData)
{
	BaseObject *ch = op->GetDown();
	if(!ch)
	{
		tData->SetBool(MCHIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *gch = ch->GetDown();
	if(!gch)
	{
		tData->SetBool(MCHIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *eff = gch->GetDown();
	if(!eff)
	{
		tData->SetBool(MCHIK_REST_ROTATION_SET,false);
		return false;
	}
	
	BaseObject *goal = tData->GetObjectLink(MCHIK_GOAL_LINK,doc);
	if(!goal)
	{
		tData->SetBool(MCHIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *pole = tData->GetObjectLink(MCHIK_SOLVER_LINK,doc);
	if(!pole)
	{
		tData->SetBool(MCHIK_REST_ROTATION_SET,false);
		return false;
	}
	
	return true;
}

Matrix CDMechIKPlugin::GetRootRPMatrix(LONG pole, Vector o, Vector g, Vector p)
{
	Matrix m = Matrix();

	m.off = o;
	switch(pole)
	{
		case POLE_X:	
			m.v1 = VNorm(p);
			m.v2 = VNorm(VCross(g, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			m.v2 = VNorm(VCross(m.v3, m.v1));
			break;
		case POLE_Y:
			m.v2 = VNorm(p);
			m.v1 = VNorm(VCross(m.v2, g));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			m.v1 = VNorm(VCross(m.v2, m.v3));
			break;
		case POLE_NX:	
			m.v1 = VNorm(p*-1);
			m.v2 = VNorm(VCross(g, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			m.v2 = VNorm(VCross(m.v3, m.v1));
			break;
		case POLE_NY:
			m.v2 = VNorm(p*-1);
			m.v1 = VNorm(VCross(m.v2, g));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			m.v1 = VNorm(VCross(m.v2, m.v3));
			break;
	}

	return m;	
}

LONG CDMechIKPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	if(!CheckLinks(op,doc,tData)) return false;
	
	BaseObject *ch = op->GetDown();
	BaseObject *gch = ch->GetDown();
	BaseObject *eff = gch->GetDown();
	
	BaseObject *goal = tData->GetObjectLink(MCHIK_GOAL_LINK,doc);
	BaseObject *pole = tData->GetObjectLink(MCHIK_SOLVER_LINK,doc);

	if(CDIsDirty(op,CD_DIRTY_MATRIX)) tData->SetBool(MCHIK_DIRTY_IK,true);
	if(CDIsDirty(ch,CD_DIRTY_MATRIX)) tData->SetBool(MCHIK_DIRTY_IK,true);
	if(CDIsDirty(gch,CD_DIRTY_MATRIX)) tData->SetBool(MCHIK_DIRTY_IK,true);
	if(CDIsDirty(eff,CD_DIRTY_MATRIX)) tData->SetBool(MCHIK_DIRTY_IK,true);
	
	Matrix opM = op->GetMg(), chM = ch->GetMg(), gchM = gch->GetMg(), effM = eff->GetMg();
	Matrix goalM = goal->GetMg(), poleM = pole->GetMg();
	Matrix rpM, twistM, targM, transM;
	
	Real ikMix = tData->GetReal(MCHIK_BLEND);
	if(ikMix != tData->GetReal(MCHIK_OLD_IK_BLEND)) tData->SetBool(MCHIK_DIRTY_IK,true);
	if(ikMix > 0 && !VEqual(gch->GetMg().off,goal->GetMg().off,0.01)) tData->SetBool(MCHIK_DIRTY_IK,true);
	
	Real twist = tData->GetReal(MCHIK_TWIST);
	
	if(!tData->GetBool(MCHIK_USE))
	{
		effM = eff->GetMg();
		
		goalM.off = effM.off;
		goal->SetMg(goalM);
		goal->Message(MSG_UPDATE);
		
		Matrix pvrM = tData->GetMatrix(MCHIK_PVR_MATRIX);
		
		poleM.off = opM * pvrM.off;
		pole->SetMg(poleM);
		pole->Message(MSG_UPDATE);
		
		if(gch->GetTag(ID_CDFOOTIKPLUGIN)) SetFootIKData(gch,tData);
		
		return true;
	}
	
	if(!tData->GetBool(MCHIK_REST_ROTATION_SET))
	{
		tData->SetVector(MCHIK_REST_ROTATION,CDGetRot(op));
		tData->SetBool(MCHIK_REST_ROTATION_SET,true);
	}
	
	Vector opPos = CDGetPos(op), opScale = CDGetScale(op), opRot = CDGetRot(op);
	Vector chPos = CDGetPos(ch), chScale = CDGetScale(ch), chRot = CDGetRot(ch);
	Vector gchPos = CDGetPos(gch), gchScale = CDGetScale(gch), gchRot = CDGetRot(gch);
	Vector effPos = CDGetPos(eff);
	
	Vector rAxis, fkP1, fkP2, fkP3, newRot, rotSet; 

	// check plane points for 0 distance
	if(poleM.off == opM.off) return false;
	if(goalM.off == opM.off) return false;
	if(goalM.off == poleM.off) return false;
	if(poleM.off == chM.off) return false;
	if(goalM.off == chM.off) return false;
	
	//	Set the vectors to the Pole and Goal
	poleV = poleM.off - opM.off;
	goalV = goalM.off - opM.off;
	
	if(!VEqual(poleV,tData->GetVector(MCHIK_P_VECTOR),0.001)) tData->SetBool(MCHIK_DIRTY_IK,true);
	if(!VEqual(goalV,tData->GetVector(MCHIK_G_VECTOR),0.001)) tData->SetBool(MCHIK_DIRTY_IK,true);
	
	if(tData->GetBool(MCHIK_LOCK_UPPER))
	{
		Real rootLen = Len(chM.off - opM.off);
		Real gchLen = Len(gchM.off - chM.off);
		Real effLen = Len(effM.off - gchM.off);
		
		Vector glPosL = MInv(opM) * goalM.off;
		glPosL.z = rootLen;
		Vector trgA = opM * glPosL;
		
		goalV = trgA - chM.off;
		rpM = GetRPMatrix(GetPoleAxis(tData),chM.off,goalV,poleV);
		
		Real rAngle;
		Real A = Len(gchM.off - chM.off);
		Real H = Len(goalV);
		if(H < A) H = A;
		rAngle = ACos(A/H);
		
		switch (tData->GetLong(MCHIK_POLE_AXIS))
		{
			case MCHIK_POLE_X:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				rAngle = -(rAngle);		// Negate the angle
				break;
			case MCHIK_POLE_Y:
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				break;
			case MCHIK_POLE_NX:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				break;
			case MCHIK_POLE_NY:
				rAngle = -(rAngle);		// Negate the angle
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				break;
		}
		targM = rpM * RotAxisToMatrix(rAxis,rAngle);
		transM = MixM(chM, targM, rAxis, ikMix);

		ch->SetMg(transM);
		CDSetScale(ch,chScale);
		CDSetPos(ch,chPos);
		
		chM = ch->GetMg();
		fkP1 = targM.off;
		rAngle = pi05;
		switch (tData->GetLong(MCHIK_POLE_AXIS))
		{
			case MCHIK_POLE_X:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				break;
			case MCHIK_POLE_Y:
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				rAngle = -(rAngle);		// Negate the angle
				break;
			case MCHIK_POLE_NX:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				rAngle = -(rAngle);		// Negate the angle
				break;
			case MCHIK_POLE_NY:
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				break;
		}
		
		Matrix m = targM * RotAxisToMatrix(rAxis,rAngle);
		m.off = targM.off + VNorm(targM.v3) * gchLen;
		targM = m;
		
		goalV = goalM.off - targM.off;
		switch (tData->GetLong(MCHIK_POLE_AXIS))
		{
			case MCHIK_POLE_X:	
				targM.v2 = VNorm(VCross(goalV, targM.v1));
				targM.v3 = VNorm(VCross(targM.v1, targM.v2));
				break;
			case MCHIK_POLE_Y:
				targM.v1 = VNorm(VCross(targM.v2, goalV));
				targM.v3 = VNorm(VCross(targM.v1, targM.v2));
				break;
			case MCHIK_POLE_NX:	
				targM.v2 = VNorm(VCross(goalV, targM.v1));
				targM.v3 = VNorm(VCross(targM.v1, targM.v2));
				break;
			case MCHIK_POLE_NY:
				targM.v1 = VNorm(VCross(targM.v2, goalV));
				targM.v3 = VNorm(VCross(targM.v1, targM.v2));
				break;
		}
		transM = MixM(gchM, targM, rAxis, ikMix);
		
		gch->SetMg(transM);
		CDSetScale(gch,gchScale);
		CDSetPos(gch,gchPos);
		
		gchM = gch->GetMg();
		fkP2 = targM.off;
		
		if(tData->GetBool(MCHIK_LOWER_SLIDE))
		{
			Real gLen = Len(goalV);
			Real min = tData->GetReal(MCHIK_SLIDE_MIN);
			Real max = tData->GetReal(MCHIK_SLIDE_MAX);
			if(gLen < min) gLen = min;
			if(gLen > max) gLen = max;
			effLen = gLen;
			
		}
		effM = gchM;
		effM.off = gchM.off + VNorm(gchM.v3) * effLen;
		eff->SetMg(effM);
		
		fkP3 = fkP2 + VNorm(targM.v3) * effLen;
		
		// Store the IK positions for drawing
		tData->SetVector(MCHIK_P1, fkP1);
		tData->SetVector(MCHIK_P2, fkP2);
		tData->SetVector(MCHIK_P3, fkP3);
	}
	else
	{
		tData->SetBool(MCHIK_LOWER_SLIDE,false);
		
		//	Get the Root joint Rotation Plane matrix.
		rpM = GetRootRPMatrix(GetPoleAxis(tData),opM.off,goalV,poleV);
		
		// calculate the Root joint angle
		Real flip = 1;
		if(tData->GetBool(MCHIK_ROOT_FLIP)) flip = -1;
		Real offset = tData->GetReal(MCHIK_ROOT_ANGLE);
		Real theta;
		Real A = Len(chM.off - opM.off);
		Real H = Len(goalV);
		
		if(H > 0.0) theta = (ACos(A/H) + offset) * flip;
		else theta = 0;
		
		switch (tData->GetLong(MCHIK_POLE_AXIS))
		{
			case MCHIK_POLE_X:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				targM = rpM * MatrixRotX(theta);
				break;
			case MCHIK_POLE_Y:
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				targM = rpM * MatrixRotY(theta);
				break;
			case MCHIK_POLE_NX:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				targM = rpM * MatrixRotX(-theta);
				break;
			case MCHIK_POLE_NY:
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				targM = rpM * MatrixRotY(-theta);
				break;
		}
		transM = MixM(opM, targM, rAxis, ikMix);
		
		op->SetMg(transM);
		CDSetScale(op,opScale);
		CDSetPos(op,opPos);
		
		// Set the Child's matrix.
		ch = op->GetDown(); if(!ch) return false;
		fkP1 = targM.off + targM.v3 * A;
		if(tData->GetBool(MCHIK_CONNECT_BONES))
		{
			chPos.x = 0.0;
			chPos.y = 0.0;
			if(CDIsBone(op)) chPos.z = GetBoneLength(op);
			CDSetPos(ch,chPos);
		}
		chM = ch->GetMg();
		
		//	Set the vectors to the Goal
		goalV = goalM.off - chM.off;

		// Get the IK joint Rotation Plane matrix
		rpM = GetRPMatrix(GetPoleAxis(tData),chM.off,goalV,poleV);
		
		// invert joint chain twist
		twistM = rpM * RotAxisToMatrix(Vector(0,0,1),twist);
		transM = rpM * (MInv(twistM) * chM);
		
		ch->SetMg(transM);
		CDSetScale(ch,chScale);
		CDSetPos(ch,chPos);

		chM = ch->GetMg();
		gchM = gch->GetMg();
		effM = eff->GetMg();

		// Calculate the IK angle of the parent object.
		Real aLen = Len(gchM.off - chM.off);
		Real bLen = Len(goalV);
		Real cLen = Len(effM.off - gchM.off);
		Real rAngle = ACos(((aLen*aLen)+(bLen*bLen)-(cLen*cLen))/(2*aLen*bLen));
		
		// Set Zero Angle Damping
		Real theMix = 1;
		Real theDamping = 0;
		if (tData->GetBool(MCHIK_DAMPING_ON))
		{
			theDamping = tData->GetReal(MCHIK_DAMPING_VALUE) / 3;
			if (rAngle < theDamping)
			{
				theMix = rAngle * (1 / theDamping);
			}
		}
		rAngle = CDBlend(0,rAngle,theMix);
		
		switch (tData->GetLong(MCHIK_POLE_AXIS))
		{
			case MCHIK_POLE_X:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				rAngle = -(rAngle);		// Negate the angle
				break;
			case MCHIK_POLE_Y:
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				break;
			case MCHIK_POLE_NX:	
				rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
				break;
			case MCHIK_POLE_NY:
				rAngle = -(rAngle);		// Negate the angle
				rAxis = Vector(1,0,0);	// Set the Rotation axis to X
				break;
		}
		targM = rpM * RotAxisToMatrix(rAxis,rAngle);
		transM = MixM(chM, targM, rAxis, ikMix);

		ch->SetMg(transM);
		CDSetScale(ch,chScale);
		CDSetPos(ch,chPos);
		
		chM = ch->GetMg();
		
		// Set the g-child's matrix.
		gch = ch->GetDown(); if(!gch) return false;
		fkP2 = fkP1 + targM.v3 * aLen;
		if(tData->GetBool(MCHIK_CONNECT_BONES))
		{
			gchPos.x = 0.0;
			gchPos.y = 0.0;
			if(CDIsBone(ch)) gchPos.z = GetBoneLength(ch);
			CDSetPos(gch,gchPos);
		}
		
		gchM = gch->GetMg();
		targM = gchM;
		targM.v3 = VNorm(goalM.off - fkP2);
		fkP3 = fkP2 + targM.v3 * cLen;
		switch (tData->GetLong(MCHIK_POLE_AXIS))
		{
			case MCHIK_POLE_X:	
				targM.v2 = rpM.v2;
				targM.v1 = VNorm(VCross(targM.v2, targM.v3));
				break;
			case MCHIK_POLE_Y:
				targM.v1 = rpM.v1;
				targM.v2 = VNorm(VCross(targM.v3, rpM.v1));
				break;
			case MCHIK_POLE_NX:	
				targM.v2 = rpM.v2;
				targM.v1 = VNorm(VCross(targM.v2, targM.v3));
				break;
			case MCHIK_POLE_NY:
				targM.v1 = rpM.v1;
				targM.v2 = VNorm(VCross(targM.v3, rpM.v1));
				break;
			default:
			break;
		}
		transM = MixM(gchM, targM, rAxis, ikMix);
		gch->SetMg(transM);
		newRot = CDGetRot(gch);

		if(ikMix > 0.0) rotSet = CDGetOptimalAngle(Vector(0,0,0), newRot, gch);
		else rotSet = CDGetOptimalAngle(gchRot, newRot, gch);

		
		CDSetRot(gch,rotSet);
		CDSetScale(gch,gchScale);
		CDSetPos(gch,gchPos);
		
		// Reset joints chain to the twist
		transM = twistM * (MInv(rpM) * chM);
		
		ch->SetMg(transM);
		newRot = CDGetRot(ch);

		if(ikMix > 0.0) rotSet = CDGetOptimalAngle(tData->GetVector(MCHIK_REST_ROTATION), newRot, ch);
		else rotSet = CDGetOptimalAngle(opRot, newRot, ch);

		
		CDSetRot(ch,rotSet);
		CDSetScale(ch,chScale);
		CDSetPos(ch,chPos);
		ch->Message(MSG_UPDATE);
		
		// Store the IK positions for drawing
		tData->SetVector(MCHIK_P1, fkP1);
		tData->SetVector(MCHIK_P2, twistM * (MInv(rpM) * fkP2));
		tData->SetVector(MCHIK_P3, twistM * (MInv(rpM) * fkP3));
	}

	effM = eff->GetMg();
	if(tData->GetBool(MCHIK_CONNECT_NEXT))
	{
		effPos.x = 0.0;
		effPos.y = 0.0;
		if(CDIsBone(gch)) effPos.z = GetBoneLength(gch);
		CDSetPos(eff,effPos);
	}
	
	if(eff->GetTag(ID_CDFOOTIKPLUGIN)) SetFootIKData(eff,tData);
	
	opM = op->GetMg();
	poleM = pole->GetMg();
	tData->SetMatrix(MCHIK_PVR_MATRIX,MInv(opM) * poleM);
	
	// touch values for dirty IK test
	tData->SetBool(MCHIK_DIRTY_IK,false);
	tData->SetVector(MCHIK_P_VECTOR,poleV);
	tData->SetVector(MCHIK_G_VECTOR,goalV);
	tData->SetReal(MCHIK_OLD_IK_BLEND,ikMix);

	return CD_EXECUTION_RESULT_OK;
}

Bool CDMechIKPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(MCHIK_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDMechIKPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case MCHIK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MCHIK_ROOT_ANGLE:	
			if(!tData->GetBool(MCHIK_LOCK_UPPER)) return true;
			else return false;
		case MCHIK_ROOT_FLIP:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(MCHIK_LOCK_UPPER)) return true;
				else return false;
			}
		case MCHIK_GOAL_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MCHIK_SOLVER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MCHIK_DAMPING_VALUE:	
			return tData->GetBool(MCHIK_DAMPING_ON);
		case MCHIK_LOWER_SLIDE:	
			return tData->GetBool(MCHIK_LOCK_UPPER);
		case MCHIK_SLIDE_MIN:	
			return tData->GetBool(MCHIK_LOWER_SLIDE);
		case MCHIK_SLIDE_MAX:	
			return tData->GetBool(MCHIK_LOWER_SLIDE);
	}
	return true;
}

Bool RegisterCDMechIKPlugin(void)
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
	String name=GeLoadString(IDS_CDMCHLIMB); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDMECHLIMBPLUGIN,name,info,CDMechIKPlugin::Alloc,"tCDMechIK","CDMechIK.tif",1);
}

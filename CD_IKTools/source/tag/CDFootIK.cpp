//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDFootIK.h"
#include "CDIKtools.h"

enum
{
	//FT_IK_SHOW_LINES		= 10001,
	//FT_IK_USE				= 10002,

	//FT_IK_POLE_AXIS		= 10003,
		//FT_IK_POLE_Y		= 10004,
		//FT_IK_POLE_X		= 10005,

	//FT_IK_SOLVER_LINK		= 10006,

	//FT_IK_GOAL_A_LINK		= 10007,
	//FT_IK_GOAL_B_LINK		= 10008,
	
	FT_PVR_MATRIX			= 10011,

	//FT_IK_SOLVER_GROUP		= 10014,
	//FT_IK_GOAL_GROUP		= 10015,
	//FT_IK_POLE_NX			= 10016,
	//FT_IK_POLE_NY			= 10017,
	
	FT_IK_POSITION			= 10018,
	FT_P1					= 10019,
	FT_P2					= 10020,
	
	//FT_LINE_COLOR			= 10050,
	
	FT_REST_ROTATION_SET		= 11000,
	FT_REST_ROTATION			= 11001,
	
	//FT_CONNECT_BONES		= 20000,
	
	//FT_IK_BLEND			= 20002,
};

class CDFootIKPlugin : public CDTagData
{
private:
	Vector poleV, goalV;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData);
	
	LONG GetPoleAxis(BaseContainer *tData);
	BaseTag* GetLinkedTag(BaseObject *op);

public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDFootIKPlugin); }
};

Bool CDFootIKPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(FT_IK_USE,true);
	tData->SetLong(FT_IK_POLE_AXIS,FT_IK_POLE_Y);
	tData->SetReal(FT_IK_BLEND,1.0);
	tData->SetBool(FT_IK_SHOW_LINES,true);
	tData->SetBool(FT_CONNECT_BONES,false);
	tData->SetVector(FT_LINE_COLOR, Vector(1,0,0));
	
	tData->SetBool(FT_REST_ROTATION_SET,false);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDFootIKPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *ch = op->GetDown(); if(!ch) return true;
	BaseObject *gch = ch->GetDown(); if(!gch) return true;
	BaseObject *pole = tData->GetObjectLink(FT_IK_SOLVER_LINK,doc); if(!pole) return true;
	BaseObject *goalA = tData->GetObjectLink(FT_IK_GOAL_A_LINK,doc); if(!goalA) return true;
	BaseObject *goalB = tData->GetObjectLink(FT_IK_GOAL_B_LINK,doc); if(!goalB) return true;

	Vector opPosition = op->GetMg().off, ikPos = opPosition;
	Vector polePosition = pole->GetMg().off;
	Vector goalAPosition = goalA->GetMg().off;
	
	if(!tData->GetBool(FT_IK_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(FT_LINE_COLOR));
	CDDrawLine(bd,polePosition, opPosition);
	
	Vector tipAPosition = ch->GetMg().off;
	CDDrawLine(bd,tipAPosition, goalAPosition);

	Vector tipBPosition, goalBPosition;
	goalBPosition = goalB->GetMg().off;
	tipBPosition = gch->GetMg().off;
	CDDrawLine(bd,tipBPosition, goalBPosition);
	CDDrawLine(bd,opPosition, tipAPosition);
	CDDrawLine(bd,tipAPosition, tipBPosition);

	BaseObject *linkedOp = op->GetPred();
	if(!linkedOp)
	{
		linkedOp = op->GetUp();
		if(linkedOp)
		{
			linkedOp = linkedOp->GetUp();
			if(linkedOp)
			{
				if(linkedOp->GetTag(ID_CDLIMBIKPLUGIN))
				{
					ikPos = tData->GetVector(FT_IK_POSITION);
				}
				else
				{
					linkedOp = linkedOp->GetUp();
					if(linkedOp)
					{
						if(linkedOp->GetTag(ID_CDQUADLEGPLUGIN))
						{
							ikPos = tData->GetVector(FT_IK_POSITION);
						}
					}
				}
			}
		}
	}
	Vector p1 = tData->GetVector(FT_P1);
	Vector p2 = tData->GetVector(FT_P2);
	ObjectColorProperties prop;
	if(tData->GetReal(FT_IK_BLEND) < 1.0)
	{
		op->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0,0.5));
		CDDrawLine(bd,ikPos, p1);
		ch->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0,0.5));
		CDDrawLine(bd,p1, p2);
	}
	
	return true;
}

void CDFootIKPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(FT_IK_POLE_AXIS+T_LST,tData->GetLong(FT_IK_POLE_AXIS));
			
			tData->SetLink(FT_IK_SOLVER_LINK+T_LST,tData->GetLink(FT_IK_SOLVER_LINK,doc));
			tData->SetLink(FT_IK_GOAL_A_LINK+T_LST,tData->GetLink(FT_IK_GOAL_A_LINK,doc));
			tData->SetLink(FT_IK_GOAL_B_LINK+T_LST,tData->GetLink(FT_IK_GOAL_B_LINK,doc));
			
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

Bool CDFootIKPlugin::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("CDLimbIKPlugin::Message");
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
	
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==FT_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}

	return true;
}

Bool CDFootIKPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(FT_IK_POLE_AXIS,tData->GetLong(FT_IK_POLE_AXIS+T_LST));
		
		tData->SetLink(FT_IK_SOLVER_LINK,tData->GetLink(FT_IK_SOLVER_LINK+T_LST,doc));
		tData->SetLink(FT_IK_GOAL_A_LINK,tData->GetLink(FT_IK_GOAL_A_LINK+T_LST,doc));
		tData->SetLink(FT_IK_GOAL_B_LINK,tData->GetLink(FT_IK_GOAL_B_LINK+T_LST,doc));
		
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

LONG CDFootIKPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(FT_IK_POLE_AXIS))
	{
		case FT_IK_POLE_X:	
			return POLE_X;
		case FT_IK_POLE_Y:
			return POLE_Y;
		case FT_IK_POLE_NX:	
			return POLE_NX;
		case FT_IK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

BaseTag* CDFootIKPlugin::GetLinkedTag(BaseObject *op)
{
	BaseTag *lnkTag = NULL;
	
	BaseObject *linkedOp = op->GetPred();
	if(!linkedOp)
	{
		linkedOp = op->GetUp();
		if(linkedOp)
		{
			linkedOp = linkedOp->GetUp();
			if(linkedOp)
			{
				if(linkedOp->GetTag(ID_CDLIMBIKPLUGIN))
				{
					lnkTag = linkedOp->GetTag(ID_CDLIMBIKPLUGIN);
				}
				else
				{
					linkedOp = linkedOp->GetUp();
					if(linkedOp)
					{
						if(linkedOp->GetTag(ID_CDQUADLEGPLUGIN))
						{
							lnkTag = linkedOp->GetTag(ID_CDQUADLEGPLUGIN);
						}
						else if(linkedOp->GetTag(ID_CDMECHLIMBPLUGIN))
						{
							lnkTag = linkedOp->GetTag(ID_CDMECHLIMBPLUGIN);
						}
					}
				}
			}
		}
	}
	
	return lnkTag;
}

Bool CDFootIKPlugin::CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData)
{
	BaseObject *ch = op->GetDown();
	if(!ch)
	{
		tData->SetBool(FT_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *gch = ch->GetDown();
	if(!gch)
	{
		tData->SetBool(FT_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *pole = tData->GetObjectLink(FT_IK_SOLVER_LINK,doc);
	if(!pole)
	{
		tData->SetBool(FT_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *goalA = tData->GetObjectLink(FT_IK_GOAL_A_LINK,doc);
	if(!goalA)
	{
		tData->SetBool(FT_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *goalB = tData->GetObjectLink(FT_IK_GOAL_B_LINK,doc);
	if(!goalB)
	{
		tData->SetBool(FT_REST_ROTATION_SET,false);
		return false;
	}
	
	return true;
}

LONG CDFootIKPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	if(!CheckLinks(op,doc,tData)) return false;

	BaseObject *ch = op->GetDown();
	BaseObject *gch = ch->GetDown();
	BaseObject *pole = tData->GetObjectLink(FT_IK_SOLVER_LINK,doc);
	BaseObject *goalA = tData->GetObjectLink(FT_IK_GOAL_A_LINK,doc);
	BaseObject *goalB = tData->GetObjectLink(FT_IK_GOAL_B_LINK,doc);

	Matrix opM = op->GetMg(), chM = ch->GetMg(), rpM, targM, transM;
	Matrix goalAM = goalA->GetMg(), goalBM = goalB->GetMg(), poleM = pole->GetMg();
	
	Real ikMix = tData->GetReal(FT_IK_BLEND);
	BaseTag *lnkTag = GetLinkedTag(op);
	
	Bool lnkTagEnbl = false;
	if(lnkTag) lnkTagEnbl = lnkTag->GetDataInstance()->GetBool(EXPRESSION_ENABLE);
	
	if(!tData->GetBool(FT_IK_USE))
	{
		Matrix gchM = gch->GetMg();
		Matrix pvrM = tData->GetMatrix(FT_PVR_MATRIX);
		
		poleM = opM * pvrM;
		pole->SetMg(poleM);
		pole->Message(MSG_UPDATE);
		
		goalAM = goalA->GetMg();
		goalAM.off = chM.off;
		goalA->SetMg(goalAM);
		goalA->Message(MSG_UPDATE);
		
		goalBM = goalB->GetMg();
		goalBM.off = gchM.off;
		goalB->SetMg(goalBM);
		goalB->Message(MSG_UPDATE);
		
		if(lnkTag)
		{
			BaseObject *lnkGoal = lnkTag->GetDataInstance()->GetObjectLink(10010,doc);
			if(lnkGoal)
			{
				Matrix lgM = lnkGoal->GetMg();
				lgM.off = opM.off;
				lnkGoal->SetMg(lgM);
				lnkGoal->Message(MSG_UPDATE);
				
			}
		}
		
		return true;
	}

	if(!tData->GetBool(FT_REST_ROTATION_SET))
	{
		tData->SetVector(FT_REST_ROTATION,CDGetRot(op));
		tData->SetBool(FT_REST_ROTATION_SET,true);
	}
	
	Vector theScale, rAxis, ikPos, oldRot, newRot, rotSet;
	Vector opPos = CDGetPos(op), chPos = CDGetPos(ch), gchPos = CDGetPos(gch);
	
	// check plane points for 0 distance
	if(poleM.off == opM.off) return false;
	if(goalAM.off == opM.off) return false;
	if(goalAM.off == poleM.off) return false;
	if(goalBM.off == chM.off) return false;
	if(goalBM.off == poleM.off) return false;
	if(goalBM.off == goalAM.off) return false;
	
	if(lnkTag && lnkTagEnbl) ikPos = tData->GetVector(FT_IK_POSITION);
	else ikPos = opM.off;
	
	//	Set the vectors to the Pole and Goal.
	poleV = poleM.off - ikPos;
	goalV = goalAM.off - ikPos;
	
	Real aLen = Len(chM.off - opM.off);
	Real cLen = Len(gch->GetMg().off - chM.off);
	
	//	Get the Rotation Plane matrix.
	rpM = GetRPMatrix(GetPoleAxis(tData),opM.off,goalV,poleV);

	switch (tData->GetLong(FT_IK_POLE_AXIS))
	{
		case FT_IK_POLE_X:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			break;
		case FT_IK_POLE_Y:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		case FT_IK_POLE_NX:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			break;
		case FT_IK_POLE_NY:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		default:
		break;
		
	}
	transM = MixM(opM, rpM, rAxis, ikMix);

	theScale = CDGetScale(op);		
	oldRot = CDGetRot(op);
	op->SetMg(transM);
	newRot = CDGetRot(op);
	
	if(ikMix > 0.0) rotSet = CDGetOptimalAngle(tData->GetVector(FT_REST_ROTATION), newRot, op);
	else rotSet = CDGetOptimalAngle(oldRot, newRot, op);
	
	CDSetRot(op,rotSet);
	CDSetScale(op,theScale);
	CDSetPos(op,opPos);	

	// Set the Child's matrix.
	ch = op->GetDown(); if(!ch) return false;
	tData->SetVector(FT_P1, ikPos + VNorm(rpM.v3) * aLen);
	if(tData->GetBool(FT_CONNECT_BONES))
	{
		chPos = CDGetPos(ch);
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(op)) chPos.z = GetBoneLength(op);
		CDSetPos(ch,chPos);
	}

	targM = chM;
	targM.v3 = VNorm(goalBM.off - tData->GetVector(FT_P1));
	tData->SetVector(FT_P2, tData->GetVector(FT_P1) + targM.v3 * cLen);
	switch (tData->GetLong(FT_IK_POLE_AXIS))
	{
		case FT_IK_POLE_X:
			targM.v1 = VNorm(VCross(transM.v2, targM.v3));
			targM.v2 = VNorm(VCross(targM.v3, targM.v1));
			break;
		case FT_IK_POLE_Y:
			targM.v2 = VNorm(VCross(targM.v3, transM.v1));
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case FT_IK_POLE_NX:	
			targM.v2 = transM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case FT_IK_POLE_NY:
			targM.v1 = transM.v1;
			targM.v2 = VNorm(VCross(targM.v3, transM.v1));
			break;
		default:
		break;
	}
	transM = MixM(chM, targM, rAxis, ikMix);

	theScale = CDGetScale(ch);
	oldRot = CDGetRot(ch);
			
	ch->SetMg(transM);
	newRot = CDGetRot(ch);

	if(ikMix > 0.0) rotSet = CDGetOptimalAngle(Vector(0,0,0), newRot, ch);
	else rotSet = CDGetOptimalAngle(oldRot, newRot, ch);

	
	CDSetRot(ch,rotSet);
	CDSetScale(ch,theScale);
	CDSetPos(ch,chPos);	
		
	opM = op->GetMg();
	poleM = pole->GetMg();
	tData->SetMatrix(FT_PVR_MATRIX,MInv(opM) * poleM);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDFootIKPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(FT_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDFootIKPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case FT_IK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FT_IK_SOLVER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FT_IK_GOAL_A_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FT_IK_GOAL_B_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDFootIKPlugin(void)
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
	String name=GeLoadString(IDS_CDFOOTIK); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDFOOTIKPLUGIN,name,info,CDFootIKPlugin::Alloc,"tCDFootIK","CDFoot_ik_tag.tif",0);
}

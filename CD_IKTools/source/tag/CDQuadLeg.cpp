//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDQuadLeg.h"
#include "CDIKtools.h"

enum
{
	//QDIK_LINE_COLOR			= 1500,
	//QDIK_TWIST				= 1501,
	QDIK_DISK_LEVEL			= 1502,

	//QDIK_STRETCH_GROUP	= 3000,
	//QDIK_SET_LENGTH			= 3001,
	//QDIK_CLEAR_LENGTH		= 3002,
	QDIK_LENGTH_SET			= 3003,
	
	//QDIK_SQUASH_N_STRETCH	= 3004,
	//QDIK_CLAMP_SQUASH		= 3005,
	//QDIK_SQUASH_DIST			= 3006,
	//QDIK_SET_SQUASH_DIST		= 3007,
	//QDIK_CLAMP_STRETCH		= 3008,
	//QDIK_STRETCH_DIST		= 3009,
	//QDIK_SET_STRETCH_DIST	= 3010,
	
	QDIK_TOTAL_LENGTH		= 3011,
	QDIK_J_NUMBER			= 3012,
	//QDIK_CHANGE_VOLUME		= 3013,
	//QDIK_USE_BIAS_CURVE		= 3014,
	//QDIK_BIAS_CURVE			= 3015,
	//QDIK_MIX_SQ_CLAMP			= 3016,
	//QDIK_MIX_ST_CLAMP			= 3017,
	//QDIK_RESET_LENGTH			= 3018,
	//QDIK_VOLUME_STRENGTH		= 3019,
	
	QDIK_JOINT_LENGTH		= 4000,

	//QDIK_SHOW_LINES		= 10001,
	//QDIK_USE			= 10002,

	//QDIK_LINE_TARGET		= 10003,
		//QDIK_LINE_ROOT	= 10004,
		//QDIK_LINE_TIP		= 10005,
		
	//QDIK_POLE_AXIS		= 10006,
		//QDIK_POLE_Y		= 10007,
		//QDIK_POLE_X		= 10008,
	
	//QDIK_SOLVER_LINK		= 10009,
		
	//QDIK_GOAL_LINK		= 10010,
	
	//QDIK_SOLVER_GROUP		= 10011,
	//QDIK_GOAL_GROUP		= 10012,

	QDIK_PVR_MATRIX				= 10014,
	QDIK_DIRTY_IK			= 10015,

	//QDIK_DAMPING_ON			= 10017,
	//QDIK_DAMPING_VALUE		= 10018,
	//QDIK_POLE_NX			= 10019,
	//QDIK_POLE_NY			= 10020,
	
	QDIK_G_VECTOR				= 10030,
	QDIK_P_VECTOR				= 10031,
	
	QDIK_REST_ROTATION_SET		= 11000,
	QDIK_REST_ROTATION			= 11001,
	
	//QDIK_CONNECT_BONES	= 20000,
	//QDIK_CONNECT_NEXT	= 20001,

	//QDIK_BLEND		= 20002,
	//QDIK_LOWER_ANGLE		= 20003,
	QDIK_P1				= 20004,
	QDIK_P2				= 20005,
	QDIK_P3				= 20006
};

class CDQuadLegPlugin : public CDTagData
{
private:
	Bool squNstrON, joints;
	Real squash, stretch;
	Vector poleV, goalV;

	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData);
	void InitSplineData(BaseContainer *tData);
	
	LONG GetPoleAxis(BaseContainer *tData);
	void SetFootIKData(BaseObject *op, BaseContainer *tData);
	Bool SetSquashAndStretch(BaseObject *op, BaseObject *ch, BaseObject *gch, BaseObject *eff, BaseContainer *tData);

public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDQuadLegPlugin); }
};

Bool CDQuadLegPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(QDIK_USE,true);
	tData->SetLong(QDIK_POLE_AXIS,QDIK_POLE_Y);
	tData->SetReal(QDIK_BLEND,1.0);
	tData->SetLong(QDIK_LINE_TARGET,QDIK_LINE_ROOT);
	tData->SetBool(QDIK_SHOW_LINES,true);
	tData->SetBool(QDIK_DAMPING_ON,false);
	tData->SetReal(QDIK_DAMPING_VALUE,0);
	tData->SetBool(QDIK_CONNECT_BONES,false);
	tData->SetBool(QDIK_CONNECT_NEXT,false);
	tData->SetVector(QDIK_LINE_COLOR, Vector(1,0,0));

	tData->SetReal(QDIK_MIX_SQ_CLAMP,1.0);
	tData->SetReal(QDIK_MIX_ST_CLAMP,1.0);
	tData->SetReal(QDIK_VOLUME_STRENGTH,1.0);
	
	tData->SetBool(QDIK_REST_ROTATION_SET,false);
	tData->SetBool(QDIK_DIRTY_IK,true);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	InitSplineData(tData);

	return true;
}

void CDQuadLegPlugin::InitSplineData(BaseContainer *tData)
{
	GeData spd(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);
	SplineData* sp = (SplineData*)spd.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
	if(sp)
	{
		sp->MakeCubicSpline(-1);
		sp->InsertKnot( 0.0, 0.5 );
		sp->InsertKnot( 1.0, 0.5 );
	#if API_VERSION < 13000
		sp->SetRound(0.0);
	#endif
	}
	tData->SetData(QDIK_BIAS_CURVE, spd);
	tData->SetBool(QDIK_DIRTY_IK,true);
}

Bool CDQuadLegPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(QDIK_DISK_LEVEL,level);
	
	return true;
}

Bool CDQuadLegPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *ch = op->GetDown(); if(!ch) return true;
	BaseObject *gch = ch->GetDown(); if(!gch) return true;
	BaseObject *eff = gch->GetDown(); if(!eff) return true;
	
	BaseObject *goal = tData->GetObjectLink(QDIK_GOAL_LINK,doc); if(!goal) return true;
	BaseObject *pole = tData->GetObjectLink(QDIK_SOLVER_LINK,doc); if(!pole) return true;
	
	Vector opPosition = op->GetMg().off;
	Vector childPosition = ch->GetMg().off;
	Vector polePosition = pole->GetMg().off;
	Vector goalPosition = goal->GetMg().off;
	
	if(!tData->GetBool(QDIK_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());

	bd->SetPen(tData->GetVector(QDIK_LINE_COLOR));
	CDDrawLine(bd,opPosition, goalPosition);
	switch (tData->GetLong(QDIK_LINE_TARGET))
	{
		case QDIK_LINE_ROOT:	
			CDDrawLine(bd,opPosition, polePosition);
			break;
		case QDIK_LINE_TIP:
			CDDrawLine(bd,polePosition, childPosition);
			break;
		default:
		break;
	}
	CDDrawLine(bd,opPosition, childPosition);
	CDDrawLine(bd,childPosition, gch->GetMg().off);
	CDDrawLine(bd,gch->GetMg().off, eff->GetMg().off);
	
	Vector p1 = tData->GetVector(QDIK_P1);
	Vector p2 = tData->GetVector(QDIK_P2);
	Vector p3 = tData->GetVector(QDIK_P3);
	ObjectColorProperties prop;
	if(tData->GetBool(QDIK_USE) && tData->GetReal(QDIK_BLEND) < 1.0)
	{
		op->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0.25,0));
		CDDrawLine(bd,opPosition, p1);
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

void CDQuadLegPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(QDIK_POLE_AXIS+T_LST,tData->GetLong(QDIK_POLE_AXIS));
			
			tData->SetLink(QDIK_SOLVER_LINK+T_LST,tData->GetLink(QDIK_SOLVER_LINK,doc));
			tData->SetLink(QDIK_GOAL_LINK+T_LST,tData->GetLink(QDIK_GOAL_LINK,doc));
			
			BaseObject *ch=NULL, *gch=NULL, *eff=NULL;
			if(!tData->GetBool(QDIK_LENGTH_SET))
			{
				ch = op->GetDown();
				if(ch)
				{
					tData->SetVector(T_CP,CDGetPos(ch));

					gch = ch->GetDown();
					if(gch)
					{
						tData->SetVector(T_GCP,CDGetPos(gch));
						
						eff = gch->GetDown();
						if(eff)
						{
							tData->SetVector(T_EFP,CDGetPos(eff));
						}
					}
				}
			}
			else
			{
				ch = op->GetDown();
				if(ch)
				{
					Vector chPos = CDGetPos(ch);
					chPos.z = tData->GetReal(QDIK_JOINT_LENGTH);
					tData->SetVector(T_CP,chPos);

					gch = ch->GetDown();
					if(gch)
					{
						Vector gchPos = CDGetPos(gch);
						gchPos.z = tData->GetReal(QDIK_JOINT_LENGTH+1);
						tData->SetVector(T_GCP,gchPos);
						
						eff = gch->GetDown();
						if(eff)
						{
							Vector effPos = CDGetPos(eff);
							effPos.z = tData->GetReal(QDIK_JOINT_LENGTH+2);
							tData->SetVector(T_EFP,effPos);
						}
					}
				}
			}
			tData->SetBool(QDIK_CHANGE_VOLUME+T_LST,tData->GetBool(QDIK_CHANGE_VOLUME));
			tData->SetBool(QDIK_USE_BIAS_CURVE+T_LST,tData->GetBool(QDIK_USE_BIAS_CURVE));
			
			tData->SetBool(QDIK_CLAMP_STRETCH+T_LST,tData->GetBool(QDIK_CLAMP_STRETCH));
			tData->SetReal(QDIK_STRETCH_DIST+T_LST,tData->GetReal(QDIK_STRETCH_DIST));
			tData->SetBool(QDIK_CLAMP_SQUASH+T_LST,tData->GetBool(QDIK_CLAMP_SQUASH));
			tData->SetReal(QDIK_SQUASH_DIST+T_LST,tData->GetReal(QDIK_SQUASH_DIST));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDQuadLegPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(tData->GetLong(QDIK_DISK_LEVEL) < 1) InitSplineData(tData);
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
	BaseObject *eff = gch->GetDown(); if(!eff) return true;
	BaseObject *goal = tData->GetObjectLink(QDIK_GOAL_LINK,doc); if(!goal) return true;
	Real goalDist = Len(goal->GetMg().off - op->GetMg().off);

	Vector gScale = GetGlobalScale(op);
	switch (type)
	{
		case MSG_CHANGE:
		{
			tData->SetBool(QDIK_DIRTY_IK,true);
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			tData->SetBool(QDIK_DIRTY_IK,true);
			break;
		}
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			if(tData->GetBool(QDIK_LENGTH_SET))
			{
				Vector *trnsSca = (Vector*)data;
				if(trnsSca)
				{
					Vector sca = *trnsSca;
					Vector lenV = VNorm(op->GetMg().v3) * tData->GetReal(QDIK_JOINT_LENGTH);
					lenV.x *= sca.x;
					lenV.y *= sca.y;
					lenV.z *= sca.z;
					tData->SetReal(QDIK_JOINT_LENGTH,Len(lenV));
					Real totalLen = Len(lenV);
					
					lenV = VNorm(ch->GetMg().v3) * tData->GetReal(QDIK_JOINT_LENGTH+1);
					lenV.x *= sca.x;
					lenV.y *= sca.y;
					lenV.z *= sca.z;
					tData->SetReal(QDIK_JOINT_LENGTH+1,Len(lenV));
					totalLen += Len(lenV);
					
					lenV = VNorm(gch->GetMg().v3) * tData->GetReal(QDIK_JOINT_LENGTH+2);
					lenV.x *= sca.x;
					lenV.y *= sca.y;
					lenV.z *= sca.z;
					tData->SetReal(QDIK_JOINT_LENGTH+2,Len(lenV));
					totalLen += Len(lenV);
					tData->SetReal(QDIK_TOTAL_LENGTH, totalLen);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==QDIK_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==QDIK_SET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					LONG jLenInd = QDIK_JOINT_LENGTH;
					Real jlength, totalLength = 0;
					
					jlength = CDGetPos(op->GetDown()).z;
					tData->SetReal(jLenInd,jlength);
					totalLength+=jlength;

					jlength = CDGetPos(ch->GetDown()).z;
					tData->SetReal(jLenInd+1,jlength);
					totalLength+=jlength;

					jlength = CDGetPos(gch->GetDown()).z;
					tData->SetReal(jLenInd+2,jlength);
					totalLength+=jlength;

					tData->SetBool(QDIK_LENGTH_SET, true);
					tData->SetReal(QDIK_TOTAL_LENGTH, totalLength);
				}
			}
			else if(dc->id[0].id==QDIK_CLEAR_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					LONG jLenInd = QDIK_JOINT_LENGTH;
					Vector chPos, gchPos, effPos;
					
					chPos = CDGetPos(ch);
					chPos.z = tData->GetReal(jLenInd);
					CDSetPos(ch,chPos);

					gchPos = CDGetPos(gch);
					gchPos.z = tData->GetReal(jLenInd+1);
					CDSetPos(gch,gchPos);

					effPos = CDGetPos(eff);
					effPos.z = tData->GetReal(jLenInd+2);
					CDSetPos(eff,effPos);

					tData->SetBool(QDIK_LENGTH_SET, false);
				}
			}
			else if(dc->id[0].id==QDIK_RESET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					LONG jLenInd = QDIK_JOINT_LENGTH;
					Real jlength, totalLength = 0;
					
					jlength = CDGetPos(op->GetDown()).z;
					tData->SetReal(jLenInd,jlength);
					totalLength+=jlength;

					jlength = CDGetPos(ch->GetDown()).z;
					tData->SetReal(jLenInd+1,jlength);
					totalLength+=jlength;

					jlength = CDGetPos(gch->GetDown()).z;
					tData->SetReal(jLenInd+2,jlength);
					totalLength+=jlength;

					tData->SetBool(QDIK_LENGTH_SET, true);
					tData->SetReal(QDIK_TOTAL_LENGTH, totalLength);
				}
			}
			else if(dc->id[0].id==QDIK_SET_SQUASH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					if(gScale.z != 0.0)
					{
						tData->SetReal(QDIK_SQUASH_DIST, tData->GetReal(QDIK_TOTAL_LENGTH) - goalDist / gScale.z);
					}
					else
					{
						tData->SetReal(QDIK_SQUASH_DIST, tData->GetReal(QDIK_TOTAL_LENGTH) - goalDist);
					}
					tData->SetBool(QDIK_CLAMP_SQUASH, true);
				}
			}
			else if(dc->id[0].id==QDIK_SET_STRETCH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					if(gScale.z != 0.0)
					{
						tData->SetReal(QDIK_STRETCH_DIST, goalDist / gScale.z - tData->GetReal(QDIK_TOTAL_LENGTH));
					}
					else
					{
						tData->SetReal(QDIK_STRETCH_DIST, goalDist - tData->GetReal(QDIK_TOTAL_LENGTH));
					}
					tData->SetBool(QDIK_CLAMP_STRETCH, true);
				}
			}
			break;
		}
	}

	return true;
}

LONG CDQuadLegPlugin::GetPoleAxis(BaseContainer *tData)
{
	LONG p = 1;
	
	switch(tData->GetLong(QDIK_POLE_AXIS))
	{
		case QDIK_POLE_X:	
			p = 0;
			break;
		case QDIK_POLE_Y:
			p = 1;
			break;
		case QDIK_POLE_NX:	
			p = 2;
			break;
		case QDIK_POLE_NY:
			p = 3;
			break;
	}
	
	return p;
}

Bool CDQuadLegPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(QDIK_POLE_AXIS,tData->GetLong(QDIK_POLE_AXIS+T_LST));
			
		tData->SetLink(QDIK_SOLVER_LINK,tData->GetLink(QDIK_SOLVER_LINK+T_LST,doc));
		tData->SetLink(QDIK_GOAL_LINK,tData->GetLink(QDIK_GOAL_LINK+T_LST,doc));
		
		tData->SetBool(QDIK_CHANGE_VOLUME,tData->GetBool(QDIK_CHANGE_VOLUME+T_LST));
		tData->SetBool(QDIK_USE_BIAS_CURVE,tData->GetBool(QDIK_USE_BIAS_CURVE+T_LST));
			
		tData->SetBool(QDIK_CLAMP_STRETCH,tData->GetBool(QDIK_CLAMP_STRETCH+T_LST));
		tData->SetReal(QDIK_STRETCH_DIST,tData->GetReal(QDIK_STRETCH_DIST+T_LST));
		tData->SetBool(QDIK_CLAMP_SQUASH,tData->GetBool(QDIK_CLAMP_SQUASH+T_LST));
		tData->SetReal(QDIK_SQUASH_DIST,tData->GetReal(QDIK_SQUASH_DIST+T_LST));
		
		BaseObject *ch = op->GetDown();
		if(ch)
		{
			CDSetPos(ch,tData->GetVector(T_CP));

			BaseObject *gch = ch->GetDown();
			if(gch)
			{
				CDSetPos(gch,tData->GetVector(T_GCP));
				
				BaseObject *eff = gch->GetDown();
				if(eff)
				{
					CDSetPos(eff,tData->GetVector(T_EFP));
				}
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

Bool CDQuadLegPlugin::SetSquashAndStretch(BaseObject *op, BaseObject *ch, BaseObject *gch, BaseObject *eff, BaseContainer *tData)
{
	Bool ssSet = false;
	
	LONG jLenInd = QDIK_JOINT_LENGTH;
	Vector gScale = GetGlobalScale(op), chPos, gchPos, effPos;
	Real sqLen = tData->GetReal(QDIK_SQUASH_DIST) * gScale.z;
	Real totalLen = tData->GetReal(QDIK_TOTAL_LENGTH) * gScale.z;
	Real stLen = tData->GetReal(QDIK_STRETCH_DIST) * gScale.z;
	Real jLength, splStart = 0, splEnd = 0;
	Real volume = tData->GetReal(QDIK_VOLUME_STRENGTH);
	
	stretch = 1;
	squash = 1;
	if(!tData->GetBool(QDIK_SQUASH_N_STRETCH) || !tData->GetBool(QDIK_LENGTH_SET))
	{
		if(squNstrON)
		{
			chPos = CDGetPos(ch);
			chPos.z = tData->GetReal(jLenInd);
			CDSetPos(ch,chPos);

			gchPos = CDGetPos(gch);
			gchPos.z = tData->GetReal(jLenInd+1);
			CDSetPos(gch,gchPos);
			
			effPos = CDGetPos(eff);
			effPos.z = tData->GetReal(jLenInd+2);
			CDSetPos(eff,effPos);

			squNstrON = false;
			ssSet = true;
			
			tData->SetBool(QDIK_DIRTY_IK,true);
		}
	}
	else
	{
		chPos = CDGetPos(ch);
		gchPos = CDGetPos(gch);
		effPos = CDGetPos(eff);
		
		Real ikMix = tData->GetReal(QDIK_BLEND);
		Real gLen = CDBlend((chPos.z+gchPos.z+effPos.z),Len(goalV),ikMix);
		
		if(totalLen != 0)
		{
			stretch = gLen/totalLen;
			if(tData->GetBool(QDIK_CLAMP_STRETCH) && gLen > totalLen + stLen)
			{
				stretch = CDBlend(gLen/totalLen,(totalLen + stLen)/totalLen,tData->GetReal(QDIK_MIX_ST_CLAMP));
			}
			if(tData->GetBool(QDIK_CLAMP_SQUASH) && gLen < totalLen - sqLen)
			{
				stretch = CDBlend(gLen/totalLen,(totalLen - sqLen)/totalLen,tData->GetReal(QDIK_MIX_SQ_CLAMP));
			}
		}
		
		chPos.z = tData->GetReal(jLenInd) * stretch;
		CDSetPos(ch,chPos);

		gchPos.z = tData->GetReal(jLenInd+1) * stretch;
		CDSetPos(gch,gchPos);
		
		effPos.z = tData->GetReal(jLenInd+2) * stretch;
		CDSetPos(eff,effPos);
		
		if(tData->GetBool(QDIK_CHANGE_VOLUME))
		{
			Real skinVol;
			if(stretch < 1)
			{
				skinVol = 0.5;
				squash = ((stretch * skinVol) - 1)/(skinVol-1);
				if(tData->GetBool(QDIK_USE_BIAS_CURVE)) squash *= 3.5;
				if(volume < 1)
				{
					Real sqVol = CDBlend(1.0,squash,volume);
					squash = sqVol;
				}
				else if(volume > 1) squash *= volume;
			}
			else if(stretch > 1)
			{
				skinVol = 0.25;
				squash = ((stretch - 1) * skinVol + 1)/stretch;
				if(tData->GetBool(QDIK_USE_BIAS_CURVE)) squash *= 0.1;
				if(volume < 1)
				{
					Real sqVol = CDBlend(1.0,squash,volume);
					squash = sqVol;
				}
				else if(volume > 1) squash *= 1/volume;
			}
		}
		
		squNstrON = true;
		ssSet = true;
	}
	SplineData *biasSpline = (SplineData*)tData->GetCustomDataType(QDIK_BIAS_CURVE,CUSTOMDATATYPE_SPLINE);
	if(biasSpline && totalLen != 0)
	{
		BaseContainer *opData = op->GetDataInstance();
		if(opData && op->IsInstanceOf(ID_CDJOINTOBJECT))
		{
			jLength = tData->GetReal(jLenInd);
			splEnd = jLength/totalLen;
			opData->SetReal(JNT_J_LENGTH,jLength);
			opData->SetBool(JNT_J_BASE,true);
			opData->SetReal(JNT_SPL_START,splStart);
			opData->SetReal(JNT_SPL_END,splEnd);
			
			opData->SetReal(JNT_STRETCH_VALUE,stretch);
			opData->SetReal(JNT_SQUASH_VALUE,squash);
			opData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
			opData->SetBool(JNT_USE_SPLINE,tData->GetBool(QDIK_USE_BIAS_CURVE));
			opData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(QDIK_VOLUME_STRENGTH));
		}
		
		BaseContainer *chData = ch->GetDataInstance();
		if(chData && ch->IsInstanceOf(ID_CDJOINTOBJECT))
		{
			jLength = tData->GetReal(jLenInd+1);
			splStart = splEnd;
			splEnd += jLength/totalLen;
			chData->SetReal(JNT_J_LENGTH,jLength);
			chData->SetBool(JNT_J_BASE,false);
			chData->SetReal(JNT_SPL_START,splStart);
			chData->SetReal(JNT_SPL_END,splEnd);
			
			chData->SetReal(JNT_STRETCH_VALUE,stretch);
			chData->SetReal(JNT_SQUASH_VALUE,squash);
			chData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
			chData->SetBool(JNT_USE_SPLINE,tData->GetBool(QDIK_USE_BIAS_CURVE));
			chData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(QDIK_VOLUME_STRENGTH));
		}
		
		BaseContainer *gchData = gch->GetDataInstance();
		if(gchData && gch->IsInstanceOf(ID_CDJOINTOBJECT))
		{
			jLength = tData->GetReal(jLenInd+2);
			splStart = splEnd;
			splEnd += jLength/totalLen;
			gchData->SetReal(JNT_J_LENGTH,jLength);
			gchData->SetBool(JNT_J_BASE,false);
			gchData->SetReal(JNT_SPL_START,splStart);
			gchData->SetReal(JNT_SPL_END,1.0);
			
			gchData->SetReal(JNT_STRETCH_VALUE,stretch);
			gchData->SetReal(JNT_SQUASH_VALUE,squash);
			gchData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
			gchData->SetBool(JNT_USE_SPLINE,tData->GetBool(QDIK_USE_BIAS_CURVE));
			gchData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(QDIK_VOLUME_STRENGTH));
		}
	}
	
	return ssSet;
}

void CDQuadLegPlugin::SetFootIKData(BaseObject *op, BaseContainer *tData)
{
	BaseTag *footIKTag = op->GetTag(ID_CDFOOTIKPLUGIN);
	BaseContainer *ftData = footIKTag->GetDataInstance();
	
	ftData->SetVector(10018,tData->GetVector(QDIK_P3));
	ftData->SetReal(QDIK_BLEND,tData->GetReal(QDIK_BLEND));
	ftData->SetBool(QDIK_USE,tData->GetBool(QDIK_USE));
	if(!tData->GetBool(QDIK_CONNECT_NEXT))
	{
		ftData->SetBool(QDIK_CONNECT_BONES,false);
	}
	else
	{
		ftData->SetBool(QDIK_CONNECT_BONES,true);
	}
}

Bool CDQuadLegPlugin::CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData)
{
	BaseObject *ch = op->GetDown();
	if(!ch)
	{
		tData->SetBool(QDIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *gch = ch->GetDown();
	if(!gch)
	{
		tData->SetBool(QDIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *eff = gch->GetDown();
	if(!eff)
	{
		tData->SetBool(QDIK_REST_ROTATION_SET,false);
		return false;
	}
	
	BaseObject *goal = tData->GetObjectLink(QDIK_GOAL_LINK,doc);
	if(!goal)
	{
		tData->SetBool(QDIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *pole = tData->GetObjectLink(QDIK_SOLVER_LINK,doc);
	if(!pole)
	{
		tData->SetBool(QDIK_REST_ROTATION_SET,false);
		return false;
	}
	
	return true;
}

LONG CDQuadLegPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	if(!CheckLinks(op,doc,tData)) return false;

	BaseObject *ch = op->GetDown();
	BaseObject *gch = ch->GetDown();
	BaseObject *eff = gch->GetDown();
	
	BaseObject *goal = tData->GetObjectLink(QDIK_GOAL_LINK,doc);
	BaseObject *pole = tData->GetObjectLink(QDIK_SOLVER_LINK,doc);

	if(CDIsDirty(op,CD_DIRTY_MATRIX)) tData->SetBool(QDIK_DIRTY_IK,true);
	if(CDIsDirty(ch,CD_DIRTY_MATRIX)) tData->SetBool(QDIK_DIRTY_IK,true);
	if(CDIsDirty(gch,CD_DIRTY_MATRIX)) tData->SetBool(QDIK_DIRTY_IK,true);
	if(CDIsDirty(eff,CD_DIRTY_MATRIX)) tData->SetBool(QDIK_DIRTY_IK,true);
	
	Matrix opM = op->GetMg(), chM = ch->GetMg(), gchM = gch->GetMg(), effM = eff->GetMg();
	Matrix goalM = goal->GetMg(), poleM = pole->GetMg();
	Matrix lrpM, rpM, twistM, targM, transM;
	
	Real ikMix = tData->GetReal(QDIK_BLEND);
	Real twist = tData->GetReal(QDIK_TWIST);
	
	if(!tData->GetBool(QDIK_USE))
	{
		gchM = gch->GetMg();
		
		goalM.off = gchM.off;
		goal->SetMg(goalM);
		goal->Message(MSG_UPDATE);
		
		Matrix pvrM = tData->GetMatrix(QDIK_PVR_MATRIX);
		
		poleM.off = opM * pvrM.off;
		pole->SetMg(poleM);
		pole->Message(MSG_UPDATE);
		
		if(eff->GetTag(ID_CDFOOTIKPLUGIN)) SetFootIKData(eff,tData);
		
		return true;
	}
	
	if(!tData->GetBool(QDIK_REST_ROTATION_SET))
	{
		tData->SetVector(QDIK_REST_ROTATION,CDGetRot(op));
		tData->SetBool(QDIK_REST_ROTATION_SET,true);
	}
	
	Vector opScale = CDGetScale(op), chScale = CDGetScale(ch), gchScale = CDGetScale(gch);
	Vector opRot = CDGetRot(op), chRot = CDGetRot(ch), gchRot = CDGetRot(gch), oldRot, newRot, rotSet;
	Vector opPos = CDGetPos(op), chPos = CDGetPos(ch), gchPos = CDGetPos(gch), effPos = CDGetPos(eff);
	Vector fkP1,fkP2,fkP3, rAxis;

	// check plane points for 0 distance
	if(poleM.off == opM.off) return false;
	if(goalM.off == opM.off) return false;
	if(goalM.off == poleM.off) return false;
	
	//	Set the vectors to the Pole and Goal and get the Pole vector length.
	poleV = poleM.off - opM.off;
	goalV = goalM.off - opM.off;
	
	if(!VEqual(poleV,tData->GetVector(QDIK_P_VECTOR),0.001)) tData->SetBool(QDIK_DIRTY_IK,true);
	if(!VEqual(goalV,tData->GetVector(QDIK_G_VECTOR),0.001)) tData->SetBool(QDIK_DIRTY_IK,true);
	
	//	Get the Plane of Rotation matrix.
	rpM = GetRPMatrix(GetPoleAxis(tData),opM.off,goalV,poleV);

	// Set squash & stretch parameters
	if(SetSquashAndStretch(op,ch,gch,eff,tData))
	{
		chPos.z = CDGetPos(ch).z;
		gchPos.z = CDGetPos(gch).z;
		effPos.z = CDGetPos(eff).z;
	}

	// invert joint chain twist
	twistM = rpM * RotAxisToMatrix(Vector(0,0,1),twist);
	transM = rpM * (MInv(twistM) * op->GetMg());
	
	op->SetMg(transM);
	CDSetScale(op,opScale);
	CDSetPos(op,opPos);

	opM = op->GetMg();
	chM = ch->GetMg();
	gchM = gch->GetMg();
	effM = eff->GetMg();

	//Calculate the lower limb angle
	Real aLen, bLen, cLen;
	Real theAngle = tData->GetReal(QDIK_LOWER_ANGLE);
	Real angleLen = Len(effM.off - gchM.off);
	
	Real b = angleLen + Len(chM.off - opM.off);
	Real c = Len(gchM.off - ch->GetMg().off);
	Real A = pi - theAngle - ASin((Sin(theAngle)/c)*b);
	aLen = Sqrt((b*b) + (c*c) - (2*c*b*Cos(A)));
	
	Real minVal = aLen;
	Real maxVal = Len(effM.off - gchM.off) + Len(gchM.off - chM.off) + Len(chM.off - opM.off);
	Real value = Len(goalV);
	Real theMix = 0;
	if(value > minVal)
	{
		if((maxVal-minVal) != 0) //Check for division by zero
		{
			theMix = (value-minVal)/(maxVal-minVal);
			theMix*=theMix;
		}
		else
		{
			theMix = 0;
		}
		if(theMix < 0) theMix = 0;
		if(theMix > 1) theMix = 1;
	}
	Real rAngle = CDBlend(theAngle, 0.0, theMix);
	
	switch (tData->GetLong(QDIK_POLE_AXIS))
	{
		case QDIK_POLE_X:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = -(rAngle);		// Negate the angle
			break;
		case QDIK_POLE_Y:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		case QDIK_POLE_NX:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			break;
		case QDIK_POLE_NY:
			rAngle = -(rAngle);		// Negate the angle
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		default:
		break;
		
	}
	//	Set the Plane of Rotation matrix.
	targM = rpM * RotAxisToMatrix(rAxis,rAngle);
	
	Vector direction =  -(targM.v3);
	Vector anglePosition = goalM.off + direction * angleLen;
	fkP2 = anglePosition;
	Vector limbVector = anglePosition - opM.off;
	
	// Calculate the IK angle of the parent object.
	aLen = Len(ch->GetMg().off - op->GetMg().off);
	bLen = Len(limbVector);
	cLen = Len(gch->GetMg().off - ch->GetMg().off);
	rAngle = ACos(((aLen*aLen)+(bLen*bLen)-(cLen*cLen))/(2*aLen*bLen));
	
	// Set Zero Angle Damping
	theMix = 1;
	Real theDamping = 0;
	if (tData->GetBool(QDIK_DAMPING_ON))
	{
		theDamping = tData->GetReal(QDIK_DAMPING_VALUE) / 3;
		if (rAngle < theDamping)
		{
			theMix = rAngle * (1 / theDamping);
		}
	}
	rAngle = CDBlend(0,rAngle,theMix);
	
	lrpM = rpM;
	switch (tData->GetLong(QDIK_POLE_AXIS))
	{
		case QDIK_POLE_X:	
			//	Set the the limb Rotation Plane matrix vectors.
			lrpM.v2 = VNorm(VCross(goalV, poleV));
			lrpM.v1 = VNorm(VCross(lrpM.v2, limbVector));
			lrpM.v3 = VNorm(VCross(lrpM.v1, lrpM.v2));

			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = -(rAngle);		// Negate the angle
			break;
		case QDIK_POLE_Y:
			//	Set the the limb Rotation Plane matrix vectors.
			lrpM.v1 = VNorm(VCross(poleV, goalV));
			lrpM.v2 = VNorm(VCross(limbVector, lrpM.v1));
			lrpM.v3 = VNorm(VCross(lrpM.v1, lrpM.v2));

			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		case QDIK_POLE_NX:	
			//	Set the the limb Rotation Plane matrix vectors.
			lrpM.v2 = VNorm(VCross(poleV, goalV));
			lrpM.v1 = VNorm(VCross(lrpM.v2, limbVector));
			lrpM.v3 = VNorm(VCross(lrpM.v1, lrpM.v2));

			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			break;
		case QDIK_POLE_NY:
			//	Set the the limb Rotation Plane matrix vectors.
			lrpM.v1 = VNorm(VCross(goalV, poleV));
			lrpM.v2 = VNorm(VCross(limbVector, lrpM.v1));
			lrpM.v3 = VNorm(VCross(lrpM.v1, lrpM.v2));

			rAngle = -(rAngle);		// Negate the angle
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		default:
		break;
		
	}
	targM = lrpM * RotAxisToMatrix(rAxis,rAngle);
	transM = MixM(opM, targM, rAxis, ikMix);

	op->SetMg(transM);
	CDSetScale(op,opScale);
	CDSetPos(op,opPos);	

	joints = true;	
	if(op->GetType() != ID_CDJOINTOBJECT) joints = false;

	// Set the Child's matrix.
	fkP1 = targM.off + targM.v3 * aLen;
	if(tData->GetBool(QDIK_CONNECT_BONES))
	{
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(op)) chPos.z = GetBoneLength(op);
		CDSetPos(ch,chPos);
	}
	
	targM = chM;
	targM.v3 = VNorm(anglePosition - fkP1);
	fkP2 = fkP1 + targM.v3 * cLen;
	switch (tData->GetLong(QDIK_POLE_AXIS))
	{
		case QDIK_POLE_X:	
			targM.v2 = lrpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case QDIK_POLE_Y:
			targM.v1 = lrpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, lrpM.v1));
			break;
		case QDIK_POLE_NX:	
			targM.v2 = lrpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case QDIK_POLE_NY:
			targM.v1 = lrpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, lrpM.v1));
			break;
		default:
		break;
	}
	transM = MixM(chM, targM, rAxis, ikMix);
	
	ch->SetMg(transM);
	newRot = CDGetRot(ch);

	if(ikMix > 0.0)  rotSet = CDGetOptimalAngle(Vector(0,0,0), newRot, ch);
	else rotSet = CDGetOptimalAngle(chRot, newRot, ch);

	
	CDSetRot(ch,rotSet);
	CDSetScale(ch,chScale);
	CDSetPos(ch,chPos);	

	if(ch->GetType() != ID_CDJOINTOBJECT) joints = false;
		
	// Set the Grand Child's matrix.
	if(tData->GetBool(QDIK_CONNECT_BONES))
	{
		gchPos.x = 0.0;
		gchPos.y = 0.0;
		if(CDIsBone(ch)) gchPos.z = GetBoneLength(ch);
		CDSetPos(gch,gchPos);
	}

	targM = gchM;
	targM.v3 = VNorm(goalM.off - fkP2);
	fkP3 = fkP2 + targM.v3 * angleLen;
	switch (tData->GetLong(QDIK_POLE_AXIS))
	{
		case QDIK_POLE_X:	
			targM.v2 = lrpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case QDIK_POLE_Y:
			targM.v1 = lrpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, lrpM.v1));
			break;
		case QDIK_POLE_NX:	
			targM.v2 = lrpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case QDIK_POLE_NY:
			targM.v1 = lrpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, lrpM.v1));
			break;
		default:
		break;
	}
	transM = MixM(gchM, targM, rAxis, ikMix);

	gch->SetMg(transM);
	newRot = CDGetRot(gch);

	if(ikMix > 0.0)  rotSet = CDGetOptimalAngle(Vector(0,0,0), newRot, gch);
	else rotSet = CDGetOptimalAngle(gchRot, newRot, gch);

	
	CDSetRot(gch,rotSet);
	CDSetScale(gch,gchScale);
	CDSetPos(gch,gchPos);	
	
	if(gch->GetType() != ID_CDJOINTOBJECT) joints = false;
	
	// Reset joint chain twist
	opM = twistM * (MInv(rpM) * op->GetMg());

	op->SetMg(opM);
	newRot = CDGetRot(op);

	if(ikMix > 0.0) rotSet = CDGetOptimalAngle(tData->GetVector(QDIK_REST_ROTATION), newRot, op);
	else rotSet = CDGetOptimalAngle(opRot, newRot, op);

	
	CDSetRot(op,rotSet);
	CDSetScale(op,opScale);
	CDSetPos(op,opPos);

	// Store the fk positions;
	tData->SetVector(QDIK_P1, twistM * (MInv(rpM) * fkP1));
	tData->SetVector(QDIK_P2, twistM * (MInv(rpM) * fkP2));
	tData->SetVector(QDIK_P3, twistM * (MInv(rpM) * fkP3));

	if(tData->GetBool(QDIK_CONNECT_NEXT))
	{
		effPos.x = 0.0;
		effPos.y = 0.0;
		if(CDIsBone(gch)) effPos.z = GetBoneLength(gch);
		CDSetPos(eff,effPos);
	}
	
	if(eff->GetTag(ID_CDFOOTIKPLUGIN)) SetFootIKData(eff,tData);
	
	opM = op->GetMg();
	poleM = pole->GetMg();
	tData->SetMatrix(QDIK_PVR_MATRIX,MInv(opM) * poleM);
	tData->SetBool(QDIK_DIRTY_IK,false);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDQuadLegPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(QDIK_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDQuadLegPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case QDIK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case QDIK_GOAL_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case QDIK_SOLVER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case QDIK_LINE_TARGET:	
			return tData->GetBool(QDIK_SHOW_LINES);
		case QDIK_DAMPING_VALUE:	
			return tData->GetBool(QDIK_DAMPING_ON);
		case QDIK_SET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(!tData->GetBool(QDIK_LENGTH_SET)) return true;
				else return false;
			}
		case QDIK_CLEAR_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(tData->GetBool(QDIK_LENGTH_SET)) return true;
				else return false;
			}
		case QDIK_RESET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(QDIK_LENGTH_SET)) return true;
				else return false;
			}
		case QDIK_SQUASH_N_STRETCH:
			if(tData->GetBool(QDIK_LENGTH_SET)) return true;
			else return false;
		case QDIK_CHANGE_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(tData->GetBool(QDIK_LENGTH_SET) && tData->GetBool(QDIK_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case QDIK_VOLUME_STRENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(QDIK_LENGTH_SET) && tData->GetBool(QDIK_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case QDIK_CLAMP_SQUASH:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(tData->GetBool(QDIK_LENGTH_SET) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case QDIK_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(tData->GetBool(QDIK_LENGTH_SET) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case QDIK_SET_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(!tData->GetBool(QDIK_CLAMP_SQUASH) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case QDIK_MIX_SQ_CLAMP:
			if(tData->GetBool(QDIK_CLAMP_SQUASH) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
			else return false;
		case QDIK_CLAMP_STRETCH:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(tData->GetBool(QDIK_LENGTH_SET) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case QDIK_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(tData->GetBool(QDIK_LENGTH_SET) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case QDIK_SET_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			{
				if(!tData->GetBool(QDIK_CLAMP_STRETCH) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case QDIK_MIX_ST_CLAMP:
			if(tData->GetBool(QDIK_CLAMP_SQUASH) && tData->GetBool(QDIK_SQUASH_N_STRETCH)) return true;
			else return false;
		case QDIK_USE_BIAS_CURVE:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(QDIK_CHANGE_VOLUME);
		case QDIK_BIAS_CURVE:
			return tData->GetBool(QDIK_USE_BIAS_CURVE);
	}
	return true;
}

Bool RegisterCDQuadLegPlugin(void)
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
	String name=GeLoadString(IDS_CDQUADLEG); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDQUADLEGPLUGIN,name,info,CDQuadLegPlugin::Alloc,"tCDQuadLeg","CDQuadLeg.tif",1);
}

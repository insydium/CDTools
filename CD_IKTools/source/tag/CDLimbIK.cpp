//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDLimbIK.h"
#include "CDIKtools.h"

enum
{
	//LMBIK_LINE_COLOR			= 1500,
	//LMBIK_TWIST					= 1501,
	LMBIK_DISK_LEVEL				= 1502,

	//LMBIK_STRETCH_GROUP		= 3000,
	//LMBIK_SET_LENGTH			= 3001,
	//LMBIK_CLEAR_LENGTH			= 3002,
	LMBIK_LENGTH_SET				= 3003,
	
	//LMBIK_SQUASH_N_STRETCH		= 3004,
	//LMBIK_CLAMP_SQUASH			= 3005,
	//LMBIK_SQUASH_DIST			= 3006,
	//LMBIK_SET_SQUASH_DIST		= 3007,
	//LMBIK_CLAMP_STRETCH			= 3008,
	//LMBIK_STRETCH_DIST			= 3009,
	//LMBIK_SET_STRETCH_DIST		= 3010,
	
	LMBIK_TOTAL_LENGTH			= 3011,
	LMBIK_J_NUMBER				= 3012,
	//LMBIK_CHANGE_VOLUME			= 3013,
	//LMBIK_USE_BIAS_CURVE		= 3014,
	//LMBIK_BIAS_CURVE			= 3015,
	//LMBIK_MIX_SQ_CLAMP			= 3016,
	//LMBIK_MIX_ST_CLAMP			= 3017,
	//LMBIK_RESET_LENGTH			= 3018,
	//LMBIK_VOLUME_STRENGTH			= 3019,
	
	LMBIK_JOINT_LENGTH			= 4000,

	//LMBIK_SHOW_LINES			= 10001,
	//LMBIK_USE				= 10002,

	//LMBIK_LINE_TARGET		= 10003,
		//LMBIK_LINE_ROOT		= 10004,
		//LMBIK_LINE_TIP		= 10005,
		
	//LMBIK_POLE_AXIS			= 10006,
		//LMBIK_POLE_Y			= 10007,
		//LMBIK_POLE_X			= 10008,
	
	//LMBIK_SOLVER_LINK		= 10009,
		
	//LMBIK_GOAL_LINK			= 10010,
	
	//LMBIK_SOLVER_GROUP		= 10011,
	//LMBIK_GOAL_GROUP			= 10012,

	LMBIK_PVR_MATRIX				= 10014,
	LMBIK_DIRTY_IK				= 10015,
	LMBIK_OLD_IK_BLEND			= 10016,

	//LMBIK_DAMPING_ON			= 10017,
	//LMBIK_DAMPING_VALUE			= 10018,
	//LMBIK_POLE_NX			= 10019,
	//LMBIK_POLE_NY			= 10020,
	//LMBIK_GOAL_TO_BONE			= 10021,
	LMBIK_GOAL_DIR_MATRIX			= 10022,
	
	LMBIK_G_VECTOR				= 10030,
	LMBIK_P_VECTOR				= 10031,
	
	LMBIK_REST_ROTATION_SET		= 11000,
	LMBIK_REST_ROTATION			= 11001,
	
	//LMBIK_CONNECT_BONES			= 20000,
	//LMBIK_CONNECT_NEXT			= 20001,

	//LMBIK_BLEND				= 20002,
	LMBIK_P1						= 20003,
	LMBIK_P2						= 20004
};

class CDLimbIKPlugin : public CDTagData
{
private:
	Bool squNstrON, joints, undoOn;
	Real squash, stretch;
	Vector poleV, goalV;
	Matrix goalDirM;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData);
	void InitSplineData(BaseContainer *tData);
	
	LONG GetPoleAxis(BaseContainer *tData);
	void SetFootIKData(BaseObject *op, BaseContainer *tData);
	Bool SetSquashAndStretch(BaseObject *op, BaseObject *ch, BaseObject *gch, BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDLimbIKPlugin); }
};

Bool CDLimbIKPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(LMBIK_USE,true);
	tData->SetLong(LMBIK_POLE_AXIS,LMBIK_POLE_Y);
	tData->SetReal(LMBIK_BLEND,1.0);
	tData->SetLong(LMBIK_LINE_TARGET,LMBIK_LINE_ROOT);
	tData->SetBool(LMBIK_SHOW_LINES,true);
	tData->SetBool(LMBIK_DAMPING_ON,false);
	tData->SetReal(LMBIK_DAMPING_VALUE,0);
	tData->SetBool(LMBIK_CONNECT_BONES,false);
	tData->SetBool(LMBIK_CONNECT_NEXT,false);
	tData->SetVector(LMBIK_LINE_COLOR, Vector(1,0,0));

	tData->SetReal(LMBIK_MIX_SQ_CLAMP,1.0);
	tData->SetReal(LMBIK_MIX_ST_CLAMP,1.0);
	tData->SetReal(LMBIK_VOLUME_STRENGTH,1.0);
	
	tData->SetBool(LMBIK_REST_ROTATION_SET,false);
	tData->SetBool(LMBIK_DIRTY_IK,true);

	squNstrON = false;
	goalDirM = Matrix();
	undoOn = false;

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	InitSplineData(tData);

	return true;
}

void CDLimbIKPlugin::InitSplineData(BaseContainer *tData)
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
	tData->SetData(LMBIK_BIAS_CURVE, spd);
	
	tData->SetBool(LMBIK_DIRTY_IK,true);
}

Bool CDLimbIKPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(LMBIK_DISK_LEVEL,level);
	
	return true;
}

Bool CDLimbIKPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *ch = op->GetDown(); if(!ch) return true;
	BaseObject *gch = ch->GetDown(); if(!gch) return true;
	
	BaseObject *goal = tData->GetObjectLink(LMBIK_GOAL_LINK,doc); if(!goal) return true;
	BaseObject *pole = tData->GetObjectLink(LMBIK_SOLVER_LINK,doc); if(!pole) return true;
	
	Vector opPosition = op->GetMg().off;
	Vector childPosition = ch->GetMg().off;
	Vector polePosition = pole->GetMg().off;
	Vector goalPosition = goal->GetMg().off;
	
	if(!tData->GetBool(LMBIK_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());

	bd->SetPen(tData->GetVector(LMBIK_LINE_COLOR));
	CDDrawLine(bd,opPosition, goalPosition);
	switch (tData->GetLong(LMBIK_LINE_TARGET))
	{
		case LMBIK_LINE_ROOT:	
			CDDrawLine(bd,opPosition, polePosition);
			break;
		case LMBIK_LINE_TIP:
			CDDrawLine(bd,polePosition, childPosition);
			break;
		default:
		break;
	}
	CDDrawLine(bd,opPosition, childPosition);
	CDDrawLine(bd,childPosition, ch->GetDown()->GetMg().off);
	
	Vector p1 = tData->GetVector(LMBIK_P1); 
	Vector p2 = tData->GetVector(LMBIK_P2);
	ObjectColorProperties prop;
	if(tData->GetBool(LMBIK_USE) && tData->GetReal(LMBIK_BLEND) < 1.0)
	{
		op->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0,0.5));
		CDDrawLine(bd,opPosition, p1);
		ch->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0,0.5));
		CDDrawLine(bd,p1, p2);
	}
	
	return true;
}

void CDLimbIKPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(LMBIK_POLE_AXIS+T_LST,tData->GetLong(LMBIK_POLE_AXIS));
			
			tData->SetLink(LMBIK_SOLVER_LINK+T_LST,tData->GetLink(LMBIK_SOLVER_LINK,doc));
			tData->SetLink(LMBIK_GOAL_LINK+T_LST,tData->GetLink(LMBIK_GOAL_LINK,doc));
			
			BaseObject *ch=NULL, *gch=NULL;
			if(!tData->GetBool(LMBIK_LENGTH_SET))
			{
				ch = op->GetDown();
				if(ch)
				{
					tData->SetVector(T_CP,CDGetPos(ch));

					gch = ch->GetDown();
					if(gch)
					{
						tData->SetVector(T_GCP,CDGetPos(gch));
					}
				}
			}
			else
			{
				ch = op->GetDown();
				if(ch)
				{
					Vector chPos = CDGetPos(ch);
					chPos.z = tData->GetReal(LMBIK_JOINT_LENGTH);
					tData->SetVector(T_CP,chPos);

					gch = ch->GetDown();
					if(gch)
					{
						Vector gchPos = CDGetPos(gch);
						gchPos.z = tData->GetReal(LMBIK_JOINT_LENGTH+1);
						tData->SetVector(T_GCP,gchPos);
					}
				}
			}
			
			tData->SetBool(LMBIK_CHANGE_VOLUME+T_LST,tData->GetBool(LMBIK_CHANGE_VOLUME));
			tData->SetBool(LMBIK_USE_BIAS_CURVE+T_LST,tData->GetBool(LMBIK_USE_BIAS_CURVE));
			
			tData->SetBool(LMBIK_CLAMP_STRETCH+T_LST,tData->GetBool(LMBIK_CLAMP_STRETCH));
			tData->SetReal(LMBIK_STRETCH_DIST+T_LST,tData->GetReal(LMBIK_STRETCH_DIST));
			tData->SetBool(LMBIK_CLAMP_SQUASH+T_LST,tData->GetBool(LMBIK_CLAMP_SQUASH));
			tData->SetReal(LMBIK_SQUASH_DIST+T_LST,tData->GetReal(LMBIK_SQUASH_DIST));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDLimbIKPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(tData->GetLong(LMBIK_DISK_LEVEL) < 1) InitSplineData(tData);
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
	BaseObject *goal = tData->GetObjectLink(LMBIK_GOAL_LINK,doc); if(!goal) return true;
	Real goalDist = Len(goal->GetMg().off - op->GetMg().off);

	if(!tData->GetBool(LMBIK_GOAL_TO_BONE))
	{
		goalDirM = MInv(ch->GetMg()) * goal->GetMg();
		tData->SetMatrix(LMBIK_GOAL_DIR_MATRIX,goalDirM);
	}
	
	Vector gScale = GetGlobalScale(op);
	switch (type)
	{
		case MSG_CHANGE:
		{
			tData->SetBool(LMBIK_DIRTY_IK,true);
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			tData->SetBool(LMBIK_DIRTY_IK,true);
			break;
		}
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			if(tData->GetBool(LMBIK_LENGTH_SET))
			{
				Vector *trnsSca = (Vector*)data;
				if(trnsSca)
				{
					Vector sca = *trnsSca;
					Vector lenV = VNorm(op->GetMg().v3) * tData->GetReal(LMBIK_JOINT_LENGTH);
					lenV.x *= sca.x;
					lenV.y *= sca.y;
					lenV.z *= sca.z;
					tData->SetReal(LMBIK_JOINT_LENGTH,Len(lenV));
					Real totalLen = Len(lenV);
					
					lenV = VNorm(ch->GetMg().v3) * tData->GetReal(LMBIK_JOINT_LENGTH+1);
					lenV.x *= sca.x;
					lenV.y *= sca.y;
					lenV.z *= sca.z;
					tData->SetReal(LMBIK_JOINT_LENGTH+1,Len(lenV));
					totalLen += Len(lenV);
					tData->SetReal(LMBIK_TOTAL_LENGTH, totalLen);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==LMBIK_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==LMBIK_SET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					LONG jLenInd = LMBIK_JOINT_LENGTH;
					Real jlength, totalLength = 0;
					
					jlength = CDGetPos(op->GetDown()).z;
					tData->SetReal(jLenInd,jlength);
					totalLength+=jlength;

					jlength = CDGetPos(ch->GetDown()).z;
					tData->SetReal(jLenInd+1,jlength);
					totalLength+=jlength;

					tData->SetBool(LMBIK_LENGTH_SET, true);
					tData->SetReal(LMBIK_TOTAL_LENGTH, totalLength);
					tData->SetBool(LMBIK_DIRTY_IK,true);
				}
			}
			else if(dc->id[0].id==LMBIK_CLEAR_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					LONG jLenInd = LMBIK_JOINT_LENGTH;
					Vector chPos, gchPos;
					
					chPos = CDGetPos(ch);
					chPos.z = tData->GetReal(jLenInd);
					CDSetPos(ch,chPos);

					gchPos = CDGetPos(gch);
					gchPos.z = tData->GetReal(jLenInd+1);
					CDSetPos(gch,gchPos);

					tData->SetBool(LMBIK_LENGTH_SET, false);
					tData->SetBool(LMBIK_DIRTY_IK,true);
				}
			}
			else if(dc->id[0].id==LMBIK_RESET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					LONG jLenInd = LMBIK_JOINT_LENGTH;
					Real jlength, totalLength = 0;
					
					jlength = CDGetPos(op->GetDown()).z;
					tData->SetReal(jLenInd,jlength);
					totalLength+=jlength;

					jlength = CDGetPos(ch->GetDown()).z;
					tData->SetReal(jLenInd+1,jlength);
					totalLength+=jlength;

					tData->SetBool(LMBIK_LENGTH_SET, true);
					tData->SetReal(LMBIK_TOTAL_LENGTH, totalLength);
					tData->SetBool(LMBIK_DIRTY_IK,true);
				}
			}
			else if(dc->id[0].id==LMBIK_SET_SQUASH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					if(gScale.z != 0.0)
					{
						tData->SetReal(LMBIK_SQUASH_DIST, tData->GetReal(LMBIK_TOTAL_LENGTH) - goalDist / gScale.z);
					}
					else
					{
						tData->SetReal(LMBIK_SQUASH_DIST, tData->GetReal(LMBIK_TOTAL_LENGTH) - goalDist);
					}
					tData->SetBool(LMBIK_CLAMP_SQUASH, true);
					tData->SetBool(LMBIK_DIRTY_IK,true);
				}
			}
			else if(dc->id[0].id==LMBIK_SET_STRETCH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					if(gScale.z != 0.0)
					{
						tData->SetReal(LMBIK_STRETCH_DIST, goalDist / gScale.z - tData->GetReal(LMBIK_TOTAL_LENGTH));
					}
					else
					{
						tData->SetReal(LMBIK_STRETCH_DIST, goalDist - tData->GetReal(LMBIK_TOTAL_LENGTH));
					}
					tData->SetBool(LMBIK_CLAMP_STRETCH, true);
					tData->SetBool(LMBIK_DIRTY_IK,true);
				}
			}
			break;
		}
	}

	return true;
}

LONG CDLimbIKPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(LMBIK_POLE_AXIS))
	{
		case LMBIK_POLE_X:	
			return POLE_X;
		case LMBIK_POLE_Y:
			return POLE_Y;
		case LMBIK_POLE_NX:	
			return POLE_NX;
		case LMBIK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

Bool CDLimbIKPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(LMBIK_POLE_AXIS,tData->GetLong(LMBIK_POLE_AXIS+T_LST));
			
		tData->SetLink(LMBIK_SOLVER_LINK,tData->GetLink(LMBIK_SOLVER_LINK+T_LST,doc));
		tData->SetLink(LMBIK_GOAL_LINK,tData->GetLink(LMBIK_GOAL_LINK+T_LST,doc));
		
		tData->SetBool(LMBIK_CHANGE_VOLUME,tData->GetBool(LMBIK_CHANGE_VOLUME+T_LST));
		tData->SetBool(LMBIK_USE_BIAS_CURVE,tData->GetBool(LMBIK_USE_BIAS_CURVE+T_LST));
			
		tData->SetBool(LMBIK_CLAMP_STRETCH,tData->GetBool(LMBIK_CLAMP_STRETCH+T_LST));
		tData->SetReal(LMBIK_STRETCH_DIST,tData->GetReal(LMBIK_STRETCH_DIST+T_LST));
		tData->SetBool(LMBIK_CLAMP_SQUASH,tData->GetBool(LMBIK_CLAMP_SQUASH+T_LST));
		tData->SetReal(LMBIK_SQUASH_DIST,tData->GetReal(LMBIK_SQUASH_DIST+T_LST));
		
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

Bool CDLimbIKPlugin::SetSquashAndStretch(BaseObject *op, BaseObject *ch, BaseObject *gch, BaseContainer *tData)
{
	Bool ssSet = false;
	LONG jLenInd = LMBIK_JOINT_LENGTH;
	Vector gScale = GetGlobalScale(op), chPos, gchPos;
	Real sqLen = tData->GetReal(LMBIK_SQUASH_DIST) * gScale.z;
	Real totalLen = tData->GetReal(LMBIK_TOTAL_LENGTH) * gScale.z;
	Real stLen = tData->GetReal(LMBIK_STRETCH_DIST) * gScale.z;
	Real volume = tData->GetReal(LMBIK_VOLUME_STRENGTH);
	
	stretch = 1;
	squash = 1;
	if(!tData->GetBool(LMBIK_SQUASH_N_STRETCH) || !tData->GetBool(LMBIK_LENGTH_SET))
	{
		if(squNstrON)
		{
			chPos = CDGetPos(ch);
			chPos.z = tData->GetReal(jLenInd);
			CDSetPos(ch,chPos);

			gchPos = CDGetPos(gch);
			gchPos.z = tData->GetReal(jLenInd+1);
			CDSetPos(gch,gchPos);
			
			tData->SetBool(LMBIK_DIRTY_IK,true);
			squNstrON = false;
			ssSet = true;
		}
	}
	else
	{
		chPos = CDGetPos(ch);
		gchPos = CDGetPos(gch);
		
		Real ikMix = tData->GetReal(LMBIK_BLEND);
		Real gLen = CDBlend((chPos.z+gchPos.z),Len(goalV),ikMix);
		
		if(totalLen != 0)
		{
			stretch = gLen/totalLen;
			if(tData->GetBool(LMBIK_CLAMP_STRETCH) && gLen > totalLen + stLen)
			{
				stretch = CDBlend(gLen/totalLen,(totalLen + stLen)/totalLen,tData->GetReal(LMBIK_MIX_ST_CLAMP));
			}
			if(tData->GetBool(LMBIK_CLAMP_SQUASH) && gLen < totalLen - sqLen)
			{
				stretch = CDBlend(gLen/totalLen,(totalLen - sqLen)/totalLen,tData->GetReal(LMBIK_MIX_SQ_CLAMP));
			}
		}
		
		chPos.z = tData->GetReal(jLenInd) * stretch;
		CDSetPos(ch,chPos);

		gchPos.z = tData->GetReal(jLenInd+1) * stretch;
		CDSetPos(gch,gchPos);
		
		if(tData->GetBool(LMBIK_CHANGE_VOLUME))
		{
			Real skinVol;
			if(stretch < 1)
			{
				skinVol = 0.5;
				squash = ((stretch * skinVol) - 1)/(skinVol-1);
				if(tData->GetBool(LMBIK_USE_BIAS_CURVE)) squash *= 3.5;
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
				if(tData->GetBool(LMBIK_USE_BIAS_CURVE)) squash *= 0.1;
				if(volume < 1)
				{
					Real sqVol = CDBlend(1.0,squash,volume);
					squash = sqVol;
				}
				else if(volume > 1) squash *= 1/volume;
			}
		}
		
		if(!squNstrON)
		{
			squNstrON = true;
			tData->SetBool(LMBIK_DIRTY_IK,true);
		}
		ssSet = true;
	}
	SplineData *biasSpline = (SplineData*)tData->GetCustomDataType(LMBIK_BIAS_CURVE,CUSTOMDATATYPE_SPLINE);
	if(biasSpline && totalLen != 0)
	{
		BaseContainer *opData = op->GetDataInstance();
		if(opData && op->IsInstanceOf(ID_CDJOINTOBJECT))
		{
			opData->SetReal(JNT_J_LENGTH,tData->GetReal(jLenInd));
			opData->SetBool(JNT_J_BASE,true);
			opData->SetReal(JNT_SPL_START,0.0);
			opData->SetReal(JNT_SPL_END,tData->GetReal(jLenInd)/totalLen);
			
			opData->SetReal(JNT_STRETCH_VALUE,stretch);
			opData->SetReal(JNT_SQUASH_VALUE,squash);
			opData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
			opData->SetBool(JNT_USE_SPLINE,tData->GetBool(LMBIK_USE_BIAS_CURVE));
			opData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(LMBIK_VOLUME_STRENGTH));
		}
		
		BaseContainer *chData = ch->GetDataInstance();
		if(chData && ch->IsInstanceOf(ID_CDJOINTOBJECT))
		{
			chData->SetReal(JNT_J_LENGTH,tData->GetReal(jLenInd+1));
			chData->SetBool(JNT_J_BASE,false);
			chData->SetReal(JNT_SPL_START,tData->GetReal(jLenInd)/totalLen);
			chData->SetReal(JNT_SPL_END,1.0);
			
			chData->SetReal(JNT_STRETCH_VALUE,stretch);
			chData->SetReal(JNT_SQUASH_VALUE,squash);
			chData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
			chData->SetBool(JNT_USE_SPLINE,tData->GetBool(LMBIK_USE_BIAS_CURVE));
			chData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(LMBIK_VOLUME_STRENGTH));
		}
	}
	
	return ssSet;
}

void CDLimbIKPlugin::SetFootIKData(BaseObject *op, BaseContainer *tData)
{
	BaseTag *footIKTag = op->GetTag(ID_CDFOOTIKPLUGIN);
	BaseContainer *ftData = footIKTag->GetDataInstance();
	
	ftData->SetVector(10018,tData->GetVector(LMBIK_P2));
	ftData->SetReal(LMBIK_BLEND,tData->GetReal(LMBIK_BLEND));
	ftData->SetBool(LMBIK_USE,tData->GetBool(LMBIK_USE));
	if(!tData->GetBool(LMBIK_CONNECT_NEXT))
	{
		ftData->SetBool(LMBIK_CONNECT_BONES,false);
	}
	else
	{
		ftData->SetBool(LMBIK_CONNECT_BONES,true);
	}
}

Bool CDLimbIKPlugin::CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData)
{
	BaseObject *ch = op->GetDown();
	if(!ch)
	{
		tData->SetBool(LMBIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *gch = ch->GetDown();
	if(!gch)
	{
		tData->SetBool(LMBIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *goal = tData->GetObjectLink(LMBIK_GOAL_LINK,doc);
	if(!goal)
	{
		tData->SetBool(LMBIK_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *pole = tData->GetObjectLink(LMBIK_SOLVER_LINK,doc);
	if(!pole)
	{
		tData->SetBool(LMBIK_REST_ROTATION_SET,false);
		return false;
	}
	
	return true;
}

LONG CDLimbIKPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	if(!CheckLinks(op,doc,tData)) return false;
	
	BaseObject *ch = op->GetDown();
	BaseObject *gch = ch->GetDown();
	BaseObject *goal = tData->GetObjectLink(LMBIK_GOAL_LINK,doc);
	BaseObject *pole = tData->GetObjectLink(LMBIK_SOLVER_LINK,doc);

	if(CDIsDirty(op,CD_DIRTY_MATRIX)) tData->SetBool(LMBIK_DIRTY_IK,true);
	if(CDIsDirty(ch,CD_DIRTY_MATRIX)) tData->SetBool(LMBIK_DIRTY_IK,true);
	if(CDIsDirty(gch,CD_DIRTY_MATRIX)) tData->SetBool(LMBIK_DIRTY_IK,true);
	
	Matrix opM = op->GetMg(), chM = ch->GetMg(), gchM = gch->GetMg();
	Matrix goalM = goal->GetMg(), poleM = pole->GetMg();
	Matrix rpM, twistM, targM, transM;
	
	Real ikMix = tData->GetReal(LMBIK_BLEND);
	if(ikMix != tData->GetReal(LMBIK_OLD_IK_BLEND)) tData->SetBool(LMBIK_DIRTY_IK,true);
	if(ikMix > 0 && !VEqual(gch->GetMg().off,goal->GetMg().off,0.01)) tData->SetBool(LMBIK_DIRTY_IK,true);
	
	Real twist = tData->GetReal(LMBIK_TWIST);
	
	if(!tData->GetBool(LMBIK_USE))
	{
		gchM = gch->GetMg();
		
		goalM.off = gchM.off;
		goal->SetMg(goalM);
		goal->Message(MSG_UPDATE);
		
		Matrix pvrM = tData->GetMatrix(LMBIK_PVR_MATRIX);
		
		poleM.off = opM * pvrM.off;
		pole->SetMg(poleM);
		pole->Message(MSG_UPDATE);
		
		if(gch->GetTag(ID_CDFOOTIKPLUGIN)) SetFootIKData(gch,tData);
		
		return true;
	}
	
	if(!tData->GetBool(LMBIK_REST_ROTATION_SET))
	{
		tData->SetVector(LMBIK_REST_ROTATION,CDGetRot(op));
		tData->SetBool(LMBIK_REST_ROTATION_SET,true);
	}
	
	Vector opPos = CDGetPos(op), opScale = CDGetScale(op), opRot = CDGetRot(op);
	Vector chPos = CDGetPos(ch), chScale = CDGetScale(ch), chRot = CDGetRot(ch);
	Vector gchPos = CDGetPos(gch);
	
	Vector rAxis, fkP1, fkP2, newRot, rotSet; 

	// check plane points for 0 distance
	if(poleM.off == opM.off) return false;
	if(goalM.off == opM.off) return false;
	if(goalM.off == poleM.off) return false;
	
	//	Set the vectors to the Pole and Goal
	poleV = poleM.off - opM.off;
	goalV = goalM.off - opM.off;
	
	if(!VEqual(poleV,tData->GetVector(LMBIK_P_VECTOR),0.001)) tData->SetBool(LMBIK_DIRTY_IK,true);
	if(!VEqual(goalV,tData->GetVector(LMBIK_G_VECTOR),0.001)) tData->SetBool(LMBIK_DIRTY_IK,true);
	
	//	Get the Rotation Plane matrix.
	rpM = GetRPMatrix(GetPoleAxis(tData),opM.off,goalV,poleV);

	// Set squash & stretch parameters
	if(SetSquashAndStretch(op,ch,gch,tData))
	{
		chPos.z = CDGetPos(ch).z;
		gchPos.z = CDGetPos(gch).z;
	}
	
	// invert joint chain twist
	twistM = rpM * RotAxisToMatrix(Vector(0,0,1),twist);
	transM = rpM * (MInv(twistM) * opM);
	
	op->SetMg(transM);
	CDSetScale(op,opScale);
	CDSetPos(op,opPos);

	opM = op->GetMg();
	chM = ch->GetMg();
	gchM = gch->GetMg();

	// Calculate the IK angle of the parent object.
	Real aLen = Len(chM.off - opM.off);
	Real bLen = Len(goalV);
	Real cLen = Len(gchM.off - chM.off);
	Real rAngle = ACos(((aLen*aLen)+(bLen*bLen)-(cLen*cLen))/(2*aLen*bLen));
	
	// Set Zero Angle Damping
	Real theMix = 1;
	Real theDamping = 0;
	if (tData->GetBool(LMBIK_DAMPING_ON))
	{
		theDamping = tData->GetReal(LMBIK_DAMPING_VALUE) / 3;
		if (rAngle < theDamping)
		{
			theMix = rAngle * (1 / theDamping);
		}
	}
	rAngle = CDBlend(0,rAngle,theMix);
	
	switch (tData->GetLong(LMBIK_POLE_AXIS))
	{
		case LMBIK_POLE_X:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = -(rAngle);		// Negate the angle
			break;
		case LMBIK_POLE_Y:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		case LMBIK_POLE_NX:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			break;
		case LMBIK_POLE_NY:
			rAngle = -(rAngle);		// Negate the angle
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
	}
	targM = rpM * RotAxisToMatrix(rAxis,rAngle);
	transM = MixM(opM, targM, rAxis, ikMix);

	op->SetMg(transM);
	CDSetScale(op,opScale);
	CDSetPos(op,opPos);
	
	opM = op->GetMg();
	
	joints = true;	
	if(op->GetType() != ID_CDJOINTOBJECT) joints = false;

	// Set the Child's matrix.
	ch = op->GetDown(); if(!ch) return false;
	fkP1 = targM.off + targM.v3 * aLen;
	if(tData->GetBool(LMBIK_CONNECT_BONES))
	{
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(op)) chPos.z = GetBoneLength(op);
		CDSetPos(ch,chPos);
	}
	
	chM = ch->GetMg();
	targM = chM;
	targM.v3 = VNorm(goalM.off - fkP1);
	fkP2 = fkP1 + targM.v3 * cLen;
	switch (tData->GetLong(LMBIK_POLE_AXIS))
	{
		case LMBIK_POLE_X:	
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case LMBIK_POLE_Y:
			targM.v1 = rpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, rpM.v1));
			break;
		case LMBIK_POLE_NX:	
			targM.v2 = rpM.v2;
			targM.v1 = VNorm(VCross(targM.v2, targM.v3));
			break;
		case LMBIK_POLE_NY:
			targM.v1 = rpM.v1;
			targM.v2 = VNorm(VCross(targM.v3, rpM.v1));
			break;
		default:
		break;
	}
	transM = MixM(chM, targM, rAxis, ikMix);
	ch->SetMg(transM);
	newRot = CDGetRot(ch);

	if(ikMix > 0.0) rotSet = CDGetOptimalAngle(Vector(0,0,0), newRot, ch);
	else rotSet = CDGetOptimalAngle(chRot, newRot, ch);

	
	CDSetRot(ch,rotSet);
	CDSetScale(ch,chScale);
	CDSetPos(ch,chPos);
	
	if(ch->GetType() != ID_CDJOINTOBJECT) joints = false;
		
	
	// Reset joints chain to the twist
	transM = twistM * (MInv(rpM) * opM);
	
	op->SetMg(transM);
	newRot = CDGetRot(op);

	if(ikMix > 0.0) rotSet = CDGetOptimalAngle(tData->GetVector(LMBIK_REST_ROTATION), newRot, op);
	else rotSet = CDGetOptimalAngle(opRot, newRot, op);

	
	CDSetRot(op,rotSet);
	CDSetScale(op,opScale);
	CDSetPos(op,opPos);
	op->Message(MSG_UPDATE);
	
	// Store the IK positions for drawing
	tData->SetVector(LMBIK_P1, twistM * (MInv(rpM) * fkP1));
	tData->SetVector(LMBIK_P2, twistM * (MInv(rpM) * fkP2));
		
	gchM = gch->GetMg();
	if(tData->GetBool(LMBIK_CONNECT_NEXT))
	{
		gchPos.x = 0.0;
		gchPos.y = 0.0;
		if(CDIsBone(ch)) gchPos.z = GetBoneLength(ch);
		CDSetPos(gch,gchPos);
	}
	
	if(gch->GetTag(ID_CDFOOTIKPLUGIN)) SetFootIKData(gch,tData);
	
	chM = ch->GetMg();
	if(tData->GetBool(LMBIK_GOAL_TO_BONE))
	{
		if(!CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING))
		{
			if(doc->GetAction() == ID_MODELING_ROTATE)
			{
				if(CDIsDirty(goal,CD_DIRTY_MATRIX))
				{
					if(!undoOn)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						undoOn = true;
					}
					goalDirM = MInv(chM) * goalM;
					tData->SetMatrix(LMBIK_GOAL_DIR_MATRIX,goalDirM);
				}
			}
			else
			{
				undoOn = false;
				goalDirM = tData->GetMatrix(LMBIK_GOAL_DIR_MATRIX);
				transM = chM * goalDirM;
				goalM.v1 = transM.v1;
				goalM.v2 = transM.v2;
				goalM.v3 = transM.v3;
				
				Vector theScale = CDGetScale(goal);		
				Vector oldRot = CDGetRot(goal);
				goal->SetMg(goalM);
				newRot = CDGetRot(goal);
				rotSet = CDGetOptimalAngle(oldRot, newRot, goal);
				CDSetRot(goal,rotSet);	
				CDSetScale(goal,theScale);
				goal->Message(MSG_UPDATE);
			}
		}
	}
	
	opM = op->GetMg();
	poleM = pole->GetMg();
	tData->SetMatrix(LMBIK_PVR_MATRIX,MInv(opM) * poleM);
	
	// touch values for dirty IK test
	tData->SetBool(LMBIK_DIRTY_IK,false);
	tData->SetVector(LMBIK_P_VECTOR,poleV);
	tData->SetVector(LMBIK_G_VECTOR,goalV);
	tData->SetReal(LMBIK_OLD_IK_BLEND,ikMix);

	return CD_EXECUTION_RESULT_OK;
}

Bool CDLimbIKPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(LMBIK_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDLimbIKPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case LMBIK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LMBIK_GOAL_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LMBIK_SOLVER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LMBIK_LINE_TARGET:	
			return tData->GetBool(LMBIK_SHOW_LINES);
		case LMBIK_DAMPING_VALUE:	
			return tData->GetBool(LMBIK_DAMPING_ON);
		case LMBIK_SET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(LMBIK_LENGTH_SET)) return true;
				else return false;
			}
		case LMBIK_CLEAR_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET)) return true;
				else return false;
			}
		case LMBIK_RESET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET)) return true;
				else return false;
			}
		case LMBIK_SQUASH_N_STRETCH:
			if(tData->GetBool(LMBIK_LENGTH_SET)) return true;
			else return false;
		case LMBIK_CHANGE_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET) && tData->GetBool(LMBIK_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case LMBIK_VOLUME_STRENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET) && tData->GetBool(LMBIK_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case LMBIK_CLAMP_SQUASH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case LMBIK_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case LMBIK_SET_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(LMBIK_CLAMP_SQUASH) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case LMBIK_MIX_SQ_CLAMP:
			if(tData->GetBool(LMBIK_CLAMP_SQUASH) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
			else return false;
		case LMBIK_CLAMP_STRETCH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case LMBIK_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LMBIK_LENGTH_SET) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case LMBIK_SET_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(LMBIK_CLAMP_STRETCH) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case LMBIK_MIX_ST_CLAMP:
			if(tData->GetBool(LMBIK_CLAMP_SQUASH) && tData->GetBool(LMBIK_SQUASH_N_STRETCH)) return true;
			else return false;
		case LMBIK_USE_BIAS_CURVE:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(LMBIK_CHANGE_VOLUME);
		case LMBIK_BIAS_CURVE:
			return tData->GetBool(LMBIK_USE_BIAS_CURVE);
	}
	return true;
}

Bool RegisterCDLimbIKPlugin(void)
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
	String name=GeLoadString(IDS_CDLIMBIK); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDLIMBIKPLUGIN,name,info,CDLimbIKPlugin::Alloc,"tCDLimbIK","CDlimb_ik_tag.tif",1);
}

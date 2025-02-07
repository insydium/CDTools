//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDDualTarget.h"
#include "CDIKtools.h"

enum
{
	//DT_PURCHASE					= 1000,
	
	//DT_LINE_COLOR					= 1500,
	DT_DISK_LEVEL					= 1501,
	//DT_STRENGTH					= 1502,

	//DT_IK_STRETCH_GROUP			= 3000,
	//DT_SET_LENGTH					= 3001,
	//DT_CLEAR_LENGTH				= 3002,
	DT_LENGTH_SET					= 3003,
	
	//DT_SQUASH_N_STRETCH			= 3004,
	//DT_CLAMP_SQUASH				= 3005,
	//DT_SQUASH_DIST					= 3006,
	//DT_SET_SQUASH_DIST				= 3007,
	//DT_CLAMP_STRETCH				= 3008,
	//DT_STRETCH_DIST				= 3009,
	//DT_SET_STRETCH_DIST			= 3010,
	
	DT_JOINT_LENGTH				= 3011,
	//DT_CHANGE_VOLUME				= 3012,
	//DT_USE_BIAS_CURVE				= 3013,
	//DT_BIAS_CURVE					= 3014,
	//DT_MIX_VOLUME					= 3015,
	//DT_VOLUME_STRENGTH				= 3016,
	
	//DT_RESET_LENGTH				= 3018,
	
	//DT_IK_POLE_AXIS				= 10010,
		//DT_IK_POLE_Y				= 10011,
		//DT_IK_POLE_X				= 10012,

	//DT_TARGET_A_LINK				= 10020,
	//DT_TARGET_B_LINK				= 10021,
	
	//DT_IK_POLE_NX					= 10022,
	//DT_IK_POLE_NY					= 10023,
	
	//DT_IK_SHOW_LINES				= 10024,
	//DT_IK_POLE_OFF				= 10025,
	
	//DT_AB_TARGET_GROUP		 = 20000,
};

class CDDualTargetPlugin : public CDTagData
{
private:
	Bool squNstrON;
	Vector vPole, vGoal;
	LONG pAxis; 
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void InitSplineData(BaseContainer *tData);
	
	LONG GetPoleAxis(BaseContainer *tData);
	Vector GetMAxis(BaseContainer *tData);
	
	void SetRPVectors(BaseContainer *tData, BaseObject *op, BaseObject *goal, BaseObject *pole);
	void SetSquashAndStretch(BaseDocument *doc, BaseObject *op, BaseObject *ch, BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDDualTargetPlugin); }
};

Bool CDDualTargetPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetLong(DT_IK_POLE_AXIS,DT_IK_POLE_Y);
	tData->SetBool(DT_IK_SHOW_LINES,false);
	tData->SetVector(DT_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(DT_STRENGTH,1.0);

	tData->SetReal(DT_VOLUME_STRENGTH,1.0);

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

void CDDualTargetPlugin::InitSplineData(BaseContainer *tData)
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
	tData->SetData(DT_BIAS_CURVE, spd);
}

Bool CDDualTargetPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(DT_DISK_LEVEL,level);
	
	return true;
}

Bool CDDualTargetPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *goal = tData->GetObjectLink(DT_TARGET_A_LINK,doc);
	BaseObject *pole = tData->GetObjectLink(DT_TARGET_B_LINK,doc);

	if(!tData->GetBool(DT_IK_SHOW_LINES))
	{
		return true;
	}
	else
	{
		CDSetBaseDrawMatrix(bd, NULL, Matrix());
		
		bd->SetPen(tData->GetVector(DT_LINE_COLOR));
		if(goal)
		{
			CDDrawLine(bd,op->GetMg().off, goal->GetMg().off);
		}
		if(pole)
		{
			CDDrawLine(bd,op->GetMg().off, pole->GetMg().off);
		}
	}
	
	return true;
}

Bool CDDualTargetPlugin::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("CDDualTargetPlugin::Message");
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
			
			if(tData->GetLong(DT_DISK_LEVEL) < 1) InitSplineData(tData);
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
	BaseObject *goal = tData->GetObjectLink(DT_TARGET_A_LINK,doc);
	Real goalDist;
	if(goal) goalDist = Len(goal->GetMg().off - op->GetMg().off);
	else goalDist = Len(ch->GetMg().off - op->GetMg().off);

	Vector gScale = GetGlobalScale(op);
	switch (type)
	{
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			if(tData->GetBool(DT_LENGTH_SET))
			{
				Vector *trnsSca = (Vector*)data;
				if(trnsSca)
				{
					Vector sca = *trnsSca;
					Vector lenV = VNorm(op->GetMg().v3) * tData->GetReal(DT_JOINT_LENGTH);
					lenV.x *= sca.x;
					lenV.y *= sca.y;
					lenV.z *= sca.z;
					tData->SetReal(DT_JOINT_LENGTH,Len(lenV));
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==DT_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==DT_SET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					Real jlength = CDGetPos(op->GetDown()).z;
					tData->SetReal(DT_JOINT_LENGTH,jlength);
					tData->SetBool(DT_LENGTH_SET, true);
				}
			}
			else if(dc->id[0].id==DT_CLEAR_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					Vector chPos = CDGetPos(ch);
					chPos.z = tData->GetReal(DT_JOINT_LENGTH);
					CDSetPos(ch,chPos);

					tData->SetBool(DT_LENGTH_SET, false);
				}
			}
			else if(dc->id[0].id==DT_RESET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					Real jlength = CDGetPos(op->GetDown()).z;
					tData->SetReal(DT_JOINT_LENGTH,jlength);
					tData->SetBool(DT_LENGTH_SET, true);
				}
			}
			else if(dc->id[0].id==DT_SET_SQUASH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					if(gScale.z != 0.0)
					{
						tData->SetReal(DT_SQUASH_DIST, tData->GetReal(DT_JOINT_LENGTH) - goalDist / gScale.z);
					}
					else
					{
						tData->SetReal(DT_SQUASH_DIST, tData->GetReal(DT_JOINT_LENGTH) - goalDist);
					}
					tData->SetBool(DT_CLAMP_SQUASH, true);
				}
			}
			else if(dc->id[0].id==DT_SET_STRETCH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
					
					if(gScale.z != 0.0)
					{
						tData->SetReal(DT_STRETCH_DIST, goalDist / gScale.z - tData->GetReal(DT_JOINT_LENGTH));
					}
					else
					{
						tData->SetReal(DT_STRETCH_DIST, goalDist - tData->GetReal(DT_JOINT_LENGTH));
					}
					tData->SetBool(DT_CLAMP_STRETCH, true);
				}
			}
			break;
		}
	}

	return true;
}

LONG CDDualTargetPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(DT_IK_POLE_AXIS))
	{
		case DT_IK_POLE_X:	
			return POLE_X;
		case DT_IK_POLE_Y:
			return POLE_Y;
		case DT_IK_POLE_NX:	
			return POLE_NX;
		case DT_IK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

Vector CDDualTargetPlugin::GetMAxis(BaseContainer *tData)
{
	switch(tData->GetLong(DT_IK_POLE_AXIS))
	{
		case DT_IK_POLE_X:	
			return Vector(0,1,0);
		case DT_IK_POLE_Y:
			return Vector(1,0,0);
		case DT_IK_POLE_NX:	
			return Vector(0,1,0);
		case DT_IK_POLE_NY:
			return Vector(1,0,0);
		default:
			return Vector(1,0,0);
	}
}

void CDDualTargetPlugin::SetRPVectors(BaseContainer *tData, BaseObject *op, BaseObject *goal, BaseObject *pole)
{
	Matrix opM = op->GetMg(), glM, plM, transM;
	if(pole) plM = pole->GetMg();
	if(goal) glM = goal->GetMg();
	
	// Check use links and World up vector option
	if(!goal && !pole)
	{
		if(tData->GetLong(DT_IK_POLE_AXIS) == DT_IK_POLE_OFF)
		{
			vPole = opM.v2;
			vGoal = opM.v3;
		}
		else
		{
			vPole = Vector(0,1,0);
			vGoal = opM.v3;
		}
	}
	else
	{
		// Set the Main target 
		if(!goal) vGoal = opM.v3;
		else vGoal = glM.off - opM.off;
		
		// Set the Secondary Target
		if(tData->GetLong(DT_IK_POLE_AXIS) == DT_IK_POLE_OFF) vPole = opM.v2;
		else
		{
			if(!pole) vPole = Vector(0,1,0);
			else vPole = plM.off - opM.off;
		}
	}
}

void CDDualTargetPlugin::SetSquashAndStretch(BaseDocument *doc, BaseObject *op, BaseObject *ch, BaseContainer *tData)
{
	Matrix opM = op->GetMg(), chM = ch->GetMg();
	Vector gScale = GetGlobalScale(op);
	Real totalLen = tData->GetReal(DT_JOINT_LENGTH) * gScale.z;
	Real sqLen = tData->GetReal(DT_SQUASH_DIST) * gScale.z;
	Real stLen = tData->GetReal(DT_STRETCH_DIST) * gScale.z;
	Real squash = 1, stretch = 1, mix = tData->GetReal(DT_STRENGTH);
	Real volume = tData->GetReal(DT_VOLUME_STRENGTH);
	
	if(ch)
	{
		Vector chPos = CDGetPos(ch);;
		BaseObject *goal = tData->GetObjectLink(DT_TARGET_A_LINK,doc);
		if(!goal)
		{
			chPos.x = 0.0;
			chPos.y = 0.0;
			CDSetPos(ch,chPos);
		}
		if(!tData->GetBool(DT_SQUASH_N_STRETCH) && squNstrON)
		{
			chPos.z = tData->GetReal(DT_JOINT_LENGTH);
			CDSetPos(ch,chPos);
			
			squNstrON = false;
		}
		if(tData->GetBool(DT_SQUASH_N_STRETCH))
		{
			Real gLen;
			if(goal) gLen = CDBlend(Len(ch->GetMg().off - opM.off),Len(vGoal),mix);
			else gLen = Len(ch->GetMg().off - opM.off);
			
			if(totalLen != 0)
			{
				stretch = gLen/totalLen;
				if(tData->GetBool(DT_CLAMP_STRETCH) && gLen > totalLen + stLen)
				{
					stretch = (totalLen + stLen)/totalLen;
				}
				if(tData->GetBool(DT_CLAMP_SQUASH) && gLen < totalLen - sqLen)
				{
					stretch = (totalLen - sqLen)/totalLen;
				}
			}
			
			chPos.z = CDBlend(chPos.z,tData->GetReal(DT_JOINT_LENGTH) * stretch,mix);
			CDSetPos(ch,chPos);

			if(tData->GetBool(DT_CHANGE_VOLUME))
			{
				Real skinVol;
				if(stretch < 1)
				{
					skinVol = 0.5;
					squash = (((stretch * skinVol) - 1)/(skinVol-1));
					if(tData->GetBool(DT_USE_BIAS_CURVE)) squash *= 3.5;
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
					if(tData->GetBool(DT_USE_BIAS_CURVE)) squash *= 0.1;
					if(volume < 1)
					{
						Real sqVol = CDBlend(1.0,squash,volume);
						squash = sqVol;
					}
					else if(volume > 1) squash *= 1/volume;
				}
			}
			else tData->SetBool(DT_MIX_VOLUME,false);
			
			squNstrON = true;
		}
		SplineData *biasSpline = (SplineData*)tData->GetCustomDataType(DT_BIAS_CURVE,CUSTOMDATATYPE_SPLINE);
		if(biasSpline && totalLen != 0)
		{
			BaseContainer *opData = op->GetDataInstance();
			if(opData && op->IsInstanceOf(ID_CDJOINTOBJECT))
			{
				if(tData->GetBool(DT_MIX_VOLUME)) opData->SetBool(JNT_J_BASE,false);
				else opData->SetBool(JNT_J_BASE,true);
				
				opData->SetReal(JNT_STRETCH_VALUE,stretch);
				opData->SetReal(JNT_SQUASH_VALUE,squash);
				opData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
				opData->SetReal(JNT_SPL_START,0.0);
				opData->SetReal(JNT_SPL_END,1.0);
				opData->SetBool(JNT_USE_SPLINE,tData->GetBool(DT_USE_BIAS_CURVE));
				opData->SetReal(JNT_J_LENGTH,tData->GetReal(DT_JOINT_LENGTH));
				opData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(DT_VOLUME_STRENGTH));
			}
		}
	}
}

LONG CDDualTargetPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	Matrix opM = op->GetMg(), glM, plM, transM, targM;
	Vector opPos = CDGetPos(op), opSca = CDGetScale(op); 
	Vector oldRot = CDGetRot(op), newRot, opRot;
	Real mix = tData->GetReal(DT_STRENGTH);
	
	BaseObject *pole = tData->GetObjectLink(DT_TARGET_B_LINK,doc);
	if(pole)
	{
		pAxis = tData->GetLong(DT_IK_POLE_AXIS);
		plM = pole->GetMg();
		if(plM.off == opM.off) return false;
	}
	BaseObject *goal = tData->GetObjectLink(DT_TARGET_A_LINK,doc);
	if(goal)
	{
		glM = goal->GetMg();
		if(glM.off == opM.off) return false;
	}

	targM = opM;
	
	SetRPVectors(tData,op,goal,pole);
	targM = GetRPMatrix(GetPoleAxis(tData),opM.off,vGoal,vPole);
	
	transM = MixM(opM,targM,GetMAxis(tData),mix);
	op->SetMg(transM);
		
	newRot = CDGetRot(op);
	opRot = CDGetOptimalAngle(oldRot, newRot, op);
	
	CDSetPos(op,opPos);
	CDSetScale(op,opSca);	
	CDSetRot(op,opRot);
	
	if(op->GetType() == ID_CDJOINTOBJECT && op->GetDown())
	{
		SetSquashAndStretch(doc,op,op->GetDown(),tData);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDDualTargetPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(DT_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

void CDDualTargetPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(DT_IK_POLE_AXIS+T_LST,tData->GetLong(DT_IK_POLE_AXIS));
			
			tData->SetLink(DT_TARGET_A_LINK+T_LST,tData->GetLink(DT_TARGET_A_LINK,doc));
			tData->SetLink(DT_TARGET_B_LINK+T_LST,tData->GetLink(DT_TARGET_B_LINK,doc));
			
			tData->SetBool(DT_CHANGE_VOLUME+T_LST,tData->GetBool(DT_CHANGE_VOLUME));
			tData->SetBool(DT_CLAMP_STRETCH+T_LST,tData->GetBool(DT_CLAMP_STRETCH));
			tData->SetReal(DT_STRETCH_DIST+T_LST,tData->GetReal(DT_STRETCH_DIST));
			tData->SetBool(DT_CLAMP_SQUASH+T_LST,tData->GetBool(DT_CLAMP_SQUASH));
			tData->SetReal(DT_SQUASH_DIST+T_LST,tData->GetReal(DT_SQUASH_DIST));

			tData->SetBool(DT_USE_BIAS_CURVE+T_LST,tData->GetBool(DT_USE_BIAS_CURVE));
			tData->SetBool(DT_MIX_VOLUME+T_LST,tData->GetBool(DT_MIX_VOLUME));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDDualTargetPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(DT_IK_POLE_AXIS,tData->GetLong(DT_IK_POLE_AXIS+T_LST));
		
		tData->SetLink(DT_TARGET_A_LINK,tData->GetLink(DT_TARGET_A_LINK+T_LST,doc));
		tData->SetLink(DT_TARGET_B_LINK,tData->GetLink(DT_TARGET_B_LINK+T_LST,doc));
		
		tData->SetBool(DT_CHANGE_VOLUME,tData->GetBool(DT_CHANGE_VOLUME+T_LST));
		tData->SetBool(DT_CLAMP_STRETCH,tData->GetBool(DT_CLAMP_STRETCH+T_LST));
		tData->SetReal(DT_STRETCH_DIST,tData->GetReal(DT_STRETCH_DIST+T_LST));
		tData->SetBool(DT_CLAMP_SQUASH,tData->GetBool(DT_CLAMP_SQUASH+T_LST));
		tData->SetReal(DT_SQUASH_DIST,tData->GetReal(DT_SQUASH_DIST+T_LST));
		
		tData->SetBool(DT_USE_BIAS_CURVE,tData->GetBool(DT_USE_BIAS_CURVE+T_LST));
		tData->SetBool(DT_MIX_VOLUME,tData->GetBool(DT_MIX_VOLUME+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDDualTargetPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case DT_TARGET_A_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case DT_TARGET_B_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case DT_IK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case DT_SET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!op->GetDown()) return false;
				else
				{
					if(!tData->GetBool(DT_LENGTH_SET) && op->GetType() == ID_CDJOINTOBJECT) return true;
					else return false;
				}
			}
		case DT_CLEAR_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(DT_LENGTH_SET)) return true;
				else return false;
			}
		case DT_RESET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(DT_LENGTH_SET)) return true;
				else return false;
			}
		case DT_SQUASH_N_STRETCH:
			if(tData->GetBool(DT_LENGTH_SET)) return true;
			else return false;
		case DT_CHANGE_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(DT_LENGTH_SET) && tData->GetBool(DT_SQUASH_N_STRETCH) && op->GetType() == ID_CDJOINTOBJECT) return true;
				else return false;
			}			
		case DT_CLAMP_SQUASH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(DT_LENGTH_SET) && tData->GetBool(DT_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case DT_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(DT_LENGTH_SET)) return true;
				else return false;
			}
		case DT_SET_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(DT_CLAMP_SQUASH) && tData->GetBool(DT_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case DT_CLAMP_STRETCH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(DT_LENGTH_SET) && tData->GetBool(DT_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case DT_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(DT_LENGTH_SET)) return true;
				else return false;
			}
		case DT_SET_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(DT_CLAMP_STRETCH) && tData->GetBool(DT_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case DT_USE_BIAS_CURVE:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(DT_CHANGE_VOLUME);
		case DT_MIX_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(DT_USE_BIAS_CURVE);
		case DT_BIAS_CURVE:
			return tData->GetBool(DT_USE_BIAS_CURVE);
	}
	return true;
}

Bool RegisterCDDualTargetPlugin(void)
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
	String name=GeLoadString(IDS_CDDUALTARGET); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDDUALTARGETPLUGIN,name,info,CDDualTargetPlugin::Alloc,"tCDDualTarget","CDdualtarg.tif",1);
}

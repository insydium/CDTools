//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDSplineIK.h"
#include "CDIKtools.h"
#include "CDArray.h"

#define MAX_ADD_LINKS		50
#define MAX_TRIES			100
#define THRESHOLD			0.1

enum
{
	//SPLIK_PURCHASE					= 1000,
	SPLIK_DISK_LEVEL					= 1001,
	SPLIK_UNIFORM						= 1002,

	//SPLIK_LINE_COLOR					= 1500,
	//SPLIK_BLEND						= 1501,
	
	//SPLIK_INTERPOLATION				= 2003,
		//SPLIK_SHORTEST				= 2004,
		//SPLIK_AVERAGE					= 2005,
		//SPLIK_LONGEST					= 2006,
		
	SPLIK_BASE_ROTATION					= 2500,

	//SPLIK_STRETCH_GROUP				= 3000,
	//SPLIK_SET_LENGTH					= 3001,
	//SPLIK_CLEAR_LENGTH				= 3002,
	SPLIK_LENGTH_SET					= 3003,
	
	//SPLIK_SQUASH_N_STRETCH			= 3004,
	//SPLIK_CLAMP_SQUASH				= 3005,
	//SPLIK_SQUASH_DIST					= 3006,
	//SPLIK_SET_SQUASH_DIST				= 3007,
	//SPLIK_CLAMP_STRETCH				= 3008,
	//SPLIK_STRETCH_DIST				= 3009,
	//SPLIK_SET_STRETCH_DIST			= 3010,
	
	SPLIK_TOTAL_LENGTH					= 3011,
	SPLIK_J_NUMBER						= 3012,
	
	//SPLIK_CHANGE_VOLUME				= 3013,
	//SPLIK_USE_BIAS_CURVE				= 3014,
	//SPLIK_BIAS_CURVE					= 3015,
	//SPLIK_MIX_VOLUME					= 3016,
	//SPLIK_LENGTH_BIAS					= 3017,
	//SPLIK_RESET_LENGTH				= 3018,
	SPLIK_PREVIOUS_FRAME				= 3019,
	//SPLIK_VOLUME_STRENGTH				= 3020,
	
	SPLIK_JOINT_LENGTH					= 4000,
	
	//SPLIK_TRUE_SPLINE_IK				= 5000,
	
	SPLIK_J_BIAS_LENGTH					= 6000,

	//SPLIK_BONES_IN_GROUP				= 10010,
	//SPLIK_SHOW_LINES					= 10011,

	//SPLIK_POLE_AXIS					= 10012,
		//SPLIK_POLE_Y					= 10013,
		//SPLIK_POLE_X					= 10014,

	//SPLIK_SOLUTION					= 10015,
		//SPLIK_USE_SOLVER				= 10016,
		//SPLIK_USE_ROTATION			= 10017,
		//SPLIK_USE_TARGETS				= 10018,

	//SPLIK_SPLINE_LINK					= 10020,
	//SPLIK_SOLVER_LINK					= 10021,
	
	SPLIK_A_POINT_LINK					= 10022, // Old data to convert
	SPLIK_B_POINT_LINK					= 10023, // Old data to convert
	SPLIK_C_POINT_LINK					= 10024, // Old data to convert
	SPLIK_D_POINT_LINK					= 10025, // Old data to convert
	//SPLIK_CONNECT_BONES				= 10026,
	//SPLIK_CONNECT_NEXT				= 10027,
	//SPLIK_POLE_NX						= 10028,
	//SPLIK_POLE_NY						= 10029,
	//SPLIK_USE							= 10030,
	SPLIK_SCALE_BONES					= 10031, // Old parameter no longer used
	
	SPLIK_ADD_CONTROLER					= 10032,
	//SPLIK_ADD_GOAL					= 10033,
	//SPLIK_SUB_GOAL					= 10034,
	
	SPLIK_BANK_WITH						= 10035, // Old parameter no longer used
	SPLIK_USE_X							= 10036, // Old parameter no longer used
	SPLIK_USE_Y							= 10037, // Old parameter no longer used
	SPLIK_USE_Z							= 10038, // Old parameter no longer used

	SPLIK_LINK_COUNT					= 10040,
	SPLIK_GROUP_ADD						= 10049,
	SPLIK_ADDITIONAL_LINKS				= 10050, // Old parameter
			
	SPLIK_A_POINT_BANK					= 10122, // Old data to convert
	SPLIK_B_POINT_BANK					= 10123, // Old data to convert
	SPLIK_C_POINT_BANK					= 10124, // Old data to convert
	SPLIK_D_POINT_BANK					= 10125, // Old data to convert
	
	SPLIK_JOINT_COUNT					= 14000,
	SPLIK_SPLINE_LENGTH					= 14001,
	
	SPLIK_T_TWIST_COUNT					= 15000,
	SPLIK_T_TOTAL_LENGTH				= 15001,
	SPLIK_TARGET_COUNT					= 15002,
	SPLIK_TARGET_POS					= 15100,
	SPLIK_TARGET_ROT					= 15200,
	SPLIK_TARGET_LEN					= 15300,
	SPLIK_TARGET_ID						= 15400,

	//SPLIK_SPLINE_GROUP				= 20000,
	//SPLIK_SOLVER_GROUP				= 30000,
	//SPLIK_CONTROL_GROUP				= 40000,
	//SPLIK_USE_EXTRN_CONT				= 40001,
	//SPLIK_TARGET						= 40100,
	//SPLIK_USE_BANK					= 40200,
};

class CDSplineIKPlugin : public CDTagData
{
private:
	Bool				squNstrON, joints, undoOn;
	Real				squash, stretch;
	Vector				poleV;;
	CDArray<Vector>		moveV;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void ConvertOldData(BaseDocument *doc, BaseContainer *tData);
	void InitSplineData(BaseContainer *tData);
	
	Bool CheckAddUndo(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector aRot, Vector bRot);
	
	LONG GetPoleAxis(BaseContainer *tData);
	LONG ConfirmJointCount(BaseObject *jnt, BaseContainer *tData);
	LONG ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData);
	void StoreSplineJointValues(BaseObject *jnt, BaseContainer *tData, CDSplineJoint *spJnt);
	
	Real GetTotalJointLength(BaseContainer *tData, BaseObject *jnt);
	void SetJointBiasLength(BaseContainer *tData, BaseObject *op);
	void SetJointSplinePos(BaseDocument *doc, BaseContainer *tData, BaseObject *jnt, SplineObject *spl, Matrix splM, CDSplineJoint *spJnt);
	
	void SetTargetLengths(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	void SetSquashAndStretch(BaseObject *op, BaseContainer *tData, CDSplineJoint *spJnt, Real sLen);
	void SetJointData(BaseContainer *tData, BaseObject *jnt, CDSplineJoint *spJnt);
	void SetSplinePoints(SplineObject *spl, BaseDocument *doc, BaseObject *op, BaseContainer *tData, Matrix splM, Real totalLen);
	
	void AlignJointsToSpline(SplineObject *spl, BaseContainer *tData, BaseObject *jnt, CDSplineJoint *spJnt, Real sLen, Real sca);
	
	void InitTargetTwist(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	void SetTargetTwistCount(BaseDocument *doc, BaseContainer *tData);
	void StoreTargetTwistRotation(BaseDocument *doc, BaseContainer *tData, SplineObject *spl, Matrix splM, CDSplineJoint *spJnt);
	void SetTargetSplinePos(BaseDocument *doc, BaseContainer *tData, SplineObject *spl, Matrix splM);
	Vector GetTargetSplineTangent(BaseDocument *doc, BaseContainer *tData, Real t);
	Vector GetMaxTargetRotation(BaseObject *op1, BaseObject *op2);
	Bool SetJointChainTwist(BaseTag *tag, BaseDocument *doc, BaseObject *op, CDSplineJoint *spJnt);
	
	void InitTrueSplineIK(SplineObject *spl, Vector *padr, BaseDocument *doc, BaseObject *op, BaseContainer *tData, Matrix splM, Real totalLen);
	void CalculateTrueSplineIK(BaseDocument *doc, SplineObject *spl, Vector *padr, BaseContainer *tData, BaseObject *jnt, CDSplineJoint *spJnt, Real sLen, Real sca);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSplineIKPlugin); }
};


Bool CDSplineIKPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(SPLIK_USE,true);
	tData->SetReal(SPLIK_BLEND,1.0);
	
	tData->SetBool(SPLIK_SHOW_LINES,true);
	tData->SetVector(SPLIK_LINE_COLOR, Vector(1,0,0));
	tData->SetLong(SPLIK_POLE_AXIS,SPLIK_POLE_Y);
	tData->SetBool(SPLIK_CONNECT_BONES,false);
	
	tData->SetLong(SPLIK_SOLUTION,SPLIK_USE_SOLVER);
	tData->SetLong(SPLIK_INTERPOLATION,SPLIK_SHORTEST);
	
	tData->SetLong(SPLIK_LINK_COUNT,1);
	tData->SetBool(SPLIK_USE_EXTRN_CONT,false);
	
	tData->SetReal(SPLIK_LENGTH_BIAS,0.5);
	
	tData->SetReal(SPLIK_VOLUME_STRENGTH,1.0);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	InitSplineData(tData);

	CDIKData spk;
	PluginMessage(ID_CDSPLINEIKPLUGIN,&spk);
	if(spk.list) spk.list->Append(node);

	return true;
}

void CDSplineIKPlugin::InitSplineData(BaseContainer *tData)
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
	tData->SetData(SPLIK_BIAS_CURVE, spd);
}

void CDSplineIKPlugin::Free(GeListNode *node)
{
	moveV.Free();
	
	CDIKData spk;
	PluginMessage(ID_CDSPLINEIKPLUGIN,&spk);
	if(spk.list) spk.list->Remove(node);
}

Bool CDSplineIKPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(SPLIK_DISK_LEVEL,level);
	
	return true;
}

void CDSplineIKPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(SPLIK_POLE_AXIS+T_LST,tData->GetLong(SPLIK_POLE_AXIS));
			
			LONG i, jcnt = tData->GetReal(SPLIK_BONES_IN_GROUP);
			tData->SetReal(SPLIK_BONES_IN_GROUP+T_LST,jcnt);
			
			BaseObject *ch = op->GetDown();
			for(i=0; i<jcnt; i++)
			{
				if(ch)
				{
					Vector chPos = CDGetPos(ch);
					if(tData->GetBool(SPLIK_LENGTH_SET))
					{
						chPos.z = tData->GetReal(SPLIK_JOINT_LENGTH+i);
					}
					tData->SetVector(T_CP+i,chPos);
					ch = ch->GetDown();
				}
			}
			
			tData->SetBool(SPLIK_USE_EXTRN_CONT+T_LST,tData->GetBool(SPLIK_USE_EXTRN_CONT));
			tData->SetBool(SPLIK_TRUE_SPLINE_IK+T_LST,tData->GetBool(SPLIK_TRUE_SPLINE_IK));
			
			tData->SetLink(SPLIK_SPLINE_LINK+T_LST,tData->GetLink(SPLIK_SPLINE_LINK,doc));
			tData->SetLink(SPLIK_SOLVER_LINK+T_LST,tData->GetLink(SPLIK_SOLVER_LINK,doc));
			
			tData->SetLong(SPLIK_INTERPOLATION+T_LST,tData->GetLong(SPLIK_INTERPOLATION));
			LONG trgCnt = tData->GetLong(SPLIK_LINK_COUNT);
			for(i=0; i<trgCnt; i++)
			{
				tData->SetLink(SPLIK_TARGET+i+T_LST,tData->GetLink(SPLIK_TARGET+i,doc));
			}
			
			tData->SetBool(SPLIK_CHANGE_VOLUME+T_LST,tData->GetBool(SPLIK_CHANGE_VOLUME));
			tData->SetBool(SPLIK_MIX_VOLUME+T_LST,tData->GetBool(SPLIK_MIX_VOLUME));
			tData->SetBool(SPLIK_USE_BIAS_CURVE+T_LST,tData->GetBool(SPLIK_USE_BIAS_CURVE));
			
			tData->SetBool(SPLIK_CLAMP_STRETCH+T_LST,tData->GetBool(SPLIK_CLAMP_STRETCH));
			tData->SetReal(SPLIK_STRETCH_DIST+T_LST,tData->GetReal(SPLIK_STRETCH_DIST));
			tData->SetBool(SPLIK_CLAMP_SQUASH+T_LST,tData->GetBool(SPLIK_CLAMP_SQUASH));
			tData->SetReal(SPLIK_SQUASH_DIST+T_LST,tData->GetReal(SPLIK_SQUASH_DIST));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSplineIKPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(SPLIK_POLE_AXIS,tData->GetLong(SPLIK_POLE_AXIS+T_LST));
		
		LONG i, jcnt = tData->GetReal(SPLIK_BONES_IN_GROUP+T_LST);
		tData->SetReal(SPLIK_BONES_IN_GROUP,jcnt);
		
		BaseObject *ch = op->GetDown();
		for(i=0; i<jcnt; i++)
		{
			if(ch)
			{
				CDSetPos(ch,tData->GetVector(T_CP+i));
				ch = ch->GetDown();
			}
		}
		
		tData->SetBool(SPLIK_USE_EXTRN_CONT,tData->GetBool(SPLIK_USE_EXTRN_CONT+T_LST));
		tData->SetBool(SPLIK_TRUE_SPLINE_IK,tData->GetBool(SPLIK_TRUE_SPLINE_IK+T_LST));
		
		tData->SetLink(SPLIK_SPLINE_LINK,tData->GetLink(SPLIK_SPLINE_LINK+T_LST,doc));
		tData->SetLink(SPLIK_SOLVER_LINK,tData->GetLink(SPLIK_SOLVER_LINK+T_LST,doc));
		
		tData->SetLong(SPLIK_INTERPOLATION,tData->GetLong(SPLIK_INTERPOLATION+T_LST));
		LONG trgCnt = tData->GetLong(SPLIK_LINK_COUNT);
		for(i=0; i<trgCnt; i++)
		{
			tData->SetLink(SPLIK_TARGET+i,tData->GetLink(SPLIK_TARGET+i+T_LST,doc));
		}
		
		tData->SetBool(SPLIK_CHANGE_VOLUME,tData->GetBool(SPLIK_CHANGE_VOLUME+T_LST));
		tData->SetBool(SPLIK_MIX_VOLUME,tData->GetBool(SPLIK_MIX_VOLUME+T_LST));
		tData->SetBool(SPLIK_USE_BIAS_CURVE,tData->GetBool(SPLIK_USE_BIAS_CURVE+T_LST));
		
		tData->SetBool(SPLIK_CLAMP_STRETCH,tData->GetBool(SPLIK_CLAMP_STRETCH+T_LST));
		tData->SetReal(SPLIK_STRETCH_DIST,tData->GetReal(SPLIK_STRETCH_DIST+T_LST));
		tData->SetBool(SPLIK_CLAMP_SQUASH,tData->GetBool(SPLIK_CLAMP_SQUASH+T_LST));
		tData->SetReal(SPLIK_SQUASH_DIST,tData->GetReal(SPLIK_SQUASH_DIST+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSplineIKPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	BaseObject *pole = tData->GetObjectLink(SPLIK_SOLVER_LINK,doc); if(!pole) return true;
	BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET,doc); if(!trg) return true;
	
	if(!tData->GetBool(SPLIK_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());

	bd->SetPen(tData->GetVector(SPLIK_LINE_COLOR));
	CDDrawLine(bd,op->GetMg().off, pole->GetMg().off);
	
	LONG jCnt = 0;
	BaseObject *jnt = op;
	BaseObject *ch = jnt->GetDown(); if(!ch) return true;
	
	CDDrawLine(bd,jnt->GetMg().off, ch->GetMg().off);
	while (ch &&  jCnt < tData->GetLong(SPLIK_BONES_IN_GROUP))
	{
		CDDrawLine(bd,jnt->GetMg().off, ch->GetMg().off);
		jnt = ch;
		ch = ch->GetDown();
		if(ch) jCnt++;
	}
	
	return true;
}

void CDSplineIKPlugin::ConvertOldData(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, lnkCnt = tData->GetLong(SPLIK_LINK_COUNT);
	tData->SetLong(SPLIK_LINK_COUNT,lnkCnt);
	
	tData->SetBool(SPLIK_SCALE_BONES,false);

	for(i=0; i<lnkCnt; i++)
	{
		if(i < 4)
		{
			tData->SetLink(SPLIK_TARGET+i,tData->GetLink(SPLIK_A_POINT_LINK+i,doc));
			tData->SetBool(SPLIK_USE_BANK+i,tData->GetBool(SPLIK_A_POINT_BANK+i));
		}
		else
		{
			LONG addlink = i-4;
			tData->SetLink(SPLIK_TARGET+i,tData->GetLink(SPLIK_ADDITIONAL_LINKS+addlink,doc));
			tData->SetBool(SPLIK_USE_BANK+i,tData->GetBool(SPLIK_ADDITIONAL_LINKS+100+addlink));
		}
	}
}

void CDSplineIKPlugin::InitTargetTwist(BaseDocument *doc, BaseContainer *tData, BaseObject *jnt)
{
	BaseObject *ikSpl = tData->GetObjectLink(SPLIK_SPLINE_LINK,doc);
	if(jnt && IsValidSplineObject(ikSpl))
	{
		Matrix splM = ikSpl->GetMg();
		SplineObject *spl = ikSpl->GetRealSpline();
		
		LONG jCnt = ConfirmJointCount(jnt,tData);
		CDArray<CDSplineJoint> spJnt;
		spJnt.Alloc(jCnt+1);
		
		StoreSplineJointValues(jnt,tData,spJnt.Array());
		SetSplinePoints(spl,doc,jnt,tData,splM,GetTotalJointLength(tData,jnt));
		
		SetTargetTwistCount(doc,tData);
		
		SetTargetSplinePos(doc,tData,spl,splM);
		StoreTargetTwistRotation(doc,tData,spl,splM,spJnt.Array());
	
		spJnt.Free();
	}
	
}

Bool CDSplineIKPlugin::Message(GeListNode *node, LONG type, void *data)
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

			if(doc && tData->GetLong(SPLIK_DISK_LEVEL) < 1)
			{
				ConvertOldData(doc,tData);
				tData->SetVector(SPLIK_BASE_ROTATION,CDGetRot(op));
				
				InitSplineData(tData);
				if(tData->GetLong(SPLIK_SOLUTION) == SPLIK_USE_TARGETS) InitTargetTwist(doc,tData,op);
			}
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
	
	LONG lnkCnt = tData->GetLong(SPLIK_LINK_COUNT);
	LONG linkID = SPLIK_TARGET;
	LONG boolID = SPLIK_USE_BANK;
	LONG jCnt = ConfirmJointCount(op,tData);
	
	Real sLen = tData->GetReal(SPLIK_SPLINE_LENGTH);
	
	LONG trgCnt = ConfirmTargetCount(doc,tData);
	if(trgCnt != tData->GetLong(SPLIK_TARGET_COUNT))
	{
		tData->SetLong(SPLIK_TARGET_COUNT,trgCnt);
		if(!moveV.IsEmpty()) moveV.Free();
	}
	if(moveV.IsEmpty()) moveV.Alloc(trgCnt);

	Vector gScale = GetGlobalScale(op);
	switch (type)
	{
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			
			if(tData->GetLong(SPLIK_SOLUTION) == SPLIK_USE_TARGETS)
			{
				Bool twChng = false;
				LONG i;
				for(i=0; i<trgCnt; i++)
				{
					if(descID[0].id  == SPLIK_USE_BANK+i)
					{
						twChng = true;
					}
				}
				if(twChng) InitTargetTwist(doc,tData,op);
			}
			break;
		}
		case CD_MSG_UPDATE:
		{
			if(undoOn)
			{
				BaseContainer state;
				GetInputState(BFM_INPUT_MOUSE, BFM_INPUT_MOUSELEFT, state);
				if(state.GetLong(BFM_INPUT_VALUE) == 0) undoOn = false;
			}
			break;
		}
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			if(tData->GetBool(SPLIK_LENGTH_SET))
			{
				Vector *trnsSca = (Vector*)data;
				if(trnsSca)
				{
					Vector sca = *trnsSca;
					Real totalLen = 0;
					LONG i;
					for(i=0; i<jCnt; i++)
					{
						if(op)
						{
							Vector lenV = VNorm(op->GetMg().v3) * tData->GetReal(SPLIK_JOINT_LENGTH+i);
							lenV.x *= sca.x;
							lenV.y *= sca.y;
							lenV.z *= sca.z;
							tData->SetReal(SPLIK_JOINT_LENGTH+i,Len(lenV));
							totalLen += Len(lenV);
							
							op = op->GetDown();
						}
					}
					tData->SetReal(SPLIK_TOTAL_LENGTH, totalLen);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SPLIK_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==SPLIK_ADD_GOAL)
			{
				if(tData->GetBool(T_REG))
				{
					if(lnkCnt < MAX_ADD_LINKS)
					{
						lnkCnt++;
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						tData->SetLong(SPLIK_LINK_COUNT,lnkCnt);
					}
				}
			}
			else if(dc->id[0].id==SPLIK_SUB_GOAL)
			{
				if(tData->GetBool(T_REG))
				{
					if(lnkCnt > 1)
					{
						linkID += lnkCnt - 1;
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						tData->SetLink(linkID,NULL);
						boolID += lnkCnt - 1;
						tData->SetBool(boolID,false);
						lnkCnt--;
						tData->SetLong(SPLIK_LINK_COUNT,lnkCnt);
					}
				}
			}
			else if(dc->id[0].id==SPLIK_SET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					if(jCnt > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						LONG i=0;
						Real jlength, totalLength = 0;
						while(op && i<jCnt)
						{
							jlength = CDGetPos(op->GetDown()).z;
							tData->SetReal(SPLIK_JOINT_LENGTH+i,jlength);
							totalLength+=jlength;
							op = op->GetDown();
							i++;
						}
						tData->SetBool(SPLIK_LENGTH_SET, true);
						tData->SetReal(SPLIK_TOTAL_LENGTH, totalLength);
						tData->SetLong(SPLIK_J_NUMBER, jCnt);
					}
				}
			}
			else if(dc->id[0].id==SPLIK_CLEAR_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					if(jCnt > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						LONG i=0;
						Vector opPos;
						op = op->GetDown();
						while(op && i<jCnt)
						{
							opPos = CDGetPos(op);
							opPos.z = tData->GetReal(SPLIK_JOINT_LENGTH+i);
							CDSetPos(op,opPos);
							op = op->GetDown();
							i++;
						}
						tData->SetBool(SPLIK_LENGTH_SET, false);
						tData->SetReal(SPLIK_LENGTH_BIAS, 0.5);
					}
				}
			}
			else if(dc->id[0].id==SPLIK_RESET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					if(jCnt > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						LONG i=0;
						Real jlength, totalLength = 0;
						while(op && i<jCnt)
						{
							jlength = CDGetPos(op->GetDown()).z;
							tData->SetReal(SPLIK_JOINT_LENGTH+i,jlength);
							totalLength+=jlength;
							op = op->GetDown();
							i++;
						}
						tData->SetBool(SPLIK_LENGTH_SET, true);
						tData->SetReal(SPLIK_TOTAL_LENGTH, totalLength);
						tData->SetLong(SPLIK_J_NUMBER, jCnt);
					}
				}
			}
			else if(dc->id[0].id==SPLIK_SET_SQUASH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					if(gScale.z != 0.0)
					{
						tData->SetReal(SPLIK_SQUASH_DIST, tData->GetReal(SPLIK_TOTAL_LENGTH) - sLen / gScale.z);
					}
					else
					{
						tData->SetReal(SPLIK_SQUASH_DIST, tData->GetReal(SPLIK_TOTAL_LENGTH) - sLen);
					}	
					tData->SetBool(SPLIK_CLAMP_SQUASH, true);
				}
			}
			else if(dc->id[0].id==SPLIK_SET_STRETCH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					if(gScale.z != 0.0)
					{
						tData->SetReal(SPLIK_STRETCH_DIST, sLen / gScale.z - tData->GetReal(SPLIK_TOTAL_LENGTH));
					}
					else
					{
						tData->SetReal(SPLIK_STRETCH_DIST, sLen - tData->GetReal(SPLIK_TOTAL_LENGTH));
					}
					tData->SetBool(SPLIK_CLAMP_STRETCH, true);
				}
			}
			break;
		}
	}

	return true;
}

LONG CDSplineIKPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(SPLIK_POLE_AXIS))
	{
		case SPLIK_POLE_X:	
			return POLE_X;
		case SPLIK_POLE_Y:
			return POLE_Y;
		case SPLIK_POLE_NX:	
			return POLE_NX;
		case SPLIK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

LONG CDSplineIKPlugin::ConfirmJointCount(BaseObject *jnt, BaseContainer *tData)
{
	LONG jCnt = 0;
	
	while(jnt &&  jCnt < tData->GetLong(SPLIK_BONES_IN_GROUP))
	{
		jnt = jnt->GetDown();
		if(jnt) jCnt++;
	}
	if(jCnt > tData->GetLong(SPLIK_J_NUMBER))  tData->SetBool(SPLIK_LENGTH_SET, false);
	tData->SetLong(SPLIK_BONES_IN_GROUP, jCnt);
	tData->SetLong(SPLIK_JOINT_COUNT,jCnt);
	
	return jCnt;
}

LONG CDSplineIKPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, lnkCnt = tData->GetLong(SPLIK_LINK_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<lnkCnt; i++)
	{
		trg = tData->GetObjectLink(SPLIK_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < lnkCnt)
			{
				j++;
				trg = tData->GetObjectLink(SPLIK_TARGET+j,doc);
			}
			if(trg)
			{
				chkCnt++;
				tData->SetLink(SPLIK_TARGET+i,tData->GetObjectLink(SPLIK_TARGET+j,doc));
				tData->SetBool(SPLIK_USE_BANK+i,tData->GetBool(SPLIK_USE_BANK+j));
				tData->SetLink(SPLIK_TARGET+j,NULL);
			}
		}
		else chkCnt++;
	}
	
	return chkCnt;
}

void CDSplineIKPlugin::SetTargetTwistCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, t = 0, trgCnt = tData->GetLong(SPLIK_TARGET_COUNT);
	for(i=trgCnt-1; i>=0; i--)
	{
		if(tData->GetBool(SPLIK_USE_BANK+i))
		{
			BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET+i,doc);
			if(trg)
			{
				tData->SetLink(SPLIK_TARGET_ID+t,trg);
				t++;
			}
		}
	}
	tData->SetLong(SPLIK_T_TWIST_COUNT,t);
}

void CDSplineIKPlugin::StoreTargetTwistRotation(BaseDocument *doc, BaseContainer *tData, SplineObject *spl, Matrix splM, CDSplineJoint *spJnt)
{
	
	LONG i, twCnt = tData->GetLong(SPLIK_T_TWIST_COUNT);
	for(i=0; i<twCnt; i++)
	{
		BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET_ID+i,doc);
		if(trg)
		{
			LONG j=0, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);
			Real mix, zRot = 0.0, trgPos = tData->GetReal(SPLIK_TARGET_POS+i);
			
			while(spJnt[j].splPos < trgPos && j < jCnt)
			{
				if(j > 0)
				{
					zRot += spJnt[j].rot.z;
				}
				j++;
			}
			if(spJnt[j].splPos != 0.0) mix = (trgPos - spJnt[j-1].splPos)/spJnt[j].splPos;
			else mix = 0.0;
			
			zRot += spJnt[j].rot.z * mix;
			
			tData->SetReal(SPLIK_TARGET_ROT+i,zRot);
		}
	}
}

void CDSplineIKPlugin::SetTargetSplinePos(BaseDocument *doc, BaseContainer *tData, SplineObject *spl, Matrix splM)
{
	LONG i, twCnt = tData->GetLong(SPLIK_T_TWIST_COUNT);
	
	for(i=0; i<twCnt; i++)
	{
		BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET_ID+i,doc);
		if(trg && spl)
		{
			Vector pos = MInv(splM) * trg->GetMg().off;
			tData->SetReal(SPLIK_TARGET_POS+i,GetNearestSplinePoint(spl,pos,0,tData->GetBool(SPLIK_UNIFORM)));
		}
	}
}

void CDSplineIKPlugin::SetTargetLengths(BaseDocument *doc, BaseContainer *tData, BaseObject *op)
{
	BaseObject *trg = NULL;
	Vector startPos = op->GetMg().off, endPos;
	Real totTLen = 0;
	LONG i, t = 0;
	
	LONG trgCnt = tData->GetLong(SPLIK_TARGET_COUNT);
	for(i=trgCnt-1; i>=0; i--)
	{
		trg = tData->GetObjectLink(SPLIK_TARGET+i,doc);
		if(trg)
		{
			endPos = trg->GetMg().off;
			tData->SetReal(SPLIK_TARGET_LEN+t,Len(startPos - endPos));
			totTLen += Len(startPos - endPos);
			startPos = endPos;
			t++;
		}
	}
	tData->SetReal(SPLIK_T_TOTAL_LENGTH,totTLen);
}

void CDSplineIKPlugin::SetJointSplinePos(BaseDocument *doc, BaseContainer *tData, BaseObject *jnt, SplineObject *spl, Matrix splM, CDSplineJoint *spJnt)
{
	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);
	
	for(i=0; i<(jCnt+1); i++)
	{
		if(jnt)
		{
			Vector pos = MInv(splM) * jnt->GetMg().off;
			spJnt[i].splPos = GetNearestSplinePoint(spl,pos,0,tData->GetBool(SPLIK_UNIFORM));
			jnt = jnt->GetDown();
		}
	}
}

Real CDSplineIKPlugin::GetTotalJointLength(BaseContainer *tData, BaseObject *jnt)
{
	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);
	Real totalLen = 0;
	
	for (i=0; i<jCnt; i++)
	{
		jnt = jnt->GetDown();
		if(jnt) totalLen += Len(jnt->GetMg().off - jnt->GetUp()->GetMg().off);
	}
	
	return totalLen;
}

void CDSplineIKPlugin::SetJointBiasLength(BaseContainer *tData, BaseObject *op)
{
	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);
	
	Real totalLen = 0.0;
	for(i=0; i<jCnt; i++)
	{
		totalLen += tData->GetReal(SPLIK_JOINT_LENGTH+i);
	}
	
	Real addLen = 0.0, jbLenAdd = 0.0;
	Real bias = CDBlend(0.01,0.99,tData->GetReal(SPLIK_LENGTH_BIAS));
	
	if(bias > 0.5)
	{
		for(i=0; i<jCnt; i++)
		{
			if(i < jCnt-1)
			{
				addLen += tData->GetReal(SPLIK_JOINT_LENGTH+i);
				Real biasLen = (totalLen * (1-Bias(1-bias,(totalLen-addLen)/totalLen))) - jbLenAdd;
				
				tData->SetReal(SPLIK_J_BIAS_LENGTH+i,biasLen);
				jbLenAdd += biasLen;
			}
			else tData->SetReal(SPLIK_J_BIAS_LENGTH+i,totalLen - jbLenAdd);
		}
	}
	else
	{
		for(i=0; i<jCnt; i++)
		{
			if(i < jCnt-1)
			{
				addLen += tData->GetReal(SPLIK_JOINT_LENGTH+i);
				Real biasLen = (totalLen * Bias(bias,addLen/totalLen)) - jbLenAdd;
				
				tData->SetReal(SPLIK_J_BIAS_LENGTH+i,biasLen);
				jbLenAdd += biasLen;
			}
			else tData->SetReal(SPLIK_J_BIAS_LENGTH+i,totalLen - jbLenAdd);
		}
	}
}

void CDSplineIKPlugin::StoreSplineJointValues(BaseObject *jnt, BaseContainer *tData, CDSplineJoint *spJnt)
{
	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);;
	
	for(i=0; i<(jCnt+1); i++)
	{
		if(jnt)
		{
			spJnt[i].jnt = jnt;
			spJnt[i].jntM = jnt->GetMg();
			spJnt[i].rot = CDGetRot(jnt);
			spJnt[i].sca = CDGetScale(jnt);
			
			Vector jntPos = CDGetPos(jnt);
			if(i>0 && tData->GetBool(SPLIK_CONNECT_BONES))
			{
				jntPos.x = 0.0;
				jntPos.y = 0.0;
				if(CDIsBone(jnt->GetUp())) jntPos.z = GetBoneLength(jnt->GetUp());
			}
			spJnt[i].pos = jntPos;
			
			spJnt[i].twist = spJnt[i].rot.z;
			
			jnt = jnt->GetDown();
			if(jnt)
			{
				if(tData->GetBool(SPLIK_LENGTH_SET)) spJnt[i].length = tData->GetReal(SPLIK_J_BIAS_LENGTH+i);
				else spJnt[i].length = CDGetPos(jnt).z;
			}
			else spJnt[i].length = 0.0;
		}
	}
}

void CDSplineIKPlugin::SetSplinePoints(SplineObject *spl, BaseDocument *doc, BaseObject *op, BaseContainer *tData, Matrix splM, Real totalLen)
{
	Vector tDir, nTipPos, gScale = GetGlobalScale(op);
	Vector *padr = GetPointArray(spl);
	Real tLen=0, sqDist = tData->GetReal(SPLIK_SQUASH_DIST) * gScale.z;
	Real jtLen = tData->GetReal(SPLIK_TOTAL_LENGTH) * gScale.z;
	LONG trgCnt = tData->GetLong(SPLIK_TARGET_COUNT);

	Matrix opM = op->GetMg();
	
	LONG i, t = 0, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);;
	padr[0] = MInv(splM) * opM.off;
	for (i=trgCnt; i>0; i--)
	{
		padr[i] = MInv(splM) * tData->GetObjectLink(SPLIK_TARGET+t,doc)->GetMg().off;
		t++;
	}
	spl->SetDefaultCoeff();
	
	Real sLen = CDGetSplineLength(spl);
	
	if(tData->GetBool(SPLIK_SQUASH_N_STRETCH))
	{
		if(tData->GetBool(SPLIK_CLAMP_SQUASH))
		{
			if(tData->GetBool(SPLIK_TRUE_SPLINE_IK))
			{
				InitTrueSplineIK(spl,padr,doc,op,tData,splM,jtLen - sqDist);
			}
			else
			{
				if(sLen < jtLen - sqDist)
				{
					tLen = jtLen - sqDist - sLen;
					tDir = VNorm(padr[trgCnt] - padr[trgCnt-1]);
					nTipPos = padr[trgCnt] + tDir * tLen;
					padr[trgCnt] = nTipPos;
				}
			}
		}
		else
		{
			if(tData->GetBool(SPLIK_TRUE_SPLINE_IK))
			{
				Vector tPos = tData->GetObjectLink(SPLIK_TARGET,doc)->GetMg().off;
				InitTrueSplineIK(spl,padr,doc,op,tData,splM,Len(tPos - opM.off));
			}
		}
	}
	else
	{
		if(tData->GetBool(SPLIK_TRUE_SPLINE_IK))
		{
			InitTrueSplineIK(spl,padr,doc,op,tData,splM,totalLen);
		}
		else
		{
			// Vector array of joint positions for FitCurve()
			CDArray<Vector> jPos;
			jPos.Alloc(jCnt+1);
			
			BaseObject *jnt = op;
			for(i=0; i<jCnt+1; i++)
			{
				if(jnt)
				{
					jPos[i] = jnt->GetMg().off;
					jnt = jnt->GetDown();
				}
			}
			
			if(sLen < totalLen)
			{
				SplineObject *splFit = FitCurve(jPos.Array(), jCnt+1, 0, NULL);
				if(splFit)
				{
					BaseContainer *sData = splFit->GetDataInstance();
					if(sData) sData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_CUBIC);
					splFit->SetDefaultCoeff();
					
					Real fitLen = CDGetSplineLength(splFit);
				
					tLen = fitLen - sLen;
					
					Vector prvPt = spl->GetSplinePoint(CDUniformToNatural(spl,0.99),0);
					
					tDir = VNorm(padr[trgCnt] - prvPt);
					nTipPos = padr[trgCnt] + tDir * tLen;
					padr[trgCnt] = nTipPos;
					
					sLen = CDGetSplineLength(spl);
					
					while(sLen < fitLen)
					{
						nTipPos = padr[trgCnt] + VNorm(tDir);
						padr[trgCnt] = nTipPos;
						
						sLen = CDGetSplineLength(spl);
					}
				}
				SplineObject::Free(splFit);
			}
			jPos.Free();
		}
	}
}

void CDSplineIKPlugin::SetSquashAndStretch(BaseObject *op, BaseContainer *tData, CDSplineJoint *spJnt, Real sLen)
{
	Vector gScale = GetGlobalScale(op), jntPos;
	Real sqDist = tData->GetReal(SPLIK_SQUASH_DIST) * gScale.z;
	Real totalLen = tData->GetReal(SPLIK_TOTAL_LENGTH) * gScale.z;
	Real stDist = tData->GetReal(SPLIK_STRETCH_DIST) * gScale.z;
	Real volume = tData->GetReal(SPLIK_VOLUME_STRENGTH);
	
	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);;
	BaseObject *jnt = NULL;
	
	squash = 1.0;
	stretch = 1.0; 
	if(!tData->GetBool(SPLIK_SQUASH_N_STRETCH) || !tData->GetBool(SPLIK_LENGTH_SET))
	{
		if(squNstrON)
		{
			jnt = op->GetDown();
			for(i=0; i<jCnt; i++)
			{
				if(jnt)
				{
					spJnt[i].length = tData->GetReal(SPLIK_J_BIAS_LENGTH+i);
					
					jntPos = CDGetPos(jnt);
					jntPos.z = spJnt[i].length;
					CDSetPos(jnt,jntPos);
					jnt = jnt->GetDown();
				}
			}
			squNstrON = false;
		}
	}
	else
	{
		if(totalLen != 0)
		{
			stretch = sLen/totalLen;
			if(tData->GetBool(SPLIK_CLAMP_STRETCH) && sLen > totalLen + stDist)
			{
				stretch = (totalLen + stDist)/totalLen;
			}
			if(tData->GetBool(SPLIK_CLAMP_SQUASH) && sLen < totalLen - sqDist)
			{
				stretch = (totalLen - sqDist)/totalLen;
			}
		}
		
		if(tData->GetBool(SPLIK_CHANGE_VOLUME))
		{
			Real skinVol;
			if(stretch < 1)
			{
				skinVol = 0.5;
				squash = ((stretch * skinVol) - 1)/(skinVol-1);
				if(tData->GetBool(SPLIK_USE_BIAS_CURVE)) squash *= 3.5;
				if(volume < 1.0)
				{
					Real sqVol = CDBlend(1.0,squash,volume);
					squash = sqVol;
				}
				else if(volume > 1) squash *= volume;
			}
			else if(stretch > 1.0)
			{
				skinVol = 0.25;
				squash = ((stretch - 1) * skinVol + 1)/stretch;
				if(tData->GetBool(SPLIK_USE_BIAS_CURVE)) squash *= 0.1;
				if(volume < 1)
				{
					Real sqVol = CDBlend(1.0,squash,volume);
					squash = sqVol;
				}
				else if(volume > 1) squash *= 1/volume;
			}
		}
		else tData->SetBool(SPLIK_MIX_VOLUME,false);
		
		squNstrON = true;
	}
}

void CDSplineIKPlugin::SetJointData(BaseContainer *tData, BaseObject *jnt, CDSplineJoint *spJnt)
{
	Vector gScale = GetGlobalScale(jnt);
	Real totalLen = tData->GetReal(SPLIK_TOTAL_LENGTH) * gScale.z;
	Real volume = tData->GetReal(SPLIK_VOLUME_STRENGTH);
	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);;
	
	SplineData *biasSpline = (SplineData*)tData->GetCustomDataType(SPLIK_BIAS_CURVE,CUSTOMDATATYPE_SPLINE);
	if(biasSpline && totalLen != 0)
	{
		BaseContainer *jData = NULL;
		Real splStart = 0, splEnd = 0;
		for(i=0; i<jCnt; i++)
		{
			if(jnt)
			{
				jData = jnt->GetDataInstance();
				if(jData && jnt->IsInstanceOf(ID_CDJOINTOBJECT))
				{
					Real jLen = tData->GetReal(SPLIK_JOINT_LENGTH+i);
					
					jData->SetReal(JNT_J_LENGTH,jLen);
					jData->SetReal(JNT_SPL_START,splStart);
					
					splEnd += jLen/totalLen;
					if(splEnd > 1.0) splEnd = 1.0;
					jData->SetReal(JNT_SPL_END,splEnd);
					
					if(i == 0 && !tData->GetBool(SPLIK_MIX_VOLUME)) jData->SetBool(JNT_J_BASE,true);
					else jData->SetBool(JNT_J_BASE,false);
					
					Real jStretch = 1.0, jSquash = 1.0;
					if(tData->GetBool(SPLIK_CHANGE_VOLUME) && tData->GetBool(SPLIK_SQUASH_N_STRETCH))
					{
						jStretch = Len(spJnt[i+1].jntM.off - spJnt[i].jntM.off)/(jLen * gScale.z);
						Real JSkinVol;
						if(jStretch < 1.0)
						{
							JSkinVol = 0.5;
							jSquash = ((jStretch * JSkinVol) - 1)/(JSkinVol-1);
							if(tData->GetBool(SPLIK_USE_BIAS_CURVE)) jSquash *= 3.5;
							if(volume < 1.0)
							{
								Real sqVol = CDBlend(1.0,jSquash,volume);
								jSquash = sqVol;
							}
							else if(volume > 1.0) jSquash *= volume;
						}
						else if(jStretch > 1.0)
						{
							JSkinVol = 0.25;
							jSquash = ((jStretch - 1) * JSkinVol + 1)/jStretch;
							if(tData->GetBool(SPLIK_USE_BIAS_CURVE)) jSquash *= 0.1;
							if(volume < 1.0)
							{
								Real sqVol = CDBlend(1.0,jSquash,volume);
								jSquash = sqVol;
							}
							else if(volume > 1.0) jSquash *= 1/volume;
						}
					}
					
					jData->SetReal(JNT_STRETCH_VALUE,jStretch);
					jData->SetReal(JNT_SQUASH_VALUE,jSquash);
					
					jData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
					jData->SetBool(JNT_USE_SPLINE,tData->GetBool(SPLIK_USE_BIAS_CURVE));
					jData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(SPLIK_VOLUME_STRENGTH));

					jnt = jnt->GetDown();
					splStart = splEnd;
				}
			}
		}
	}
}

Vector CDSplineIKPlugin::GetTargetSplineTangent(BaseDocument *doc, BaseContainer *tData, Real t)
{
	Vector tng = Vector(0,0,1);
	
	BaseObject *ikSpl = tData->GetObjectLink(SPLIK_SPLINE_LINK,doc);
	if(IsValidSplineObject(ikSpl))
	{
		Matrix splM = ikSpl->GetMg();
		SplineObject *spl = ikSpl->GetRealSpline();
		tng = splM * (spl->GetSplineTangent(CDUniformToNatural(spl,t)));
	}
	
	return tng;
}

Bool CDSplineIKPlugin::CheckAddUndo(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector aRot, Vector bRot)
{
	Bool undo = false;
	
	if(!CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING))
	{
		if(doc->GetAction() == ID_MODELING_ROTATE)
		{
			if(doc->GetTime().GetFrame(doc->GetFps()) == tData->GetLong(SPLIK_PREVIOUS_FRAME))
			{
				if(!VEqual(aRot,bRot,0.001))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					undo = true;
				}
			}
		}
	}
	
	return undo;
}

Vector CDSplineIKPlugin::GetMaxTargetRotation(BaseObject *op1, BaseObject *op2)
{
	Vector maxRot = Vector(0,0,0);
	
	Vector gRot = CDGetOptimalAngle(GetGlobalRotation(op2) - GetGlobalRotation(op1), CDMatrixToHPB(MInv(op1->GetMg()) * op2->GetMg()));
	maxRot.z = gRot.z;
	
	return maxRot;
}

Bool CDSplineIKPlugin::SetJointChainTwist(BaseTag *tag, BaseDocument *doc, BaseObject *op, CDSplineJoint *spJnt)
{
	BaseContainer	*tData = tag->GetDataInstance();

	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);
	
	switch (tData->GetLong(SPLIK_SOLUTION))
	{
		case SPLIK_USE_SOLVER:
		{
			Matrix rpM, jntM = spJnt[0].jntM;
			for(i=1; i<(jCnt+1); i++)
			{
				rpM = GetRPMatrix(GetPoleAxis(tData),spJnt[i].jntM.off,spJnt[i].jntM.v3,poleV);
				Vector jntRot = CDMatrixToHPB(MInv(jntM) * rpM, spJnt[i].jnt);
				if(jntRot.z > pi) jntRot.z -= pi2;
				spJnt[i].jntM = rpM;
				spJnt[i].twist = jntRot.z;
				jntM = spJnt[i].jntM;
			}
			break;
		}	
		case SPLIK_USE_ROTATION:
		{
			for(i=1; i<(jCnt+1); i++)
			{
				spJnt[i].twist = 0.0;
			}
			break;
		}	
		case SPLIK_USE_TARGETS:
		{
			Vector oldRot = tData->GetVector(SPLIK_BASE_ROTATION);
			Vector opRot = CDGetRot(op);
			if(!undoOn)
			{
				undoOn = CheckAddUndo(tag,doc,tData,oldRot,opRot);
			}
			Vector setRot = CDGetOptimalAngle(oldRot,opRot,op);
			tData->SetVector(SPLIK_BASE_ROTATION,setRot);
			
			Real bRot, mix, aPos = 0, bPos;
			
			CDQuaternion quatA, quatB, quat25, quat50, quat75, twistQuat;
			LONG j=1, twCnt = tData->GetLong(SPLIK_T_TWIST_COUNT);
			
			Real aRot = 0.0;
			for(i=0; i<twCnt; i++)
			{
				BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET_ID+i,doc);
				if(trg)
				{
					Matrix trgM = trg->GetMg();
					
					if(tData->GetLong(SPLIK_INTERPOLATION) != SPLIK_SHORTEST)
					{
						Vector tng = GetTargetSplineTangent(doc,tData,tData->GetReal(SPLIK_TARGET_POS+i));
						Vector axis = VNorm(VCross(trgM.v3, tng));
						Real angle = ACos(VDot(VNorm(trgM.v3), VNorm(tng)));
						trgM = trgM * RotAxisToMatrix(axis,angle);
						
						//re-square up matrix
						trgM.v3 = VNorm(tng);
						Real xdot = Abs(VDot(VNorm(trgM.v3), VNorm(trgM.v1)));
						Real ydot = Abs(VDot(VNorm(trgM.v3), VNorm(trgM.v2)));
						if(ydot > xdot)
						{
							trgM.v2 = VNorm(VCross(trgM.v3, trgM.v1));
							trgM.v1 = VNorm(VCross(trgM.v2, trgM.v3));
						}
						else
						{
							trgM.v1 = VNorm(VCross(trgM.v2, trgM.v3));
							trgM.v2 = VNorm(VCross(trgM.v3, trgM.v1));
						}
						
						Vector tRot = Vector(0,0,0), tMRot = Vector(0,0,0);
						tRot.z = tData->GetReal(SPLIK_TARGET_ROT+i);
						tMRot.z = CDMatrixToHPB(MInv(spJnt[0].jntM) * trgM, trg).z;
						Vector trgRot = CDGetOptimalAngle(tRot, tMRot, trg);
						
						if(!undoOn)
						{
							undoOn = CheckAddUndo(tag,doc,tData,tRot,trgRot);
						}
						
						if(tData->GetLong(SPLIK_INTERPOLATION) == SPLIK_LONGEST)
						{
							Vector maxRot = GetMaxTargetRotation(op,trg);
							if(Abs(trgRot.z) != Abs(maxRot.z))
							{
								trgRot = CDGetOptimalAngle(maxRot,tMRot,trg);
							}
						}
						bRot = trgRot.z;
					}
					else
					{
						Matrix opM = op->GetMg(), prM = Matrix();
						Matrix localM = MInv(opM) * trgM;
						localM.off = Vector(0,0,0);
						
						Real xdot = Abs(VDot(VNorm(localM.v1), VNorm(prM.v1)));
						Real ydot = Abs(VDot(VNorm(localM.v2), VNorm(prM.v2)));
						if(ydot > xdot)
						{
							prM.v3 = localM.v3;
							prM.v1 = VNorm(VCross(VNorm(prM.v2), VNorm(prM.v3)));
							prM.v2 = VNorm(VCross(VNorm(prM.v3), VNorm(prM.v1)));
							prM.v3 = VNorm(VCross(VNorm(prM.v1), VNorm(prM.v2)));
						}
						else
						{
							prM.v3 = localM.v3;
							prM.v2 = VNorm(VCross(VNorm(prM.v3), VNorm(prM.v1)));
							prM.v1 = VNorm(VCross(VNorm(prM.v2), VNorm(prM.v3)));
							prM.v3 = VNorm(VCross(VNorm(prM.v1), VNorm(prM.v2)));
						}
						
						localM = MInv(prM) * localM;
						bRot = CDMatrixToHPB(localM,trg).z;
					}
					bPos = tData->GetReal(SPLIK_TARGET_POS+i);
					
					switch(tData->GetLong(SPLIK_INTERPOLATION))
					{
						case SPLIK_SHORTEST:
						{
							quatA.SetMatrix(MatrixRotZ(aRot));
							quatB.SetMatrix(MatrixRotZ(bRot));
								
							while(j<(jCnt+1) && spJnt[j].splPos<bPos)
							{
								Real jPos = spJnt[j].splPos;
								if((bPos-aPos) != 0) mix = (jPos-aPos)/(bPos-aPos); //Check for division by zero
								else mix = 0;
								
								// interpolate 
								twistQuat = CDQSlerp(quatA,quatB,mix);
								spJnt[j].jntM = spJnt[j].jntM * twistQuat.GetMatrix();
								spJnt[j].rot = CDGetOptimalAngle(spJnt[j].rot, CDMatrixToHPB(MInv(spJnt[j-1].jntM) * spJnt[j].jntM, spJnt[j].jnt), spJnt[j].jnt);
								j++;
							}
							break;
						}
						case SPLIK_AVERAGE:
						{
							if(bRot > pi2) bRot -= pi2;
							if(bRot < -pi2) bRot += pi2;
							quatA.SetHPB(Vector(0.0,0.0,aRot));
							quatB.SetHPB(Vector(0.0,0.0,bRot));
							quat50 = !(quatB + quatA); // calculate the 50 quat
							if(quat50.w == 0.0 && quat50.v.z == 0.0) quat50.v.z = -1.0;
							quat25 = !(quat50 + quatA); // calculate the 25 quat
							if(quat25.w == 0.0 && quat25.v.z == 0.0) quat25.v.z = -1.0;
							quat75 = !(quat50 + quatB); // calculate the 75 quat
							if(quat75.w == 0.0 && quat75.v.z == 0.0) quat75.v.z = -1.0;
								
							while(j<(jCnt+1) && spJnt[j].splPos<bPos)
							{
								Real jPos = spJnt[j].splPos;
								if((bPos-aPos) != 0) mix = (jPos-aPos)/(bPos-aPos); //Check for division by zero
								else mix = 0;
								
								// interpolate 
								twistQuat = CDQSlerpBezier(quatA,quat25,quat50,quat75,quatB,mix);
								spJnt[j].jntM = spJnt[j].jntM * twistQuat.GetMatrix();
								spJnt[j].rot = CDGetOptimalAngle(spJnt[j].rot, CDMatrixToHPB(MInv(spJnt[j-1].jntM) * spJnt[j].jntM, spJnt[j].jnt), spJnt[j].jnt);
								j++;
								
							}
							break;
						}
						case SPLIK_LONGEST:
						{
							while(j<(jCnt+1) && spJnt[j].splPos<bPos)
							{
								Real jPos = spJnt[j].splPos;
								if((bPos-aPos) != 0) mix = (jPos-aPos)/(bPos-aPos); //Check for division by zero
								else mix = 0;
								
								// interpolate 
								spJnt[j].twist = CDBlend(aRot,bRot,mix);
								spJnt[j].jntM = spJnt[j].jntM * MatrixRotZ(spJnt[j].twist);
								spJnt[j].rot = CDGetOptimalAngle(spJnt[j].rot, CDMatrixToHPB(MInv(spJnt[j-1].jntM) * spJnt[j].jntM, spJnt[j].jnt), spJnt[j].jnt);
								j++;
							}
							if(i == twCnt-1 && j<jCnt+1)
							{
								while(j<(jCnt+1))
								{
									spJnt[j].twist = bRot;
									spJnt[j].jntM = spJnt[j].jntM * MatrixRotZ(spJnt[j].twist);
									j++;
								}
							}
							break;
						}
					}
					tData->SetReal(SPLIK_TARGET_ROT+i,bRot);
					aRot = bRot;
					aPos = bPos;
				}
			}
			
			break;
		}	
	}
	return true;
}

void CDSplineIKPlugin::AlignJointsToSpline(SplineObject *spl, BaseContainer *tData, BaseObject *jnt, CDSplineJoint *spJnt, Real sLen, Real sca)
{
	LONG i, jCnt = tData->GetLong(SPLIK_JOINT_COUNT);
	Real jLen, chainLen = 0;
	
	Matrix jntM = jnt->GetMg(), splM = spl->GetMg();
	
	if(tData->GetBool(SPLIK_USE_EXTRN_CONT))
	{
		jntM.off = splM * spl->GetSplinePoint(CDUniformToNatural(spl,chainLen/sLen));
	}
	
	for(i=0; i<jCnt; i++)
	{
		// add the length of current joint to the chain length
		if(tData->GetBool(SPLIK_SQUASH_N_STRETCH))
		{
			jLen = tData->GetReal(SPLIK_J_BIAS_LENGTH+i) * sca * stretch;
		}
		else
		{
			jLen = spJnt[i].length * sca;
		}
		chainLen += jLen;
		
		// get the joint's aim vector
		Vector aimPoint = splM * spl->GetSplinePoint(CDUniformToNatural(spl,chainLen/sLen));
		if(tData->GetBool(SPLIK_SQUASH_N_STRETCH) && !tData->GetBool(SPLIK_CLAMP_STRETCH) && i == jCnt-1)
		{
			aimPoint = splM * spl->GetSplinePoint(CDUniformToNatural(spl,1.0));
		}
		
		Vector mSca = GetMatrixScale(jntM); // save the current matrix scale
		if(i > 0)
		{
			
			// rotate the matrix to point to the aim vector
			Vector axis = VNorm(VCross(VNorm(MInv(jntM) * aimPoint), Vector(0,0,1)));
			Real angle = ACos(VDot(VNorm(aimPoint - jntM.off), VNorm(jntM.v3)));
			jntM = jntM * RotAxisToMatrix(axis,angle);
			
			//re-square up matrix
			jntM.v3 = VNorm(aimPoint - jntM.off);
			jntM.v1 = VNorm(VCross(jntM.v2, jntM.v3));
			jntM.v2 = VNorm(VCross(jntM.v3, jntM.v1));
			
			jntM = ScaleMatrix(jntM,mSca); // restore the original matrix scale
			
			spJnt[i].rot = CDGetOptimalAngle(Vector(0,0,0), CDMatrixToHPB(MInv(spJnt[i-1].jntM) * jntM, spJnt[i].jnt), spJnt[i].jnt);
			spJnt[i].pos = MInv(spJnt[i-1].jntM) * jntM.off;
		}
		else
		{
			if(!tData->GetBool(SPLIK_USE_EXTRN_CONT))
			{
				jntM = GetRPMatrix(GetPoleAxis(tData),jntM.off,aimPoint - jntM.off,poleV);
			}
			else
			{
				jntM = GetRPMatrix(1,jntM.off,aimPoint - jntM.off,jntM.v2);
			}
			jntM = ScaleMatrix(jntM,mSca); // restore the original matrix scale
		}
		spJnt[i].jntM = jntM;
		
		//move the offset to the next joint's position
		jntM.off = jntM.off + VNorm(jntM.v3) * jLen;
		if(tData->GetBool(SPLIK_SQUASH_N_STRETCH) && i == jCnt-1)
		{
			jntM.off = aimPoint;
		}
	}
	spJnt[i].jntM = jntM;
	spJnt[i].pos = MInv(spJnt[i-1].jntM) * jntM.off;
}

void CDSplineIKPlugin::InitTrueSplineIK(SplineObject *spl, Vector *padr, BaseDocument *doc, BaseObject *op, BaseContainer *tData, Matrix splM, Real totalLen)
{
	SetTargetLengths(doc,tData,op);
	BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET,doc);
	Matrix opM = op->GetMg(), trgM = trg->GetMg();
	
	Real adj, dotP, tLenPlus = 0.0, tTrgLen = 0.0;
	Vector linePt, tDir, prvPos = opM.off, gDir = trgM.off - opM.off;
	
	LONG trgCnt = tData->GetLong(SPLIK_TARGET_COUNT);
	LONG i, t = 0;

	CDArray<Vector> ptPos;
	ptPos.Alloc(trgCnt+1);
	
	CDArray<Real> wt;
	wt.Alloc(trgCnt+1);
	
	ptPos[0] = opM.off;
	wt[0] = 1.0;
	for (i=trgCnt; i>0; i--)
	{
		ptPos[i] = tData->GetObjectLink(SPLIK_TARGET+t,doc)->GetMg().off;
		wt[i] = 1.0;
		t++;
	}
	for(i=1; i<=trgCnt; i++)
	{
		tTrgLen += Len(ptPos[i-1] - ptPos[i]);
	}
	
	for(i=1; i<trgCnt; i++)
	{
		tDir = ptPos[i] - opM.off;
		
		dotP = VDot(VNorm(gDir), VNorm(tDir));
		adj = dotP * Len(tDir);
		
		Vector absV, relV;
		linePt = opM.off + VNorm(gDir) * adj;
		moveV[i] = MInv(splM) *  ptPos[i] - MInv(splM) * linePt;
		
		tLenPlus += tData->GetReal(SPLIK_TARGET_LEN+i-1);
		padr[i] = MInv(splM) * (opM.off + gDir * tLenPlus/tTrgLen);
	}
	
	ptPos.Free();
	
	Real sLen = CDGetSplineLength(spl);
	
	if(Len(gDir) < totalLen)
	{
		if(sLen < totalLen)
		{
			Real initialMove = Len(gDir) * (1 - sLen / totalLen);
			
			for(i=1; i<trgCnt; i++)
			{
				padr[i] += moveV[i] * (1 - sLen / totalLen);
			}
			
			sLen = CDGetSplineLength(spl);
			
			LONG tries = 0;
			while(sLen < totalLen && tries < MAX_TRIES)//
			{
				initialMove = Len(gDir) * (1 - sLen / totalLen);
				if(initialMove < 0.1) initialMove = 0.1;
				for(i=1; i<trgCnt; i++)
				{
					padr[i] += VNorm(moveV[i]) * initialMove * 0.5;
				}
				sLen = CDGetSplineLength(spl);
				tries++;
			}
		}
	}
	spl->SetDefaultCoeff();
	wt.Free();
}

void CDSplineIKPlugin::CalculateTrueSplineIK(BaseDocument *doc, SplineObject *spl, Vector *padr, BaseContainer *tData, BaseObject *jnt, CDSplineJoint *spJnt, Real sLen, Real sca)
{
	BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET,doc);
	LONG i, trgCnt = tData->GetLong(SPLIK_TARGET_COUNT), jCnt = tData->GetLong(SPLIK_JOINT_COUNT);
	
	Real endLen = Len(spJnt[jCnt].jntM.off - spJnt[jCnt-1].jntM.off);
	Real tipLen = Len(trg->GetMg().off - spJnt[jCnt-1].jntM.off);
	
	if(tipLen < endLen)
	{
		LONG tries = 0;
		Real step;
		while(tipLen != endLen && tries < MAX_TRIES)
		{
			if(tipLen > endLen) step = -0.1;
			else step = 0.1;
			
			for(i=1; i<trgCnt; i++)
			{
				padr[i] += VNorm(moveV[i]) * step;
			}
			spl->SetDefaultCoeff();
			
			AlignJointsToSpline(spl,tData,jnt,spJnt,sLen,sca);
			tipLen = Len(trg->GetMg().off - spJnt[jCnt-1].jntM.off);
			tries++;
		}
	}
	
}

LONG CDSplineIKPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseObject *ch = op->GetDown(); if(!ch) return false;
	BaseObject *pole = tData->GetObjectLink(SPLIK_SOLVER_LINK,doc); if(!pole) return false;
	
	// confirm spline IS a spline object and has a points array
	BaseObject *ikSpline = tData->GetObjectLink(SPLIK_SPLINE_LINK,doc); if(!ikSpline) return false;
	if(!IsValidSplineObject(ikSpline)) return false;
		
	BaseContainer *sData = ikSpline->GetDataInstance(); if(!sData) return false;
	if(sData->GetLong(SPLINEOBJECT_INTERPOLATION) == SPLINEOBJECT_INTERPOLATION_UNIFORM) tData->SetBool(SPLIK_UNIFORM,true);
	else tData->SetBool(SPLIK_UNIFORM,false);

	LONG trgCnt = ConfirmTargetCount(doc,tData); if(trgCnt < 1) return false;
	tData->SetLong(SPLIK_TARGET_COUNT, trgCnt);
	BaseObject *trg = tData->GetObjectLink(SPLIK_TARGET,doc); if(!trg) return false;
	SetTargetTwistCount(doc,tData);
	
	Matrix opM = op->GetMg(), poleM = pole->GetMg(), splM = ikSpline->GetMg();
	Vector theScale, oldRot, newRot, rotSet;
	Real ikMix = tData->GetReal(SPLIK_BLEND);
	LONG i;
	
	// check pole vector for 0 distance
	if(VEqual(poleM.off,opM.off,0.001)) return false;
	poleV = poleM.off - opM.off;
	
	if(tData->GetLong(SPLIK_SOLUTION) == SPLIK_USE_TARGETS) tData->SetBool(SPLIK_USE_EXTRN_CONT,false);

	//Normalize the splines matrix vectors forcing the scale to 1
	splM = GetNormalizedMatrix(splM);
	ikSpline->SetMg(splM);
	
	SplineObject *spl = ikSpline->GetRealSpline();
	LONG seg = spl->GetSegmentCount();
	spl->ResizeObject(trgCnt+1,seg);     //resize the spline to the number of valid target objects
	if(tData->GetBool(SPLIK_TRUE_SPLINE_IK))
	{
		BaseContainer *sData = ikSpline->GetDataInstance();
		sData->SetLong(SPLINEOBJECT_TYPE,SPLINEOBJECT_TYPE_BSPLINE);
		sData->SetLong(SPLINEOBJECT_INTERPOLATION,SPLINEOBJECT_INTERPOLATION_NATURAL);
		sData->SetLong(SPLINEOBJECT_SUB,16);
	}
	ikSpline->Message(MSG_UPDATE);
	
	LONG jCnt = ConfirmJointCount(op,tData);
	
	Real totalLen = GetTotalJointLength(tData,op);
	SetJointBiasLength(tData,op);

	CDArray<CDSplineJoint> spJnt;
	spJnt.Alloc(jCnt+1);
	
	StoreSplineJointValues(op,tData,spJnt.Array());
	 
	Vector *padr = GetPointArray(spl);

	if(!tData->GetBool(SPLIK_USE_EXTRN_CONT))
	{
		SetSplinePoints(spl,doc,op,tData,splM,totalLen);
	}
	else 
	{
		tData->SetBool(SPLIK_TRUE_SPLINE_IK,false);
		spJnt[0].jntM.off = splM * padr[0];
	}
	
	if(!tData->GetBool(SPLIK_USE))
	{
		BaseObject *tip = op;
		for(i=0; i<jCnt; i++)
		{
			if(tip->GetDown()) tip = tip->GetDown();
		}
		
		Matrix trgM = trg->GetMg();
		trgM.off = tip->GetMg().off;
		theScale = CDGetScale(trg);		
		oldRot = CDGetRot(trg);
		trg->SetMg(trgM);
		
		newRot = CDGetRot(trg);
		rotSet = CDGetOptimalAngle(oldRot, newRot, trg);
		CDSetRot(trg,rotSet);	
		CDSetScale(trg,theScale);	
		
		spJnt.Free();
		
		return true;
	}

	Real sLen = CDGetSplineLength(spl);
	
	SetSquashAndStretch(op,tData,spJnt.Array(),sLen);
	
	Vector gScale = GetGlobalScale(op);
	AlignJointsToSpline(spl,tData,op,spJnt.Array(),sLen,gScale.z);
	
	SetJointData(tData,op,spJnt.Array());
	
	if(tData->GetBool(SPLIK_TRUE_SPLINE_IK))
	{
		if(tData->GetBool(SPLIK_SQUASH_N_STRETCH))
		{
			if(tData->GetBool(SPLIK_CLAMP_SQUASH))
			{
				CalculateTrueSplineIK(doc,spl,padr,tData,op,spJnt.Array(),sLen,gScale.z);
			}
		}
		else CalculateTrueSplineIK(doc,spl,padr,tData,op,spJnt.Array(),sLen,gScale.z);
	}
	
	SetTargetSplinePos(doc,tData,spl,splM);
	SetJointSplinePos(doc,tData,op,spl,splM,spJnt.Array());
	
	op->SetMg(spJnt[0].jntM);
	CDSetScale(op,spJnt[0].sca);
	SetJointChainTwist(tag,doc,op,spJnt.Array());
	
	joints = true;
	BaseObject *jnt = op;
	if(ikMix > 0.0) spJnt[0].rot = Vector(0,0,0);
	
	CDQuaternion fkQ, ikQ, mixQ, quat25, quat50, quat75;
	for(i=0; i<=jCnt; i++)
	{
		if(jnt)
		{
			if(jnt->GetType() != ID_CDJOINTOBJECT) joints = false;
			
			Matrix setM;
			if(ikMix < 1.0)
			{
				if(i>0)
				{
					fkQ.SetHPB(CDGetRot(jnt));
					ikQ.SetHPB(spJnt[i].rot);
					quat50 = !(ikQ + fkQ); // calculate the 50 quat
					quat25 = !(quat50 + fkQ); // calculate the 25 quat
					quat75 = !(quat50 + ikQ); // calculate the 75 quat
					
					// interpolate 
					mixQ = CDQSlerpBezier(fkQ,quat25,quat50,quat75,ikQ,ikMix);
					setM = mixQ.GetMatrix();
					
					setM = mixQ.GetMatrix();
					jnt->SetMl(setM);
					CDSetPos(jnt,spJnt[i].pos);
				}
				else
				{
					fkQ.SetMatrix(jnt->GetMg());
					ikQ.SetMatrix(spJnt[i].jntM);
					mixQ = CDQSlerp(fkQ,ikQ,ikMix);
					
					setM = mixQ.GetMatrix();
					setM.off = spJnt[i].jntM.off;
					jnt->SetMg(setM);
				}
			}
			else jnt->SetMg(spJnt[i].jntM);

			Vector jntRot = CDGetOptimalAngle(spJnt[i].rot,CDGetRot(jnt),jnt);
			CDSetRot(jnt,jntRot);
			CDSetScale(jnt,spJnt[i].sca);
			
			if(i>0 && tData->GetBool(SPLIK_CONNECT_BONES))
			{
				CDSetPos(jnt,spJnt[i].pos);
			}
			
			jnt = jnt->GetDown();
		}
	}

	spJnt.Free();
	
	tData->SetLong(SPLIK_PREVIOUS_FRAME,doc->GetTime().GetFrame(doc->GetFps()));
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSplineIKPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG trgCnt = tData->GetLong(SPLIK_LINK_COUNT);
	
	if(!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SPLIK_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	BaseContainer subgroup = GetCustomDataTypeDefault(DTYPE_GROUP);
	subgroup.SetLong(DESC_COLUMNS, 2);

	if(!description->SetParameter(DescLevel(SPLIK_GROUP_ADD, DTYPE_GROUP, 0), subgroup, DescLevel(SPLIK_CONTROL_GROUP))) return true;

	LONG i;
	for (i=0; i<trgCnt; i++)
	{
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(S_TARGET)+CDLongToString(i+1));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(SPLIK_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(SPLIK_GROUP_ADD))) return false;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc3.SetString(DESC_NAME, GeLoadString(S_BANKING));
		bc3.SetBool(DESC_DEFAULT, false);
		if(!description->SetParameter(DescLevel(SPLIK_USE_BANK+i, DTYPE_BOOL, 0), bc3, DescLevel(SPLIK_GROUP_ADD))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSplineIKPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	LONG i, trgCnt = tData->GetLong(SPLIK_LINK_COUNT);
	switch (id[0].id)
	{
		case SPLIK_TRUE_SPLINE_IK:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetLong(SPLIK_USE_EXTRN_CONT))	return true;
				else return false;
			}
		case SPLIK_USE_TARGETS:
			if(!tData->GetLong(SPLIK_TRUE_SPLINE_IK))	return true;
			else return false;
		case SPLIK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLIK_BONES_IN_GROUP:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLIK_USE_EXTRN_CONT:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetLong(SPLIK_SOLUTION) != SPLIK_USE_TARGETS) return true;
				else return false;
			}
		case SPLIK_ADD_GOAL:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLIK_SUB_GOAL:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLIK_INTERPOLATION:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLIK_SPLINE_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLIK_SOLVER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLIK_SET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(SPLIK_LENGTH_SET)) return true;
				else return false;
			}
		case SPLIK_CLEAR_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET)) return true;
				else return false;
			}
		case SPLIK_RESET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET)) return true;
				else return false;
			}
		case SPLIK_LENGTH_BIAS:
			if(tData->GetBool(SPLIK_LENGTH_SET)) return true;
			else return false;
		case SPLIK_SQUASH_N_STRETCH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET)) return true;
				else return false;
			}
		case SPLIK_CHANGE_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET) && tData->GetBool(SPLIK_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case SPLIK_VOLUME_STRENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET) && tData->GetBool(SPLIK_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case SPLIK_CLAMP_SQUASH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET) && tData->GetBool(SPLIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPLIK_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET)) return true;
				else return false;
			}
		case SPLIK_SET_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(SPLIK_CLAMP_SQUASH) && tData->GetBool(SPLIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPLIK_CLAMP_STRETCH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET) && tData->GetBool(SPLIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPLIK_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPLIK_LENGTH_SET)) return true;
				else return false;
			}
		case SPLIK_SET_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(SPLIK_CLAMP_STRETCH) && tData->GetBool(SPLIK_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPLIK_USE_BIAS_CURVE:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SPLIK_CHANGE_VOLUME);
		case SPLIK_MIX_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SPLIK_USE_BIAS_CURVE);
		case SPLIK_BIAS_CURVE:
			return tData->GetBool(SPLIK_USE_BIAS_CURVE);
	}
	for(i=0; i<trgCnt; i++)
	{
		if(id[0].id == SPLIK_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
	}
	return true;
}

Bool RegisterCDSplineIKPlugin(void)
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
	String name=GeLoadString(IDS_CDSPLINEIK); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSPLINEIKPLUGIN,name,info,CDSplineIKPlugin::Alloc,"tCDSplineIK","CDspline_ik_tag.tif",1);
}

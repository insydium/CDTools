//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDSpinal.h"
#include "CDIKtools.h"
#include "CDArray.h"

#define MAX_ADD_LINKS 	20

enum
{	
	//SPN_IK_USE					= 1001,
	
	//SPN_POLE_AXIS			= 1002,
		//SPN_POLE_X			= 1003,
		//SPN_POLE_Y			= 1004,
		//SPN_POLE_Z			= 1005,
		//SPN_POLE_NX			= 1006,
		//SPN_POLE_NY			= 1007,
		//SPN_POLE_NZ			= 1008,

	//SPN_BONES_IN_GROUP			= 1010,

	SPN_ORIGINAL_BN_CNT			= 1011,
	
	//SPN_CONNECT_BONES			= 1012,
	//SPN_CONNECT_NEXT			= 1013,
	SPN_STRETCH_BONES			= 1014,

	//SPN_BASE_LINK				= 1015,
	//SPN_BASE_DEPTH				= 1016,
	//SPN_TIP_LINK				= 1017,
	//SPN_TIP_DEPTH				= 1018,
	//SPN_TIP_TWIST				= 1019,
	//SPN_BASE_TWIST				= 1020,
	
	//SPN_SHOW_LINES			= 1021,
	//SPN_LINE_COLOR				= 1022,
	//SPN_BLEND				= 1023,
	
	SPN_DISK_LEVEL				= 1100,
	
	//SPN_CONTROL_GROUP		= 2000,
	//SPN_USE_TARGET_TWIST		= 2001,
	//SPN_INCLUDE_TIP_TWIST		= 2002,
	//SPN_INTERPOLATION			= 2003,
		//SPN_SHORTEST			= 2004,
		//SPN_AVERAGE				= 2005,
		//SPN_LONGEST				= 2006,
		
	SPN_TIP_ROTATION			= 2050,
	
	SPN_M_25					= 2500,
	SPN_M_50					= 2501,
	SPN_M_75					= 2502,
	
	SPN_TOGGLE					= 2510,
	SPN_HALF_TOGGLE				= 2511,
	SPN_QTM						= 2512,

	//SPN_STRETCH_GROUP		= 3000,
	//SPN_SET_LENGTH				= 3001,
	//SPN_CLEAR_LENGTH			= 3002,
	SPN_LENGTH_SET				= 3003,
	
	//SPN_SQUASH_N_STRETCH		= 3004,
	//SPN_CLAMP_SQUASH			= 3005,
	//SPN_SQUASH_DIST				= 3006,
	//SPN_SET_SQUASH_DIST			= 3007,
	//SPN_CLAMP_STRETCH			= 3008,
	//SPN_STRETCH_DIST			= 3009,
	//SET_STRETCH_DIST		= 3010,
	
	SPN_TOTAL_LENGTH			= 3011,
	SPN_J_NUMBER				= 3012,
	//SPN_CHANGE_VOLUME			= 3013,
	//SPN_USE_BIAS_CURVE		= 3014,
	//SPN_BIAS_CURVE			= 3015,
	//SPN_MIX_VOLUME				= 3016,
	//SPN_DEPTH_S_N_S				= 3017,
	//SPN_SQUASH_DEPTH				= 3018,
	//SPN_STRETCH_DEPTH				= 3019,
	//SPN_RESET_LENGTH				= 3020,
	SPN_PREVIOUS_FRAME			= 3021,
	//SPN_VOLUME_STRENGTH				= 3022,
	
	SPN_JOINT_LENGTH			= 4000,
	
	SPN_REST_ROTATION_SET		= 11000,
	SPN_REST_ROTATION			= 11001,
};

class CDSpinalPlugin : public CDTagData
{
private:
	Bool	squNstrON, joints, undoOn;
	Real 	squash, stretch, sLen;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	void InitSplineData(BaseContainer *tData);
	Bool CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData);
	LONG ConfirmJointCount(BaseObject *op, BaseContainer *tData);
	void StoreSplineJointValues(BaseObject *op, BaseContainer *tData, CDSplineJoint *spJnt, LONG jCnt);
	
	Real GetTotalJointLength(BaseObject *op, LONG jCnt);
	Real GetTwistLength(CDSplineJoint *spJnt, LONG cnt, Real sca);
	Vector GetTipRotation(BaseObject *jnt, LONG jCnt);
	Matrix GetSpineAxisRotM(BaseContainer *tData);
	Vector GetMaxTipRotation(BaseContainer *tData, BaseObject *op1, BaseObject *op2);
	
	void SetInitialBaseRotation(BaseContainer *tData, BaseObject *op, BaseObject *bCntr);
	void SetSplinePoints(SplineObject *spl, BaseObject *op, BaseContainer *tData, Matrix baseM, Matrix tipM, Real totalLen);
	void SetSquashAndStretch(BaseObject *op, BaseContainer *tData, CDSplineJoint *spJnt, LONG jCnt);
	Bool SetJointChainTwist(BaseTag *tag, BaseDocument *doc, BaseObject *op, CDSplineJoint *spJnt, LONG jCnt);

public:

	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual void Free(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSpinalPlugin); }
};


Bool CDSpinalPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(SPN_USE,true);
	tData->SetReal(SPN_BLEND,1.0);
	tData->SetLong(SPN_POLE_AXIS,SPN_POLE_Y);
	tData->SetBool(SPN_CONNECT_BONES,false);
	tData->SetReal(SPN_BASE_DEPTH,50);
	tData->SetReal(SPN_TIP_DEPTH,50);
	tData->SetLong(SPN_INTERPOLATION,SPN_SHORTEST);
		
	tData->SetBool(SPN_SHOW_LINES,true);
	tData->SetVector(SPN_LINE_COLOR, Vector(1,0,0));
	
	tData->SetBool(SPN_REST_ROTATION_SET,false);
	
	tData->SetReal(SPN_VOLUME_STRENGTH,1.0);

	squNstrON = false;
	undoOn = false;
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	InitSplineData(tData);

	CDIKData spnl;
	PluginMessage(ID_CDSPINALPLUGIN,&spnl);
	if(spnl.list) spnl.list->Append(node);
	
	return true;
}

void CDSpinalPlugin::InitSplineData(BaseContainer *tData)
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
	tData->SetData(SPN_BIAS_CURVE, spd);
}

Bool CDSpinalPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(SPN_DISK_LEVEL,level);
	
	return true;
}

void CDSpinalPlugin::Free(GeListNode *node)
{
	CDIKData spnl;
	PluginMessage(ID_CDSPINALPLUGIN,&spnl);
	if(spnl.list) spnl.list->Remove(node);
}

Bool CDSpinalPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument(); if(!doc) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	BaseObject *ch = op->GetDown(); if(!ch) return true;
	BaseObject *bCntr = tData->GetObjectLink(SPN_BASE_LINK,doc); if(!bCntr) return true;
	BaseObject *tCntr = tData->GetObjectLink(SPN_TIP_LINK,doc); if(!tCntr) return true;
	
	if(!tData->GetBool(SPN_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Matrix baseM = bCntr->GetMg();	
	Matrix tipM = tCntr->GetMg();
	Matrix bdM = baseM, tdM = tipM;

	Real bDepth = tData->GetReal(SPN_BASE_DEPTH);
	Real tDepth = tData->GetReal(SPN_TIP_DEPTH);
	
	Vector bVec, tVec;
	switch (tData->GetLong(SPN_POLE_AXIS))
	{
		case SPN_POLE_X:
			bVec = baseM.off + baseM.v1 * bDepth;
			tVec = tipM.off - tipM.v1 * tDepth;
			break;
		case SPN_POLE_Y:	
			bVec = baseM.off + baseM.v2 * bDepth;
			tVec = tipM.off - tipM.v2 * tDepth;
			break;
		case SPN_POLE_Z:	
			bVec = baseM.off + baseM.v3 * bDepth;
			tVec = tipM.off - tipM.v3 * tDepth;
			break;
		case SPN_POLE_NX:
			bVec = baseM.off - baseM.v1 * bDepth;
			tVec = tipM.off + tipM.v1 * tDepth;
			break;
		case SPN_POLE_NY:	
			bVec = baseM.off - baseM.v2 * bDepth;
			tVec = tipM.off + tipM.v2 * tDepth;
			break;
		case SPN_POLE_NZ:	
			bVec = baseM.off - baseM.v3 * bDepth;
			tVec = tipM.off + tipM.v3 * tDepth;
			break;
		default:
		break;
	}
	bd->SetPen(Vector(1,1,1));
	CDDrawLine(bd,baseM.off, bVec);
	CDDrawLine(bd,tipM.off, tVec);
	bd->SetPen(Vector(1,0,0));
	bdM.off = bVec; tdM.off = tVec;
	CDDrawCircle(bd,bdM);
	CDDrawCircle(bd,tdM);
	
	LONG jCnt = 0;
	BaseObject *jnt = op;
	ch = jnt->GetDown();
	
	bd->SetPen(tData->GetVector(SPN_LINE_COLOR));
	CDDrawLine(bd,jnt->GetMg().off, ch->GetMg().off);
	while (ch &&  jCnt < tData->GetLong(SPN_BONES_IN_GROUP))
	{
		CDDrawLine(bd,jnt->GetMg().off, ch->GetMg().off);
		jnt = ch;
		ch = ch->GetDown();
		if(ch) jCnt++;
	}
	CDDrawLine(bd,jnt->GetMg().off, tipM.off);
		
	if(tData->GetLong(SPN_USE) && tData->GetReal(SPN_BLEND) == 1.0) return true;
	
	bd->SetPen(Vector(1,1,0));
	
	SplineObject *spl = CDAllocateSplineObject(0,CD_SPLINETYPE_BSPLINE);
	spl->GetRealSpline();
	spl->ResizeObject(4,0);

	Vector  *padr = GetPointArray(spl);
	padr[0] = baseM.off;
	padr[1] = bVec;
	padr[2] = tVec;
	padr[3] = tipM.off;

	Vector oldpoint = padr[0];
	
	LONG i;
	for (i=1; i<24; i++)
	{
		Vector newPoint = spl->GetSplinePoint(CDUniformToNatural(spl,Real(i)/24.0));
		CDDrawLine(bd,oldpoint, newPoint);
		oldpoint = newPoint;
		jCnt++;
	}
	CDDrawLine(bd,oldpoint, tipM.off);
	
	SplineObject::Free(spl);
	
	return true;
}

Vector CDSpinalPlugin::GetTipRotation(BaseObject *jnt, LONG jCnt)
{
	Vector tipRot = Vector(0,0,0);
	LONG i;
	
	for(i=0; i<jCnt; i++)
	{
		if(jnt)
		{
			jnt = jnt->GetDown();
			tipRot.z += CDGetRot(jnt).z;
		}
	}
	
	return tipRot;
}

void CDSpinalPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(SPN_POLE_AXIS+T_LST,tData->GetLong(SPN_POLE_AXIS));
			tData->SetReal(SPN_BONES_IN_GROUP+T_LST,tData->GetReal(SPN_BONES_IN_GROUP));
			
			tData->SetBool(SPN_USE_TARGET_TWIST+T_LST,tData->GetBool(SPN_USE_TARGET_TWIST));
			tData->SetBool(SPN_INCLUDE_TIP_TWIST+T_LST,tData->GetBool(SPN_INCLUDE_TIP_TWIST));
			tData->SetLong(SPN_INTERPOLATION+T_LST,tData->GetLong(SPN_INTERPOLATION));
			
			tData->SetLink(SPN_BASE_LINK+T_LST,tData->GetLink(SPN_BASE_LINK,doc));
			tData->SetLink(SPN_TIP_LINK+T_LST,tData->GetLink(SPN_TIP_LINK,doc));
			
			tData->SetBool(SPN_CHANGE_VOLUME+T_LST,tData->GetBool(SPN_CHANGE_VOLUME));
			tData->SetBool(SPN_MIX_VOLUME+T_LST,tData->GetBool(SPN_MIX_VOLUME));
			tData->SetBool(SPN_USE_BIAS_CURVE+T_LST,tData->GetBool(SPN_USE_BIAS_CURVE));
			
			tData->SetBool(SPN_DEPTH_S_N_S+T_LST,tData->GetBool(SPN_DEPTH_S_N_S));
			tData->SetBool(SPN_SQUASH_DEPTH+T_LST,tData->GetBool(SPN_SQUASH_DEPTH));
			tData->SetBool(SPN_STRETCH_DEPTH+T_LST,tData->GetBool(SPN_STRETCH_DEPTH));
			
			tData->SetBool(SPN_CLAMP_STRETCH+T_LST,tData->GetBool(SPN_CLAMP_STRETCH));
			tData->SetReal(SPN_STRETCH_DIST+T_LST,tData->GetReal(SPN_STRETCH_DIST));
			tData->SetBool(SPN_CLAMP_SQUASH+T_LST,tData->GetBool(SPN_CLAMP_SQUASH));
			tData->SetReal(SPN_SQUASH_DIST+T_LST,tData->GetReal(SPN_SQUASH_DIST));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSpinalPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(SPN_POLE_AXIS,tData->GetLong(SPN_POLE_AXIS+T_LST));
		tData->SetReal(SPN_BONES_IN_GROUP,tData->GetReal(SPN_BONES_IN_GROUP+T_LST));
		
		tData->SetBool(SPN_USE_TARGET_TWIST,tData->GetBool(SPN_USE_TARGET_TWIST+T_LST));
		tData->SetBool(SPN_INCLUDE_TIP_TWIST,tData->GetBool(SPN_INCLUDE_TIP_TWIST+T_LST));
		tData->SetLong(SPN_INTERPOLATION,tData->GetLong(SPN_INTERPOLATION+T_LST));
			
		tData->SetLink(SPN_BASE_LINK,tData->GetLink(SPN_BASE_LINK+T_LST,doc));
		tData->SetLink(SPN_TIP_LINK,tData->GetLink(SPN_TIP_LINK+T_LST,doc));
		
		tData->SetBool(SPN_CHANGE_VOLUME,tData->GetBool(SPN_CHANGE_VOLUME+T_LST));
		tData->SetBool(SPN_MIX_VOLUME,tData->GetBool(SPN_MIX_VOLUME+T_LST));
		tData->SetBool(SPN_USE_BIAS_CURVE,tData->GetBool(SPN_USE_BIAS_CURVE+T_LST));
			
		tData->SetBool(SPN_DEPTH_S_N_S,tData->GetBool(SPN_DEPTH_S_N_S+T_LST));
		tData->SetBool(SPN_SQUASH_DEPTH,tData->GetBool(SPN_SQUASH_DEPTH+T_LST));
		tData->SetBool(SPN_STRETCH_DEPTH,tData->GetBool(SPN_STRETCH_DEPTH+T_LST));
		
		tData->SetBool(SPN_CLAMP_STRETCH,tData->GetBool(SPN_CLAMP_STRETCH+T_LST));
		tData->SetReal(SPN_STRETCH_DIST,tData->GetReal(SPN_STRETCH_DIST+T_LST));
		tData->SetBool(SPN_CLAMP_SQUASH,tData->GetBool(SPN_CLAMP_SQUASH+T_LST));
		tData->SetReal(SPN_SQUASH_DIST,tData->GetReal(SPN_SQUASH_DIST+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSpinalPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	LONG i, jCnt = ConfirmJointCount(op,tData);
	
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
			
			Vector tipRot = GetTipRotation(op,jCnt);
			tData->SetVector(SPN_TIP_ROTATION,tipRot);
			
			if(tData->GetLong(SPN_DISK_LEVEL) < 1)
			{
				InitSplineData(tData);
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

	Vector gScale = GetGlobalScale(op);
	switch (type)
	{
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
			if(tData->GetBool(SPN_LENGTH_SET))
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
							Vector lenV = VNorm(op->GetMg().v3) * tData->GetReal(SPN_JOINT_LENGTH+i);
							lenV.x *= sca.x;
							lenV.y *= sca.y;
							lenV.z *= sca.z;
							tData->SetReal(SPN_JOINT_LENGTH+i,Len(lenV));
							totalLen += Len(lenV);
							
							op = op->GetDown();
						}
					}
					tData->SetReal(SPN_TOTAL_LENGTH, totalLen);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SPN_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==SPN_SET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					if(jCnt > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						Real jlength, totalLength = 0;
						for(i=0; i<jCnt; i++)
						{
							if(op)
							{
								jlength = CDGetPos(op->GetDown()).z;
								tData->SetReal(SPN_JOINT_LENGTH+i,jlength);
								totalLength+=jlength;
								op = op->GetDown();
							}
							else break;
						}
						tData->SetBool(SPN_LENGTH_SET, true);
						tData->SetReal(SPN_TOTAL_LENGTH, totalLength);
						tData->SetLong(SPN_J_NUMBER, jCnt);
					}
				}
			}
			else if(dc->id[0].id==SPN_CLEAR_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					if(jCnt > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						Vector opPos;
						op = op->GetDown();
						for(i=0; i<jCnt; i++)
						{
							if(op)
							{
								opPos = CDGetPos(op);
								opPos.z = tData->GetReal(SPN_JOINT_LENGTH+i);
								CDSetPos(op,opPos);
								op = op->GetDown();
							}
							else break;
						}
						tData->SetBool(SPN_LENGTH_SET, false);
					}
				}
			}
			else if(dc->id[0].id==SPN_RESET_LENGTH)
			{
				if(tData->GetBool(T_REG))
				{
					if(jCnt > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						Real jlength, totalLength = 0;
						for(i=0; i<jCnt; i++)
						{
							if(op)
							{
								jlength = CDGetPos(op->GetDown()).z;
								tData->SetReal(SPN_JOINT_LENGTH+i,jlength);
								totalLength+=jlength;
								op = op->GetDown();
							}
							else break;
						}
						tData->SetBool(SPN_LENGTH_SET, true);
						tData->SetReal(SPN_TOTAL_LENGTH, totalLength);
						tData->SetLong(SPN_J_NUMBER, jCnt);
					}
				}
			}
			else if(dc->id[0].id==SPN_SET_SQUASH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					if(gScale.z != 0.0)
					{
						tData->SetReal(SPN_SQUASH_DIST, tData->GetReal(SPN_TOTAL_LENGTH) - sLen / gScale.z);
					}
					else
					{
						tData->SetReal(SPN_SQUASH_DIST, tData->GetReal(SPN_TOTAL_LENGTH) - sLen);
					}
					tData->SetBool(SPN_CLAMP_SQUASH, true);
				}
			}
			else if(dc->id[0].id==SPN_SET_STRETCH_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);

					if(gScale.z != 0.0)
					{
						tData->SetReal(SPN_STRETCH_DIST, sLen / gScale.z - tData->GetReal(SPN_TOTAL_LENGTH));
					}
					else
					{
						tData->SetReal(SPN_STRETCH_DIST, sLen - tData->GetReal(SPN_TOTAL_LENGTH));
					}
					tData->SetBool(SPN_CLAMP_STRETCH, true);
				}
			}
			break;
		}
	}

	return true;
}

LONG CDSpinalPlugin::ConfirmJointCount(BaseObject *op, BaseContainer *tData)
{
	LONG jCnt = 0;
	
	while(op &&  jCnt < tData->GetLong(SPN_BONES_IN_GROUP))
	{
		op = op->GetDown();
		if(op) jCnt++;
	}
	if(jCnt > tData->GetLong(SPN_J_NUMBER))  tData->SetBool(SPN_LENGTH_SET, false);
	tData->SetLong(SPN_BONES_IN_GROUP, jCnt);
	
	return jCnt;
}

Real CDSpinalPlugin::GetTotalJointLength(BaseObject *op, LONG jCnt)
{
	LONG i;
	Real totalLen = 0;
	
	BaseObject *prev = op;
	for (i=0; i<jCnt; i++)
	{
		op = op->GetDown();
		if(op)
		{
			totalLen += Len(op->GetMg().off - prev->GetMg().off);
			prev = op;
		}
	}
	
	return totalLen;
}

void CDSpinalPlugin::SetSplinePoints(SplineObject *spl, BaseObject *op, BaseContainer *tData, Matrix baseM, Matrix tipM, Real totalLen)
{
	Vector nTipPos, gScale = GetGlobalScale(op);
	Real sqDist = tData->GetReal(SPN_SQUASH_DIST) * gScale.z;
	Real tLen=0, jtLen = tData->GetReal(SPN_TOTAL_LENGTH) * gScale.z;
	Vector  *padr = GetPointArray(spl);

	padr[0] = baseM.off;
	Real bDepth = tData->GetReal(SPN_BASE_DEPTH);
	Real tDepth = tData->GetReal(SPN_TIP_DEPTH);
	Real dSca = 1;
	
	if(tData->GetBool(SPN_SQUASH_N_STRETCH))
	{
		if(tData->GetBool(SPN_DEPTH_S_N_S) && jtLen != 0.0)
		{
			Real cntrLen = Len(tipM.off - baseM.off);
			if(tData->GetBool(SPN_SQUASH_DEPTH) && cntrLen < jtLen)
			{
				dSca = cntrLen/jtLen;
				if(tData->GetBool(SPN_CLAMP_SQUASH)) dSca = sqDist/jtLen;
			}
			else if(tData->GetBool(SPN_STRETCH_DEPTH) && cntrLen > jtLen)
			{
				dSca = cntrLen/jtLen;
				if(tData->GetBool(SPN_CLAMP_STRETCH))
				{
					Real strDist = tData->GetReal(SPN_STRETCH_DIST) * gScale.z;
					dSca = strDist/jtLen;
				}
			}
		}
	}
	
	Vector bdPos, tdPos;
	switch (tData->GetLong(SPN_POLE_AXIS))
	{
		case SPN_POLE_X:
			bdPos = baseM.off + baseM.v1 * bDepth * dSca;
			tdPos = tipM.off - tipM.v1 * tDepth * dSca;
			break;
		case SPN_POLE_Y:	
			bdPos = baseM.off + baseM.v2 * bDepth * dSca;
			tdPos = tipM.off - tipM.v2 * tDepth * dSca;
			break;
		case SPN_POLE_Z:	
			bdPos = baseM.off + baseM.v3 * bDepth * dSca;
			tdPos = tipM.off - tipM.v3 * tDepth * dSca;
			break;
		case SPN_POLE_NX:
			bdPos = baseM.off - baseM.v1 * bDepth * dSca;
			tdPos = tipM.off + tipM.v1 * tDepth * dSca;
			break;
		case SPN_POLE_NY:	
			bdPos = baseM.off - baseM.v2 * bDepth * dSca;
			tdPos = tipM.off + tipM.v2 * tDepth * dSca;
			break;
		case SPN_POLE_NZ:	
			bdPos = baseM.off - baseM.v3 * bDepth * dSca;
			tdPos = tipM.off + tipM.v3 * tDepth * dSca;
			break;
		default:
		break;
	}
	padr[1] = bdPos;
	padr[2] = tdPos;
	padr[3] = tipM.off;
	
	sLen = CDGetSplineLength(spl) * gScale.z;
	
	if(tData->GetBool(SPN_SQUASH_N_STRETCH))
	{
		if(tData->GetBool(SPN_CLAMP_SQUASH) && sLen < jtLen - sqDist)
		{
			tLen = jtLen - sqDist - sLen;
		}
	}
	else
	{
		if(tData->GetBool(SPN_LENGTH_SET))
		{
			if(sLen < jtLen)
			{
				tLen = jtLen - sLen;
			}
		}
		else
		{
			if(sLen < totalLen)
			{
				tLen = totalLen - sLen;
			}
		}
	}
	
	switch (tData->GetLong(SPN_POLE_AXIS))
	{
		case SPN_POLE_X:
			nTipPos = tipM.off + tipM.v1 * tLen;
			break;
		case SPN_POLE_Y:	
			nTipPos = tipM.off + tipM.v2 * tLen;
			break;
		case SPN_POLE_Z:	
			nTipPos = tipM.off + tipM.v3 * tLen;
			break;
		case SPN_POLE_NX:
			nTipPos = tipM.off - tipM.v1 * tLen;
			break;
		case SPN_POLE_NY:	
			nTipPos = tipM.off - tipM.v2 * tLen;
			break;
		case SPN_POLE_NZ:	
			nTipPos = tipM.off - tipM.v3 * tLen;
			break;
		default:
		break;
	}
	
	padr[3] = nTipPos;
}

void CDSpinalPlugin::SetSquashAndStretch(BaseObject *op, BaseContainer *tData, CDSplineJoint *spJnt, LONG jCnt)
{
	Vector gScale = GetGlobalScale(op), jntPos;
	Real sqDist = tData->GetReal(SPN_SQUASH_DIST) * gScale.z, totalLen = tData->GetReal(SPN_TOTAL_LENGTH) * gScale.z;
	Real stDist = tData->GetReal(SPN_STRETCH_DIST) * gScale.z;
	Real volume = tData->GetReal(SPN_VOLUME_STRENGTH);
	
	LONG i;
	BaseObject *jnt = op;
	
	squash = 1;
	stretch = 1; 
	if(!tData->GetBool(SPN_SQUASH_N_STRETCH) || !tData->GetBool(SPN_LENGTH_SET))
	{
		if(squNstrON)
		{
			for(i=0; i<jCnt; i++)
			{
				if(jnt)
				{
					spJnt[i].length = tData->GetReal(SPN_JOINT_LENGTH+i);

					jnt = jnt->GetDown();
					jntPos = CDGetPos(jnt);
					jntPos.z = spJnt[i].length;
					CDSetPos(jnt,jntPos);
				}
				else break;
			}
			squNstrON = false;
		}
	}
	else
	{
		stretch = sLen/totalLen;
		
		if(tData->GetBool(SPN_CLAMP_STRETCH) && sLen > totalLen + stDist)
		{
			stretch = (totalLen + stDist)/totalLen;
		}
		if(tData->GetBool(SPN_CLAMP_SQUASH) && sLen < totalLen - sqDist)
		{
			stretch = (totalLen - sqDist)/totalLen;
		}
		
		if(tData->GetBool(SPN_CHANGE_VOLUME))
		{
			Real skinVol;
			if(stretch < 1)
			{
				skinVol = 0.5;
				squash = ((stretch * skinVol) - 1)/(skinVol-1);
				if(tData->GetBool(SPN_USE_BIAS_CURVE)) squash *= 3.5;
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
				if(tData->GetBool(SPN_USE_BIAS_CURVE)) squash *= 0.1;
				if(volume < 1)
				{
					Real sqVol = CDBlend(1.0,squash,volume);
					squash = sqVol;
				}
				else if(volume > 1) squash *= 1/volume;
			}
		}
		else tData->SetBool(SPN_MIX_VOLUME,false);
		
		squNstrON = true;
		
	}
	
	SplineData *biasSpline = (SplineData*)tData->GetCustomDataType(SPN_BIAS_CURVE,CUSTOMDATATYPE_SPLINE);
	if(biasSpline && totalLen != 0)
	{
		jnt = op;
		BaseContainer *jData = NULL;
		Real splStart = 0, splEnd = 0;
		for(i=0; i<jCnt; i++)
		{
			if(jnt)
			{
				jData = jnt->GetDataInstance();
				if(jData && jnt->IsInstanceOf(ID_CDJOINTOBJECT))
				{
					jData->SetReal(JNT_J_LENGTH,tData->GetReal(SPN_JOINT_LENGTH+i));
					jData->SetReal(JNT_SPL_START,splStart);
					
					splEnd += tData->GetReal(SPN_JOINT_LENGTH+i)/totalLen;
					if(splEnd > 1.0) splEnd = 1.0;
					jData->SetReal(JNT_SPL_END,splEnd);
					
					if(i == 0 && !tData->GetBool(SPN_MIX_VOLUME)) jData->SetBool(JNT_J_BASE,true);
					else jData->SetBool(JNT_J_BASE,false);
					
					jData->SetReal(JNT_STRETCH_VALUE,stretch);
					jData->SetReal(JNT_SQUASH_VALUE,squash);
					jData->SetData(JNT_S_AND_S_SPLINE,GeData(CUSTOMDATATYPE_SPLINE,*biasSpline));
					jData->SetBool(JNT_USE_SPLINE,tData->GetBool(SPN_USE_BIAS_CURVE));
					jData->SetReal(JNT_SKIN_VOLUME,tData->GetReal(SPN_VOLUME_STRENGTH));
					jnt = jnt->GetDown();
					splStart = splEnd;
				}
			}
		}
	}
}

void CDSpinalPlugin::StoreSplineJointValues(BaseObject *jnt, BaseContainer *tData, CDSplineJoint *spJnt, LONG jCnt)
{
	LONG i;
	
	for(i=0; i<(jCnt+1); i++)
	{
		if(jnt)
		{
			spJnt[i].jnt = jnt;
			spJnt[i].jntM = jnt->GetMg();
			spJnt[i].rot = CDGetRot(jnt);
			spJnt[i].sca = CDGetScale(jnt);
			
			Vector jntPos = CDGetPos(jnt);
			if(i>0 && tData->GetBool(SPN_CONNECT_BONES))
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
				if(tData->GetBool(SPN_SQUASH_N_STRETCH) && tData->GetBool(SPN_LENGTH_SET)) spJnt[i].length = tData->GetReal(SPN_JOINT_LENGTH+i);
				else spJnt[i].length = CDGetPos(jnt).z;
			}
			else spJnt[i].length = 0.0;
		}
	}
}

Real CDSpinalPlugin::GetTwistLength(CDSplineJoint *spJnt, LONG cnt, Real sca)
{
	Real twLen = 0.0;
	LONG i;
	
	for(i=0; i<cnt; i++)
	{
		twLen += spJnt[i].length * sca;
	}
	
	return twLen;
}

Matrix CDSpinalPlugin::GetSpineAxisRotM(BaseContainer *tData)
{
	Matrix rotM = Matrix();
	
	switch (tData->GetLong(SPN_POLE_AXIS))
	{
		case SPN_POLE_X:
			rotM = Matrix(Vector(0,0,0), Vector(0,0,-1), Vector(0,1,0), Vector(1,0,0));
			break;
		case SPN_POLE_Y:	
			rotM = Matrix(Vector(0,0,0), Vector(1,0,0), Vector(0,0,-1), Vector(0,1,0));
			break;
		case SPN_POLE_Z:	
			rotM = Matrix(Vector(0,0,0), Vector(1,0,0), Vector(0,1,0), Vector(0,0,1));
			break;
		case SPN_POLE_NX:
			rotM = Matrix(Vector(0,0,0), Vector(0,0,1), Vector(0,1,0), Vector(-1,0,0));
			break;
		case SPN_POLE_NY:	
			rotM = Matrix(Vector(0,0,0), Vector(1,0,0), Vector(0,0,1), Vector(0,-1,0));
			break;
		case SPN_POLE_NZ:	
			rotM = Matrix(Vector(0,0,0), Vector(-1,0,0), Vector(0,1,0), Vector(0,0,-1));
			break;
	}
	
	return rotM;
}

Vector CDSpinalPlugin::GetMaxTipRotation(BaseContainer *tData, BaseObject *op1, BaseObject *op2)
{
	Vector maxRot = Vector(0,0,0);
	
	Vector gRot = GetGlobalRotation(op2) - GetGlobalRotation(op1);
	switch (tData->GetLong(SPN_POLE_AXIS))
	{
		case SPN_POLE_X:
			maxRot.z = gRot.x;
			if(Abs(gRot.y) > Abs(maxRot.z)) maxRot.z = gRot.y;
			break;
		case SPN_POLE_Y:	
			maxRot.z = gRot.y;
			if(Abs(gRot.x) > Abs(maxRot.z)) maxRot.z = gRot.x;
			break;
		case SPN_POLE_Z:	
			maxRot.z = gRot.z;
			break;
		case SPN_POLE_NX:
			maxRot.z = -gRot.x;
			if(Abs(gRot.y) > Abs(maxRot.z)) maxRot.z = -gRot.y;
			break;
		case SPN_POLE_NY:	
			maxRot.z = -gRot.y;
			if(Abs(gRot.x) > Abs(maxRot.z)) maxRot.z = -gRot.x;
			break;
		case SPN_POLE_NZ:	
			maxRot.z = -gRot.z;
			break;
	}
	
	return maxRot;
}

Bool CDSpinalPlugin::SetJointChainTwist(BaseTag *tag, BaseDocument *doc, BaseObject *op, CDSplineJoint *spJnt, LONG jCnt)
{
	BaseContainer	*tData = tag->GetDataInstance();
	BaseObject		*bCntr = tData->GetObjectLink(SPN_BASE_LINK,doc); if(!bCntr) return false;
	BaseObject		*tCntr = tData->GetObjectLink(SPN_TIP_LINK,doc); if(!tCntr) return false;
	
	Matrix bContM = bCntr->GetMg(), tContM = tCntr->GetMg();
	LONG i, twCnt = jCnt;
	Vector gScale = GetGlobalScale(op);
		
	if(!tData->GetBool(SPN_USE_TARGET_TWIST))
	{
		Real tTwist = tData->GetReal(SPN_TIP_TWIST); 
		Real bTwist = tData->GetReal(SPN_BASE_TWIST);
		
		Vector tContRot = Vector(0,0,0);
		tContRot.z = tTwist - bTwist;
		
		Real tMix, bank = 0.0, jLen = 0.0, twLen = GetTwistLength(spJnt,twCnt-1,gScale.z);
		for(i=0; i<twCnt; i++)
		{
			if(twLen > 0.0) tMix = jLen/twLen;
			else tMix = 0.0;
			
			spJnt[i].twist = CDBlend(0.0,tContRot.z,tMix);
			spJnt[i].jntM = spJnt[i].jntM * MatrixRotZ(spJnt[i].twist);
			spJnt[i].twist -= bank;
			bank += spJnt[i].twist;
			if(i > 0)
			{
				spJnt[i].rot = CDGetOptimalAngle(spJnt[i].rot, CDMatrixToHPB(MInv(spJnt[i-1].jntM) * spJnt[i].jntM, spJnt[i].jnt), spJnt[i].jnt);
			}
			
			jLen += spJnt[i].length * gScale.z;
		}
	}
	else
	{
		Matrix tipM = tContM * GetSpineAxisRotM(tData);
		Matrix jEndM = spJnt[jCnt].jntM;
		
		tipM.off = jEndM.off;
		Real xdot = Abs(VDot(VNorm(jEndM.v1), VNorm(tipM.v1)));
		Real ydot = Abs(VDot(VNorm(jEndM.v2), VNorm(tipM.v2)));
		if(ydot > xdot)
		{
			tipM.v3 = VNorm(jEndM.v3);
			tipM.v1 = VNorm(VCross(tipM.v2, tipM.v3));
			tipM.v2 = VNorm(VCross(tipM.v3, tipM.v1));
		}
		else
		{
			tipM.v3 = VNorm(jEndM.v3);
			tipM.v2 = VNorm(VCross(tipM.v3, tipM.v1));
			tipM.v1 = VNorm(VCross(tipM.v2, tipM.v3));
		}
		Vector tipMRot = CDMatrixToHPB(MInv(jEndM) * tipM, tCntr);
		tipMRot.x = 0.0;
		tipMRot.y = 0.0;
		
		Vector gRot,tipRot = tData->GetVector(SPN_TIP_ROTATION);
		Vector tContRot = CDGetOptimalAngle(tipRot,tipMRot, tCntr);

		if(!CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING))
		{
			if(doc->GetAction() == ID_MODELING_ROTATE)
			{
				if(doc->GetTime().GetFrame(doc->GetFps()) == tData->GetLong(SPN_PREVIOUS_FRAME))
				{
					if(!VEqual(tipRot,tContRot,0.001))
					{
						if(!undoOn)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE,tag);
							undoOn = true;
						}
					}
				}
			}
			else undoOn = false;
		}
		
		if(tData->GetLong(SPN_INTERPOLATION) == SPN_LONGEST)
		{
			Vector maxRot = GetMaxTipRotation(tData,bCntr,tCntr);
			if(Abs(tContRot.z) != Abs(maxRot.z))
			{
				tContRot = CDGetOptimalAngle(maxRot,tipMRot, tCntr);
			}
		}
		
		if(tData->GetBool(SPN_INCLUDE_TIP_TWIST)) twCnt = jCnt + 1;
		Real tMix, bank = 0.0, jLen = 0.0, twLen = GetTwistLength(spJnt,twCnt-1,gScale.z);
		for(i=0; i<twCnt; i++)
		{
			if(twLen > 0.0) tMix = jLen/twLen;
			else tMix = 0.0;
			
			CDQuaternion quatA = CDQuaternion(), quatB, quat25, quat50, quat75, twistQuat;
			switch (tData->GetLong(SPN_INTERPOLATION))
			{
				case SPN_SHORTEST:
					quatB.SetMatrix(MInv(jEndM) * tipM);
					twistQuat = CDQSlerp(quatA,quatB,tMix);
					spJnt[i].jntM = spJnt[i].jntM * twistQuat.GetMatrix();
					break;
				case SPN_AVERAGE:
					if(tContRot.z > 6.283185307) tContRot.z -= 6.283185307;
					if(tContRot.z < -6.283185307) tContRot.z += 6.283185307;
					quatB.SetHPB(tContRot);
					quat50 = !(quatB + quatA); // calculate the 50 quat
					if(quat50.w == 0.0 && quat50.v.z == 0.0) quat50.v.z = -1.0;
					quat25 = !(quat50 + quatA); // calculate the 25 quat
					if(quat25.w == 0.0 && quat25.v.z == 0.0) quat25.v.z = -1.0;
					quat75 = !(quat50 + quatB); // calculate the 75 quat
					if(quat75.w == 0.0 && quat75.v.z == 0.0) quat75.v.z = -1.0;
					// interpolate 
					twistQuat = CDQSlerpBezier(quatA,quat25,quat50,quat75,quatB,tMix);
					spJnt[i].jntM = spJnt[i].jntM * twistQuat.GetMatrix();
					break;
				case SPN_LONGEST:
					spJnt[i].twist = CDBlend(0.0,tContRot.z,tMix);
					spJnt[i].jntM = spJnt[i].jntM * MatrixRotZ(spJnt[i].twist);
					spJnt[i].twist -= bank;
					bank += spJnt[i].twist;
					break;
			}
			if(i > 0)
			{
				spJnt[i].rot = CDGetOptimalAngle(spJnt[i].rot, CDMatrixToHPB(MInv(spJnt[i-1].jntM) * spJnt[i].jntM, spJnt[i].jnt), spJnt[i].jnt);
			}
			
			jLen += spJnt[i].length * gScale.z;
		}
		tData->SetVector(SPN_TIP_ROTATION,tContRot);
	}
	
	return true;
}


Bool CDSpinalPlugin::CheckLinks(BaseObject *op, BaseDocument *doc, BaseContainer *tData)
{
	BaseObject *ch = op->GetDown();
	if(!ch)
	{
		tData->SetBool(SPN_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *bCntr = tData->GetObjectLink(SPN_BASE_LINK,doc);
	if(!bCntr)
	{
		tData->SetBool(SPN_REST_ROTATION_SET,false);
		return false;
	}
	BaseObject *tCntr = tData->GetObjectLink(SPN_TIP_LINK,doc);
	if(!tCntr)
	{
		tData->SetBool(SPN_REST_ROTATION_SET,false);
		return false;
	}
	
	return true;
}

void CDSpinalPlugin::SetInitialBaseRotation(BaseContainer *tData, BaseObject *op, BaseObject *bCntr)
{
	Vector opGRot = GetGlobalRotation(op), bGRot = GetGlobalRotation(bCntr);
	
	switch (tData->GetLong(SPN_POLE_AXIS))
	{
		case SPN_POLE_X:
			bGRot += Vector(-1.570796,0,0);
			break;
		case SPN_POLE_Y:	
			bGRot += Vector(0,1.570796,0);
			break;
		case SPN_POLE_NX:
			bGRot += Vector(1.570796,0,0);
			break;
		case SPN_POLE_NY:	
			bGRot += Vector(0,-1.570796,0);
			break;
		case SPN_POLE_NZ:	
			bGRot += Vector(3.141592,0,0);
			break;
	}
	SetGlobalRotation(op,CDGetOptimalAngle(bGRot,opGRot,op));
}

LONG CDSpinalPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer 	*tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	if(!CheckLinks(op,doc,tData)) return false;

	BaseObject *bCntr = tData->GetObjectLink(SPN_BASE_LINK,doc);
	BaseObject *tCntr = tData->GetObjectLink(SPN_TIP_LINK,doc);
		
	if(!tData->GetBool(SPN_USE_TARGET_TWIST)) tData->SetBool(SPN_INCLUDE_TIP_TWIST,false);
	
	tData->SetBool(SPN_STRETCH_BONES,false);
	
	Matrix opM = op->GetMg(), baseM = bCntr->GetMg(), tipM = tCntr->GetMg();	
	Vector theScale, chPos, oldRot, newRot, rotSet;
	Real ikMix = tData->GetReal(SPN_BLEND);
	LONG i,twCnt;

	SetInitialBaseRotation(tData,op,bCntr);
	
	LONG jCnt = ConfirmJointCount(op,tData);
	
	if(!tData->GetBool(SPN_USE))
	{
		BaseObject *tip = op;
		for(i=0; i<jCnt; i++)
		{
			if(tip->GetDown()) tip = tip->GetDown();
		}
		
		baseM.off = opM.off;
		theScale = CDGetScale(bCntr);		
		oldRot = CDGetRot(bCntr);
		bCntr->SetMg(baseM);
		
		newRot = CDGetRot(bCntr);
		rotSet = CDGetOptimalAngle(oldRot, newRot, bCntr);
		CDSetRot(bCntr,rotSet);	
		CDSetScale(bCntr,theScale);	

		tipM.off = tip->GetMg().off;
		theScale = CDGetScale(tCntr);		
		oldRot = CDGetRot(tCntr);
		tCntr->SetMg(tipM);
		
		newRot = CDGetRot(tCntr);
		rotSet = CDGetOptimalAngle(oldRot, newRot, tCntr);
		CDSetRot(tCntr,rotSet);	
		CDSetScale(tCntr,theScale);	
		
		return true;
	}
	
	if(!tData->GetBool(SPN_REST_ROTATION_SET))
	{
		tData->SetVector(SPN_REST_ROTATION,CDGetRot(op));
		tData->SetBool(SPN_REST_ROTATION_SET,true);
	}

	// Create Spline object
	SplineObject *spl = CDAllocateSplineObject(0,CD_SPLINETYPE_BSPLINE);
	spl->ResizeObject(4,0); 
	
	Real totalLen = GetTotalJointLength(op,jCnt);
	
	CDArray<CDSplineJoint> spJnt;
	spJnt.Alloc(jCnt+1);
	StoreSplineJointValues(op,tData,spJnt.Array(),jCnt);
	 
	SetSplinePoints(spl,op,tData,baseM,tipM,totalLen);
	
	sLen = CDGetSplineLength(spl);
	
	SetSquashAndStretch(op,tData,spJnt.Array(),jCnt);
	
	Vector gScale = GetGlobalScale(op);
	Real jLen, chainLen = 0.0;
	
	Matrix jntM = ScaleMatrix(GetNormalizedMatrix(baseM),gScale) * GetSpineAxisRotM(tData);
	if(!tData->GetBool(SPN_USE_TARGET_TWIST))
	{
		jntM = jntM * MatrixRotZ(tData->GetReal(SPN_BASE_TWIST));
	}
	
	for(i=0; i<jCnt; i++)
	{
		// add the length of current joint to the chain length
		if(tData->GetBool(SPN_SQUASH_N_STRETCH))
		{
			jLen = tData->GetReal(SPN_JOINT_LENGTH+i) * gScale.z * stretch;
		}
		else
		{
			jLen = spJnt[i].length * gScale.z;
		}
		chainLen += jLen;
		
		// get the joint's aim vector
		Vector aimPoint = spl->GetSplinePoint(CDUniformToNatural(spl,chainLen/sLen));
		if(tData->GetBool(SPN_SQUASH_N_STRETCH) && !tData->GetBool(SPN_CLAMP_STRETCH) && i == jCnt-1)
		{
			aimPoint = spl->GetSplinePoint(CDUniformToNatural(spl,1.0));
		}
		
		Vector mSca = GetMatrixScale(jntM); // save the current matrix scale
		
		// rotate the matrix to point to the aim vector
		Vector axis = VNorm(VCross(VNorm(MInv(jntM) * aimPoint), Vector(0,0,1)));
		Real angle = ACos(VDot(VNorm(aimPoint - jntM.off), VNorm(jntM.v3)));
		jntM = jntM * RotAxisToMatrix(axis,angle);
		
		//re-square up matrix
		jntM.v3 = VNorm(aimPoint - jntM.off);
		jntM.v1 = VNorm(VCross(jntM.v2, jntM.v3));
		jntM.v2 = VNorm(VCross(jntM.v3, jntM.v1));
		
		jntM = ScaleMatrix(jntM,mSca); // restore the original matrix scale
		
		spJnt[i].jntM = jntM;
		if(i > 0)
		{
			spJnt[i].rot = CDGetOptimalAngle(Vector(0,0,0), CDMatrixToHPB(MInv(spJnt[i-1].jntM) * jntM, spJnt[i].jnt), spJnt[i].jnt);
			spJnt[i].pos = MInv(spJnt[i-1].jntM) * jntM.off;
		}
		
		//move the offset to the next joint's position
		jntM.off = jntM.off + VNorm(jntM.v3) * jLen;
		if(tData->GetBool(SPN_SQUASH_N_STRETCH) && i == jCnt-1)
		{
			jntM.off = aimPoint;
		}
	}
	spJnt[i].jntM = jntM;
	spJnt[i].pos = MInv(spJnt[i-1].jntM) * jntM.off;
	
	SetJointChainTwist(tag,doc,op,spJnt.Array(),jCnt);
	
	joints = true;
	BaseObject *jnt = op;
	if(ikMix > 0.0) spJnt[0].rot = Vector(0,0,0);
	
	twCnt = jCnt;
	if(tData->GetBool(SPN_INCLUDE_TIP_TWIST)) twCnt = jCnt + 1;
	CDQuaternion fkQ, ikQ, mixQ, quat25, quat50, quat75;
	for(i=0; i<twCnt; i++)
	{
		if(jnt)
		{
			if(jnt->GetType() != ID_CDJOINTOBJECT) joints = false;
			
			Matrix setM;
			if(ikMix < 1.0)
			{
				if(i > 0)
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
			
			Vector jntRot = CDGetOptimalAngle(spJnt[i].rot,CDGetRot(jnt), jnt);
			CDSetRot(jnt,jntRot);
			CDSetScale(jnt,spJnt[i].sca);
			
			jnt = jnt->GetDown();
		}
	}
	if(i == jCnt && !tData->GetBool(SPN_INCLUDE_TIP_TWIST))
	{
		jnt->SetMg(spJnt[i].jntM);
		CDSetRot(jnt,spJnt[i].rot);
		CDSetScale(jnt,spJnt[i].sca);
		if(jnt) CDSetPos(jnt,spJnt[i].pos);
	}
	
	SplineObject::Free(spl);
	spJnt.Free();
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSpinalPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SPN_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSpinalPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case SPN_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPN_BONES_IN_GROUP:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPN_USE_TARGET_TWIST:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPN_INTERPOLATION:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_USE_TARGET_TWIST)) return true;
				else return false;
			}
		case SPN_BASE_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPN_BASE_DEPTH:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPN_TIP_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPN_TIP_DEPTH:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPN_INCLUDE_TIP_TWIST:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SPN_USE_TARGET_TWIST);
		case SPN_TIP_TWIST:
			if(!tData->GetBool(SPN_USE_TARGET_TWIST)) return true;
			else return false;
		case SPN_BASE_TWIST:
			if(!tData->GetBool(SPN_USE_TARGET_TWIST)) return true;
			else return false;
		case SPN_SET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(SPN_LENGTH_SET)) return true;
				else return false;
			}
		case SPN_CLEAR_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET)) return true;
				else return false;
			}
		case SPN_RESET_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET)) return true;
				else return false;
			}
		case SPN_SQUASH_N_STRETCH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET)) return true;
				else return false;
			}
		case SPN_CHANGE_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET) && tData->GetBool(SPN_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case SPN_VOLUME_STRENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET) && tData->GetBool(SPN_SQUASH_N_STRETCH) && joints) return true;
				else return false;
			}
		case SPN_CLAMP_SQUASH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET) && tData->GetBool(SPN_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPN_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET)) return true;
				else return false;
			}
		case SPN_SET_SQUASH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(SPN_CLAMP_SQUASH) && tData->GetBool(SPN_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPN_CLAMP_STRETCH:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET) && tData->GetBool(SPN_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPN_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET)) return true;
				else return false;
			}
		case SPN_SET_STRETCH_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(SPN_CLAMP_STRETCH) && tData->GetBool(SPN_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPN_DEPTH_S_N_S:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(SPN_LENGTH_SET) && tData->GetBool(SPN_SQUASH_N_STRETCH)) return true;
				else return false;
			}
		case SPN_SQUASH_DEPTH:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SPN_DEPTH_S_N_S) && tData->GetBool(SPN_SQUASH_N_STRETCH);
		case SPN_STRETCH_DEPTH:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SPN_DEPTH_S_N_S) && tData->GetBool(SPN_SQUASH_N_STRETCH);
		case SPN_USE_BIAS_CURVE:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SPN_CHANGE_VOLUME);
		case SPN_MIX_VOLUME:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SPN_USE_BIAS_CURVE);
		case SPN_BIAS_CURVE:
			return tData->GetBool(SPN_USE_BIAS_CURVE);
	}
	return true;
}

Bool RegisterCDSpinalPlugin(void)
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
	String name=GeLoadString(IDS_CDSPINAL); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSPINALPLUGIN,name,info,CDSpinalPlugin::Alloc,"tCDSpinal","CDspinal.tif",1);
}

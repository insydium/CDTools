// Cactus Dan's Springy Keys plugin
// Copyright 2010 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDGeneral.h"
#include "CDRungeKutta.h"
#include "CDCompatibility.h"
#include "CDTagData.h"

#include "CDSpringyKeys.h"

#include "tCDSpringyKeys.h"

enum
{
	//SPK_PURCHASE			= 1000,
	
	//SPK_STRENGTH			= 1200,
	
	//SPK_P_STIFFNESS			= 1201,
	//SPK_P_DAMPING				= 1202,
	//SPK_P_MASS				= 1203,
	
	//SPK_USE_POSITION		= 1204,
	//SPK_USE_ROTATION		= 1205,
	//SPK_USE_SCALE			= 1206,
	
	//SPK_SPLIT_FORCES		= 1300,
	//SPK_POS_STATIC			= 1301,
	//SPK_SCA_STATIC			= 1302,
	//SPK_ROT_STATIC			= 1303,
	
	
	//SPK_S_STIFFNESS			= 1304,
	//SPK_S_DAMPING			= 1305,
	//SPK_S_MASS				= 1306,
	
	//SPK_R_STIFFNESS			= 1307,
	//SPK_R_DAMPING			= 1308,
	//SPK_R_MASS				= 1309,
	
	//SPK_ID_FORCES			= 3000.
	//SPK_ID_SPLIT			= 3001
	//SPK_ID_P				= 3002,
	//SPK_ID_S				= 3003,
	//SPK_ID_R				= 3004
	
	SPK_PREV_TIME				= 10000,
	SPK_PREV_M					= 10001,
	SPK_PREV_SCA				= 10002,
	SPK_PREV_FRAME				= 10003
};

#if API_VERSION < 9800
static Bool PSRToolsActive(BaseDocument *doc)
{
	if(doc->GetAction() == ID_MODELING_MOVE) return true;
	if(doc->GetAction() == ID_MODELING_SCALE) return true;
	if(doc->GetAction() == ID_MODELING_ROTATE) return true;
	return false;
}
#endif

class CDSpringyKeys : public CDTagData
{
private:
	Real				t;
	CDRungeKutta		rk4;
	State				opState, prvState;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
		
	Bool HasAnimation(BaseObject *op);
	LONG GetFirstKeyFrame(BaseObject *op, LONG fps);
	LONG GetLastKeyFrame(BaseObject *op, LONG fps);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);

	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSpringyKeys); }
};

Bool CDSpringyKeys::Init(GeListNode *node)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(SPK_USE_POSITION,true);
	tData->SetBool(SPK_USE_SCALE,false);
	tData->SetBool(SPK_USE_ROTATION,true);
	
	tData->SetReal(SPK_STRENGTH,1.0);
	
	tData->SetBool(SPK_SPLIT_FORCES,false);
	
	tData->SetReal(SPK_P_STIFFNESS,0.5);
	tData->SetReal(SPK_P_DAMPING,0.5);
	tData->SetReal(SPK_P_MASS,1.0);
	
	tData->SetReal(SPK_S_STIFFNESS,0.5);
	tData->SetReal(SPK_S_DAMPING,0.5);
	tData->SetReal(SPK_S_MASS,1.0);
	
	tData->SetReal(SPK_R_STIFFNESS,0.5);
	tData->SetReal(SPK_R_DAMPING,0.5);
	tData->SetReal(SPK_R_MASS,1.0);
	
	tData->SetReal(SPK_PREV_TIME,0.0);
	tData->SetMatrix(SPK_PREV_M,Matrix());
	tData->SetVector(SPK_PREV_SCA,Vector());
	tData->SetLong(SPK_PREV_FRAME,0);

	t = 0.0;

	opState.position = Vector(0,0,0);
	opState.momentum = Vector(0,0,0);
	opState.velocity = Vector(0,0,0);
	
	opState.scale = Vector(0,0,0);
	opState.scaleMomentum = Vector(0,0,0);
	opState.scaleVelocity = Vector(0,0,0);
	
	opState.orientation.SetMatrix(Matrix());
	opState.spin.SetMatrix(Matrix());
	opState.angularMomentum = Vector(0,0,0);
	opState.angularVelocity = Vector(0,0,0);

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,GeData(false));
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDSpringyKeys::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	((CDSpringyKeys*)dest)->opState = opState;
	((CDSpringyKeys*)dest)->prvState = prvState;
	
	return true;
}

void CDSpringyKeys::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdsknr = tData->GetString(T_STR);
	
	if(!CheckKeyChecksum(cdsknr)) reg = false;
	
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	kb = GetKeyByte(cdsknr,pK,2);
	b = GetKeyByte(seed,aK,bK,cK);
	if(kb != LongToHexString(b,2)) reg = false;
	
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
			
			tData->SetBool(SPK_USE_POSITION+T_LST,tData->GetBool(SPK_USE_POSITION));
			tData->SetBool(SPK_USE_ROTATION+T_LST,tData->GetBool(SPK_USE_ROTATION));
			
			tData->SetBool(SPK_SPLIT_FORCES+T_LST,tData->GetBool(SPK_SPLIT_FORCES));
			
			tData->SetReal(SPK_P_STIFFNESS+T_LST,tData->GetReal(SPK_P_STIFFNESS));
			tData->SetReal(SPK_P_DAMPING+T_LST,tData->GetReal(SPK_P_DAMPING));
			tData->SetReal(SPK_P_MASS+T_LST,tData->GetReal(SPK_P_MASS));
			
			tData->SetReal(SPK_S_STIFFNESS+T_LST,tData->GetReal(SPK_S_STIFFNESS));
			tData->SetReal(SPK_S_DAMPING+T_LST,tData->GetReal(SPK_S_DAMPING));
			tData->SetReal(SPK_S_MASS+T_LST,tData->GetReal(SPK_S_MASS));
			
			tData->SetReal(SPK_R_STIFFNESS+T_LST,tData->GetReal(SPK_R_STIFFNESS));
			tData->SetReal(SPK_R_DAMPING+T_LST,tData->GetReal(SPK_R_DAMPING));
			tData->SetReal(SPK_R_MASS+T_LST,tData->GetReal(SPK_R_MASS));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSpringyKeys::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetBool(SPK_USE_POSITION,tData->GetBool(SPK_USE_POSITION+T_LST));
		tData->SetBool(SPK_USE_ROTATION,tData->GetBool(SPK_USE_ROTATION+T_LST));
		
		tData->SetBool(SPK_SPLIT_FORCES,tData->GetBool(SPK_SPLIT_FORCES+T_LST));
		
		tData->SetReal(SPK_P_STIFFNESS,tData->GetReal(SPK_P_STIFFNESS+T_LST));
		tData->SetReal(SPK_P_DAMPING,tData->GetReal(SPK_P_DAMPING+T_LST));
		tData->SetReal(SPK_P_MASS,tData->GetReal(SPK_P_MASS+T_LST));
		
		tData->SetReal(SPK_S_STIFFNESS,tData->GetReal(SPK_S_STIFFNESS+T_LST));
		tData->SetReal(SPK_S_DAMPING,tData->GetReal(SPK_S_DAMPING+T_LST));
		tData->SetReal(SPK_S_MASS,tData->GetReal(SPK_S_MASS+T_LST));
		
		tData->SetReal(SPK_R_STIFFNESS,tData->GetReal(SPK_R_STIFFNESS+T_LST));
		tData->SetReal(SPK_R_DAMPING,tData->GetReal(SPK_R_DAMPING+T_LST));
		tData->SetReal(SPK_R_MASS,tData->GetReal(SPK_R_MASS+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSpringyKeys::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	BaseDocument *doc = tag->GetDocument();
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDSK_SERIAL_SIZE];
			String cdsknr;
			
			CDReadPluginInfo(ID_CDSPRINGYKEYS,snData,CDSK_SERIAL_SIZE);
			cdsknr.SetCString(snData,CDSK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdsknr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDSK_SERIAL_SIZE];
			String cdsknr;
			
			CDReadPluginInfo(ID_CDSPRINGYKEYS,snData,CDSK_SERIAL_SIZE);
			cdsknr.SetCString(snData,CDSK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdsknr);
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
			if(dc->id[0].id==SPK_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

Bool CDSpringyKeys::HasAnimation(BaseObject *op)
{
	Bool anim = false;
	
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_POSITION)) anim = true;
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_SCALE)) anim = true;
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION)) anim = true;
	
	return anim;
}

LONG CDSpringyKeys::GetFirstKeyFrame(BaseObject *op, LONG fps)
{
	LONG frm = CDMAXLONG;
	
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_POSITION))
	{
		frm = CDGetFirstVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_POSITION);
	}
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_SCALE))
	{
		LONG sFrm = CDGetFirstVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_SCALE);
		if(sFrm < frm) frm = sFrm;
	}
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION))
	{
		LONG rFrm = CDGetFirstVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_ROTATION);
		if(rFrm < frm) frm = rFrm;
	}
	
	return frm;
}

LONG CDSpringyKeys::GetLastKeyFrame(BaseObject *op, LONG fps)
{
	LONG frm = CDMAXLONG;
	
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_POSITION))
	{
		frm = CDGetLastVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_POSITION);
	}
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_SCALE))
	{
		LONG sFrm = CDGetLastVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_POSITION);
		if(sFrm > frm) frm = sFrm;
	}
	if(CDHasVectorTrack(op,CD_ID_BASEOBJECT_ROTATION))
	{
		LONG rFrm = CDGetLastVectorKeyFrame(op,fps,CD_ID_BASEOBJECT_POSITION);
		if(rFrm > frm) frm = rFrm;
	}
	
	return frm;
}

static Real GetFrameRate(BaseDocument *doc)
{
	Real fRate = 0.0;

	if(doc)
	{
		Real fps = Real(doc->GetFps());
		BaseTime btA = BaseTime(fps,fps);
		BaseTime btB = BaseTime(fps-1,fps);
		fRate = btA.Get() - btB.Get();
	}
	
	return fRate;
}

LONG CDSpringyKeys::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;

	if(!HasAnimation(op)) return false;
	
	// set spring constants
	Real kp = tData->GetReal(SPK_P_STIFFNESS);
	Real bp = tData->GetReal(SPK_P_DAMPING);
	
	Real ks, bs, kr, br, f = 1500.0;
	if(!tData->GetBool(SPK_SPLIT_FORCES))
	{
		tData->SetReal(SPK_S_STIFFNESS,kp);
		tData->SetReal(SPK_S_DAMPING,bp);
		tData->SetReal(SPK_S_MASS,tData->GetReal(SPK_P_MASS));
		
		tData->SetReal(SPK_R_STIFFNESS,kp);
		tData->SetReal(SPK_R_DAMPING,bp);
		tData->SetReal(SPK_R_MASS,tData->GetReal(SPK_P_MASS));
		
		ks = kp;
		bs = bp;
		kr = kp;
		br = bp;
	}
	else
	{
		ks = tData->GetReal(SPK_S_STIFFNESS);
		bs = tData->GetReal(SPK_S_DAMPING);
		
		kr = tData->GetReal(SPK_R_STIFFNESS);
		br = tData->GetReal(SPK_R_DAMPING);
	}
	rk4.SetSpringConstants(kp,bp,ks,bs,kr,br,f);
	
	Matrix trgM = op->GetMl();
	Vector opSca = CDGetScale(op);
	
	BaseTime bTime = doc->GetTime();
	Real curTime = bTime.Get();
	Real prvTime = tData->GetReal(SPK_PREV_TIME);
	
	LONG fps = doc->GetFps();
	LONG curFrm = bTime.GetFrame(fps);
	LONG prvFrm = tData->GetLong(SPK_PREV_FRAME);
	
	Bool initStart = false;
	if(CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING))
	{
		if(CDIsCommandChecked(12412) && curFrm <= GetFirstKeyFrame(op,fps)) initStart = true;
		if(CDIsCommandChecked(12411) && curFrm >= GetLastKeyFrame(op,fps)) initStart = true;
	}
	else if(curFrm <= GetFirstKeyFrame(op,fps)) initStart = true;

	if(initStart)
	{
		// initialize object state
		opState.position = Vector(0,0,0);
		opState.momentum = Vector(0,0,0);
		opState.velocity = Vector(0,0,0);
		
		opState.scale = Vector(0,0,0);
		opState.scaleMomentum = Vector(0,0,0);
		opState.scaleVelocity = Vector(0,0,0);
		
		opState.orientation.SetMatrix(Matrix());
		opState.spin.SetMatrix(Matrix());
		opState.angularMomentum = Vector(0,0,0);
		opState.angularVelocity = Vector(0,0,0);
		
		tData->SetReal(SPK_PREV_TIME,curTime);
		tData->SetMatrix(SPK_PREV_M,trgM);
		tData->SetVector(SPK_PREV_SCA,opSca);
		tData->SetLong(SPK_PREV_FRAME,curFrm);
		
		return true;
	}
	
	if(curTime == prvTime)
	{
		Bool resetPSR = false;
		if(CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING)) resetPSR = true;
		else
		{
			if(op->GetBit(BIT_ACTIVE))
			{
				BaseContainer keyState;
				GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, keyState);
				if(keyState.GetLong(BFM_INPUT_QUALIFIER) & QALT) resetPSR = true;
				else
				{
				#if API_VERSION <9800
					if(!PSRToolsActive(doc)) resetPSR = true;
				#else
					if(flags & CD_EXECUTION_ANIMATION) resetPSR = true;
					if(flags & CD_EXECUTION_INDRAG) resetPSR = true;
				#endif
				}
			}
			else resetPSR = true;
		}

		if(resetPSR)
		{
			op->SetMl(tData->GetMatrix(SPK_PREV_M));
			CDSetScale(op,tData->GetVector(SPK_PREV_SCA));
		}
		return true;
	}
	
	Real strength = tData->GetReal(SPK_STRENGTH);
	if(strength == 0.0) return true;
	
	if(prvTime != curTime)
	{
		Matrix prM = ScaleMatrix(Matrix(),opSca);
		prM.off = trgM.off;
		
		Matrix prvM = tData->GetMatrix(SPK_PREV_M);
		Matrix locM = MInv(trgM) * prvM;
		locM.off = MInv(prM) * prvM.off;
		
		Vector trgSca = opSca;
		Vector prvSca = tData->GetVector(SPK_PREV_SCA);
		Matrix scaM = prM;
		scaM.off = trgSca;
		Vector locSca = MInv(scaM) * prvSca;
		
		// build current state
		State curState = opState;
		
		curState.position = locM.off;
		curState.pMass = tData->GetReal(SPK_P_MASS);
		if(curState.pMass < 1.0) curState.pMass = 1.0;
		curState.invPMass = 1.0 / curState.pMass;
		
		curState.scale = locSca;
		curState.sMass = tData->GetReal(SPK_S_MASS);
		if(curState.sMass < 1.0) curState.sMass = 1.0;
		curState.invSMass = 1.0 / curState.sMass;
		
		curState.orientation.SetMatrix(locM);
		curState.rMass = tData->GetReal(SPK_R_MASS);
		if(curState.rMass < 1.0) curState.rMass = 1.0;
		curState.invRMass = 1.0 / curState.rMass;
		curState.inertia = 4 * curState.rMass * (1.0 / 6.0);
		if(curState.inertia < 0.01) curState.inertia = 0.01;
		curState.invInertia = 1.0 / curState.inertia;
			
		Real deltaTime, fRate = GetFrameRate(doc);
		if(prvTime < curTime) deltaTime = curTime-prvTime;
		else deltaTime = prvTime-curTime;
		
		if(deltaTime > fRate*2)
		{
			LONG frmDif = 1;
			if(prvFrm < curFrm) frmDif = curFrm - prvFrm;
			else frmDif = prvFrm - curFrm;
			
			Real tPrv, tCur, dt = 0.01;
			Real tCounter = 0.0;
			while(tCounter < (fRate * frmDif))
			{		
				tPrv = t;
				rk4.Integrate(curState, t, dt);
				t += dt;
				tCounter += dt;
			}
			prvState = curState;
			rk4.Integrate(curState, t, dt);
			
			Real mix = fRate/tCounter;
			opState = rk4.Interpolate(prvState, curState, mix);
			
			tCur = CDBlend(tPrv,t,mix);
			t = tCur;
		}
		else
		{
			rk4.Integrate(curState, prvTime, deltaTime);
			opState = curState;
		}
		
		// set op orientation
		Matrix dtRotM;
		dtRotM = opState.orientation.GetMatrix();
		dtRotM.off = opState.position;
		
		CDQuaternion opQ, trgQ, mixQ;
		opQ.SetMatrix(trgM * dtRotM);
		trgQ.SetMatrix(trgM);
			
		if(!tData->GetBool(SPK_USE_ROTATION)) mixQ = trgQ;
		else mixQ = CDQSlerp(trgQ,opQ,strength);
		Matrix transM = mixQ.GetMatrix();
		
		// set op position
		Vector opPos = prM * opState.position;
		if(!tData->GetBool(SPK_USE_POSITION)) transM.off = trgM.off;
		else transM.off = CDBlend(trgM.off,opPos,strength);
		
		// set op scale
		Vector tranSca, dtSca = scaM * opState.scale;
		if(!tData->GetBool(SPK_USE_SCALE)) tranSca = trgSca;
		else tranSca = CDBlend(trgSca,dtSca,strength);
		
		// set object transformation
		Vector oldRot, newRot, rotSet;
		oldRot = CDGetRot(op);
		op->SetMl(transM);
		newRot = CDGetRot(op);
		rotSet = CDGetOptimalAngle(oldRot, newRot, op);
		CDSetRot(op,rotSet);
		CDSetScale(op,tranSca);
		
		tData->SetReal(SPK_PREV_TIME,curTime);
		tData->SetMatrix(SPK_PREV_M,transM);
		tData->SetVector(SPK_PREV_SCA,tranSca);
		tData->SetLong(SPK_PREV_FRAME,curFrm);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSpringyKeys::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SPK_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	if(!tData->GetBool(SPK_SPLIT_FORCES))
	{
		BaseContainer subgroup0 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup0.SetLong(DESC_COLUMNS, 1);
		if(!description->SetParameter(DescLevel(SPK_ID_SPLIT, DTYPE_GROUP, 0), subgroup0, DescLevel(SPK_ID_FORCES))) return true;
		
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc1.SetString(DESC_NAME, GeLoadString(IDS_STIFFNESS));
		bc1.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc1.SetReal(DESC_MIN,0.0);
		bc1.SetReal(DESC_MAX,1.0);
		bc1.SetReal(DESC_STEP,0.01);
		bc1.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_P_STIFFNESS, DTYPE_REAL, 0), bc1, DescLevel(SPK_ID_SPLIT))) return true;

		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc2.SetString(DESC_NAME, GeLoadString(IDS_DAMPING));
		bc2.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc2.SetReal(DESC_MIN,0.0);
		bc2.SetReal(DESC_MAX,1.0);
		bc2.SetReal(DESC_STEP,0.01);
		bc2.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_P_DAMPING, DTYPE_REAL, 0), bc2, DescLevel(SPK_ID_SPLIT))) return true;

		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_MASS));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_REAL);
		bc3.SetReal(DESC_MIN,1.0);
		bc3.SetReal(DESC_MAX,100.0);
		bc3.SetReal(DESC_STEP,0.1);
		bc3.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_P_MASS, DTYPE_REAL, 0), bc3, DescLevel(SPK_ID_SPLIT))) return true;
	}
	else
	{
		BaseContainer subgroup0 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup0.SetLong(DESC_COLUMNS, 1);
		if(!description->SetParameter(DescLevel(SPK_ID_SPLIT, DTYPE_GROUP, 0), subgroup0, DescLevel(SPK_ID_FORCES))) return true;
		
		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 1);
		subgroup1.SetString(DESC_NAME, GeLoadString(IDS_POS_FORCES));
		if(!description->SetParameter(DescLevel(SPK_ID_P, DTYPE_GROUP, 0), subgroup1, DescLevel(SPK_ID_SPLIT))) return true;
		
		BaseContainer subgroup2 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup2.SetLong(DESC_COLUMNS, 1);
		subgroup2.SetString(DESC_NAME, GeLoadString(IDS_SCA_FORCES));
		if(!description->SetParameter(DescLevel(SPK_ID_S, DTYPE_GROUP, 0), subgroup2, DescLevel(SPK_ID_SPLIT))) return true;
		
		BaseContainer subgroup3 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup3.SetLong(DESC_COLUMNS, 1);
		subgroup3.SetString(DESC_NAME, GeLoadString(IDS_ROT_FORCES));
		if(!description->SetParameter(DescLevel(SPK_ID_R, DTYPE_GROUP, 0), subgroup3, DescLevel(SPK_ID_SPLIT))) return true;
		
		// position forces
		BaseContainer stbc1 = GetCustomDataTypeDefault(DTYPE_STATICTEXT);
		stbc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_STATICTEXT);
		stbc1.SetString(DESC_NAME, GeLoadString(IDS_POS_FORCES));
		if (!description->SetParameter(DescLevel(SPK_POS_STATIC, DTYPE_STATICTEXT, 0), stbc1, DescLevel(SPK_ID_P))) return true;
		
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc1.SetString(DESC_NAME, "P."+GeLoadString(IDS_STIFFNESS));
		bc1.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc1.SetReal(DESC_MIN,0.0);
		bc1.SetReal(DESC_MAX,1.0);
		bc1.SetReal(DESC_STEP,0.01);
		bc1.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_P_STIFFNESS, DTYPE_REAL, 0), bc1, DescLevel(SPK_ID_P))) return true;
		
		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc4.SetString(DESC_NAME, "P."+GeLoadString(IDS_DAMPING));
		bc4.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc4.SetReal(DESC_MIN,0.0);
		bc4.SetReal(DESC_MAX,1.0);
		bc4.SetReal(DESC_STEP,0.01);
		bc4.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_P_DAMPING, DTYPE_REAL, 0), bc4, DescLevel(SPK_ID_P))) return true;
		
		BaseContainer bc7 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc7.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc7.SetString(DESC_NAME, "P."+GeLoadString(IDS_MASS));
		bc7.SetLong(DESC_UNIT, DESC_UNIT_REAL);
		bc7.SetReal(DESC_MIN,1.0);
		bc7.SetReal(DESC_MAX,100.0);
		bc7.SetReal(DESC_STEP,0.1);
		bc7.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(SPK_P_MASS, DTYPE_REAL, 0), bc7, DescLevel(SPK_ID_P))) return true;
		
		// scale forces
		BaseContainer stbc2 = GetCustomDataTypeDefault(DTYPE_STATICTEXT);
		stbc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_STATICTEXT);
		stbc2.SetString(DESC_NAME, GeLoadString(IDS_SCA_FORCES));
		if (!description->SetParameter(DescLevel(SPK_SCA_STATIC, DTYPE_STATICTEXT, 0), stbc2, DescLevel(SPK_ID_S))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc2.SetString(DESC_NAME, "S."+GeLoadString(IDS_STIFFNESS));
		bc2.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc2.SetReal(DESC_MIN,0.0);
		bc2.SetReal(DESC_MAX,1.0);
		bc2.SetReal(DESC_STEP,0.01);
		bc2.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_S_STIFFNESS, DTYPE_REAL, 0), bc2, DescLevel(SPK_ID_S))) return true;
		
		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc5.SetString(DESC_NAME, "S."+GeLoadString(IDS_DAMPING));
		bc5.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc5.SetReal(DESC_MIN,0.0);
		bc5.SetReal(DESC_MAX,1.0);
		bc5.SetReal(DESC_STEP,0.01);
		bc5.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_S_DAMPING, DTYPE_REAL, 0), bc5, DescLevel(SPK_ID_S))) return true;
		
		BaseContainer bc8 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc8.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc8.SetString(DESC_NAME, "S."+GeLoadString(IDS_MASS));
		bc8.SetLong(DESC_UNIT, DESC_UNIT_REAL);
		bc8.SetReal(DESC_MIN,1.0);
		bc8.SetReal(DESC_MAX,100.0);
		bc8.SetReal(DESC_STEP,0.1);
		bc8.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(SPK_S_MASS, DTYPE_REAL, 0), bc8, DescLevel(SPK_ID_S))) return true;
		
		// rotation forces
		BaseContainer stbc3 = GetCustomDataTypeDefault(DTYPE_STATICTEXT);
		stbc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_STATICTEXT);
		stbc3.SetString(DESC_NAME, GeLoadString(IDS_ROT_FORCES));
		if (!description->SetParameter(DescLevel(SPK_ROT_STATIC, DTYPE_STATICTEXT, 0), stbc3, DescLevel(SPK_ID_R))) return true;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, "R."+GeLoadString(IDS_STIFFNESS));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_R_STIFFNESS, DTYPE_REAL, 0), bc3, DescLevel(SPK_ID_R))) return true;
		
		BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc6.SetString(DESC_NAME, "R."+GeLoadString(IDS_DAMPING));
		bc6.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc6.SetReal(DESC_MIN,0.0);
		bc6.SetReal(DESC_MAX,1.0);
		bc6.SetReal(DESC_STEP,0.01);
		bc6.SetReal(DESC_DEFAULT,0.5);
		if (!description->SetParameter(DescLevel(SPK_R_DAMPING, DTYPE_REAL, 0), bc6, DescLevel(SPK_ID_R))) return true;
		
		BaseContainer bc9 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc9.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc9.SetString(DESC_NAME, "R."+GeLoadString(IDS_MASS));
		bc9.SetLong(DESC_UNIT, DESC_UNIT_REAL);
		bc9.SetReal(DESC_MIN,1.0);
		bc9.SetReal(DESC_MAX,100.0);
		bc9.SetReal(DESC_STEP,0.1);
		bc9.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(SPK_R_MASS, DTYPE_REAL, 0), bc9, DescLevel(SPK_ID_R))) return true;
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSpringyKeys::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	
	if(!HasAnimation(op)) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case SPK_USE_POSITION:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_USE_SCALE:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_USE_ROTATION:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_SPLIT_FORCES:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_P_STIFFNESS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_P_DAMPING:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_P_MASS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_S_STIFFNESS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_S_DAMPING:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_S_MASS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_R_STIFFNESS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_R_DAMPING:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPK_R_MASS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDSpringyKeysTagPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSK_SERIAL_SIZE];
	String cdsknr, kb;
	
	if(!CDReadPluginInfo(ID_CDSPRINGYKEYS,data,CDSK_SERIAL_SIZE)) reg = false;
	
	cdsknr.SetCString(data,CDSK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsknr)) reg = false;
	
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	kb = GetKeyByte(cdsknr,pK,2);
	b = GetKeyByte(seed,aK,bK,cK);
	if(kb != LongToHexString(b,2)) reg = false;
	
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

	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDSPRNGKYS); if (!name.Content()) return true;
	
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;
	

	return CDRegisterTagPlugin(ID_CDSPRINGYKEYSTAG,name,info,CDSpringyKeys::Alloc,"tCDSpringyKeys","CDSprgyKeys.tif",1);
}

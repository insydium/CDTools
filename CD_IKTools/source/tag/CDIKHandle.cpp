//	Cactus Dan's IK Tools plugin
//	Copyright 2011 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDIKtools.h"
#include "tCDIKHandle.h"
#include "CDQuaternion.h"
#include "CDCompatibility.h"
#include "CDArray.h"

#define MAX_BONES		100
#define MAX_TRIES		1000
#define THRESHOLD		0.01
#define STEP			0.01

enum
{	
	//IKH_PURCHASE					= 1000,
	
	//IKH_USE						= 1010,
	//IKH_BLEND					= 1011,
	
	//IKH_SHOW_LINES				= 1012,
	
	//IKH_LINE_TARGET				= 1013,
		//IKH_LINE_ROOT			= 1014,
		//IKH_LINE_TIP				= 1015,
	
	//IKH_LINE_COLOR					= 1016,
	
	//IKH_POLE_AXIS				= 1017,
		//IKH_POLE_Y				= 1018,
		//IKH_POLE_X				= 1019,
		//IKH_POLE_NX				= 1020,
		//IKH_POLE_NY				= 1021,
	
	//IKH_BONES_IN_GROUP				= 1022,
	//IKH_CONNECT_BONES				= 1023,
	//IKH_CONNECT_NEXT				= 1024,
	//IKH_TWIST						= 1026,
	
	//IKH_HANDLE_GROUP				= 2000,
	
	//IKH_SOLVER					= 2001,
		//IKH_IKRP					= 2002,
		//IKH_IKSC					= 2003,
		//IKH_IKHD					= 2004,
	
	//IKH_UNLOCK_ROOT				= 2005,
	
	//IKH_POLE_LINK				= 2010,
	//IKH_GOAL_LINK				= 2011,
	
	//IKH_IKSC_POLE_VECTOR			= 2014,
	
	//IKH_POLE_GROUP				= 2015
	
	
	IKH_PREFERED_ANGLE_SET			= 10000,
	IKH_GOAL_VECTOR_LENGTH			= 10001,
	IKH_TOTAL_LENGTH				= 10002,
	IKH_GOAL_START_POSITION			= 10003,
	
	IKH_PREV_POLE_AXIS				= 10010,
	
	IKH_REST_ANGLE					= 20000,
	
	IKH_IK_POSITION					= 30000
};

struct CDJacobian
{
	Real px;
	Real py;
	Real pz;
	Real ox;
	Real oy;
	Real oz;
};

struct CDIKJoint
{
	Matrix		m;
	Vector		gPos;
	Vector		pos;
	Vector		sca;
	Vector		rot;
	Real		len;
	BaseObject	*jnt;
};

struct State
{
	Real x; // position
	Real v; // velocity
};

struct Derivative
{
	Real dx; // derivative of position: velocity
	Real dv; // derivative of velocity: Acceleration
};

Real Acceleration(const State &state, Real t)
{
	const Real k = 10;
	const Real b = 1;
	return -k * state.x - b * state.v;
}

Derivative Evaluate(const State &initial, Real t)
{
	Derivative output;
	output.dx = initial.v;
	output.dv = Acceleration(initial, t);
	return output;
}

Derivative Evaluate(const State &initial, Real t, Real dt, const Derivative &d)
{
	State state;
	state.x = initial.x + d.dx*dt;
	state.v = initial.v + d.dv*dt;
	Derivative output;
	output.dx = state.v;
	output.dv = Acceleration(state, t+dt);
	return output;
}

void Integrate(State &state, Real t, Real dt)
{
	Derivative a = Evaluate(state, t);
	Derivative b = Evaluate(state, t, dt * 0.5, a);
	Derivative c = Evaluate(state, t, dt * 0.5, b);
	Derivative d = Evaluate(state, t, dt, c);

	const Real dxdt = 1.0/6.0 * (a.dx + 2.0 * (b.dx + c.dx) + d.dx);
	const Real dvdt = 1.0/6.0 * (a.dv + 2.0 * (b.dv + c.dv) + d.dv);
	
	state.x = state.x + dxdt * dt;
	state.v = state.v + dvdt * dt;
}

class CDIKHandlePlugin : public CDTagData
{
private:
	CDArray<CDIKJoint>		Jnt;
	CDArray<CDJacobian>		J;
	CDArray<Real>			q, K, rAngle;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	void ConfirmJointCount(BaseObject *jnt, BaseContainer *tData);
	Real GetTotalJointLength(BaseContainer *tData, BaseObject *jnt, LONG jCnt);
	BaseObject* GetEffector(BaseObject *jnt, LONG jCnt);
	
	void InitStorageArrays(BaseContainer *tData);
	void FreeStorageArrays(void);
	Bool StorageAllocated(void);
	
	Matrix GetRPMatrix(BaseContainer *tData, Vector o, Vector goalV, Vector poleV, LONG pAxis);
	Vector GetRPNorm(BaseContainer *tData, Matrix rpM);
	Vector GetRAxis(BaseContainer *tData);
	
	Matrix AlignJointMatrixToRP(BaseContainer *tData, Matrix jntM, Matrix rpM);
	void RealignJoints(BaseObject *jnt, BaseContainer *tData, Matrix rpM);
	
	void SetStraightChain(BaseDocument *doc, BaseContainer *tData, BaseObject *jnt, Matrix rpM);
	void SetPreferedAngle(BaseContainer *tData, BaseObject *jnt, Matrix rpM, Vector goalV);
	void RestoreRestState(BaseContainer *tData, BaseObject *jnt, Matrix rpM, Vector goalV, LONG pAxis);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode* node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDIKHandlePlugin); }
};

Bool CDIKHandlePlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(IKH_USE,true);
	tData->SetReal(IKH_BLEND,1.0);
	
	tData->SetBool(IKH_SHOW_LINES,true);
	tData->SetVector(IKH_LINE_COLOR,Vector(1,0,0));
	
	tData->SetLong(IKH_POLE_AXIS,IKH_POLE_Y);
	tData->SetLong(IKH_LINE_TARGET,IKH_LINE_ROOT);
	tData->SetReal(IKH_TWIST,0.0);
	
	tData->SetLong(IKH_BONES_IN_GROUP,1);
	tData->SetBool(IKH_CONNECT_BONES,false);
	tData->SetBool(IKH_CONNECT_NEXT,false);
	
	tData->SetLong(IKH_SOLVER,IKH_IKRP);
	
	tData->SetVector(IKH_IKSC_POLE_VECTOR,Vector(1.0,0.0,0.0));
	
	tData->SetBool(IKH_PREFERED_ANGLE_SET,false);

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

void CDIKHandlePlugin::Free(GeListNode* node)
{
	FreeStorageArrays();
}

void CDIKHandlePlugin::InitStorageArrays(BaseContainer *tData)
{
	FreeStorageArrays();
	
	LONG jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
	
	Jnt.Alloc(jCnt+1);
	J.Alloc(jCnt);
	K.Alloc(jCnt);
	q.Alloc(jCnt);
	q.Fill(0);
	
	rAngle.Alloc(jCnt);
}

void CDIKHandlePlugin::FreeStorageArrays(void)
{
	Jnt.Free();
	J.Free();
	K.Free();
	q.Free();
	rAngle.Free();
}

Bool CDIKHandlePlugin::StorageAllocated(void)
{
	if(Jnt.IsEmpty()) return false;
	if(J.IsEmpty()) return false;
	if(K.IsEmpty()) return false;
	if(q.IsEmpty()) return false;
	if(rAngle.IsEmpty()) return false;
	
	return true;
}

Bool CDIKHandlePlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag  = (BaseTag*)snode; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
	
	if (jCnt > 0)
	{
		Jnt.Copy(((CDIKHandlePlugin*)dest)->Jnt);
		J.Copy(((CDIKHandlePlugin*)dest)->J);
		K.Copy(((CDIKHandlePlugin*)dest)->K);
		q.Copy(((CDIKHandlePlugin*)dest)->q);
		rAngle.Copy(((CDIKHandlePlugin*)dest)->rAngle);
	}
	
	return true;
}

void CDIKHandlePlugin::ConfirmJointCount(BaseObject *jnt, BaseContainer *tData)
{
	LONG jCnt = 0;
	
	if(jnt && tData)
	{
		while(jnt &&  jCnt < tData->GetLong(IKH_BONES_IN_GROUP))
		{
			jnt = jnt->GetDown();
			if(jnt) jCnt++;
		}
		
		if(jCnt != tData->GetLong(IKH_BONES_IN_GROUP))
		{
			if(jCnt > MAX_BONES) jCnt = MAX_BONES;
			
			FreeStorageArrays();
			InitStorageArrays(tData);
			
			tData->SetLong(IKH_BONES_IN_GROUP, jCnt);
		}
	}
}

Bool CDIKHandlePlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *theChild = op->GetDown(); if(!theChild) return true;
	
	BaseObject *goal = tData->GetObjectLink(IKH_GOAL_LINK,doc); if(!goal) return true;
	
	Matrix opM = op->GetMg(), goalM = goal->GetMg();
	Vector childPosition = theChild->GetMg().off;
	
	if(!tData->GetBool(IKH_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Vector lnClr = tData->GetVector(IKH_LINE_COLOR);
	bd->SetPen(lnClr);
	CDDrawLine(bd, opM.off, goalM.off);
	
	BaseObject *joint = op;
	theChild = joint->GetDown();
	LONG i, jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
 	for(i=0; i<jCnt; i++)
 	{
		if(theChild)
		{
			CDDrawLine(bd,joint->GetMg().off, theChild->GetMg().off);
			joint = theChild;
			theChild = theChild->GetDown();
		}
 	}
	
	BaseObject *pole = tData->GetObjectLink(IKH_POLE_LINK,doc);
	if(tData->GetLong(IKH_SOLVER) == IKH_IKRP)
	{
		Vector poleV, goalV = goalM.off - opM.off;
		if(pole)
		{
			Matrix poleM = pole->GetMg();
			poleV = poleM.off - opM.off;
			
			switch (tData->GetLong(IKH_LINE_TARGET))
			{
				case IKH_LINE_ROOT:	
					CDDrawLine(bd,opM.off, poleM.off);
					break;
				case IKH_LINE_TIP:
					CDDrawLine(bd,poleM.off, childPosition);
					break;
				default:
				break;
			}
		}
		else poleV = tData->GetVector(IKH_IKSC_POLE_VECTOR);
	}
	
	if(tData->GetReal(IKH_BLEND) < 1.0)
	{
		Vector posB, posA = tData->GetVector(IKH_IK_POSITION);
		joint = op;
		
		ObjectColorProperties prop;
		joint->GetColorProperties(&prop);
		if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
		else bd->SetPen(Vector(0,0,0.5));
		
		for(i=0; i<jCnt; i++)
		{
			posB = tData->GetVector(IKH_IK_POSITION+1+i);
			CDDrawLine(bd,posA,posB);
			posA = posB;
			
			joint = joint->GetDown();
			if(joint)
			{
				joint->GetColorProperties(&prop);
				if(prop.usecolor > 0)  bd->SetPen(prop.color*0.5);
				else bd->SetPen(Vector(0,0,0.5));
			}
		}
	}
	
	return true;
}

void CDIKHandlePlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(IKH_POLE_AXIS+T_LST,tData->GetLong(IKH_POLE_AXIS));
			tData->SetLong(IKH_SOLVER+T_LST,tData->GetLong(IKH_SOLVER));
			
			tData->SetLink(IKH_GOAL_LINK+T_LST,tData->GetLink(IKH_GOAL_LINK,doc));
			tData->SetLink(IKH_POLE_LINK+T_LST,tData->GetLink(IKH_POLE_LINK,doc));
			
			LONG i, jcnt = tData->GetReal(IKH_BONES_IN_GROUP);
			BaseObject *ch = op->GetDown();
			for(i=0; i<jcnt; i++)
			{
				if(ch)
				{
					tData->SetVector(T_CP+i, CDGetPos(ch));
					ch = ch->GetDown();
				}
			}
			tData->SetReal(IKH_BONES_IN_GROUP+T_LST,jcnt);
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDIKHandlePlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(IKH_POLE_AXIS,tData->GetLong(IKH_POLE_AXIS+T_LST));
		
		LONG i, jcnt = tData->GetReal(IKH_BONES_IN_GROUP+T_LST);
		tData->SetReal(IKH_BONES_IN_GROUP,jcnt);
		
		BaseObject *ch = op->GetDown();
		for(i=0; i<jcnt; i++)
		{
			if(ch)
			{
				CDSetPos(ch,tData->GetVector(T_CP+i));
				ch = ch->GetDown();
			}
		}
		
		tData->SetLong(IKH_POLE_AXIS,tData->GetLong(IKH_POLE_AXIS+T_LST));
		tData->SetLong(IKH_SOLVER,tData->GetLong(IKH_SOLVER+T_LST));
		
		tData->SetLink(IKH_GOAL_LINK,tData->GetLink(IKH_GOAL_LINK+T_LST,doc));
		tData->SetLink(IKH_POLE_LINK,tData->GetLink(IKH_POLE_LINK+T_LST,doc));
		
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDIKHandlePlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	BaseObject *op = tag->GetObject();
	
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
	
	ConfirmJointCount(op, tData);
	
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			InitStorageArrays(tData);
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == IKH_POLE_AXIS)
			{
				Matrix goalM, poleM, opM = op->GetMg();
				BaseObject *goal = tData->GetObjectLink(IKH_POLE_LINK,doc);
				if(goal)
				{
					tData->SetBool(IKH_PREFERED_ANGLE_SET,false);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==IKH_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

Matrix CDIKHandlePlugin::GetRPMatrix(BaseContainer *tData, Vector o, Vector goalV, Vector poleV, LONG pAxis)
{
	Matrix m;

	m.off = o;
	switch(pAxis)
	{
		case IKH_POLE_X:	
			m.v2 = VNorm(VCross(goalV, poleV));
			m.v1 = VNorm(VCross(m.v2, goalV));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case IKH_POLE_Y:
			m.v1 = VNorm(VCross(poleV, goalV));
			m.v2 = VNorm(VCross(goalV, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case IKH_POLE_NX:	
			m.v2 = VNorm(VCross(poleV, goalV));
			m.v1 = VNorm(VCross(m.v2, goalV));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case IKH_POLE_NY:
			m.v1 = VNorm(VCross(goalV, poleV));
			m.v2 = VNorm(VCross(goalV, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		default:
		break;
	}

	return m;	
}

Real CDIKHandlePlugin::GetTotalJointLength(BaseContainer *tData, BaseObject *jnt, LONG jCnt)
{
	Real tLen = 0;
	
	LONG i;
	for(i=0; i<jCnt; i++)
	{
		Jnt[i].jnt = jnt;
		
		Matrix jM = jnt->GetMg();
		Vector a = jM.off;
		
		Vector jPos = CDGetPos(jnt);
		if(i>0 && tData->GetBool(IKH_CONNECT_BONES))
		{
			jPos.x = 0.0;
			jPos.y = 0.0;
			CDSetPos(jnt,jPos);
		}
		
		Jnt[i].m = jM;
		Jnt[i].gPos = a;
		Jnt[i].sca = CDGetScale(jnt);
		Jnt[i].pos = jPos;
		Jnt[i].rot = CDGetOptimalAngle(Vector(0,0,0),CDGetRot(jnt), jnt);
		Jnt[i].len = 0.0;
		
		jnt = jnt->GetDown();
		if(jnt)
		{
			Vector b = jnt->GetMg().off;
			Real l = Len(b-a);
			Jnt[i].len = l;
			tLen += l;
			if(i+1 == jCnt)
			{
				jPos = CDGetPos(jnt);
				if(tData->GetBool(IKH_CONNECT_NEXT))
				{
					jPos.x = 0.0;
					jPos.y = 0.0;
					CDSetPos(jnt,jPos);
				}
				
				Jnt[jCnt].m = jnt->GetMg();
				Jnt[jCnt].gPos = b;
				Jnt[jCnt].pos = jPos;
				Jnt[jCnt].sca = CDGetScale(jnt);
			}
		}
	}
	
	return tLen;
}

Vector CDIKHandlePlugin::GetRPNorm(BaseContainer *tData, Matrix rpM)
{
	Vector rpNorm;
	switch (tData->GetLong(IKH_POLE_AXIS))
	{
		case IKH_POLE_X:
			rpNorm = VNorm(rpM.v2);	
			break;
		case IKH_POLE_Y:
			rpNorm = VNorm(rpM.v1);	
			break;
		case IKH_POLE_NX:	
			rpNorm = VNorm(rpM.v2);	
			break;
		case IKH_POLE_NY:
			rpNorm = VNorm(rpM.v1);	
			break;
	}
	return rpNorm;	
}

Matrix CDIKHandlePlugin::AlignJointMatrixToRP(BaseContainer *tData, Matrix jntM, Matrix rpM)
{
	Matrix transM;
	
	transM.off = jntM.off;
	transM.v3 = VNorm(jntM.v3);
	
	switch (tData->GetLong(IKH_POLE_AXIS))
	{
		case IKH_POLE_X:	
			transM.v2 = VNorm(rpM.v2);
			transM.v1 = VNorm(VCross(transM.v2, transM.v3));
			transM.v2 = VNorm(VCross(transM.v3, transM.v1));
			break;
		case IKH_POLE_Y:
			transM.v1 = VNorm(rpM.v1);
			transM.v2 = VNorm(VCross(transM.v3, transM.v1));
			transM.v1 = VNorm(VCross(transM.v2, transM.v3));
			break;
		case IKH_POLE_NX:	
			transM.v2 = VNorm(rpM.v2);
			transM.v1 = VNorm(VCross(transM.v2, transM.v3));
			transM.v2 = VNorm(VCross(transM.v3, transM.v1));
			break;
		case IKH_POLE_NY:
			transM.v1 = VNorm(rpM.v1);
			transM.v2 = VNorm(VCross(transM.v3, transM.v1));
			transM.v1 = VNorm(VCross(transM.v2, transM.v3));
			break;
	}
	
	return transM;
}

void CDIKHandlePlugin::SetPreferedAngle(BaseContainer *tData, BaseObject *op, Matrix rpM, Vector goalV)
{
	tData->SetVector(IKH_GOAL_START_POSITION,goalV);
	BaseObject *jnt = op;
	LONG i, jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
	for(i=0; i<jCnt; i++)
	{
		if(jnt)
		{
			Jnt[i].m = AlignJointMatrixToRP(tData,jnt->GetMg(),rpM);
			jnt = jnt->GetDown();
		}
	}
	Jnt[jCnt].m = jnt->GetMg();
	
	jnt = op;
	jnt->SetMg(Jnt[0].m);
	Matrix jntM = jnt->GetMg();
	
	Vector rpNorm = GetRPNorm(tData,rpM);
	
	Real angle = ACos(VDot(VNorm(goalV), VNorm(jntM.v3)));
	if(VDot(VNorm(VCross(jntM.v3, goalV)), rpNorm) < 0) angle *= -1;
	
	tData->SetReal(IKH_REST_ANGLE,angle);
	tData->SetReal(IKH_GOAL_VECTOR_LENGTH,Len(goalV));
	
	jnt = jnt->GetDown();
	for(i=1; i<jCnt; i++)
	{
		if(jnt)
		{
			jnt->SetMg(Jnt[i].m);
			tData->SetVector(IKH_REST_ANGLE+i,CDGetOptimalAngle(Vector(0,0,0),CDGetRot(jnt), jnt));
		}
		jnt = jnt->GetDown();
	}
	jnt->SetMg(Jnt[jCnt].m);
	
	tData->SetBool(IKH_PREFERED_ANGLE_SET,true);
	tData->SetLong(IKH_PREV_POLE_AXIS,tData->GetLong(IKH_POLE_AXIS));
}

void CDIKHandlePlugin::RestoreRestState(BaseContainer *tData, BaseObject *jnt, Matrix rpM, Vector goalV, LONG pAxis)
{
	Vector axis, scale = CDGetScale(jnt);
	Real angle = tData->GetReal(IKH_REST_ANGLE);
	
	switch(pAxis)
	{
		case IKH_POLE_X:
			axis = Vector(0,1,0);	
			break;
		case IKH_POLE_Y:
			axis = Vector(1,0,0);	
			break;
		case IKH_POLE_NX:	
			axis = Vector(0,1,0);	
			break;
		case IKH_POLE_NY:
			axis = Vector(1,0,0);	
			break;
	}
	
	Matrix transM = rpM * RotAxisToMatrix(axis,angle);
	jnt->SetMg(transM);
	CDSetScale(jnt,scale);
	
	
	LONG i, jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
	for (i=1; i<jCnt; i++)
	{
		jnt = jnt->GetDown();
		if(jnt)
		{
			scale = CDGetScale(jnt);
			Vector rot = tData->GetVector(IKH_REST_ANGLE+i);
			CDSetRot(jnt,rot);
			CDSetScale(jnt,scale);
		}
	}
}

BaseObject* CDIKHandlePlugin::GetEffector(BaseObject *jnt, LONG jCnt)
{
	LONG i;
	for(i=0; i<jCnt; i++)
	{
		if(jnt) jnt = jnt->GetDown();
	}
	
	return jnt;
}

void CDIKHandlePlugin::SetStraightChain(BaseDocument *doc, BaseContainer *tData, BaseObject *jnt, Matrix rpM)
{
	Vector scale = CDGetScale(jnt);
	if(tData->GetLong(IKH_SOLVER) != IKH_IKHD) jnt->SetMg(rpM);
	else
	{
		BaseObject *goal = tData->GetObjectLink(IKH_GOAL_LINK,doc);
		if(goal)
		{
			Matrix jntM = jnt->GetMg(), glM = goal->GetMg(), transM;
			
			transM.off = jntM.off;
			transM.v3 = VNorm(glM.off - jntM.off);
			transM.v2 = VNorm(tData->GetVector(IKH_IKSC_POLE_VECTOR));
			transM.v1 = VNorm(VCross(transM.v2, transM.v3));
			transM.v2 = VNorm(VCross(transM.v3, transM.v1));
			
			jnt->SetMg(transM);
		}
	}
	CDSetScale(jnt,scale);
	
	LONG i, jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
	for (i=1; i<jCnt; i++)
	{
		jnt = jnt->GetDown();
		if(jnt)
		{
			scale = CDGetScale(jnt);
			CDSetRot(jnt,Vector(0,0,0));
			CDSetScale(jnt,scale);
		}
	}
}

Vector CDIKHandlePlugin::GetRAxis(BaseContainer *tData)
{
	Vector rAxis = Vector(1,0,0);
	
	switch (tData->GetLong(IKH_POLE_AXIS))
	{
		case IKH_POLE_X:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			break;
		case IKH_POLE_Y:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
		case IKH_POLE_NX:	
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			break;
		case IKH_POLE_NY:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			break;
	}
	
	return rAxis;
}

void CDIKHandlePlugin::RealignJoints(BaseObject *jnt, BaseContainer *tData, Matrix rpM)
{
	Matrix transM;
	
	LONG i, jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
	for(i=0; i<jCnt; i++)
	{
		if(jnt)
		{
			transM.off = Jnt[i].gPos;
			transM.v3 = VNorm(Jnt[i+1].gPos - Jnt[i].gPos);
			if(tData->GetLong(IKH_SOLVER) != IKH_IKHD)
			{
				switch(tData->GetLong(IKH_POLE_AXIS))
				{
					case IKH_POLE_X:	
						transM.v2 = VNorm(rpM.v2);
						transM.v1 = VNorm(VCross(transM.v2, transM.v3));
						transM.v3 = VNorm(VCross(transM.v1, transM.v2));
						break;
					case IKH_POLE_Y:
						transM.v1 = VNorm(rpM.v1);
						transM.v2 = VNorm(VCross(transM.v3, transM.v1));
						transM.v3 = VNorm(VCross(transM.v1, transM.v2));
						break;
					case IKH_POLE_NX:	
						transM.v2 = VNorm(rpM.v2);
						transM.v1 = VNorm(VCross(transM.v2, transM.v3));
						transM.v3 = VNorm(VCross(transM.v1, transM.v2));
						break;
					case IKH_POLE_NY:
						transM.v1 = VNorm(rpM.v1);
						transM.v2 = VNorm(VCross(transM.v3, transM.v1));
						transM.v3 = VNorm(VCross(transM.v1, transM.v2));
						break;
				}
			}
			else
			{
				transM.v2 = VNorm(tData->GetVector(IKH_IKSC_POLE_VECTOR));
				transM.v1 = VNorm(VCross(transM.v2, transM.v3));
				transM.v2 = VNorm(VCross(transM.v3, transM.v1));
			}
			jnt->SetMg(transM);
			
			CDSetScale(jnt,Jnt[i].sca);
			if(i>0) CDSetPos(jnt,Jnt[i].pos);
			
			jnt = jnt->GetDown();
		}
	}
}

LONG CDIKHandlePlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	//GePrint("CDIKHandlePlugin::Execute");
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return true;
	if(!CheckTagAssign(doc,op,tData)) return false;

	Real ikMix = tData->GetReal(IKH_BLEND);
	
	BaseObject *pole, *goal = tData->GetObjectLink(IKH_GOAL_LINK,doc); if(!goal) return true;
	Vector opScale = CDGetScale(op);
	
	Matrix jntM, rotM, transM, targM, opRestMatrix, rpM, twistM, effM;
	Matrix opM = op->GetMg(), goalM = goal->GetMg(), poleM;
	Matrix opRM = opM;
	
	if(tData->GetLong(IKH_SOLVER) == IKH_IKRP)
	{
		pole = tData->GetObjectLink(IKH_POLE_LINK,doc);
	
		// Check the global positions of the object, goal and pole.
		if(pole)
		{
			poleM = pole->GetMg();
			if(poleM.off == opM.off) return true;
			if(goalM.off == poleM.off) return true;
		}
		if(goalM.off == opM.off) return true;
	}
	
	if(tData->GetLong(IKH_SOLVER) != IKH_IKHD) tData->SetBool(IKH_UNLOCK_ROOT,false);
	
	if(!StorageAllocated()) InitStorageArrays(tData);
	
	LONG pAxis = tData->GetLong(IKH_POLE_AXIS);
	
	LONG i, jCnt = tData->GetLong(IKH_BONES_IN_GROUP);
	BaseObject *jnt = op;
	
	Real totalLen = GetTotalJointLength(tData,op,jCnt);
	tData->SetReal(IKH_TOTAL_LENGTH,totalLen);
	
	//	Set the vectors to the Pole and Goal.
	Vector poleV, goalV = goalM.off - opM.off;
	
	Real gLen = Len(goalV);
	Vector rAxis, rVector;

	switch(tData->GetLong(IKH_SOLVER))
	{
		case IKH_IKRP:
			if(pole) poleV = poleM.off - opM.off;
			else poleV = tData->GetVector(IKH_IKSC_POLE_VECTOR);
			break;
		case IKH_IKSC:
			poleV = (goalM * tData->GetVector(IKH_IKSC_POLE_VECTOR)) - goalM.off;
			break;
		case IKH_IKHD:
			poleV = tData->GetVector(IKH_IKSC_POLE_VECTOR);
			break;
	}
	
	//	Set the Rotation Plane matrix vectors.
	rpM = GetRPMatrix(tData,opM.off,goalV,poleV,pAxis);
	
	if(!tData->GetBool(IKH_PREFERED_ANGLE_SET)) SetPreferedAngle(tData,op,rpM,goalV);
	
	// invert joint chain twist
	if(tData->GetLong(IKH_SOLVER) != IKH_IKHD)
	{
		
		Real twist = tData->GetReal(IKH_TWIST);
		twistM = rpM * RotAxisToMatrix(Vector(0,0,1),twist);
		transM = rpM * (MInv(twistM) * opM);
		
		switch (pAxis)
		{
			case IKH_POLE_X:	
				transM.v2 = VNorm(rpM.v2);
				transM.v1 = VNorm(VCross(transM.v2, transM.v3));
				transM.v3 = VNorm(VCross(transM.v1, transM.v2));
				break;
			case IKH_POLE_Y:
				transM.v1 = VNorm(rpM.v1);
				transM.v2 = VNorm(VCross(transM.v3, transM.v1));
				transM.v3 = VNorm(VCross(transM.v1, transM.v2));
				break;
			case IKH_POLE_NX:	
				transM.v2 = VNorm(rpM.v2);
				transM.v1 = VNorm(VCross(transM.v2, transM.v3));
				transM.v3 = VNorm(VCross(transM.v1, transM.v2));
				break;
			case IKH_POLE_NY:
				transM.v1 = VNorm(rpM.v1);
				transM.v2 = VNorm(VCross(transM.v3, transM.v1));
				transM.v3 = VNorm(VCross(transM.v1, transM.v2));
				break;
		}
		op->SetMg(transM);
		CDSetScale(op,opScale);
		
		opM = op->GetMg();
	}
	
	BaseObject *eff = GetEffector(op,jCnt);
	if(!eff) return true;
	
	if(!tData->GetBool(IKH_USE)) return true;
	
	Real trgLen = Len(eff->GetMg().off - goalM.off);
	
	if(trgLen > THRESHOLD)
	{
		// clear values in velocity array
		q.Fill(0);
		
		if(gLen > totalLen+1 && !tData->GetBool(IKH_UNLOCK_ROOT)) SetStraightChain(doc,tData,op,rpM);
		else
		{
			LONG j = 0;
			if(tData->GetLong(IKH_SOLVER) != IKH_IKHD)
			{
				RestoreRestState(tData,op,rpM,goalV,pAxis);
				Vector rpNorm = GetRPNorm(tData,rpM);
				rAxis = GetRAxis(tData);
				
				Real toTipLen = 0.0;					
				for(i=0; i<jCnt; i++)
				{
					// store joint matrix
					Jnt[i].m = Jnt[i].jnt->GetMg();
					
					// calculate scaling
					Real jLen = Jnt[i].len;
					toTipLen += jLen;
					Real wt = toTipLen/totalLen;
					K[i] = (1/(wt * totalLen)) * (1+(1-wt));
				}
				
				State state;
				Real t = 0.0;
				Real dt = STEP * 2;//
				do
				{			
					if(jCnt < 3) break;
					
					Vector axis;
					
					Vector p = eff->GetMg().off; 
					Vector force = p - goalM.off;
					trgLen = Len(force);
					
					if(trgLen > THRESHOLD)
					{
						jnt = op;
						for(i=0; i<jCnt; i++)
						{
							jnt = Jnt[i].jnt;
							if(jnt)
							{
								// Build Jacobian
								Vector jntPos = Jnt[i].m.off; // Position of current joint
								Vector jToGoal = goalM.off - jntPos; // Vector current joint to Goal
								Vector jToEnd = p - jntPos;
								
								axis = VNorm(VCross(jToGoal, jToEnd));
								Vector cross = VNorm(VCross(jToEnd, axis));
								
								J[i].px = cross.x;
								J[i].py = cross.y;
								J[i].pz = cross.z;
								J[i].ox = axis.x;
								J[i].oy = axis.y;
								J[i].oz = axis.z;
								
								// compute q'
								q[i] = 0.0;
								q[i] += J[i].px * force.x * K[i];
								q[i] += J[i].py * force.y * K[i];
								q[i] += J[i].pz * force.z * K[i];
								
								state.x = 0.0;
								state.v = q[i];
								Integrate(state, t, dt);
								
								rAngle[i] = state.x;
							}
						}
						t += dt;
						
						jnt = op;
						for(i=0; i<jCnt; i++)
						{
							jnt = Jnt[i].jnt;
							if(jnt)
							{
								axis.x = J[i].ox;
								axis.y = J[i].oy;
								axis.z = J[i].oz;
								Real adot = VDot(rpNorm, VNorm(axis));
								
								if(adot > 0.0) rotM = RotAxisToMatrix(rAxis, -rAngle[i]);
								else rotM = RotAxisToMatrix(rAxis, rAngle[i]);
								targM = jnt->GetMg() * rotM;
								
								jnt->SetMg(targM);
								CDSetPos(jnt,Jnt[i].pos);
								CDSetScale(jnt,Jnt[i].sca);
								Jnt[i].m = jnt->GetMg();
							}
						}
						Jnt[jCnt].m.off = Jnt[jCnt-1].m * Jnt[jCnt].pos;
					}
				}
				while (j++ < MAX_TRIES * 0.2 && trgLen > THRESHOLD);
			}
			else
			{
				if(tData->GetBool(IKH_RESTORE_REST))
				{
					tData->SetBool(IKH_UNLOCK_ROOT, false);
					Matrix r = GetRPMatrix(tData,opM.off,tData->GetVector(IKH_GOAL_START_POSITION),poleV,pAxis);
					RestoreRestState(tData,op,r,goalV,pAxis);
				}
			}
			
			LONG iTries = 0;
			if(jCnt == 1) SetStraightChain(doc,tData,op,rpM);
			else
			{
				Jnt[0].gPos = opM.off;
				while(iTries < MAX_TRIES * 0.8 && trgLen > THRESHOLD)
				{
					eff = GetEffector(op,jCnt);
					jnt = eff->GetUp();
					
					Vector p1 = goalM.off, p2;
					for(i=0; i<jCnt; i++)
					{
						Jnt[jCnt-i].gPos = p1;
						effM = eff->GetMg();
						jntM = jnt->GetMg();
						p2 = p1 + VNorm(jntM.off - p1) * (Len(jntM.off - effM.off));
						p1 = p2;
						eff = jnt;
						jnt = eff->GetUp();
					}
					Jnt[0].gPos = p1;
					
					RealignJoints(op,tData,rpM);
					
					if(!tData->GetBool(IKH_UNLOCK_ROOT))
					{
						p1 = opM.off;
						jnt = op;
						eff = jnt->GetDown();
						for(i=0; i<jCnt; i++)
						{
							Jnt[i].gPos = p1;
							effM = eff->GetMg();
							jntM = jnt->GetMg();
							p2 = p1 + VNorm(effM.off - p1) * (Len(effM.off - jntM.off));
							p1 = p2;
							jnt = eff;
							eff = jnt->GetDown();
						}
						
						RealignJoints(op,tData,rpM);
					}
					
					eff = GetEffector(op,jCnt);
					effM = eff->GetMg();
					trgLen = Len(effM.off - goalM.off);
					iTries++;
				}
			}
		}
	}
	
	// Reset joint chain to the twist
	if(tData->GetLong(IKH_SOLVER) != IKH_IKHD)
	{
		transM = twistM * (MInv(rpM) * op->GetMg());
		op->SetMg(transM);
		CDSetScale(op,opScale);
	}
	
	// Store IK joint positions for drawing
	jnt = op;
	tData->SetVector(IKH_IK_POSITION,jnt->GetMg().off);
	for(i=0; i<jCnt; i++)
	{
		jnt = jnt->GetDown();
		if(jnt)
			tData->SetVector(IKH_IK_POSITION+1+i,jnt->GetMg().off);
	}
	
	if(ikMix < 1.0)
	{
		jnt = op;
		CDQuaternion fkQ, ikQ, mixQ, quat25, quat50, quat75;
		for(i=0; i<jCnt; i++)
		{
			Matrix setM, jntM = jnt->GetMg();
			if(i > 0)
			{
				fkQ.SetHPB(Jnt[i].rot);
				ikQ.SetHPB(CDGetRot(jnt));
				quat50 = !(ikQ + fkQ); // calculate the 50 quat
				quat25 = !(quat50 + fkQ); // calculate the 25 quat
				quat75 = !(quat50 + ikQ); // calculate the 75 quat
				
				// interpolate 
				mixQ = CDQSlerpBezier(fkQ,quat25,quat50,quat75,ikQ,ikMix);
				setM = mixQ.GetMatrix();
				
				setM = mixQ.GetMatrix();
				jnt->SetMl(setM);
				CDSetPos(jnt,Jnt[i].pos);
			}
			else
			{
				fkQ.SetMatrix(opRM);
				ikQ.SetMatrix(jntM);
				mixQ = CDQSlerp(fkQ,ikQ,ikMix);
				
				setM = mixQ.GetMatrix();
				setM.off = jntM.off;
				jnt->SetMg(setM);
			}

			Vector jntRot = CDGetOptimalAngle(Jnt[i].rot,CDGetRot(jnt), jnt);
			CDSetRot(jnt,jntRot);
			CDSetScale(jnt,Jnt[i].sca);
			
			jnt = jnt->GetDown();
		}
	}
		
	op->Message(MSG_UPDATE);

	return CD_EXECUTION_RESULT_OK;
}

Bool CDIKHandlePlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(IKH_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDIKHandlePlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case IKH_LINE_TARGET:	
			return tData->GetBool(IKH_SHOW_LINES);
		case IKH_UNLOCK_ROOT:
			if(tData->GetLong(IKH_SOLVER) == IKH_IKHD && !tData->GetBool(IKH_RESTORE_REST)) return true;
			else return false;
		case IKH_RESTORE_REST:
			if(tData->GetLong(IKH_SOLVER) == IKH_IKHD && !tData->GetBool(IKH_UNLOCK_ROOT)) return true;
			else return false;
		case IKH_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case IKH_TWIST:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetLong(IKH_SOLVER) == IKH_IKHD) return false;
				else return true;
			}
		case IKH_GOAL_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case IKH_POLE_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetLong(IKH_SOLVER) == IKH_IKSC) return false;
				else return true;
			}
		case IKH_IKSC_POLE_VECTOR:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	
	return true;
}

Bool RegisterCDIKHandlePlugin(void)
{
	if(CDFindPlugin(ID_CDIKHANDLEPLUGIN,CD_TAG_PLUGIN)) return true;
	
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
	
	
	// decide by name ifthe plugin shall be registered - just for user convenience    
	String name = GeLoadString(IDS_CDIKHNDL); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDIKHANDLEPLUGIN,name,TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE,CDIKHandlePlugin::Alloc,"tCDIKHandle","CDIKHandle.tif",0);
}

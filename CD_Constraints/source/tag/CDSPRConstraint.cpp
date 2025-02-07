//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDSPRConstraint.h"
#include "CDConstraint.h"

#define MAXFORCE		1000000.0

enum
{
	//SPRC_STRENGTH			= 1009,
	//SPRC_TARGET				= 1010,
	//SPRC_STIFFNESS			= 1011,
	//SPRC_SHOW_LINES			= 1012,
	//SPRC_LINE_COLOR			= 1013,
	//SPRC_DAMPING				= 1014,
	
	//SPRC_MASS				= 1015,
	//SPRC_GRAVITY				= 1016,
	//SPRC_LENGTH				= 1017,
	//SPRC_GRAV_TARGET			= 1018,
	//SPRC_GRAVITY_MIX			= 1019,
	//SPRC_SHOW_GRAVITY		= 1020,
	
	//SPRC_COLLISION			= 1021,
	//SPRC_DEFLECTION			= 1022,
	//SPRC_BOUNCE				= 1023,
	
	//SPRC_FRICTION			= 1025,
	
	//SPRC_USE_POSITION		= 1026,
	//SPRC_P_OFFSET			= 1027,
	//SPRC_USE_ROTATION		= 1028,
	//SPRC_R_OFFSET			= 1029,
	
	//SPRC_OFF_CENTER			= 1030,
	
	//SPRC_ATTACHMENT			= 1100,

	//SPRC_ID_SPRING			= 2000,
	//SPRC_USE_START_FRAME			= 2001,
	//SPRC_SET_START_FRAME			= 2002,
	//SPRC_RELEASE_START_FRAME		= 2003,

	//SPRC_ID_FORCES			= 3000,
	
	//SPRC_GRAVITY_GROUP		= 4000,
	
	SPRC_START_SET				= 10000,
	SPRC_START_FRAME				= 10001,
	SPRC_START_POS				= 10002,
	SPRC_START_SCA				= 10003,
	SPRC_START_ROT				= 10004
};

enum // CD Clamp Constraint
{
	CDC_TARGET				= 1010,
	
	CDC_CLAMP_COUNT			= 1100,
	CDC_POLY_SURFACE		= 1140,
		
	CDC_CONTACT_HIT				= 5000,
	CDC_CONTACT_PT				= 5100,
	CDC_CONTACT_NORM			= 5200,
	CDC_CONTACT_CNST			= 5300
};

class CDSPRConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

	void Recalculate(State &s);
	State Interpolate(State a, State b, Real mix);
	Derivative Evaluate(BaseTag *tag, State &s, Real t);
	Derivative Evaluate(BaseTag *tag, State s, Real t, Real dt, Derivative &d);
	void Integrate(BaseTag *tag, State &s, Real t, Real dt);
	
	Vector GetGravity(BaseTag *tag, BaseObject *op, Matrix m, BaseContainer *data, BaseDocument *doc);
	void Collision(BaseTag *tag, State &s, Matrix m, CDContact &contact, Vector &force, Vector &torque);
	void Forces(BaseTag *tag, State &s, Real t, Vector &force, Vector &torque);

	Bool DrawSpring(BaseTag *tag, BaseObject *op, BaseObject *goal, BaseDraw *bd, BaseDrawHelp *bh);
	Bool DrawGravity(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
			
public:
	Real t, oldTime, mass, invMass, inertiaTensor, invInertiaTensor;
	Vector oldGoalPos;
	CDQuaternion oldGoalOrientation;
	Bool oldPosSet;
	State opState, prevState;
	LONG oldFrame;
	
	// Debug
	Matrix beginM;
	
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSPRConstraintPlugin); }
};

Bool CDSPRConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(SPRC_SHOW_LINES,true);
	tData->SetVector(SPRC_LINE_COLOR, Vector(1,0,0));

	tData->SetBool(SPRC_USE_POSITION,true);
	tData->SetBool(SPRC_USE_ROTATION,false);
	tData->SetReal(SPRC_STRENGTH,1.0);

	tData->SetReal(SPRC_STIFFNESS,0.5);
	tData->SetReal(SPRC_DAMPING,0.5);
	tData->SetReal(SPRC_MASS,1.0);
	tData->SetReal(SPRC_GRAVITY_MIX,1.0);
	tData->SetReal(SPRC_DEFLECTION,1.0);
	tData->SetReal(SPRC_BOUNCE,0.75);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	t = 0.0;
	oldPosSet = false;
	opState.position = Vector(0,0,0);
	opState.momentum = Vector(0,0,0);
	opState.angularMomentum = Vector(0,0,0);
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDSPRConstraintPlugin::DrawSpring(BaseTag *tag, BaseObject *op, BaseObject *goal, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *cp = bd->GetSceneCamera(doc); 
	if (!cp) cp=bd->GetEditorCamera();
	if (!cp) return true;
	
	Vector gScale = GetGlobalScale(goal);
	Real k = tData->GetReal(SPRC_STIFFNESS);
	Real b = ((50 - tData->GetReal(SPRC_DAMPING) * 50) * gScale.x) * 0.5;
	Vector opPosition = op->GetMg().off;
	Vector goalPosition = goal->GetMg().off;
	
	Vector zView = cp->GetMg().v3;
	Vector toGoal = goalPosition - opPosition;
	Vector crossV = VNorm(VCross(VNorm(toGoal), VNorm(zView)));
	Real length = Len(toGoal);
	Vector lineStart = opPosition + (VNorm(toGoal) * length * 0.45 * k);
	Vector lineEnd = opPosition + (VNorm(toGoal) * length * (1 - 0.45 * k));
	Real lineLen = Len(lineEnd-lineStart);
	
	Vector startPt = opPosition;
	Vector endPt = lineStart;
	CDDrawLine(bd,startPt, endPt);
	
	startPt = endPt;
	endPt = (lineStart + (VNorm(toGoal) * lineLen * 0.2)) + crossV*b;
	CDDrawLine(bd,startPt, endPt);
	
	startPt = endPt;
	endPt = (lineStart + (VNorm(toGoal) * lineLen * 0.4)) - crossV*b;
	CDDrawLine(bd,startPt, endPt);
	
	startPt = endPt;
	endPt = (lineStart + (VNorm(toGoal) * lineLen * 0.6)) + crossV*b;
	CDDrawLine(bd,startPt, endPt);
	
	startPt = endPt;
	endPt = (lineStart + (VNorm(toGoal) * lineLen * 0.8)) - crossV*b;
	CDDrawLine(bd,startPt, endPt);
	
	startPt = endPt;
	endPt = lineEnd;
	CDDrawLine(bd,startPt, endPt);
	
	startPt = endPt;
	endPt = goalPosition;
	CDDrawLine(bd,startPt, endPt);
	
	return true;
}

Bool CDSPRConstraintPlugin::DrawGravity(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	
	if(tData->GetReal(SPRC_GRAVITY) != 0.0)
	{
		BaseObject *cp = bd->GetSceneCamera(doc); 
		if (!cp) cp=bd->GetEditorCamera();
		if (!cp) return true;
		
		Vector gDir = Vector(0,-1,0);
		BaseObject *attr = tData->GetObjectLink(SPRC_GRAV_TARGET,doc);
		if(attr)
		{
			Vector aDir = VNorm(attr->GetMg().off - op->GetMg().off);
			gDir = CDBlend(Vector(0,-1,0),aDir,tData->GetReal(SPRC_GRAVITY_MIX));
		}
		Vector zView = cp->GetMg().v3;
		Vector crossV = VNorm(VCross(VNorm(gDir), VNorm(zView)));
		Vector opPosition = op->GetMg().off;
		
		Real length = tData->GetReal(SPRC_GRAVITY)*5;
		Vector startPt = opPosition;
		Vector endPt = opPosition + VNorm(gDir) * length;
		CDDrawLine(bd,startPt, endPt);
		
		startPt = endPt;
		endPt = (startPt - (VNorm(gDir) * length * 0.2)) + (crossV * length * 0.2);
		CDDrawLine(bd,startPt, endPt);
		
		endPt = (startPt - (VNorm(gDir) * length * 0.2)) - (crossV*length * 0.2);
		CDDrawLine(bd,startPt, endPt);
	}
		
	return true;
}

Bool CDSPRConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument(); if(!doc) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	if(tData->GetBool(SPRC_SHOW_LINES))
	{
		bd->SetPen(tData->GetVector(SPRC_LINE_COLOR));
		BaseObject *goal = tData->GetObjectLink(SPRC_TARGET,doc);
		if (goal)
		{
			DrawSpring(tag,op,goal,bd,bh);
		}
		BaseObject *attch = tData->GetObjectLink(SPRC_ATTACHMENT,doc);
		if (attch)
		{
			DrawSpring(tag,op,attch,bd,bh);
		}
	}
	
	if(tData->GetBool(SPRC_SHOW_GRAVITY))
	{
		DrawGravity(tag,op,bd,bh);
	}
	
	return true;
}

void CDSPRConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdcnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
			
			tData->SetLink(SPRC_TARGET+T_LST,tData->GetLink(SPRC_TARGET,doc));
			tData->SetLink(SPRC_ATTACHMENT+T_LST,tData->GetLink(SPRC_ATTACHMENT,doc));
			tData->SetLink(SPRC_GRAV_TARGET+T_LST,tData->GetLink(SPRC_GRAV_TARGET,doc));
			
			tData->SetBool(SPRC_USE_POSITION+T_LST,tData->GetBool(SPRC_USE_POSITION));
			tData->SetVector(SPRC_P_OFFSET+T_LST,tData->GetVector(SPRC_P_OFFSET));
			tData->SetBool(SPRC_USE_ROTATION+T_LST,tData->GetBool(SPRC_USE_ROTATION));
			tData->SetVector(SPRC_R_OFFSET+T_LST,tData->GetVector(SPRC_R_OFFSET));
			
			tData->SetReal(SPRC_OFF_CENTER+T_LST,tData->GetReal(SPRC_OFF_CENTER));
			tData->SetReal(SPRC_GRAVITY+T_LST,tData->GetReal(SPRC_GRAVITY));
			tData->SetReal(SPRC_LENGTH+T_LST,tData->GetReal(SPRC_LENGTH));
			
			tData->SetBool(SPRC_COLLISION+T_LST,tData->GetBool(SPRC_COLLISION));

			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSPRConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(SPRC_TARGET,tData->GetLink(SPRC_TARGET+T_LST,doc));
		tData->SetLink(SPRC_ATTACHMENT,tData->GetLink(SPRC_ATTACHMENT+T_LST,doc));
		tData->SetLink(SPRC_GRAV_TARGET,tData->GetLink(SPRC_GRAV_TARGET+T_LST,doc));
		
		tData->SetBool(SPRC_USE_POSITION,tData->GetBool(SPRC_USE_POSITION+T_LST));
		tData->SetVector(SPRC_P_OFFSET,tData->GetVector(SPRC_P_OFFSET+T_LST));
		tData->SetBool(SPRC_USE_ROTATION,tData->GetBool(SPRC_USE_ROTATION+T_LST));
		tData->SetVector(SPRC_R_OFFSET,tData->GetVector(SPRC_R_OFFSET+T_LST));
		
		tData->SetReal(SPRC_OFF_CENTER,tData->GetReal(SPRC_OFF_CENTER+T_LST));
		tData->SetReal(SPRC_GRAVITY,tData->GetReal(SPRC_GRAVITY+T_LST));
		tData->SetReal(SPRC_LENGTH,tData->GetReal(SPRC_LENGTH+T_LST));
			
		tData->SetBool(SPRC_COLLISION,tData->GetBool(SPRC_COLLISION+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSPRConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("Message");
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	BaseDocument *doc = node->GetDocument();
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
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
			if(dc->id[0].id==SPRC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==SPRC_SET_START_FRAME)
			{
				LONG frm = doc->GetTime().GetFrame(doc->GetFps());
				tData->SetLong(SPRC_START_FRAME,frm);
				tData->SetVector(SPRC_START_POS,CDGetPos(op));
				tData->SetVector(SPRC_START_SCA,CDGetScale(op));
				tData->SetVector(SPRC_START_ROT,CDGetRot(op));
				tData->SetBool(SPRC_START_SET,true);
			}
			else if(dc->id[0].id==SPRC_RELEASE_START_FRAME)
			{
				tData->SetBool(SPRC_START_SET,false);
				tData->SetBool(SPRC_USE_START_FRAME,false);
			}
		}
	}

	return true;
}

State CDSPRConstraintPlugin::Interpolate(State a, State b, Real mix)
{
	State s;
	
	s.position = CDBlend(a.position, b.position, mix);
	s.momentum = CDBlend(a.momentum, b.momentum, mix);
	s.orientation = CDQSlerp(a.orientation, b.orientation, mix);
	s.angularMomentum = CDBlend(a.angularMomentum, b.angularMomentum, mix);
	
	Recalculate(s);
	
	return s;
}

void CDSPRConstraintPlugin::Recalculate(State &s)
{
	s.velocity = s.momentum * invMass;
	s.angularVelocity = s.angularMomentum * invInertiaTensor;
	s.orientation = !(s.orientation);
	s.spin = 0.5 * CDQuaternion(0.0, s.angularVelocity.x, s.angularVelocity.y, s.angularVelocity.z) * s.orientation;
}

Derivative CDSPRConstraintPlugin::Evaluate(BaseTag *tag, State &s, Real t)
{
	Derivative output;
	
	output.velocity = s.velocity;
	output.spin = s.spin;
	Forces(tag, s, t, output.force, output.torque);
	return output;
}

Derivative CDSPRConstraintPlugin::Evaluate(BaseTag *tag, State s, Real t, Real dt, Derivative &d)
{
	s.position += d.velocity*dt;
	s.momentum += d.force*dt;
	s.orientation += d.spin * dt;
	s.angularMomentum += d.torque * dt;
	Recalculate(s);
	
	Derivative output;
	output.velocity = s.velocity;
	output.spin = s.spin;
	Forces(tag, s, t+dt, output.force, output.torque);
	return output;
}

void CDSPRConstraintPlugin::Integrate(BaseTag *tag, State &s, Real t, Real dt)
{
	Derivative a = Evaluate(tag, s, t);
	Derivative b = Evaluate(tag, s, t, dt*0.5f, a);
	Derivative c = Evaluate(tag, s, t, dt*0.5f, b);
	Derivative d = Evaluate(tag, s, t, dt, c);

	s.position += 1.0 / 6.0 * dt * (a.velocity + 2.0 * (b.velocity + c.velocity) + d.velocity);
	s.momentum += 1.0 / 6.0 * dt * (a.force + 2.0 * (b.force + c.force) + d.force);
	s.orientation += (1.0 / 6.0 * dt * (a.spin + (2.0 * (b.spin + c.spin)) + d.spin));
	s.angularMomentum += 1.0 / 6.0 * dt * (a.torque + 2.0 * (b.torque + c.torque) + d.torque);

	Recalculate(s);
}

Vector CDSPRConstraintPlugin::GetGravity(BaseTag *tag, BaseObject *op, Matrix m, BaseContainer *tData, BaseDocument *doc)
{
	Vector g;
	
	Vector dir, gDir = MInv(m) * Vector(0,-1,0);
	Real gForce = tData->GetReal(SPRC_GRAVITY)*1000;
	
	BaseObject *dirTarget = tData->GetObjectLink(SPRC_GRAV_TARGET,doc);
	if(!dirTarget)
	{
		dir = gDir;
	}
	else
	{
		Vector dTargPos;
		Real toOp, toDt;
		BaseObject *at = tData->GetObjectLink(SPRC_ATTACHMENT,doc);
		if(at)
		{
			BaseObject *goal = tData->GetObjectLink(SPRC_TARGET,doc); 
			dTargPos = MInv(m) * (dirTarget->GetMg().off - CDBlend(goal->GetMg().off,at->GetMg().off,0.5));

			toOp = Len(op->GetMg().off - CDBlend(goal->GetMg().off,at->GetMg().off,0.5));
			toDt = Len(dirTarget->GetMg().off - CDBlend(goal->GetMg().off,at->GetMg().off,0.5));
			if(toOp > toDt) gForce *= (toDt/toOp);
		}
		else
		{
			BaseObject *goal = tData->GetObjectLink(SPRC_TARGET,doc); 
			dTargPos = MInv(m) * (dirTarget->GetMg().off - goal->GetMg().off);
			
			toOp = Len(op->GetMg().off - goal->GetMg().off);
			toDt = Len(dirTarget->GetMg().off - goal->GetMg().off);
			if(toOp > toDt) gForce *= (toDt/toOp);
		}
		dir = CDBlend(VNorm(gDir),VNorm(dTargPos), tData->GetReal(SPRC_GRAVITY_MIX));
	}
	g = VNorm(dir) * mass * gForce;

	return g;
}

void CDSPRConstraintPlugin::Collision(BaseTag *tag, State &s, Matrix m, CDContact &contact, Vector &force, Vector &torque)
{
	BaseContainer *tData = tag->GetDataInstance();

	Real deflect = tData->GetReal(SPRC_DEFLECTION)*10000*mass;
	Real bounce = (1.0 - tData->GetReal(SPRC_BOUNCE))*90*mass;
	Real friction = tData->GetReal(SPRC_FRICTION)*10*mass;
	
	Matrix mRot = MInv(m);
	mRot.off = Vector(0,0,0);
	Vector n = contact.normal;
	
	Vector p = m * (s.position + tData->GetVector(SPRC_P_OFFSET));
	Real penetration = contact.constant - VDot(p, n);
	if(penetration > 0.0)
	{
		Real relativeSpeed = VDot(-n, s.velocity);
		
		Vector penaltyForce = mRot * (n * (penetration * deflect));
		force += penaltyForce;
		
		Vector tangetialVelocity = mRot * (s.velocity + (n * relativeSpeed));
		Vector frictionForce = -tangetialVelocity * friction;
		force += frictionForce;

		Vector dampingForce = mRot * (n * (relativeSpeed * bounce));
		force += dampingForce;
	}
}

void CDSPRConstraintPlugin::Forces(BaseTag *tag, State &s, Real t, Vector &force, Vector &torque)
{
	BaseContainer *tData = tag->GetDataInstance();
	BaseDocument *doc = tag->GetDocument();
	BaseObject *goal = tData->GetObjectLink(SPRC_TARGET,doc); 
	BaseObject *op = tag->GetObject();
	
	Matrix goalM = goal->GetMg(), opM = op->GetMg(), gRotM, rotM;
	Vector goalPos = goalM.off;
	rotM = CDHPBToMatrix(tData->GetVector(SPRC_R_OFFSET));
	gRotM = goalM * rotM;
	gRotM.off = Vector(0,0,0);

	//Clear forces
	force = Vector();
	torque = Vector();
	
	Real k = tData->GetReal(SPRC_STIFFNESS)*1500;
	Real b = tData->GetReal(SPRC_DAMPING)*k*0.08;
	Real d = tData->GetReal(SPRC_LENGTH);
	
	if(tData->GetBool(SPRC_USE_POSITION))
	{
		force += (-k * (Len(s.position)-d) * VNorm(s.position) - b * s.velocity);
		
		BaseObject *attch = tData->GetObjectLink(SPRC_ATTACHMENT,doc); 
		if(attch)
		{
			Vector aPos = MInv(goalM) * attch->GetMg().off;
			force += (-k * (Len(s.position - aPos)-d) * VNorm(s.position - aPos) - b * s.velocity);
		}
		
		force += GetGravity(tag,op,gRotM,tData,doc);
	}
	if(tData->GetBool(SPRC_USE_ROTATION))
	{
		CDQuaternion diffQuat; 
		diffQuat = CDQConjugate(s.orientation) * CDQuaternion();
		Real angle = diffQuat.GetAngle();
		Vector axis = diffQuat.v;

		Real kR = k*5000, bR = b*5000;
		torque += kR * angle * VNorm(axis) - bR * s.angularVelocity;

		Real cLen = tData->GetReal(SPRC_OFF_CENTER);
		if(cLen > 0.0)
		{
			Vector rForce = (MInv(gRotM) * (-k * mass * (oldGoalPos - goalPos)));// * MInv(rotM) - b * mass *prevState.velocity
			Vector offset = tData->GetVector(SPRC_P_OFFSET);
			if(VEqual(offset,Vector(0,0,0),0.001) && tData->GetReal(SPRC_GRAVITY) > 0.0)
			{
				offset = GetGravity(tag,op,gRotM,tData,doc);
			}
			if(!tData->GetBool(SPRC_USE_POSITION) && op->GetType() == ID_CDJOINTOBJECT)
			{
				offset = Vector(0,0,1);
				
				if(op->GetDown())
				{
					if(op->GetDown()->GetType() == ID_CDJOINTOBJECT)
					{
						cLen *= 10.0;
						cLen += CDGetPos(op->GetDown()).z;
					}
				}
				Vector offcenter = rotM * (VNorm(offset) * cLen);
				torque += VCross(rForce, (((s.position + offcenter) - s.position))); 
			}
			else
			{
				Vector offcenter = (VNorm(offset) * cLen);
				torque += VCross(rForce, ((s.position + offcenter) - s.position));
			}
			
		}
	}
		
	// check for collisions
	BaseTag *cTag = op->GetTag(ID_CDCCONSTRAINTPLUGIN);
	if(cTag && tData->GetBool(SPRC_COLLISION))
	{
		BaseContainer *ctData = cTag->GetDataInstance();
		if(ctData)
		{
			LONG i, cnt = ctData->GetLong(CDC_CLAMP_COUNT);
			for(i=0; i<cnt; i++)
			{
				CDContact contact;
				contact.hit = ctData->GetBool(CDC_CONTACT_HIT+i);
				contact.point = ctData->GetVector(CDC_CONTACT_PT+i);
				contact.normal = ctData->GetVector(CDC_CONTACT_NORM+i);
				contact.constant = ctData->GetReal(CDC_CONTACT_CNST+i);
				
				if(contact.hit)
				{
					BaseObject *clampOp = ctData->GetObjectLink(CDC_TARGET+i,doc);
					if(clampOp)
					{
						Collision(tag,s,goalM,contact,force,torque);
					}
				}
			}
		}
	}
	if(Len(force) > MAXFORCE)
	{
		Vector dir = VNorm(force);
		force = dir * MAXFORCE;
	}
}

LONG CDSPRConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{	
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseObject *goal = tData->GetObjectLink(SPRC_TARGET,doc); if(!goal) return false;
		
	if(tData->GetBool(SPRC_USE_START_FRAME) && !tData->GetBool(SPRC_START_SET)) return true;
	
	LONG fps = doc->GetFps();
	LONG curFrame = doc->GetTime().GetFrame(fps);
	
	// check for start frame
	if(tData->GetBool(SPRC_START_SET))
	{
		if(curFrame == tData->GetLong(SPRC_START_FRAME))
		{
			CDSetPos(op,tData->GetVector(SPRC_START_POS));
			CDSetScale(op,tData->GetVector(SPRC_START_SCA));
			CDSetRot(op,tData->GetVector(SPRC_START_ROT));
			oldPosSet = false;
			return true;
		}
	}
	
	Vector theScale = CDGetScale(op);
	Matrix opM = op->GetMg(), goalM = goal->GetMg(), targM, rotM, transM, dtM, localM;
	Vector oldRot = CDGetRot(op), newRot, rotSet;
	
	// build target matrix
	rotM = CDHPBToMatrix(tData->GetVector(SPRC_R_OFFSET));
	targM = goalM * rotM;
	targM.off = goal->GetMg() * tData->GetVector(SPRC_P_OFFSET);

	Real strength = tData->GetReal(SPRC_STRENGTH);
	if(strength == 0.0)
	{
		// if the strength is 0%, constrain to target and return
		oldRot = CDGetRot(op);
		op->SetMg(targM);
		newRot = CDGetRot(op);
		rotSet = CDGetOptimalAngle(oldRot, newRot, op);
		CDSetRot(op,rotSet);
		CDSetScale(op,theScale);
		return true;
	}
	
	// get target quat
	CDQuaternion trgQ;
	trgQ.SetMatrix(targM);
	
	// get op local matrix
	localM = MInv(targM) * opM;
	
	if(!op->GetTag(ID_CDCCONSTRAINTPLUGIN)) tData->SetBool(SPRC_COLLISION, false);
	
	if(!CheckIsRunning(CHECKISRUNNING_EXTERNALRENDERING))
	{
		fps = doc->GetFps();
		LONG newTime = GeGetTimer();
		if(newTime < oldTime+(1000/fps) && curFrame == oldFrame)
		{
			transM = opM;
			if(!tData->GetBool(SPRC_USE_POSITION))
			{
				transM.off = targM.off;
			}
			if(!tData->GetBool(SPRC_USE_ROTATION))
			{
				transM.v1 = targM.v1;
				transM.v2 = targM.v2;
				transM.v3 = targM.v3;
			}
			oldRot = CDGetRot(op);
			op->SetMg(transM);
			
			newRot = CDGetRot(op);
			rotSet = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,rotSet);
			CDSetScale(op,theScale);
			
			return true;
		}
	}
	else
	{
		BaseContainer *bcRender = doc->GetActiveRenderData()->GetDataInstance();
		fps = bcRender->GetLong(RDATA_FRAMERATE);
	}

	LONG frmDif = 1;
	if(CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING))
	{
		if(curFrame - oldFrame > 1) frmDif = curFrame - oldFrame;
	}
	oldFrame = curFrame;
	
	// Get mass
	mass = tData->GetReal(SPRC_MASS);
	invMass = 1.0 / mass;
	
	//Get inertia tensor
	Real opSize = 0.0;
	if(op->GetType() == Onull)
	{
		BaseContainer *oData = op->GetDataInstance();
		if(oData) opSize = oData->GetReal(NULLOBJECT_RADIUS)*2;
	}
	else
	{
		Vector rad = op->GetRad();
		opSize = rad.x*2;
		if(rad.y*2 > opSize)  opSize = rad.y*2;
		if(rad.z*2 > opSize)  opSize = rad.z*2;
	}
	if(opSize < 0.001) opSize = 0.001;
	inertiaTensor = 25000 * mass * 1.0 / 6.0;
	invInertiaTensor = 1.0 / inertiaTensor;
	
	// Set op initial orientation
	State curState = opState;
	curState.orientation.SetMatrix(localM);
	curState.position = localM.off;
	
	if(!oldPosSet)
	{
		oldGoalPos = goalM.off;
		curState.position = localM.off;
		curState.orientation.SetMatrix(localM);
		
		Recalculate(curState);
		oldPosSet = true;
	}

	Real fRate = 1/(Real)fps;
	Real timeCounter = 0.0;
	Real dt = 0.01;
	Real t_previous, t_current;
	while(timeCounter < (fRate * frmDif))
	{		
		t_previous = t;
		if(!VEqual(goalM.off,oldGoalPos,0.001))
		{
			curState.position = localM.off;
		}
		if(trgQ != oldGoalOrientation)
		{
			if(tData->GetBool(SPRC_USE_ROTATION))
			{
				curState.orientation.SetMatrix(localM);
			}
		}
		Integrate(tag, curState, t, dt);
		t += dt;
		timeCounter += dt;
		oldGoalPos = goalM.off;
		prevState = curState;
	}
	Integrate(tag, curState, t, dt);
	Real mix = fRate/timeCounter;
	opState = Interpolate(prevState, curState, mix);
	t_current = CDBlend(t_previous,t,mix);
	t = t_current;
	
	dtM = opState.orientation.GetMatrix();
	dtM.off = opState.position;
	
	CDQuaternion opQuat, goalQuat, mixQuat;
	opQuat.SetMatrix(targM * dtM);
	goalQuat.SetMatrix(targM);
	
	// set op orientation
	if(!tData->GetBool(SPRC_USE_ROTATION)) mixQuat = CDQSlerp(goalQuat,opQuat,0);
	else mixQuat = CDQSlerp(goalQuat,opQuat,strength);
	transM = mixQuat.GetMatrix();
	
	// set op position
	Vector opPos = targM * dtM.off;
	if(!tData->GetBool(SPRC_USE_POSITION)) transM.off = CDBlend(targM.off,opPos,0);
	else transM.off = CDBlend(targM.off,opPos,strength);
	
	oldRot = CDGetRot(op);
	op->SetMg(transM);
	newRot = CDGetRot(op);
	rotSet = CDGetOptimalAngle(oldRot, newRot, op);
	CDSetRot(op,rotSet);
	CDSetScale(op,theScale);
	oldTime = GeGetTimer();
	
	oldGoalOrientation = trgQ;
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSPRConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SPRC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSPRConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case SPRC_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_ATTACHMENT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_USE_POSITION:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_P_OFFSET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_USE_ROTATION:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_R_OFFSET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_USE_START_FRAME:
			if(!tData->GetBool(SPRC_START_SET)) return true;
			else return false;
		case SPRC_SET_START_FRAME:
			if(!tData->GetBool(SPRC_START_SET) && tData->GetBool(SPRC_USE_START_FRAME)) return true;
			else return false;
		case SPRC_RELEASE_START_FRAME:
			return tData->GetBool(SPRC_START_SET);
		case SPRC_OFF_CENTER:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_GRAVITY:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_LENGTH:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_GRAV_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPRC_GRAVITY_MIX:
			if(tData->GetObjectLink(SPRC_GRAV_TARGET,doc)) return true;
			else return false;
		case SPRC_COLLISION:
			if(!tData->GetBool(T_REG)) return false;
			else if(op->GetTag(ID_CDCCONSTRAINTPLUGIN)) return true;
			else return false;
		case SPRC_DEFLECTION:
			return tData->GetBool(SPRC_COLLISION);
		case SPRC_BOUNCE:
			return tData->GetBool(SPRC_COLLISION);
		case SPRC_FRICTION:
			return tData->GetBool(SPRC_COLLISION);
	}
	return true;
}

Bool RegisterCDSPRConstraintPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDSPRCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSPRCONSTRAINTPLUGIN,name,info,CDSPRConstraintPlugin::Alloc,"tCDSPRConstraint","CDSPRConstraint.tif",0);
}

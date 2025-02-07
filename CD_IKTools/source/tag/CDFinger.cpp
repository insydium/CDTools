//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDFinger.h"
#include "CDIKtools.h"
#include "CDMessages.h"

enum
{
	//FNGR_PURCHASE					= 1000,
	
	FNGR_DISK_LEVEL					= 1001,
	FNGR_FREE_OLD_TAG				= 1010,
	FNGR_OLD_TAG						= 1011,
	
	//FNGR_IK_SHOW_LINES				= 10010,
	
	//FNGR_IK_POLE_AXIS				= 10011,
		//FNGR_IK_POLE_Y				= 10012,
		//FNGR_IK_POLE_X				= 10013,
		
	//FNGR_IK_SOLVER_LINK				= 10014,
	//IK_BONE_LINK				= 10015, // Older version

	//FNGR_BEND_VALUE					= 10020,
	//FNGR_CURL_VALUE					= 10021,
	//FNGR_DAMPING_VALUE				= 10022,
	//FNGR_SPREAD_VALUE				= 10023,
	//FNGR_CONNECT_BONES				= 10024,
	//FNGR_IK_POLE_NX					= 10025,
	//FNGR_IK_POLE_NY					= 10026,

	//FNGR_SPREAD_MIN					= 10027,
	//FNGR_SPREAD_MAX					= 10028,
	//FNGR_BEND_MIN					= 10029,
	//FNGR_BEND_MAX					= 10030,
	//FNGR_CURL_MIN					= 10031,
	//FNGR_CURL_MAX					= 10032,
	//FNGR_BEND_N_CURL				= 10033,

	//FNGR_LINE_COLOR					= 10050,
	//FNGR_LEFT_HAND						= 10051,
	//FNGR_INCLUDE_TIP					= 10052,

	//FNGR_IK_SOLVER_GROUP		= 20000,
	//FNGR_CONTROLLER_GROUP		= 30000,
	//FNGR_RANGE_GROUP			= 40000,
};

class CDFingerPlugin : public CDTagData
{
private:
	Vector poleV;
	
	LONG GetPoleAxis(BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
public:		
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDFingerPlugin); }		
};

Bool CDFingerPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(FNGR_FREE_OLD_TAG,false);
	
	tData->SetLong(FNGR_IK_POLE_AXIS,FNGR_IK_POLE_Y);
	tData->SetBool(FNGR_IK_SHOW_LINES,true);
	tData->SetReal(FNGR_DAMPING_VALUE,0);
	tData->SetReal(FNGR_SPREAD_VALUE,0);
	tData->SetBool(FNGR_CONNECT_BONES,false);
	tData->SetBool(FNGR_INCLUDE_TIP,false);
	
	tData->SetReal(FNGR_SPREAD_MIN,Rad(-15.0));
	tData->SetReal(FNGR_SPREAD_MAX,Rad(15.0));
	tData->SetReal(FNGR_BEND_MIN,Rad(-25.0));
	tData->SetReal(FNGR_BEND_MAX,Rad(100.0));
	tData->SetReal(FNGR_CURL_MIN,Rad(-5.0));
	tData->SetReal(FNGR_CURL_MAX,Rad(100.0));
	tData->SetVector(FNGR_LINE_COLOR, Vector(1,0,0));

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	return true;
}

Bool CDFingerPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(FNGR_DISK_LEVEL,level);
	
	return true;
}

void CDFingerPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(FNGR_IK_POLE_AXIS+T_LST,tData->GetLong(FNGR_IK_POLE_AXIS));
			tData->SetLink(FNGR_IK_SOLVER_LINK+T_LST,tData->GetLink(FNGR_IK_SOLVER_LINK,doc));
			
			tData->SetReal(FNGR_SPREAD_MIN+T_LST,tData->GetReal(FNGR_SPREAD_MIN));
			tData->SetReal(FNGR_SPREAD_MAX+T_LST,tData->GetReal(FNGR_SPREAD_MAX));
			tData->SetReal(FNGR_BEND_MIN+T_LST,tData->GetReal(FNGR_BEND_MIN));
			tData->SetReal(FNGR_BEND_MAX+T_LST,tData->GetReal(FNGR_BEND_MAX));
			tData->SetReal(FNGR_CURL_MIN+T_LST,tData->GetReal(FNGR_CURL_MIN));
			tData->SetReal(FNGR_CURL_MAX+T_LST,tData->GetReal(FNGR_CURL_MAX));
			
			LONG i;
			BaseObject *ch = op->GetDown();
			for(i=0; i<4; i++)
			{
				if(ch)
				{
					tData->SetVector(T_CP+i,CDGetPos(ch));
					ch = ch->GetDown();
				}
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDFingerPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(FNGR_DISK_LEVEL) < 1)
			{
				BaseObject *dest = tData->GetObjectLink(IK_BONE_LINK,doc);
				if(dest && dest != op)
				{
					SpecialEventAdd(CD_MSG_FINGER_THUMB_UPDATE,0,0);
				}
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
	
	switch (type)
	{
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == FNGR_LEFT_HAND)
			{
				tData->SetReal(FNGR_SPREAD_VALUE,tData->GetReal(FNGR_SPREAD_VALUE) * -1);
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==FNGR_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

Bool CDFingerPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();

	BaseObject *theBone = op;
	BaseObject *ch = theBone->GetDown(); if(!ch) return true;
	BaseObject *pole = tData->GetObjectLink(FNGR_IK_SOLVER_LINK,doc); if(!pole) return true;

	Vector bonePosition = theBone->GetMg().off;
	Vector polePosition = pole->GetMg().off;
	Vector tipPosition;
	
	if(!tData->GetBool(FNGR_IK_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(FNGR_LINE_COLOR));
	CDDrawLine(bd,polePosition, bonePosition);
	
	CDDrawLine(bd,bonePosition, ch->GetMg().off);
	
	theBone = ch;
	ch = theBone->GetDown(); 
	if(!ch)
	{
		if(CDIsBone(theBone))
		{
			tipPosition = theBone->GetMg().off + theBone->GetMg().v3 * GetBoneLength(theBone);
			CDDrawLine(bd,theBone->GetMg().off, tipPosition);
		}
		return true;
	}
	else
	{
		CDDrawLine(bd,theBone->GetMg().off, ch->GetMg().off);
	}
		
	theBone = ch;
	ch = theBone->GetDown(); 
	if(!ch)
	{
		if(CDIsBone(theBone))
		{
			tipPosition = theBone->GetMg().off + theBone->GetMg().v3 * GetBoneLength(theBone);
			CDDrawLine(bd,theBone->GetMg().off, tipPosition);
		}
		return true;
	}
	else
	{
		CDDrawLine(bd,theBone->GetMg().off, ch->GetMg().off);
		if(CDIsBone(ch))
		{
			tipPosition = theBone->GetMg().off + theBone->GetMg().v3 * GetBoneLength(theBone);
			CDDrawLine(bd,theBone->GetMg().off, tipPosition);
		}
	}

	return true;
}

Bool CDFingerPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(FNGR_IK_POLE_AXIS,tData->GetLong(FNGR_IK_POLE_AXIS+T_LST));
		tData->SetLink(FNGR_IK_SOLVER_LINK,tData->GetLink(FNGR_IK_SOLVER_LINK+T_LST,doc));
		
		tData->SetReal(FNGR_SPREAD_MIN,tData->GetReal(FNGR_SPREAD_MIN+T_LST));
		tData->SetReal(FNGR_SPREAD_MAX,tData->GetReal(FNGR_SPREAD_MAX+T_LST));
		tData->SetReal(FNGR_BEND_MIN,tData->GetReal(FNGR_BEND_MIN+T_LST));
		tData->SetReal(FNGR_BEND_MAX,tData->GetReal(FNGR_BEND_MAX+T_LST));
		tData->SetReal(FNGR_CURL_MIN,tData->GetReal(FNGR_CURL_MIN+T_LST));
		tData->SetReal(FNGR_CURL_MAX,tData->GetReal(FNGR_CURL_MAX+T_LST));
		
		LONG i;
		BaseObject *ch = op->GetDown();
		for(i=0; i<4; i++)
		{
			if(ch)
			{
				CDSetPos(ch,tData->GetVector(T_CP+i));
				ch = ch->GetDown();
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

LONG CDFingerPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(FNGR_IK_POLE_AXIS))
	{
		case FNGR_IK_POLE_X:	
			return POLE_X;
		case FNGR_IK_POLE_Y:
			return POLE_Y;
		case FNGR_IK_POLE_NX:	
			return POLE_NX;
		case FNGR_IK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

LONG CDFingerPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseDraw   *bd = doc->GetRenderBaseDraw(); if(!bd) return false;
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	BaseObject *pole = tData->GetObjectLink(FNGR_IK_SOLVER_LINK,doc); if(!pole) return false;
	BaseObject *ch = op->GetDown(); if(!ch) return false;

	Matrix opM = op->GetMg(), poleM = pole->GetMg();
	Matrix rotM, transM;
	Vector theScale, oldRot, newRot, rotSet, opPos, chPos;
	
	// check plane points for 0 distance
	if(poleM.off == opM.off) return false;
	
	//	Set the vector to the Pole
	poleV = poleM.off - opM.off;
	
	Matrix rpM = GetRPMatrix(GetPoleAxis(tData),opM.off,opM.v3,poleV);

	theScale = CDGetScale(op);
	opPos = CDGetPos(op);
	oldRot = CDGetRot(op);
			
	op->SetMg(rpM);
	
	newRot = CDGetRot(op);
	rotSet = CDGetOptimalAngle(oldRot, newRot, op);
	
	CDSetRot(op,rotSet);	
	CDSetScale(op,theScale);
	CDSetPos(op,opPos);	

	// Set the finger's first bone
	chPos = CDGetPos(ch);
	transM = ch->GetMg();
	if(tData->GetBool(FNGR_CONNECT_BONES))
	{
		chPos = CDGetPos(ch);
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(ch->GetUp())) chPos.z = GetBoneLength(ch->GetUp());
		CDSetPos(ch,chPos);
	}

	transM.v1 = rpM.v1;
	transM.v2 = rpM.v2;
	transM.v3 = rpM.v3;
	Vector rAxis; 
	Real rAngle;
	switch(tData->GetLong(FNGR_IK_POLE_AXIS))
	{
		case FNGR_IK_POLE_X:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = (tData->GetReal(FNGR_SPREAD_VALUE));	
			break;
		case FNGR_IK_POLE_Y:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = (tData->GetReal(FNGR_SPREAD_VALUE));	
			break;
		case FNGR_IK_POLE_NX:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = -(tData->GetReal(FNGR_SPREAD_VALUE));	
			break;
		case FNGR_IK_POLE_NY:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = -(tData->GetReal(FNGR_SPREAD_VALUE));	
			break;
		default:
		break;
	}
	if(tData->GetBool(FNGR_LEFT_HAND)) rAngle *= -1;
	
	rotM = RotAxisToMatrix(rAxis,rAngle);
	transM = transM * rotM;

	switch (tData->GetLong(FNGR_IK_POLE_AXIS))
	{
		case FNGR_IK_POLE_X:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = tData->GetReal(FNGR_BEND_VALUE);	
			break;
		case FNGR_IK_POLE_Y:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = -(tData->GetReal(FNGR_BEND_VALUE));	
			break;
		case FNGR_IK_POLE_NX:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = -(tData->GetReal(FNGR_BEND_VALUE));	
			break;
		case FNGR_IK_POLE_NY:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = tData->GetReal(FNGR_BEND_VALUE);	
			break;
		default:
		break;
	}
	rotM = RotAxisToMatrix(rAxis,rAngle);
	transM = transM * rotM;
	
	theScale = CDGetScale(ch);
	oldRot = CDGetRot(ch);		
	ch->SetMg(transM);
	
	newRot = CDGetRot(ch);
	rotSet = CDGetOptimalAngle(oldRot, newRot, ch);
	
	CDSetRot(ch,rotSet);	
	CDSetScale(ch,theScale);
	CDSetPos(ch,chPos);	
	
	//Set the finger's second bone
	ch = ch->GetDown();
	if(!ch) return true;
	
	chPos = CDGetPos(ch);
	if(tData->GetBool(FNGR_CONNECT_BONES))
	{
		chPos = CDGetPos(ch);
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(ch->GetUp())) chPos.z = GetBoneLength(ch->GetUp());
	}

	LONG curlID = FNGR_CURL_VALUE;
	if(tData->GetBool(FNGR_BEND_N_CURL)) curlID = FNGR_BEND_VALUE;
	switch (tData->GetLong(FNGR_IK_POLE_AXIS))
	{
		case FNGR_IK_POLE_X:
			rAngle = CDClamp(tData->GetReal(FNGR_CURL_MIN),tData->GetReal(FNGR_CURL_MAX),tData->GetReal(curlID));
			rotSet = Vector(rAngle,0,0);
			break;
		case FNGR_IK_POLE_Y:
			rAngle = -CDClamp(tData->GetReal(FNGR_CURL_MIN),tData->GetReal(FNGR_CURL_MAX),tData->GetReal(curlID));
			rotSet = Vector(0,rAngle,0);
			break;
		case FNGR_IK_POLE_NX:
			rAngle = -CDClamp(tData->GetReal(FNGR_CURL_MIN),tData->GetReal(FNGR_CURL_MAX),tData->GetReal(curlID));	
			rotSet = Vector(rAngle,0,0);
			break;
		case FNGR_IK_POLE_NY:
			rAngle = CDClamp(tData->GetReal(FNGR_CURL_MIN),tData->GetReal(FNGR_CURL_MAX),tData->GetReal(curlID));	
			rotSet = Vector(0,rAngle,0);
			break;
		default:
		break;
	}
	CDSetRot(ch,rotSet);	
	CDSetPos(ch,chPos);	
	
	//Set the finger's third bone
	ch = ch->GetDown();
	if(!ch) return true;
	
	chPos = CDGetPos(ch);
	if(tData->GetBool(FNGR_CONNECT_BONES))
	{
		chPos = CDGetPos(ch);
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(ch->GetUp())) chPos.z = GetBoneLength(ch->GetUp());
	}
	
	Real damp = 1 - tData->GetReal(FNGR_DAMPING_VALUE);
	rAngle = CDBlend(0,rAngle,damp);
	switch (tData->GetLong(FNGR_IK_POLE_AXIS))
	{
		case FNGR_IK_POLE_X:
			rotSet = Vector(rAngle,0,0);
			break;
		case FNGR_IK_POLE_Y:
			rotSet = Vector(0,rAngle,0);
			break;
		case FNGR_IK_POLE_NX:
			rotSet = Vector(rAngle,0,0);
			break;
		case FNGR_IK_POLE_NY:
			rotSet = Vector(0,rAngle,0);
			break;
		default:
			break;
	}
	CDSetRot(ch,rotSet);	
	CDSetPos(ch,chPos);	
	
	if(tData->GetBool(FNGR_INCLUDE_TIP))
	{
		ch = ch->GetDown();
		while(ch)
		{
			chPos = CDGetPos(ch);
			if(tData->GetBool(FNGR_CONNECT_BONES))
			{
				chPos = CDGetPos(ch);
				chPos.x = 0.0;
				chPos.y = 0.0;
				if(CDIsBone(ch->GetUp())) chPos.z = GetBoneLength(ch->GetUp());
			}

			Real damp = 1 - tData->GetReal(FNGR_DAMPING_VALUE);
			rAngle = CDBlend(0,rAngle,damp);
			switch (tData->GetLong(FNGR_IK_POLE_AXIS))
			{
				case FNGR_IK_POLE_X:
					rotSet = Vector(rAngle,0,0);
					break;
				case FNGR_IK_POLE_Y:
					rotSet = Vector(0,rAngle,0);
					break;
				case FNGR_IK_POLE_NX:
					rotSet = Vector(rAngle,0,0);
					break;
				case FNGR_IK_POLE_NY:
					rotSet = Vector(0,rAngle,0);
					break;
				default:
					break;
			}
			CDSetRot(ch,rotSet);	
			CDSetPos(ch,chPos);	

			ch = ch->GetDown();
		}
	}

	return CD_EXECUTION_RESULT_OK;
}

Bool CDFingerPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(FNGR_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	LONG idOffset = 0;
	if(!tData->GetBool(T_REG)) idOffset = 510000;
	
	Real spreadMin = tData->GetReal(FNGR_SPREAD_MIN+idOffset);	
	Real spreadMax = tData->GetReal(FNGR_SPREAD_MAX+idOffset);	
	Real bendMin = tData->GetReal(FNGR_BEND_MIN+idOffset);	
	Real bendMax = tData->GetReal(FNGR_BEND_MAX+idOffset);	
	Real curlMin = tData->GetReal(FNGR_CURL_MIN+idOffset);	
	Real curlMax = tData->GetReal(FNGR_CURL_MAX+idOffset);	

	BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc1.SetString(DESC_NAME, GeLoadString(H_SPREAD));
	bc1.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc1.SetReal(DESC_MINSLIDER, spreadMin);
	bc1.SetReal(DESC_MAXSLIDER, spreadMax);
	bc1.SetReal(DESC_STEP, Rad(0.1));
	bc1.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc1.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(FNGR_SPREAD_VALUE, DTYPE_REAL, 0), bc1, DescLevel(FNGR_CONTROLLER_GROUP))) return true;

	BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc2.SetString(DESC_NAME, GeLoadString(H_BEND));
	bc2.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc2.SetReal(DESC_MINSLIDER, bendMin);
	bc2.SetReal(DESC_MAXSLIDER, bendMax);
	bc2.SetReal(DESC_STEP, Rad(0.1));
	bc2.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc2.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(FNGR_BEND_VALUE, DTYPE_REAL, 0), bc2, DescLevel(FNGR_CONTROLLER_GROUP))) return true;

	BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc3.SetString(DESC_NAME, GeLoadString(H_CURL));
	bc3.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc3.SetReal(DESC_MINSLIDER, curlMin);
	bc3.SetReal(DESC_MAXSLIDER, curlMax);
	bc3.SetReal(DESC_STEP, Rad(0.1));
	bc3.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc3.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(FNGR_CURL_VALUE, DTYPE_REAL, 0), bc3, DescLevel(FNGR_CONTROLLER_GROUP))) return true;

	BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc4.SetString(DESC_NAME, GeLoadString(H_DAMPING));
	bc4.SetReal(DESC_MINSLIDER,0.0);
	bc4.SetReal(DESC_MAXSLIDER,1.0);
	bc4.SetReal(DESC_STEP,0.01);
	bc4.SetLong(DESC_UNIT,DESC_UNIT_PERCENT);
	bc4.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
	bc4.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(FNGR_DAMPING_VALUE,DTYPE_REAL,0),bc4,DescLevel(FNGR_CONTROLLER_GROUP))) return false;

	BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_BOOL);
	bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
	bc5.SetString(DESC_NAME, GeLoadString(H_BND_N_CRL));
	bc5.SetBool(DESC_DEFAULT, false);
	if(!description->SetParameter(DescLevel(FNGR_BEND_N_CURL, DTYPE_BOOL, 0), bc5, DescLevel(FNGR_CONTROLLER_GROUP))) return true;

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDFingerPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case FNGR_IK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FNGR_IK_SOLVER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FNGR_CURL_VALUE:	
			if(tData->GetBool(FNGR_BEND_N_CURL)) return false;
			else return true;
		case FNGR_DAMPING_VALUE:	
			if(tData->GetBool(FNGR_BEND_N_CURL)) return false;
			else return true;
		case FNGR_SPREAD_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FNGR_SPREAD_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FNGR_BEND_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FNGR_BEND_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FNGR_CURL_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case FNGR_CURL_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDFingerPlugin(void)
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
	if(!reg) info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE;
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDFINGER); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDFINGERPLUGIN,name,info,CDFingerPlugin::Alloc,"tCDFinger","CDfinger.tif",1);
}

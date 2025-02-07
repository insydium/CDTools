//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDThumb.h"
#include "CDIKtools.h"
//#include "CDMessages.h"

enum
{
	//TMB_PURCHASE					= 1000,
	
	TMB_DISK_LEVEL				= 1001,
	TMB_FREE_OLD_TAGL			= 1010,
	TMB_OLD_TAG					= 1011,
	
	//TMB_IK_SHOW_LINES		= 10010,
	
	//TMB_IK_POLE_AXIS		= 10011,
		//TMB_IK_POLE_Y		= 10012,
		//TMB_IK_POLE_X		= 10013,
		
	//TMB_IK_SOLVER_LINK		= 10014,
	//TMB_IK_BONE_LINK		= 10015,

	//TMB_GRIP_VALUE			= 10016,
	//TMB_TWIST_VALUE			= 10017,
	//TMB_SPREAD_VALUE		= 10018,
	//TMB_BEND_VALUE			= 10019,
	//TMB_CURL_VALUE			= 10020,
	//TMB_IK_POLE_NX			= 10021,
	//TMB_IK_POLE_NY			= 10022,

	//TMB_GRIP_MIN			= 10023,
	//TMB_GRIP_MAX			= 10024,
	//TMB_TWIST_MIN			= 10025,
	//TMB_TWIST_MAX			= 10026,
	//TMB_SPREAD_MIN			= 10027,
	//TMB_SPREAD_MAX			= 10028,
	//TMB_BEND_MIN			= 10029,
	//TMB_BEND_MAX			= 10030,
	//TMB_CURL_MIN			= 10031,
	//TMB_CURL_MAX			= 10032,
	//TMB_CONNECT_BONES		= 10033,
	//TMB_BEND_N_CURL			= 10034,

	//TMB_LINE_COLOR			= 10050,
	//TMB_LEFT_HAND				= 10051,

	//TMB_IK_SOLVER_GROUP		 = 20000,
	//TMB_CONTROLLER_GROUP	 = 30000,
	//TMB_RANGE_GROUP			= 40000,
};

class CDThumbPlugin : public CDTagData
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
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDThumbPlugin); }		
};

Bool CDThumbPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(TMB_FREE_OLD_TAGL,false);
	
	tData->SetLong(TMB_IK_POLE_AXIS,TMB_IK_POLE_Y);
	tData->SetBool(TMB_IK_SHOW_LINES,true);
	tData->SetReal(TMB_SPREAD_VALUE,0);
	tData->SetReal(TMB_TWIST_VALUE,0);
	tData->SetBool(TMB_CONNECT_BONES,false);

	tData->SetReal(TMB_GRIP_MIN,Rad(-30.0));
	tData->SetReal(TMB_GRIP_MAX,Rad(30.0));
	tData->SetReal(TMB_TWIST_MIN,Rad(-30.0));
	tData->SetReal(TMB_TWIST_MAX,Rad(30.0));
	tData->SetReal(TMB_SPREAD_MIN,Rad(-25.0));
	tData->SetReal(TMB_SPREAD_MAX,Rad(35.0));
	tData->SetReal(TMB_BEND_MIN,Rad(0.0));
	tData->SetReal(TMB_BEND_MAX,Rad(60.0));
	tData->SetReal(TMB_CURL_MIN,Rad(-25.0));
	tData->SetReal(TMB_CURL_MAX,Rad(90.0));
	tData->SetVector(TMB_LINE_COLOR, Vector(1,0,0));

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	return true;
}

Bool CDThumbPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(TMB_DISK_LEVEL,level);
	
	return true;
}

void CDThumbPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(TMB_IK_POLE_AXIS+T_LST,tData->GetLong(TMB_IK_POLE_AXIS));
			tData->SetLink(TMB_IK_SOLVER_LINK+T_LST,tData->GetLink(TMB_IK_SOLVER_LINK,doc));
			
			tData->SetReal(TMB_GRIP_MIN+T_LST,tData->GetReal(TMB_GRIP_MIN));
			tData->SetReal(TMB_GRIP_MAX+T_LST,tData->GetReal(TMB_GRIP_MAX));
			tData->SetReal(TMB_TWIST_MIN+T_LST,tData->GetReal(TMB_TWIST_MIN));
			tData->SetReal(TMB_TWIST_MAX+T_LST,tData->GetReal(TMB_TWIST_MAX));
			tData->SetReal(TMB_SPREAD_MIN+T_LST,tData->GetReal(TMB_SPREAD_MIN));
			tData->SetReal(TMB_SPREAD_MAX+T_LST,tData->GetReal(TMB_SPREAD_MAX));
			tData->SetReal(TMB_BEND_MIN+T_LST,tData->GetReal(TMB_BEND_MIN));
			tData->SetReal(TMB_BEND_MAX+T_LST,tData->GetReal(TMB_BEND_MAX));
			tData->SetReal(TMB_CURL_MIN+T_LST,tData->GetReal(TMB_CURL_MIN));
			tData->SetReal(TMB_CURL_MAX+T_LST,tData->GetReal(TMB_CURL_MAX));
			
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

Bool CDThumbPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(TMB_DISK_LEVEL) < 1)
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
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == TMB_LEFT_HAND)
			{
				tData->SetReal(TMB_SPREAD_VALUE,tData->GetReal(TMB_SPREAD_VALUE) * -1);
				tData->SetReal(TMB_TWIST_VALUE,tData->GetReal(TMB_TWIST_VALUE) * -1);
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==TMB_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

Bool CDThumbPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	
	BaseObject *theBone = op;
	BaseObject *ch = theBone->GetDown(); if(!ch) return true;
	BaseObject *pole = tData->GetObjectLink(TMB_IK_SOLVER_LINK,doc); if(!pole) return true;
	
	Vector bonePosition = theBone->GetMg().off;
	Vector polePosition = pole->GetMg().off;
	Vector tipPosition;
	
	if(!tData->GetBool(TMB_IK_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(TMB_LINE_COLOR));
	CDDrawLine(bd,polePosition, bonePosition);
	
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

Bool CDThumbPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(TMB_IK_POLE_AXIS,tData->GetLong(TMB_IK_POLE_AXIS+T_LST));
		tData->SetLink(TMB_IK_SOLVER_LINK,tData->GetLink(TMB_IK_SOLVER_LINK+T_LST,doc));
		
		tData->SetReal(TMB_GRIP_MIN,tData->GetReal(TMB_GRIP_MIN+T_LST));
		tData->SetReal(TMB_GRIP_MAX,tData->GetReal(TMB_GRIP_MAX+T_LST));
		tData->SetReal(TMB_TWIST_MIN,tData->GetReal(TMB_TWIST_MIN+T_LST));
		tData->SetReal(TMB_TWIST_MAX,tData->GetReal(TMB_TWIST_MAX+T_LST));
		tData->SetReal(TMB_SPREAD_MIN,tData->GetReal(TMB_SPREAD_MIN+T_LST));
		tData->SetReal(TMB_SPREAD_MAX,tData->GetReal(TMB_SPREAD_MAX+T_LST));
		tData->SetReal(TMB_BEND_MIN,tData->GetReal(TMB_BEND_MIN+T_LST));
		tData->SetReal(TMB_BEND_MAX,tData->GetReal(TMB_BEND_MAX+T_LST));
		tData->SetReal(TMB_CURL_MIN,tData->GetReal(TMB_CURL_MIN+T_LST));
		tData->SetReal(TMB_CURL_MAX,tData->GetReal(TMB_CURL_MAX+T_LST));
		
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

LONG CDThumbPlugin::GetPoleAxis(BaseContainer *tData)
{
	switch(tData->GetLong(TMB_IK_POLE_AXIS))
	{
		case TMB_IK_POLE_X:	
			return POLE_X;
		case TMB_IK_POLE_Y:
			return POLE_Y;
		case TMB_IK_POLE_NX:	
			return POLE_NX;
		case TMB_IK_POLE_NY:
			return POLE_NY;
		default:
			return POLE_Y;
	}
}

LONG CDThumbPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseDraw   *bd = doc->GetRenderBaseDraw(); if(!bd) return false;
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	BaseObject *pole = tData->GetObjectLink(TMB_IK_SOLVER_LINK,doc); if(!pole) return false;
	BaseObject *ch = op->GetDown(); if(!ch) return false;

	Matrix opM = op->GetMg(), poleM = pole->GetMg();
	Matrix rotM, transM;
	Vector theScale, oldRot, newRot, rotSet, opPos, chPos;
	
	//	Set the vector to the Pole
	if(poleM.off == opM.off) return false;
	
	//	Set the vectors to the Pole and Goal.
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

	chPos = CDGetPos(ch);
	transM = ch->GetMg();
	if(tData->GetBool(TMB_CONNECT_BONES))
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
	switch (tData->GetLong(TMB_IK_POLE_AXIS))
	{
		case TMB_IK_POLE_X:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = -(tData->GetReal(TMB_SPREAD_VALUE));	
			break;
		case TMB_IK_POLE_Y:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = -(tData->GetReal(TMB_SPREAD_VALUE));	
			break;
		case TMB_IK_POLE_NX:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = (tData->GetReal(TMB_SPREAD_VALUE));	
			break;
		case TMB_IK_POLE_NY:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = (tData->GetReal(TMB_SPREAD_VALUE));	
			break;
		default:
		break;
	}
	if(tData->GetBool(TMB_LEFT_HAND)) rAngle *= -1;
	
	rotM = RotAxisToMatrix(rAxis,rAngle);
	transM = transM * rotM;

	switch (tData->GetLong(TMB_IK_POLE_AXIS))
	{
		case TMB_IK_POLE_X:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = -(tData->GetReal(TMB_GRIP_VALUE));
			break;
		case TMB_IK_POLE_Y:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = (tData->GetReal(TMB_GRIP_VALUE));
			break;
		case TMB_IK_POLE_NX:
			rAxis = Vector(0,1,0);	// Set the Rotation axis to Y
			rAngle = (tData->GetReal(TMB_GRIP_VALUE));
			break;
		case TMB_IK_POLE_NY:
			rAxis = Vector(1,0,0);	// Set the Rotation axis to X
			rAngle = -(tData->GetReal(TMB_GRIP_VALUE));
			break;
		default:
		break;
	}
	rotM = RotAxisToMatrix(rAxis,rAngle);
	transM = transM * rotM;

	rAxis = Vector(0,0,1);
	switch (tData->GetLong(TMB_IK_POLE_AXIS))
	{
		case TMB_IK_POLE_X:
			rAngle = -(tData->GetReal(TMB_TWIST_VALUE));	
			break;
		case TMB_IK_POLE_Y:
			rAngle = -(tData->GetReal(TMB_TWIST_VALUE));	
			break;
		case TMB_IK_POLE_NX:
			rAngle = -(tData->GetReal(TMB_TWIST_VALUE));	
			break;
		case TMB_IK_POLE_NY:
			rAngle = -(tData->GetReal(TMB_TWIST_VALUE));	
			break;
		default:
		break;
	}
	if(tData->GetBool(TMB_LEFT_HAND)) rAngle *= -1;
	
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

	ch = ch->GetDown();
	if(!ch) return true;
	
	chPos = CDGetPos(ch);
	if(tData->GetBool(TMB_CONNECT_BONES))
	{
		chPos = CDGetPos(ch);
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(ch->GetUp())) chPos.z = GetBoneLength(ch->GetUp());
	}

	switch (tData->GetLong(TMB_IK_POLE_AXIS))
	{
		case TMB_IK_POLE_X:
			rAngle = tData->GetReal(TMB_BEND_VALUE);	
			rotSet = Vector(rAngle,0,0);
			break;
		case TMB_IK_POLE_Y:
			rAngle = -(tData->GetReal(TMB_BEND_VALUE));	
			rotSet = Vector(0,rAngle,0);
			break;
		case TMB_IK_POLE_NX:
			rAngle = -(tData->GetReal(TMB_BEND_VALUE));	
			rotSet = Vector(rAngle,0,0);
			break;
		case TMB_IK_POLE_NY:
			rAngle = tData->GetReal(TMB_BEND_VALUE);	
			rotSet = Vector(0,rAngle,0);
			break;
		default:
		break;
	}
	CDSetRot(ch,rotSet);	
	CDSetPos(ch,chPos);	
	
	ch = ch->GetDown();
	if(!ch) return true;
	
	chPos = CDGetPos(ch);
	if(tData->GetBool(TMB_CONNECT_BONES))
	{
		chPos = CDGetPos(ch);
		chPos.x = 0.0;
		chPos.y = 0.0;
		if(CDIsBone(ch->GetUp())) chPos.z = GetBoneLength(ch->GetUp());
	}

	LONG curlID = TMB_CURL_VALUE;
	if(tData->GetBool(TMB_BEND_N_CURL)) curlID = TMB_BEND_VALUE;
	switch (tData->GetLong(TMB_IK_POLE_AXIS))
	{
		case TMB_IK_POLE_X:
			rAngle = CDClamp(tData->GetReal(TMB_CURL_MIN),tData->GetReal(TMB_CURL_MAX),tData->GetReal(curlID));	
			rotSet = Vector(rAngle,0,0);
			break;
		case TMB_IK_POLE_Y:
			rAngle = -CDClamp(tData->GetReal(TMB_CURL_MIN),tData->GetReal(TMB_CURL_MAX),tData->GetReal(curlID));	
			rotSet = Vector(0,rAngle,0);
			break;
		case TMB_IK_POLE_NX:
			rAngle = -CDClamp(tData->GetReal(TMB_CURL_MIN),tData->GetReal(TMB_CURL_MAX),tData->GetReal(curlID));	
			rotSet = Vector(rAngle,0,0);
			break;
		case TMB_IK_POLE_NY:
			rAngle = CDClamp(tData->GetReal(TMB_CURL_MIN),tData->GetReal(TMB_CURL_MAX),tData->GetReal(curlID));	
			rotSet = Vector(0,rAngle,0);
			break;
		default:
		break;
	}
	CDSetRot(ch,rotSet);	
	CDSetPos(ch,chPos);	
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDThumbPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(TMB_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	LONG idOffset = 0;
	if(!tData->GetBool(T_REG)) idOffset = 510000;
	
	Real gripMin = tData->GetReal(TMB_GRIP_MIN+idOffset);	
	Real gripMax = tData->GetReal(TMB_GRIP_MAX+idOffset);	
	Real twistMin = tData->GetReal(TMB_TWIST_MIN+idOffset);	
	Real twistMax = tData->GetReal(TMB_TWIST_MAX+idOffset);	
	Real spreadMin = tData->GetReal(TMB_SPREAD_MIN+idOffset);	
	Real spreadMax = tData->GetReal(TMB_SPREAD_MAX+idOffset);	
	Real bendMin = tData->GetReal(TMB_BEND_MIN+idOffset);	
	Real bendMax = tData->GetReal(TMB_BEND_MAX+idOffset);	
	Real curlMin = tData->GetReal(TMB_CURL_MIN+idOffset);	
	Real curlMax = tData->GetReal(TMB_CURL_MAX+idOffset);	

	BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc1.SetString(DESC_NAME, GeLoadString(H_GRIP));
	bc1.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc1.SetReal(DESC_MINSLIDER, gripMin);
	bc1.SetReal(DESC_MAXSLIDER, gripMax);
	bc1.SetReal(DESC_STEP, Rad(0.1));
	bc1.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc1.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(TMB_GRIP_VALUE, DTYPE_REAL, 0), bc1, DescLevel(TMB_CONTROLLER_GROUP))) return true;

	BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc2.SetString(DESC_NAME, GeLoadString(H_TWIST));
	bc2.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc2.SetReal(DESC_MINSLIDER, twistMin);
	bc2.SetReal(DESC_MAXSLIDER, twistMax);
	bc2.SetReal(DESC_STEP, Rad(0.1));
	bc2.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc2.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(TMB_TWIST_VALUE, DTYPE_REAL, 0), bc2, DescLevel(TMB_CONTROLLER_GROUP))) return true;

	BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc3.SetString(DESC_NAME, GeLoadString(H_SPREAD));
	bc3.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc3.SetReal(DESC_MINSLIDER, spreadMin);
	bc3.SetReal(DESC_MAXSLIDER, spreadMax);
	bc3.SetReal(DESC_STEP, Rad(0.1));
	bc3.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc3.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(TMB_SPREAD_VALUE, DTYPE_REAL, 0), bc3, DescLevel(TMB_CONTROLLER_GROUP))) return true;

	BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc4.SetString(DESC_NAME, GeLoadString(H_BEND));
	bc4.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc4.SetReal(DESC_MINSLIDER, bendMin);
	bc4.SetReal(DESC_MAXSLIDER, bendMax);
	bc4.SetReal(DESC_STEP, Rad(0.1));
	bc4.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc4.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(TMB_BEND_VALUE, DTYPE_REAL, 0), bc4, DescLevel(TMB_CONTROLLER_GROUP))) return true;

	BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_REAL);
	bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
	bc5.SetString(DESC_NAME, GeLoadString(H_CURL));
	bc5.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
	bc5.SetReal(DESC_MINSLIDER, curlMin);
	bc5.SetReal(DESC_MAXSLIDER, curlMax);
	bc5.SetReal(DESC_STEP, Rad(0.1));
	bc5.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
	bc5.SetBool(DESC_REMOVEABLE,false);
	if(!description->SetParameter(DescLevel(TMB_CURL_VALUE, DTYPE_REAL, 0), bc5, DescLevel(TMB_CONTROLLER_GROUP))) return true;

	BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_BOOL);
	bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
	bc6.SetString(DESC_NAME, GeLoadString(H_BND_N_CRL));
	bc6.SetBool(DESC_DEFAULT, false);
	if(!description->SetParameter(DescLevel(TMB_BEND_N_CURL, DTYPE_BOOL, 0), bc6, DescLevel(TMB_CONTROLLER_GROUP))) return true;

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDThumbPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case TMB_IK_POLE_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_IK_SOLVER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_CURL_VALUE:	
			if(tData->GetBool(TMB_BEND_N_CURL)) return false;
			else return true;
		case TMB_GRIP_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_GRIP_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_TWIST_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_TWIST_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_SPREAD_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_SPREAD_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_BEND_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_BEND_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_CURL_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case TMB_CURL_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDThumbPlugin(void)
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
	String name=GeLoadString(IDS_CDTHUMB); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDTHUMBPLUGIN,name,info,CDThumbPlugin::Alloc,"tCDThumb","CDthumb.tif",1);
}

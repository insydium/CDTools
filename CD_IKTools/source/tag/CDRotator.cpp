//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDRotator.h"
#include "CDIKtools.h"

enum
{	
	//RTR_PURCHASE					= 1000,
	
	//RTR_LINE_COLOR					= 1500,
	RTR_DISK_LEVEL					= 1501,

	//RTR_SNAP_ON						= 10010,
	
	//RTR_SNAP_TO						= 10011,
	
	//RTR_LOCK_CONTROLLER				= 10012,

	//RTR_LOCK_BONE					= 10013,

	//RTR_SHOW_LINES				= 10014,

	//RTR_LINE_TARGET				= 10015,
		//RTR_LINE_ROOT			= 10016,
		//RTR_LINE_TIP				= 10017,
		
	//RTR_BONES_IN_GROUP				= 10018,
	
	//RTR_USE_BIAS_CURVE				= 10020,
	//RTR_BIAS_CURVE					= 10021,
	//RTR_CURVE_SCALE					= 10022,

	//RTR_CONTROLLER_LINK				= 10030,
		
	//RTR_CONTROLLER_GROUP		= 20000,
	//RTR_CONNECT_BONES,
	//RTR_CONNECT_NEXT,
};

class CDRotatorPlugin : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	LONG ConfirmJointCount(BaseObject *jnt, BaseContainer *tData);
	Real GetJointChainLength(BaseObject *jnt, LONG jCnt);
	void InitSplineData(BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDRotatorPlugin); }
};

Bool CDRotatorPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetLong(RTR_LINE_TARGET,RTR_LINE_ROOT);
	tData->SetLong(RTR_SNAP_TO,RTR_LOCK_BONE);
	tData->SetBool(RTR_SHOW_LINES,true);
	tData->SetBool(RTR_USE_BIAS_CURVE,false);
	tData->SetBool(RTR_CONNECT_BONES,false);
	tData->SetVector(RTR_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(RTR_CURVE_SCALE,1.0);

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

void CDRotatorPlugin::InitSplineData(BaseContainer *tData)
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
	tData->SetData(RTR_BIAS_CURVE, spd);
}

Bool CDRotatorPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(RTR_DISK_LEVEL,level);
	
	return true;
}

Bool CDRotatorPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(RTR_SHOW_LINES)) return true;

	BaseDocument *doc = bh->GetDocument();

	BaseObject *cntr = tData->GetObjectLink(RTR_CONTROLLER_LINK,doc); if(!cntr) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());

	Vector cntlrPosition = cntr->GetMg().off;
	Vector opPosition = op->GetMg().off;
	Real aLen = 0.0;
	if(CDIsBone(op)) aLen = GetBoneLength(op);
	else
	{
		if(op->GetDown()) aLen = Len(op->GetDown()->GetMg().off - opPosition);
	}
	Vector tipAPosition = opPosition + op->GetMg().v3 * aLen;

	bd->SetPen(tData->GetVector(RTR_LINE_COLOR));
	switch (tData->GetLong(RTR_LINE_TARGET))
	{
		case RTR_LINE_ROOT:	
			CDDrawLine(bd,cntlrPosition, opPosition);
			break;
		case RTR_LINE_TIP:
			CDDrawLine(bd,cntlrPosition, tipAPosition);
			break;
		default:
		break;
	}
	
	BaseObject *bn = op;
	BaseObject *ch = bn->GetDown();
	LONG boneCount = 1;
 	while (ch &&  boneCount < tData->GetLong(RTR_BONES_IN_GROUP))
 	{
		CDDrawLine(bd,bn->GetMg().off, ch->GetMg().off);
		bn = ch;
		ch = ch->GetDown();
		if(ch) boneCount++;
 	}
	if(CDIsBone(bn))
	{
		Real bnLen = GetBoneLength(bn);
		Vector boneTip = bn->GetMg().off + bn->GetMg().v3 * bnLen;
		CDDrawLine(bd,bn->GetMg().off, boneTip);
	}
	
	return true;
}

void CDRotatorPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLink(RTR_CONTROLLER_LINK+T_LST,tData->GetLink(RTR_CONTROLLER_LINK,doc));
			
			LONG i, jcnt = tData->GetReal(RTR_BONES_IN_GROUP);
			tData->SetReal(RTR_BONES_IN_GROUP+T_LST,jcnt);
			
			BaseObject *ch = op->GetDown();
			for(i=0; i<jcnt; i++)
			{
				if(ch)
				{
					Vector chPos = CDGetPos(ch);
					tData->SetVector(T_CP+i,chPos);
					ch = ch->GetDown();
				}
			}
			
			tData->SetBool(RTR_USE_BIAS_CURVE+T_LST,tData->GetBool(RTR_USE_BIAS_CURVE));
			tData->SetReal(RTR_CURVE_SCALE+T_LST,tData->GetReal(RTR_CURVE_SCALE));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDRotatorPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(RTR_CONTROLLER_LINK,tData->GetLink(RTR_CONTROLLER_LINK+T_LST,doc));
		
		LONG i, jcnt = tData->GetReal(RTR_BONES_IN_GROUP+T_LST);
		tData->SetReal(RTR_BONES_IN_GROUP,jcnt);
		
		BaseObject *ch = op->GetDown();
		for(i=0; i<jcnt; i++)
		{
			if(ch)
			{
				CDSetPos(ch,tData->GetVector(T_CP+i));
				ch = ch->GetDown();
			}
		}
		
		tData->SetBool(RTR_USE_BIAS_CURVE,tData->GetBool(RTR_USE_BIAS_CURVE+T_LST));
		tData->SetReal(RTR_CURVE_SCALE,tData->GetReal(RTR_CURVE_SCALE+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDRotatorPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(RTR_DISK_LEVEL) < 1) InitSplineData(tData);
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
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==RTR_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

LONG CDRotatorPlugin::ConfirmJointCount(BaseObject *jnt, BaseContainer *tData)
{
	LONG jCnt = 0;
	
	while(jnt &&  jCnt < tData->GetLong(RTR_BONES_IN_GROUP))
	{
		jnt = jnt->GetDown();
		if(jnt) jCnt++;
	}
	tData->SetLong(RTR_BONES_IN_GROUP, jCnt);
	
	return jCnt;
}

Real CDRotatorPlugin::GetJointChainLength(BaseObject *jnt, LONG jCnt)
{
	Real len = 0.0;
	LONG i;
	
	for(i=0; i<jCnt; i++)
	{
		jnt = jnt->GetDown();
		len += Len(CDGetPos(jnt));
	}
	
	return len;
}

LONG CDRotatorPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseDraw   *bd = doc->GetRenderBaseDraw(); if(!bd) return false;
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseObject *cntr = tData->GetObjectLink(RTR_CONTROLLER_LINK,doc); if(!cntr) return false;
	
	Matrix opM = op->GetMg(), cntrM = cntr->GetMg(), transM;
	Vector theScale, rotSet, opPos, chPos, newRot, oldRot = CDGetRot(op);
	
	Bool lockBone = false;
	if(tData->GetBool(RTR_SNAP_ON))
	{
		switch(tData->GetLong(RTR_SNAP_TO))
		{
			case RTR_LOCK_CONTROLLER:
				if(cntr->GetUp())
				{
					Matrix prM = cntr->GetUp()->GetMg();
					CDSetPos(cntr,MInv(prM) * opM.off);
				}
				else CDSetPos(cntr,opM.off);
				break;
			case RTR_LOCK_BONE:
				if(op->GetUp())
				{
					Matrix prM = op->GetUp()->GetMg();
					CDSetPos(op,MInv(prM) * cntrM.off);
				}
				else CDSetPos(op,cntrM.off);
				lockBone = true;
				break;
			default:
			break;
		}
	}
	
	opPos = CDGetPos(op);
	theScale = CDGetScale(op);		
			
	transM = Matrix(opM.off,cntrM.v1,cntrM.v2,cntrM.v3);
	op->SetMg(transM);
	SetGlobalRotation(op,GetGlobalRotation(cntr));
	
	CDSetScale(op,theScale);
	if(!lockBone) CDSetPos(op,opPos);	
	
	SplineData *spl = (SplineData*)tData->GetCustomDataType(RTR_BIAS_CURVE,CUSTOMDATATYPE_SPLINE);
	
	newRot = CDGetRot(op);
	rotSet = CDGetOptimalAngle(oldRot,newRot,op);
	CDSetRot(op,rotSet);	
	
	LONG i, jCnt = ConfirmJointCount(op,tData);
	Real t, crvSca = tData->GetReal(RTR_CURVE_SCALE), jLen = 0.0, chnLen = GetJointChainLength(op,jCnt);
	
	BaseObject *jnt = op;
	for(i=0; i<jCnt; i++)
	{
		if(i > 0)
		{
			jnt = jnt->GetDown();
			if(jnt)
			{
				chPos = CDGetPos(jnt);
				if(tData->GetBool(RTR_CONNECT_BONES))
				{
					chPos.x = 0.0;
					chPos.y = 0.0;
					if(CDIsBone(jnt->GetUp())) chPos.z = GetBoneLength(jnt->GetUp());
					CDSetPos(jnt,chPos);
				}
				
				if(spl && tData->GetBool(RTR_USE_BIAS_CURVE) && chnLen > 0.0)
				{
					jLen += Len(chPos);
					t = CDClamp(0.0,1.0,jLen/chnLen);
					CDSetRot(jnt,rotSet * ((spl->GetPoint(t).y - 0.5) * crvSca));
				}
				else CDSetRot(jnt,rotSet);
				
			}
		}
	}
	
	if(jnt->GetDown())
	{
		if(tData->GetBool(RTR_CONNECT_NEXT))
		{
			jnt = jnt->GetDown();
			chPos = CDGetPos(jnt);
			chPos.x = 0.0;
			chPos.y = 0.0;
			if(CDIsBone(jnt->GetUp())) chPos.z = GetBoneLength(jnt->GetUp());
			CDSetPos(jnt,chPos);
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDRotatorPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(RTR_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDRotatorPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case RTR_CONTROLLER_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTR_BONES_IN_GROUP:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTR_SNAP_TO:	
			return tData->GetBool(RTR_SNAP_ON);
		case RTR_LINE_TARGET:	
			return tData->GetBool(RTR_SHOW_LINES);
		case RTR_USE_BIAS_CURVE:
			if(!tData->GetBool(T_REG)) return false;	
			else return true;
		case RTR_BIAS_CURVE:
			return tData->GetBool(RTR_USE_BIAS_CURVE);
		case RTR_CURVE_SCALE:
			return tData->GetBool(RTR_USE_BIAS_CURVE);
	}
	return true;
}


Bool RegisterCDRotatorPlugin(void)
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
	String name=GeLoadString(IDS_CDROTATOR); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDROTATORPLUGIN,name,info,CDRotatorPlugin::Alloc,"tCDRotator","CDrotator.tif",1);
}

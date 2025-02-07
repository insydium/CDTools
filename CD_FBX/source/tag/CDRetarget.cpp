//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDFBX.h"
#include "CDCompatibility.h"
#include "tCDRetarget.h"
#include "CDTagData.h"

#define MAX_SOURCE		50
#define MAX_JOINTS		300

enum
{
	//RTRG_PURCHASE					= 1000,
	
	//RTRG_ID_SOURCE_TARGET_GROUP		= 1200,
	//RTRG_ID_JOINT_TARGET_GROUP		= 1201,
	//RTRG_ID_BIND_POSE_GROUP			= 1202,
	
	//RTRG_SHOW_LINES					= 1500,
	//RTRG_LINE_COLOR					= 1501,
	
	//RTRG_ROOT_POSITION_ONLY			= 1502,
	//RTRG_MANUAL_JOINT_ASSIGNMENT		= 1503,
	
	//RTRG_ADD_SRC						= 1504,
	//RTRG_SUB_SRC						= 1505,
	
	//RTRG_ROOT_OFFSET					= 1506,
	//RTRG_ROOT_SCALE					= 1507,
	
	//RTRG_BIND_NAME					= 1520,
	//RTRG_BIND_SET					= 1521,
	//RTRG_BIND_EDIT					= 1522,
	//RTRG_BIND_RESTORE				= 1523,
	RTRG_BIND_POSE_SET				= 1524,
	
	RTRG_ROOT_JOINT_LINK				= 1530,
	RTRG_ROOT_BIND_MATRIX			= 1531,
	
	RTRG_SOURCE_GROUP				= 1600,
	RTRG_OFFSET_GROUP				= 1700,
	
	RTRG_SOURCE_COUNT				= 1800,
	RTRG_SOURCE_TYPE					= 1801,
	
	RTRG_OBJECT_TYPE					= 1802,
	RTRG_JOINT_COUNT					= 1803,
	
	//RTRG_RETARGET					= 2000,
		//RTRG_RETARGET_OFF			= 2001,
		//RTRG_RETARGET_SOURCE		= 2002,
	
	//RTRG_ROOT_SOURCE_LINK				= 3000,
	RTRG_ROOT_SOURCE_SCALE				= 3200,
	RTRG_ROOT_SOURCE_OFFSET				= 3300,
	RTRG_ROOT_SOURCE_MANUAL				= 3400,
	
	//RTRG_JOINT_LINK					= 4000,
	RTRG_JOINT_BIND_MATRIX				= 4500,
	//RTRG_JOINT_TWIST					= 5000,
	
	//RTRG_SOURCE_LINK					= 6000,
};

class CDRetargetTag : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	void FillJointLinks(BaseContainer *tData, BaseObject *op);
	Bool RootObjectChanged(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool SetChildren(BaseTag *tag, BaseContainer *tData, BaseObject *src, BaseObject *dst, Vector offset, Bool rootOnly);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDRetargetTag); }
};

Bool CDRetargetTag::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(RTRG_SHOW_LINES,false);
	tData->SetVector(RTRG_LINE_COLOR, Vector(1,0,0));
	
	tData->SetLong(RTRG_RETARGET,RTRG_RETARGET_OFF);
	
	tData->SetBool(RTRG_ROOT_POSITION_ONLY,true);
	tData->SetBool(RTRG_MANUAL_JOINT_ASSIGNMENT,false);

	tData->SetBool(RTRG_BIND_POSE_SET,false);
	
	tData->SetLong(RTRG_JOINT_COUNT,0);
	
	tData->SetLong(RTRG_SOURCE_COUNT,1);
	tData->SetReal(RTRG_ROOT_SOURCE_SCALE,1.0);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDRetargetTag::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	
	if(tData->GetLong(RTRG_RETARGET) == RTRG_RETARGET_OFF) return true;

	LONG sInd = tData->GetLong(RTRG_RETARGET) - RTRG_RETARGET_SOURCE;
	BaseObject *src = tData->GetObjectLink(RTRG_ROOT_SOURCE_LINK+sInd,doc);

	if(!tData->GetBool(RTRG_SHOW_LINES))
	{
		return true;
	}
	else
	{
		CDSetBaseDrawMatrix(bd, NULL, Matrix());
		
		bd->SetPen(tData->GetVector(RTRG_LINE_COLOR));
		if(src)CDDrawLine(bd,op->GetMg().off, src->GetMg().off);
	}
	
	return true;
}

void CDRetargetTag::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetBool(RTRG_ROOT_POSITION_ONLY+T_LST,tData->GetBool(RTRG_ROOT_POSITION_ONLY));
			
			tData->SetLink(RTRG_ROOT_SOURCE_LINK+T_LST,tData->GetLink(RTRG_ROOT_SOURCE_LINK,doc));
			tData->SetReal(RTRG_ROOT_SOURCE_SCALE+T_LST,tData->GetReal(RTRG_ROOT_SOURCE_SCALE));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDRetargetTag::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetBool(RTRG_ROOT_POSITION_ONLY,tData->GetBool(RTRG_ROOT_POSITION_ONLY+T_LST));
		
		tData->SetLink(RTRG_ROOT_SOURCE_LINK,tData->GetLink(RTRG_ROOT_SOURCE_LINK+T_LST,doc));
		tData->SetReal(RTRG_ROOT_SOURCE_SCALE,tData->GetReal(RTRG_ROOT_SOURCE_SCALE+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDRetargetTag::FillJointLinks(BaseContainer *tData, BaseObject *op)
{
	while(op)
	{
		if(op->GetType() == tData->GetLong(RTRG_OBJECT_TYPE))
		{
			LONG cnt = tData->GetLong(RTRG_JOINT_COUNT);
			tData->SetLink(RTRG_JOINT_LINK+cnt, op);
			tData->SetMatrix(RTRG_JOINT_BIND_MATRIX+cnt,op->GetMl());
			cnt++;
			tData->SetLong(RTRG_JOINT_COUNT,cnt);
		}
		
		FillJointLinks(tData,op->GetDown());
		
		op = op->GetNext();
	}
}

Bool CDRetargetTag::Message(GeListNode *node, LONG type, void *data)
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
			CHAR snData[CDFBX_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDFBXPLUGIN,snData,CDFBX_SERIAL_SIZE);
			cdknr.SetCString(snData,CDFBX_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDFBX_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDFBXPLUGIN,snData,CDFBX_SERIAL_SIZE);
			cdknr.SetCString(snData,CDFBX_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	LONG sInd = tData->GetLong(RTRG_RETARGET) - RTRG_RETARGET_SOURCE;
	switch (type)
	{
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*)data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == RTRG_RETARGET)
			{
				tData->SetVector(RTRG_ROOT_OFFSET,tData->GetVector(RTRG_ROOT_SOURCE_OFFSET+sInd));
				tData->SetBool(RTRG_MANUAL_JOINT_ASSIGNMENT,tData->GetBool(RTRG_ROOT_SOURCE_MANUAL+sInd));
				tData->SetReal(RTRG_ROOT_SCALE,tData->GetReal(RTRG_ROOT_SOURCE_SCALE+sInd));
			}
			else if(descID[0].id  == RTRG_ROOT_OFFSET)
			{
				if(sInd >= 0) tData->SetVector(RTRG_ROOT_SOURCE_OFFSET+sInd,tData->GetVector(RTRG_ROOT_OFFSET));
			}
			else if(descID[0].id  == RTRG_MANUAL_JOINT_ASSIGNMENT)
			{
				if(sInd >= 0) tData->SetBool(RTRG_ROOT_SOURCE_MANUAL+sInd,tData->GetBool(RTRG_MANUAL_JOINT_ASSIGNMENT));
			}
			else if(descID[0].id  == RTRG_ROOT_SCALE)
			{
				if(sInd >= 0) tData->SetReal(RTRG_ROOT_SOURCE_SCALE+sInd,tData->GetReal(RTRG_ROOT_SCALE));
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==RTRG_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==RTRG_BIND_SET)
			{
				tData->SetLink(RTRG_ROOT_JOINT_LINK, op);
				tData->SetLong(RTRG_OBJECT_TYPE,op->GetType());
				tData->SetMatrix(RTRG_ROOT_BIND_MATRIX,op->GetMl());
				
				FillJointLinks(tData,op->GetDown());
				
				tData->SetBool(RTRG_BIND_POSE_SET,true);
			}
			else if(dc->id[0].id==RTRG_BIND_EDIT)
			{
				if(tData->GetBool(RTRG_BIND_POSE_SET))
				{
					Matrix opM = tData->GetMatrix(RTRG_ROOT_BIND_MATRIX);
					op->SetMl(opM);
					
					LONG i, jCnt = tData->GetLong(RTRG_JOINT_COUNT);
					for(i=0; i<jCnt; i++)
					{
						BaseObject *jnt = tData->GetObjectLink(RTRG_JOINT_LINK+i,doc);
						Matrix jM = tData->GetMatrix(RTRG_JOINT_BIND_MATRIX+i);
						jnt->SetMl(jM);
					}
					tData->SetLong(RTRG_JOINT_COUNT,0);
				}
				tData->SetBool(RTRG_BIND_POSE_SET,false);
			}
			else if(dc->id[0].id==RTRG_BIND_RESTORE)
			{
				if(tData->GetBool(RTRG_BIND_POSE_SET))
				{
					Matrix opM = tData->GetMatrix(RTRG_ROOT_BIND_MATRIX);
					op->SetMl(opM);
					
					LONG i, jCnt = tData->GetLong(RTRG_JOINT_COUNT);
					for(i=0; i<jCnt; i++)
					{
						BaseObject *jnt = tData->GetObjectLink(RTRG_JOINT_LINK+i,doc);
						Matrix jM = tData->GetMatrix(RTRG_JOINT_BIND_MATRIX+i);
						jnt->SetMl(jM);
					}
				}
			}
			else if(dc->id[0].id==RTRG_ADD_SRC)
			{
				LONG sCnt = tData->GetLong(RTRG_SOURCE_COUNT);
				if(sCnt < MAX_SOURCE)
				{
					tData->SetVector(RTRG_ROOT_SOURCE_OFFSET+sCnt,Vector(0.0,0.0,0.0));
					tData->SetBool(RTRG_ROOT_SOURCE_MANUAL,false);
					tData->SetReal(RTRG_ROOT_SOURCE_SCALE+sCnt,1.0);
					sCnt++;
					tData->SetLong(RTRG_SOURCE_COUNT,sCnt);
				}
			}
			else if(dc->id[0].id==RTRG_SUB_SRC)
			{
				LONG sCnt = tData->GetLong(RTRG_SOURCE_COUNT);
				if(sCnt > 1)
				{
					tData->SetVector(RTRG_ROOT_SOURCE_OFFSET+sCnt,Vector(0.0,0.0,0.0));
					tData->SetBool(RTRG_ROOT_SOURCE_MANUAL,false);
					tData->SetReal(RTRG_ROOT_SOURCE_SCALE+sCnt,1.0);
					sCnt--;
					tData->SetLong(RTRG_SOURCE_COUNT,sCnt);
				}
			}
			break;
		}
	}

	return true;
}

Bool CDRetargetTag::RootObjectChanged(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
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
	
	if(tagMoved || tData->GetBool(T_MOV)) return true;
	
	return false;
}

Bool CDRetargetTag::SetChildren(BaseTag *tag, BaseContainer *tData, BaseObject *src, BaseObject *dst, Vector offset, Bool rootOnly)
{
	BaseObject *pr = tag->GetObject();
	
	while(src && dst)
	{
		if(dst->GetUp() == pr)
		{
			if(!VEqual(offset,Vector(0.0,0.0,0.0),0.001))
			{
				BaseObject *sPr = src->GetUp();
				Vector oldRot = CDGetRot(dst);
				
				Matrix prM = sPr->GetMg() * CDHPBToMatrix(offset, sPr);
				Vector newRot = CDMatrixToHPB(MInv(prM) * src->GetMg(), src);
				CDSetRot(dst,CDGetOptimalAngle(oldRot,newRot, dst));
				
				if(!rootOnly) CDSetPos(dst,MInv(prM) * src->GetMg().off);
			}
			else
			{
				CDSetRot(dst,CDGetRot(src));
				if(!rootOnly) CDSetPos(dst,CDGetPos(src));
			}
		}
		else
		{
			CDSetRot(dst,CDGetRot(src));
			if(!rootOnly) CDSetPos(dst,CDGetPos(src));
		}
		
		SetChildren(tag,tData,src->GetDown(),dst->GetDown(),offset,rootOnly);
		
		src = src->GetNext(); if(!src) return true;
		dst = dst->GetNext();
	}
	
	return true;
}

LONG CDRetargetTag::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	if(RootObjectChanged(doc,op,tData)) tData->SetBool(RTRG_BIND_POSE_SET,false);
	
	if(!tData->GetBool(RTRG_BIND_POSE_SET)) return false;
	
	if(tData->GetLong(RTRG_RETARGET) == RTRG_RETARGET_OFF) return false;
	LONG sInd = tData->GetLong(RTRG_RETARGET) - RTRG_RETARGET_SOURCE;
	
	BaseObject *src = tData->GetObjectLink(RTRG_ROOT_SOURCE_LINK+sInd,doc);
	if(!src) return false;
	
	Bool rootOnly = tData->GetBool(RTRG_ROOT_POSITION_ONLY);
	Real scale = tData->GetReal(RTRG_ROOT_SOURCE_SCALE+sInd);
	
	Vector offset = tData->GetVector(RTRG_ROOT_SOURCE_OFFSET+sInd);
	Matrix prM = src->GetMg() * CDHPBToMatrix(offset, src);
	
	Vector opScale = CDGetScale(op);
	if(!VEqual(offset,Vector(0.0,0.0,0.0),0.001))
	{
		Vector oldRot = CDGetRot(op);
		op->SetMg(prM);
		Vector newRot = CDGetRot(op);
		CDSetRot(op,CDGetOptimalAngle(oldRot,newRot,op));
	}
	else CDSetRot(op,CDGetRot(src));
	CDSetPos(op,CDGetPos(src) * scale);
	CDSetScale(op,opScale);
	
	if(!tData->GetBool(RTRG_MANUAL_JOINT_ASSIGNMENT))
	{
		SetChildren(tag,tData,src->GetDown(),op->GetDown(),offset,rootOnly);
	}
	else
	{
		LONG i, jCnt = tData->GetLong(RTRG_JOINT_COUNT);
		LONG ind = sInd * 1000;
		for(i=0; i<jCnt; i++)
		{
			BaseObject *sJnt = tData->GetObjectLink(RTRG_SOURCE_LINK+ind+i,doc);
			if(sJnt)
			{
				BaseObject *jnt = tData->GetObjectLink(RTRG_JOINT_LINK+i,doc);
				if(jnt)
				{
					Real twist = tData->GetReal(RTRG_JOINT_TWIST+ind+i);
					Matrix rotM = CDHPBToMatrix(Vector(0,0,twist),jnt);
					
					Vector jntScale = CDGetScale(jnt);
					Vector jntPos = CDGetPos(jnt);
					Vector oldRot = CDGetRot(jnt);
					
					BaseObject *pr = sJnt->GetUp();
					if(pr == src && !VEqual(offset,Vector(0.0,0.0,0.0),0.001))
					{
						jnt->SetMg(sJnt->GetMg() * rotM);
						CDSetPos(jnt,jntPos);
						
						Vector newRot = CDGetRot(jnt);
						CDSetRot(jnt,CDGetOptimalAngle(oldRot,newRot,jnt));
						
						if(!rootOnly) CDSetPos(jnt,MInv(prM) * sJnt->GetMg().off);
					}
					else
					{
						jnt->SetMg(sJnt->GetMg() * rotM);
						CDSetPos(jnt,jntPos);
						
						Vector newRot = CDGetRot(jnt);
						CDSetRot(jnt,CDGetOptimalAngle(oldRot,newRot,jnt));
						
						if(!rootOnly) CDSetPos(jnt,CDGetPos(sJnt));
					}
					CDSetScale(jnt,jntScale);
				}
			}
		}
	}
		
	return CD_EXECUTION_RESULT_OK;
}

Bool CDRetargetTag::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	BaseDocument *doc = tag->GetDocument(); if(!doc) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(RTRG_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	bc = description->GetParameterI(DescLevel(RTRG_RETARGET), ar);
	
	BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_LONG);
	bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
	if(bc) bc1.SetString(DESC_NAME,bc->GetString(DESC_NAME));
	else bc1.SetString(DESC_NAME,"Retarget");
	
	BaseContainer cycle;
	cycle.SetString(RTRG_RETARGET_OFF,"Off");
	
	LONG i, sCnt = tData->GetLong(RTRG_SOURCE_COUNT);
	for (i=0; i<sCnt; i++)
	{
		BaseObject *src = tData->GetObjectLink(RTRG_ROOT_SOURCE_LINK+i,doc);
		if(src) cycle.SetString(RTRG_RETARGET_SOURCE+i,src->GetName());
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_ROOT_SOURCE_LINK)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_OFF);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(RTRG_ROOT_SOURCE_LINK+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(RTRG_ID_SOURCE_TARGET_GROUP))) return true;
	}
	
	bc1.SetContainer(DESC_CYCLE, cycle);
	if(!description->SetParameter(DescLevel(RTRG_RETARGET, DTYPE_LONG, 0), bc1, DescLevel(ID_TAGPROPERTIES))) return true;
	
	LONG sInd = tData->GetLong(RTRG_RETARGET) - RTRG_RETARGET_SOURCE;
	if(sInd >= 0 && tData->GetBool(RTRG_ROOT_SOURCE_MANUAL+sInd))
	{
		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(RTRG_SOURCE_GROUP, DTYPE_GROUP, 0), subgroup1, DescLevel(RTRG_ID_JOINT_TARGET_GROUP))) return true;
		
		LONG ind = sInd * 1000;
		LONG jCnt = tData->GetLong(RTRG_JOINT_COUNT);
		for(i=0; i<jCnt; i++)
		{
			BaseObject *jnt = tData->GetObjectLink(RTRG_JOINT_LINK+i,doc);
			
			BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
			bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
			bc3.SetString(DESC_NAME,jnt->GetName());
			bc3.SetLong(DESC_ANIMATE,DESC_ANIMATE_OFF);
			bc3.SetBool(DESC_REMOVEABLE,true);
			if (!description->SetParameter(DescLevel(RTRG_SOURCE_LINK+ind+i,DTYPE_BASELISTLINK,0),bc3,DescLevel(RTRG_SOURCE_GROUP))) return true;

			BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
			bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
			bc4.SetString(DESC_NAME, GeLoadString(IDS_JOINT_TWIST)+"."+CDLongToString(i));
			bc4.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
			bc4.SetReal(DESC_STEP,Rad(1.0));
			if(!description->SetParameter(DescLevel(RTRG_JOINT_TWIST+ind+i, DTYPE_REAL, 0), bc4, DescLevel(RTRG_SOURCE_GROUP))) return true;
		}
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDRetargetTag::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case RTRG_ROOT_POSITION_ONLY:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTRG_MANUAL_JOINT_ASSIGNMENT:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTRG_BIND_SET:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(RTRG_BIND_POSE_SET)) return true;
				else return false;
			}
		case RTRG_BIND_EDIT:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(RTRG_BIND_POSE_SET)) return false;
				else return true;
			}
		case RTRG_BIND_RESTORE:	
			if(!tData->GetBool(RTRG_BIND_POSE_SET)) return false;
			else return true;
		case RTRG_ADD_SRC:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTRG_SUB_SRC:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTRG_ROOT_SOURCE_SCALE:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTRG_ROOT_SOURCE_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RTRG_SOURCE_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDRetargetTag(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDFBX_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDFBXPLUGIN,data,CDFBX_SERIAL_SIZE)) reg = false;
	
	cdknr.SetCString(data,CDFBX_SERIAL_SIZE-1);
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
	String name=GeLoadString(IDS_CDRETARGET); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDRETARGETTAG,name,info,CDRetargetTag::Alloc,"tCDRetarget","CDRetarget.tif",0);
}

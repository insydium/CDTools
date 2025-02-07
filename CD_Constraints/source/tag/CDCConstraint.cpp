//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "tCDCConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 10

enum
{
	//CC_PURCHASE			= 1000,
	
	//CC_SHOW_LINES			= 1001,
	//CC_LINE_COLOR			= 1002,
	//CC_BOX_SIZE			= 1003,
	
	//CC_ADD_CLAMP			= 1004,
	//CC_SUB_CLAMP			= 1005,

	//CC_TARGET				= 1010,
	//CC_STRENGTH			= 1020,
	
	//CC_CLAMP_AXIS			= 1030,
		//CC_CLAMP_X			= 1040,
		//CC_CLAMP_Y			= 1050,
		//CC_CLAMP_Z			= 1060,
		//CC_CLAMP_NX		= 1070,
		//CC_CLAMP_NY		= 1080,
		//CC_CLAMP_NZ		= 1090,
		
	
	CC_COUNT			= 1100,
	//CC_OFFSET				= 1110,
	//CC_LOCAL_AXIS			= 1120,
	//CC_SURFACE_CLAMP		= 1130,
	CC_POLY_SURFACE		= 1140,
		
	//CC_LINK_GROUP			= 2010,
	//CC_AXIS_GROUP			= 2020,
	//CC_LINE_ADD			= 2030,
	//CC_ID_TARGET			= 2040,
	
	CC_DISK_LEVEL				= 3000,
	CC_TARGET_COUNT			= 3001,
	
	CC_CONTACT_HIT				= 5000,
	CC_CONTACT_PT				= 5100,
	CC_CONTACT_NORM			= 5200,
	CC_CONTACT_CNST			= 5300,
	
	CC_INTERSECTION			= 5400
};

class CDCConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

	LONG ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData);
	void SetDefaultTargetParameters(BaseContainer *tData, LONG n);
	void CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst);

	Bool PointInBounds(Vector rad, Vector pt);
	
	// debug drawing variables
	Vector nrstPt, rStart, rDir, rLen;
	LONG plyInd;
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDCConstraintPlugin); }
};

Bool CDCConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(CC_SHOW_LINES,true);
	tData->SetVector(CC_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(CC_BOX_SIZE,10.0);

	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	tData->SetLong(CC_TARGET_COUNT,1);
	
	LONG i;
	for (i=0; i<6; i++)	
	{
		tData->SetReal(CC_STRENGTH+i,1.00);
		tData->SetLong(CC_CLAMP_AXIS+i,CC_CLAMP_Y+i);
		tData->SetLong(CC_POLY_SURFACE+i,false);
	}

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDCConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(CC_DISK_LEVEL,level);
	if(level < 1) tData->SetLong(CC_TARGET_COUNT,tData->GetLong(CC_COUNT) + 1);
	
	return true;
}

Bool CDCConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;
	
	BaseObject *goal = NULL;

	LONG i, clCnt = tData->GetLong(CC_TARGET_COUNT);
	
	Vector boxPosition, goalPos, upLt, lowLt, upRt, lowRt;
	Matrix goalM = Matrix();
	Real  theOffset;
	
	if(!tData->GetBool(CC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Real boxSize = tData->GetReal(CC_BOX_SIZE)/2;

	for (i=0; i<clCnt; i++)
	{
		Vector intPt = tData->GetVector(CC_INTERSECTION+i);
		
		goal = tData->GetObjectLink(CC_TARGET+i,doc);
		if(goal && goal != op)
		{
			if(tData->GetBool(CC_LOCAL_AXIS+i))
			{
				goalM = goal->GetMg();
			}
			
			boxPosition = MInv(goalM) * op->GetMg().off;
			goalPos = MInv(goalM) * goal->GetMg().off;
			theOffset = tData->GetReal(CC_OFFSET+i);
			if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_X+i)
			{
				if(tData->GetBool(CC_SURFACE_CLAMP+i)) boxPosition.x = intPt.x + theOffset;
				else boxPosition.x = goalPos.x + theOffset;
				// Get the upper left corner position
				upLt.x = boxPosition.x; upLt.y = boxPosition.y + boxSize; upLt.z = boxPosition.z - boxSize;
				// Get the lower left corner position
				lowLt.x = boxPosition.x; lowLt.y = boxPosition.y - boxSize; lowLt.z = boxPosition.z - boxSize;
				// Get the upper right corner position
				upRt.x = boxPosition.x; upRt.y = boxPosition.y + boxSize; upRt.z = boxPosition.z + boxSize;
				// Get the lower right corner position
				lowRt.x = boxPosition.x; lowRt.y = boxPosition.y - boxSize; lowRt.z = boxPosition.z + boxSize;
			}
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_Y+i)
			{
				if(tData->GetBool(CC_SURFACE_CLAMP+i)) boxPosition.y = intPt.y + theOffset;
				else boxPosition.y = goalPos.y + theOffset;
				// Get the upper left corner position
				upLt.x = boxPosition.x + boxSize; upLt.y = boxPosition.y; upLt.z = boxPosition.z - boxSize;
				// Get the lower left corner position
				lowLt.x = boxPosition.x - boxSize; lowLt.y = boxPosition.y; lowLt.z = boxPosition.z - boxSize;
				// Get the upper right corner position
				upRt.x = boxPosition.x + boxSize; upRt.y = boxPosition.y; upRt.z = boxPosition.z + boxSize;
				// Get the lower right corner position
				lowRt.x = boxPosition.x - boxSize; lowRt.y = boxPosition.y; lowRt.z = boxPosition.z + boxSize;
			}
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_Z+i)
			{
				if(tData->GetBool(CC_SURFACE_CLAMP+i)) boxPosition.z = intPt.z + theOffset;
				else boxPosition.z = goalPos.z + theOffset;
				// Get the upper left corner position
				upLt.x = boxPosition.x - boxSize; upLt.y = boxPosition.y + boxSize; upLt.z = boxPosition.z;
				// Get the lower left corner position
				lowLt.x = boxPosition.x - boxSize; lowLt.y = boxPosition.y - boxSize; lowLt.z = boxPosition.z;
				// Get the upper right corner position
				upRt.x = boxPosition.x + boxSize; upRt.y = boxPosition.y + boxSize; upRt.z = boxPosition.z;
				// Get the lower right corner position
				lowRt.x = boxPosition.x + boxSize; lowRt.y = boxPosition.y - boxSize; lowRt.z = boxPosition.z;
			}	
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_NX+i)
			{
				if(tData->GetBool(CC_SURFACE_CLAMP+i)) boxPosition.x = intPt.x + theOffset;
				else boxPosition.x = goalPos.x + theOffset;
				// Get the upper left corner position
				upLt.x = boxPosition.x; upLt.y = boxPosition.y + boxSize; upLt.z = boxPosition.z - boxSize;
				// Get the lower left corner position
				lowLt.x = boxPosition.x; lowLt.y = boxPosition.y - boxSize; lowLt.z = boxPosition.z - boxSize;
				// Get the upper right corner position
				upRt.x = boxPosition.x; upRt.y = boxPosition.y + boxSize; upRt.z = boxPosition.z + boxSize;
				// Get the lower right corner position
				lowRt.x = boxPosition.x; lowRt.y = boxPosition.y - boxSize; lowRt.z = boxPosition.z + boxSize;
			}	
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_NY+i)
			{
				if(tData->GetBool(CC_SURFACE_CLAMP+i)) boxPosition.y = intPt.y + theOffset;
				else boxPosition.y = goalPos.y + theOffset;
				// Get the upper left corner position
				upLt.x = boxPosition.x + boxSize; upLt.y = boxPosition.y; upLt.z = boxPosition.z - boxSize;
				// Get the lower left corner position
				lowLt.x = boxPosition.x - boxSize; lowLt.y = boxPosition.y; lowLt.z = boxPosition.z - boxSize;
				// Get the upper right corner position
				upRt.x = boxPosition.x + boxSize; upRt.y = boxPosition.y; upRt.z = boxPosition.z + boxSize;
				// Get the lower right corner position
				lowRt.x = boxPosition.x - boxSize; lowRt.y = boxPosition.y; lowRt.z = boxPosition.z + boxSize;
			}
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_NZ+i)
			{
				if(tData->GetBool(CC_SURFACE_CLAMP+i)) boxPosition.z = intPt.z + theOffset;
				else boxPosition.z = goalPos.z + theOffset;
				// Get the upper left corner position
				upLt.x = boxPosition.x - boxSize; upLt.y = boxPosition.y + boxSize; upLt.z = boxPosition.z;
				// Get the lower left corner position
				lowLt.x = boxPosition.x - boxSize; lowLt.y = boxPosition.y - boxSize; lowLt.z = boxPosition.z;
				// Get the upper right corner position
				upRt.x = boxPosition.x + boxSize; upRt.y = boxPosition.y + boxSize; upRt.z = boxPosition.z;
				// Get the lower right corner position
				lowRt.x = boxPosition.x + boxSize; lowRt.y = boxPosition.y - boxSize; lowRt.z = boxPosition.z;
			}

			bd->SetPen(tData->GetVector(CC_LINE_COLOR));
			CDDrawLine(bd,op->GetMg().off, goalM * boxPosition);
			CDDrawLine(bd,goalM * upLt, goalM * upRt);
			CDDrawLine(bd,goalM * upRt, goalM * lowRt);
			CDDrawLine(bd,goalM * lowRt, goalM * lowLt);
			CDDrawLine(bd,goalM * lowLt, goalM * upLt);
		}
	}
	
	return true;
}
void CDCConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(CC_TARGET_COUNT+T_LST,tData->GetLong(CC_TARGET_COUNT));
			LONG i, clCnt = tData->GetLong(CC_TARGET_COUNT);
			for(i=0; i<clCnt; i++)
			{
				tData->SetLink(CC_TARGET+i+T_LST,tData->GetLink(CC_TARGET+i,doc));
				tData->SetLong(CC_CLAMP_AXIS+i+T_LST,tData->GetLong(CC_CLAMP_AXIS+i));
				tData->SetReal(CC_OFFSET+i+T_LST,tData->GetReal(CC_OFFSET+i));
				tData->SetBool(CC_LOCAL_AXIS+i+T_LST,tData->GetBool(CC_LOCAL_AXIS+i));
				tData->SetBool(CC_SURFACE_CLAMP+i+T_LST,tData->GetBool(CC_SURFACE_CLAMP+i));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

void CDCConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(CC_TARGET+i,NULL);
	tData->SetReal(CC_STRENGTH+i,1.0);
	tData->SetLong(CC_CLAMP_AXIS+i,CC_CLAMP_Y+i);
	tData->SetReal(CC_OFFSET+i,0.0);
	tData->SetBool(CC_LOCAL_AXIS+i,false);
	tData->SetBool(CC_SURFACE_CLAMP+i,false);
}

void CDCConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(CC_TARGET+dst,tData->GetObjectLink(CC_TARGET+src,doc));
	tData->SetReal(CC_STRENGTH+dst,tData->GetReal(CC_STRENGTH+src));
	tData->SetLong(CC_CLAMP_AXIS+dst,tData->GetLong(CC_CLAMP_AXIS+src)-(src-dst));
	tData->SetReal(CC_OFFSET+dst,tData->GetReal(CC_OFFSET+src));
	tData->SetBool(CC_LOCAL_AXIS+dst,tData->GetBool(CC_LOCAL_AXIS+src));
	tData->SetBool(CC_SURFACE_CLAMP+dst,tData->GetBool(CC_SURFACE_CLAMP+src));
}

Bool CDCConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	BaseDocument *doc = tag->GetDocument();
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
	
	LONG clCnt = ConfirmTargetCount(doc,tData);
	if(clCnt < 1)
	{
		clCnt = 1;
		tData->SetLong(CC_TARGET_COUNT,1);
	}
	
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==CC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==CC_ADD_CLAMP)
			{
				if(tData->GetBool(T_REG))
				{
					if(clCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						
						SetDefaultTargetParameters(tData,clCnt);
						clCnt++;
						tData->SetLong(CC_TARGET_COUNT,clCnt);
					}
				}
			}
			else if(dc->id[0].id==CC_SUB_CLAMP)
			{
				if(tData->GetBool(T_REG))
				{
					if(clCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						
						SetDefaultTargetParameters(tData,clCnt);
						clCnt--;
						tData->SetLong(CC_TARGET_COUNT,clCnt);
					}
				}
			}
			break;
		}
	}

	return true;
}

Bool CDCConstraintPlugin::PointInBounds(Vector rad, Vector pt)
{
	Bool rt = true;
	Vector min, max;
	
	min.x = -rad.x;
	min.y = -rad.y;
	min.z = -rad.z;

	max.x = rad.x;
	max.y = rad.y;
	max.z = rad.z;
	
	if(pt.x < min.x || pt.x > max.x) rt = false;
	if(pt.y < min.y || pt.y > max.y) rt = false;
	if(pt.z < min.z || pt.z > max.z) rt = false;

	return rt;
}

Bool CDCConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(CC_TARGET_COUNT,tData->GetLong(CC_TARGET_COUNT+T_LST));
		LONG i, clCnt = tData->GetLong(CC_TARGET_COUNT);
		for(i=0; i<clCnt; i++)
		{
			tData->SetLink(CC_TARGET+i,tData->GetLink(CC_TARGET+i+T_LST,doc));
			tData->SetLong(CC_CLAMP_AXIS+i,tData->GetLong(CC_CLAMP_AXIS+i+T_LST));
			tData->SetReal(CC_OFFSET+i,tData->GetReal(CC_OFFSET+i+T_LST));
			tData->SetBool(CC_LOCAL_AXIS+i,tData->GetBool(CC_LOCAL_AXIS+i+T_LST));
			tData->SetBool(CC_SURFACE_CLAMP+i,tData->GetBool(CC_SURFACE_CLAMP+i+T_LST));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDCConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, clCnt = tData->GetLong(CC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<clCnt; i++)
	{
		trg = tData->GetObjectLink(CC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < clCnt)
			{
				j++;
				trg = tData->GetObjectLink(CC_TARGET+j,doc);
			}
			if(trg)
			{
				chkCnt++;
				CopyTargetParameters(doc,tData,j,i);
				SetDefaultTargetParameters(tData,j);
			}
		}
		else chkCnt++;
	}
	
	return chkCnt;
}

LONG CDCConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, clCnt = ConfirmTargetCount(doc,tData);
	tData->SetLong(CC_COUNT,clCnt);

	BaseObject *goal = NULL, *trg = NULL, *opDef = NULL;
	
	AutoAlloc<GeRayCollider> rc; if(!rc) return false;
	GeRayColResult res;
	
	Vector opPos, goalPos, mixPos, targPos, radius, offRL, dirRL;
	Vector theScale = CDGetScale(op);
	Vector oldRot, newRot, rotSet;
	Real strength, theOffset, rayLen;
	Matrix opM, goalM, rayM;
	
	Vector gScale = GetGlobalScale(op);

	opM = op->GetMg();
	opPos = opM.off;
	
	Bool doClamp = false;
	for(i=0; i<clCnt; i++)
	{
		CDContact contact;
		Vector intPt;
		
		trg = tData->GetObjectLink(CC_TARGET+i,doc);
		if(trg && trg != op)
		{
			goal = trg;
			if(!GetPolygonalObject(trg))
			{
				tData->SetBool(CC_POLY_SURFACE+i, false);
				tData->SetBool(CC_SURFACE_CLAMP+i, false);
			}
			else
			{
				goal = GetPolygonalObject(trg);
				tData->SetBool(CC_POLY_SURFACE+i, true);
				
				if(tData->GetBool(CC_SURFACE_CLAMP+i))
				{
					opDef = trg->GetDeformCache();
					if(opDef)
					{
						goal = opDef;
					}
					rc->Init(goal, true);
				}
			}
			radius = goal->GetRad();
			
			goalM = goal->GetMg();
			rayM = goalM;
			rayM.off = Vector(0,0,0);
			
			if(tData->GetBool(CC_LOCAL_AXIS+i))
			{
				goalPos = MInv(goalM) * goalM.off;
				mixPos = MInv(goalM) * opPos;
			}
			else
			{
				tData->SetBool(CC_SURFACE_CLAMP+i, false);
				goalPos = goalM.off;
				mixPos = opPos;
			}
			
			theOffset = tData->GetReal(CC_OFFSET+i);
			if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_X+i)
			{
				contact.hit = true;
				contact.normal = VNorm(goalM.v1);
				
				if(!tData->GetBool(CC_LOCAL_AXIS+i))  theOffset *= gScale.x;
				if(tData->GetBool(CC_SURFACE_CLAMP+i))
				{
					offRL = MInv(goalM) * opM.off;
					offRL.x = radius.x + 1000;
					dirRL = Vector(-1,0,0);
					
					rayLen = radius.y+2000;
					if(rc->Intersect(offRL, VNorm(dirRL), radius.x+2000, false))
					{
						if(rc->GetNearestIntersection(&res))
						{
							intPt = res.hitpos;
							goalPos.x = intPt.x;
							contact.normal = VNorm(res.f_normal);
						}
						if(mixPos.x < goalPos.x + theOffset)
						{
							mixPos.x = goalPos.x + theOffset;
							doClamp = true;
						}
						contact.point = mixPos;
						contact.point.x = goalPos.x + theOffset;
					}
					else contact.hit = false;
				}
				else
				{
					if(mixPos.x < goalPos.x + theOffset)
					{
						mixPos.x = goalPos.x + theOffset;
						doClamp = true;
					}
					contact.point = mixPos;
					contact.point.x = goalPos.x + theOffset;
					
					if(!tData->GetBool(CC_LOCAL_AXIS+i)) contact.normal = Vector(1,0,0);
					else contact.normal = VNorm(goalM.v1);
					contact.constant = VDot(contact.normal, (goalM.off + contact.normal * theOffset));
				}
			}
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_Y+i)
			{
				contact.hit = true;
				contact.normal = VNorm(goalM.v2);
				
				if(!tData->GetBool(CC_LOCAL_AXIS+i))  theOffset *= gScale.y;
				if(tData->GetBool(CC_SURFACE_CLAMP+i))
				{
					offRL = MInv(goalM) * opM.off;
					offRL.y = radius.y + 1000;
					dirRL = Vector(0,-1,0);
					
					rayLen = radius.y+2000;
					if(rc->Intersect(offRL, VNorm(dirRL), radius.y+2000, false))
					{
						if(rc->GetNearestIntersection(&res))
						{
							intPt = res.hitpos;
							goalPos.y = intPt.y;
							contact.normal = VNorm(res.f_normal);
						}
						if(mixPos.y < goalPos.y + theOffset)
						{
							mixPos.y = goalPos.y + theOffset;
							doClamp = true;
						}
						contact.point = mixPos;
						contact.point.y = goalPos.y + theOffset;
					}
					else contact.hit = false;
				}
				else
				{
					if(mixPos.y < goalPos.y + theOffset)
					{
						mixPos.y = goalPos.y + theOffset;
						doClamp = true;
					}
					contact.point = mixPos;
					contact.point.y = goalPos.y + theOffset;
					
					if(!tData->GetBool(CC_LOCAL_AXIS+i)) contact.normal = Vector(0,1,0);
					else contact.normal = VNorm(goalM.v2);
					contact.constant = VDot(contact.normal, (goalM.off + contact.normal * theOffset));
				}
			}
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_Z+i)
			{
				contact.hit = true;
				contact.normal = VNorm(goalM.v3);

				if(!tData->GetBool(CC_LOCAL_AXIS+i))  theOffset *= gScale.z;
				if(tData->GetBool(CC_SURFACE_CLAMP+i))
				{
					offRL = MInv(goalM) * opM.off;
					offRL.z = radius.z + 1000;
					dirRL = Vector(0,0,-1);
					
					rayLen = radius.z+2000;
					if(rc->Intersect(offRL, VNorm(dirRL), radius.z+2000, false))
					{
						if(rc->GetNearestIntersection(&res))
						{
							intPt = res.hitpos;
							goalPos.z = intPt.z;
							contact.normal = VNorm(res.f_normal);
						}
						if(mixPos.z < goalPos.z + theOffset)
						{
							mixPos.z = goalPos.z + theOffset;
							doClamp = true;
						}
						contact.point = mixPos;
						contact.point.z = goalPos.z + theOffset;
					}
					else contact.hit = false;
				}
				else
				{
					if(mixPos.z < goalPos.z + theOffset)
					{
						mixPos.z = goalPos.z + theOffset;
						doClamp = true;
					}
					contact.point = mixPos;
					contact.point.z = goalPos.z + theOffset;
					
					if(!tData->GetBool(CC_LOCAL_AXIS+i)) contact.normal = Vector(0,0,1);
					else contact.normal = VNorm(goalM.v3);
					contact.constant = VDot(contact.normal, (goalM.off + contact.normal * theOffset));
				}
			}	
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_NX+i)
			{
				contact.hit = true;
				contact.normal = VNorm(-goalM.v1);

				if(!tData->GetBool(CC_LOCAL_AXIS+i))  theOffset *= gScale.x;
				if(tData->GetBool(CC_SURFACE_CLAMP+i))
				{
					offRL = MInv(goalM) * opM.off;
					offRL.x = -radius.x - 1000;
					dirRL = Vector(1,0,0);
					
					rayLen = radius.x+2000;
					if(rc->Intersect(offRL, VNorm(dirRL), radius.x+2000, false))
					{
						if(rc->GetNearestIntersection(&res))
						{
							intPt = res.hitpos;
							goalPos.x = intPt.x;
							contact.normal = VNorm(res.f_normal);
						}
						if(mixPos.x > goalPos.x + theOffset)
						{
							mixPos.x = goalPos.x + theOffset;
							doClamp = true;
						}
						contact.point = mixPos;
						contact.point.x = goalPos.x + theOffset;
					}
					else contact.hit = false;
				}
				else
				{
					if(mixPos.x > goalPos.x + theOffset)
					{
						mixPos.x = goalPos.x + theOffset;
						doClamp = true;
					}
					contact.point = mixPos;
					contact.point.x = goalPos.x + theOffset;
					
					if(!tData->GetBool(CC_LOCAL_AXIS+i)) contact.normal = Vector(-1,0,0);
					else contact.normal = VNorm(-goalM.v1);
					contact.constant = VDot(contact.normal, (goalM.off + contact.normal * -theOffset));
				}
			}	
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_NY+i)
			{
				contact.hit = true;
				contact.normal = VNorm(-goalM.v2);
				
				if(!tData->GetBool(CC_LOCAL_AXIS+i))  theOffset *= gScale.y;
				if(tData->GetBool(CC_SURFACE_CLAMP+i))
				{
					offRL = MInv(goalM) * opM.off;
					offRL.y = -radius.y - 1000;
					dirRL = Vector(0,1,0);
					
					rayLen = radius.y+2000;
					if(rc->Intersect(offRL, VNorm(dirRL), radius.y+2000, false))
					{
						if(rc->GetNearestIntersection(&res))
						{
							intPt = res.hitpos;
							goalPos.y = intPt.y;
							contact.normal = VNorm(res.f_normal);
						}
						if(mixPos.y > goalPos.y + theOffset)
						{
							mixPos.y = goalPos.y + theOffset;
							doClamp = true;
						}
						contact.point = mixPos;
						contact.point.y = goalPos.y + theOffset;
					}
					else contact.hit = false;
				}
				else
				{
					if(mixPos.y > goalPos.y + theOffset)
					{
						mixPos.y = goalPos.y + theOffset;
						doClamp = true;
					}
					contact.point = mixPos;
					contact.point.y = goalPos.y + theOffset;
					
					if(!tData->GetBool(CC_LOCAL_AXIS+i)) contact.normal = Vector(0,-1,0);
					else contact.normal = VNorm(-goalM.v2);
					contact.constant = VDot(contact.normal, (goalM.off + contact.normal * -theOffset));
				}
			}
			else if(tData->GetLong(CC_CLAMP_AXIS+i) == CC_CLAMP_NZ+i)
			{
				contact.hit = true;
				contact.normal = VNorm(-goalM.v3);
				
				if(!tData->GetBool(CC_LOCAL_AXIS+i))  theOffset *= gScale.z;
				if(tData->GetBool(CC_SURFACE_CLAMP+i))
				{
					offRL = MInv(goalM) * opM.off;
					offRL.z = -radius.z - 1000;
					dirRL = Vector(0,0,1);
					
					rayLen = radius.z+2000;
					if(rc->Intersect(offRL, VNorm(dirRL), radius.z+2000, false))
					{
						if(rc->GetNearestIntersection(&res))
						{
							intPt = res.hitpos;
							goalPos.z = intPt.z;
							contact.normal = VNorm(res.f_normal);
						}
						if(mixPos.z > goalPos.z + theOffset)
						{
							mixPos.z = goalPos.z + theOffset;
							doClamp = true;
						}
						contact.point = mixPos;
						contact.point.z = goalPos.z + theOffset;
					}
					else contact.hit = false;
				}
				else
				{
					if(mixPos.z > goalPos.z + theOffset)
					{
						mixPos.z = goalPos.z + theOffset;
						doClamp = true;
					}
					contact.point = mixPos;
					contact.point.z = goalPos.z + theOffset;
					
					if(!tData->GetBool(CC_LOCAL_AXIS+i)) contact.normal = Vector(0,0,-1);
					else contact.normal = VNorm(-goalM.v3);
					contact.constant = VDot(contact.normal, (goalM.off + contact.normal * -theOffset));
				}
			}
			tData->SetBool(CC_CONTACT_HIT+i,contact.hit);
			tData->SetVector(CC_CONTACT_PT+i,contact.point);
			tData->SetVector(CC_CONTACT_NORM+i,contact.normal);
			tData->SetReal(CC_CONTACT_CNST+i,contact.constant);
			
			tData->SetVector(CC_INTERSECTION+i,intPt);
			
			strength = tData->GetReal(CC_STRENGTH+i);
			if(tData->GetBool(CC_LOCAL_AXIS+i))
			{
				mixPos = goalM * mixPos;
			}
			targPos = CDBlend(opPos, mixPos, strength);
			opPos = targPos;
		}
	}
	if(!op->GetTag(ID_CDSPRCONSTRAINTPLUGIN))
	{
		if(doClamp)
		{
			opM.off = opPos;
			oldRot = CDGetRot(op);
			op->SetMg(opM);
			newRot = CDGetRot(op);
			rotSet = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,rotSet);
			CDSetScale(op,theScale);
		}
	}
	else
	{
		Bool clampOn = true;
		BaseTag *nxt = tag->GetNext();
		while(nxt)
		{
			if(nxt->GetType() == ID_CDSPRCONSTRAINTPLUGIN) clampOn = false;
			nxt = nxt->GetNext();
		}
		if(clampOn)
		{
			opM.off = opPos;
			oldRot = CDGetRot(op);
			op->SetMg(opM);
			newRot = CDGetRot(op);
			rotSet = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,rotSet);
			CDSetScale(op,theScale);
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDCConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, clCnt = tData->GetLong(CC_TARGET_COUNT);
	
	if(!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(CC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	for (i=0; i<clCnt; i++)
	{

		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if(!description->SetParameter(DescLevel(CC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(CC_ID_TARGET))) return false;

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(CC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(CC_ID_TARGET))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(C_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(CC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(CC_LINK_GROUP+i))) return false;
		
		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc5.SetString(DESC_NAME, GeLoadString(C_STRENGTH)+"."+CDLongToString(i));
		bc5.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc5.SetReal(DESC_MIN,0.0);
		bc5.SetReal(DESC_MAX,1.0);
		bc5.SetReal(DESC_STEP,0.01);
		if(!description->SetParameter(DescLevel(CC_STRENGTH+i, DTYPE_REAL, 0), bc5, DescLevel(CC_LINK_GROUP+i))) return true;
		
		BaseContainer subgroup2 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup2.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(CC_AXIS_GROUP+i, DTYPE_GROUP, 0), subgroup2, DescLevel(CC_ID_TARGET))) return true;

		BaseContainer bc7 = GetCustomDataTypeDefault(DTYPE_LONG);
		bc7.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
		bc7.SetString(DESC_NAME, GeLoadString(C_CLAMP_AXIS)+"."+CDLongToString(i));
		BaseContainer cycle;
		cycle.SetString(CC_CLAMP_X+i, GeLoadString(C_CLAMP_X));
		cycle.SetString(CC_CLAMP_Y+i, GeLoadString(C_CLAMP_Y));
		cycle.SetString(CC_CLAMP_Z+i, GeLoadString(C_CLAMP_Z));
		cycle.SetString(CC_CLAMP_NX+i, GeLoadString(C_CLAMP_NX));
		cycle.SetString(CC_CLAMP_NY+i, GeLoadString(C_CLAMP_NY));
		cycle.SetString(CC_CLAMP_NZ+i, GeLoadString(C_CLAMP_NZ));
		bc7.SetContainer(DESC_CYCLE, cycle);
		if(!description->SetParameter(DescLevel(CC_CLAMP_AXIS+i, DTYPE_LONG, 0), bc7, DescLevel(CC_AXIS_GROUP+i))) return true;

		BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc6.SetString(DESC_NAME, GeLoadString(C_OFFSET)+"."+CDLongToString(i));
		bc6.SetLong(DESC_UNIT, DESC_UNIT_METER);
		if(!description->SetParameter(DescLevel(CC_OFFSET+i, DTYPE_REAL, 0), bc6, DescLevel(CC_AXIS_GROUP+i))) return true;

		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc3.SetString(DESC_NAME, GeLoadString(C_LOCAL_AXIS)+"."+CDLongToString(i));
		bc3.SetBool(DESC_DEFAULT, false);
		if(!description->SetParameter(DescLevel(CC_LOCAL_AXIS+i, DTYPE_BOOL, 0), bc3, DescLevel(CC_AXIS_GROUP+i))) return true;

		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc4.SetString(DESC_NAME, GeLoadString(C_SURFACE_CLAMP)+"."+CDLongToString(i));
		bc4.SetBool(DESC_DEFAULT, false);
		if(!description->SetParameter(DescLevel(CC_SURFACE_CLAMP+i, DTYPE_BOOL, 0), bc4, DescLevel(CC_AXIS_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDCConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	LONG i, clCnt = tData->GetLong(CC_TARGET_COUNT);

	switch (id[0].id)
	{
		case CC_ADD_CLAMP:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case CC_SUB_CLAMP:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}

	for(i=0; i<clCnt;i++)
	{
		if(id[0].id == CC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == CC_CLAMP_AXIS+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == CC_OFFSET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == CC_LOCAL_AXIS+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == CC_SURFACE_CLAMP+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(CC_LOCAL_AXIS+i) && tData->GetBool(CC_POLY_SURFACE+i)) return true;
			else return false;
		}
	}
	return true;
}

Bool RegisterCDCConstraintPlugin(void)
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
	String name=GeLoadString(IDS_CDCCONSTRAINT); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDCCONSTRAINTPLUGIN,name,info,CDCConstraintPlugin::Alloc,"tCDCConstraint","CDCConstraint.tif",1);
}

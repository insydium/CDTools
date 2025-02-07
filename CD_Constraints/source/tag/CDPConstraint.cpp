//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDPConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	10

enum
{
	//PC_PURCHASE			= 1000,
	
	//PC_OFFSETS				= 1009,
	
	PC_TARGET_A_OLD			= 1010,
	PC_TARGET_B_OLD			= 1011,
	//PC_AB_MIX				= 1012,
	
	//PC_AXIS_X				= 1013,
	//PC_OFFSET_X			= 1014,
	//PC_AXIS_Y				= 1015,
	//PC_OFFSET_Y			= 1016,
	//PC_AXIS_Z				= 1017,
	//PC_OFFSET_Z			= 1018,
	
	//PC_STRENGTH			= 1019,
	//PC_SHOW_LINES			= 1020,
	//PC_LINE_COLOR			= 1021,
	
	//PC_LOCAL_POS			= 1022,
	//PC_USE_AB_MIX			= 1023,
	//PC_LOCAL_OFFSET			= 1024,

	PC_COUNT			= 1030,
	//PC_ADD_POS				= 1031,
	//PC_SUB_POS				= 1032,
	PC_DISK_LEVEL				= 1033,					
	
	PC_TARGET_COUNT			= 1040,
	
	//PC_ID_TARGET			= 1050,

	//PC_LINK_GROUP			= 2000,
	//PC_LINE_ADD			= 3000,

	//PC_TARGET				= 4000,
	//PC_POS_MIX				= 5000,

	//PC_PNP_GROUP			= 6000,
	//PC_CHK_GROUP			= 6100,
	//PC_SET_GROUP			= 6200,
	//PC_USE_POINT			= 7000,
	//PC_USE_NORMAL			= 8000,
	//PC_P_INDEX				= 9000,
	//PC_SET_P_SEL			= 10000,
	PC_GUIDE_POSITION		= 11000
};

class CDPConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

	void ConvertOldData(BaseDocument *doc, BaseContainer *tData);
	LONG ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData);

	void SetDefaultTargetParameters(BaseContainer *tData, LONG n);
	void CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst);

public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);

	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDPConstraintPlugin); }
};

Bool CDPConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(PC_SHOW_LINES,true);
	tData->SetVector(PC_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(PC_STRENGTH,1.00);
	tData->SetReal(PC_AB_MIX,0.5);
	
	tData->SetBool(PC_AXIS_X,true);
	tData->SetReal(PC_OFFSET_X,0.0);
	tData->SetBool(PC_AXIS_Y,true);
	tData->SetReal(PC_OFFSET_Y,0.0);
	tData->SetBool(PC_AXIS_Z,true);
	tData->SetReal(PC_OFFSET_Z,0.0);

	tData->SetLong(PC_TARGET_COUNT,1);
	tData->SetReal(PC_POS_MIX,1.00);
		
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDPConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(PC_DISK_LEVEL,level);
	
	return true;
}

Bool CDPConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	if (!tData->GetBool(PC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	LONG i, pCnt = tData->GetLong(PC_TARGET_COUNT);
	BaseObject *goal = NULL;
	
	bd->SetPen(tData->GetVector(PC_LINE_COLOR));
	Vector startLn = op->GetMg().off;
	
	for(i=0; i<pCnt; i++)
	{
		goal = tData->GetObjectLink(PC_TARGET+i,doc);
		if(goal)
		{
			Vector endLn = tData->GetVector(PC_GUIDE_POSITION+i);
			CDDrawLine(bd,startLn,endLn);
		}
	}
	
	return true;
}

void CDPConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetBool(PC_LOCAL_POS+T_LST,tData->GetBool(PC_LOCAL_POS));
			
			tData->SetBool(PC_USE_AB_MIX+T_LST,tData->GetBool(PC_USE_AB_MIX));
			
			tData->SetBool(PC_AXIS_X+T_LST,tData->GetBool(PC_AXIS_X));
			tData->SetBool(PC_AXIS_Y+T_LST,tData->GetBool(PC_AXIS_Y));
			tData->SetBool(PC_AXIS_Z+T_LST,tData->GetBool(PC_AXIS_Z));
			tData->SetReal(PC_OFFSET_X+T_LST,tData->GetReal(PC_OFFSET_X));
			tData->SetReal(PC_OFFSET_Y+T_LST,tData->GetReal(PC_OFFSET_Y));
			tData->SetReal(PC_OFFSET_Z+T_LST,tData->GetReal(PC_OFFSET_Z));
			
			tData->SetLong(PC_TARGET_COUNT+T_LST,tData->GetLong(PC_TARGET_COUNT));
			LONG i, pCnt = tData->GetLong(PC_TARGET_COUNT);
			for(i=0; i<pCnt; i++)
			{
				tData->SetLink(PC_TARGET+i+T_LST,tData->GetLink(PC_TARGET+i,doc));
				tData->SetBool(PC_USE_POINT+i+T_LST,tData->GetBool(PC_USE_POINT+i));
				tData->SetBool(PC_USE_NORMAL+i+T_LST,tData->GetBool(PC_USE_NORMAL+i));
				tData->SetLong(PC_P_INDEX+i+T_LST,tData->GetLong(PC_P_INDEX+i));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDPConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetBool(PC_LOCAL_POS,tData->GetBool(PC_LOCAL_POS+T_LST));
		tData->SetBool(PC_USE_AB_MIX,tData->GetBool(PC_USE_AB_MIX+T_LST));
		tData->SetBool(PC_AXIS_X,tData->GetBool(PC_AXIS_X+T_LST));
		tData->SetBool(PC_AXIS_Y,tData->GetBool(PC_AXIS_Y+T_LST));
		tData->SetBool(PC_AXIS_Z,tData->GetBool(PC_AXIS_Z+T_LST));
		tData->SetReal(PC_OFFSET_X,tData->GetReal(PC_OFFSET_X+T_LST));
		tData->SetReal(PC_OFFSET_Y,tData->GetReal(PC_OFFSET_Y+T_LST));
		tData->SetReal(PC_OFFSET_Z,tData->GetReal(PC_OFFSET_Z+T_LST));
		tData->SetLong(PC_TARGET_COUNT,tData->GetLong(PC_TARGET_COUNT+T_LST));
		LONG i, pCnt = tData->GetLong(PC_TARGET_COUNT);
		for(i=0; i<pCnt; i++)
		{
			tData->SetLink(PC_TARGET+i,tData->GetLink(PC_TARGET+i+T_LST,doc));
			tData->SetBool(PC_USE_POINT+i,tData->GetBool(PC_USE_POINT+i+T_LST));
			tData->SetBool(PC_USE_NORMAL+i,tData->GetBool(PC_USE_NORMAL+i+T_LST));
			tData->SetLong(PC_P_INDEX+i,tData->GetLong(PC_P_INDEX+i+T_LST));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDPConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(PC_TARGET+i,NULL);
	tData->SetReal(PC_POS_MIX+i,1.00);
	tData->SetBool(PC_USE_POINT+i,false);
	tData->SetBool(PC_USE_NORMAL+i,false);
	tData->SetLong(PC_P_INDEX+i,0);
}

void CDPConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(PC_TARGET+dst,tData->GetObjectLink(PC_TARGET+src,doc));
	tData->SetReal(PC_POS_MIX+dst,tData->GetReal(PC_POS_MIX+src));
	tData->SetBool(PC_USE_POINT+dst,tData->GetBool(PC_USE_POINT+src));
	tData->SetBool(PC_USE_NORMAL+dst,tData->GetBool(PC_USE_NORMAL+src));
	tData->SetLong(PC_P_INDEX+dst,tData->GetLong(PC_P_INDEX+src));
}

void CDPConstraintPlugin::ConvertOldData(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, oldCnt = 0;
	
	BaseObject *goalA = tData->GetObjectLink(PC_TARGET_A_OLD,doc);
	if(goalA)
	{
		tData->SetLink(PC_TARGET,goalA);
		tData->SetReal(PC_POS_MIX,1.00);
		oldCnt++;
		tData->SetLong(PC_TARGET_COUNT,oldCnt);
	}
	BaseObject *goalB = tData->GetObjectLink(PC_TARGET_B_OLD,doc);
	if(goalB)
	{
		tData->SetLink(PC_TARGET+1,goalB);
		tData->SetReal(PC_POS_MIX,1.0-tData->GetReal(PC_AB_MIX));
		tData->SetReal(PC_POS_MIX+1,tData->GetReal(PC_AB_MIX));
		oldCnt++;
		tData->SetLong(PC_TARGET_COUNT,oldCnt);
		tData->SetBool(PC_USE_AB_MIX,true);
	}
	
	if(oldCnt == 0) tData->SetLong(PC_TARGET_COUNT,tData->GetLong(PC_COUNT) + 1);
	else
	{
		for(i=oldCnt; i<MAX_ADD; i++)
		{
			if(tData->GetObjectLink(PC_TARGET+i,doc)) oldCnt++;
		}
		tData->SetLong(PC_TARGET_COUNT,oldCnt);
	}
}

Bool CDPConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			if(doc && tData->GetLong(PC_DISK_LEVEL) < 1) ConvertOldData(doc,tData);
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
	
	LONG i, pCnt = tData->GetLong(PC_TARGET_COUNT);
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==PC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==PC_ADD_POS)
			{
				if(tData->GetBool(T_REG))
				{
					if(pCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,pCnt);
						pCnt++;
						tData->SetLong(PC_TARGET_COUNT,pCnt);
					}
				}
			}
			else if(dc->id[0].id==PC_SUB_POS)
			{
				if(tData->GetBool(T_REG))
				{
					if(pCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,pCnt);
						pCnt--;
						tData->SetLong(PC_TARGET_COUNT,pCnt);
					}
				}
			}
			if(tData->GetBool(T_REG))
			{
				for(i=0; i<pCnt; i++)
				{
					if(dc->id[0].id==PC_SET_P_SEL+i)
					{
						BaseSelect *bs = NULL;
						BaseObject *goal = tData->GetObjectLink(PC_TARGET+i,doc);
						if(goal)
						{
							if(tData->GetBool(PC_USE_POINT+i))
							{
								bs = ToPoint(goal)->GetPointS();
								if(bs)
								{
									LONG bsPtCount = bs->GetCount();
									if(bsPtCount > 0)
									{
										CDAddUndo(doc,CD_UNDO_CHANGE,tag);
										LONG a,b;
										CDGetRange(bs,0,&a,&b);
										tData->SetLong(PC_P_INDEX+i,a);
									}
								}
							}
							else if(tData->GetBool(PC_USE_NORMAL+i))
							{
								bs = ToPoly(goal)->GetPolygonS();
								if(bs)
								{
									LONG bsPtCount = bs->GetCount();
									if(bsPtCount > 0)
									{
										CDAddUndo(doc,CD_UNDO_CHANGE,tag);
										LONG a,b;
										CDGetRange(bs,0,&a,&b);
										tData->SetLong(PC_P_INDEX+i,a);
									}
								}
							}
						}
					}
				}
			}
			break;
		}
	}

	return true;
}

LONG CDPConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, pCnt = tData->GetLong(PC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<pCnt; i++)
	{
		trg = tData->GetObjectLink(PC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < pCnt)
			{
				j++;
				trg = tData->GetObjectLink(PC_TARGET+j,doc);
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

LONG CDPConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, pCnt = ConfirmTargetCount(doc,tData);
	BaseObject *goal = NULL, *trg = NULL, *opDeform = NULL;

	AutoAlloc<GeRayCollider> rc; if (!rc) return false;
	GeRayColResult res;

	Vector opPos, mixPosition = Vector(0,0,0), theScale = CDGetScale(op);
	Vector gScale = Vector(0,0,0);
	Vector oldRot, newRot, rotSet;
	Matrix opM, goalM, transM;
	
	Bool local = tData->GetBool(PC_LOCAL_POS);
	if(local) opM = op->GetMl();
	else opM = op->GetMg();
	transM = opM;
	
	// Get the total constraint mix
	Bool ABMixAvailable = true;
	Real totalMix = 0.0;
	
	if(pCnt < 1) ABMixAvailable = false;
	
	for(i=0; i<pCnt; i++)
	{
		goal = tData->GetObjectLink(PC_TARGET+i,doc);
		if(goal)
		{
			totalMix += tData->GetReal(PC_POS_MIX+i);
		}
		else ABMixAvailable = false;
	}
	if(totalMix == 0.0) return false;
	
	if(!ABMixAvailable) tData->SetBool(PC_USE_AB_MIX,false);
	
	for(i=0; i<pCnt; i++)
	{
		Real maxVal = 0.0, minVal = 1.0;
		if(pCnt-1 != 0) minVal = 1.0/(Real)(pCnt-1);//Check for division by zero
		
		Real mixVal, value = Abs((minVal*i) - tData->GetReal(PC_AB_MIX));
		if(value > minVal) value = minVal;
		if ((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
		else mixVal = 0.0;
		
		trg = tData->GetObjectLink(PC_TARGET+i,doc);
		if(trg && trg != op)
		{
			goal = trg;
			
			if(!IsValidPointObject(trg)) tData->SetBool(PC_USE_POINT+i,false);
			if(!IsValidPolygonObject(trg)) tData->SetBool(PC_USE_NORMAL+i,false);
			
			if(tData->GetBool(PC_USE_POINT+i) || tData->GetBool(PC_USE_NORMAL+i))
			{
				opDeform = trg->GetDeformCache();
				if(opDeform) goal = opDeform;
			}
			
			gScale += GetGlobalScale(goal) * (tData->GetReal(PC_POS_MIX+i)/totalMix);
			if(local) goalM = goal->GetMl();
			else goalM = goal->GetMg();
			
			if(tData->GetBool(PC_USE_POINT+i))
			{
				LONG ind = tData->GetLong(PC_P_INDEX+i), cnt = ToPoint(goal)->GetPointCount();
				if(ind > cnt-1)
				{
					ind = cnt-1;
					tData->SetLong(PC_P_INDEX+i, ind);
				}
				Vector *padr = GetPointArray(goal);
				if(tData->GetBool(PC_USE_AB_MIX)) mixPosition += goalM * padr[ind] * mixVal;
				else mixPosition += goalM * padr[ind] * (tData->GetReal(PC_POS_MIX+i)/totalMix);
				tData->SetVector(PC_GUIDE_POSITION+i,goalM * padr[ind]);
			}
			else if(tData->GetBool(PC_USE_NORMAL+i))
			{
				LONG ind = tData->GetLong(PC_P_INDEX+i), cnt = ToPoly(goal)->GetPolygonCount();
				if(ind > cnt-1)
				{
					ind = cnt-1;
					tData->SetLong(PC_P_INDEX+i, ind);
				}
				CPolygon *vadr = GetPolygonArray(goal);
				Vector *padr = GetPointArray(goal);
				CPolygon ply = vadr[ind];
				Vector normal = CalcFaceNormal(padr,ply);
				Vector a,b,c,d, center, offRL, dirRL, intPt;
				a = padr[ply.a];
				b = padr[ply.b];
				c = padr[ply.c];
				d = padr[ply.d];
				if(c == d)
				{
					center = (a+b+c)/3;
				}
				else
				{
					center = (a+b+c+d)/4;
				}
				intPt = center;
				rc->Init(goal, true);
				offRL = center + normal*1000;
				dirRL = -normal;
				if(rc->Intersect(offRL, !dirRL, 2000, false))
				{
					for(LONG ri=0; ri<rc->GetIntersectionCount(); ri++)
					{
						rc->GetIntersection(ri, &res);
						if(res.face_id == ind)
						{
							intPt = res.hitpos;
						}
					}
				}
				if(tData->GetBool(PC_USE_AB_MIX)) mixPosition += goalM * intPt * mixVal;
				else mixPosition += goalM * intPt * (tData->GetReal(PC_POS_MIX+i)/totalMix);
				tData->SetVector(PC_GUIDE_POSITION+i,goalM * intPt);
			}
			else
			{
				if(tData->GetBool(PC_USE_AB_MIX)) mixPosition += goalM.off * mixVal;
				else mixPosition += goalM.off * (tData->GetReal(PC_POS_MIX+i)/totalMix);
				tData->SetVector(PC_GUIDE_POSITION+i,goalM.off);
			}
		}
	}
	
	Vector offset = Vector(tData->GetReal(PC_OFFSET_X),tData->GetReal(PC_OFFSET_Y),tData->GetReal(PC_OFFSET_Z));
	if(pCnt > 1) tData->SetBool(PC_LOCAL_OFFSET,false);
	
	Real strength = tData->GetReal(PC_STRENGTH);
	if(tData->GetBool(PC_LOCAL_OFFSET))
	{
		Vector localPos = MInv(goalM) * transM.off;
		opPos = localPos;
		if(tData->GetBool(PC_AXIS_X)) opPos.x = CDBlend(localPos.x, offset.x, strength);
		if(tData->GetBool(PC_AXIS_Y)) opPos.y = CDBlend(localPos.y, offset.y, strength);
		if(tData->GetBool(PC_AXIS_Z)) opPos.z = CDBlend(localPos.z, offset.z, strength);
		transM.off = goalM * opPos;
	}
	else
	{
		mixPosition.x += (offset.x * gScale.x);
		mixPosition.y += (offset.y * gScale.y);
		mixPosition.z += (offset.z * gScale.z);
		
		opPos = transM.off;
		if(tData->GetBool(PC_AXIS_X)) opPos.x = CDBlend(transM.off.x, mixPosition.x, strength);
		if(tData->GetBool(PC_AXIS_Y)) opPos.y = CDBlend(transM.off.y, mixPosition.y, strength);
		if(tData->GetBool(PC_AXIS_Z)) opPos.z = CDBlend(transM.off.z, mixPosition.z, strength);
		transM.off = opPos;
		
	}
	
	oldRot = CDGetRot(op);
	if(!MatrixEqual(transM,opM,0.001))
	{
		if(local) op->SetMl(transM);
		else op->SetMg(transM);
		newRot = CDGetRot(op);
		rotSet = CDGetOptimalAngle(oldRot, newRot, op);
		CDSetRot(op,rotSet);
		CDSetScale(op,theScale);
	}
	//op->Message(MSG_UPDATE);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDPConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, pCnt = tData->GetLong(PC_TARGET_COUNT);
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(PC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	for (i=0; i<pCnt; i++)
	{
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if (!description->SetParameter(DescLevel(PC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(PC_ID_TARGET))) return false;

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(PC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(PC_ID_TARGET))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(PC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(PC_LINK_GROUP+i))) return false;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(PC_POS_MIX+i, DTYPE_REAL, 0), bc3, DescLevel(PC_LINK_GROUP+i))) return true;

		BaseContainer subgroup2 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup2.SetLong(DESC_COLUMNS, 1);
		subgroup2.SetString(DESC_NAME, GeLoadString(IDS_OPTIONS));
		if(!description->SetParameter(DescLevel(PC_PNP_GROUP+i, DTYPE_GROUP, 0), subgroup2, DescLevel(PC_ID_TARGET))) return true;
		
		BaseContainer subgroup3 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup3.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(PC_CHK_GROUP+i, DTYPE_GROUP, 0), subgroup3, DescLevel(PC_PNP_GROUP+i))) return true;
		
		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc4.SetString(DESC_NAME, GeLoadString(IDS_POINT)+"."+CDLongToString(i));
		bc4.SetBool(DESC_DEFAULT, false);
		if (!description->SetParameter(DescLevel(PC_USE_POINT+i, DTYPE_BOOL, 0), bc4, DescLevel(PC_CHK_GROUP+i))) return true;
		
		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc5.SetString(DESC_NAME, GeLoadString(IDS_NORMAL)+"."+CDLongToString(i));
		bc5.SetBool(DESC_DEFAULT, false);
		if (!description->SetParameter(DescLevel(PC_USE_NORMAL+i, DTYPE_BOOL, 0), bc5, DescLevel(PC_CHK_GROUP+i))) return true;
		
		BaseContainer subgroup4 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup4.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(PC_SET_GROUP+i, DTYPE_GROUP, 0), subgroup4, DescLevel(PC_PNP_GROUP+i))) return true;
		
		BaseContainer bc7 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc7.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc7.SetString(DESC_NAME, GeLoadString(IDS_SET_P_SEL));
		if (!description->SetParameter(DescLevel(PC_SET_P_SEL+i, DTYPE_BUTTON, 0), bc7, DescLevel(PC_SET_GROUP+i))) return true;

		BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_LONG);
		bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
		bc6.SetString(DESC_NAME, GeLoadString(IDS_INDEX)+"."+CDLongToString(i));
		bc6.SetLong(DESC_UNIT, DESC_UNIT_LONG);
		bc6.SetReal(DESC_MIN,0);
		bc6.SetReal(DESC_DEFAULT,0);
		if (!description->SetParameter(DescLevel(PC_P_INDEX+i, DTYPE_LONG, 0), bc6, DescLevel(PC_SET_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDPConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	LONG i, pCnt = tData->GetLong(PC_TARGET_COUNT);

	switch (id[0].id)
	{
		case PC_LOCAL_POS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PC_LOCAL_OFFSET:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(pCnt > 1) return false;
				else return true;
			}
		case PC_AXIS_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PC_AXIS_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PC_AXIS_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PC_OFFSET_X:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(PC_AXIS_X);
		case PC_OFFSET_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(PC_AXIS_Y);
		case PC_OFFSET_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(PC_AXIS_Z);
		case PC_USE_AB_MIX:
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				Bool ABMixAvailable = true;
				if(pCnt < 2) ABMixAvailable = false;
				for(i=0; i<pCnt; i++)
				{
					BaseObject *goal = tData->GetObjectLink(PC_TARGET+i,doc);
					if(!goal) ABMixAvailable = false;
				}
				return ABMixAvailable;
			}
		}
		case PC_AB_MIX:
			return tData->GetBool(PC_USE_AB_MIX);
		case PC_ADD_POS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PC_SUB_POS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	for(i=0; i<pCnt;i++)
	{
		if(id[0].id == PC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		BaseObject *goal = tData->GetObjectLink(PC_TARGET+i,doc);
		if(goal)
		{
			if(id[0].id == PC_USE_POINT+i)
			{
				if(!tData->GetBool(T_REG)) return false;
				else if(IsValidPointObject(goal) && !tData->GetBool(PC_USE_NORMAL+i)) return true;
				else return false;
			}
			else if(id[0].id == PC_USE_NORMAL+i)
			{
				if(!tData->GetBool(T_REG)) return false;
				else if(IsValidPolygonObject(goal) && !tData->GetBool(PC_USE_POINT+i)) return true;
				else return false;
			}
			else if(id[0].id == PC_P_INDEX+i)
			{
				if(!tData->GetBool(T_REG)) return false;
				else if(tData->GetBool(PC_USE_POINT+i) || tData->GetBool(PC_USE_NORMAL+i)) return true;
				else return false;
			}
			else if(id[0].id == PC_SET_P_SEL+i)
			{
				if(!tData->GetBool(T_REG)) return false;
				else if(tData->GetBool(PC_USE_POINT+i) || tData->GetBool(PC_USE_NORMAL+i)) return true;
				else return false;
			}
		}
		else
		{
			if(id[0].id == PC_USE_POINT+i) return false;
			else if(id[0].id == PC_USE_NORMAL+i) return false;
			else if(id[0].id == PC_P_INDEX+i) return false;
			else if(id[0].id == PC_SET_P_SEL+i) return false;
		}
	}
	for(i=0; i<pCnt; i++)
	{
		if(id[0].id == PC_POS_MIX+i)
		{
			BaseObject *goal = tData->GetObjectLink(PC_TARGET+i,doc);
			if(!goal) return false;
			else if(tData->GetBool(PC_USE_AB_MIX)) return false;
			else return true;
		}
	}
	return true;
}

Bool RegisterCDPConstraintPlugin(void)
{
	if(CDFindPlugin(ID_CDPCONSTRAINTPLUGIN,CD_TAG_PLUGIN)) return true;
	
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
	String name=GeLoadString(IDS_CDPCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDPCONSTRAINTPLUGIN,name,info,CDPConstraintPlugin::Alloc,"tCDPConstraint","CDPConstraint.tif",1);
}

//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "tCDAConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	10

enum
{
	//AC_PURCHASE		= 1000,
	
	AC_TARGET_OLD			= 1010,
	AC_UP_VECTOR_OLD		= 1011,
	//AC_STRENGTH		= 1012,
	
	//AC_SHOW_LINES		= 1013,
	//AC_LINE_COLOR		= 1014,
	
	//AC_POLE_AXIS		= 1015,
		//AC_POLE_X		= 1016,
		//AC_POLE_Y		= 1017,
		//AC_POLE_NX		= 1018,
		//AC_POLE_NY		= 1019,
		
	//AC_AB_MIX				= 1020,
	//AC_USE_AB_MIX			= 1023,

	//AC_POLE_OFF			= 1024,

	AC_COUNT			= 1030,
	//AC_ADD_AIM				= 1031,
	//AC_SUB_AIM				= 1032,
	AC_DISK_LEVEL				= 1033,					
	
	AC_TARGET_COUNT			= 1040,
	
	//AC_ID_TARGET			= 1050,
	//AC_ID_T_SUBGROUP		= 1500,

	//AC_T_LINK_GROUP		= 2000,
	//AC_U_LINK_GROUP		= 3000,

	//AC_TARGET				= 4000,
	//AC_UP_VECTOR			= 5000,
	//AC_AIM_MIX				= 6000,
	//AC_UP_MIX				= 7000,
	//AC_USE_A_SPLINE		= 8000,
	//AC_AIM_SPOINT			= 9000,
	//AC_USE_U_SPLINE		= 10000,
	//AC_UP_SPOINT			= 11000,
	//AC_A_SPLINE_GROUP		= 12000,
	//AC_U_SPLINE_GROUP		= 13000,
	//AC_LINE_ADD			= 14000,
	//SPACE_ADD			= 15000,
	
	//AC_ALIGN_AXIS			= 16000,
		//AC_ALIGN_OFF		= 16100,
		//AC_ALIGN_X			= 16200,
		//AC_ALIGN_Y			= 16300,
		//AC_ALIGN_Z			= 16400,
		//AC_ALIGN_NX		= 16500,
		//AC_ALIGN_NY		= 16600,
		//AC_ALIGN_NZ		= 16700,
	
	//AC_Z_DIRECTION			= 16800,
	//AC_UPV_OFFSET			= 16900,

};

class CDAConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	void ConvertOldData(BaseDocument *doc, BaseContainer *tData);
	LONG ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData);
	Vector CalcPoleOffset(Matrix m, Vector aV, Vector pV, Real offset);

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
	

	static NodeData *Alloc(void) { return CDDataAllocator(CDAConstraintPlugin); }
};

Bool CDAConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(AC_SHOW_LINES,true);
	tData->SetVector(AC_LINE_COLOR, Vector(1,0,0));
	tData->SetLong(AC_POLE_AXIS, AC_POLE_Y);
	tData->SetReal(AC_STRENGTH,1.00);
	tData->SetReal(AC_AB_MIX,0.5);

	tData->SetReal(AC_AIM_MIX,1.00);
	tData->SetReal(AC_UP_MIX,1.00);
	tData->SetLong(AC_ALIGN_AXIS,AC_ALIGN_OFF);
	tData->SetLong(AC_TARGET_COUNT,1);
	tData->SetBool(AC_Z_DIRECTION,false);
		
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

Bool CDAConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(AC_DISK_LEVEL,level);
	
	return true;
}

void CDAConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(AC_POLE_AXIS+T_LST,tData->GetLong(AC_POLE_AXIS));
			tData->SetBool(AC_USE_AB_MIX+T_LST,tData->GetBool(AC_USE_AB_MIX));
			tData->SetLong(AC_TARGET_COUNT+T_LST,tData->GetLong(AC_TARGET_COUNT));
			
			LONG i, aCnt = tData->GetLong(AC_TARGET_COUNT);
			for(i=0; i<aCnt; i++)
			{
				tData->SetLink(AC_TARGET+i+T_LST,tData->GetLink(AC_TARGET+i,doc));
				tData->SetLink(AC_UP_VECTOR+i+T_LST,tData->GetLink(AC_UP_VECTOR+i,doc));
				tData->SetBool(AC_USE_A_SPLINE+i+T_LST,tData->GetBool(AC_USE_A_SPLINE+i));
				tData->SetBool(AC_USE_U_SPLINE+i+T_LST,tData->GetBool(AC_USE_U_SPLINE+i));
				tData->SetLong(AC_ALIGN_AXIS+i+T_LST,tData->GetLong(AC_ALIGN_AXIS+i));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDAConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(AC_POLE_AXIS,tData->GetLong(AC_POLE_AXIS+T_LST));
		tData->SetBool(AC_USE_AB_MIX,tData->GetBool(AC_USE_AB_MIX+T_LST));
		tData->SetLong(AC_TARGET_COUNT,tData->GetLong(AC_TARGET_COUNT+T_LST));
		
		LONG i, aCnt = tData->GetLong(AC_TARGET_COUNT);
		for(i=0; i<aCnt; i++)
		{
			tData->SetLink(AC_TARGET+i,tData->GetLink(AC_TARGET+i+T_LST,doc));
			tData->SetLink(AC_UP_VECTOR+i,tData->GetLink(AC_UP_VECTOR+i+T_LST,doc));
			tData->SetBool(AC_USE_A_SPLINE+i,tData->GetBool(AC_USE_A_SPLINE+i+T_LST));
			tData->SetBool(AC_USE_U_SPLINE+i,tData->GetBool(AC_USE_U_SPLINE+i+T_LST));
			tData->SetLong(AC_ALIGN_AXIS+i,tData->GetLong(AC_ALIGN_AXIS+i+T_LST));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDAConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(AC_TARGET+i,NULL);
	tData->SetReal(AC_AIM_MIX+i,1.0);
	
	tData->SetLink(AC_UP_VECTOR+i,NULL);
	tData->SetReal(AC_UP_MIX+i,1.0);
	
	tData->SetBool(AC_USE_A_SPLINE+i,false);
	tData->SetLong(AC_AIM_SPOINT+i,0);
	
	tData->SetBool(AC_USE_U_SPLINE+i,false);
	tData->SetLong(AC_UP_SPOINT+i,0);
	
	tData->SetLong(AC_ALIGN_AXIS+i,AC_ALIGN_OFF+i);
	tData->SetBool(AC_Z_DIRECTION+i,false);
}

void CDAConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(AC_TARGET+dst,tData->GetObjectLink(AC_TARGET+src,doc));
	tData->SetReal(AC_AIM_MIX+dst,tData->GetReal(AC_AIM_MIX+src));
	
	tData->SetLink(AC_UP_VECTOR+dst,tData->GetObjectLink(AC_UP_VECTOR+src,doc));
	tData->SetReal(AC_UP_MIX+dst,tData->GetReal(AC_UP_MIX+src));
	
	tData->SetBool(AC_USE_A_SPLINE+dst,tData->GetBool(AC_USE_A_SPLINE+src));
	tData->SetLong(AC_AIM_SPOINT+dst,tData->GetLong(AC_AIM_SPOINT+src));
	
	tData->SetBool(AC_USE_U_SPLINE+dst,tData->GetBool(AC_USE_U_SPLINE+src));
	tData->SetLong(AC_UP_SPOINT+dst,tData->GetLong(AC_UP_SPOINT+src));
	
	tData->SetLong(AC_ALIGN_AXIS+dst,tData->GetLong(AC_ALIGN_AXIS+src)-(src-dst));
	tData->SetBool(AC_Z_DIRECTION+dst,tData->GetBool(AC_Z_DIRECTION+src));
}

void CDAConstraintPlugin::ConvertOldData(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, oldCnt = 0;
	
	BaseObject *goal = tData->GetObjectLink(AC_TARGET_OLD,doc);
	if(goal)
	{
		tData->SetLink(AC_TARGET,goal);
		tData->SetReal(AC_AIM_MIX,1.00);
		oldCnt = 1;
	}
	BaseObject *upVector = tData->GetObjectLink(AC_UP_VECTOR_OLD,doc);
	if(upVector)
	{
		tData->SetLink(AC_UP_VECTOR,upVector);
		oldCnt = 1;
	}
	if(oldCnt == 0) tData->SetLong(AC_TARGET_COUNT,tData->GetLong(AC_COUNT) + 1);
	else
	{
		for(i=oldCnt; i<MAX_ADD; i++)
		{
			if(tData->GetObjectLink(AC_TARGET+i,doc) || tData->GetObjectLink(AC_UP_VECTOR+i,doc)) oldCnt++;
		}
		tData->SetLong(AC_TARGET_COUNT,oldCnt);
	}
	
}

Bool CDAConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(AC_DISK_LEVEL) < 1) ConvertOldData(doc,tData);
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
	
	LONG aCnt = tData->GetLong(AC_TARGET_COUNT);
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==AC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==AC_ADD_AIM)
			{
				if(tData->GetBool(T_REG))
				{
					if (aCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,aCnt);
						aCnt++;
						tData->SetLong(AC_TARGET_COUNT,aCnt);
					}
				}
			}
			else if (dc->id[0].id==AC_SUB_AIM)
			{
				if(tData->GetBool(T_REG))
				{
					if (aCnt > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,aCnt);
						aCnt--;
						tData->SetLong(AC_TARGET_COUNT,aCnt);
					}
				}
			}
			break;
		}
	}

	return true;
}

Bool CDAConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	if (!tData->GetBool(AC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	LONG i, aCnt = tData->GetLong(AC_TARGET_COUNT);
	BaseObject *goal = NULL, *pole = NULL;
	SplineObject *spline=NULL;
	
	bd->SetPen(tData->GetVector(AC_LINE_COLOR));
	for (i=0; i<aCnt; i++)
	{
		goal = tData->GetObjectLink(AC_TARGET+i,doc); 
		if(goal)
		{
			Vector goalPosition = goal->GetMg().off;
			if(tData->GetBool(AC_USE_A_SPLINE+i))
			{
				spline = goal->GetRealSpline();
				goalPosition = goal->GetMg() * CDGetSplinePoint(spline,CDUniformToNatural(spline,tData->GetReal(AC_AIM_SPOINT+i)));
			}
			CDDrawLine(bd,op->GetMg().off, goalPosition);
		}
		pole = tData->GetObjectLink(AC_UP_VECTOR+i,doc);
		if(pole)
		{
			Vector polePosition = pole->GetMg().off;
			if(tData->GetBool(AC_USE_U_SPLINE+i))
			{
				spline = pole->GetRealSpline();
				polePosition = pole->GetMg() * CDGetSplinePoint(spline,CDUniformToNatural(spline,tData->GetReal(AC_UP_SPOINT+i)));
			}
			CDDrawLine(bd,op->GetMg().off, polePosition);
		}
	}
	
	return true;
}

LONG CDAConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, aCnt = tData->GetLong(AC_TARGET_COUNT);
	BaseObject *trg = NULL, *upV = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<aCnt; i++)
	{
		trg = tData->GetObjectLink(AC_TARGET+i,doc);
		upV = tData->GetObjectLink(AC_UP_VECTOR+i,doc);
		if(!trg && !upV)
		{
			LONG j = i;
			while(!trg && !upV && j < aCnt)
			{
				j++;
				trg = tData->GetObjectLink(AC_TARGET+j,doc);
				upV = tData->GetObjectLink(AC_UP_VECTOR+j,doc);
			}
			if(trg || upV)
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

Vector CDAConstraintPlugin::CalcPoleOffset(Matrix m, Vector aV, Vector pV, Real offset)
{
	Real poleLen = Len(pV - m.off);
	if(offset > poleLen) offset = poleLen - 1;
	if(offset < -poleLen) offset = -poleLen + 1;
	
	Real angle = ASin(offset/poleLen);

	Matrix aM, offM, rotM = CDHPBToMatrix(Vector(0.0,0.0,angle));
	
	aM.off = m.off;
	aM.v3 = aV;
	aM.v2 = VNorm(pV - m.off);
	aM.v1 = VNorm(VCross(aM.v2, aM.v3));
	aM.v2 = VNorm(VCross(aM.v3, aM.v1));
	
	offM = aM * rotM;
	Real adj = Sqrt((poleLen*poleLen) - (offset*offset));
	
	return m.off + offM.v2 * adj;
}

LONG CDAConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, aCnt = ConfirmTargetCount(doc,tData);
	BaseObject *goal = NULL, *pole = NULL;
	SplineObject *spline=NULL;
	
	Matrix opM = op->GetMg(), targM, transM;
	Vector opPos = CDGetPos(op), opScale = CDGetScale(op), opRot = CDGetRot(op);
	Vector newRot, setRot;
	
	Vector poleMix = Vector(0,0,0), goalMix = Vector(0,0,0), poleV, goalV;
	Real theStrength = tData->GetReal(AC_STRENGTH);
	CDQuaternion opQ, targQ, transQ;

	// Get the total constraint mix
	Bool ABMixAim = true;
	Real totalAimMix = 0.0, totalUpMix = 0.0;
	if(aCnt < 2)
	{
		ABMixAim = false;
		tData->SetBool(AC_USE_AB_MIX,false);
	}
	for(i=0; i<aCnt; i++)
	{
		goal = tData->GetObjectLink(AC_TARGET+i,doc);
		if(goal)
		{
			if (goal->GetMg().off != opM.off)
			{
				totalAimMix += tData->GetReal(AC_AIM_MIX+i);
			}
		}
		else
		{
			tData->SetBool(AC_Z_DIRECTION+i,false);
			ABMixAim = false;
		}
		totalUpMix += tData->GetReal(AC_UP_MIX+i);
		
		pole = tData->GetObjectLink(AC_UP_VECTOR+i,doc);
		if(!pole) tData->SetLong(AC_ALIGN_AXIS+i,AC_ALIGN_OFF+i);
	}
	if(totalAimMix == 0.0 && totalUpMix == 0.0) return false;

	for(i=0; i<aCnt; i++)
	{
		Vector aimVector = opM.v3;
		
		Real maxVal = 0.0, minVal = 1.0;
		if(aCnt-1 != 0) minVal = 1.0/(Real)(aCnt-1);//Check for division by zero
		
		Real mixVal, value = Abs((minVal*i) - tData->GetReal(AC_AB_MIX));
		if(value > minVal) value = minVal;
		if ((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
		else mixVal = 0.0;

		if(totalAimMix > 0.0)
		{
			goal = tData->GetObjectLink(AC_TARGET+i,doc);
			if(goal)
			{
				Vector goalPos;
				if(goal->GetType() != Ospline) tData->SetBool(AC_USE_A_SPLINE+i, false);
				
				if(tData->GetBool(AC_USE_A_SPLINE+i))
				{
					spline = goal->GetRealSpline();
					goalPos = goal->GetMg() * CDGetSplinePoint(spline,CDUniformToNatural(spline,tData->GetReal(AC_AIM_SPOINT+i)));
				}
				else
				{
					goalPos = goal->GetMg().off;
				}
				
				if (goalPos != opM.off)
				{
					aimVector = VNorm(goalPos - opM.off);
					
					if(tData->GetBool(AC_Z_DIRECTION+i)) goalPos = opM.off + (opM.off - goalPos);
					
					if(tData->GetBool(AC_USE_AB_MIX) && ABMixAim) goalMix += goalPos * mixVal;
					else goalMix += goalPos * (tData->GetReal(AC_AIM_MIX+i)/totalAimMix);
				}
			}
		}
		
		if(totalUpMix > 0.0)
		{
			
			pole = tData->GetObjectLink(AC_UP_VECTOR+i,doc);
			if(pole)
			{
				if(pole->GetType() != Ospline) tData->SetBool(AC_USE_U_SPLINE+i, false);
				
				if(tData->GetBool(AC_USE_U_SPLINE+i))
				{
					spline = pole->GetRealSpline();
					Vector sPt = pole->GetMg() * CDGetSplinePoint(spline,CDUniformToNatural(spline,tData->GetReal(AC_UP_SPOINT+i)));

					if(tData->GetBool(AC_USE_AB_MIX)) poleMix += sPt * mixVal;
					else poleMix += sPt * (tData->GetReal(AC_UP_MIX+i)/totalUpMix);
				}
				else
				{
					Vector poleVector;
					
					if(tData->GetLong(AC_ALIGN_AXIS+i) == AC_ALIGN_OFF+i)
					{
						if(pole->GetMg().off != opM.off)
						{
							poleVector = pole->GetMg().off;
							
							// calculate offset
							Real poleOffset = tData->GetReal(AC_UPV_OFFSET+i);
							if(poleOffset != 0.0)
							{
								poleVector = CalcPoleOffset(opM,aimVector,poleVector,poleOffset);
							}
							
							if(tData->GetBool(AC_USE_AB_MIX)) poleMix += poleVector * mixVal;
							else poleMix += poleVector * (tData->GetReal(AC_UP_MIX+i)/totalUpMix);
						}
					}
					else
					{
						if(tData->GetLong(AC_ALIGN_AXIS+i) == AC_ALIGN_X+i)
							poleVector = opM.off + VNorm(pole->GetMg().v1) * 100;
						
						else if(tData->GetLong(AC_ALIGN_AXIS+i) == AC_ALIGN_Y+i)
							poleVector = opM.off + VNorm(pole->GetMg().v2) * 100;
						
						else if(tData->GetLong(AC_ALIGN_AXIS+i) == AC_ALIGN_Z+i)
							poleVector = opM.off + VNorm(pole->GetMg().v3) * 100;
						
						else if(tData->GetLong(AC_ALIGN_AXIS+i) == AC_ALIGN_NX+i)
							poleVector = opM.off - VNorm(pole->GetMg().v1) * 100;
						
						else if(tData->GetLong(AC_ALIGN_AXIS+i) == AC_ALIGN_NY+i)
							poleVector = opM.off - VNorm(pole->GetMg().v2) * 100;
						
						else if(tData->GetLong(AC_ALIGN_AXIS+i) == AC_ALIGN_NZ+i)
							poleVector = opM.off - VNorm(pole->GetMg().v3) * 100;
						
						// calculate offset
						Real poleOffset = tData->GetReal(AC_UPV_OFFSET+i);
						if(poleOffset != 0.0)
						{
							poleVector = CalcPoleOffset(opM,aimVector,poleVector,poleOffset);
						}
						
						if(tData->GetBool(AC_USE_AB_MIX)) poleMix += poleVector * mixVal;
						else poleMix += poleVector * (tData->GetReal(AC_UP_MIX+i)/totalUpMix);
					}
				}
			}
			else
			{
				if(tData->GetBool(AC_USE_AB_MIX)) poleMix += (opM.off + Vector(0,1,0) * 100) * mixVal;
				else poleMix += (opM.off + Vector(0,1,0) * 100) * (tData->GetReal(AC_UP_MIX+i)/totalUpMix);
			}
		}
	}
	
	if(totalUpMix > 0.0) poleV = VNorm(poleMix - opM.off);
	else  poleV = opM.off + Vector(0,1,0);
	if(totalAimMix > 0.0)  goalV = VNorm(goalMix - opM.off);
	else  goalV = opM.v3;
	
	// Set the target matrix
	targM = opM;
	switch (tData->GetLong(AC_POLE_AXIS))
	{
			//	Set the Rotation Plane matrix vectors.
		case AC_POLE_OFF:	
			targM.v2 = VNorm(VCross(goalV, targM.v1));
			targM.v1 = VNorm(VCross(targM.v2, goalV));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		case AC_POLE_X:	
			targM.v2 = VNorm(VCross(goalV, poleV));
			targM.v1 = VNorm(VCross(targM.v2, goalV));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		case AC_POLE_Y:
			targM.v1 = VNorm(VCross(poleV, goalV));
			targM.v2 = VNorm(VCross(goalV, targM.v1));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		case AC_POLE_NX:	
			targM.v2 = VNorm(VCross(poleV, goalV));
			targM.v1 = VNorm(VCross(targM.v2, goalV));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		case AC_POLE_NY:
			targM.v1 = VNorm(VCross(goalV, poleV));
			targM.v2 = VNorm(VCross(goalV , targM.v1));
			targM.v3 = VNorm(VCross(targM.v1, targM.v2));
			break;
		default:
			break;
		
	}
	opQ.SetMatrix(opM);
	targQ.SetMatrix(targM);
	transQ = CDQSlerp(opQ, targQ, theStrength);
	
	transM = transQ.GetMatrix();
	transM.off = opM.off;
	
	if(!MatrixEqual(transM,opM,0.001))
	{
		op->SetMg(transM);
		
		newRot = CDGetRot(op);
		setRot = CDGetOptimalAngle(opRot, newRot, op);
		
		CDSetRot(op,setRot);
		CDSetPos(op,opPos);
		CDSetScale(op,opScale);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDAConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, aCnt = tData->GetLong(AC_TARGET_COUNT);
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(AC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	for (i=0; i<aCnt; i++)
	{
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if (!description->SetParameter(DescLevel(AC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(AC_ID_TARGET))) return false;

		BaseContainer subgroup0 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup0.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		subgroup0.SetLong(DESC_COLUMNS, 1);
		if(!description->SetParameter(DescLevel(AC_ID_T_SUBGROUP+i, DTYPE_GROUP, 0), subgroup0, DescLevel(AC_ID_TARGET))) return true;
		
		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(AC_T_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(AC_ID_T_SUBGROUP+i))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(AC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(AC_T_LINK_GROUP+i))) return false;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i)+".t");
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(AC_AIM_MIX+i, DTYPE_REAL, 0), bc3, DescLevel(AC_T_LINK_GROUP+i))) return true;

		BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc6.SetString(DESC_NAME,GeLoadString(IDS_UPVECTOR)+"."+CDLongToString(i));
		bc6.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc6.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(AC_UP_VECTOR+i,DTYPE_BASELISTLINK,0),bc6,DescLevel(AC_T_LINK_GROUP+i))) return false;
		
		BaseContainer bc7 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc7.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc7.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i)+".u");
		bc7.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc7.SetReal(DESC_MIN,0.0);
		bc7.SetReal(DESC_MAX,1.0);
		bc7.SetReal(DESC_STEP,0.01);
		bc7.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(AC_UP_MIX+i, DTYPE_REAL, 0), bc7, DescLevel(AC_T_LINK_GROUP+i))) return true;

		BaseContainer subgroup2 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup2.SetLong(DESC_COLUMNS, 1);
		subgroup2.SetString(DESC_NAME, GeLoadString(IDS_OPTIONS)+"."+CDLongToString(i));
		if(!description->SetParameter(DescLevel(AC_A_SPLINE_GROUP+i, DTYPE_GROUP, 0), subgroup2, DescLevel(AC_ID_T_SUBGROUP+i))) return true;
		
		BaseContainer subgroup3 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup3.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(AC_U_SPLINE_GROUP+i, DTYPE_GROUP, 0), subgroup3, DescLevel(AC_A_SPLINE_GROUP+i))) return true;
		
		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc4.SetString(DESC_NAME, GeLoadString(IDS_USE_SPLINE)+"."+GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc4.SetBool(DESC_DEFAULT, false);
		if (!description->SetParameter(DescLevel(AC_USE_A_SPLINE+i, DTYPE_BOOL, 0), bc4, DescLevel(AC_U_SPLINE_GROUP+i))) return true;

		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc5.SetString(DESC_NAME, GeLoadString(IDS_SPLINE_OFFSET)+"."+GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc5.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc5.SetReal(DESC_MIN,0.0);
		bc5.SetReal(DESC_MAX,1.0);
		bc5.SetReal(DESC_STEP,0.01);
		bc5.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
		bc5.SetBool(DESC_REMOVEABLE,false);
		if (!description->SetParameter(DescLevel(AC_AIM_SPOINT+i, DTYPE_REAL, 0), bc5, DescLevel(AC_U_SPLINE_GROUP+i))) return true;

		BaseContainer bc8 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc8.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc8.SetString(DESC_NAME, GeLoadString(IDS_USE_SPLINE)+"."+GeLoadString(IDS_UPVECTOR)+"."+CDLongToString(i));
		bc8.SetBool(DESC_DEFAULT, false);
		if (!description->SetParameter(DescLevel(AC_USE_U_SPLINE+i, DTYPE_BOOL, 0), bc8, DescLevel(AC_U_SPLINE_GROUP+i))) return true;

		BaseContainer bc9 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc9.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc9.SetString(DESC_NAME, GeLoadString(IDS_SPLINE_OFFSET)+"."+GeLoadString(IDS_UPVECTOR)+"."+CDLongToString(i));
		bc9.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc9.SetReal(DESC_MIN,0.0);
		bc9.SetReal(DESC_MAX,1.0);
		bc9.SetReal(DESC_STEP,0.01);
		bc9.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
		bc9.SetBool(DESC_REMOVEABLE,false);
		if (!description->SetParameter(DescLevel(AC_UP_SPOINT+i, DTYPE_REAL, 0), bc9, DescLevel(AC_U_SPLINE_GROUP+i))) return true;

		BaseContainer subgroup4 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup4.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(AC_U_OPTION_GROUP+i, DTYPE_GROUP, 0), subgroup4, DescLevel(AC_A_SPLINE_GROUP+i))) return true;
		
		BaseContainer bc11 = GetCustomDataTypeDefault(DTYPE_LONG);
		bc11.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
		bc11.SetString(DESC_NAME, GeLoadString(IDS_ALIGN_AXIS)+"."+CDLongToString(i));
		BaseContainer cycle;
		cycle.SetString(AC_ALIGN_OFF+i, GeLoadString(IDS_ALIGN_OFF));
		cycle.SetString(AC_ALIGN_X+i, GeLoadString(IDS_ALIGN_X));
		cycle.SetString(AC_ALIGN_Y+i, GeLoadString(IDS_ALIGN_Y));
		cycle.SetString(AC_ALIGN_Z+i, GeLoadString(IDS_ALIGN_Z));
		cycle.SetString(AC_ALIGN_NX+i, GeLoadString(IDS_ALIGN_NX));
		cycle.SetString(AC_ALIGN_NY+i, GeLoadString(IDS_ALIGN_NY));
		cycle.SetString(AC_ALIGN_NZ+i, GeLoadString(IDS_ALIGN_NZ));
		bc11.SetContainer(DESC_CYCLE, cycle);
		if (!description->SetParameter(DescLevel(AC_ALIGN_AXIS+i, DTYPE_LONG, 0), bc11, DescLevel(AC_U_OPTION_GROUP+i))) return true;

		BaseContainer bc10 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc10.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc10.SetString(DESC_NAME, GeLoadString(IDS_UPV_OFFSET)+"."+CDLongToString(i));
		bc10.SetLong(DESC_UNIT, DESC_UNIT_METER);
		bc10.SetReal(DESC_STEP,0.01);
		bc10.SetReal(DESC_DEFAULT,0.0);
		if (!description->SetParameter(DescLevel(AC_UPV_OFFSET+i, DTYPE_REAL, 0), bc10, DescLevel(AC_U_OPTION_GROUP+i))) return true;
		
		BaseContainer bc12 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc12.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc12.SetString(DESC_NAME, GeLoadString(IDS_Z_DIRECTION)+"."+CDLongToString(i));
		bc12.SetBool(DESC_DEFAULT, false);
		if (!description->SetParameter(DescLevel(AC_Z_DIRECTION+i, DTYPE_BOOL, 0), bc12, DescLevel(AC_A_SPLINE_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDAConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	BaseObject *t=NULL, *u=NULL;

	LONG i, aCnt = tData->GetLong(AC_TARGET_COUNT);
	switch (id[0].id)
	{
		case AC_POLE_AXIS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case AC_USE_AB_MIX:
			if(!tData->GetBool(T_REG)) return false;
			else if(aCnt < 2) return false;
			else return true;
		case AC_AB_MIX:
			return tData->GetBool(AC_USE_AB_MIX);
		case AC_ADD_AIM:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case AC_SUB_AIM:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	for(i=0; i<aCnt;i++)
	{
		if(id[0].id == AC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == AC_UP_VECTOR+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == AC_AIM_MIX+i)
		{
			t = tData->GetObjectLink(AC_TARGET+i,doc);
			if(!t) return false;
			if(tData->GetBool(AC_USE_AB_MIX)) return false;
			else return true;
		}
		else if(id[0].id == AC_UP_MIX+i)
		{
			if(tData->GetBool(AC_USE_AB_MIX)) return false;
			else return true;
		}
		else if(id[0].id == AC_USE_A_SPLINE+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				t = tData->GetObjectLink(AC_TARGET+i,doc);
				if(t && t->GetType() == Ospline) return true;
				else return false;
			}
		}
		else if(id[0].id == AC_AIM_SPOINT+i)
		{
			t = tData->GetObjectLink(AC_TARGET+i,doc);
			if(t && t->GetType() == Ospline && tData->GetBool(AC_USE_A_SPLINE+i)) return true;
			else return false;
		}
		else if(id[0].id == AC_USE_U_SPLINE+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				u = tData->GetObjectLink(AC_UP_VECTOR+i,doc);
				if(u && u->GetType() == Ospline) return true;
				else return false;
			}
		}
		else if(id[0].id == AC_UP_SPOINT+i)
		{
			u = tData->GetObjectLink(AC_UP_VECTOR+i,doc);
			if(u && u->GetType() == Ospline && tData->GetBool(AC_USE_U_SPLINE+i)) return true;
			else return false;
		}
		else if(id[0].id == AC_UPV_OFFSET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetObjectLink(AC_UP_VECTOR+i,doc)) return false;
				else return true;
			}
		}
		else if(id[0].id == AC_ALIGN_AXIS+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetObjectLink(AC_UP_VECTOR+i,doc)) return false;
				else return true;
			}
		}
		else if(id[0].id == AC_Z_DIRECTION+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetObjectLink(AC_TARGET+i,doc)) return false;
				else return true;
			}
		}
	}
	return true;
}

Bool RegisterCDAConstraintPlugin(void)
{
	if(CDFindPlugin(ID_CDACONSTRAINTPLUGIN,CD_TAG_PLUGIN)) return true;
	
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
	String name=GeLoadString(IDS_CDACONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDACONSTRAINTPLUGIN,name,info,CDAConstraintPlugin::Alloc,"tCDAConstraint","CDAConstraint.tif",1);
}

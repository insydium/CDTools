//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDSPLConstraint.h"
#include "CDConstraint.h"
//#include "CDDebug.h"

/*enum
{
	//SPLC_TARGET				= 1000,
	//SPLC_TANGENT				= 1001,
	
	//SPLC_FORCE_POS				= 1002,
	//SPLC_POSITION				= 1003,

	//SPLC_SEGMENT				= 1004,
	//SPLC_UP_VECTOR				= 1005,

	//SPLC_ALIGN_CHILDREN		= 1006,
	//SPLC_CHILD_OFFSET			= 1007,

	//SPLC_ALIGN_AXIS			= 1100,
		//SPLC_ALIGN_X			= 1101,
		//SPLC_ALIGN_NX			= 1102,
		//SPLC_ALIGN_Y			= 1103,
		//SPLC_ALIGN_NY			= 1104,
		//SPLC_ALIGN_Z			= 1105,
		//SPLC_ALIGN_NZ			= 1106,

	//SPLC_PURCHASE				= 1200
};*/

class CDSPLConstraintPlugin : public CDTagData
{		
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	void GetMatrixOrientation(BaseDocument *doc, BaseTag *tag, SplineObject *spl, Matrix *m, Vector pt, Real n, Bool unif);
	
public:				
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSPLConstraintPlugin); }
};

Bool CDSPLConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(SPLC_TANGENT,false);
	tData->SetBool(SPLC_FORCE_POS,false);
	tData->SetLong(SPLC_ALIGN_AXIS,SPLC_ALIGN_Z);

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

void CDSPLConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
			tData->SetLink(SPLC_TARGET+T_LST,tData->GetLink(SPLC_TARGET,doc));
			tData->SetLink(SPLC_UP_VECTOR+T_LST,tData->GetLink(SPLC_UP_VECTOR,doc));
			tData->SetLong(SPLC_ALIGN_AXIS+T_LST,tData->GetLong(SPLC_ALIGN_AXIS));
			
			tData->SetBool(SPLC_ALIGN_CHILDREN+T_LST,tData->GetBool(SPLC_ALIGN_CHILDREN));
			
			tData->SetBool(SPLC_TANGENT+T_LST,tData->GetBool(SPLC_TANGENT));
			tData->SetBool(SPLC_FORCE_POS+T_LST,tData->GetBool(SPLC_FORCE_POS));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSPLConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(SPLC_TARGET,tData->GetLink(SPLC_TARGET+T_LST,doc));
		tData->SetLink(SPLC_UP_VECTOR,tData->GetLink(SPLC_UP_VECTOR+T_LST,doc));
		tData->SetLong(SPLC_ALIGN_AXIS,tData->GetLong(SPLC_ALIGN_AXIS+T_LST));
		
		tData->SetBool(SPLC_ALIGN_CHILDREN,tData->GetBool(SPLC_ALIGN_CHILDREN+T_LST));
			
		tData->SetBool(SPLC_TANGENT,tData->GetBool(SPLC_TANGENT+T_LST));
		tData->SetBool(SPLC_FORCE_POS,tData->GetBool(SPLC_FORCE_POS+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSPLConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			if(dc->id[0].id==SPLC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
		}
	}

	return true;
}


void CDSPLConstraintPlugin::GetMatrixOrientation(BaseDocument *doc, BaseTag *tag, SplineObject *spl, Matrix *m, Vector pt, Real n, Bool unif)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return;
	BaseObject *goal = tData->GetObjectLink(SPLC_TARGET,doc); if(!goal) return;
	BaseContainer *sData = goal->GetDataInstance(); if(!sData) return;
	
	Bool splPrm = false;
	LONG opType = goal->GetType();
	if(opType != Ospline)
	{
		if(goal->GetRealSpline()) splPrm = true;
	}
	
	Matrix goalM = goal->GetMg();
	Vector a, upV, aimV;
	LONG seg = tData->GetLong(SPLC_SEGMENT);
	
	if(n <= 0.9999)
	{
		if(n < 0.0)
		{
			if(unif) a = spl->GetSplinePoint(CDUniformToNatural(spl,0.0),seg);
			else a = spl->GetSplinePoint(0.0,seg);
			aimV = VNorm(a - m->off);
		}
		else
		{
			if(unif) a = spl->GetSplinePoint(CDUniformToNatural(spl,n + 0.0001),seg);
			else a = spl->GetSplinePoint(n + 0.0001,seg);
			aimV = VNorm(a - pt);
		}
	}
	else
	{
		if(unif) a = spl->GetSplinePoint(CDUniformToNatural(spl,n - 0.0001),seg);
		else a = spl->GetSplinePoint(n - 0.0001,seg);
		aimV = VNorm(pt - a);
	}
	
	upV = Vector(0,1,0);
	BaseObject *rail = tData->GetObjectLink(SPLC_UP_VECTOR,doc);
	if(rail)
	{
		Matrix rM = rail->GetMg();
		if((rail->GetInfo() & OBJECT_ISSPLINE))
		{
			SplineObject *rSpl = rail->GetRealSpline();
			
			if(rSpl)
			{
				BaseContainer *rData = rail->GetDataInstance();
				if(rData)
				{
					if(rData->GetLong(SPLINEOBJECT_INTERPOLATION) == SPLINEOBJECT_INTERPOLATION_UNIFORM)
					{
						upV = VNorm((MInv(goalM) * (rM * CDGetSplinePoint(rSpl,CDUniformToNatural(rSpl,n)))) - pt);
					}
					else upV = VNorm((MInv(goalM) * (rM * CDGetSplinePoint(rSpl,n))) - pt);
				}
			}
			else upV = VNorm((MInv(goalM) * rM.off) - pt);
		}
		else upV = VNorm((MInv(goalM) * rM.off) - pt);
	}
	else
	{
		if(splPrm)
		{
			LONG plane = sData->GetLong(PRIM_PLANE);
			switch(plane)
			{
				case PRIM_PLANE_XY:
					upV = Vector(0,0,1);
					break;
				case PRIM_PLANE_ZY:
					upV = Vector(1,0,0);
					break;
				case PRIM_PLANE_XZ:
					upV = Vector(0,1,0);
					break;
			}
		}
		else
		{
			LONG cnt = spl->GetPointCount();
			
			Vector *padr = GetPointArray(spl);
			if(cnt < 3)
			{
				Vector spPts[3];
				spPts[0] = padr[0];
				spPts[2] = padr[1];
				spPts[1] = spl->GetSplinePoint(CDUniformToNatural(spl,0.5),seg);
				upV = GetBestFitPlaneNormal(spPts,3);
			}
			else upV = GetBestFitPlaneNormal(padr,cnt);
		}
	}

	switch(tData->GetLong(SPLC_ALIGN_AXIS))
	{
		case SPLC_ALIGN_X:
		{
			m->v1 = VNorm(aimV);
			m->v3 = upV;
			m->v2 = VNorm(VCross(m->v3, m->v1));
			m->v3 = VNorm(VCross(m->v1, m->v2));
			break;
		}
		case SPLC_ALIGN_NX:
		{
			m->v1 = VNorm(-aimV);
			m->v3 = upV;
			m->v2 = VNorm(VCross(m->v3, m->v1));
			m->v3 = VNorm(VCross(m->v1, m->v2));
			break;
		}
		case SPLC_ALIGN_Y:
		{
			m->v2 = VNorm(aimV);
			m->v1 = upV;
			m->v3 = VNorm(VCross(m->v1, m->v2));
			m->v1 = VNorm(VCross(m->v2, m->v3));
			break;
		}
		case SPLC_ALIGN_NY:
		{
			m->v2 = VNorm(-aimV);
			m->v1 = upV;
			m->v3 = VNorm(VCross(m->v1, m->v2));
			m->v1 = VNorm(VCross(m->v2, m->v3));
			break;
		}
		case SPLC_ALIGN_Z:
		{
			m->v3 = VNorm(aimV);
			m->v2 = upV;
			m->v1 = VNorm(VCross(m->v2, m->v3));
			m->v2 = VNorm(VCross(m->v3, m->v1));
			break;
		}
		case SPLC_ALIGN_NZ:
		{
			m->v3 = VNorm(-aimV);
			m->v2 = upV;
			m->v1 = VNorm(VCross(m->v2, m->v3));
			m->v2 = VNorm(VCross(m->v3, m->v1));
			break;
		}
	}
}

LONG CDSPLConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	BaseObject *goal = tData->GetObjectLink(SPLC_TARGET,doc); if(!goal) return false;
	BaseContainer *sData = goal->GetDataInstance(); if(!sData) return false;
	
	if(!(goal->GetInfo() & OBJECT_ISSPLINE)) return false;
	
	Bool unif = false;
	if(sData->GetLong(SPLINEOBJECT_INTERPOLATION) == SPLINEOBJECT_INTERPOLATION_UNIFORM) unif = true;
	
	Matrix goalM = goal->GetMg(), opM = op->GetMg();
	Matrix localM = MInv(goalM) * opM, transM;
	Vector sPt, newRot, setRot, opScale = CDGetScale(op), opRot = CDGetRot(op);
	LONG seg = tData->GetLong(SPLC_SEGMENT);
	Real n, sLen;
	
	SplineObject *spl = goal->GetRealSpline();
	if(!spl) return false;
	
	LONG splineSegments = spl->GetSegmentCount();
	if(seg > splineSegments - 1)
	{
		seg = splineSegments - 1;
		if(seg < 0) seg = 0;
		tData->GetLong(SPLC_SEGMENT,seg);
	}
	
	sLen = CDGetSplineLength(spl); if(sLen == 0.0) return false;
	
	if(!tData->GetBool(SPLC_FORCE_POS))
	{
		n = GetNearestSplinePoint(spl,localM.off,seg,unif);
		tData->SetReal(SPLC_POSITION,n);
	}
	else
	{
		n = tData->GetReal(SPLC_POSITION);
		
		if(spl->IsClosed())
		{
			if(n > 1.0) while(n > 1.0) n -= 1.0;
			if(n < 0.0) while(n < 0.0) n += 1.0;
		}
		else
		{
			if(n > 1.0) n = 1.0;
			if(n < 0.0) n = 0.0;
		}
	}
	if(unif) sPt = spl->GetSplinePoint(CDUniformToNatural(spl,n),seg);
	else sPt = spl->GetSplinePoint(n,seg);
	
	Vector strtPt = spl->GetSplinePoint(0,seg);
	Vector endPt = spl->GetSplinePoint(1,seg);
	
	localM.off = sPt;
	if(tData->GetBool(SPLC_TANGENT))
	{
		GetMatrixOrientation(doc,tag,spl,&localM,sPt,n,unif);
	}
	transM = goalM * localM;
	op->SetMg(transM);
	
	newRot = CDGetRot(op);
	setRot = CDGetOptimalAngle(opRot, newRot, op);
	
	CDSetRot(op,setRot);
	CDSetScale(op,opScale);
	//op->Message(MSG_UPDATE);
	
	if(tData->GetBool(SPLC_ALIGN_CHILDREN))
	{
		Real offset = tData->GetReal(SPLC_CHILD_OFFSET);
		BaseObject *ch = op->GetDown();
		
		Vector prPos = transM.off;
		Vector frstPt = spl->GetSplinePoint(0.01,seg);
		Vector offDir = VNorm(strtPt - frstPt);
		
		Bool prOffEnd = false;
		
		LONG chNum = 1;
		while(ch)
		{
			Vector chScale = CDGetScale(ch);
			n -= offset/sLen;
			
			if(spl->IsClosed())
			{
				if(n < 0.0) n += 1.0;
				else if(n > 1.0) n -= 1.0;
			}
			if(unif) sPt = spl->GetSplinePoint(CDUniformToNatural(spl,n),seg);
			else sPt = spl->GetSplinePoint(n,seg);
			
			Matrix chM = ch->GetMg();
			localM = MInv(goalM) * chM;
			
			if(!spl->IsClosed())
			{
				if(n >= 0.0) localM.off = sPt;
				if(n < 0.0)
				{
					if(!prOffEnd)
					{
						Real length = offset - Len(strtPt - prPos);
						localM.off = strtPt + offDir * length;
					}
					else
					{
						localM.off = prPos + offDir * offset;
					}
					prOffEnd = true;
				}
			}
			else localM.off = sPt;
			
			if(tData->GetBool(SPLC_TANGENT))
			{
				GetMatrixOrientation(doc,tag,spl,&localM,sPt,n,unif);
			}
			
			chM = goalM * localM;
			ch->SetMg(chM);
			CDSetScale(ch,chScale);
			//ch->Message(MSG_UPDATE);
			
			prPos = chM.off;
			
			ch = ch->GetDown();
			chNum++;
		}
	}

	return CD_EXECUTION_RESULT_OK;
}

Bool CDSPLConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseDocument *doc = node->GetDocument();
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SPLC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	BaseObject *goal = tData->GetObjectLink(SPLC_TARGET,doc);
	if(goal)
	{
		bc = description->GetParameterI(DescLevel(SPLC_SEGMENT), ar);
		if(bc)
		{
			SplineObject *spl = goal->GetRealSpline();
			if(spl)
			{
				LONG segCnt = spl->GetSegmentCount();
				if(segCnt-1 < 0) bc->SetReal(DESC_MAX, segCnt);
				else bc->SetReal(DESC_MAX, segCnt-1);
			}
			else bc->SetReal(DESC_MAX, 0.0);
		}
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSPLConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case SPLC_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLC_UP_VECTOR:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLC_TANGENT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLC_ALIGN_AXIS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLC_ALIGN_CHILDREN:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLC_CHILD_OFFSET:
			return tData->GetBool(SPLC_ALIGN_CHILDREN);
		case SPLC_FORCE_POS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SPLC_POSITION:
			return tData->GetBool(SPLC_FORCE_POS);
	}
	return true;
}

Bool RegisterCDSPLConstraintPlugin(void)
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
	String name=GeLoadString(IDS_CDSPLCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSPLCONSTRAINTPLUGIN,name,info,CDSPLConstraintPlugin::Alloc,"tCDSPLConstraint","CDSPLConstraint.tif",0);
}

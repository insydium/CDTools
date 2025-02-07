//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_description.h"
#include "lib_sds.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDNConstraint.h"
#include "CDConstraint.h"

enum
{
	//NC_PURCHASE			= 1000,
	
	//NC_STRENGTH			= 1005,
	//NC_SHOW_LINES			= 1006,
	//NC_LINE_COLOR			= 1007,
	
	//NC_TARGET			= 1010,
	
	//NC_N_SET				= 1020,
	//NC_N_RELEASE			= 1021,
	//NC_POS_ONLY			= 1022,
	
	//NC_ID_TARGET			= 1050,

	NC_N_IS_SET				= 3000,

	NC_R_MATRIX				= 3001,
	NC_B_COORDS				= 3002,
	NC_POLY_INDEX				= 3003,
	NC_POLY_TRI				= 3004,
	
	NC_PG_SCALE					= 3100
};

class CDNConstraintPlugin : public CDTagData
{
private:
	Vector drawPos;
	
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	LONG GetPointPolyTri(Vector *padr, CPolygon ply, Vector pt);
	Matrix GetPolyTriMatrix(Vector a, Vector b, Vector c);
	Bool SetNailPosition(BaseDocument *doc, BaseTag *tag, BaseContainer *tData, BaseObject *op, BaseObject *goal);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDNConstraintPlugin); }
};

Bool CDNConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	tData->SetBool(NC_SHOW_LINES,false);
	tData->SetVector(NC_LINE_COLOR,Vector(1,0,0));
	tData->SetReal(NC_STRENGTH,1.0);

	tData->SetBool(NC_N_IS_SET,false);
	
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

Bool CDNConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseContainer *tData = tag->GetDataInstance();
	BaseDocument *doc = bh->GetDocument();
	BaseObject *trg = tData->GetObjectLink(NC_TARGET,doc); if(!trg) return true;
	if(!trg) return true;
	if(trg == op) return true;
	
	BaseObject *goal = GetPolygonalObject(trg);
	if(!goal) return true;
	
	if(!tData->GetBool(NC_SHOW_LINES)) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());

	if(!tData->GetBool(NC_N_IS_SET)) SetNailPosition(doc,tag,tData,op,trg);
		
	bd->SetPen(tData->GetVector(NC_LINE_COLOR));
	CDDrawHandle3D(bd,drawPos,CD_DRAWHANDLE_MIDDLE,0);
	CDDrawLine(bd,drawPos,op->GetMg().off);
	
	return true;
}

Matrix CDNConstraintPlugin::GetPolyTriMatrix(Vector a, Vector b, Vector c)
{
	Matrix m;
	
	m.off = a;
	m.v3 = VNorm(b-a);
	m.v1 = VNorm(c-a);
	m.v2 = VNorm(VCross(m.v3, m.v1));
	m.v1 = VNorm(VCross(m.v2, m.v3));
	
	return m;
}

LONG CDNConstraintPlugin::GetPointPolyTri(Vector *padr, CPolygon ply, Vector pt)
{
	Vector a,b,c,d,ptA,ptB;
	
	a = padr[ply.a];
	b = padr[ply.b];
	c = padr[ply.c];
	d = padr[ply.d];
	
	ptA = ClosestPointOnTriangle(pt,a,b,c);
	if(ply.c != ply.d)
	{
		ptB = ClosestPointOnTriangle(pt,c,d,a);
		Real aLen = Len(ptA-pt), bLen = Len(ptB-pt);
		if(aLen < bLen) return 0;
		else return 1;
	}
	else return 0;
}

Bool CDNConstraintPlugin::SetNailPosition(BaseDocument *doc, BaseTag *tag, BaseContainer *tData, BaseObject *op, BaseObject *goal)
{
	if(!goal) return false;
	
	Vector *padr = GetPointArray(goal); if(!padr) return false;
	CPolygon *vadr = GetPolygonArray(goal); if(!vadr) return false;
	
	LONG i, pInd = 0, vCnt = ToPoly(goal)->GetPolygonCount();
	if(vCnt < 1) return false;
	
	Matrix plyM, opM = op->GetMg(), glM = goal->GetMg();
	Vector posL = MInv(glM) * opM.off, bryC;
	
	CPolygon ply;
	Vector plyPt;
	
	Real dist = CDMAXREAL;
	for(i=0; i<vCnt; i++)
	{
		ply = vadr[i];
		plyPt = ClosestPointOnPolygon(padr,ply,posL);
		
		Real pLen = Len(plyPt - posL);
		if(pLen < dist)
		{
			dist = pLen;
			pInd = i;
		}
	}
	
	ply = vadr[pInd];
	plyPt = ClosestPointOnPolygon(padr,ply,posL);
	
	LONG plyTri = GetPointPolyTri(padr,ply,plyPt);
	
	Vector aPt = padr[ply.a];
	Vector bPt = padr[ply.b];
	Vector cPt = padr[ply.c];
	Vector dPt = padr[ply.d];
	
	switch(plyTri)
	{
		case 0:
			plyM = GetPolyTriMatrix(aPt,bPt,cPt);
			bryC = GetBarycentricCoords(aPt,bPt,cPt,plyPt);
			plyM.off = (aPt * bryC.x) + (bPt * bryC.y) + (cPt * bryC.z);
			break;
		case 1:
			plyM = GetPolyTriMatrix(dPt,aPt,cPt);
			bryC = GetBarycentricCoords(aPt,cPt,dPt,plyPt);
			plyM.off = (aPt * bryC.x) + (cPt * bryC.y) + (dPt * bryC.z);
			break;
	}
	if(!tData->GetBool(NC_N_IS_SET)) drawPos = glM * plyM.off;
	
	CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
	tData->SetLong(NC_POLY_TRI,plyTri);
	tData->SetLong(NC_POLY_INDEX,pInd);
	tData->SetVector(NC_B_COORDS,bryC);
	tData->SetMatrix(NC_R_MATRIX,MInv(plyM) * (MInv(glM) * opM));
	
	return true;
}

void CDNConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLink(NC_TARGET+T_LST,tData->GetLink(NC_TARGET,doc));
			tData->SetBool(NC_POS_ONLY+T_LST,tData->GetBool(NC_POS_ONLY));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDNConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
	
	BaseObject *trg = tData->GetObjectLink(NC_TARGET,doc);
	BaseObject *goal = GetPolygonalObject(trg);
	
	if(tData->GetBool(NC_N_IS_SET))
	{
		if(goal)
		{
			LONG pInd = tData->GetLong(NC_POLY_INDEX);
			LONG vCnt = ToPoly(goal)->GetPolygonCount();
			if(pInd > vCnt-1)
			{
				if(SetNailPosition(doc,tag,tData,op,goal))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
					tData->SetBool(NC_N_IS_SET,true);
				}
			}
		}
		else tData->SetBool(NC_N_IS_SET,false);
	}
	
	switch (type)
	{
		case ID_CDFREEZETRANSFORMATION:
		{
			Vector *trnsSca = (Vector*)data;
			if(trnsSca)
			{
				if(goal)
				{
					Vector gScale = GetGlobalScale(goal);
					if(!VEqual(gScale,tData->GetVector(NC_PG_SCALE),0.001))
					{
						Matrix refM = MatrixScale(tData->GetVector(NC_PG_SCALE)) * tData->GetMatrix(NC_R_MATRIX);
						tData->SetMatrix(NC_R_MATRIX,refM);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==NC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==NC_N_SET)
			{
				if(tData->GetBool(T_REG))
				{
					if(goal)
					{
						if(SetNailPosition(doc,tag,tData,op,goal))
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
							tData->SetBool(NC_N_IS_SET,true);
						}
					}
				}
			}
			else if(dc->id[0].id==NC_N_RELEASE)
			{
				if(tData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
					tData->SetBool(NC_N_IS_SET,false);
				}
			}
			break;
		}
	}

	return true;
}

Bool CDNConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(NC_TARGET,tData->GetLink(NC_TARGET+T_LST,doc));
		tData->SetBool(NC_POS_ONLY,tData->GetBool(NC_POS_ONLY+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDNConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	BaseObject *goal = NULL, *opDeform = NULL, *trg = tData->GetObjectLink(NC_TARGET,doc);
	if(!trg) return false;
	if(trg == op) return false;
	
	goal = GetPolygonalObject(trg);
	if(!goal) return false;
	
	LONG vCnt = ToPoly(goal)->GetPolygonCount(), pInd = tData->GetLong(NC_POLY_INDEX);
	if(pInd > vCnt-1) tData->SetBool(NC_N_IS_SET,false);

	if(!tData->GetBool(NC_N_IS_SET)) return false;

	Matrix refM = tData->GetMatrix(NC_R_MATRIX), plyM, opM = op->GetMg(), glM, trgM;
	Vector opSca = CDGetScale(op), bCoords = tData->GetVector(NC_B_COORDS);//
	Vector oldRot = CDGetRot(op), newRot, aPt, bPt, cPt, dPt, nPos = Vector();
		
	opDeform = trg->GetDeformCache();
	if(opDeform) goal = opDeform;
	
	Vector *padr = GetPointArray(goal); if(!padr) return false;
	CPolygon *vadr = GetPolygonArray(goal); if(!vadr) return false;
	CPolygon ply = vadr[pInd];
	
	Vector *sadr = NULL;
	BaseObject *tempOp=NULL, *hnPr = GetHNParentObject(trg);
	if(hnPr)
	{
		if(hnPr->GetDeformMode())
		{
			SDSObject *sdsOp = static_cast<SDSObject*>(hnPr);
			if(sdsOp)
			{
				PolygonObject *sdsMesh = sdsOp->GetSDSMesh(trg);
				if(sdsMesh) sadr = GetPointArray(sdsMesh);
			}
		}
	}
	
	glM = goal->GetMg();
	if(sadr)
	{
		aPt = sadr[ply.a];
		bPt = sadr[ply.b];
		cPt = sadr[ply.c];
		dPt = sadr[ply.d];
	}
	else
	{
		aPt = padr[ply.a];
		bPt = padr[ply.b];
		cPt = padr[ply.c];
		dPt = padr[ply.d];
	}
	switch(tData->GetLong(NC_POLY_TRI))
	{
		case 0:
		{
			plyM = GetPolyTriMatrix(aPt,bPt,cPt);
			nPos += aPt * bCoords.x;
			nPos += bPt * bCoords.y;
			nPos += cPt * bCoords.z;
			plyM.off = nPos;
			break;
		}
		case 1:
		{
			plyM = GetPolyTriMatrix(dPt,aPt,cPt);
			nPos += aPt * bCoords.x;
			nPos += cPt * bCoords.y;
			nPos += dPt * bCoords.z;
			plyM.off = nPos;
			break;
		}
	}
	drawPos = glM * plyM.off;
	
	if(tData->GetBool(NC_POS_ONLY))
	{
		Vector opPos = glM * (plyM * refM.off);
		trgM = op->GetMg();
		trgM.off = opPos;
	}
	else trgM = glM * (plyM * refM);
	
	Real t = tData->GetReal(NC_STRENGTH);
	Matrix transM = trgM;
	transM.off = CDBlend(opM.off,trgM.off,t);
	
	if(!MatrixEqual(opM,transM,0.001))
	{
		op->SetMg(transM);
		newRot = CDGetRot(op);
		CDSetRot(op,CDGetOptimalAngle(oldRot, newRot, op));
		CDSetScale(op,opSca);
	}
	
	Vector gScale = GetGlobalScale(goal);
	tData->SetVector(NC_PG_SCALE,gScale);
	if(tempOp) BaseObject::Free(tempOp);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDNConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(NC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDNConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument 	*doc = node->GetDocument(); if(!doc) return false;
	BaseObject *trg = tData->GetObjectLink(NC_TARGET,doc);
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case NC_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case NC_N_SET:
			if(!tData->GetBool(T_REG)) return false;
			else if(trg)
			{
				BaseObject *goal = GetPolygonalObject(trg);
				if(goal)
				{
					if(!tData->GetBool(NC_N_IS_SET)) return true;
					else return false;
				}
				else return false;
			}
			else return false;
		case NC_N_RELEASE:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(NC_N_IS_SET);
		case NC_POS_ONLY:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDNConstraintPlugin(void)
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
	String name=GeLoadString(IDS_CDNCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDNCONSTRAINTPLUGIN,name,info,CDNConstraintPlugin::Alloc,"tCDNConstraint","CDNConstraint.tif",0);
}

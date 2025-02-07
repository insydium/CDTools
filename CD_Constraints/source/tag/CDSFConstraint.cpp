//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_description.h"
#include "lib_sds.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDSFConstraint.h"
#include "CDConstraint.h"

/*enum
{
	//SFC_TARGET				= 1010,
	//SFC_STRENGTH				= 1011,
	
	//SFC_ALIGN_NORMAL			= 1020,
	//SFC_PURCHASE				= 1030,
};*/

class CDSFConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	LONG GetPointPolyTri(Vector *padr, CPolygon ply, Vector pt);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSFConstraintPlugin); }
};

Bool CDSFConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
			
	tData->SetBool(SFC_ALIGN_NORMAL, false);
	tData->SetReal(SFC_STRENGTH, 1.0);
	
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

void CDSFConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLink(SFC_TARGET+T_LST,tData->GetLink(SFC_TARGET,doc));
			tData->SetBool(SFC_ALIGN_NORMAL+T_LST,tData->GetBool(SFC_ALIGN_NORMAL));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSFConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			if(dc->id[0].id==SFC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

LONG CDSFConstraintPlugin::GetPointPolyTri(Vector *padr, CPolygon ply, Vector pt)
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

Bool CDSFConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(SFC_TARGET,tData->GetLink(SFC_TARGET+T_LST,doc));
		tData->SetBool(SFC_ALIGN_NORMAL,tData->GetBool(SFC_ALIGN_NORMAL+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDSFConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseObject *goal = NULL, *opDeform = NULL;
	BaseObject *trg = tData->GetObjectLink(SFC_TARGET,doc);
	if(!trg) return false;
	if(trg == op) return false;
	
	goal = GetPolygonalObject(trg);
	if(!goal) return false;

	AutoAlloc<GeRayCollider> rc; if (!rc) return false;

	Matrix opM = op->GetMg(), glM, rotM, plyM, pM, transM;
	Vector posL, theScale = CDGetScale(op), mixPosition = opM.off, phNorm;//
	Vector oldRot, newRot, rotSet;
	transM = opM;
		
	opDeform = trg->GetDeformCache();
	if(opDeform) goal = opDeform;
	
	BaseObject *hnPr = GetHNParentObject(trg);
	if(hnPr)
	{
		if(hnPr->GetDeformMode())
		{
			SDSObject *sdsOp = static_cast<SDSObject*>(hnPr);
			if(sdsOp)
			{
				PolygonObject *sdsMesh = sdsOp->GetSDSMesh(trg);
				if(sdsMesh) goal = sdsMesh;
			}
		}
	}
	
	glM = goal->GetMg();
	rotM = glM;
	rotM.off = Vector(0,0,0);
	posL = MInv(glM) * opM.off;
	
	Vector *padr = GetPointArray(goal);
	CPolygon *vadr = GetPolygonArray(goal);
	LONG i, pInd = 0, plyCnt = ToPoly(goal)->GetPolygonCount();
	
	CPolygon ply;

	Vector plyPt;
	posL = MInv(glM) * opM.off;
	Real dist = CDMAXREAL;
	for(i=0; i<plyCnt; i++)
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
	mixPosition = glM * plyPt;
	
	Vector plyNormal = CalcFaceNormal(padr,ply);
	plyM.off = glM * CalcPolygonCenter(padr,ply);
	plyM.v2 = VNorm(rotM * plyNormal);
	plyM.v1 = VNorm(rotM * (padr[ply.b] - padr[ply.a]));
	plyM.v3 = VNorm(VCross(plyM.v1, plyM.v2));
	plyM.v1 = VNorm(VCross(plyM.v2, plyM.v3));
	
	
	if(tData->GetReal(SFC_ALIGN_NORMAL))
	{
		transM = plyM;
		if(goal->GetTag(Tphong))
		{
			LONG half = GetPointPolyTri(padr,ply,plyPt);
			Vector aN, bN, cN, dN;
			Vector brC, dirN = transM.v2;

		#if API_VERSION < 12000
			Vector *phN = ToPoly(goal)->CreatePhongNormals();
			
			aN = VNorm(phN[pInd*4]);
			bN = VNorm(phN[pInd*4+1]);
			cN = VNorm(phN[pInd*4+2]);
			dN = VNorm(phN[pInd*4+3]);
		#else
			SVector *phN = ToPoly(goal)->CreatePhongNormals();
			
            #if API_VERSION < 15000
                aN = VNorm(phN[pInd*4].ToRV());
                bN = VNorm(phN[pInd*4+1].ToRV());
                cN = VNorm(phN[pInd*4+2].ToRV());
                dN = VNorm(phN[pInd*4+3].ToRV());
            #else
                aN = VNorm((Vector)phN[pInd*4]);
                bN = VNorm((Vector)phN[pInd*4+1]);
                cN = VNorm((Vector)phN[pInd*4+2]);
                dN = VNorm((Vector)phN[pInd*4+3]);
            #endif
		#endif
			if(phN) CDFree(phN);
			
			
			switch(half)
			{
				case 0:
					brC = GetBarycentricCoords(padr[ply.a],padr[ply.b],padr[ply.c],plyPt);
					dirN = VNorm(aN * brC.x + bN * brC.y + cN * brC.z);
					break;
				case 1:
					brC = GetBarycentricCoords(padr[ply.a],padr[ply.c],padr[ply.d],plyPt);
					dirN = VNorm(aN * brC.x + cN * brC.y + dN * brC.z);
					break;
			}
			
			transM.v2 = VNorm(rotM * dirN);
			transM.v1 = VNorm(VCross(transM.v2, transM.v3));
			transM.v3 = VNorm(VCross(transM.v1, transM.v2));
			
		}
	}
	
	Real strength = tData->GetReal(SFC_STRENGTH);
	transM.off = Mix(transM.off, mixPosition, strength);
	
	oldRot = CDGetRot(op);
	op->SetMg(transM);
	newRot = CDGetRot(op);
	rotSet = CDGetOptimalAngle(oldRot, newRot, op);
	CDSetRot(op,rotSet);
	CDSetScale(op,theScale);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSFConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *data = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SFC_PURCHASE), ar);
	if(bc)
	{
		if(!data->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSFConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case SFC_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SFC_ALIGN_NORMAL:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetObjectLink(SFC_TARGET,doc)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDSFConstraintPlugin(void)
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
	String name=GeLoadString(IDS_CDSFCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSFCONSTRAINTPLUGIN,name,info,CDSFConstraintPlugin::Alloc,"tCDSFConstraint","CDSFConstraint.tif",0);
}

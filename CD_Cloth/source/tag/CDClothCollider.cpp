//
//  CDColliderTag.cpp
//	Cactus Dan's Cloth
//	Copyright 2013 by Cactus Dan Libisch
//
#include "c4d_symbols.h"

#include "CDClothCollider.h"
//#include "CDClothFuncLib.h"
//#include "CDDebug.h"

#include "oCDJoint.h"

Bool CDClothCollider::Init(GeListNode *node)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(CLD_COLLIDER_TYPE, CLD_SPHERE);
	tData->SetReal(CLD_RADIUS, 100);
	tData->SetLong(CLD_PLANE_NORM, CLD_NORM_YP);
	
	tData->SetBool(CLD_SELECTION_SET, false);
	tData->SetLong(CLD_SELECTION_COUNT, 0);
	
	tData->SetBool(CLD_POLYGON_OBJECT, false);
	
	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_EXPRESSION);
			pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,250);
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDCLData cld;
	PluginMessage(ID_CDCLOTHCOLLIDER,&cld);
	if(cld.list) cld.list->Append(node);
	
	return true;
}

void CDClothCollider::Free(GeListNode *node)
{
	plyInd.Free();
	
	FreeSpacePartitions();
	
	CDCLData cld;
	PluginMessage(ID_CDCLOTHCOLLIDER,&cld);
	if(cld.list) cld.list->Remove(node);
}

Bool CDClothCollider::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	hf->ReadReal(&bounds.min.x);
	hf->ReadReal(&bounds.min.y);
	hf->ReadReal(&bounds.min.z);
	hf->ReadReal(&bounds.max.x);
	hf->ReadReal(&bounds.max.y);
	hf->ReadReal(&bounds.max.z);
	
	if(tData->GetBool(CLD_POLYGON_OBJECT))
	{
		LONG i, pCnt = tData->GetLong(CLD_SELECTION_COUNT);
		if(pCnt > 0)
		{
			if(!plyInd.Alloc(pCnt)) return false;
			for(i=0; i<pCnt; i++)
			{
				hf->ReadLong(&plyInd[i]);
			}
		}
	}
	return true;
}

Bool CDClothCollider::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	//Level 1
	hf->WriteReal(bounds.min.x);
	hf->WriteReal(bounds.min.y);
	hf->WriteReal(bounds.min.z);
	hf->WriteReal(bounds.max.x);
	hf->WriteReal(bounds.max.y);
	hf->WriteReal(bounds.max.z);
	
	if(tData->GetBool(CLD_POLYGON_OBJECT))
	{
		LONG i, pCnt = tData->GetLong(CLD_SELECTION_COUNT);
		if(pCnt > 0)
		{
			for(i=0; i<pCnt; i++)
			{
				hf->WriteLong(plyInd[i]);
			}
		}
	}
	return true;
}

Bool CDClothCollider::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag  = (BaseTag*)snode; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	((CDClothCollider*)dest)->bounds.min = bounds.min;
	((CDClothCollider*)dest)->bounds.max = bounds.max;
	
	if(tData->GetBool(CLD_POLYGON_OBJECT))
	{
		spcA.Copy(((CDClothCollider*)dest)->spcA);
		spcB.Copy(((CDClothCollider*)dest)->spcB);
		spcC.Copy(((CDClothCollider*)dest)->spcC);
		spcD.Copy(((CDClothCollider*)dest)->spcD);
		spcE.Copy(((CDClothCollider*)dest)->spcE);
		spcF.Copy(((CDClothCollider*)dest)->spcF);
		spcG.Copy(((CDClothCollider*)dest)->spcG);
		spcH.Copy(((CDClothCollider*)dest)->spcH);
		
		LONG pCnt = tData->GetLong(CLD_SELECTION_COUNT);
		if(pCnt > 0)
		{
			plyInd.Copy(((CDClothCollider*)dest)->plyInd);
		}
	}
	
	return true;
}

Bool CDClothCollider::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	return true;
}

void CDClothCollider::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	//GePrint("CDClothCollider::CheckTagReg()");
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdclnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdclnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdclnr.FindFirst("-",&pos);
	while(h)
	{
		cdclnr.Delete(pos,1);
		h = cdclnr.FindFirst("-",&pos);
	}
	cdclnr.ToUpper();
	
	kb = cdclnr.SubStr(pK,2);
	
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
			
			//tData->SetLink(TARGET+T_LST,tData->GetLink(TARGET,doc));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDClothCollider::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDClothCollider::FreeSpacePartitions(void)
{
	spcA.Free();
	spcB.Free();
	spcC.Free();
	spcD.Free();
	spcE.Free();
	spcF.Free();
	spcG.Free();
	spcH.Free();
}

Bool CDClothCollider::BuildSpacePartitions(BaseObject *op)
{
	FreeSpacePartitions();
	
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, cnt = ToPoint(op)->GetPointCount();
	Vector mp = op->GetMp();
	
	for(i=0; i<cnt; i++)
	{
		Vector pt = padr[i];
		if(pt.x > mp.x)
		{
			if(pt.y > mp.y)
			{
				if(pt.z > mp.z) spcF.Append(i);
				else spcE.Append(i);
			}
			else
			{
				if(pt.z > mp.z) spcH.Append(i);
				else spcG.Append(i);
			}
		}
		else
		{
			if(pt.y > mp.y)
			{
				if(pt.z > mp.z) spcB.Append(i);
				else spcA.Append(i);
			}
			else
			{
				if(pt.z > mp.z) spcD.Append(i);
				else spcC.Append(i);
			}
		}
	}
	
	return true;
}

LONG CDClothCollider::GetNearestPointIndex(Vector pt, BaseObject *op)
{
	Real prvDif = CDMAXREAL;
	LONG i, ind = -1;
	Vector *padr = GetPointArray(op);
	if(padr)
	{
		Vector mp = op->GetMp();
		if(pt.x > mp.x)
		{
			if(pt.y > mp.y)
			{
				if(pt.z > mp.z)
				{
					for(i=0; i<spcF.Size(); i++)
					{
						Vector opPt = padr[spcF[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcF[i];
							prvDif = dif;
						}
					}
				}
				else
				{
					for(i=0; i<spcE.Size(); i++)
					{
						Vector opPt = padr[spcE[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcE[i];
							prvDif = dif;
						}
					}
				}
			}
			else
			{
				if(pt.z > mp.z)
				{
					for(i=0; i<spcH.Size(); i++)
					{
						Vector opPt = padr[spcH[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcH[i];
							prvDif = dif;
						}
					}
				}
				else
				{
					for(i=0; i<spcG.Size(); i++)
					{
						Vector opPt = padr[spcG[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcG[i];
							prvDif = dif;
						}
					}
				}
			}
		}
		else
		{
			if(pt.y > mp.y)
			{
				if(pt.z > mp.z)
				{
					for(i=0; i<spcB.Size(); i++)
					{
						Vector opPt = padr[spcB[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcB[i];
							prvDif = dif;
						}
					}
				}
				else
				{
					for(i=0; i<spcA.Size(); i++)
					{
						Vector opPt = padr[spcA[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcA[i];
							prvDif = dif;
						}
					}
				}
			}
			else
			{
				if(pt.z > mp.z)
				{
					for(i=0; i<spcD.Size(); i++)
					{
						Vector opPt = padr[spcD[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcD[i];
							prvDif = dif;
						}
					}
				}
				else
				{
					for(i=0; i<spcC.Size(); i++)
					{
						Vector opPt = padr[spcC[i]];
						Real dif = LenSquare(pt,opPt);
						if(dif < prvDif)
						{
							ind = spcC[i];
							prvDif = dif;
						}
					}
				}
			}
		}
	}
	
	return ind;
}

LONG CDClothCollider::GetNearestPolygonIndex(Vector pt, Vector *padr, LONG pCnt, BaseObject *op, CPolygon *vadr, LONG vCnt)
{
	LONG ptIndex = GetNearestPointIndex(pt,op);
	LONG *dadr = NULL, dcnt = 0, p, plyInd = -1;
	
	Neighbor n;
	n.Init(pCnt,vadr,vCnt,NULL);
	n.GetPointPolys(ptIndex, &dadr, &dcnt);
	
	CPolygon ply;
	Vector plyPt;
	Real dist = CDMAXREAL;
	for(p=0; p<dcnt; p++)
	{
		ply = vadr[dadr[p]];
		plyPt = ClosestPointOnPolygon(padr,ply,pt);
		
		Real pLen = Len(plyPt - pt);
		if(pLen < dist)
		{
			dist = pLen;
			plyInd = dadr[p];
		}
	}
	
	return plyInd;
}

Vector CDClothCollider::GetNearestSurfacePoint(BaseObject *op, Vector pt, Vector &n)
{
	Vector res = pt;
	Matrix opM = op->GetMg();
	
	Vector *padr = GetPointArray(op);
	CPolygon *vadr = GetPolygonArray(op);
	
	if(padr && vadr)
	{
		LONG pCnt = ToPoint(op)->GetPointCount();
		LONG vCnt = ToPoly(op)->GetPolygonCount();
		
		Vector lPt = MInv(opM) * pt;
		LONG plyInd = GetNearestPolygonIndex(lPt,padr,pCnt,op,vadr,vCnt);
		
		CPolygon ply = vadr[plyInd];
		n = CalcFaceNormal(padr, ply);
		
		Vector srfPt = ClosestPointOnPolygon(padr,ply,lPt);
		res = opM * srfPt;
	}
	
	return res;
}

Bool CDClothCollider::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;

	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDCL_SERIAL_SIZE];
			String cdclnr;
			
			CDReadPluginInfo(ID_CDCLOTH,snData,CDCL_SERIAL_SIZE);
			cdclnr.SetCString(snData,CDCL_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdclnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDCL_SERIAL_SIZE];
			String cdclnr;
			
			CDReadPluginInfo(ID_CDCLOTH,snData,CDCL_SERIAL_SIZE);
			cdclnr.SetCString(snData,CDCL_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdclnr);
			break;
		}
	}
	BaseDocument *doc = tag->GetDocument(); if(!doc) return false;
	
	//ListReceivedMessages("CDClothTagPlugin",type,data);
	
	CheckTagReg(doc,op,tData);
	
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			if(IsValidPolygonObject(op))
			{
				BuildSpacePartitions(op);
				tData->SetBool(CLD_POLYGON_OBJECT, true);
			}
			break;
		}
		case MSG_MENUPREPARE:
		{
			if(IsValidPolygonObject(op))
			{
				BuildSpacePartitions(op);
				tData->SetBool(CLD_POLYGON_OBJECT, true);
			}
			break;
		}
		case MSG_POINTS_CHANGED:
		{
			if(IsValidPolygonObject(op))
			{
				BuildSpacePartitions(op);
				tData->SetBool(CLD_POLYGON_OBJECT, true);
			}
			break;
		}
		case MSG_TRANSLATE_POINTS:
		{
			if(IsValidPolygonObject(op))
			{
				BuildSpacePartitions(op);
				tData->SetBool(CLD_POLYGON_OBJECT, true);
			}
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == CLD_COLLIDER_TYPE)
			{
				if(tData->GetLong(CLD_COLLIDER_TYPE) == CLD_POLYGON && !tData->GetBool(CLD_POLYGON_OBJECT))
				{
					if(IsValidPolygonObject(op))
					{
						BuildSpacePartitions(op);
						tData->SetBool(CLD_POLYGON_OBJECT, true);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==CLD_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==CLD_SET_SELECTION)
			{
				
			}
			else if(dc->id[0].id==CLD_EDIT_SELECTION)
			{
				
			}
			else if(dc->id[0].id==CLD_RESTORE_SELECTION)
			{
				
			}
			break;
		}

	}
	return true;
}

LONG CDClothCollider::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	Matrix opM = op->GetMg();
	Vector mp, rad;
	
	if(IsValidPointObject(op))
	{
		mp = op->GetMp();
		rad = op->GetRad();
	}
	else
	{
		if(op->GetType() == ID_CDJOINTOBJECT && tData->GetLong(CLD_COLLIDER_TYPE) == CLD_JOINT)
		{
			mp = op->GetMp();
			rad = op->GetRad();
		}
		else
		{
			mp = Vector();
			Real r = tData->GetReal(CLD_RADIUS);
			rad = Vector(r,r,r);
		}
	}
	
	bounds.Empty();
	bounds.AddPoint(mp - rad);
	bounds.AddPoint(mp + rad);
	
	if(op->GetType() == ID_CDJOINTOBJECT && tData->GetLong(CLD_COLLIDER_TYPE) == CLD_JOINT)
	{
		BaseObject *ch = op->GetDown();
		if(ch)
		{
			Matrix chM = ch->GetMg();
			mp = MInv(opM) * chM * ch->GetMp();
			rad = ch->GetRad();
			
			bounds.AddPoint(mp - rad);
			bounds.AddPoint(mp + rad);
		}
	}
			
	return CD_EXECUTION_RESULT_OK;
}

Bool CDClothCollider::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(CLD_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDClothCollider::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case CLD_COLLIDER_TYPE:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case CLD_RADIUS:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetLong(CLD_COLLIDER_TYPE) == CLD_SPHERE) return true;
				else if(tData->GetLong(CLD_COLLIDER_TYPE) == CLD_JOINT) return true;
				else return false;
			}
		case CLD_OFFSET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case CLD_PLANE_NORM:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetLong(CLD_COLLIDER_TYPE) == CLD_PLANE) return true;
				else return false;
			}
		case CLD_SET_SELECTION:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!IsValidPolygonObject(op)) return false;
				else return true;
			}
		case CLD_EDIT_SELECTION:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!IsValidPolygonObject(op)) return false;
				else return true;
			}
		case CLD_RESTORE_SELECTION:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!IsValidPolygonObject(op)) return false;
				else return true;
			}
	}
	
	return true;
}

Bool RegisterCDClothColliderTagPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDCL_SERIAL_SIZE];
	String cdclnr, kb;
	
	if(!CDReadPluginInfo(ID_CDCLOTH,data,CDCL_SERIAL_SIZE)) reg = false;
	
	cdclnr.SetCString(data,CDCL_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdclnr)) reg = false;
	
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdclnr.FindFirst("-",&pos);
	while(h)
	{
		cdclnr.Delete(pos,1);
		h = cdclnr.FindFirst("-",&pos);
	}
	cdclnr.ToUpper();
	kb = cdclnr.SubStr(pK,2);
	
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
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;
	
	// decide by name if the plugin shall be registered - just for user convenience  
	String name=GeLoadString(IDS_CDCLDRTAG); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDCLOTHCOLLIDER,name,info,CDClothCollider::Alloc,"tCDClothCollider","CDClothCollider.tif",0);
}

// library functions
CDAABB* iCDColliderGetBounds(BaseTag *tag)
{
	CDClothCollider *cldTag = static_cast<CDClothCollider*>(tag->GetNodeData());
	return cldTag ? cldTag->GetBounds() : NULL;
}

//Vector iCDGetNearestSurfacePoint(BaseTag *tag, BaseObject *op, Vector pt, Vector &n)
Vector iCDGetNearestSurfacePoint(CDClothCollider *cldTag, BaseObject *op, Vector pt, Vector &n)
{
	//CDClothCollider *cldTag = static_cast<CDClothCollider*>(tag->GetNodeData());
	return cldTag ? cldTag->GetNearestSurfacePoint(op, pt, n) : Vector();
}



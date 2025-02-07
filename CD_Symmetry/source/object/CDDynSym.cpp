//	Cactus Dan's Symmetry Tools 1.0
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "customgui_bitmapbutton.h"
#include "lib_collider.h"

#include "oCDDynSym.h"
#include "CDSymmetry.h"
#include "CDEdgeCutArray.h"
#include "CDObjectData.h"
#include "CDArray.h"

enum
{
	//DS_PURCHASE				= 1000,

	//DS_SHOW_GUIDE				= 1001,
	//DS_GUIDE_SIZE				= 1002,
	//DS_GUIDE_COLOR				= 1003,
	
	//DS_SYMMETRY_AXIS			= 1010,
		//DS_MX					= 1011,
		//DS_MY					= 1012,
		//DS_MZ					= 1013,
		
	//DS_TOLERANCE				= 1020,
	
	//DS_ACTIVE_SYMMETRY			= 1030,
		//DS_POSITIVE			= 1031,
		//DS_NEGATIVE			= 1032,
		
	//DS_AUTO_UPDATE				= 2000,
	//DS_LOCK_CENTER				= 2001,
	//DS_KEEP_SEL					= 2002,
	//DS_KEEP_UVW					= 2003
};

class CDDynamicSymmetry : public CDObjectData
{
private:
	CDArray<LONG>	indA, indB, indC, indD, indE, indF, indG, indH;
	
	void CheckOpReg(BaseContainer *oData);
	void AddChildBounds(BaseObject *op, CDAABB *bnd);
	LONG GetSymmetryAxis(BaseContainer *oData);
	
	void InitSpacePartitions(LONG cnt);
	void FreeSpacePartitions(void);
	LONG GetPartitionSpace(Vector pt, Vector mp);
	LONG* GetPartitionArray(LONG pSpace, LONG &cnt);
	
	Bool BuildPointSpacePartitions(Vector *padr, LONG cnt, Vector mp, LONG size);
	Bool BuildPolygonSpacePartitions(Vector *padr, CPolygon *vadr, LONG cnt, Vector mp, LONG size);
	Bool BuildPolySelSpacePartitions(Vector *padr, CPolygon *vadr, BaseSelect *bs, Vector mp, LONG size);
	
	void GetUVWTags(BaseObject *op, BaseObject *cln, AtomArray *oUVWTags, AtomArray *cUVWTags);
	void TransferUVWCoordinates(Vector *opadr, Vector *cpadr, CPolygon oPly, CPolygon cPly, UVWStruct &ouv, UVWStruct &cuv);
	Bool CompareUVWTags(BaseObject *op, BaseObject *cln, LONG axis, LONG dir);
	
	Bool AutoUpdateAvailable(BaseObject *op);
	Bool CompareSelections(BaseObject *op, BaseObject *cln);
	
	Bool IsCrossPlaneLength(Real a, Real b);
	LONG FindCutPoint(Vector *padr, CPolygon ply, LONG axis, CutEdge e);
	Bool IsCrossPlanePoly(Vector *padr, const CPolygon &ply, Matrix opM, Matrix prM, LONG axis, Real t);
	LONG GetCrossPlanePolyEdges(Vector *padr, CPolygon ply, CutEdge *edg, Matrix opM, Matrix prM, LONG axis);
	
	Bool DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis);
	Bool CenterCutObject(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis, Real t);
	Bool SplitObject(BaseContainer *oData, BaseDocument *doc, BaseObject *op, Matrix prM);
	Bool BuildSymmetry(HierarchyHelp *hh, BaseThread *bt, Matrix prM, BaseObject *pr, BaseObject *op, BaseContainer *oData, BaseDocument *doc);
		
public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode* node);
	virtual Bool Message(GeListNode *node, LONG type, void *t_data);

	virtual BaseObject* CDGetVirtualObjects(BaseObject* op, HierarchyHelp* hh);
	virtual Bool CDDraw(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDDynamicSymmetry); }
};

// initialize settings
Bool CDDynamicSymmetry::Init(GeListNode *node)
{
	BaseObject *op = (BaseObject*)node;
	BaseContainer *oData = op->GetDataInstance();

	oData->SetBool(DS_SHOW_GUIDE,false);
	oData->SetReal(DS_GUIDE_SIZE,50.0);
	oData->SetVector(DS_GUIDE_COLOR, Vector(1,0,0));
	
	oData->SetLong(DS_SYMMETRY_AXIS,DS_MX);
	oData->SetLong(DS_ACTIVE_SYMMETRY,DS_POSITIVE);
	oData->SetReal(DS_TOLERANCE,0.01);
	
	oData->SetBool(DS_AUTO_UPDATE,false);
	
	return true;
}

void CDDynamicSymmetry::CheckOpReg(BaseContainer *oData)
{
	if(oData)
	{
		Bool reg = true;
		
		LONG pK;
		CHAR aK, bK, cK;
		SetRValues(&pK,&aK,&bK,&cK);
		
		CHAR b;
		String kb, cdsnr = oData->GetString(T_STR);
		SerialInfo si;
		
		if(!CheckKeyChecksum(cdsnr)) reg = false;
		
		CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
		LONG seed = GetSeed(si.nr);
		
		LONG pos;
		Bool h = cdsnr.FindFirst("-",&pos);
		while(h)
		{
			cdsnr.Delete(pos,1);
			h = cdsnr.FindFirst("-",&pos);
		}
		cdsnr.ToUpper();
		
		kb = cdsnr.SubStr(pK,2);
		
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
		
		oData->SetBool(T_REG,reg);
		if(!reg)
		{
			if(!oData->GetBool(T_SET))
			{
				oData->SetBool(T_SET,true);
			}
		}
	}
}

void CDDynamicSymmetry::Free(GeListNode* node)
{
	BaseObject *op = (BaseObject*)node;
	if(!op)
	{
		BaseObject *ch = op->GetDown();
		if(ch)
		{
			BaseTag *clTag = ch->GetTag(ID_CDCENTERLOCKTAG);
			if(clTag) BaseTag::Free(clTag);
		}
	}
}

Bool CDDynamicSymmetry::AutoUpdateAvailable(BaseObject *op)
{
	BaseObject *ch = op->GetDown();
	
	if(!ch) return false;
	if(!IsValidPolygonObject(ch)) return false;
	if(ch->GetDown()) return false;
	
	return true;
}


void CDDynamicSymmetry::AddChildBounds(BaseObject *op, CDAABB *bnd)
{
	Matrix opM;
	
	while(op)
	{
		opM = op->GetMg();
		bnd->AddPoint(opM * (op->GetMp() + op->GetRad()));
		bnd->AddPoint(opM * (op->GetMp() - op->GetRad()));
		
		AddChildBounds(op->GetDown(), bnd);
		
		op = op->GetNext();
	}
}

Bool CDDynamicSymmetry::CDDraw(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh)
{
	if(drawpass == CD_DRAWPASS_OBJECT)
	{
	#if API_VERSION < 12000
		BaseDocument *doc = op->GetDocument(); if(!doc) return true;
		BaseContainer *oData = op->GetDataInstance(); if(!oData) return true;
		if(!oData->GetBool(DS_SHOW_GUIDE)) return true;
	#else
		BaseDocument *doc = op->GetDocument(); if(!doc) return DRAWRESULT_SKIP;
		BaseContainer *oData = op->GetDataInstance(); if(!oData) return DRAWRESULT_SKIP;
		if(!oData->GetBool(DS_SHOW_GUIDE)) return DRAWRESULT_SKIP;
	#endif
		
		CDSetBaseDrawMatrix(bd, NULL, Matrix());
		
		Matrix opM = op->GetMg();
		Vector rad = op->GetRad();
		Vector mp = op->GetMp();
		
		CDAABB hBounds;
		hBounds.Empty();
		
		hBounds.AddPoint(opM * (mp + rad));
		hBounds.AddPoint(opM * (mp - rad));
		
		AddChildBounds(op->GetDown(),&hBounds);
		rad = (hBounds.max - hBounds.min) * 0.5;
		mp = MInv(opM) * hBounds.GetCenterPoint();
		
		rad.x += oData->GetReal(DS_GUIDE_SIZE);
		rad.y += oData->GetReal(DS_GUIDE_SIZE);
		rad.z += oData->GetReal(DS_GUIDE_SIZE);
		
		Vector p1, p2, p3, p4;
		
		switch(oData->GetLong(DS_SYMMETRY_AXIS))
		{
			case DS_MX:
				p1 = opM * Vector(0,mp.y+rad.y,mp.z-rad.z);
				p2 = opM * Vector(0,mp.y+rad.y,mp.z+rad.z);
				p3 = opM * Vector(0,mp.y-rad.y,mp.z+rad.z);
				p4 = opM * Vector(0,mp.y-rad.y,mp.z-rad.z);
				break;
			case DS_MY:
				p1 = opM * Vector(mp.x+rad.x,0,mp.z-rad.z);
				p2 = opM * Vector(mp.x+rad.x,0,mp.z+rad.z);
				p3 = opM * Vector(mp.x-rad.x,0,mp.z+rad.z);
				p4 = opM * Vector(mp.x-rad.x,0,mp.z-rad.z);
				break;
			case DS_MZ:
				p1 = opM * Vector(mp.x-rad.x,mp.y+rad.y,0);
				p2 = opM * Vector(mp.x+rad.x,mp.y+rad.y,0);
				p3 = opM * Vector(mp.x+rad.x,mp.y-rad.y,0);
				p4 = opM * Vector(mp.x-rad.x,mp.y-rad.y,0);
				break;
		}
		
		Vector p[4] = { p1,p2,p3,p4 };
		
		Vector color = oData->GetVector(DS_GUIDE_COLOR);
		
		Vector f[4] = { color,color,color,color };
		
		bd->SetPen(color);
		CDDrawLine(bd,p1,p2);
		CDDrawLine(bd,p2,p3);
		CDDrawLine(bd,p3,p4);
		CDDrawLine(bd,p4,p1);
		
		LONG trnsp = bd->GetTransparency();
		bd->SetTransparency(255);
		CDDrawPolygon(bd,p, f, true);
		bd->SetTransparency(trnsp);
	}
		
	return CDSuperDrawReturn(op, drawpass, bd, bh);
}

void CDDynamicSymmetry::InitSpacePartitions(LONG cnt)
{
	indA.Init();
	indB.Init();
	indC.Init();
	indD.Init();
	
	indE.Init();
	indF.Init();
	indG.Init();
	indH.Init();
}

void CDDynamicSymmetry::FreeSpacePartitions(void)
{
	indA.Free();
	indB.Free();
	indC.Free();
	indD.Free();
	
	indE.Free();
	indF.Free();
	indG.Free();
	indH.Free();
}

LONG CDDynamicSymmetry::GetPartitionSpace(Vector pt, Vector mp)
{
	if(pt.x > mp.x)
	{
		if(pt.y > mp.y)
		{
			if(pt.z > mp.z) return 1;
			else return 0;
		}
		else
		{
			if(pt.z > mp.z) return 3;
			else return 2;
		}
	}
	else
	{
		if(pt.y > mp.y)
		{
			if(pt.z > mp.z) return 5;
			else return 4;
		}
		else
		{
			if(pt.z > mp.z) return 7;
			else return 6;
		}
	}
	
	return 0;
}

LONG* CDDynamicSymmetry::GetPartitionArray(LONG pSpace, LONG &cnt)
{
	switch(pSpace)
	{
		case 0:
		{
			cnt = indA.Size();
			return indA.Array();
			break;
		}
		case 1:
		{
			cnt = indB.Size();
			return indB.Array();
			break;
		}
		case 2:
		{
			cnt = indC.Size();
			return indC.Array();
			break;
		}
		case 3:
		{
			cnt = indD.Size();
			return indD.Array();
			break;
		}
		case 4:
		{
			cnt = indE.Size();
			return indE.Array();
			break;
		}
		case 5:
		{
			cnt = indF.Size();
			return indF.Array();
			break;
		}
		case 6:
		{
			cnt = indG.Size();
			return indG.Array();
			break;
		}
		case 7:
		{
			cnt = indH.Size();
			return indH.Array();
			break;
		}
	}
	
	return NULL;
}

Bool CDDynamicSymmetry::BuildPolygonSpacePartitions(Vector *padr, CPolygon *vadr, LONG cnt, Vector mp, LONG size)
{
	LONG i;
	if(padr && vadr && cnt > 0)
	{
		for(i=0; i<cnt; ++i)
		{
			CPolygon ply = vadr[i];
			Vector pt = CalcPolygonCenter(padr,ply);
			
			if(pt.x > mp.x)
			{
				if(pt.y > mp.y)
				{
					if(pt.z > mp.z) indB.Append(i);
					else indA.Append(i);
				}
				else
				{
					if(pt.z > mp.z) indD.Append(i);
					else indC.Append(i);
				}
			}
			else
			{
				if(pt.y > mp.y)
				{
					if(pt.z > mp.z) indF.Append(i);
					else indE.Append(i);
				}
				else
				{
					if(pt.z > mp.z) indH.Append(i);
					else indG.Append(i);
				}
			}
		}
	}
	else return false;
	
	return true;
}

Bool CDDynamicSymmetry::BuildPointSpacePartitions(Vector *padr, LONG cnt, Vector mp, LONG size)
{
	LONG i;
	if(padr && cnt > 0)
	{
		for(i=0; i<cnt; ++i)
		{
			Vector pt = padr[i];
			
			if(pt.x > mp.x)
			{
				if(pt.y > mp.y)
				{
					if(pt.z > mp.z) indB.Append(i);
					else indA.Append(i);
				}
				else
				{
					if(pt.z > mp.z) indD.Append(i);
					else indC.Append(i);
				}
			}
			else
			{
				if(pt.y > mp.y)
				{
					if(pt.z > mp.z) indF.Append(i);
					else indE.Append(i);
				}
				else
				{
					if(pt.z > mp.z) indH.Append(i);
					else indG.Append(i);
				}
			}
		}
	}
	else return false;
	
	return true;
}

Bool CDDynamicSymmetry::BuildPolySelSpacePartitions(Vector *padr, CPolygon *vadr, BaseSelect *bs, Vector mp, LONG size)
{
	LONG bsCnt = bs->GetCount();
	if(bsCnt > 0)
	{
		LONG i, a, b, seg=0;
		while(CDGetRange(bs,seg++,&a,&b))
		{
			for(i=a; i<=b; ++i)
			{
				CPolygon ply = vadr[i];
				Vector pt = CalcPolygonCenter(padr,ply);
				
				if(pt.x > mp.x)
				{
					if(pt.y > mp.y)
					{
						if(pt.z > mp.z) indB.Append(i);
						else indA.Append(i);
					}
					else
					{
						if(pt.z > mp.z) indD.Append(i);
						else indC.Append(i);
					}
				}
				else
				{
					if(pt.y > mp.y)
					{
						if(pt.z > mp.z) indF.Append(i);
						else indE.Append(i);
					}
					else
					{
						if(pt.z > mp.z) indH.Append(i);
						else indG.Append(i);
					}
				}
			}
		}
	}
	else return false;
	
	return true;
}

Bool CDDynamicSymmetry::CompareSelections(BaseObject *op, BaseObject *cln)
{
	Vector *opadr = GetPointArray(op);
	Vector *cpadr = GetPointArray(cln);
	if(opadr && cpadr)
	{
		LONG i, oCnt = ToPoint(op)->GetPointCount(), cCnt = ToPoint(cln)->GetPointCount();

		// compare point selections
		BaseSelect *opBs = ToPoly(op)->GetPolygonS();
		BaseSelect *clnBs = ToPoly(cln)->GetPolygonS();
		
		LONG size = oCnt/2;
		InitSpacePartitions(size);
		
		BaseDocument *doc = GetActiveDocument();
		switch(doc->GetMode())
		{
			case Mpoints:
			{
				if(opBs && clnBs)
				{
					LONG pbsCnt = opBs->GetCount();
					LONG cbsCnt = clnBs->GetCount();
					
					if(pbsCnt > 0 && cbsCnt > 0)
					{
						Vector mp = op->GetMp();
						if(!BuildPointSpacePartitions(opadr,oCnt,mp,size))
						{
							FreeSpacePartitions();
							return false;
						}
					
						LONG seg=0,a,b;
						while(CDGetRange(clnBs,seg++,&a,&b))
						{
							for(i=a; i<=b; ++i)
							{
								if(i<oCnt && i<cCnt)
								{
									Vector opPt = opadr[i];
									Vector clnPt = cpadr[i];
									
									if(!VEqual(opPt,clnPt,0.01))
									{
										opBs->Deselect(i);
										
										LONG cnt, pSpace = GetPartitionSpace(clnPt,mp);
										LONG *ind = GetPartitionArray(pSpace,cnt);
										
										Real dist = CDMAXREAL;
										LONG p, pInd = 0;
										for(p=0; p<cnt; ++p)
										{
											opPt = opadr[ind[p]];
											
											Real pLen = Len(clnPt - opPt);
											if(pLen < dist)
											{
												dist = pLen;
												pInd = ind[p];
											}
										}
										opBs->Select(pInd);
									}
								}
							}
						}
					}
				}
				break;
			}
			case Medges:
			{
				if(IsValidPolygonObject(op) && IsValidPolygonObject(cln))
				{
					CPolygon *ovadr = GetPolygonArray(op);
					CPolygon *cvadr = GetPolygonArray(cln);
					
					LONG opCnt = ToPoly(op)->GetPolygonCount();
					LONG cpCnt = ToPoly(cln)->GetPolygonCount();
					
					// compare edge selections
					opBs = ToPoly(op)->GetEdgeS();
					clnBs = ToPoly(cln)->GetEdgeS();
					
					if(opBs && clnBs)
					{
						LONG pbsCnt = opBs->GetCount();
						LONG cbsCnt = clnBs->GetCount();
						
						if(pbsCnt > 0 && cbsCnt > 0)
						{
							Vector mp = op->GetMp();
							if(!BuildPolygonSpacePartitions(opadr,ovadr,opCnt,mp,size))
							{
								FreeSpacePartitions();
								return false;
							}
					
							LONG seg=0,a,b;
							while(CDGetRange(clnBs,seg++,&a,&b))
							{
								for(i=a; i<=b; ++i)
								{
									if(i<opCnt*4 && i<cpCnt*4)
									{
										Vector opPt = CalcEdgeCenter(opadr,ovadr,i);
										Vector clnPt = CalcEdgeCenter(cpadr,cvadr,i);
										
										if(!VEqual(opPt,clnPt,0.01))
										{
											opBs->Deselect(i);
											
											Vector clnA, clnB;
											GetEdgePoints(cpadr,cvadr,i,clnA,clnB);
											
											Neighbor n;
											if(n.Init(cCnt,cvadr,cpCnt,NULL))
											{
												LONG aPt, bPt, side;
												side = i-((i/4)*4);
												switch (side)
												{
													case 0:
														aPt = cvadr[i/4].a;
														bPt = cvadr[i/4].b;
														break;
													case 1:
														aPt = cvadr[i/4].b;
														bPt = cvadr[i/4].c;
														break;
													case 2:
														aPt = cvadr[i/4].c;
														bPt = cvadr[i/4].d;
														break;
													case 3:
														aPt = cvadr[i/4].d;
														bPt = cvadr[i/4].a;
														break;
												}
												
												LONG plyA,plyB;
												n.GetEdgePolys(aPt,bPt,&plyA,&plyB);
												
												CPolygon cPly = cvadr[plyA];
												Vector clnPt = CalcPolygonCenter(cpadr,cPly);
												
												LONG cnt, pSpace = GetPartitionSpace(clnPt,mp);
												LONG *ind = GetPartitionArray(pSpace,cnt);
											
												Real dist = CDMAXREAL;
												LONG p, pInd = 0;
												for(p=0; p<cnt; ++p)
												{
													CPolygon oPly = ovadr[ind[p]];
													opPt = CalcPolygonCenter(opadr,oPly);
													
													Real pLen = Len(clnPt - opPt);
													if(pLen < dist)
													{
														dist = pLen;
														pInd = ind[p];
													}
												}
												LONG e;
												for(e=0; i<4; i++)
												{
													LONG ind = 4*pInd+i;
													Vector opPt = CalcEdgeCenter(opadr,ovadr,ind);
													if(VEqual(opPt,clnPt,0.01))
													{
														opBs->Select(ind);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				break;
			}
			case Mpolygons:
			{
				if(IsValidPolygonObject(op) && IsValidPolygonObject(cln))
				{
					CPolygon *ovadr = GetPolygonArray(op);
					CPolygon *cvadr = GetPolygonArray(cln);
					
					LONG opCnt = ToPoly(op)->GetPolygonCount();
					LONG cpCnt = ToPoly(cln)->GetPolygonCount();
					
					// compare polygon selections
					opBs = ToPoly(op)->GetPolygonS();
					clnBs = ToPoly(cln)->GetPolygonS();
					
					if(opBs && clnBs)
					{
						LONG pbsCnt = opBs->GetCount();
						LONG cbsCnt = clnBs->GetCount();
						
						if(pbsCnt > 0 && cbsCnt > 0)
						{
							Vector mp = op->GetMp();
							if(!BuildPolygonSpacePartitions(opadr,ovadr,opCnt,mp,size))
							{
								FreeSpacePartitions();
								return false;
							}
					
							LONG seg=0,a,b;
							while(CDGetRange(clnBs,seg++,&a,&b))
							{
								for(i=a; i<=b; ++i)
								{
									if(i<opCnt && i<cpCnt)
									{
										CPolygon oPly = ovadr[i];
										CPolygon cPly = cvadr[i];
										
										Vector opPt = CalcPolygonCenter(opadr,oPly);
										Vector clnPt = CalcPolygonCenter(cpadr,cPly);
										
										if(!VEqual(opPt,clnPt,0.01))
										{
											opBs->Deselect(i);
											
											LONG cnt, pSpace = GetPartitionSpace(clnPt,mp);
											LONG *ind = GetPartitionArray(pSpace,cnt);
										
											Real dist = CDMAXREAL;
											LONG p, pInd = 0;
											for(p=0; p<cnt; ++p)
											{
												oPly = ovadr[ind[p]];
												opPt = CalcPolygonCenter(opadr,oPly);
												
												Real pLen = Len(clnPt - opPt);
												if(pLen < dist)
												{
													dist = pLen;
													pInd = ind[p];
												}
											}
											opBs->Select(pInd);
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
	}
	
	return true;
}

void CDDynamicSymmetry::GetUVWTags(BaseObject *op, BaseObject *cln, AtomArray *oUVWTags, AtomArray *cUVWTags)
{
	BaseTag *opTag = op->GetFirstTag();
	BaseTag *clnTag = cln->GetFirstTag();
	
	while(opTag && clnTag)
	{
		if(opTag->IsInstanceOf(Tuvw) && clnTag->IsInstanceOf(Tuvw))
		{
			UVWTag *oTag = static_cast<UVWTag*>(opTag);
			UVWTag *cTag = static_cast<UVWTag*>(clnTag);
			
			if(oTag && cTag)
			{
				oUVWTags->Append(oTag);
				cUVWTags->Append(cTag);
			}
		}
		opTag = opTag->GetNext();
		clnTag = clnTag->GetNext();
	}
}

void CDDynamicSymmetry::TransferUVWCoordinates(Vector *opadr, Vector *cpadr, CPolygon oPly, CPolygon cPly, UVWStruct &ouv, UVWStruct &cuv)
{
	Vector uPt[4];
	uPt[0] = cuv.a;
	uPt[1] = cuv.b;
	uPt[2] = cuv.c;
	uPt[3] = cuv.d;
	
	LONG ind[4];
	ind[0] = -1;
	ind[1] = -1;
	ind[2] = -1;
	ind[3] = -1;
	
	Vector oPt[4];
	oPt[0] = opadr[oPly.a];
	oPt[1] = opadr[oPly.b];
	oPt[2] = opadr[oPly.c];
	oPt[3] = opadr[oPly.d];
	
	Vector cPt[4];
	cPt[0] = cpadr[cPly.a];
	cPt[1] = cpadr[cPly.b];
	cPt[2] = cpadr[cPly.c];
	cPt[3] = cpadr[cPly.d];
	
	LONG i, j;
	for(i=0; i<4; i++)
	{
		Real dist = CDMAXREAL;
		if(oPly.c == oPly.d && cPly.c == cPly.d)
		{
			for(j=0; j<3; j++)
			{
				Real pLen = Len(oPt[i]-cPt[j]);
				if(pLen < dist)
				{
					dist = pLen;
					ind[i] = j;
				}
			}
		}
		else
		{
			for(j=0; j<4; j++)
			{
				Real pLen = Len(oPt[i]-cPt[j]);
				if(pLen < dist)
				{
					dist = pLen;
					ind[i] = j;
				}
			}
		}
	}
	
	if(ind[0] > -1) ouv.a = uPt[ind[0]];
	if(ind[1] > -1) ouv.b = uPt[ind[1]];
	if(ind[2] > -1) ouv.c = uPt[ind[2]];
	if(ind[3] > -1) ouv.d = uPt[ind[3]];
}

Bool CDDynamicSymmetry::CompareUVWTags(BaseObject *op, BaseObject *cln, LONG axis, LONG dir)
{
	if(!IsValidPolygonObject(op) || !IsValidPolygonObject(cln)) return false;
	
	AutoAlloc<AtomArray> oUVWTags; if(!oUVWTags) return false;
	AutoAlloc<AtomArray> cUVWTags; if(!cUVWTags) return false;
	
	GetUVWTags(op,cln,oUVWTags,cUVWTags);
	LONG ouCnt = oUVWTags->GetCount(), cuCnt = cUVWTags->GetCount();
	if(ouCnt < 1 || cuCnt < 1) return false;
	if(ouCnt != cuCnt) return false;
	
	Vector *opadr = GetPointArray(op);
	Vector *cpadr = GetPointArray(cln);
	if(!opadr || !cpadr) return false;
	
	CPolygon *ovadr = GetPolygonArray(op);
	CPolygon *cvadr = GetPolygonArray(cln);
	if(!ovadr || !cvadr) return false;
	
	LONG io, ic, cpCnt = ToPoly(cln)->GetPolygonCount();
	
	BaseSelect *obs = ToPoly(op)->GetPolygonS();
	BaseSelect *cbs = ToPoly(cln)->GetPolygonS();
	if(obs && cbs)
	{
		LONG bsCnt = obs->GetCount();
		if(bsCnt > 0)
		{
			LONG size = cpCnt/2;
			InitSpacePartitions(size);
			Vector mp = GetSelectionCenter(cpadr,cvadr,cbs,bsCnt);
			
			if(!BuildPolySelSpacePartitions(cpadr,cvadr,cbs,mp,size))
			{
				FreeSpacePartitions();
				return false;
			}
			
			StatusClear();
			StatusSetText("Processing");
			LONG sego=0,ao,bo;
			LONG bar = 0;
			while(CDGetRange(obs,sego++,&ao,&bo))
			{
				for(io=ao; io<=bo; ++io)
				{
					LONG prg = LONG(Real(bar++)/Real(bsCnt)*100.0);
					StatusSetBar(prg);
					
					CPolygon cPly, oPly = ovadr[io];
					Vector clnPt, opPt = CalcPolygonCenter(opadr,oPly);
					if(io < cpCnt)
					{
						cPly = cvadr[io];
						clnPt = CalcPolygonCenter(cpadr,cPly);
					}
					
					if(VEqual(opPt,clnPt,0.01))
					{
						LONG u;
						for(u=0; u<ouCnt; u++)
						{
							UVWTag *oTag = static_cast<UVWTag*>(oUVWTags->GetIndex(u));
							UVWTag *cTag = static_cast<UVWTag*>(cUVWTags->GetIndex(u));
							if(oTag && cTag)
							{
								UVWStruct ouv, cuv;
                                CDGetUVWStruct(oTag,ouv,io);
                                CDGetUVWStruct(oTag,cuv,io);
                                TransferUVWCoordinates(opadr,cpadr,oPly,cPly,ouv,cuv);
                                CDSetUVWStruct(oTag,ouv,io);
							}
						}
					}
					else
					{
						LONG bsCnt = cbs->GetCount();
						if(bsCnt > 0)
						{
							Real dist = CDMAXREAL;
							
							LONG cnt, pSpace = GetPartitionSpace(opPt,mp);
							LONG *ind = GetPartitionArray(pSpace,cnt);
							
							LONG pInd = 0;
							for(ic=0; ic<cnt; ++ic)
							{
								cPly = cvadr[ind[ic]];
								clnPt = CalcPolygonCenter(cpadr,cPly);
								
								Real pLen = Len(clnPt - opPt);
								if(pLen < dist)
								{
									dist = pLen;
									pInd = ind[ic];
								}
							}
							
							LONG u;
							for(u=0; u<ouCnt; u++)
							{
								UVWTag *oTag = static_cast<UVWTag*>(oUVWTags->GetIndex(u));
								UVWTag *cTag = static_cast<UVWTag*>(cUVWTags->GetIndex(u));
								if(oTag && cTag)
								{
									UVWStruct ouv, cuv;
                                    CDGetUVWStruct(oTag,ouv,io);
                                    CDGetUVWStruct(oTag,cuv,io);
                                    TransferUVWCoordinates(opadr,cpadr,oPly,cPly,ouv,cuv);
                                    CDSetUVWStruct(oTag,ouv,io);
								}
							}
						}
					}
				}
			}
			FreeSpacePartitions();
		}
	}
	
	return true;
}

Bool CDDynamicSymmetry::DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis)
{
	if(!IsValidPolygonObject(op)) return false;
	
	CPolygon *vadr = GetPolygonArray(op);
	Vector *padr = GetPointArray(op);
	LONG i, vCnt = ToPoly(op)->GetPolygonCount();
	
	BaseSelect *bs = ToPoly(op)->GetPolygonS();
	bs->DeselectAll();
	
	for(i=0; i<vCnt; i++)
	{
		CPolygon ply = vadr[i];
		Vector a,b,c,d;
		a = MInv(prM) * (opM * padr[ply.a]);
		b = MInv(prM) * (opM * padr[ply.b]);
		c = MInv(prM) * (opM * padr[ply.c]);
		d = MInv(prM) * (opM * padr[ply.d]);
		
		switch(axis)
		{
			case 0: // x axis
				if(a.x == 0 && b.x == 0 && c.x == 0 && d.x == 0) bs->Select(i);
				break;
			case 1: // y axis
				if(a.y == 0 && b.y == 0 && c.y == 0 && d.y == 0) bs->Select(i);
				break;
			case 2: // z axis
				if(a.z == 0 && b.z == 0 && c.z == 0 && d.z == 0) bs->Select(i);
				break;
		}
	}
	
	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = op;
#if API_VERSION < 12000
	mcd.mode = MODIFY_POLYGONSELECTION;
#else
	mcd.mode = MODELINGCOMMANDMODE_POLYGONSELECTION;
#endif
	if(!SendModelingCommand(MCOMMAND_DELETE, mcd)) return false;
	
	return true;
}

Bool CDDynamicSymmetry::IsCrossPlanePoly(Vector *padr, const CPolygon &ply, Matrix opM, Matrix prM, LONG axis, Real t)
{
	if(!padr) return false;
	
	Vector a,b,c,d;
	a = MInv(prM) * (opM * padr[ply.a]);
	b = MInv(prM) * (opM * padr[ply.b]);
	c = MInv(prM) * (opM * padr[ply.c]);
	d = MInv(prM) * (opM * padr[ply.d]);

	switch(axis)
	{
		case 0:
		{
			if(a.x > -t && b.x > -t && c.x > -t && d.x > -t) return false;
			else if(a.x < t && b.x < t && c.x < t && d.x < t) return false;
			break;
		}
		case 1:
		{
			if(a.y > -t && b.y > -t && c.y > -t && d.y > -t) return false;
			else if(a.y < t && b.y < t && c.y < t && d.y < t) return false;
			break;
		}
		case 2:
		{
			if(a.z > -t && b.z > -t && c.z > -t && d.z > -t) return false;
			else if(a.z < t && b.z < t && c.z < t && d.z < t) return false;
			break;
		}
	}

	return true;
}

Bool CDDynamicSymmetry::IsCrossPlaneLength(Real a, Real b)
{
	if( ( Abs(a)+Abs(b) ) > Abs(a+b) ) return true;
	else return false;
}

LONG CDDynamicSymmetry::GetCrossPlanePolyEdges(Vector *padr, CPolygon ply, CutEdge *edg, Matrix opM, Matrix prM, LONG axis)
{
	LONG cnt = 0;
	
	Vector a,b,c,d;
	a = MInv(prM) * (opM * padr[ply.a]);
	b = MInv(prM) * (opM * padr[ply.b]);
	c = MInv(prM) * (opM * padr[ply.c]);
	d = MInv(prM) * (opM * padr[ply.d]);
	
	switch(axis)
	{
		case 0:
		{
			if(IsCrossPlaneLength(a.x,b.x))
			{
				if(a.x < 0.0)
				{
					edg[cnt].a = ply.a;
					edg[cnt].b = ply.b;
					edg[cnt].c = Abs(a.x)/(Abs(a.x)+Abs(b.x));
					cnt++;
				}
				else
				{
					edg[cnt].a = ply.b;
					edg[cnt].b = ply.a;
					edg[cnt].c = Abs(b.x)/(Abs(b.x)+Abs(a.x));
					cnt++;
				}
			}
			if(IsCrossPlaneLength(b.x,c.x))
			{
				if(b.x < 0.0)
				{
					edg[cnt].a = ply.b;
					edg[cnt].b = ply.c;
					edg[cnt].c = Abs(b.x)/(Abs(b.x)+Abs(c.x));
					cnt++;
				}
				else
				{
					edg[cnt].a = ply.c;
					edg[cnt].b = ply.b;
					edg[cnt].c = Abs(c.x)/(Abs(c.x)+Abs(b.x));
					cnt++;
				}
			}
			if(ply.c == ply.d)
			{
				if(IsCrossPlaneLength(c.x,a.x))
				{
					if(c.x < 0.0)
					{
						edg[cnt].a = ply.c;
						edg[cnt].b = ply.a;
						edg[cnt].c = Abs(c.x)/(Abs(c.x)+Abs(a.x));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.a;
						edg[cnt].b = ply.c;
						edg[cnt].c = Abs(a.x)/(Abs(a.x)+Abs(c.x));
						cnt++;
					}
				}
			}
			else
			{
				if(IsCrossPlaneLength(c.x,d.x))
				{
					if(c.x < 0.0)
					{
						edg[cnt].a = ply.c;
						edg[cnt].b = ply.d;
						edg[cnt].c = Abs(c.x)/(Abs(c.x)+Abs(d.x));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.d;
						edg[cnt].b = ply.c;
						edg[cnt].c = Abs(d.x)/(Abs(d.x)+Abs(c.x));
						cnt++;
					}
				}
				if(IsCrossPlaneLength(d.x,a.x))
				{
					if(d.x < 0.0)
					{
						edg[cnt].a = ply.d;
						edg[cnt].b = ply.a;
						edg[cnt].c = Abs(d.x)/(Abs(d.x)+Abs(a.x));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.a;
						edg[cnt].b = ply.d;
						edg[cnt].c = Abs(a.x)/(Abs(a.x)+Abs(d.x));
						cnt++;
					}
				}
			}
			break;
		}
		case 1:
		{
			if(IsCrossPlaneLength(a.y,b.y))
			{
				if(a.y < 0.0)
				{
					edg[cnt].a = ply.a;
					edg[cnt].b = ply.b;
					edg[cnt].c = Abs(a.y)/(Abs(a.y)+Abs(b.y));
					cnt++;
				}
				else
				{
					edg[cnt].a = ply.b;
					edg[cnt].b = ply.a;
					edg[cnt].c = Abs(b.y)/(Abs(b.y)+Abs(a.y));
					cnt++;
				}
			}
			if(IsCrossPlaneLength(b.y,c.y))
			{
				if(b.y < 0.0)
				{
					edg[cnt].a = ply.b;
					edg[cnt].b = ply.c;
					edg[cnt].c = Abs(b.y)/(Abs(b.y)+Abs(c.y));
					cnt++;
				}
				else
				{
					edg[cnt].a = ply.c;
					edg[cnt].b = ply.b;
					edg[cnt].c = Abs(c.y)/(Abs(c.y)+Abs(b.y));
					cnt++;
				}
			}
			if(ply.c == ply.d)
			{
				if(IsCrossPlaneLength(c.y,a.y))
				{
					if(c.y < 0.0)
					{
						edg[cnt].a = ply.c;
						edg[cnt].b = ply.a;
						edg[cnt].c = Abs(c.y)/(Abs(c.y)+Abs(a.y));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.a;
						edg[cnt].b = ply.c;
						edg[cnt].c = Abs(a.y)/(Abs(a.y)+Abs(c.y));
						cnt++;
					}
				}
			}
			else
			{
				if(IsCrossPlaneLength(c.y,d.y))
				{
					if(c.y < 0.0)
					{
						edg[cnt].a = ply.c;
						edg[cnt].b = ply.d;
						edg[cnt].c = Abs(c.y)/(Abs(c.y)+Abs(d.y));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.d;
						edg[cnt].b = ply.c;
						edg[cnt].c = Abs(d.y)/(Abs(d.y)+Abs(c.y));
						cnt++;
					}
				}
				if(IsCrossPlaneLength(d.y,a.y))
				{
					if(d.y < 0.0)
					{
						edg[cnt].a = ply.d;
						edg[cnt].b = ply.a;
						edg[cnt].c = Abs(d.y)/(Abs(d.y)+Abs(a.y));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.a;
						edg[cnt].b = ply.d;
						edg[cnt].c = Abs(a.y)/(Abs(a.y)+Abs(d.y));
						cnt++;
					}
				}
			}
			break;
		}
		case 2:
		{
			if(IsCrossPlaneLength(a.z,b.z))
			{
				if(a.z < 0.0)
				{
					edg[cnt].a = ply.a;
					edg[cnt].b = ply.b;
					edg[cnt].c = Abs(a.z)/(Abs(a.z)+Abs(b.z));
					cnt++;
				}
				else
				{
					edg[cnt].a = ply.b;
					edg[cnt].b = ply.a;
					edg[cnt].c = Abs(b.z)/(Abs(b.z)+Abs(a.z));
					cnt++;
				}
			}
			if(IsCrossPlaneLength(b.z,c.z))
			{
				if(b.z < 0.0)
				{
					edg[cnt].a = ply.b;
					edg[cnt].b = ply.c;
					edg[cnt].c = Abs(b.z)/(Abs(b.z)+Abs(c.z));
					cnt++;
				}
				else
				{
					edg[cnt].a = ply.c;
					edg[cnt].b = ply.b;
					edg[cnt].c = Abs(c.z)/(Abs(c.z)+Abs(b.z));
					cnt++;
				}
			}
			if(ply.c == ply.d)
			{
				if(IsCrossPlaneLength(c.z,a.z))
				{
					if(c.z < 0.0)
					{
						edg[cnt].a = ply.c;
						edg[cnt].b = ply.a;
						edg[cnt].c = Abs(c.z)/(Abs(c.z)+Abs(a.z));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.a;
						edg[cnt].b = ply.c;
						edg[cnt].c = Abs(a.z)/(Abs(a.z)+Abs(c.z));
						cnt++;
					}
				}
			}
			else
			{
				if(IsCrossPlaneLength(c.z,d.z))
				{
					if(c.z < 0.0)
					{
						edg[cnt].a = ply.c;
						edg[cnt].b = ply.d;
						edg[cnt].c = Abs(c.z)/(Abs(c.z)+Abs(d.z));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.d;
						edg[cnt].b = ply.c;
						edg[cnt].c = Abs(d.z)/(Abs(d.z)+Abs(c.z));
						cnt++;
					}
				}
				if(IsCrossPlaneLength(d.z,a.z))
				{
					if(d.z < 0.0)
					{
						edg[cnt].a = ply.d;
						edg[cnt].b = ply.a;
						edg[cnt].c = Abs(d.z)/(Abs(d.z)+Abs(a.z));
						cnt++;
					}
					else
					{
						edg[cnt].a = ply.a;
						edg[cnt].b = ply.d;
						edg[cnt].c = Abs(a.z)/(Abs(a.z)+Abs(d.z));
						cnt++;
					}
				}
			}
			break;
		}
	}
	
	return cnt;
}

LONG CDDynamicSymmetry::FindCutPoint(Vector *padr, CPolygon ply, LONG axis, CutEdge e)
{
	Vector a,b,c,d;
	a = padr[ply.a];
	b = padr[ply.b];
	c = padr[ply.c];
	d = padr[ply.d];

	switch(axis)
	{
		case 0:
		{
			if(a.x == 0.0 && ply.a != e.a && ply.a != e.b) return ply.a;
			else if(b.x == 0.0 && ply.b != e.a && ply.b != e.b) return ply.b;
			else
			{
				if(ply.c == ply.d)
				{
					if(c.x == 0.0 && ply.c != e.a && ply.c != e.b) return ply.c;
				}
				else
				{
					if(c.x == 0.0 && ply.c != e.a && ply.c != e.b) return ply.c;
					else if(d.x == 0.0 && ply.d != e.a && ply.d != e.b) return ply.d;
				}
			}
			break;
		}
		case 1:
		{
			if(a.y == 0.0 && ply.a != e.a && ply.a != e.b) return ply.a;
			else if(b.y == 0.0 && ply.b != e.a && ply.b != e.b) return ply.b;
			else
			{
				if(ply.c == ply.d)
				{
					if(c.y == 0.0 && ply.c != e.a && ply.c != e.b) return ply.c;
				}
				else
				{
					if(c.y == 0.0 && ply.c != e.a && ply.c != e.b) return ply.c;
					else if(d.y == 0.0 && ply.d != e.a && ply.d != e.b) return ply.d;
				}
			}
			break;
		}
		case 2:
		{
			if(a.z == 0.0 && ply.a != e.a && ply.a != e.b) return ply.a;
			else if(b.z == 0.0 && ply.b != e.a && ply.b != e.b) return ply.b;
			else
			{
				if(ply.c == ply.d)
				{
					if(c.z == 0.0 && ply.c != e.a && ply.c != e.b) return ply.c;
				}
				else
				{
					if(c.z == 0.0 && ply.c != e.a && ply.c != e.b) return ply.c;
					else if(d.z == 0.0 && ply.d != e.a && ply.d != e.b) return ply.d;
				}
			}
			break;
		}
	}
	
	return NOTOK;
}

Bool CDDynamicSymmetry::CenterCutObject(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis, Real t)
{
	Vector *padr = GetPointArray(op);
	if(!padr) return false;
	
	LONG i, pCnt = ToPoint(op)->GetPointCount();
	
	// snap existing center points
	for(i=0; i<pCnt; i++)
	{
		Vector pt = MInv(prM) * (opM * padr[i]);
		switch(axis)
		{
			case 0:
			{
				if(pt.x > -t && pt.x < t)
				{
					pt.x = 0.0;
					padr[i] = pt;
				}
				break;
			}
			case 1:
			{
				if(pt.y > -t && pt.y < t)
				{
					pt.y = 0.0;
					padr[i] = pt;
				}
				break;
			}
			case 2:
			{
				if(pt.z > -t && pt.z < t)
				{
					pt.z = 0.0;
					padr[i] = pt;
				}
				break;
			}
		}
	}
	
	Vector min, max, mp = op->GetMp(), rad = op->GetRad();
	min = opM * mp - rad;
	max = opM * mp + rad;

	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = op;
#if API_VERSION < 12000
	mcd.flags = MODELINGCOMMANDFLAG_CREATEUNDO;
#else
	mcd.flags = MODELINGCOMMANDFLAGS_CREATEUNDO;
#endif
	
	if(IsValidPolygonObject(op))
	{
		// check for cross plane polygons
		CPolygon *vadr = GetPolygonArray(op);
		if(vadr)
		{
			BaseSelect *bsPly = ToPoly(op)->GetPolygonS();
			if(bsPly) bsPly->DeselectAll();
				
			LONG vCnt = ToPoly(op)->GetPolygonCount();
			for(i=0; i<vCnt; i++)
			{
				CPolygon ply = vadr[i];
				if(IsCrossPlanePoly(padr,ply,opM,prM,axis,t)) bsPly->Select(i);
			}
			
			LONG plySCnt = bsPly->GetCount();
			if(plySCnt > 0)
			{
				AutoAlloc<Modeling> mod;
				if(!mod || !mod->InitObject(op)) return false;
				
				CDEdgeCutArray edges;
				edges.Init();
				LONG seg=0,a,b;
				while(CDGetRange(bsPly,seg++,&a,&b))
				{
					for(i=a; i<=b; ++i)
					{
						CPolygon ply = vadr[i];
						CutEdge edg[4];
						LONG eCnt = GetCrossPlanePolyEdges(padr,ply,edg,opM,prM,axis);
						if(eCnt > 0)
						{
							if(eCnt > 1)
							{
								LONG e, aInd=0, bInd=0;
								for(e=0; e<eCnt-1; e++)
								{
									if(mod->IsValidEdge(op,i,edg[e].a,edg[e].b))
									{
										aInd = mod->SplitEdge(op,edg[e].a,edg[e].b,edg[e].c);
										if(!aInd) return false;
										else
										{
											edg[e].vInd = aInd;
											edges.Append(edg[e]);
										}
									}
									else
									{
										aInd = edges.GetCutPtIndex(edg[e]);
										if(!aInd) return false;
									}
									
									if(mod->IsValidEdge(op,i,edg[e+1].a,edg[e+1].b))
									{
										bInd = mod->SplitEdge(op,edg[e+1].a,edg[e+1].b,edg[e+1].c);
										if(!bInd) return false;
										else
										{
											edg[e+1].vInd = bInd;
											edges.Append(edg[e+1]);
										}
									}
									else
									{
										bInd = edges.GetCutPtIndex(edg[e+1]);
										if(!bInd) return false;
									}
									
									if(aInd && bInd)
									{
										if(!mod->SplitPolygon(op,i,aInd,bInd)) return false;
									}
								}
							}
							else
							{
								LONG aInd, bInd;
								if(mod->IsValidEdge(op,i,edg[0].a,edg[0].b))
								{
									aInd = mod->SplitEdge(op,edg[0].a,edg[0].b,edg[0].c);
									if(!aInd) return false;
									else
									{
										edg[0].vInd = aInd;
										edges.Append(edg[0]);
									}
								}
								else
								{
									aInd = edges.GetCutPtIndex(edg[0]);
									if(!aInd) return false;
								}
								
								if(aInd)
								{
									bInd = FindCutPoint(padr,ply,axis,edg[0]);
									if(bInd > -1)
									{
										if(!mod->SplitPolygon(op,i,aInd,bInd)) return false;
									}
								}
							}
						}
					}
				}
				if(!mod->Commit(op,MODELING_COMMIT_NO_NGONS)) return false;
				
				if(edges.Size() > 0) edges.Free();
			}
		}
	}
	else
	{
		Vector center = prM.off;
		BaseContainer bc;
		switch(axis)
		{
			case 0:
				bc.SetVector(OLD_MDATA_KNIFE_P1,Vector(center.x,max.y+100,0));
				bc.SetVector(OLD_MDATA_KNIFE_P2,Vector(center.x,min.y-100,0));
				bc.SetVector(OLD_MDATA_KNIFE_V1,Vector(0,0,-1));
				bc.SetVector(OLD_MDATA_KNIFE_V2,Vector(0,0,-1));
				break;
			case 1:
				bc.SetVector(OLD_MDATA_KNIFE_P1,Vector(max.x+100,center.y,0));
				bc.SetVector(OLD_MDATA_KNIFE_P2,Vector(min.x-100,center.y,0));
				bc.SetVector(OLD_MDATA_KNIFE_V1,Vector(0,0,-1));
				bc.SetVector(OLD_MDATA_KNIFE_V2,Vector(0,0,-1));
				break;
			case 2:
				bc.SetVector(OLD_MDATA_KNIFE_P1,Vector(0,max.y+100,center.z));
				bc.SetVector(OLD_MDATA_KNIFE_P2,Vector(0,min.y-100,center.z));
				bc.SetVector(OLD_MDATA_KNIFE_V1,Vector(1,0,0));
				bc.SetVector(OLD_MDATA_KNIFE_V2,Vector(1,0,0));
				break;
		}
		bc.SetBool(OLD_MDATA_KNIFE_RESTRICT,false);
		bc.SetReal(OLD_MDATA_KNIFE_ANGLE,0.785);
		
		mcd.bc = &bc;
	#if API_VERSION < 12000
		mcd.mode = MODIFY_ALL;
	#else
		mcd.mode = MODELINGCOMMANDMODE_ALL;
	#endif
		if(!SendModelingCommand(MCOMMAND_KNIFE, mcd)) return false;
	}
	
	CDOptimize(doc,op,0.001,true,false,true,CD_MODELINGCOMMANDMODE_ALL);
	
	return true;
}

Bool CDDynamicSymmetry::SplitObject(BaseContainer *oData, BaseDocument *doc, BaseObject *op, Matrix prM)
{
	if(!op || !doc) return false;
	
	LONG axis = GetSymmetryAxis(oData);
	LONG dir = oData->GetLong(DS_ACTIVE_SYMMETRY);
	Real t = oData->GetReal(DS_TOLERANCE);
	Bool updt = oData->GetBool(DS_AUTO_UPDATE);
	Bool kpSel = oData->GetBool(DS_KEEP_SEL);
	Bool kpUVW = oData->GetBool(DS_KEEP_UVW);
	
	Matrix opM = op->GetMg();
	Vector center = prM.off;
	Vector min, max, mp = op->GetMp(), rad = op->GetRad();
	
	min = opM * mp - rad;
	max = opM * mp + rad;
	
	Bool isPoly = true;
	if(!IsValidPolygonObject(op))  isPoly = false;
	
	BaseObject *uvCln = NULL, *selCln = NULL;
	
	// store the current selections
	AutoAlloc<BaseSelect> ptSelStored;
	AutoAlloc<BaseSelect> edgSelStored;
	AutoAlloc<BaseSelect> plySelStored;
	
	if(updt && kpSel)
	{
		StoreSelections(op, ptSelStored, edgSelStored, plySelStored);
		selCln = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,NULL); // clone op to compare selections
	}
	
	// split object
	if(!CenterCutObject(doc,op,opM,prM,axis,t)) return false;
	
	// delete unused points after split
	Vector *padr = GetPointArray(op);
	if(!padr) return false;
	
	BaseSelect *bsPt = ToPoint(op)->GetPointS();
	if(bsPt) bsPt->DeselectAll();
	
	LONG i, pCnt = ToPoint(op)->GetPointCount();
	
	// snap points to center & select points to remove
	for(i=0; i<pCnt; i++)
	{
		Vector pt = MInv(prM) * (opM * padr[i]);
		switch(dir)
		{
			case DS_POSITIVE:
			{
				switch(axis)
				{
					case 0:
					{
						if(pt.x < t)
						{
							if(pt.x > -t)
							{
								pt.x = 0.0;
								padr[i] = MInv(opM) * (prM * pt);
							}
							else bsPt->Select(i);
						}
						break;
					}
					case 1:
					{
						if(pt.y < t)
						{
							if(pt.y > -t)
							{
								pt.y = 0.0;
								padr[i] = MInv(opM) * (prM * pt);
							}
							else bsPt->Select(i);
						}
						break;
					}
					case 2:
					{
						if(pt.z < t)
						{
							if(pt.z > -t)
							{
								pt.z = 0.0;
								padr[i] = MInv(opM) * (prM * pt);
							}
							else bsPt->Select(i);
						}
						break;
					}
				}
				break;
			}
			case DS_NEGATIVE:
			{
				switch(axis)
				{
					case 0:
					{
						if(pt.x > -t)
						{
							if(pt.x < t)
							{
								pt.x = 0.0;
								padr[i] = MInv(opM) * (prM * pt);
							}
							else bsPt->Select(i);
						}
						break;
					}
					case 1:
					{
						if(pt.y > -t)
						{
							if(pt.y < t)
							{
								pt.y = 0.0;
								padr[i] = MInv(opM) * (prM * pt);
							}
							else bsPt->Select(i);
						}
						break;
					}
					case 2:
					{
						if(pt.z > -t)
						{
							if(pt.z < t)
							{
								pt.z = 0.0;
								padr[i] = MInv(opM) * (prM * pt);
							}
							else bsPt->Select(i);
						}
						break;
					}
				}
				break;
			}
		}
	}
	
	CPolygon *vadr = NULL;
	BaseSelect *bsPly = NULL;
	if(isPoly)
	{
		// select polygons to remove
		bsPly = ToPoly(op)->GetPolygonS();
		if(bsPly) bsPly->DeselectAll();
		
		vadr = GetPolygonArray(op);
		LONG vCnt = ToPoly(op)->GetPolygonCount();
		
		for(i=0; i<vCnt; i++)
		{
			CPolygon ply = vadr[i];
			Vector cntr = CalcPolygonCenter(padr,ply);
			
			Vector pt = MInv(prM) * (opM * cntr);
			switch(dir)
			{
				case DS_POSITIVE:
				{
					switch(axis)
					{
						case 0:
						{
							if(pt.x < 0.0) bsPly->Select(i);
							break;
						}
						case 1:
						{
							if(pt.y < 0.0) bsPly->Select(i);
							break;
						}
						case 2:
						{
							if(pt.z < 0.0) bsPly->Select(i);
							break;
						}
					}
					break;
				}
				case DS_NEGATIVE:
				{
					switch(axis)
					{
						case 0:
						{
							if(pt.x > 0.0) bsPly->Select(i);
							break;
						}
						case 1:
						{
							if(pt.y > 0.0) bsPly->Select(i);
							break;
						}
						case 2:
						{
							if(pt.z > 0.0) bsPly->Select(i);
							break;
						}
					}
					break;
				}
			}
		}
	}
	if(updt && isPoly && kpUVW) uvCln =(BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,NULL); // clone op to compare uvw's
	
	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = op;
#if API_VERSION < 12000
	mcd.flags = MODELINGCOMMANDFLAG_CREATEUNDO;
	if(isPoly) mcd.mode = MODIFY_POLYGONSELECTION;
	else mcd.mode = MODIFY_POINTSELECTION;
#else
	mcd.flags = MODELINGCOMMANDFLAGS_CREATEUNDO;
	if(isPoly) mcd.mode = MODELINGCOMMANDMODE_POLYGONSELECTION;
	else mcd.mode = MODELINGCOMMANDMODE_POINTSELECTION;
#endif
	

	if(!SendModelingCommand(MCOMMAND_DELETE, mcd)) return false;
	
	// delete center plane polygons
	if(isPoly)
	{
		if(bsPly) bsPly->DeselectAll();
		DeleteSymmetryPlanePolys(doc,op,opM,prM,axis);
	}
	CDOptimize(doc,op,t,false,false,true,CD_MODELINGCOMMANDMODE_ALL);
	
	op->SetMg(MInv(prM) * opM);
	
	// mirror geometry
	BaseContainer mirData;
	switch(axis)
	{
		case 0:
			mirData.SetLong(MDATA_MIRROR_PLANE,1);
			mirData.SetVector(MDATA_MIRROR_VECTOR,Vector(1,0,0));
			break;
		case 1:
			mirData.SetLong(MDATA_MIRROR_PLANE,2);
			mirData.SetVector(MDATA_MIRROR_VECTOR,Vector(0,1,0));
			break;
		case 2:
			mirData.SetLong(MDATA_MIRROR_PLANE,0);
			mirData.SetVector(MDATA_MIRROR_VECTOR,Vector(0,0,1));
			break;
	}
	mirData.SetLong(MDATA_MIRROR_SYSTEM,1);
	mirData.SetBool(MDATA_MIRROR_SNAPPOINTS,true);
	mirData.SetBool(MDATA_MIRROR_DUPLICATE,true);
	mirData.SetBool(MDATA_MIRROR_ONPLANE,true);
	mirData.SetBool(MDATA_MIRROR_SELECTIONS,true);
	
	mirData.SetBool(MDATA_MIRROR_WELD,true);
	mirData.SetReal(MDATA_MIRROR_TOLERANCE,t);
	mirData.SetReal(MDATA_MIRROR_VALUE,0.0);
	
	mirData.SetVector(MDATA_MIRROR_POINT,Vector(0,0,0));
	
#if API_VERSION < 12000
	mcd.mode = MODIFY_ALL;
#else
	mcd.mode = MODELINGCOMMANDMODE_ALL;
#endif
	mcd.bc = &mirData;
	if(!SendModelingCommand(MCOMMAND_MIRROR, mcd)) return false;
	
	op->SetMg(opM);
	
	if(updt && isPoly)
	{
		if(kpUVW && uvCln && op->GetTag(Tuvw))
		{
			CompareUVWTags(op,uvCln,axis,dir);
			BaseObject::Free(uvCln);
		}
		if(kpSel && selCln)
		{
			RestoreSelections(op, ptSelStored, edgSelStored, plySelStored);
			CompareSelections(op,selCln);
			BaseObject::Free(selCln);
		}
	}
	
	return true;
}

LONG CDDynamicSymmetry::GetSymmetryAxis(BaseContainer *oData)
{
	switch(oData->GetLong(DS_SYMMETRY_AXIS))
	{
		case DS_MX:
			return 0;
		case DS_MY:
			return 1;
		case DS_MZ:
			return 2;
		default:
			return 0;
	}
}

Bool CDDynamicSymmetry::BuildSymmetry(HierarchyHelp *hh, BaseThread *bt, Matrix prM, BaseObject *pr, BaseObject *op, BaseContainer *oData, BaseDocument *doc)
{
	if(IsValidPointObject(op)) SplitObject(oData,doc,op,prM);
	else if((op->GetInfo() & OBJECT_GENERATOR))
	{
		ModelingCommandData mcd;
		mcd.doc = doc;
		mcd.op = op;
	#if API_VERSION < 12000
		mcd.flags = MODELINGCOMMANDFLAG_CREATEUNDO;
	#else
		mcd.flags = MODELINGCOMMANDFLAGS_CREATEUNDO;
	#endif
		
		if(SendModelingCommand(MCOMMAND_MAKEEDITABLE, mcd))
		{
			BaseObject *newOp = static_cast<BaseObject*>(mcd.result->GetIndex(0));
			if(newOp)
			{
				op = newOp;
				SplitObject(oData,doc,op,prM);
			}
		}
	}

	pr = op;
	for(op=op->GetDown(); op; op=op->GetNext())
	{
		BuildSymmetry(hh,bt,prM,pr,op,oData,doc);
	}

	// check for user break
	return !bt || !bt->TestBreak();
}

Bool CDDynamicSymmetry::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseObject *op = (BaseObject*)node;
	BaseContainer *data = op->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(909), ar);
	if(bc) bc->SetBool(DESC_HIDE, false);
	
	bc = description->GetParameterI(DescLevel(DS_PURCHASE), ar);
	if(bc)
	{
		if(!data->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node, description, flags);
}

Bool CDDynamicSymmetry::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseObject	*op  = (BaseObject*)node; if(!op) return false;
	BaseContainer *oData = op->GetDataInstance(); if(!oData) return false;
	
	switch (id[0].id)
	{
		case DS_SHOW_GUIDE:	
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case DS_GUIDE_SIZE:	
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case DS_GUIDE_COLOR:	
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case DS_SYMMETRY_AXIS:	
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case DS_TOLERANCE:	
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case DS_ACTIVE_SYMMETRY:	
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case DS_AUTO_UPDATE:
			if(!oData->GetBool(T_REG)) return false;
			else return AutoUpdateAvailable(op);
		case DS_LOCK_CENTER:
			if(!oData->GetBool(T_REG)) return false;
			else return oData->GetBool(DS_AUTO_UPDATE);
		case DS_KEEP_SEL:
			if(!oData->GetBool(T_REG)) return false;
			else return oData->GetBool(DS_AUTO_UPDATE);
		case DS_KEEP_UVW:
			if(!oData->GetBool(T_REG)) return false;
			else return oData->GetBool(DS_AUTO_UPDATE);
	}
	return true;
}

Bool CDDynamicSymmetry::Message(GeListNode *node, LONG type, void *t_data)
{
	BaseObject *op = (BaseObject*)node; if(!op) return true;
	BaseContainer *oData = op->GetDataInstance(); if(!oData) return true;
	
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			oData->SetBool(T_SET,false);
			CHAR snData[CDSY_SERIAL_SIZE];
			String cdsnr;
			
			CDReadPluginInfo(ID_CDSYMMETRYTOOLS,snData,CDSY_SERIAL_SIZE);
			cdsnr.SetCString(snData,CDSY_SERIAL_SIZE-1);
			oData->SetString(T_STR,cdsnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			oData->SetBool(T_SET,false);
			CHAR snData[CDSY_SERIAL_SIZE];
			String cdsnr;
			
			CDReadPluginInfo(ID_CDSYMMETRYTOOLS,snData,CDSY_SERIAL_SIZE);
			cdsnr.SetCString(snData,CDSY_SERIAL_SIZE-1);
			oData->SetString(T_STR,cdsnr);
			break;
		}
	}
	CheckOpReg(oData);
	BaseDocument *doc = op->GetDocument(); if(!doc) return true;
	
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			if(oData->GetBool(DS_AUTO_UPDATE))
			{
				BaseObject *ch = op->GetDown();
				if(ch)
				{
					BaseTag *clTag = ch->GetTag(ID_CDCENTERLOCKTAG);
					if(!clTag)
					{
						clTag = BaseTag::Alloc(ID_CDCENTERLOCKTAG);
						ch->InsertTag(clTag,NULL);
					}
					
					BaseContainer *tData = clTag->GetDataInstance();
					if(tData)
					{
						switch(oData->GetLong(DS_SYMMETRY_AXIS))
						{
							case DS_MX:
								tData->SetLong(CL_SYMMETRY_AXIS,0);
								break;
							case DS_MY:
								tData->SetLong(CL_SYMMETRY_AXIS,1);
								break;
							case DS_MZ:
								tData->SetLong(CL_SYMMETRY_AXIS,2);
								break;
						}
						tData->SetReal(CL_TOLERANCE,oData->GetReal(DS_TOLERANCE));
						tData->SetBool(CL_LOCK_ON,oData->GetBool(DS_LOCK_CENTER));
						clTag->Message(CD_MSG_UPDATE);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*)t_data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == DS_AUTO_UPDATE)
			{
				if(oData->GetBool(DS_AUTO_UPDATE))
				{
					if(QuestionDialog(GeLoadString(QD_PRESERVE_UVW))) oData->SetBool(DS_KEEP_UVW,true);
					
					// rebuild cache
					ModelingCommandData mcd;
					mcd.doc = doc;
					mcd.op = op;
				#if API_VERSION < 12000
					mcd.mode = MODIFY_ALL;
				#else
					mcd.mode = MODELINGCOMMANDMODE_ALL;
				#endif

					if(!SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT, mcd)) return false;

					BaseObject* tempOp = static_cast<BaseObject*>(mcd.result->GetIndex(0));
					BaseObject::Free(tempOp);
					
					// get cache object
					if(op->GetCache())
					{
						CacheToMesh(doc,op,op->GetDown());
					}
					BaseObject *ch = op->GetDown();
					if(ch)
					{
						BaseTag *clTag = ch->GetTag(ID_CDCENTERLOCKTAG);
						if(!clTag)
						{
							clTag = BaseTag::Alloc(ID_CDCENTERLOCKTAG);
							ch->InsertTag(clTag,NULL);
						}
						
						BaseContainer *tData = clTag->GetDataInstance();
						if(tData)
						{
							switch(oData->GetLong(DS_SYMMETRY_AXIS))
							{
								case DS_MX:
									tData->SetLong(CL_SYMMETRY_AXIS,0);
									break;
								case DS_MY:
									tData->SetLong(CL_SYMMETRY_AXIS,1);
									break;
								case DS_MZ:
									tData->SetLong(CL_SYMMETRY_AXIS,2);
									break;
							}
							tData->SetReal(CL_TOLERANCE,oData->GetReal(DS_TOLERANCE));
							tData->SetBool(CL_LOCK_ON,oData->GetBool(DS_LOCK_CENTER));
							clTag->Message(CD_MSG_UPDATE);
						}
					}
				}
				else
				{
					BaseObject *ch = op->GetDown();
					if(ch)
					{
						BaseTag *clTag = ch->GetTag(ID_CDCENTERLOCKTAG);
						if(clTag) BaseTag::Free(clTag);
					}
					oData->SetBool(DS_LOCK_CENTER,false);
					oData->SetBool(DS_KEEP_SEL,false);
					oData->SetBool(DS_KEEP_UVW,false);
				}
			}
			else if(descID[0].id  == DS_SYMMETRY_AXIS)
			{
				if(oData->GetBool(DS_AUTO_UPDATE))
				{
					BaseObject *ch = op->GetDown();
					if(ch)
					{
						BaseTag *clTag = ch->GetTag(ID_CDCENTERLOCKTAG);
						if(clTag)
						{
							BaseContainer *tData = clTag->GetDataInstance();
							if(tData)
							{
								switch(oData->GetLong(DS_SYMMETRY_AXIS))
								{
									case DS_MX:
										tData->SetLong(CL_SYMMETRY_AXIS,0);
										break;
									case DS_MY:
										tData->SetLong(CL_SYMMETRY_AXIS,1);
										break;
									case DS_MZ:
										tData->SetLong(CL_SYMMETRY_AXIS,2);
										break;
								}
								clTag->Message(CD_MSG_UPDATE);
							}
						}
					}
				}
			}
			else if(descID[0].id  == DS_TOLERANCE)
			{
				if(oData->GetBool(DS_AUTO_UPDATE))
				{
					BaseObject *ch = op->GetDown();
					if(ch)
					{
						BaseTag *clTag = ch->GetTag(ID_CDCENTERLOCKTAG);
						if(clTag)
						{
							BaseContainer *tData = clTag->GetDataInstance();
							if(tData)
							{
								tData->SetReal(CL_TOLERANCE,oData->GetReal(DS_TOLERANCE));
								clTag->Message(CD_MSG_UPDATE);
							}
						}
					}
				}
			}
			else if(descID[0].id  == DS_LOCK_CENTER)
			{
				BaseObject *ch = op->GetDown();
				if(ch)
				{
					BaseTag *clTag = ch->GetTag(ID_CDCENTERLOCKTAG);
					if(clTag)
					{
						BaseContainer *tData = clTag->GetDataInstance();
						if(tData)
						{
							tData->SetBool(CL_LOCK_ON,oData->GetBool(DS_LOCK_CENTER));
						}
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) t_data;
			if(dc->id[0].id==DS_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

BaseObject* CDDynamicSymmetry::CDGetVirtualObjects(BaseObject* op, HierarchyHelp* hh)
{
	BaseContainer *oData = op->GetDataInstance(); if(!oData) return NULL;
	if(!AutoUpdateAvailable(op))
	{
		oData->SetBool(DS_AUTO_UPDATE,false);
	}
	
	BaseDocument *doc = op->GetDocument(); if(!doc) return NULL;
	AutoAlloc<BaseDocument> tDoc; if(!tDoc) return NULL;

	BaseThread *bt=hh->GetThread();
	BaseObject *orig = op->GetDown(); if(!orig) return NULL;
	Matrix prM = Matrix();
	
#if API_VERSION < 12000
	hh->AddVFlags(VFLAG_POLYGONAL|VFLAG_ISOPARM);
#endif

	// generate clone of input object
	Bool dirty = false;
#if API_VERSION < 12000
	#if API_VERSION < 9800
		PluginObject *pOp = (PluginObject*)op;
		BaseObject *hCln = pOp->GetAndCheckHierarchyClone(hh,orig,HCLONE_ASPOLY|HCLONE_ASSPLINE,&dirty,NULL,false);
	#else
		BaseObject *hCln = op->GetAndCheckHierarchyClone(hh,orig,HCLONE_ASPOLY|HCLONE_ASSPLINE,&dirty,NULL,false); 
	#endif
#else
	BaseObject *hCln = op->GetAndCheckHierarchyClone(hh,orig,HIERARCHYCLONEFLAGS_ASPOLY|HIERARCHYCLONEFLAGS_ASSPLINE,&dirty,NULL,false);
#endif

	if(!dirty) return hCln; // if !dirty object doesn't need to be rebuilt
	if(!hCln) return NULL;

	if(oData->GetBool(DS_AUTO_UPDATE))
	{
		Matrix opM = hCln->GetMg();
		Matrix newM = opM;
		newM.off = prM.off;
		RecalculatePoints(hCln,newM,opM);
		hCln->SetMg(newM);
		
		SpecialEventAdd(CD_MSG_SYMMETRY_UPDATE);
	}
	else
	{
		oData->SetBool(DS_LOCK_CENTER,false);
		oData->SetBool(DS_KEEP_SEL,false);
		oData->SetBool(DS_KEEP_UVW,false);
	}
	
	// perform CSTO on cloned hierarchy
	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = hCln;

	if(!SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT,mcd))
	{
		blDelete(hCln);
		return NULL;
	}
	BaseObject *res = static_cast<BaseObject*>(mcd.result->GetIndex(0));
	if(!res)
	{
		blDelete(hCln);
		return NULL;
	}
	blDelete(hCln);
	tDoc->InsertObject(res,NULL,NULL,false);
	
	// go through all child hierarchies
	BuildSymmetry(hh,bt,prM,NULL,res,oData,tDoc);
	
	res->Remove();

	return res;
}

Bool RegisterCDDynamicSymmetry(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) reg = false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdsnr.FindFirst("-",&pos);
	while(h)
	{
		cdsnr.Delete(pos,1);
		h = cdsnr.FindFirst("-",&pos);
	}
	cdsnr.ToUpper();
	
	kb = cdsnr.SubStr(pK,2);
	
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
	
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDDYNSYM); if(!name.Content()) return true;

	if(!reg) return CDRegisterObjectPlugin(ID_CDDYNSYMMETRY,name,PLUGINFLAG_HIDE,CDDynamicSymmetry::Alloc,"oCDDynSym","CDDynSym.tif","CDDynSym_small.tif",0);
	else return CDRegisterObjectPlugin(ID_CDDYNSYMMETRY,name,OBJECT_GENERATOR|OBJECT_INPUT,CDDynamicSymmetry::Alloc,"oCDDynSym","CDDynSym.tif","CDDynSym_small.tif",0);	
}

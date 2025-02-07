//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"
#include "osymmetry.h"

#include "CDSymmetry.h"
#include "CDEdgeCutArray.h"
#include "CDArray.h"

class CDMakeSymmetricalDialog : public GeModalDialog
{
	private:
		CDSymOptionsUA 			ua;
		
	public:	
		LONG axis, dir;
		Real t;
		Bool uv;
		
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDMakeSymmetricalDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDMKSYM));
			
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDSY_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDSY_OPTIONS_IMAGE);
		}
		GroupEnd();
			
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_MIRROR_AXIS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(D_MIRROR_AXIS,BFH_LEFT,3,1);
					AddChild(D_MIRROR_AXIS, 0, GeLoadString(M_X));
					AddChild(D_MIRROR_AXIS, 1, GeLoadString(M_Y));
					AddChild(D_MIRROR_AXIS, 2, GeLoadString(M_Z));
					
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDC_MIRROR_DIRECTION),0);
					AddComboBox(IDC_MIRROR_DIRECTION,BFH_CENTER);
						AddChild(IDC_MIRROR_DIRECTION, 0, GeLoadString(IDS_M_POS));
						AddChild(IDC_MIRROR_DIRECTION, 1, GeLoadString(IDS_M_NEG));
				}
				GroupEnd();
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
			{
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_TOLERANCE),0);
				AddEditNumberArrows(D_TOLERANCE,BFH_LEFT);
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
			{
				AddCheckbox(D_PRESERVE_UVW,BFH_LEFT,0,0,GeLoadString(D_PRESERVE_UVW));
			}
			GroupEnd();
		}
		GroupEnd();
		
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDMakeSymmetricalDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_MAKESYMMETRICAL);
	if(!wpData)
	{
		SetLong(D_MIRROR_AXIS,0);
		SetLong(IDC_MIRROR_DIRECTION,0);
		SetReal(D_TOLERANCE,0.01,0.001,100,0.001,FORMAT_REAL);
		SetBool(D_PRESERVE_UVW,true);
	}
	else
	{
		SetLong(D_MIRROR_AXIS,wpData->GetLong(D_MIRROR_AXIS));
		SetLong(IDC_MIRROR_DIRECTION,wpData->GetLong(IDC_MIRROR_DIRECTION));
		SetReal(D_TOLERANCE,wpData->GetReal(D_TOLERANCE),0.001,100,0.001,FORMAT_REAL);
		SetBool(D_PRESERVE_UVW,wpData->GetBool(D_PRESERVE_UVW));
	}
	
	return true;
}

Bool CDMakeSymmetricalDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(D_MIRROR_AXIS,axis);
	GetLong(IDC_MIRROR_DIRECTION,dir);
	GetReal(D_TOLERANCE,t);
	GetBool(D_PRESERVE_UVW,uv);
	
	BaseContainer bc;
	bc.SetLong(D_MIRROR_AXIS,axis);
	bc.SetLong(IDC_MIRROR_DIRECTION,dir);
	bc.SetReal(D_TOLERANCE,t);
	bc.SetBool(D_PRESERVE_UVW,uv);
	SetWorldPluginData(ID_MAKESYMMETRICAL,bc,false);
	
	return true;
}


class CDMakeSymmetrical : public CommandData
{
private:
	CDMakeSymmetricalDialog		dlg;

	CDArray<LONG>	indA, indB, indC, indD, indE, indF, indG, indH;
	
	void InitSymPartitions(LONG cnt);
	void FreeSymPartitions(void);
	LONG* GetSymPartitionSpace(Vector pt, Vector mp, LONG &cnt);
	Bool BuildSymSpacePartitions(Vector *padr, CPolygon *vadr, BaseSelect *bs, Vector mp, LONG size);
	
	void GetUVWTags(BaseObject *op, BaseObject *cln, AtomArray *oUVWTags, AtomArray *cUVWTags);
	void TransferUVWCoordinates(Vector *opadr, Vector *cpadr, CPolygon oPly, CPolygon cPly, UVWStruct &ouv, UVWStruct &cuv);
	Bool CompareUVWTags(BaseObject *op, BaseObject *cln, LONG axis, LONG dir);
	
	Bool IsCrossPlaneLength(Real a, Real b);
	LONG FindCutPoint(Vector *padr, CPolygon ply, LONG axis, CutEdge e);
	Bool IsCrossPlanePoly(Vector *padr, const CPolygon &ply, LONG axis, Real t);
	LONG GetCrossPlanePolyEdges(Vector *padr, CPolygon ply, CutEdge *edg, LONG axis);
	
	Bool DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, LONG axis);
	Bool CenterCutObject(BaseDocument *doc, BaseObject *op, Matrix opM, LONG axis, Real t);
	Bool SplitObject(BaseDocument *doc, BaseObject *op, LONG axis, LONG dir, Real t, Bool uv);
	
public:
	virtual Bool Execute(BaseDocument *doc);
};

void CDMakeSymmetrical::InitSymPartitions(LONG cnt)
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

void CDMakeSymmetrical::FreeSymPartitions(void)
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

LONG* CDMakeSymmetrical::GetSymPartitionSpace(Vector pt, Vector mp, LONG &cnt)
{
	if(pt.x > mp.x)
	{
		if(pt.y > mp.y)
		{
			if(pt.z > mp.z)
			{
				cnt = indB.Size();
				return indB.Array();
			}
			else
			{
				cnt = indA.Size();
				return indA.Array();
			}
		}
		else
		{
			if(pt.z > mp.z)
			{
				cnt = indD.Size();
				return indD.Array();
			}
			else
			{
				cnt = indC.Size();
				return indC.Array();
			}
		}
	}
	else
	{
		if(pt.y > mp.y)
		{
			if(pt.z > mp.z)
			{
				cnt = indF.Size();
				return indF.Array();
			}
			else
			{
				cnt = indE.Size();
				return indE.Array();;
			}
		}
		else
		{
			if(pt.z > mp.z)
			{
				cnt = indH.Size();
				return indH.Array();
			}
			else
			{
				cnt = indG.Size();
				return indG.Array();
			}
		}
	}
	
	return NULL;
}

Bool CDMakeSymmetrical::BuildSymSpacePartitions(Vector *padr, CPolygon *vadr, BaseSelect *bs, Vector mp, LONG size)
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

void CDMakeSymmetrical::GetUVWTags(BaseObject *op, BaseObject *cln, AtomArray *oUVWTags, AtomArray *cUVWTags)
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

void CDMakeSymmetrical::TransferUVWCoordinates(Vector *opadr, Vector *cpadr, CPolygon oPly, CPolygon cPly, UVWStruct &ouv, UVWStruct &cuv)
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

Bool CDMakeSymmetrical::CompareUVWTags(BaseObject *op, BaseObject *cln, LONG axis, LONG dir)
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
			InitSymPartitions(size);
			Vector mp = GetSelectionCenter(cpadr,cvadr,cbs,bsCnt);
			
			if(!BuildSymSpacePartitions(cpadr,cvadr,cbs,mp,size))
			{
				FreeSymPartitions();
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
														
							LONG cnt;
							LONG *ind = GetSymPartitionSpace(opPt,mp,cnt);
							
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
			FreeSymPartitions();
		}
	}
	
	return true;
}

Bool CDMakeSymmetrical::DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, LONG axis)
{
	if(!IsValidPolygonObject(op)) return false;
	
	CPolygon *vadr = GetPolygonArray(op);
	Vector *padr = GetPointArray(op);
	LONG i, vCnt = ToPoly(op)->GetPolygonCount();
	
	BaseSelect *bs = ToPoly(op)->GetPolygonS();
	bs->DeselectAll();
	
	Matrix opM = op->GetMg();
	for(i=0; i<vCnt; i++)
	{
		CPolygon ply = vadr[i];
		Vector ptA = padr[ply.a];
		Vector ptB = padr[ply.b];
		Vector ptC = padr[ply.c];
		Vector ptD = padr[ply.d];
		
		switch(axis)
		{
			case 0: // x axis
				if(ptA.x == 0 && ptB.x == 0 && ptC.x == 0 && ptD.x == 0) bs->Select(i);
				break;
			case 1: // y axis
				if(ptA.y == 0 && ptB.y == 0 && ptC.y == 0 && ptD.y == 0) bs->Select(i);
				break;
			case 2: // z axis
				if(ptA.z == 0 && ptB.z == 0 && ptC.z == 0 && ptD.z == 0) bs->Select(i);
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

Bool CDMakeSymmetrical::IsCrossPlanePoly(Vector *padr, const CPolygon &ply, LONG axis, Real t)
{
	if(!padr) return false;
	
	Vector a,b,c,d;
	a = padr[ply.a];
	b = padr[ply.b];
	c = padr[ply.c];
	d = padr[ply.d];

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

Bool CDMakeSymmetrical::IsCrossPlaneLength(Real a, Real b)
{
	if( ( Abs(a)+Abs(b) ) > Abs(a+b) ) return true;
	else return false;
}

LONG CDMakeSymmetrical::GetCrossPlanePolyEdges(Vector *padr, CPolygon ply, CutEdge *edg, LONG axis)
{
	LONG cnt = 0;
	
	Vector a = padr[ply.a];
	Vector b = padr[ply.b];
	Vector c = padr[ply.c];
	Vector d = padr[ply.d];
	
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

LONG CDMakeSymmetrical::FindCutPoint(Vector *padr, CPolygon ply, LONG axis, CutEdge e)
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

Bool CDMakeSymmetrical::CenterCutObject(BaseDocument *doc, BaseObject *op, Matrix opM, LONG axis, Real t)
{
	Vector *padr = GetPointArray(op);
	if(!padr) return false;
	
	LONG i, pCnt = ToPoint(op)->GetPointCount();
	
	// snap existing center points
	for(i=0; i<pCnt; i++)
	{
		Vector pt = padr[i];
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
				if(IsCrossPlanePoly(padr,ply,axis,t)) bsPly->Select(i);
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
						StatusSetSpin();
						CPolygon ply = vadr[i];
						CutEdge edg[4];
						LONG eCnt = GetCrossPlanePolyEdges(padr,ply,edg,axis);
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
				if(!mod->Commit(op,MODELING_COMMIT_NO_NGONS|MODELING_COMMIT_ADDUNDO)) return false;
				
				if(edges.Size() > 0) edges.Free();
			}
		}
	}
	else
	{
		Vector center = opM.off;
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
	
	StatusSetSpin();
	CDOptimize(doc,op,0.001,true,false,true,CD_MODELINGCOMMANDMODE_ALL);
	
	return true;
}

Bool CDMakeSymmetrical::SplitObject(BaseDocument *doc, BaseObject *op, LONG axis, LONG dir, Real t, Bool uv)
{
	if(!doc || !op) return false;
	
	Matrix opM = op->GetMg();
	
	Bool isPoly = true;
	if(!IsValidPolygonObject(op))  isPoly = false;
	
	// split object
	StatusSetText("Processing");
	if(!CenterCutObject(doc,op,opM,axis,t)) return false;
	
	// delete unused points after split
	Vector *padr = GetPointArray(op);
	if(!padr) return false;
	
	BaseSelect *bsPt = ToPoint(op)->GetPointS();
	if(bsPt) bsPt->DeselectAll();
	
	LONG i, pCnt = ToPoint(op)->GetPointCount();
	
	// snap points to center & select points to remove
	for(i=0; i<pCnt; i++)
	{
		StatusSetSpin();
		Vector pt = padr[i];
		switch(dir)
		{
			case 0:
				switch(axis)
				{
					case 0:
						if(pt.x > -t)
						{
							if(pt.x < t)
							{
								pt.x = 0.0;
								padr[i] = pt;
							}
							else bsPt->Select(i);
						}
						break;
					case 1:
						if(pt.y > -t)
						{
							if(pt.y < t)
							{
								pt.y = 0.0;
								padr[i] = pt;
							}
							else bsPt->Select(i);
						}
						break;
					case 2:
						if(pt.z > -t)
						{
							if(pt.z < t)
							{
								pt.z = 0.0;
								padr[i] = pt;
							}
							else bsPt->Select(i);
						}
						break;
				}
				break;
			case 1:
				switch(axis)
				{
					case 0:
						if(pt.x < t)
						{
							if(pt.x > -t)
							{
								pt.x = 0.0;
								padr[i] =  pt;
							}
							else bsPt->Select(i);
						}
						break;
					case 1:
						if(pt.y < t)
						{
							if(pt.y > -t)
							{
								pt.y = 0.0;
								padr[i] = pt;
							}
							else bsPt->Select(i);
						}
						break;
					case 2:
						if(pt.z < t)
						{
							if(pt.z > -t)
							{
								pt.z = 0.0;
								padr[i] = pt;
							}
							else bsPt->Select(i);
						}
						break;
				}
				break;
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
			StatusSetSpin();
			CPolygon ply = vadr[i];
			Vector pt = CalcPolygonCenter(padr,ply);
			switch(dir)
			{
				case 0:
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
				case 1:
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
			}
		}
	}
	
	BaseObject *delCln = NULL;
	if(isPoly && uv) delCln = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,NULL); // clone op to compare uvw's
	
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
	

	StatusSetSpin();
	if(!SendModelingCommand(MCOMMAND_DELETE, mcd)) return false;
	
	// delete center plane polygons
	if(isPoly)
	{
		if(bsPly) bsPly->DeselectAll();
		DeleteSymmetryPlanePolys(doc,op,axis);
	}
	
	StatusSetSpin();
	CDOptimize(doc,op,t,false,false,true,CD_MODELINGCOMMANDMODE_ALL);
	
	// mirror plygon geometry
	BaseContainer mirData;
	switch(axis)
	{
		case 0:
			mirData.SetLong(MDATA_MIRROR_PLANE,1);
			mirData.SetVector(MDATA_MIRROR_VECTOR,Vector(1,0,0));
			mirData.SetVector(MDATA_MIRROR_POINT,Vector(opM.off.x,0,0));
			break;
		case 1:
			mirData.SetLong(MDATA_MIRROR_PLANE,2);
			mirData.SetVector(MDATA_MIRROR_VECTOR,Vector(0,1,0));
			mirData.SetVector(MDATA_MIRROR_POINT,Vector(0,opM.off.y,0));
			break;
		case 2:
			mirData.SetLong(MDATA_MIRROR_PLANE,0);
			mirData.SetVector(MDATA_MIRROR_VECTOR,Vector(0,0,1));
			mirData.SetVector(MDATA_MIRROR_POINT,Vector(0,0,opM.off.z));
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
	
	mcd.bc = &mirData;
#if API_VERSION < 12000	
	mcd.mode = MODIFY_ALL;
#else
	mcd.mode = MODELINGCOMMANDMODE_ALL;
#endif
	
	StatusSetSpin();
	if(!SendModelingCommand(MCOMMAND_MIRROR, mcd)) return false;

	if(isPoly && uv)
	{
		CompareUVWTags(op,delCln,axis,dir);
		BaseObject::Free(delCln);
	}
	
	return true;
}

Bool CDMakeSymmetrical::Execute(BaseDocument *doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG  i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		if(!dlg.Open()) return false;
		
		StatusSetSpin();
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				Matrix opM = op->GetMg();
				
				if(IsValidPointObject(op))
				{
					BaseTag *tag = op->GetTag(ID_CDSYMMETRYTAG);
					if(tag)
					{
						CDAddUndo(doc,CD_UNDO_DELETE,tag);
						BaseTag::Free(tag);
					}
					
					StatusSetSpin();
					SplitObject(doc,op,dlg.axis,dlg.dir,dlg.t,dlg.uv);
					StatusClear();
				}
				else if((op->GetInfo() & OBJECT_GENERATOR) && !(op->GetInfo() & OBJECT_INPUT))
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
							StatusSetSpin();
							SplitObject(doc,op,dlg.axis,dlg.dir,dlg.t,dlg.uv);
							StatusClear();
						}
					}
				}
			}
		}
		StatusClear();
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

class CDMakeSymmetricalR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMakeSymmetrical(void)
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
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDMKSYM); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_MAKESYMMETRICAL,name,PLUGINFLAG_HIDE,"CDMakeSym.tif","CD Make Symmetrical",CDDataAllocator(CDMakeSymmetricalR));
	else return CDRegisterCommandPlugin(ID_MAKESYMMETRICAL,name,0,"CDMakeSym.tif","CD Make Symmetrical",CDDataAllocator(CDMakeSymmetrical));
}

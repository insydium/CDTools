//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"
#include "osymmetry.h"

#include "tCDSymTag.h"
#include "CDSymmetry.h"
#include "CDEdgeCutArray.h"

class CDAddSymmetryObjectDialog : public GeModalDialog
{
	private:
		CDSymOptionsUA 			ua;
		
	public:	
		LONG axis, dir;
		Real tolerance;
		
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDAddSymmetryObjectDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDTAGTOSYM));
			
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
		}
		GroupEnd();
		
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDAddSymmetryObjectDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDTAGTOSYMMETRY);
	if(!wpData)
	{
		SetLong(D_MIRROR_AXIS,0);
		SetLong(IDC_MIRROR_DIRECTION,0);
		SetReal(D_TOLERANCE,0.01,0.001,100,0.001,FORMAT_REAL);
	}
	else
	{
		SetLong(D_MIRROR_AXIS,wpData->GetLong(D_MIRROR_AXIS));
		SetLong(IDC_MIRROR_DIRECTION,wpData->GetLong(IDC_MIRROR_DIRECTION));
		SetReal(D_TOLERANCE,wpData->GetReal(D_TOLERANCE),0.001,100,0.001,FORMAT_REAL);
	}
	
	return true;
}

Bool CDAddSymmetryObjectDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(D_MIRROR_AXIS,axis);
	GetLong(IDC_MIRROR_DIRECTION,dir);
	GetReal(D_TOLERANCE,tolerance);
	
	BaseContainer bc;
	bc.SetLong(D_MIRROR_AXIS,axis);
	bc.SetLong(IDC_MIRROR_DIRECTION,dir);
	bc.SetReal(D_TOLERANCE,tolerance);
	SetWorldPluginData(ID_CDTAGTOSYMMETRY,bc,false);
	
	return true;
}


class CDAddSymmetryObject : public CommandData
{
	private:
		CDAddSymmetryObjectDialog dlg;
		
		void AddSymmetryObject(BaseDocument *doc, BaseObject *op, Matrix m, LONG axis);
		
		Bool IsCrossPlaneLength(Real a, Real b);
		LONG FindCutPoint(Vector *padr, CPolygon ply, LONG axis, CutEdge e);
		Bool IsCrossPlanePoly(Vector *padr, const CPolygon &ply, Matrix opM, Matrix prM, LONG axis, Real t);
		LONG GetCrossPlanePolyEdges(Vector *padr, CPolygon ply, CutEdge *edg, Matrix opM, Matrix prM, LONG axis);
		
		Bool DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis);
		Bool CenterCutObject(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis, Real t);
		Bool SplitObject(BaseDocument *doc, BaseObject *op, Matrix prM, LONG axis, LONG dir, Real t);
		void SplitChildren(BaseDocument *doc, BaseObject *ch, Matrix prM, LONG axis, LONG dir, Real t);
		
	public:
		virtual Bool Execute(BaseDocument *doc);
};

void CDAddSymmetryObject::AddSymmetryObject(BaseDocument *doc, BaseObject *op, Matrix m, LONG axis)
{
	BaseObject *sym = BaseObject::Alloc(Osymmetry);
	if(sym)
	{
		doc->InsertObject(sym,op->GetUp(),op->GetPred(),true);
		CDAddUndo(doc,CD_UNDO_NEW,sym);
		
		sym->SetMg(m);
		
		Matrix opM = op->GetMg();
		CDAddUndo(doc,CD_UNDO_CHANGE,op);
		op->Remove();
		doc->InsertObject(op,sym,NULL,false);
		op->SetMg(opM);
		
		BaseContainer *sData = sym->GetDataInstance();
		if(sData)
		{
			switch(axis)
			{
				case 0:
					sData->SetLong(SYMMETRYOBJECT_PLANE,SYMMETRYOBJECT_PLANE_YZ); // x axis
					break;
				case 1:
					sData->SetLong(SYMMETRYOBJECT_PLANE,SYMMETRYOBJECT_PLANE_XZ); // y axis
					break;
				case 2:
					sData->SetLong(SYMMETRYOBJECT_PLANE,SYMMETRYOBJECT_PLANE_XY); // z axis
					break;
			}
		}
		
	}
}

Bool CDAddSymmetryObject::DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis)
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

Bool CDAddSymmetryObject::IsCrossPlanePoly(Vector *padr, const CPolygon &ply, Matrix opM, Matrix prM, LONG axis, Real t)
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

Bool CDAddSymmetryObject::IsCrossPlaneLength(Real a, Real b)
{
	if( ( Abs(a)+Abs(b) ) > Abs(a+b) ) return true;
	else return false;
}

LONG CDAddSymmetryObject::GetCrossPlanePolyEdges(Vector *padr, CPolygon ply, CutEdge *edg, Matrix opM, Matrix prM, LONG axis)
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

LONG CDAddSymmetryObject::FindCutPoint(Vector *padr, CPolygon ply, LONG axis, CutEdge e)
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

Bool CDAddSymmetryObject::CenterCutObject(BaseDocument *doc, BaseObject *op, Matrix opM, Matrix prM, LONG axis, Real t)
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
				if(!mod->Commit(op,MODELING_COMMIT_NO_NGONS|MODELING_COMMIT_ADDUNDO)) return false;
				
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

Bool CDAddSymmetryObject::SplitObject(BaseDocument *doc, BaseObject *op, Matrix prM, LONG axis, LONG dir, Real t)
{
	if(!doc || !op) return false;
	
	Matrix opM = op->GetMg();
	
	Bool isPoly = true;
	if(!IsValidPolygonObject(op))  isPoly = false;
	
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
			CPolygon ply = vadr[i];
			Vector pt = MInv(prM) * (opM * CalcPolygonCenter(padr,ply));
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
	
	return true;
}

void CDAddSymmetryObject::SplitChildren(BaseDocument *doc, BaseObject *ch, Matrix prM, LONG axis, LONG dir, Real t)
{
	while(ch)
	{
		Matrix chM = ch->GetMg();
		
		if(IsValidPointObject(ch))
		{
			BaseTag *tag = ch->GetTag(ID_CDSYMMETRYTAG);
			if(tag)
			{
				CDAddUndo(doc,CD_UNDO_DELETE,tag);
				BaseTag::Free(tag);
			}
					
			RepositionChildren(doc,ch->GetDown(),prM,chM,true);
			
			CDAddUndo(doc,CD_UNDO_CHANGE,ch);
			RecalculatePoints(ch,prM,chM);
			ch->SetMg(prM);
			
			SplitObject(doc,ch,prM,axis,dir,t);
		}
		else if((ch->GetInfo() & OBJECT_GENERATOR) && !(ch->GetInfo() & OBJECT_INPUT))
		{
			ModelingCommandData mcd;
			mcd.doc = doc;
			mcd.op = ch;
		#if API_VERSION  < 12000
			mcd.flags = MODELINGCOMMANDFLAG_CREATEUNDO;
		#else
			mcd.flags = MODELINGCOMMANDFLAGS_CREATEUNDO;
		#endif

			if(SendModelingCommand(MCOMMAND_MAKEEDITABLE, mcd))
			{
				BaseObject *newOp = static_cast<BaseObject*>(mcd.result->GetIndex(0));
				if(newOp)
				{
					ch = newOp;
					RepositionChildren(doc,ch->GetDown(),prM,chM,true);
					
					CDAddUndo(doc,CD_UNDO_CHANGE,ch);
					RecalculatePoints(ch,prM,chM);
					ch->SetMg(prM);
					
					SplitObject(doc,ch,prM,axis,dir,t);
				}
			}
		}
		SplitChildren(doc,ch->GetDown(),prM,axis,dir,t);
		
		ch = ch->GetNext();
	}
}

Bool CDAddSymmetryObject::Execute(BaseDocument *doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG  i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		if(!dlg.Open()) return false;
		
		StatusSetText("Processing");
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
					
					SplitObject(doc,op,opM,dlg.axis,dlg.dir,dlg.tolerance);
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
							SplitObject(doc,op,opM,dlg.axis,dlg.dir,dlg.tolerance);
						}
					}
				}
				SplitChildren(doc,op->GetDown(),opM,dlg.axis,dlg.dir,dlg.tolerance);
				
				AddSymmetryObject(doc,op,opM,dlg.axis);
			}
		}
		StatusClear();
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

class CDAddSymmetryObjectR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddSymmetryObject(void)
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
	String name=GeLoadString(IDS_CDTAGTOSYM); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDTAGTOSYMMETRY,name,PLUGINFLAG_HIDE,"CDAddSymObj.tif","CD Convert to Symmetry",CDDataAllocator(CDAddSymmetryObjectR));
	else return CDRegisterCommandPlugin(ID_CDTAGTOSYMMETRY,name,0,"CDAddSymObj.tif","CD Convert to Symmetry",CDDataAllocator(CDAddSymmetryObject));
}

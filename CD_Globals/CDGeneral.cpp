//	Cactus Dan's plugin globals
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"

#include "CDCompatibility.h"
#include "CDGeneral.h"
#include "CDAABB.h"
#include "CDDebug.h"

Real GetCDPluginVersion(LONG id)
{
	Real vrs = 0.0;
	BaseContainer *wpData = GetWorldPluginData(id);
	if(wpData) vrs = wpData->GetReal(CD_VERSION);
	return vrs;
}

// compatibility functions
Bool CDIsCommandChecked(LONG id)
{
	if(!GeIsMainThread()) return false;
	BaseContainer msg(300000115);
	msg.SetLong(300000115, id);
	return SendCoreMessage(COREMSG_CINEMA, msg, 0).GetLong();
}

Bool CDIsAxisMode(BaseDocument *doc)
{
	if(!doc) return false;
	
#if API_VERSION	< 13000
	return doc->GetMode() == Maxis;
#else
	return doc->IsAxisEnabled();
#endif
}

void CDGeGetSerialInfo(LONG type, SerialInfo *si)
{
#if API_VERSION < 12000
	GeGetSerialInfo(type, si);
#else
	GeGetSerialInfo((SERIALINFO)type, si);
#endif
}

Bool CDReadPluginInfo(LONG pluginid, void *buffer, LONG size)
{
#if API_VERSION	< 11000	
	return ReadPluginInfo(pluginid, buffer, size); // single license
#else
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_MULTILICENSE, &si);
	
	if(si.nr.Content())
		return ReadRegInfo(pluginid, buffer, size); // multi license
	else
		return ReadPluginInfo(pluginid, buffer, size); // single license
#endif
}

Bool CDWritePluginInfo(LONG pluginid, void *buffer, LONG size)
{
#if API_VERSION	< 11000	
	return WritePluginInfo(pluginid, buffer, size); // single license
#else
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_MULTILICENSE, &si);
		
	if(si.nr.Content())
		return WriteRegInfo(pluginid, buffer, size); // multi license
	else
		return WritePluginInfo(pluginid, buffer, size); // single license
#endif
}

Bool CDCheckC4DVersion(String pName)
{
	// check for c4d versions
	LONG c4dVers = GetC4DVersion();
	
#if API_VERSION == 15000
	if(c4dVers > 15999) // R15
	{
		GePrint(pName+" for R15 can't run in Cinema 4D R"+CDRealToString(Real(c4dVers) * 0.001));
		GePrint("Please download the correct version of "+pName+" for your version of Cinema 4D");
		return false;
	}
#elif API_VERSION == 14000
	if(c4dVers > 14999) // R14
	{
		GePrint(pName+" for R14 can't run in Cinema 4D R"+CDRealToString(Real(c4dVers) * 0.001));
		GePrint("Please download the correct version of "+pName+" for your version of Cinema 4D");
		return false;
	}
#elif API_VERSION == 13000
	if(c4dVers > 13999) // R13
	{
		GePrint(pName+" for R13 can't run in Cinema 4D R"+CDRealToString(Real(c4dVers) * 0.001));
		GePrint("Please download the correct version of "+pName+" for your version of Cinema 4D");
		return false;
	}
#elif API_VERSION == 12000
	if(c4dVers > 12999) // R12
	{
		GePrint(pName+" for R12 can't run in Cinema 4D R"+CDRealToString(Real(c4dVers) * 0.001));
		GePrint("Please download the correct version of "+pName+" for your version of Cinema 4D");
		return false;
	}
#elif API_VERSION == 11000
	if(c4dVers > 11999) // R11
	{
		GePrint(pName+" for R11 can't run in Cinema 4D R"+CDRealToString(Real(c4dVers) * 0.001));
		GePrint("Please download the correct version of "+pName+" for your version of Cinema 4D");
		return false;
	}
#elif API_VERSION == 10500
	if(c4dVers > 10999) // R10.5
	{
		GePrint(pName+" for R10 can't run in Cinema 4D R"+CDRealToString(Real(c4dVers) * 0.001));
		GePrint("Please download the correct version of "+pName+" for your version of Cinema 4D");
		return false;
	}
#elif API_VERSION == 9800
	if(c4dVers > 10499) // R10
	{
		return false;
	}
#else
	if(c4dVers > 9999) // R9
	{
		GePrint(pName+" for R9 can't run in Cinema 4D R"+CDRealToString(Real(c4dVers) * 0.001));
		GePrint("Please download the correct version of "+pName+" for your version of Cinema 4D");
		return false;
	}
#endif

	return true;
}

Bool CDDrawLayerXray(BaseDocument *doc, BaseObject *op)
{
#if API_VERSION < 9800
	if(!op || !doc) return false;
	return true;
#else
	if(!op || !doc) return false;
	
	LayerObject *layer = op->GetLayerObject(doc);
	if(layer)
	{
		LayerData lData(*layer->GetLayerData(doc, true));
		return lData.view;
	}
    return true;
#endif
}

Vector CDGetActiveViewColor()
{
#if API_VERSION < 9800
	return GetWorldContainer().GetVector(WPREF_ACTIVEPOLYBOX_COL);
#else
	return GetViewColor(VIEWCOLOR_ACTIVEPOLYBOX);
#endif
}

Vector CDGetActiveChildViewColor()
{
#if API_VERSION < 9800
	return GetWorldContainer().GetVector(WPREF_ACTIVEPOLYCHILDBOX_COL);
#else
	return GetViewColor(VIEWCOLOR_ACTIVEPOLYCHILDBOX);
#endif
}

Vector CDGetVertexStartColor()
{
#if API_VERSION < 9800
	return GetWorldContainer().GetVector(WPREF_VERTEXSTART_COL);
#else
	return GetViewColor(VIEWCOLOR_VERTEXSTART);
#endif
}

Vector CDGetVertexEndColor()
{
#if API_VERSION < 9800
	return GetWorldContainer().GetVector(WPREF_VERTEXEND_COL);
#else
	return GetViewColor(VIEWCOLOR_VERTEXEND);
#endif
}




// geometry functions
Bool IsValidPointObject(BaseObject *op)
{
	if(!op) return false;
	
#if API_VERSION < 9800
	if(op->IsInstanceOf(Opoint) && ToPoint(op)->GetPoint()) return true;
	else return false;
#else
	if(op->IsInstanceOf(Opoint) && ToPoint(op)->GetPointR()) return true;
	else return false;
#endif
}

Vector* GetPointArray(BaseObject *op)
{
	if(!IsValidPointObject(op)) return NULL;
	
#if API_VERSION < 9800
	return ToPoint(op)->GetPoint();
#else
	return ToPoint(op)->GetPointW();
#endif
}

Bool IsValidSplineObject(BaseObject *op)
{
	if(!op) return false;
	
	if(!(op->GetInfo() & OBJECT_ISSPLINE)) return false;
	if(!IsValidPointObject(op)) return false;

	return true;	
}

Segment* GetSegmentArray(SplineObject *spl)
{
	if(!IsValidSplineObject(spl)) return NULL;
	
#if API_VERSION < 9800
	return spl->GetSegment();
#else
	return spl->GetSegmentW();
#endif
}

Tangent* GetTangentArray(BaseObject *op)
{
	if(!IsValidSplineObject(op)) return NULL;
	
#if API_VERSION < 9800
	return ToSpline(op)->GetTangent();
#else
	return ToSpline(op)->GetTangentW();
#endif
}

Vector CalcPointNormal(BaseObject *op, Vector *padr, LONG ind)
{
	Vector pt, norm, ptNorm = Vector(0, 0, 0);
	
	CPolygon *vadr = GetPolygonArray(op);
	if(vadr)
	{
		LONG pCnt = ToPoint(op)->GetPointCount(), vCnt = ToPoly(op)->GetPolygonCount();
		LONG *dadr = NULL, dcnt = 0, p;
		CPolygon ply;
		
		Neighbor n;
		n.Init(pCnt,vadr,vCnt,NULL);
		n.GetPointPolys(ind, &dadr, &dcnt);

		pt = padr[ind];
		Real totalLen = 0.0;
		for(p=0; p<dcnt; p++)
		{
			ply = vadr[dadr[p]];
			totalLen += Len(pt - CalcPolygonCenter(padr, ply));
		}
		for(p=0; p<dcnt; p++)
		{
			ply = vadr[dadr[p]];
			norm = CalcFaceNormal(padr, ply);
			ptNorm += norm * Len(pt - CalcPolygonCenter(padr, ply)) / totalLen;
		}
	}
	else
	{
		ptNorm = padr[ind];
	}
	
	return VNorm(ptNorm);
}

Vector GetBestFitPlaneNormal(Vector *padr, LONG cnt)
{
	if(!padr) return Vector(0, 1, 0);
	
	if(cnt < 3) return Vector(0, 1, 0);
	
	Vector norm = Vector();
	Vector pV = padr[cnt-1];
	
	LONG i;
	for(i=0; i<cnt; i++)
	{
		Vector cV = padr[i];
		norm.x += (pV.z + cV.z) * (pV.y - cV.y);
		norm.y += (pV.x + cV.x) * (pV.z - cV.z);
		norm.z += (pV.y + cV.y) * (pV.x - cV.x);
		pV = cV;
	}
	
	Real dot = VDot(VNorm(norm), Vector(0, 1, 0));
	if(dot < 0.0) norm *= -1;
	
	return VNorm(norm);
}

Real GetNearestSplinePoint(SplineObject *spl, Vector p, LONG seg, Bool unif)
{
	Real n = 0.0, length = CDMAXREAL, start = 0.0, step = 0.1;
	Vector sPt, prvPt, nxtPt;
	
	LONG i, j;
	for(j=0; j<6; j++)
	{
		// nearest spline point in ten steps
		for(i=0; i<=10; i++)
		{
			if(unif) sPt = spl->GetSplinePoint(CDUniformToNatural(spl, start + (Real)i * step), seg);
			else sPt = spl->GetSplinePoint(start + (Real)i * step, seg);
			if(Len(p - sPt) < length)
			{
				length = Len(p - sPt);
				n = start + (Real)i * step;
			}
		}
		
		if(!spl->IsClosed())
		{
			// Get the previous and next spline point of n
			if(unif)
			{
				prvPt = spl->GetSplinePoint(CDUniformToNatural(spl, n-step), seg);
				nxtPt = spl->GetSplinePoint(CDUniformToNatural(spl, n+step), seg);
			}
			else
			{
				prvPt = spl->GetSplinePoint(n-step, seg);
				nxtPt = spl->GetSplinePoint(n+step, seg);
			}
			
			// Determine the next starting point and divide the next step by 10
			if(Len(prvPt - p) < Len(nxtPt - p)) start = n-step;
			else
			{
				if(n >= 1.0) start = n-step;
				else start = n;
			}
			if(start < 0.0) start = 0.0;
		}
		else
		{
			Real prvStep = n-step, nxtStep = n+step;
			if(prvStep < 0.0) prvStep += 1.0;
			if(nxtStep > 1.0) nxtStep -= 1.0;
			
			// Get the previous and next spline point of n
			if(unif)
			{
				prvPt = spl->GetSplinePoint(CDUniformToNatural(spl, prvStep), seg);
				nxtPt = spl->GetSplinePoint(CDUniformToNatural(spl, nxtStep), seg);
			}
			else
			{
				prvPt = spl->GetSplinePoint(prvStep, seg);
				nxtPt = spl->GetSplinePoint(nxtStep, seg);
			}
			
			// Determine the next starting point and divide the next step by 10
			if(Len(prvPt - p) < Len(nxtPt - p)) start = n-step;
			else
			{
				if(n >= 1.0) start = n-step;
				else start = n;
			}
			if(start < 0.0) start += 1.0;
			if(start > 1.0) start -= 1.0;
		}
		step *= 0.1;
	}
	
	if(!spl->IsClosed())
	{
		if(n < 0.0)	n = 0.0;
		if(n > 1.0) n = 1.0;
	}
	
	return n; // returns spline position 0.0 to 1.0
}

void RecalculatePoints(BaseObject *op, Matrix newM, Matrix oldM)
{
	if(op)
	{
		Vector pt, *padr = GetPointArray(op);
		Tangent *tngt = NULL;
		
		if(padr)
		{
			LONG i, ptCnt = ToPoint(op)->GetPointCount();
			
			BaseContainer *oData = op->GetDataInstance();
			if(op->IsInstanceOf(Ospline) && oData->GetLong(SPLINEOBJECT_TYPE) == SPLINEOBJECT_TYPE_BEZIER)
			{
				tngt = GetTangentArray(op);
			}
			
			Matrix nTngM = newM;
			nTngM.off = oldM.off;
			
			for(i=0; i<ptCnt; i++)
			{
				pt = MInv(newM) * oldM * padr[i];
				padr[i] = pt;
				
				if(tngt)
				{
					
					Vector tngL = MInv(nTngM) * oldM * tngt[i].vl;
					Vector tngR = MInv(nTngM) * oldM * tngt[i].vr;
					tngt[i].vl = tngL;
					tngt[i].vr = tngR;
				}
			}
			op->Message(MSG_UPDATE);
		}
	}
}

LONG GetSharedPolyPointsCount(CPolygon pA, CPolygon pB)
{
	LONG count=0;
	
	if(pB.a == pA.a) count++;
	if(pB.b == pA.a) count++;
	if(pB.c == pA.a) count++;
	if(pB.d == pA.a) count++;
	
	if(pB.a == pA.b) count++;
	if(pB.b == pA.b) count++;
	if(pB.c == pA.b) count++;
	if(pB.d == pA.b) count++;
	
	if(pB.a == pA.c) count++;
	if(pB.b == pA.c) count++;
	if(pB.c == pA.c) count++;
	if(pB.d == pA.c) count++;
	
	if(pB.a == pA.d) count++;
	if(pB.b == pA.d) count++;
	if(pB.c == pA.d) count++;
	if(pB.d == pA.d) count++;
	
	return count;
}

Bool IsValidPolygonObject(BaseObject *op)
{
	if(!op) return false;
	
#if API_VERSION < 9800
	if(op->IsInstanceOf(Opolygon) && ToPoly(op)->GetPolygon()) return true;
	else return false;
#else
	if(op->IsInstanceOf(Opolygon) && ToPoly(op)->GetPolygonR()) return true;
	else return false;
#endif
}

CPolygon* GetPolygonArray(BaseObject *op)
{
#if API_VERSION < 9800
	if(!IsValidPolygonObject(op)) return NULL;
	else return ToPoly(op)->GetPolygon();
#else
	if(!IsValidPolygonObject(op)) return NULL;
	else return ToPoly(op)->GetPolygonW();
#endif
}

Real GetPolygonArea(Vector *padr, const CPolygon &v)
{
	Vector a, b, c, d;
	if(v.c==v.d)
	{
		a = padr[v.a];
		b = padr[v.b];
		c = padr[v.c];
		
		return (Len(VCross((b-a), (c-a))) * 0.5);
	}
	else
	{
		a = padr[v.a];
		b = padr[v.b];
		c = padr[v.c];
		d = padr[v.d];
		
		return ((Len(VCross((b-a), (d-a))) * 0.5) + (Len(VCross((b-c), (d-c))) * 0.5));
	}
	
	return 0.0;
}

void GetEdgePointIndices(CPolygon *vadr, LONG edge, LONG &a, LONG &b)
{
	LONG side = edge-((edge/4)*4);
	switch (side)
	{
		case 0:
			a = vadr[edge/4].a;
			b = vadr[edge/4].b;
			break;
		case 1:
			a = vadr[edge/4].b;
			b = vadr[edge/4].c;
			break;
		case 2:
			a = vadr[edge/4].c;
			b = vadr[edge/4].d;
			break;
		case 3:
			a = vadr[edge/4].d;
			b = vadr[edge/4].a;
			break;
	}
}

void GetEdgePoints(Vector *padr, CPolygon *vadr, LONG ind, Vector &a, Vector &b)
{
	LONG aPt=0, bPt=0, side;
	side = ind-((ind/4)*4);
	switch (side)
	{
		case 0:
			aPt = vadr[ind/4].a;
			bPt = vadr[ind/4].b;
			break;
		case 1:
			aPt = vadr[ind/4].b;
			bPt = vadr[ind/4].c;
			break;
		case 2:
			aPt = vadr[ind/4].c;
			bPt = vadr[ind/4].d;
			break;
		case 3:
			aPt = vadr[ind/4].d;
			bPt = vadr[ind/4].a;
			break;
	}
	
	a = padr[aPt];
	b = padr[bPt];
}

Vector CalcEdgeCenter(Vector *padr, CPolygon *vadr, LONG ind)
{
	Vector a, b;
	GetEdgePoints(padr, vadr, ind, a, b);
	return a + (b - a) * 0.5;
}

Vector CalcPolygonCenter(Vector *padr, const CPolygon &v)
{
	if(v.c==v.d)
		return (padr[v.a] + padr[v.b] + padr[v.c]) / 3;
	else
		return (padr[v.a] + padr[v.b] + padr[v.c] + padr[v.d]) / 4;
}

Vector CalcUVWCenter(UVWStruct u)
{
	if(VEqual(u.c,u.d,0.001))
		return (u.a + u.b  + u.c) / 3;
	else
		return (u.a + u.b  + u.c + u.c) / 4;
}

Vector GetBarycentricCoords(Vector a, Vector b, Vector c, Vector p)
{
    Vector bryC = Vector();
    Vector v0 = b - a, v1 = c - a, v2 = p - a;
    
    Real d00 = VDot(v0, v0);
    Real d01 = VDot(v0, v1);
    Real d11 = VDot(v1, v1);
    Real d20 = VDot(v2, v0);
    Real d21 = VDot(v2, v1);
    Real denom = d00 * d11 - d01 * d01;
    bryC.y = (d11 * d20 - d01 * d21) / denom;
    bryC.z = (d00 * d21 - d01 * d20) / denom;
    bryC.x = 1.0 - bryC.y - bryC.z;
    
    return bryC;
} 

Vector ClosestPointOnTriangle(Vector p, Vector a, Vector b, Vector c)
{
    // Check if P in vertex region outside A
    Vector ab = b - a;
    Vector ac = c - a;
    Vector ap = p - a;
    
    Real d1 = VDot(ab, ap);
    Real d2 = VDot(ac, ap);
    if (d1 <= 0.0 && d2 <= 0.0) return a; // barycentric coordinates (1,0,0)

    // Check if P in vertex region outside B
    Vector bp = p - b;
    Real d3 = VDot(ab, bp);
    Real d4 = VDot(ac, bp);
    if (d3 >= 0.0 && d4 <= d3) return b; // barycentric coordinates (0,1,0)

    // Check if P in edge region of AB, if so return projection of P onto AB
    Real vc = d1*d4 - d3*d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
    {
        Real v = d1 / (d1 - d3);
        return a + v * ab; // barycentric coordinates (1-v,v,0)
    }

    // Check if P in vertex region outside C
    Vector cp = p - c;
    Real d5 = VDot(ab, cp);
    Real d6 = VDot(ac, cp);
    if (d6 >= 0.0 && d5 <= d6) return c; // barycentric coordinates (0,0,1)

    // Check if P in edge region of AC, if so return projection of P onto AC
    Real vb = d5*d2 - d1*d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
    {
        Real w = d2 / (d2 - d6);
        return a + w * ac; // barycentric coordinates (1-w,0,w)
    }

    // Check if P in edge region of BC, if so return projection of P onto BC
    Real va = d3*d6 - d5*d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
    {
        Real w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + w * (c - b); // barycentric coordinates (0,1-w,w)
    }

    // P inside face region. Compute Q through its barycentric coordinates (u,v,w)
    Real denom = 1.0 / (va + vb + vc);
    Real v = vb * denom;
    Real w = vc * denom;
    
    return a + ab * v + ac * w; // = u*a + v*b + w*c, u = va * denom = 1.0 - v - w
}

Vector ClosestPointOnPolygon(Vector *padr, CPolygon ply, Vector pt)
{
	Vector a,b,c,d,ptA,ptB;
	
	a = padr[ply.a];
	b = padr[ply.b];
	c = padr[ply.c];
	d = padr[ply.d];
	
	ptA = ClosestPointOnTriangle(pt, a, b, c);
	if(ply.c != ply.d)
	{
		ptB = ClosestPointOnTriangle(pt, c, d, a);
		
		Real aLen = Len(ptA - pt), bLen = Len(ptB - pt);
		if(aLen < bLen) return ptA;
		else return ptB;
	}
	else return ptA;
}

void StoreSelections(BaseObject *op, BaseSelect *ptS, BaseSelect *edgS, BaseSelect *plyS)
{
	BaseSelect *bs = ToPoint(op)->GetPointS();
	bs->CopyTo(ptS);
	
	if(IsValidPolygonObject(op))
	{
		bs = ToPoly(op)->GetEdgeS();
		bs->CopyTo(edgS);
		
		bs = ToPoly(op)->GetPolygonS();
		bs->CopyTo(plyS);
	}
}

void RestoreSelections(BaseObject *op, BaseSelect *ptS, BaseSelect *edgS, BaseSelect *plyS)
{
	BaseSelect *bs = ToPoint(op)->GetPointS();
	bs->DeselectAll();
	ptS->CopyTo(bs);
	
	if(IsValidPolygonObject(op))
	{
		bs = ToPoly(op)->GetEdgeS();
		bs->DeselectAll();
		edgS->CopyTo(bs);
		
		bs = ToPoly(op)->GetPolygonS();
		bs->DeselectAll();
		plyS->CopyTo(bs);
	}
}

Vector GetSelectionCenter(Vector *padr, CPolygon *vadr, BaseSelect *bs, LONG bsCnt)
{
	CDAABB bnds;
	bnds.Empty();
	
	LONG i, a, b, seg=0;
	while(CDGetRange(bs, seg++, &a, &b))
	{
		for(i=a; i<=b; ++i)
		{
			CPolygon ply = vadr[i];
			Vector pt = CalcPolygonCenter(padr, ply);
			bnds.AddPoint(pt);
		}
	}
	
	return bnds.GetCenterPoint();
}



// object functions
Bool ObjectHasSize(BaseObject *op)
{
	if(!op) return false;
	Vector rad = op->GetRad();
	
	if(rad.x > 0.0) return true;
	if(rad.y > 0.0) return true;
	if(rad.z > 0.0) return true;
	
	return false;
}

Bool IsParentObject(BaseObject *ch, BaseObject *pr)
{
	if(!ch || !pr) return false;
	BaseObject *op = ch->GetUp();
	
	while(op)
	{
		if(op == pr) return true;
		op = op->GetUp();
	}
	
	return false;
}

Bool IsParentSelected(BaseObject *op)
{
	while(op)
	{
		if(op->GetBit(BIT_ACTIVE)) return true;
		op = op->GetUp();
	}
	
	return false;
}

BaseObject* GetHNParentObject(BaseObject *op)
{
	while(op)
	{
		if(op->IsInstanceOf(Osds)) return op;
		op = op->GetUp();
	}
	return NULL;
}

BaseObject* GetPolygonalObject(BaseObject *op)
{
	BaseObject *res = NULL;
	
	if(op)
	{
		if(IsValidPolygonObject(op)) res = op;
		else
		{
			while(op)
			{
				op = op->GetCache();
				if(IsValidPolygonObject(op))
				{
					res = op;
					break;
				}
			}
		}
	}
	
	return res;
}

BaseObject* GetObjectDeformer(BaseObject *op)
{
	BaseObject *ch = op->GetDown();
	while(ch)
	{
		if(ch->GetInfo() & OBJECT_MODIFIER) return ch;
		ch = ch->GetNext();
	}
	
	BaseObject *pr = op->GetUp();
	while(pr)
	{
		if(pr->GetInfo() & OBJECT_MODIFIER) return pr;
		
		ch = pr->GetDown();
		while(ch)
		{
			if(ch->GetInfo() & OBJECT_MODIFIER) return ch;
			ch = ch->GetNext();
		}
		pr = pr->GetUp();
	}
	
	return NULL;
}

Bool IsObjectInDocument(BaseDocument *doc, BaseObject *docOp, BaseObject *op)
{
	while(docOp)
	{
		if(docOp == op) return true;
		if(IsObjectInDocument(doc, docOp->GetDown(), op)) return true;
		docOp = docOp->GetNext();
	}
	return false;
}

Bool IsHierarchyEqual(BaseObject *src, BaseObject *dst)
{
	if(src && !dst) return false;
	if(dst && !src) return false;
	
	while(src && dst)
	{
		if(!IsHierarchyEqual(src->GetDown(),dst->GetDown())) return false;
		src = src->GetNext();
		dst = dst->GetNext();
		if(src && !dst) return false;
		if(dst && !src) return false;
	}
	
	return true;
}

Bool IsUserDataEqual(BaseObject *src, BaseObject *dst, Bool hrchl)
{
	DynamicDescription *srcDD = NULL, *dstDD = NULL;
	
	if(!hrchl)
	{
		if(src && dst)
		{
			srcDD = src->GetDynamicDescription();
			dstDD = dst->GetDynamicDescription();
			if(!srcDD || !dstDD)return false;
			
			DescID srcID, dstID;
			const BaseContainer *srcBC, *dstBC;
			
			void *srcHnd = srcDD->BrowseInit();
			void *dstHnd = dstDD->BrowseInit();
			while(srcDD->BrowseGetNext(srcHnd, &srcID, &srcBC) && dstDD->BrowseGetNext(dstHnd, &dstID, &dstBC))
			{
				GeData srcData, dstData;

				CDGetParameter(src, srcID, srcData);
				CDGetParameter(dst, dstID, dstData);
				
				if(srcData.GetType() != dstData.GetType())
				{
					srcDD->BrowseFree(srcHnd);
					dstDD->BrowseFree(dstHnd);
					return false;
				}
			}
			srcDD->BrowseFree(srcHnd);
			dstDD->BrowseFree(dstHnd);
		}
	}
	else
	{
		while(src && dst)
		{
			srcDD = src->GetDynamicDescription();
			dstDD = dst->GetDynamicDescription();
			if(!srcDD || !dstDD)return false;
			
			DescID srcID, dstID;
			const BaseContainer *srcBC, *dstBC;
			
			void *srcHnd = srcDD->BrowseInit();
			void *dstHnd = dstDD->BrowseInit();
			while(srcDD->BrowseGetNext(srcHnd, &srcID, &srcBC) && dstDD->BrowseGetNext(dstHnd, &dstID, &dstBC))
			{
				GeData srcData, dstData;

				CDGetParameter(src, srcID, srcData);
				CDGetParameter(dst, dstID, dstData);
				
				if(srcData.GetType() != dstData.GetType())
				{
					srcDD->BrowseFree(srcHnd);
					dstDD->BrowseFree(dstHnd);
					return false;
				}
			}
			srcDD->BrowseFree(srcHnd);
			dstDD->BrowseFree(dstHnd);
			
			if(!IsUserDataEqual(src->GetDown(), dst->GetDown(), hrchl)) return false;
			src = src->GetNext();
			dst = dst->GetNext();
		}
	}
	
	return true;
}

LONG GetChildCount(BaseObject *op)
{
	LONG cnt = 0;
	
	if(op)
	{
		BaseObject *ch = op->GetDown();
		while(ch)
		{
			cnt++;
			ch = ch->GetNext();
		}
	}
	
	return cnt;
}

Vector GetGlobalPosition(BaseObject *op)
{
	Vector p;
	
	Matrix m = Matrix();
	if(op) m = op->GetMg();
	
	p = m.off;
	
	return p;
}

void SetGlobalPosition(BaseObject *op, Vector pos)
{
	if(op)
	{
		Matrix opM = op->GetMg();
		opM.off = pos;
		op->SetMg(opM);
	}
}

Vector GetGlobalScale(BaseObject *op)
{
	Vector s;
	
	Matrix m = Matrix();
	if(op) m = op->GetMg();
	
	s.x = Len(m.v1);
	s.y = Len(m.v2);
	s.z = Len(m.v3);
	
	return s;
}

void SetGlobalScale(BaseObject *op, Vector sca)
{
	if(op)
	{
		Matrix opM = op->GetMg();
		opM.v1 = VNorm(opM.v1) * sca.x;
		opM.v2 = VNorm(opM.v2) * sca.y;
		opM.v3 = VNorm(opM.v3) * sca.z;
		op->SetMg(opM);
	}
}

void ScaleAttributes(BaseList2D *lst, Vector sca)
{
	BaseContainer *lstData = lst->GetDataInstance();
	Real rSca = sca.x;
	if(rSca < sca.y) rSca = sca.y;
	if(rSca < sca.z) rSca = sca.z;
	
	AutoAlloc<Description> desc;

	CDGetDescription(lst, desc, CD_DESCFLAGS_DESC_0);

	void* h = desc->BrowseInit();
	const BaseContainer *bc = NULL;
	DescID id, groupid, scope;
	LONG i;
	while (desc->GetNext(h, &bc, id, groupid))
	{
		if(bc->GetLong(DESC_UNIT) == DESC_UNIT_METER && !bc->GetBool(DESC_HIDE))
		{
			for (i=0; i<id.GetDepth(); ++i)
			{
				if(id[i].id > 999)
				{
					if(id[i].dtype == DTYPE_VECTOR)
					{
						Vector v = lstData->GetVector(id[i].id);
						v *= rSca;
						lstData->SetVector(id[i].id,v);
					}
					if(id[i].dtype == DTYPE_REAL)
					{
						Real r = lstData->GetReal(id[i].id);
						r *= rSca;
						lstData->SetReal(id[i].id,r);
					}
				}
			}
		}
	}
	desc->BrowseFree(h);
}

Vector GetGlobalRotation(BaseObject *op)
{
	Vector gRot = Vector(0,0,0);
	
	AutoAlloc<AtomArray> objects;
	if(objects)
	{
		while(op)
		{
			objects->Append(op);
			op = op->GetUp();
		}
		
		LONG i, cnt = objects->GetCount();
		for(i=cnt; i>0; i--)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i-1));
			if(op)
			{
				gRot = CDGetOptimalAngle(gRot + CDGetRot(op), CDMatrixToHPB(op->GetMg()), op);
			}
		}
	}
	
	return gRot;
}

void SetGlobalRotation(BaseObject *op, Vector rot)
{
	AutoAlloc<AtomArray> objects;
	Vector oldRot = CDGetRot(op);
	
	if(objects)
	{
		while(op)
		{
			objects->Append(op);
			op = op->GetUp();
		}
		
		LONG i, cnt = objects->GetCount();
		for(i=cnt; i>1; i--)
		{
			op = static_cast<BaseObject*>(objects->GetIndex(i-1));
			if(op)
			{
				rot = CDGetOptimalAngle(rot-CDGetRot(op),CDMatrixToHPB(op->GetMg(), op), op);
			}
		}
		op = static_cast<BaseObject*>(objects->GetIndex(0));
		if(op)
		{
			rot = CDGetOptimalAngle(rot, CDGetRot(op), op);
		}
	}
	
	CDSetRot(op,rot);
	Vector newRot = CDGetRot(op);
	CDSetRot(op,CDGetOptimalAngle(oldRot, newRot, op));
}

Vector GetRotationalDifference(BaseObject *opA, BaseObject *opB, Bool local)
{
	Matrix localM;
	Vector difRot;
	
	if(local)
	{
		difRot = CDGetRot(opB) - CDGetRot(opA);
		localM = MInv(opA->GetMl()) * opB->GetMl();
	}
	else
	{
		difRot = GetGlobalRotation(opB) - GetGlobalRotation(opA);
		localM = MInv(opA->GetMg()) * opB->GetMg();
	}
	
	return CDGetOptimalAngle(difRot, CDMatrixToHPB(localM), opB);
}

Bool ObjectsEqual(BaseObject *op, BaseObject *cln, Real t)
{
	if(!op || !cln) return false;
	if(!IsValidPointObject(op) || !IsValidPointObject(cln)) return false;
	
	LONG opCnt, clnCnt;

	opCnt = ToPoint(op)->GetPointCount();
	clnCnt = ToPoint(cln)->GetPointCount();
	if(opCnt != clnCnt) return false;
	
	Vector *padrOp = GetPointArray(op);
	Vector *padrCln = GetPointArray(cln);
	if(!padrOp || !padrCln) return false;
	
	LONG i;
	for(i=0; i<opCnt; i++)
	{
		Vector opPt = padrOp[i];
		Vector clnPt = padrCln[i];
		if(!VEqual(opPt,clnPt,t)) return false;
	}
	
	if(IsValidPolygonObject(op) && IsValidPolygonObject(cln))
	{
		opCnt = ToPoly(op)->GetPolygonCount();
		clnCnt = ToPoly(cln)->GetPolygonCount();
		if(opCnt != clnCnt) return false;
	}

	return true;	
}

Bool ActiveModelToolScaling(BaseObject *op)
{
	while(op)
	{
		if(op->GetBit(BIT_ACTIVE)) return true;
		op = op->GetUp();
	}
	
	return false;
}


// transfer functions
void CDTransferGoals(BaseList2D *src, BaseList2D *dst)
{
#if API_VERSION < 10499
	if(src && dst) src->TransferGoal(dst);
#else
	if(src && dst) src->TransferGoal(dst, false);
#endif
}

void CDTransferSelections(BaseObject *src, BaseObject *dst)
{
	if(src && dst && IsValidPointObject(src) && IsValidPointObject(dst))
	{
		// copy point selections
		BaseSelect *sPtS = ToPoint(src)->GetPointS();
		BaseSelect *dPtS = ToPoint(dst)->GetPointS();
		dPtS->DeselectAll();
		sPtS->CopyTo(dPtS);
		
		// copy edge selections
		BaseSelect *sEdgS = ToPoly(src)->GetEdgeS();
		BaseSelect *dEdgS = ToPoly(dst)->GetEdgeS();
		dEdgS->DeselectAll();
		sEdgS->CopyTo(dEdgS);
		
		// copy edge selections
		BaseSelect *sPlyS = ToPoly(src)->GetPolygonS();
		BaseSelect *dPlyS = ToPoly(dst)->GetPolygonS();
		dPlyS->DeselectAll();
		sPlyS->CopyTo(dPlyS);
	}
}

#define ID_CDMORPHREFTAG			1017273
#define ID_CDSKINREFTAG				1019291
#define ID_CDSKINTAG				1019251

static Bool ObjectSpecificTag(BaseTag *tag)
{
	//if(tag->GetType() == Tvariable) return true;
	
	if(tag->GetType() == Tpoint) return true;
	if(tag->GetType() == Tphong) return true;
	if(tag->GetType() == Tparticle) return true;
	if(tag->GetType() == Tpolygon) return true;
	if(tag->GetType() == Tuvw) return true;
	if(tag->GetType() == Tsegment) return true;
	if(tag->GetType() == Tpolygonselection) return true;
	if(tag->GetType() == Tpointselection) return true;
	if(tag->GetType() == Tline) return true;
	if(tag->GetType() == Tvertexmap) return true;
	if(tag->GetType() == Tmetaball) return true;
	if(tag->GetType() == Tsticktexture) return true;
	if(tag->GetType() == Tedgeselection) return true;
	if(tag->GetType() == Tnormal) return true;
	if(tag->GetType() == Tcorner) return true;
	if(tag->GetType() == Tsds) return true;
	if(tag->GetType() == Tsoftselection) return true;
	
	
#if API_VERSION > 9999
	if(tag->GetType() == Tweights) return true;
	if(tag->GetType() == Tsdsdata) return true;
	if(tag->GetType() == Tmorph) return true;
#endif
	
#if API_VERSION > 11999
	if(tag->GetType() == Tposemorph) return true;
#endif
	
#if API_VERSION < 12000
	if(tag->GetType() == Thermite2d) return true;
	if(tag->GetType() == Tclaudebonet) return true;
#endif
	
	// CD plugin tags
	if(tag->GetType() == ID_CDMORPHREFTAG) return true;
	if(tag->GetType() == ID_CDSKINREFTAG) return true;
	if(tag->GetType() == ID_CDSKINTAG) return true;
	
	return false;
}

void TransferAMTracks(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, BaseList2D *prvTrk)
{
	AutoAlloc<Description> desc;
	
	CDGetDescription(src, desc, CD_DESCFLAGS_DESC_0);
	void* h = desc->BrowseInit();
	const BaseContainer *bc = NULL;
	DescID id, groupid, srcID;
	
	Bool skip = false;
	while(desc->GetNext(h, &bc, id, groupid))
	{
		if(!skip && id == DescID(ID_BASEOBJECT_GROUP1)) skip = true;
		if(skip && id == DescID(ID_OBJECTPROPERTIES)) skip = false;
		
		if(!skip)
		{
			if(id[0].dtype == DTYPE_VECTOR)
			{
				// transfer X vector component
				srcID = DescID(DescLevel(id[0].id, id[0].dtype,0), DescLevel(VECTOR_X,DTYPE_REAL, 0));
				if(CDHasAnimationTrack(src, srcID)) CDCopyAnimationTrack(doc, src, dst, srcID, prvTrk);
				
				// transfer Y vector component
				srcID = DescID(DescLevel(id[0].id, id[0].dtype,0), DescLevel(VECTOR_Y,DTYPE_REAL, 0));
				if(CDHasAnimationTrack(src, srcID)) CDCopyAnimationTrack(doc, src, dst, srcID, prvTrk);
				
				// transfer Z vector component
				srcID = DescID(DescLevel(id[0].id, id[0].dtype,0), DescLevel(VECTOR_Z,DTYPE_REAL, 0));
				if(CDHasAnimationTrack(src, srcID)) CDCopyAnimationTrack(doc, src, dst, srcID, prvTrk);
			}
			else
			{
				if(CDHasAnimationTrack(src, id)) CDCopyAnimationTrack(doc, src, dst, id, prvTrk);
			}
		}
	}
	desc->BrowseFree(h);
}

void TransferUDTracks(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, BaseList2D *prvTrk)
{
	DynamicDescription *srcDD = src->GetDynamicDescription();
	DynamicDescription *dstDD = dst->GetDynamicDescription();
	
	if(srcDD && dstDD)
	{
		CDAddUndo(doc,CD_UNDO_CHANGE,src);
		CDAddUndo(doc,CD_UNDO_CHANGE,dst);
		
		DescID srcID, dstID;
		const BaseContainer *srcBC, *dstBC;
		
		void *srcHnd = srcDD->BrowseInit();
		void *dstHnd = dstDD->BrowseInit();
		while(srcDD->BrowseGetNext(srcHnd, &srcID, &srcBC) && dstDD->BrowseGetNext(dstHnd, &dstID, &dstBC))
		{
			GeData srcData, dstData;
			CDGetParameter(src, srcID, srcData);
			CDGetParameter(dst, dstID, dstData);
			
			if(srcData.GetType() == dstData.GetType())
			{
				if(srcData.GetType() == DA_VECTOR)
				{
					DescLevel dscLevel = srcID.operator[](1);
					
					DescID xdscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0), DescLevel(dscLevel.id, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
					if(CDHasAnimationTrack(src, xdscID)) CDCopyAnimationTrack(doc, src, dst, xdscID, prvTrk);
					
					DescID ydscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0), DescLevel(dscLevel.id, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
					if(CDHasAnimationTrack(src, ydscID)) CDCopyAnimationTrack(doc, src, dst, ydscID, prvTrk);
					
					DescID zdscID = DescID(DescLevel(ID_USERDATA, DTYPE_SUBCONTAINER, 0), DescLevel(dscLevel.id, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
					if(CDHasAnimationTrack(src, zdscID)) CDCopyAnimationTrack(doc, src, dst, zdscID, prvTrk);
				}
				else if(CDHasAnimationTrack(src, srcID)) CDCopyAnimationTrack(doc, src, dst, srcID, prvTrk);
			}
		}
		srcDD->BrowseFree(srcHnd);
		dstDD->BrowseFree(dstHnd);
	}
}

void TransferVectorTracks(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, BaseList2D *prvTrk, LONG trackID)
{
	DescID dscID;
	
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	CDCopyTrackAttributes(doc, dst, src, dscID);
	CDCopyAnimationTrack(doc, src, dst, dscID, prvTrk);
	
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	CDCopyTrackAttributes(doc, dst, src, dscID);
	CDCopyAnimationTrack(doc, src, dst, dscID, prvTrk);
	
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	CDCopyTrackAttributes(doc, dst, src, dscID);
	CDCopyAnimationTrack(doc, src, dst, dscID, prvTrk);
}

Bool TransferTags(BaseDocument *doc, BaseObject *src, BaseObject *dst)
{
	BaseTag *tag = NULL, *nxt = NULL, *pred = NULL;
	
	tag = src->GetFirstTag();
	while(tag)
	{
		nxt = tag->GetNext();
		if(tag->GetInfo() & TAG_VISIBLE)
		{
			if(!ObjectSpecificTag(tag))
			{
				CDAddUndo(doc,CD_UNDO_CHANGE, tag);
				tag->Remove();
				dst->InsertTag(tag, pred);
				pred = tag;
			}
		}
		tag = nxt;
	}
	
	return true;
}

Bool TransferUserData(BaseDocument *doc, BaseList2D *src, BaseList2D *dst)
{
	DynamicDescription *srcDD = src->GetDynamicDescription();
	if(!srcDD) return false;
	
	DynamicDescription *dstDD = dst->GetDynamicDescription();
	if(!dstDD) return false;
	
	CDAddUndo(doc,CD_UNDO_CHANGE, dst);
	srcDD->CopyTo(dstDD);
	
	return true;
}

Bool TransferChildren(BaseDocument *doc, BaseObject *src, BaseObject *dst)
{
	BaseObject *pred = NULL, *ch = NULL, *nxt = NULL;
	
	ch = src->GetDown();
	while(ch)
	{
		Matrix chM = ch->GetMg();
		nxt = ch->GetNext();
		
		CDAddUndo(doc, CD_UNDO_CHANGE, ch);
		ch->Remove();
		doc->InsertObject(ch, dst, pred, false);
		ch->SetMg(chM);
		
		pred = ch;
		ch = nxt;
	}
	
	return true;
}

void RepositionChildren(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM, Bool undo)
{
	Matrix chM;
	
	while(op)
	{
		chM = oldM * MInv(newM) * op->GetMg();
		if(undo)
		{
			CDAddUndo(doc, CD_UNDO_CHANGE, op);
		}
		op->SetMg(chM);
		
		op = op->GetNext();
	}
}



// matrix/vector functions
Vector VectorSlerp(Vector v1, Vector v2, Real t)
{
	Real w = ACos(VDot(v1, v2));
	return (Sin((1-t)*w)/Sin(w))*v1 + (Sin(t*w)/Sin(w))*v2;
}

Matrix MixM(Matrix a, Matrix b, Vector axis, Real theMix)
{
	Matrix mixMatrix;
	Vector offAxis, xAxis, yAxis, zAxis;
	
	offAxis = CDBlend(a.off, b.off, theMix);
	zAxis = CDBlend(a.v3, b.v3, theMix);
	if(axis == Vector(0, 1, 0))
	{
		xAxis = CDBlend(a.v1, b.v1, theMix);
		yAxis = VNorm(VCross(zAxis, xAxis));
		xAxis = VNorm(VCross(yAxis, zAxis));
	}
	else
	{
		yAxis = CDBlend(a.v2, b.v2, theMix);
		xAxis = VNorm(VCross(yAxis, zAxis));
		yAxis = VNorm(VCross(zAxis, xAxis));
	}
	mixMatrix = Matrix(offAxis, xAxis, yAxis, zAxis);
	
	return mixMatrix;
}

Bool MatrixEqual(Matrix a, Matrix b, Real e)
{
	if(!VEqual(a.off, b.off, e)) return false;
	if(!VEqual(a.v1, b.v1, e)) return false;
	if(!VEqual(a.v2, b.v2, e)) return false;
	if(!VEqual(a.v3, b.v3, e)) return false;
	
	return true;
}

Matrix GetAffineMatrix(Matrix m)
{
	Matrix aM;

	aM.off = m.off;
	aM.v3 = VNorm(m.v3);
	
	Real ydot = VDot(aM.v3, VNorm(m.v2));
	Real xdot = VDot(aM.v3, VNorm(m.v1));
	
	if(Abs(ydot) < Abs(xdot))
	{
		aM.v1 = VNorm(VCross(m.v2, aM.v3));
		aM.v2 = VNorm(VCross(aM.v3, aM.v1));
	}
	else
	{
		aM.v2 = VNorm(VCross(aM.v3, m.v1));
		aM.v1 = VNorm(VCross(aM.v2, aM.v3));
	}
	aM.v3 = VNorm(VCross(aM.v1, aM.v2));
	
	return aM;
}

Matrix GetNormalizedMatrix(Matrix m)
{
	Matrix nM;
	
	nM.off = m.off;
	nM.v1 = VNorm(m.v1);
	nM.v2 = VNorm(m.v2);
	nM.v3 = VNorm(m.v3);
	
	return nM;
}

Vector GetMatrixScale(Matrix m)
{
	Vector s;
	
	s.x = Len(m.v1);
	s.y = Len(m.v2);
	s.z = Len(m.v3);
	
	return s;
}

Matrix ScaleMatrix(Matrix m, Vector sca)
{
	Matrix sM;
	
	sM.off = m.off;
	sM.v1 = VNorm(m.v1) * sca.x;
	sM.v2 = VNorm(m.v2) * sca.y;
	sM.v3 = VNorm(m.v3) * sca.z;
	
	return sM;
}

Matrix GetMirrorMatrix(Matrix opM, Matrix prM, LONG axis)
{
	Matrix m, retM;
	
	Vector scale = GetMatrixScale(opM);
	
	switch (axis)
	{
		case 0:
			m = MInv(prM) * opM;
			m.off.x *= -1;
			m.v3.x *= -1;
			if(Abs(VDot(VNorm(m.v1), Vector(1, 0, 0))) < Abs(VDot(VNorm(m.v2), Vector(1, 0, 0))))
			{
				m.v1.x *= -1;
				m.v2 = VNorm(VCross(m.v3, m.v1));
				m.v1 = VNorm(VCross(m.v2, m.v3));
			}
				else if(Abs(VDot(VNorm(m.v1), Vector(1, 0, 0))) > Abs(VDot(VNorm(m.v2), Vector(1, 0, 0))))
				{
					m.v2.x *= -1;
					m.v1 = VNorm(VCross(m.v2, m.v3));
					m.v2 = VNorm(VCross(m.v3, m.v1));
				}
				else
				{
					m.v1 = VNorm(VCross(m.v2, m.v3));
					m.v2 = VNorm(VCross(m.v3, m.v1));
				}
				retM = prM * m;
			break;
		case 1:
			m = MInv(prM) * opM;
			m.off.y *= -1;
			m.v3.y *= -1;
			if(Abs(VDot(VNorm(m.v1), Vector(0, 1, 0))) < Abs(VDot(VNorm(m.v2), Vector(0, 1, 0))))
			{
				m.v1.y *= -1;
				m.v2 = VNorm(VCross(m.v3, m.v1));
				m.v1 = VNorm(VCross(m.v2, m.v3));
			}
				else if(Abs(VDot(VNorm(m.v1), Vector(0, 1, 0))) > Abs(VDot(VNorm(m.v2), Vector(0, 1, 0))))
				{
					m.v2.y *= -1;
					m.v1 = VNorm(VCross(m.v2, m.v3));
					m.v2 = VNorm(VCross(m.v3, m.v1));
				}
				else
				{
					m.v1 = VNorm(VCross(m.v2, m.v3));
					m.v2 = VNorm(VCross(m.v3, m.v1));
				}
				retM = prM * m;
			break;
		case 2:
			m = MInv(prM) * opM;
			m.off.z *= -1;
			m.v3.z *= -1;
			if(Abs(VDot(VNorm(m.v1), Vector(0, 0, 1))) < Abs(VDot(VNorm(m.v2), Vector(0, 0, 1))))
			{
				m.v1.z *= -1;
				m.v2 = VNorm(VCross(m.v3, m.v1));
				m.v1 = VNorm(VCross(m.v2, m.v3));
			}
				else if(Abs(VDot(VNorm(m.v1), Vector(0, 0, 1))) > Abs(VDot(VNorm(m.v2), Vector(0, 0, 1))))
				{
					m.v2.z *= -1;
					m.v1 = VNorm(VCross(m.v2, m.v3));
					m.v2 = VNorm(VCross(m.v3, m.v1));
				}
				else
				{
					m.v1 = VNorm(VCross(m.v2, m.v3));
					m.v2 = VNorm(VCross(m.v3, m.v1));
				}
				retM = prM * m;
			break;
	}
	
	//return retM;
	return ScaleMatrix(m, scale);
}

Real LenSquare(Vector v1, Vector v2)
{
	return (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y) + (v1.z-v2.z)*(v1.z-v2.z);
}


// animation functions
Bool CDHasVectorTrack(BaseList2D *op, LONG trackID)
{
	DescID dscID;
	
	// track X
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID)) return true;
	
	// track Y
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID)) return true;
	
	// track Z
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID)) return true;
	
	return false;
}

LONG CDGetFirstVectorKeyFrame(BaseList2D *op, LONG fps, LONG trackID)
{
	LONG frm = 0;
	DescID dscID;
	
	// track X
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
		frm = CDGetKeyFrame(op, dscID, 0, fps);
	
	// track Y
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
		if(CDGetKeyFrame(op, dscID, 0, fps) < frm)
			frm = CDGetKeyFrame(op, dscID, 0, fps);
	
	// track Z
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
		if(CDGetKeyFrame(op, dscID, 0, fps) < frm)
			frm = CDGetKeyFrame(op, dscID, 0, fps);
	
	return frm;
}

LONG CDGetPreviousVectorKeyFrame(BaseObject *op, LONG curFrm, LONG fps, LONG trackID)
{
	LONG frm = 0, keyInd;
	DescID dscID;
	
	// track X
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0)); 
	if(CDHasAnimationTrack(op, dscID))
	{
		keyInd = CDGetNearestKey(op, dscID, curFrm, fps);
		frm = CDGetKeyFrame(op, dscID, keyInd, fps);
	}
	
	// track Y
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
	{
		keyInd = CDGetNearestKey(op, dscID, curFrm, fps);
		if(CDGetKeyFrame(op, dscID, keyInd, fps) > frm) frm = CDGetKeyFrame(op, dscID, keyInd, fps);
	}
	
	// track Z
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
	{
		keyInd = CDGetNearestKey(op,dscID,curFrm,fps);
		if(CDGetKeyFrame(op, dscID, keyInd, fps) > frm) frm = CDGetKeyFrame(op, dscID, keyInd, fps);
	}
	
	return frm;	
}

LONG CDGetNextVectorKeyFrame(BaseObject *op, LONG curFrm, LONG fps, LONG trackID)
{
	LONG frm = 0;
	DescID dscID;
	
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
		frm = CDGetNextKeyFrame(op, dscID, curFrm, fps);

	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
		if(CDGetNextKeyFrame(op, dscID, curFrm, fps) < frm)
			frm = CDGetNextKeyFrame(op, dscID, curFrm, fps);
	
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID))
		if(CDGetNextKeyFrame(op, dscID, curFrm, fps) < frm)
			frm = CDGetNextKeyFrame(op, dscID, curFrm, fps);
	
	return frm;	
}

LONG CDGetLastVectorKeyFrame(BaseList2D *op, LONG fps, LONG trackID)
{
	LONG frm, kCnt;
	DescID dscID;
	
	// position track X
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	kCnt = CDGetTrackKeyCount(op, dscID);
	frm = CDGetKeyFrame(op, dscID, kCnt-1, fps);
	
	// position track Y
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	kCnt = CDGetTrackKeyCount(op, dscID);
	if(CDGetKeyFrame(op, dscID, kCnt-1, fps) > frm) frm = CDGetKeyFrame(op, dscID, kCnt-1, fps);
	
	// position track Z
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	kCnt = CDGetTrackKeyCount(op,dscID);
	if(CDGetKeyFrame(op, dscID, kCnt-1, fps) > frm) frm = CDGetKeyFrame(op, dscID, kCnt-1, fps);
	
	return frm;
}

void CDCopyVectorTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, LONG trackID, BaseList2D *prvTrk)
{
	DescID dscID;

	// track X
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(src, dscID)) CDCopyAnimationTrack(doc, src, dst, dscID, prvTrk);
	
	// track Y
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(src, dscID)) CDCopyAnimationTrack(doc, src, dst, dscID, prvTrk);
	
	// track Z
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(src, dscID)) CDCopyAnimationTrack(doc, src, dst, dscID, prvTrk);
}

void CDScaleVectorTrack(BaseDocument *doc, BaseList2D *op, Vector sca, LONG trackID)
{
	DescID dscID;
	
	// track X
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID)) CDScaleTrack(doc, op, dscID, sca.x);
	
	// track Y
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID)) CDScaleTrack(doc, op, dscID, sca.y);
	
	// track Z
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	if(CDHasAnimationTrack(op, dscID)) CDScaleTrack(doc, op, dscID, sca.z);
}

void CDSetVectorKeys(BaseDocument *doc, BaseObject *op, BaseTime time, LONG trackID, LONG intrp)
{
	DescID dscID;
	GeData gData;
	Real value;
	
	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0));
	CDGetParameter(op, dscID, gData);
	value = gData.GetReal();
	CDSetKey(doc, op, time, dscID, value, intrp);

	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Y, DTYPE_REAL, 0));
	CDGetParameter(op, dscID, gData);
	value = gData.GetReal();
	CDSetKey(doc, op, time, dscID, value, intrp);

	dscID = DescID(DescLevel(trackID, DTYPE_VECTOR, 0), DescLevel(VECTOR_Z, DTYPE_REAL, 0));
	CDGetParameter(op, dscID, gData);
	value = gData.GetReal();
	CDSetKey(doc, op, time, dscID, value, intrp);
}

void CDAnimateDocument(BaseDocument *doc, BaseThread *bt, Bool anim, Bool expr, Bool caches)
{
#if API_VERSION < 12000
	#if API_VERSION	< 11500
		doc->AnimateDocument(bt, anim, expr);
	#else
		doc->ExecutePasses(bt, anim, expr, caches);
	#endif
#else
	doc->ExecutePasses(bt, anim, expr, caches, BUILDFLAGS_0);
#endif
	
}


// general math functions
Real TruncatePercentFractions(Real x)
{
	LONG i = (LONG)(x * 100);
	Real r = (Real)i * 0.01;
	
	return r;
}

Real RoundToWhole(Real value)
{
	Real flr = Floor(value);
	Real clg = Ceil(value);
	
	if(FMod(flr, value) > 0.5) return clg;
	else return flr;
}

// utility functions
Bool ForceDeformCacheRebuild(BaseDocument *doc, BaseObject *op)
{
	Bool chDef = false;
	
	BaseObject *ch = op->GetDown();
	if(ch)
	{
		if(ch->GetInfo() & OBJECT_MODIFIER) chDef = true;
		else
		{
			ch = ch->GetNext();
			while(ch)
			{
				if((ch->GetInfo() & OBJECT_MODIFIER)) chDef = true;
				ch = ch->GetNext();
			}
		}
	}
	
	if(!chDef)
	{
		while(op->GetUp())
		{
			op = op->GetUp();
		}
	}
	
	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = op;
#if API_VERSION < 12000
	mcd.mode = MODIFY_ALL;
#else
	mcd.mode = MODELINGCOMMANDMODE_ALL;
#endif
	
	if(!SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT, mcd)) return false;
	
	BaseObject *tempOp = static_cast<BaseObject*>(mcd.result->GetIndex(0));
	BaseObject::Free(tempOp);
	
	return true;
}




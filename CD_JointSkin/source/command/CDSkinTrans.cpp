#include "stdlib.h"
#include <vector>
using std::vector;

#include "c4d.h"
#include "c4d_symbols.h"

#include "c4d_graphview.h"
#include "lib_modeling.h"
#include "c4d_operatorplugin.h"
#include "c4d_operatordata.h"

#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"
#include "CDGeneral.h"
#include "CDAABB.h"
//#include "CDDebug.h"

#include "CDJointSkin.h"

class CDTransferSkin : public CommandData
{
private:
	vector<LONG>	spcA, spcB, spcC, spcD, spcE, spcF, spcG, spcH;
	
	Bool BuildSpacePartitions(BaseObject *src);
	void FreeSpacePartitions(void);
	
	LONG GetNearestPointIndex(Vector pt, BaseObject *op);
	LONG GetNearestPolygonIndex(Vector pt, Vector *padr, LONG pCnt, BaseObject *op, CPolygon *vadr, LONG vCnt);
	
	void GetTrianglePointIndeces(LONG &a, LONG &b, LONG &c, CPolygon ply, Vector pt, Vector *padr);
	void BuildJointList(BaseDocument *doc, BaseContainer *tData, CDSkinVertex *dWeight, LONG ind, CDSkinVertex *sWeight, LONG a, LONG b, LONG c);
	
	Bool CopySkinData(BaseDocument *doc, BaseObject *src, BaseTag *sTag, BaseObject *dst, BaseTag *dTag);
	
public:
	virtual Bool Execute(BaseDocument* doc);
};

Bool CDTransferSkin::BuildSpacePartitions(BaseObject *src)
{
	Vector *padr = GetPointArray(src); if(!padr) return false;
	LONG i, cnt = ToPoint(src)->GetPointCount();
	Vector mp = src->GetMp();
	
	for(i=0; i<cnt; i++)
	{
		Vector pt = padr[i];
		if(pt.x > mp.x)
		{
			if(pt.y > mp.y)
			{
				if(pt.z > mp.z) spcF.push_back(i);
				else spcE.push_back(i);
			}
			else
			{
				if(pt.z > mp.z) spcH.push_back(i);
				else spcG.push_back(i);
			}
		}
		else
		{
			if(pt.y > mp.y)
			{
				if(pt.z > mp.z) spcB.push_back(i);
				else spcA.push_back(i);
			}
			else
			{
				if(pt.z > mp.z) spcD.push_back(i);
				else spcC.push_back(i);
			}
		}
	}

	return true;
}

void CDTransferSkin::FreeSpacePartitions(void)
{
	spcA.clear();
	spcB.clear();
	spcC.clear();
	spcD.clear();
	spcE.clear();
	spcF.clear();
	spcG.clear();
	spcH.clear();
}

LONG CDTransferSkin::GetNearestPointIndex(Vector pt, BaseObject *op)
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
					for(i=0; i<spcF.size(); i++)
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
					for(i=0; i<spcE.size(); i++)
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
					for(i=0; i<spcH.size(); i++)
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
					for(i=0; i<spcG.size(); i++)
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
					for(i=0; i<spcB.size(); i++)
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
					for(i=0; i<spcA.size(); i++)
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
					for(i=0; i<spcD.size(); i++)
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
					for(i=0; i<spcC.size(); i++)
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

LONG CDTransferSkin::GetNearestPolygonIndex(Vector pt, Vector *padr, LONG pCnt, BaseObject *op, CPolygon *vadr, LONG vCnt)
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

void CDTransferSkin::GetTrianglePointIndeces(LONG &a, LONG &b, LONG &c, CPolygon ply, Vector pt, Vector *padr)
{
	if(ply.c == ply.d)
	{
		a = ply.a;
		b = ply.b;
		c = ply.c;
	}
	else
	{
		Vector pA, pB, pC, pD, ptA, ptB;
		
		pA = padr[ply.a];
		pB = padr[ply.b];
		pC = padr[ply.c];
		pD = padr[ply.d];
		
		ptA = ClosestPointOnTriangle(pt,pA,pB,pC);
		ptB = ClosestPointOnTriangle(pt,pC,pD,pA);

		Real aLen = Len(ptA-pt), bLen = Len(ptB-pt);
		if(aLen < bLen)
		{
			a = ply.a;
			b = ply.b;
			c = ply.c;
		}
		else
		{
			a = ply.c;
			b = ply.d;
			c = ply.a;
		}
	}
}

void CDTransferSkin::BuildJointList(BaseDocument *doc, BaseContainer *tData, CDSkinVertex *dWeight, LONG ind, CDSkinVertex *sWeight, LONG a, LONG b, LONG c)
{
	AutoAlloc<AtomArray> joints;
	if(joints)
	{
		LONG j, indPlus, jCnt;
		BaseObject *jnt = NULL;
		
		// get joints for point a
		jCnt = sWeight[a].jn;
		if(sWeight[a].taw > 0)
		{
			for(j=0; j<jCnt; j++)
			{
				indPlus = sWeight[a].jPlus[j];
				jnt = tData->GetObjectLink(SKN_J_LINK+indPlus,doc);
				if(jnt)
				{
					if(joints->Find(jnt) == NOTOK) joints->Append(jnt);
				}
			}
		}
		
		// get joints for point b
		jCnt = sWeight[b].jn;
		if(sWeight[b].taw > 0)
		{
			for(j=0; j<jCnt; j++)
			{
				indPlus = sWeight[b].jPlus[j];
				jnt = tData->GetObjectLink(SKN_J_LINK+indPlus,doc);
				if(jnt)
				{
					if(joints->Find(jnt) == NOTOK) joints->Append(jnt);
				}
			}
		}
		
		// get joints for point c
		jCnt = sWeight[c].jn;
		if(sWeight[c].taw > 0)
		{
			for(j=0; j<jCnt; j++)
			{
				indPlus = sWeight[c].jPlus[j];
				jnt = tData->GetObjectLink(SKN_J_LINK+indPlus,doc);
				if(jnt)
				{
					if(joints->Find(jnt) == NOTOK) joints->Append(jnt);
				}
			}
		}
		
		dWeight[ind].jn = joints->GetCount();
		jCnt = tData->GetLong(SKN_J_COUNT);
		
		LONG i = 0;
		for(j=0; j<jCnt; j++)
		{
			jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
			if(jnt && joints->Find(jnt) > NOTOK)
			{
				dWeight[ind].jPlus[i] = j;
				i++;
			}
		}
	}
}

Bool CDTransferSkin::CopySkinData(BaseDocument *doc, BaseObject *src, BaseTag *sTag, BaseObject *dst, BaseTag *dTag)
{
	BaseContainer *sData = sTag->GetDataInstance(); if(!sData) return false;
	BaseContainer *dData = dTag->GetDataInstance(); if(!dData) return false;
	
	Vector *sadr = GetPointArray(src); if(!sadr) return false;
	Vector *dadr = GetPointArray(dst); if(!dadr) return false;
	
	CPolygon *vadr = GetPolygonArray(src); if(!vadr) return false;
	LONG vCnt = ToPoly(src)->GetPolygonCount();
	
	LONG sCnt = ToPoint(src)->GetPointCount();
	LONG dCnt = ToPoint(dst)->GetPointCount();
	
	dData->SetLink(SKN_DEST_ID,dst);
	dData->SetLong(SKN_J_LINK_ID,0);
	
	LONG i, jCnt = sData->GetLong(SKN_J_COUNT);
	for(i=0; i<jCnt; i++)
	{
		BaseObject *jnt = sData->GetObjectLink(SKN_J_LINK+i,doc);
		if(jnt)
		{
			dData->SetLink(SKN_J_LINK+i,jnt);
		}
	}
	dData->SetLong(SKN_J_COUNT,jCnt);
	
	// Initialize skin tag's weight array
	dTag->Message(MSG_MENUPREPARE);
	
	CDSkinVertex *dWeight = CDGetSkinWeights(dTag); if(!dWeight) return false;
	CDSkinVertex *sWeight = CDGetSkinWeights(sTag); if(!sWeight) return false;
	
	Matrix srcM = src->GetMg();
	Matrix dstM = dst->GetMg();
	
	for(i=0; i<dCnt; i++)
	{
		LONG prgrs = LONG((Real(i)/Real(dCnt)) * 100.0);
		StatusSetBar(prgrs);
		
		Vector pt = MInv(srcM) * (dstM * dadr[i]);
		LONG plyInd = GetNearestPolygonIndex(pt,sadr,sCnt,src,vadr,vCnt);
		
		LONG a, b, c;
		GetTrianglePointIndeces(a,b,c,vadr[plyInd],pt,sadr);
		
		Vector bCoords = GetBarycentricCoords(sadr[a],sadr[b],sadr[c],pt);
		
		BuildJointList(doc, sData, dWeight, i, sWeight, a, b, c);
		
		LONG j, jCnt = dWeight[i].jn;
		for(j=0; j<jCnt; j++)
		{
			LONG indPlus = dWeight[i].jPlus[j];
			dWeight[i].jw[indPlus] = sWeight[a].jw[indPlus] * bCoords.x + sWeight[b].jw[indPlus] * bCoords.y + sWeight[c].jw[indPlus] * bCoords.z;
		}
	}
	
	// Set accumulated weights
	DescriptionCommand dc;
	dc.id = DescID(SKN_ACCUMULATE);
	dTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	// Normalize the weights
	Real minWt = 0.0;
	dTag->Message(CD_MSG_NORMALIZE_WEIGHT,&minWt);
	
	//Bind skin
	dc.id = DescID(SKN_AUTO_BIND_SKIN);
	dTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	dTag->Message(MSG_UPDATE);

	StatusClear();
	
	return true;
}

Bool CDTransferSkin::Execute(BaseDocument* doc)
{
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
		
	LONG oCnt = opSelLog->GetCount();
	if(oCnt == 2)
	{
		BaseObject *src = static_cast<BaseObject*>(opSelLog->GetIndex(0));
		BaseObject *dst = static_cast<BaseObject*>(opSelLog->GetIndex(1));
		
		if(src && dst)
		{
			
			if(!IsValidPointObject(src) || !IsValidPointObject(dst)) return false;
			
			BaseTag *sTag = NULL, *dTag = NULL, *dRef = NULL;
			
			sTag = src->GetTag(ID_CDSKINPLUGIN); if(!sTag) return false;
			dTag = dst->GetTag(ID_CDSKINPLUGIN);
			if(dTag)
			{	
				CDAddUndo(doc,CD_UNDO_DELETE,dTag);
				BaseTag::Free(dTag);
			}
			
			dRef = dst->GetTag(ID_CDSKINREFPLUGIN);
			if(dRef)
			{
				CDAddUndo(doc,CD_UNDO_DELETE,dRef);
				BaseTag::Free(dRef);
			}
			dRef = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
			dst->InsertTag(dRef,NULL);
			CDAddUndo(doc,CD_UNDO_NEW,dRef);
			
			dst->Message(MSG_UPDATE);
			DescriptionCommand skrdc;
			skrdc.id = DescID(SKR_SET_REFERENCE);
			dRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);

			dTag = BaseTag::Alloc(ID_CDSKINPLUGIN);
			
			BaseTag *mRef = dst->GetTag(ID_CDMORPHREFPLUGIN);
			if(mRef) dst->InsertTag(dTag,mRef);
			else dst->InsertTag(dTag,dRef);
			CDAddUndo(doc,CD_UNDO_NEW,dTag);
			
			BuildSpacePartitions(src);
			
			if(!CopySkinData(doc,src,sTag,dst,dTag))
			{
				if(dTag) BaseTag::Free(dTag);
				if(dRef) BaseTag::Free(dRef);
				return false;
			}
			
			FreeSpacePartitions();
		}
	}
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;
}

class CDTransferSkinR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		return true;
	}
};

Bool RegisterCDTransferSkinCommand(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
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
	
	// decide by name if the plugin shall be registered - just for user convenience  PLUGINFLAG_HIDE|
	String name = GeLoadString(IDS_CDSKINTRANS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDTRANSFERSKIN,name,PLUGINFLAG_HIDE,"CDTransSkin.tif","CD Transfer Skin",CDDataAllocator(CDTransferSkinR));
	else return CDRegisterCommandPlugin(ID_CDTRANSFERSKIN,name,0,"CDTransSkin.tif","CD Transfer Skin",CDDataAllocator(CDTransferSkin));
}

//	Cactus Dan's Symmetry Tools 1.0
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDCompatibility.h"

#include "CDSymmetryTag.h"
#include "CDMorph.h"

#include "tCDSymTag.h"

Bool CDSymmetryTag::Init(GeListNode *node)
{	
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(SYM_SHOW_GUIDE,false);
	tData->SetReal(SYM_GUIDE_SIZE,50.0);
	tData->SetVector(SYM_GUIDE_COLOR, Vector(1,0,0));
	
	tData->SetReal(SYM_TOLERANCE,0.01);
	tData->SetLong(SYM_SYMMETRY_AXIS,SYM_MX);
	tData->SetLong(SYM_RESTRICT_AXIS,SYM_POSITIVE);
	
	tData->SetBool(SYM_POINTS_ADDED,false);
	tData->SetBool(SYM_OPEN_TOOL,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	mpt.Init();
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDMData syd;
	PluginMessage(ID_CDSYMMETRYTAG,&syd);
	if(syd.list) syd.list->Append(node);
	
	return true;
}

void CDSymmetryTag::Free(GeListNode *node)
{	
	mpt.Free();
	
	CDMData syd;
	PluginMessage(ID_CDSYMMETRYTAG,&syd);
	if(syd.list) syd.list->Remove(node);
}

Bool CDSymmetryTag::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG pCnt = tData->GetLong(SYM_POINT_COUNT);
	
	if (pCnt > 0)
	{
		//Level 0
		if (level >= 0)
		{
			if(!mpt.Alloc(pCnt)) return false;
			LONG i;
			for (i=0; i<pCnt; i++)
			{
				CDHFReadLong(hf, &mpt[i]);
			}
		}
	}
	
	
	return true;
}

Bool CDSymmetryTag::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG pCnt = tData->GetLong(SYM_POINT_COUNT);
	
	if (pCnt > 0)
	{
		//Level 0
		LONG i;
		for (i=0; i<pCnt; i++)
		{
			CDHFWriteLong(hf, mpt[i]);
		}
	}
	
	return true;
}

Bool CDSymmetryTag::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag  = (BaseTag*)snode; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG pCnt = tData->GetLong(SYM_POINT_COUNT);
	
	if(pCnt > 0)
	{
		mpt.Copy(((CDSymmetryTag*)dest)->mpt);
	}
	
	return true;
}

Bool CDSymmetryTag::SetNewPointsSymmetry(BaseTag *tag, BaseObject *op, BaseContainer *tData, TranslationMaps *tMap)
{
	Vector  *padr = GetPointArray(op); if(!padr) return false;
	LONG i, mi, mInd, ptCnt = ToPoint(op)->GetPointCount();
	LONG mCnt = tMap->m_mPointCount;
	
	
	Bool mirrored = false, allMirrored = true;
	Real tolerance = tData->GetReal(SYM_TOLERANCE);
	
	for(i=0; i<mCnt; i++)
	{
		LONG ind = tMap->m_pPointMap[i].nIndex;
		if(ind > -1 && ind < ptCnt)
		{
			if(mpt[ind] == -1)
			{
				Vector  ptMirAsn, ptMir, pt = padr[ind];
				
				switch (tData->GetLong(SYM_SYMMETRY_AXIS))
				{
					case SYM_MX:
						pt.x *= -1;
						break;
					case SYM_MY:
						pt.y *= -1;
						break;
					case SYM_MZ:
						pt.z *= -1;
						break;
					default:
						break;
				}
				Real prvDif = CDMAXREAL;
				mInd = -1;
				for(mi=0; mi<mCnt; mi++)
				{
					LONG sInd = tMap->m_pPointMap[mi].nIndex;
					if(sInd > -1 && sInd < ptCnt)
					{
						ptMir = padr[sInd];
						Real dif = LenSquare(ptMir,pt);
						if(dif < tolerance*tolerance)
						{
							if(dif < prvDif)
							{
								if(ind != sInd)
								{
									mInd = sInd;
									prvDif = dif;
								}
								else
								{
									if(VEqual(ptMir,pt,tolerance)) mInd = -2;
								}
								mirrored = true;
							}
						}
					}
				}
				if(mpt[ind] < 0)
				{
					if(mInd >= 0)
					{
						if(mpt[mInd] < 0)
						{
							mpt[ind] = mInd;
							mpt[mInd] = ind;
						}
						else
						{
							ptMir = padr[mInd];
							if(mpt[mInd] > -1)
							{
								ptMirAsn = padr[mpt[mInd]];
								
								if(LenSquare(ptMir,pt) < LenSquare(ptMir,ptMirAsn))
								{
									mpt[ind] = mInd;
									mpt[mpt[mInd]] = -1;
									mpt[mInd] = ind;
								}
							}
						}
					}
					else if(mInd < -1) mpt[ind] = mInd;
				}
				else
				{
					if(mInd > -1)
					{
						if(mpt[ind] != mInd)
						{
							mpt[ind] = mInd;
							if(mpt[mInd] > -1) mpt[mpt[mInd]] = -1;
							mpt[mInd] = ind;
						}
					}
				}
				
				if(!mirrored) allMirrored = false;
			}
		}
	}
	
	return allMirrored;
}

Bool CDSymmetryTag::SetAddedPointsSymmetry(BaseTag *tag, BaseObject *op, BaseContainer *tData, LONG oCnt, LONG nCnt)
{
	Vector  *padr = GetPointArray(op); if(!padr) return false;
	LONG i, mi, mInd;
	
	Vector pt, ptMir, ptMirAsn;
	Bool mirrored = false, allMirrored = true;
	Real dif, prvDif, tolerance = tData->GetReal(SYM_TOLERANCE);
	
	if(!mpt.IsEmpty())
	{
		// calculate mirror assignments
		for(i=oCnt; i<nCnt; i++)
		{
			pt = padr[i];
			
			// calculate the points mirror position
			switch (tData->GetLong(SYM_SYMMETRY_AXIS))
			{
				case SYM_MX:
					pt.x *= -1;
					break;
				case SYM_MY:
					pt.y *= -1;
					break;
				case SYM_MZ:
					pt.z *= -1;
					break;
				default:
					break;
			}
			mirrored = false;
			
			prvDif = CDMAXREAL;
			mInd = -1;
			for(mi=oCnt; mi<nCnt; mi++)
			{
				// find the closest point index to the mirror position
				ptMir = padr[mi];
				dif = LenSquare(ptMir,pt);
				if(dif < tolerance*tolerance)
				{
					if(dif < prvDif)
					{
						if(i != mi)
						{
							mInd = mi;
							prvDif = dif;
						}
						else
						{
							if(VEqual(ptMir,pt,tolerance)) mInd = -2;
						}
						mirrored = true;
					}
				}
			}
			
			if(mpt[i] < 0)
			{
				if(mInd > -1)
				{
					if(mpt[mInd] < 0)
					{
						// if the mirror index has no previous assignment
						mpt[i] = mInd;
						mpt[mInd] = i;
					}
					else
					{
						// if the mirror index was already assigned, double check for closer assignment
						if(mpt[mInd] > -1)
						{
							ptMir = padr[mInd];
							ptMirAsn = padr[mpt[mInd]];
							
							if(LenSquare(ptMir,pt) < LenSquare(ptMir,ptMirAsn))
							{
								mpt[i] = mInd;
								mpt[mpt[mInd]] = -1;
								mpt[mInd] = i;
							}
						}
					}
				}
				else if(mInd < -1) mpt[i] = mInd;
			}
			else
			{
				if(mInd > -1)
				{
					if(mpt[i] != mInd)
					{
						mpt[i] = mInd;
						if(mpt[mInd] > -1) mpt[mpt[mInd]] = -1;
						mpt[mInd] = i;
					}
				}
			}
			
			if(!mirrored) allMirrored = false;
		}
		
	}
	
	return allMirrored;
}

void CDSymmetryTag::CheckMirrorAssignment(Vector *padr, Vector pt, Vector ptMir, LONG pInd, LONG mInd)
{
	if(mpt[pInd] < 0)
	{
		if(mInd > -1)
		{
			if(mpt[mInd] < 0)
			{
				// if the mirror index has no previous assignment
				mpt[pInd] = mInd;
				mpt[mInd] = pInd;
			}
			else
			{
				// if the mirror index was already assigned, double check for closer assignment
				ptMir = padr[mInd];
				if(mpt[mInd] > -1)
				{
					Vector ptMirAsn = padr[mpt[mInd]];
					
					if(LenSquare(ptMir,pt) < LenSquare(ptMir,ptMirAsn))
					{
						mpt[pInd] = mInd;
						mpt[mpt[mInd]] = -1;
						mpt[mInd] = pInd;
					}
				}
			}
		}
		else if(mInd < -1) mpt[pInd] = mInd;
	}
	else
	{
		if(mInd > -1)
		{
			if(mpt[pInd] != mInd)
			{
				mpt[pInd] = mInd;
				if(mpt[mInd] > -1) mpt[mpt[mInd]] = -1;
				mpt[mInd] = pInd;
			}
		}
	}
	
}

LONG CDSymmetryTag::GetMirrorIndex(Vector *padr, Vector pt, LONG *neg, LONG n, LONG pInd, LONG pCnt, Real t)
{
	Real prvDif = CDMAXREAL;
	LONG mi, mInd = -1;
	
	for(mi=0; mi<n; mi++)
	{
		LONG nInd = neg[mi];
		if(nInd > -1 && nInd < pCnt)
		{
			if(mpt[nInd] == -1)
			{
				Vector ptMir = padr[nInd];
				
				// find the closest point index to the mirror position
				if(VEqual(pt,ptMir,t))
				{
					Real dif = LenSquare(ptMir,pt);
					if(dif < t*t)
					{
						if(dif < prvDif)
						{
							if(pInd != nInd)
							{
								mInd = nInd;
								prvDif = dif;
							}
							else
							{
								if(VEqual(ptMir,pt,t)) mInd = -2;
							}
						}
					}
				}
			}
		}
	}
	
	return mInd;
}

void CDSymmetryTag::BuildSpacePartitions(BaseContainer *tData, Vector *padr, Vector mp, LONG pCnt, LONG qSize)
{
	LONG i;
	for(i=0; i<pCnt; i++)
	{
		Vector pt = padr[i];
		switch (tData->GetLong(SYM_SYMMETRY_AXIS))
		{
			case SYM_MX:
			{
				if(pt.x > 0.0)
				{
					if(pt.y > mp.y)
					{
						if(pt.z > mp.z) pB.Append(i);
						else pA.Append(i);
					}
					else
					{
						if(pt.z > mp.z) pD.Append(i);
						else pC.Append(i);
					}
				}
				else
				{
					if(pt.y > mp.y)
					{
						if(pt.z > mp.z) nB.Append(i);
						else nA.Append(i);
					}
					else
					{
						if(pt.z > mp.z) nD.Append(i);
						else nC.Append(i);
					}
				}
				break;
			}
			case SYM_MY:
			{
				if(pt.y > 0.0)
				{
					if(pt.x > mp.x)
					{
						if(pt.z > mp.z) pB.Append(i);
						else pA.Append(i);
					}
					else
					{
						if(pt.z > mp.z) pD.Append(i);
						else pC.Append(i);
					}
				}
				else
				{
					if(pt.x > mp.x)
					{
						if(pt.z > mp.z) nB.Append(i);
						else nA.Append(i);
					}
					else
					{
						if(pt.z > mp.z) nD.Append(i);
						else nC.Append(i);
					}
				}
				break;
			}
			case SYM_MZ:
			{
				if(pt.z > 0.0)
				{
					if(pt.y > mp.y)
					{
						if(pt.x > mp.x) pB.Append(i);
						else pA.Append(i);
					}
					else
					{
						if(pt.x > mp.x) pD.Append(i);
						else pC.Append(i);
					}
				}
				else
				{
					if(pt.y > mp.y)
					{
						if(pt.x > mp.x) nB.Append(i);
						else nA.Append(i);
					}
					else
					{
						if(pt.x > mp.x) nD.Append(i);
						else nC.Append(i);
					}
				}
				break;
			}
			default:
				break;
		}
	}
}

Bool CDSymmetryTag::SetSymmetry(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Vector  *padr = GetPointArray(op); if(!padr) return false;
	LONG i, j, pCnt = ToPoint(op)->GetPointCount();
	
	CDAddUndo(doc,CD_UNDO_CHANGE,tag);
	mpt.Init();
	mpt.Alloc(pCnt);
	
	Bool mirrored = true;
	Vector mp = op->GetMp();
	Vector pt, ptMir, ptMirAsn;
	Real tolerance = tData->GetReal(SYM_TOLERANCE);
	LONG mInd; 
	
	//initialize all array elements to -1
	mpt.Fill(-1);
	
	StatusSetSpin();
	
	// find center points
	for(i=0; i<pCnt; i++)
	{
		pt = padr[i];
		switch (tData->GetLong(SYM_SYMMETRY_AXIS))
		{
			case SYM_MX:
			{
				pt.x = 0.0;
				if(VEqual(pt,padr[i],tolerance)) mpt[i] = -2;
				break;
			}
			case SYM_MY:
			{
				pt.y = 0.0;
				if(VEqual(pt,padr[i],tolerance)) mpt[i] = -2;
				break;
			}
			case SYM_MZ:
			{
				pt.z = 0.0;
				if(VEqual(pt,padr[i],tolerance)) mpt[i] = -2;
				break;
			}
			default:
				break;
		}
	}
	
	LONG qSize = sizeof(LONG)*(pCnt*0.5);
	
	// allocate space partition arrays
	pA.Init();
	pB.Init();
	pC.Init();
	pD.Init();
	nA.Init();
	nB.Init();
	nC.Init();
	nD.Init();
	
	BuildSpacePartitions(tData, padr,mp,pCnt,qSize);
	
	StatusClear();
	
	// calculate mirror assignments
	StatusSetText("Checking Mesh Symmetry");
	
	LONG p, n;
	LONG *pos=NULL, *neg=NULL;
	
	LONG fin = pA.Size() + pB.Size() + pC.Size() + pD.Size();
	LONG prg = 0;
	for(i=0; i<4; i++)
	{
		if(i==0)
		{
			p = pA.Size();  pos = pA.Array();
			n = nA.Size();  neg = nA.Array();
		}
		else if(i==1)
		{
			p = pB.Size();  pos = pB.Array();
			n = nB.Size();  neg = nB.Array();
		}
		else if(i==2)
		{
			p = pC.Size();  pos = pC.Array();
			n = nC.Size();  neg = nC.Array();
		}
		else if(i==3)
		{
			p = pD.Size();  pos = pD.Array();
			n = nD.Size();  neg = nD.Array();
		}
		
		for(j=0; j<p; j++)
		{
			LONG prgrs = LONG(Real(prg)/Real(fin)*100.0);
			StatusSetBar(prgrs);
			
			if(pos[j] > -1 && pos[j] < pCnt)
			{
				LONG pInd = pos[j];
				pt = padr[pInd];
				
				// calculate the points mirror position
				switch (tData->GetLong(SYM_SYMMETRY_AXIS))
				{
					case SYM_MX:
						pt.x *= -1;
						break;
					case SYM_MY:
						pt.y *= -1;
						break;
					case SYM_MZ:
						pt.z *= -1;
						break;
					default:
						break;
				}
				
				mInd = GetMirrorIndex(padr,pt,neg,n,pInd,pCnt,tolerance);
				CheckMirrorAssignment(padr,pt,ptMir,pInd,mInd);
				prg++;
			}
		}
	}
	pA.Free();
	pB.Free();
	pC.Free();
	pD.Free();
	nA.Free();
	nB.Free();
	nC.Free();
	nD.Free();
	
	StatusClear();
	
	// check for unsymetrical points 
	StatusSetSpin();
	LONG unSym = 0;
	for(i=0; i<pCnt; i++)
	{
		if(mpt[i] == -1) unSym++;
	}
	
	if(unSym > 0)
	{
		// build space partitian arrays
		CDArray<LONG> ps, ng;
		ps.Init();
		ng.Init();
		for(i=0; i<pCnt; i++)
		{
			if(mpt[i] == -1)
			{
				pt = padr[i];
				
				switch (tData->GetLong(SYM_SYMMETRY_AXIS))
				{
					case SYM_MX:
					{
						if(pt.x > 0.0) ps.Append(i);
						else if(pt.x < 0.0) ng.Append(i);
						break;
					}
					case SYM_MY:
					{
						if(pt.y > 0.0) ps.Append(i);
						else if(pt.y < 0.0) ng.Append(i);
						break;
					}
					case SYM_MZ:
					{
						if(pt.z > 0.0) ps.Append(i);
						else if(pt.z < 0.0) ng.Append(i);
						break;
					}
					default:
						break;
				}
			}
		}
		StatusClear();
		
		StatusSetText("Testing Unsymmetrical Points");
		for(i=0; i<ps.Size(); i++)
		{
			LONG prgrs = LONG(Real(i)/Real(p)*100.0);
			StatusSetBar(prgrs);
			
			LONG pInd = pos[i];
			pt = padr[pInd];
			
			// calculate the points mirror position
			switch (tData->GetLong(SYM_SYMMETRY_AXIS))
			{
				case SYM_MX:
					pt.x *= -1;
					break;
				case SYM_MY:
					pt.y *= -1;
					break;
				case SYM_MZ:
					pt.z *= -1;
					break;
				default:
					break;
			}
			
			mInd = GetMirrorIndex(padr,pt,ng.Array(),ng.Size(),pInd,pCnt,tolerance*10);
			CheckMirrorAssignment(padr,pt,ptMir,pInd,mInd);
		}
		
		ps.Free();
		ng.Free();
		
		unSym = 0;
		for(i=0; i<pCnt; i++)
		{
			if(mpt[i] == -1) unSym++;
		}
		if(unSym > 0) mirrored = false;
	}
	
	StatusClear();
	
	tData->SetLong(SYM_POINT_COUNT,pCnt);
	tData->SetLink(SYM_OP_ID,op);
	tData->SetBool(SYM_SYMMETRY_IS_SET,true);
	tData->SetBool(SYM_USE_SYMMETRY,true);
	
	return mirrored;
}

Bool CDSymmetryTag::TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg)
{
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, pCnt =ToPoint(op)->GetPointCount();
	
	LONG oCnt = vchg->old_cnt, nCnt = vchg->new_cnt;
	if(vchg->map)
	{
		TranslationMaps tMap;
		
		tMap.m_oPointCount = oCnt; // old point count
		tMap.m_nPointCount = nCnt; // new point count
		
		if(nCnt < oCnt)
		{
			// Get remap count
			LONG mCnt = 0;
			for(i=0; i<oCnt; i++)
			{
				if(vchg->map[i] != i) mCnt++;
			}
			tMap.m_mPointCount = mCnt; // map point count
			
			// Allocate the m_pPointMap array
			CDArray<TransMapData> pMap;
			
			// build the TranslationMaps data
			mCnt = 0;
			for(i=0; i<oCnt; i++)
			{
				if(vchg->map[i] != i)
				{
					TransMapData tmd = TransMapData(i,vchg->map[i]);
					tmd.mIndex = mCnt;
					if(vchg->map[i] < 0) tmd.lFlags |= TRANSMAP_FLAG_DELETED;
					pMap.Append(tmd);
					mCnt++;
				}
			}
			tMap.m_pPointMap = pMap.Array();
			
			if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			RemapMirrorPoints(&tMap);
			
			tData->SetLong(SYM_POINT_COUNT,nCnt);
			pMap.Free();
		}
		else if(nCnt > oCnt)
		{
			// Get remap count
			LONG mCnt = 0;
			for(i=0; i<nCnt; i++)
			{
				if(i<oCnt)
				{
					if(vchg->map[i] != i) mCnt++;
				}
				else mCnt++;
			}
			tMap.m_mPointCount = mCnt; // map point count
			
			// Allocate the m_pPointMap array
			CDArray<TransMapData> pMap;
			
			// build the TranslationMaps data
			mCnt = 0;
			for(i=0; i<nCnt; i++)
			{
				if(i<oCnt)
				{
					if(vchg->map[i] != i)
					{
						TransMapData tmd = TransMapData(i,vchg->map[i]);
						tmd.mIndex = mCnt;
						if(vchg->map[i] < 0) tmd.lFlags |= TRANSMAP_FLAG_DELETED;
						pMap.Append(tmd);
						mCnt++;
					}
				}
				else
				{
					TransMapData tmd = TransMapData(-1,i);
					tmd.mIndex = mCnt;
					tmd.lFlags |= TRANSMAP_FLAG_NEW;
					pMap.Append(tmd);
					mCnt++;
				}
			}
			tMap.m_pPointMap = pMap.Array();
			
			if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			RemapMirrorPoints(&tMap);
			
			tData->SetBool(SYM_POINTS_ADDED,true);
			tData->SetLong(SYM_POINT_COUNT,nCnt);
			tData->SetLong(SYM_OLD_PT_COUNT,oCnt);
			pMap.Free();
		}
	}
	else
	{
		if(nCnt < oCnt)
		{
			if(!mpt.IsEmpty() && nCnt == pCnt && pCnt < tData->GetLong(SYM_POINT_COUNT))
			{
				// resize storage array
				CDArray<LONG> temp;
				mpt.Copy(temp);
				mpt.Resize(nCnt);
				
				for(i=0; i<nCnt; i++)
				{
					if(temp[i] >= pCnt) mpt[i] = -1;
				}
				temp.Free();
				tData->SetLong(SYM_POINT_COUNT,nCnt);
			}
		}
		else if(nCnt > oCnt)
		{
			if(!mpt.IsEmpty() && nCnt == pCnt)
			{
				// resize storage array
				mpt.Resize(nCnt);
				
				for(i=oCnt; i<nCnt; i++)
				{
					mpt[i] = -1;
				}
				tData->SetLong(SYM_POINT_COUNT,nCnt);
				tData->SetLong(SYM_OLD_PT_COUNT,oCnt);
				tData->SetBool(SYM_POINTS_ADDED,true);
			}
		}
		else if(nCnt == oCnt)
		{
			if(!mpt.IsEmpty() && tData->GetBool(SYM_POINTS_ADDED))
			{
				LONG oldCnt = tData->GetLong(SYM_OLD_PT_COUNT);
				
				SetAddedPointsSymmetry(tag,op,tData,oldCnt,nCnt);
				
				tData->SetBool(SYM_POINTS_ADDED,false);
				tData->SetLong(SYM_POINT_COUNT,nCnt);
				tData->SetLong(SYM_OLD_PT_COUNT,oCnt);
			}
		}
	}
	
	return true;
}

void CDSymmetryTag::RemapMirrorPoints(TranslationMaps *tMap)
{
	LONG oCnt = tMap->m_oPointCount, nCnt = tMap->m_nPointCount;
	LONG i, oInd, nInd, mCnt = tMap->m_mPointCount;
	
	if(!mpt.IsEmpty() && tMap)
	{
		// Create temp storage array
		CDArray<LONG> temp;
		mpt.Copy(temp);
		mpt.Resize(nCnt);
		
		if(nCnt > oCnt)
		{
			for(i=oCnt; i<nCnt; i++)
			{
				mpt[i] = -1;
			}
		}
		
		// create remapping index storage array
		CDArray<LONG> mapInd;
		mapInd.Alloc(oCnt);
		for(i=0; i<oCnt; i++)
		{
			mapInd[i] = i;
		}
		
		// store mapped mirror indices
		for(i=0; i<mCnt; i++)
		{
			oInd = tMap->m_pPointMap[i].oIndex;
			nInd = tMap->m_pPointMap[i].nIndex;
			
			if(nCnt < oCnt)
			{
				if(oInd > -1 && nInd != oInd) mapInd[oInd] = nInd;
			}
			else if(nCnt > oCnt)
			{
				if(oInd > -1 && nInd > -1 && nInd != oInd) mapInd[oInd] = nInd;
			}
		}
		
		// remap storage array
		for(i=0; i<mCnt; i++)
		{
			oInd = tMap->m_pPointMap[i].oIndex;
			nInd = tMap->m_pPointMap[i].nIndex;
			
			
			if(tMap->m_pPointMap[i].lFlags & TRANSMAP_FLAG_DELETED)
			{
				if(temp[oInd] > -1)
				{
					mpt[oInd] = -1;
					mpt[temp[oInd]] = -1;
				}
			}
			else
			{
				if(tMap->m_pPointMap[i].lFlags & TRANSMAP_FLAG_NEW) continue;
				else if(nInd != oInd)
				{
					mpt[oInd] = -1;
					if(temp[oInd] > -1)
					{
						mpt[nInd] = mapInd[temp[oInd]];
						if(mapInd[temp[oInd]] > -1)  mpt[mapInd[temp[oInd]]] = nInd;
					}
					else if(temp[oInd] == -2) mpt[nInd] = -2;
				}
			}
		}
		mapInd.Free();
		temp.Free();
	}
}

Bool CDSymmetryTag::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdsnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdsnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdsnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdsnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdsnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdsnr);
			break;
		}
	}
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	if(doc) CheckTagReg(op,tData);
	
	LONG oldPCnt = tData->GetLong(SYM_POINT_COUNT);
	
	switch (type)
	{
		case CD_MSG_UPDATE:
		{
			CDExecute(tag,doc,op,NULL,0,0);
			break;
		}
		case MSG_POINTS_CHANGED:
		{
			if(doc)
			{
				if(doc->GetAction() == ID_MODELING_BEVEL_TOOL) break;
			}
			VariableChanged *vchg = (VariableChanged*)data;
			if(!mpt.IsEmpty() && vchg)
			{
				if(vchg->old_cnt == oldPCnt) TransferTMaps(tag,doc,tData,op,vchg);
			}
			break;
		}
		case MSG_TRANSLATE_POINTS:
		{
			TranslationMaps *tMap = (TranslationMaps*)data;
			if(!mpt.IsEmpty() && tMap) 
			{
				LONG oCnt = tMap->m_oPointCount, nCnt = tMap->m_nPointCount, mCnt = tMap->m_mPointCount;
				if(oCnt == oldPCnt && mCnt > 0)
				{
					LONG i, ind;
					if(nCnt == oCnt)
					{
						for(i=0; i<mCnt; i++)
						{
							Vector *padr = GetPointArray(op);
							if(padr)
							{
								ind = tMap->m_pPointMap[i].nIndex;
								Vector pt = padr[ind];
								switch (tData->GetLong(SYM_SYMMETRY_AXIS))
								{
									case SYM_MX:
										pt.x *= -1;
										break;
									case SYM_MY:
										pt.y *= -1;
										break;
									case SYM_MZ:
										pt.z *= -1;
										break;
									default:
										break;
								}
								if(mpt[ind] > -1)  padr[mpt[ind]] = pt;
							}
						}
					}
					else if(nCnt != oCnt)
					{
						if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						RemapMirrorPoints(tMap);
						
						if(!mpt.IsEmpty() && nCnt > oCnt)
						{
							SetNewPointsSymmetry(tag,op,tData,tMap);
						}
						
						tData->SetLong(SYM_POINT_COUNT,nCnt);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SYM_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==SYM_SET_SYMMETRY)
			{
				if(doc)
				{
					if(tData->GetBool(T_REG))
					{
						if(!SetSymmetry(tag,doc,op,tData))
						{
							MessageDialog(GeLoadString(NOT_ALL_POINTS));
							if(tData->GetBool(SYM_OPEN_TOOL))
							{
							 	doc->SetAction(ID_CDSYMMETRYASSIGN);
							 	tData->SetBool(SYM_OPEN_TOOL,false);
							}
						}
					}
				}
			}
			else if (dc->id[0].id==SYM_RELEASE_SYMMETRY)
			{
				if(doc)
				{
					if(tData->GetBool(T_REG))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						tData->SetLong(SYM_POINT_COUNT,0);
						tData->SetBool(SYM_SYMMETRY_IS_SET,false);
						tData->SetBool(SYM_USE_SYMMETRY,false);
						mpt.Free();
					}
				}
			}
			break;
		}
	}
	
	return true;
}

void CDSymmetryTag::MirrorSplineTangents(BaseContainer *tData, BaseObject *op, LONG src, LONG dst)
{
	Tangent *tngt = NULL;
	
#if API_VERSION < 9800
	tngt = ToSpline(op)->GetTangent();
#else
	tngt = ToSpline(op)->GetTangentW();
#endif
	
	if(tngt)
	{
		switch(tData->GetLong(SYM_SYMMETRY_AXIS))
		{
			case SYM_MX:
				tngt[dst].vl.x = tngt[src].vr.x * -1;
				tngt[dst].vr.x = tngt[src].vl.x * -1;
				tngt[dst].vl.y = tngt[src].vr.y;
				tngt[dst].vr.y = tngt[src].vl.y;
				tngt[dst].vl.z = tngt[src].vr.z;
				tngt[dst].vr.z = tngt[src].vl.z;
				break;
			case SYM_MY:
				tngt[dst].vl.x = tngt[src].vr.x;
				tngt[dst].vr.x = tngt[src].vl.x;
				tngt[dst].vl.y = tngt[src].vr.y * -1;
				tngt[dst].vr.y = tngt[src].vl.y * -1;
				tngt[dst].vl.z = tngt[src].vr.z;
				tngt[dst].vr.z = tngt[src].vl.z;
				break;
			case SYM_MZ:
				tngt[dst].vl.y = tngt[src].vr.y;
				tngt[dst].vr.y = tngt[src].vl.y;
				tngt[dst].vl.x = tngt[src].vr.x;
				tngt[dst].vr.x = tngt[src].vl.x;
				tngt[dst].vl.z = tngt[src].vr.z * -1;
				tngt[dst].vr.z = tngt[src].vl.z * -1;
				break;
		}
	}
}

LONG CDSymmetryTag::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	if(mpt.IsEmpty()) return false;
	if(!IsValidPointObject(op)) return false;
	
	Vector  *padr = GetPointArray(op);
	LONG i, pCnt = ToPoint(op)->GetPointCount();
	if(pCnt != tData->GetLong(SYM_POINT_COUNT)) return false;
	
	Bool mst = false;
	if(op->IsInstanceOf(Ospline) && op->GetDataInstance()->GetLong(SPLINEOBJECT_TYPE) == SPLINEOBJECT_TYPE_BEZIER) mst = true;
	
	if(tData->GetBool(SYM_SYMMETRY_IS_SET) && tData->GetBool(SYM_USE_SYMMETRY))
	{
		if(doc->IsEditMode() && op->GetBit(BIT_ACTIVE))
		{
			if(!tData->GetBool(SYM_RESTRICT_SYM))
			{
				BaseSelect *ptSel = ToPoint(op)->GetPointS();
				
				AutoAlloc<BaseSelect> bs;
				if(bs)
				{
					switch (doc->GetMode())
					{
						case Mpoints:
						{
							bs->Merge(ptSel);
							break;
						}
						case Medges:
						{
							if(IsValidPolygonObject(op))
							{
								BaseSelect *edgeS = ToPoly(op)->GetEdgeS();
								if(edgeS) ConvertToPointSelection(edgeS,bs,op,Medges);
							}
							break;
						}
						case Mpolygons:
						{
							if(IsValidPolygonObject(op))
							{
								BaseSelect *plyS = ToPoly(op)->GetPolygonS();
								if(plyS) ConvertToPointSelection(plyS,bs,op,Mpolygons);
							}
							break;
						}
					}
					LONG bsCnt = bs->GetCount();
					if(bsCnt>0)
					{
						LONG seg=0,a,b;
						while(CDGetRange(bs,seg++,&a,&b))
						{
							for(i=a; i<=b; ++i)
							{
								Vector pt = padr[i];
								switch (tData->GetLong(SYM_SYMMETRY_AXIS))
								{
									case SYM_MX:
										pt.x *= -1;
										break;
									case SYM_MY:
										pt.y *= -1;
										break;
									case SYM_MZ:
										pt.z *= -1;
										break;
									default:
										break;
								}
								if(mpt[i] > -1)
								{
									padr[mpt[i]] = pt;
									
									// if bezier spline mirror tangents
									if(mst) MirrorSplineTangents(tData,op,i,mpt[i]);
								}
							}
						}
					}
				}
				if(tData->GetBool(SYM_LOCK_CENTER))
				{
					for(i=0; i<pCnt; i++)
					{
						if(mpt[i] < -1)
						{
							Vector pt = padr[i];
							switch (tData->GetLong(SYM_SYMMETRY_AXIS))
							{
								case SYM_MX:
									pt.x = 0.0;
									break;
								case SYM_MY:
									pt.y = 0.0;
									break;
								case SYM_MZ:
									pt.z = 0.0;
									break;
								default:
									break;
							}
							padr[i] = pt;
						}
					}
				}
			}
			else
			{
				for(i=0; i<pCnt; i++)
				{
					Vector pt = padr[i];
					switch (tData->GetLong(SYM_SYMMETRY_AXIS))
					{
						case SYM_MX:
							pt.x *= -1;
							break;
						case SYM_MY:
							pt.y *= -1;
							break;
						case SYM_MZ:
							pt.z *= -1;
							break;
						default:
							break;
					}
					if(mpt[i] > -1)
					{
						switch (tData->GetLong(SYM_RESTRICT_AXIS))
						{
							case SYM_POSITIVE:
							{
								switch (tData->GetLong(SYM_SYMMETRY_AXIS))
								{
									case SYM_MX:
										if(pt.x < 0.0)
										{
											padr[mpt[i]] = pt;
											if(mst) MirrorSplineTangents(tData,op,i,mpt[i]);
										}
										break;
									case SYM_MY:
										if(pt.y < 0.0)
										{
											padr[mpt[i]] = pt;
											if(mst) MirrorSplineTangents(tData,op,i,mpt[i]);
										}
										break;
									case SYM_MZ:
										if(pt.z < 0.0)
										{
											padr[mpt[i]] = pt;
											if(mst) MirrorSplineTangents(tData,op,i,mpt[i]);
										}
										break;
									default:
										break;
								}
								break;
							}
							case SYM_NEGATIVE:
							{
								switch (tData->GetLong(SYM_SYMMETRY_AXIS))
								{
									case SYM_MX:
										if(pt.x > 0.0)
										{
											padr[mpt[i]] = pt;
											if(mst) MirrorSplineTangents(tData,op,i,mpt[i]);
										}
										break;
									case SYM_MY:
										if(pt.y > 0.0)
										{
											padr[mpt[i]] = pt;
											if(mst) MirrorSplineTangents(tData,op,i,mpt[i]);
										}
										break;
									case SYM_MZ:
										if(pt.z > 0.0)
										{
											padr[mpt[i]] = pt;
											if(mst) MirrorSplineTangents(tData,op,i,mpt[i]);
										}
										break;
									default:
										break;
								}
								break;
							}
						}
					}
					
					if(tData->GetBool(SYM_LOCK_CENTER) && mpt[i] < -1)
					{
						switch (tData->GetLong(SYM_SYMMETRY_AXIS))
						{
							case SYM_MX:
								pt.x = 0.0;
								break;
							case SYM_MY:
								pt.y = 0.0;
								break;
							case SYM_MZ:
								pt.z = 0.0;
								break;
							default:
								break;
						}
						padr[i] = pt;
					}
				}
			}
			op->Message(MSG_UPDATE);
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSymmetryTag::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = tag->GetDocument(); if(!doc) return true;
	if(!doc->IsEditMode()) return true;
	
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	if(!tData->GetBool(SYM_SHOW_GUIDE)) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Matrix opM = op->GetMg();
	
	Vector rad = op->GetRad();
	rad.x += tData->GetReal(SYM_GUIDE_SIZE);
	rad.y += tData->GetReal(SYM_GUIDE_SIZE);
	rad.z += tData->GetReal(SYM_GUIDE_SIZE);
	
	Vector mp = op->GetMp();
	Vector p1, p2, p3, p4;
	
	switch(tData->GetLong(SYM_SYMMETRY_AXIS))
	{
		case SYM_MX:
			p1 = opM * Vector(0,mp.y+rad.y,mp.z-rad.z);
			p2 = opM * Vector(0,mp.y+rad.y,mp.z+rad.z);
			p3 = opM * Vector(0,mp.y-rad.y,mp.z+rad.z);
			p4 = opM * Vector(0,mp.y-rad.y,mp.z-rad.z);
			break;
		case SYM_MY:
			p1 = opM * Vector(mp.x+rad.x,0,mp.z-rad.z);
			p2 = opM * Vector(mp.x+rad.x,0,mp.z+rad.z);
			p3 = opM * Vector(mp.x-rad.x,0,mp.z+rad.z);
			p4 = opM * Vector(mp.x-rad.x,0,mp.z-rad.z);
			break;
		case SYM_MZ:
			p1 = opM * Vector(mp.x-rad.x,mp.y+rad.y,0);
			p2 = opM * Vector(mp.x+rad.x,mp.y+rad.y,0);
			p3 = opM * Vector(mp.x+rad.x,mp.y-rad.y,0);
			p4 = opM * Vector(mp.x-rad.x,mp.y-rad.y,0);
			break;
	}
	
	Vector p[4] = { p1,p2,p3,p4 };
	
	Vector color = tData->GetVector(SYM_GUIDE_COLOR);
	
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
		
	return true;
}

Bool CDSymmetryTag::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SYM_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSymmetryTag::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case SYM_USE_SYMMETRY:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SYM_SYMMETRY_IS_SET);
		case SYM_SET_SYMMETRY:
			if(tData->GetBool(SYM_SYMMETRY_IS_SET) || !tData->GetBool(T_REG)) return false;
			else return true;
		case SYM_RELEASE_SYMMETRY:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SYM_SYMMETRY_IS_SET);
		case SYM_TOLERANCE:
			if(tData->GetBool(SYM_SYMMETRY_IS_SET) || !tData->GetBool(T_REG)) return false;
			else return true;
		case SYM_SYMMETRY_AXIS:
			if(tData->GetBool(SYM_SYMMETRY_IS_SET) || !tData->GetBool(T_REG)) return false;
			else return true;
		case SYM_RESTRICT_SYM:
			if(!tData->GetBool(SYM_SYMMETRY_IS_SET) || !tData->GetBool(T_REG)) return false;
			else return true;
		case SYM_RESTRICT_AXIS:
			if(!tData->GetBool(SYM_SYMMETRY_IS_SET) || !tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SYM_RESTRICT_SYM);
		case SYM_LOCK_CENTER:
			if(!tData->GetBool(SYM_SYMMETRY_IS_SET) || !tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

void CDSymmetryTag::CheckTagReg(BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdmnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdmnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdmnr.FindFirst("-",&pos);
	while(h)
	{
		cdmnr.Delete(pos,1);
		h = cdmnr.FindFirst("-",&pos);
	}
	cdmnr.ToUpper();
	kb = cdmnr.SubStr(pK,2);
	
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
			
			tData->SetBool(SYM_USE_SYMMETRY+T_LST,tData->GetBool(SYM_USE_SYMMETRY));
			tData->SetReal(SYM_TOLERANCE+T_LST,tData->GetReal(SYM_TOLERANCE));
			tData->SetLong(SYM_SYMMETRY_AXIS+T_LST,tData->GetLong(SYM_SYMMETRY_AXIS));
			
			tData->SetBool(SYM_LOCK_CENTER+T_LST,tData->GetBool(SYM_LOCK_CENTER));
			tData->SetBool(SYM_RESTRICT_SYM+T_LST,tData->GetBool(SYM_RESTRICT_SYM));
			tData->SetLong(SYM_RESTRICT_AXIS+T_LST,tData->GetLong(SYM_RESTRICT_AXIS));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSymmetryTag::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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

		tData->SetBool(SYM_USE_SYMMETRY,tData->GetBool(SYM_USE_SYMMETRY+T_LST));
		tData->SetReal(SYM_TOLERANCE,tData->GetReal(SYM_TOLERANCE+T_LST));
		tData->SetLong(SYM_SYMMETRY_AXIS,tData->GetLong(SYM_SYMMETRY_AXIS+T_LST));
		
		tData->SetBool(SYM_LOCK_CENTER,tData->GetBool(SYM_LOCK_CENTER+T_LST));
		tData->SetBool(SYM_RESTRICT_SYM,tData->GetBool(SYM_RESTRICT_SYM+T_LST));
		tData->SetLong(SYM_RESTRICT_AXIS,tData->GetLong(SYM_RESTRICT_AXIS+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
		
		Bool tMov = false;
		if(op != tData->GetObjectLink(SYM_OP_ID,doc))
		{
			BaseObject *opId = tData->GetObjectLink(SYM_OP_ID,doc);
			if(opId)
			{
				if(opId->GetDocument())
				{
					tMov = true;
				}
			}
			if(!tMov)  tData->SetLink(SYM_OP_ID,op);
			else
			{
				tData->SetBool(SYM_SYMMETRY_IS_SET,false);
				 enable = false;
			}
		}
	}
	
	return enable;
}

Bool RegisterCDSymmetryTag(void)
{
	if(CDFindPlugin(ID_CDSYMMETRYTAG,CD_TAG_PLUGIN)) return true;
	
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDM_SERIAL_SIZE];
	String cdmnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDMORPH,data,CDM_SERIAL_SIZE)) reg = false;
	
	cdmnr.SetCString(data,CDM_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdmnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdmnr.FindFirst("-",&pos);
	while(h)
	{
		cdmnr.Delete(pos,1);
		h = cdmnr.FindFirst("-",&pos);
	}
	cdmnr.ToUpper();
	kb = cdmnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDSYMTAG); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSYMMETRYTAG,name,info,CDSymmetryTag::Alloc,"tCDSymTag","CDSymTag.tif",0);
}

// library functions
LONG* iCDSymGetMirrorPoints(BaseTag *tag)
{
	CDSymmetryTag *sTag = static_cast<CDSymmetryTag*>(tag->GetNodeData());
	return sTag ? sTag->GetMirrorPoints() : NULL;
}

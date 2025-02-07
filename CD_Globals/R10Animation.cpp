// R10 animation functions

#include "lib_description.h"
#include "ckvalue.h"
#include "ctbase.h"
#include "obaselist.h"

#include "CDCompatibility.h"
#include "R10Animation.h"


// R10 track functions
Bool CDScaleTrack(BaseDocument *doc, BaseList2D *op, DescID dscID, Real sca)
{
	if(!op) return false;
	
	Real fps = 30;
	if(doc) fps = doc->GetFps();

	CTrack *track = op->FindCTrack(dscID);
	if(track)
	{
		CCurve *crv = track->GetCurve();
		if(crv)
		{
			LONG i, kCnt = crv->GetKeyCount();
			if(kCnt > 0)
			{
				if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,op);
				for(i=0; i<kCnt; i++)
				{
					CKey *key = crv->GetKey(i);
					if(key)
					{
						Real val = key->GetValue() * sca;
						
					#if API_VERSION < 12000
						Real lT = ((key->GetTimeLeft().GetNominator()/key->GetTimeLeft().GetDenominator()) * fps) * sca;
						Real rT = ((key->GetTimeRight().GetNominator()/key->GetTimeRight().GetDenominator()) * fps) * sca;
					#else
						Real lT = ((key->GetTimeLeft().GetNumerator()/key->GetTimeLeft().GetDenominator()) * fps) * sca;
						Real rT = ((key->GetTimeRight().GetNumerator()/key->GetTimeRight().GetDenominator()) * fps) * sca;
					#endif
						BaseTime tLt = BaseTime(lT,fps);
						BaseTime tRt = BaseTime(rT,fps);
						
						Real valLt = key->GetValueLeft() * sca;
						Real valRt = key->GetValueRight() * sca;

						key->SetValue(crv,val);
						key->SetTimeLeft(crv,tLt);
						key->SetTimeRight(crv,tRt);
						key->SetValueLeft(crv,valLt);
						key->SetValueRight(crv,valRt);
					}
				}
			}
		}
	}
	
	return true;
}

Bool CDHasAnimationTrack(BaseList2D *op, DescID dscID)
{
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			CCurve *crv = track->GetCurve();
			if(crv)
			{
				if(crv->GetKeyCount() > 0) return true;
			}
		}
	}
		
	return false;
}

Bool CDDeleteAnimationTrack(BaseDocument *doc, BaseList2D *op, DescID dscID)
{
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			if(doc) CDAddUndo(doc,CD_UNDO_DELETE,op);
			CTrack::Free(track);
			return true;
		}
	}
	
	return false;
}

CTrack* CDGetLastTrack(BaseList2D *op)
{
	if(!op) return NULL;
	
	CTrack *trk = op->GetFirstCTrack();
	while(trk)
	{
		if(!trk->GetNext()) break;
		trk = trk->GetNext();
	}
	
	return trk;
}

Bool CDDeleteAllAnimationTracks(BaseDocument *doc, BaseList2D *op)
{
	if(!op) return false;
	
	CTrack *trk = CDGetLastTrack(op);
	while(trk)
	{
		CTrack *pred = trk->GetPred();
		if(doc) CDAddUndo(doc,CD_UNDO_DELETE,trk);
		CTrack::Free(trk);
		trk = pred;
	}
	
	return true;
}

Bool CDCopyAnimationTrack(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, DescID dscID, BaseList2D *prvTrk)
{
	if(!src || !dst) return false;
	
	CTrack *srcTrk = src->FindCTrack(dscID); if(!srcTrk) return false;
	CTrack *dstTrk = dst->FindCTrack(dscID);
	if(dstTrk)
	{
		if(doc) CDAddUndo(doc,CD_UNDO_DELETE,dstTrk);
		CTrack::Free(dstTrk);
	}
	
	CTrack *clnTrk = (CTrack*)CDGetClone(srcTrk,CD_COPYFLAGS_0,NULL);
	if(clnTrk)
	{
		dst->InsertTrackSorted(clnTrk);
		if(doc) CDAddUndo(doc,CD_UNDO_NEW,clnTrk);
	}
	
	return true;
}

Bool CDCopyTrackAttributes(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, DescID dscID)
{
	if(!src || !dst) return false;
	
	CTrack *srcTrk = src->FindCTrack(dscID); if(!srcTrk) return false;
	CTrack *dstTrk = dst->FindCTrack(dscID); if(!dstTrk) return false;
	
	GeData d;

	CDGetParameter(srcTrk,DescLevel(ID_LAYER_LINK),d);
	CDSetParameter(dstTrk,DescLevel(ID_LAYER_LINK),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_BASELIST_NAME),d);
	CDSetParameter(dstTrk,DescLevel(ID_BASELIST_NAME),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_ANIMOFF),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_ANIMOFF),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_ANIMSOLO),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_ANIMSOLO),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_FCURVE_COLOR),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_FCURVE_COLOR),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_FCURVE_OFFSET),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_FCURVE_OFFSET),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_FCURVE_SCALE),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_FCURVE_SCALE),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_BEFORE),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_BEFORE),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_AFTER),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_AFTER),d);
	
	CDGetParameter(srcTrk,DescLevel(ID_CTRACK_TIME),d);
	CDSetParameter(dstTrk,DescLevel(ID_CTRACK_TIME),d);
	
	return true;
}

Bool CDHasLoopBefore(BaseObject *op, DescID dscID)
{
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			if(track->GetBefore() == CLOOP_REPEAT) return true;
			if(track->GetBefore() == CLOOP_OFFSETREPEAT) return true;
			if(track->GetBefore() == CLOOP_OSCILLATE) return true;
		}
	}
	
	return false;
}

Bool CDHasLoopAfter(BaseObject *op, DescID dscID)
{
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			if(track->GetAfter() == CLOOP_REPEAT) return true;
			if(track->GetAfter() == CLOOP_OFFSETREPEAT) return true;
			if(track->GetAfter() == CLOOP_OSCILLATE) return true;
		}
	}
	
	return false;
}

Bool CDIsLoopBeforeOscillate(BaseObject *op, DescID dscID)
{
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			if(track->GetBefore() == CLOOP_OSCILLATE) return true;
		}
	}
	
	return false;
}

Bool CDIsLoopAfterOscillate(BaseObject *op, DescID dscID)
{
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			if(track->GetAfter() == CLOOP_OSCILLATE) return true;
		}
	}
	
	return false;
}


// R10 key functions
Bool CDSetKey(BaseDocument *doc, BaseList2D *op, const BaseTime &time, DescID dscID, Real value, LONG intrp)
{
	if(!op) return false;
	
	//GePrint("Key interpolation = "+CDLongToString(intrp));
	
	CTrack *track = op->FindCTrack(dscID);
	if (!track)
	{
		track = CTrack::Alloc(op,dscID); if (!track) return false;
		op->InsertTrackSorted(track);
		if(doc) CDAddUndo(doc,CD_UNDO_NEW,track);
	}
	
	CCurve *crv = track->GetCurve(); if(!crv) return false;
	
	CKey *key = crv->AddKey(time); if (!key) return false;
	if(doc) CDAddUndo(doc,CD_UNDO_NEW,key);
	
#if API_VERSION < 12000
	key->SetInterpolation(crv,(CInterpolation)intrp);
#else
	key->SetInterpolation(crv,(CINTERPOLATION)intrp);
#endif

	key->SetValue(crv,value);
	
	if(intrp == CD_SPLINE_INTERPOLATION)
	{
		LONG kInd;
		crv->FindKey(time,&kInd);
		
		Real vl, vr;
		BaseTime tl, tr;
		crv->CalcSoftTangents(kInd,&vl,&vr,&tl,&tr);
		
		// set Auto tangents
		key->SetTimeLeft(crv,tl);
		key->SetTimeRight(crv,tr);
		key->SetValueLeft(crv,vl);
		key->SetValueRight(crv,vr);
		
		//GePrint("Frame right = "+CDRealToString(key->GetTimeRight().GetFrame(doc->GetFps())));
	}

	GeData d = GeData(true);
	CDSetParameter(key,DescLevel(ID_CKEY_AUTO),d);
	
	return true;
}

LONG CDGetNearestKey(BaseList2D *op, DescID dscID, LONG curFrm, LONG fps)
{
	LONG ind = 0;
	
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			CCurve *crv = track->GetCurve();
			if(crv)
			{
				if(crv->GetKeyCount() > 0)
				{
					CKey *key = crv->GetKey(kInd);
					if(key)
					{
						LONG keyFrm = key->GetTime().GetFrame(fps);
						while(key && keyFrm <= curFrm)
						{
							if(kInd > ind) ind = kInd;
							key = crv->GetKey(kInd+1);
							if(key)
							{
								kInd++;
								keyFrm = key->GetTime().GetFrame(fps);
							}
						}
					}
				}
			}
		}
	}
	
	return ind;
}

LONG CDGetNextKeyFrame(BaseList2D *op, DescID dscID, LONG curFrm, LONG fps)
{
	LONG frm = CDMAXLONG, ind = 0;
	
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			CCurve *crv = track->GetCurve();
			if(crv)
			{
				if(crv->GetKeyCount() > 0)
				{
					CKey *key = crv->GetKey(kInd);
					if(key)
					{
						LONG keyFrm = key->GetTime().GetFrame(fps);
						while(key && keyFrm <= curFrm)
						{
							if(kInd > ind) ind = kInd;
							key = crv->GetKey(kInd+1);
							if(key)
							{
								kInd++;
								keyFrm = key->GetTime().GetFrame(fps);
							}
						}
						if(key) frm = keyFrm;
					}
				}
			}
		}
	}
	
	return frm;
}

LONG CDGetTrackKeyCount(BaseList2D *op, DescID dscID)
{
	LONG cnt = 0;
	
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			CCurve *crv = track->GetCurve();
			if(crv) cnt = crv->GetKeyCount();
		}
	}
	
	return cnt;
}

LONG CDGetKeyFrame(BaseList2D *op, DescID dscID, LONG ind, LONG fps)
{
	LONG frm = 0;
	
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			CCurve *crv = track->GetCurve();
			if(crv)
			{
				CKey *key = crv->GetKey(ind);
				if(key) frm = key->GetTime().GetFrame(fps);
			}
		}
	}
	
	return frm;
}

Real CDGetKeyValue(BaseList2D *op, DescID dscID, LONG ind)
{
	Real val = 0;
	
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			CCurve *crv = track->GetCurve();
			if(crv)
			{
				CKey *key = crv->GetKey(ind);
				if(key) val = key->GetValue();
			}
		}
	}
	
	return val;
}

Real CDGetCurveValue(BaseList2D *op, DescID dscID, BaseTime time, LONG fps)
{
	Real val = 0.0;
	
	CTrack *track = op->FindCTrack(dscID);
	if(track)
	{
		CCurve *crv = track->GetCurve();
		if(crv) val = crv->GetValue(time,fps);
	}
	
	return val;
}

void CDSetTrackZeroAngle(BaseList2D *op, DescID dscID)
{
	if(op)
	{
		CTrack *track = op->FindCTrack(dscID);
		if(track)
		{
			CCurve *crv = track->GetCurve();
			if(crv)
			{
				LONG i, kCnt = crv->GetKeyCount();
				if(kCnt > 0)
				{
					for(i=0; i<kCnt; i++)
					{
						CKey *key = crv->GetKey(i);
						if(key)
						{
						#if API_VERSION < 12000
							key->ChangeNBit(NBIT_CKEY_AUTO,NBIT_DEL);
							key->ChangeNBit(NBIT_CKEY_ZERO_O,NBIT_SET);
						#else
							key->ChangeNBit(NBIT_CKEY_AUTO,NBITCONTROL_CLEAR);
							#if API_VERSION < 13000
								key->ChangeNBit(NBIT_CKEY_ZERO_O,NBITCONTROL_SET);
							#else
								key->ChangeNBit(NBIT_CKEY_ZERO_O_OLD,NBITCONTROL_SET);
							#endif
						#endif
						}
					}
				}
			}
		}
	}
}

LONG CDGetKeyInterpolation(BaseList2D *op, DescID dscID, LONG ind)
{
	LONG intrp = CD_SPLINE_INTERPOLATION;
	
	CTrack *track = op->FindCTrack(dscID);
	if(track)
	{
		CCurve *crv = track->GetCurve();
		if(crv)
		{
			CKey *key = crv->GetKey(ind);
			if(key)
			{
			#if API_VERSION < 12000
				intrp = key->GetInterpolation();
			#else
				intrp = (LONG)key->GetInterpolation();
			#endif
			}
		}
	}
	
	return intrp;
}

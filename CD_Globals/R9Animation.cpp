// R9 functions

#include "CDCompatibility.h"
#include "R9Animation.h"

// R9 track functions
Bool CDScaleTrack(BaseDocument *doc, BaseList2D *op, DescID dscID, Real sca)
{
	if(!op) return FALSE;

	BaseTrack *track = op->FindTrack(dscID);
	if(track)
	{
		BaseSequence *seq = NULL;
		for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
		{
			LONG i, kCnt = seq->GetKeyCount();
			if(kCnt > 0)
			{
				if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,op);
				BaseKey *key = seq->GetFirstKey();
				for(i=0; i<kCnt; i++)
				{
					if(!key) break;
					
					AnimValue *av = GetKeyValue(key);
					if(av)
					{
						Real val = av->value * sca;
						av->value = val;
					}
					key = key->GetNext();
				}
			}
		}
	}
		
	return TRUE;
}

Bool CDHasAnimationTrack(BaseList2D *op, DescID dscID)
{
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				if(seq->GetKeyCount() > 0) return TRUE;
			}
		}
	}
	
	return FALSE;
}

Bool CDDeleteAnimationTrack(BaseDocument *doc, BaseList2D *op, DescID dscID)
{
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			if(doc) CDAddUndo(doc,CD_UNDO_DELETE,op);
			BaseTrack::Free(track);
			return TRUE;
		}
	}
	
	return FALSE;
}

BaseTrack* CDGetLastTrack(BaseList2D *op)
{
	if(!op) return NULL;
	
	BaseTrack *trk = op->GetFirstTrack();
	while(trk)
	{
		if(!trk->GetNext()) break;
		trk = trk->GetNext();
	}
	
	return trk;
}

Bool CDDeleteAllAnimationTracks(BaseDocument *doc, BaseList2D *op)
{
	if(op) return FALSE;
	
	BaseTrack *trk = CDGetLastTrack(op);
	while(trk)
	{
		BaseTrack *pred = trk->GetPred();
		if(doc) CDAddUndo(doc,CD_UNDO_DELETE,trk);
		BaseTrack::Free(trk);
		trk = pred;
	}
	
	return TRUE;
}

Bool CDCopyAnimationTrack(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, DescID dscID, BaseList2D *prvTrk)
{
	if(!src || !dst) return FALSE;
	
	BaseTrack *srcTrk = src->FindTrack(dscID); if(!srcTrk) return FALSE;
	BaseTrack *dstTrk = dst->FindTrack(dscID);
	if(dstTrk)
	{
		if(doc) CDAddUndo(doc,CD_UNDO_DELETE,dstTrk);
		BaseTrack::Free(dstTrk);
	}
	
	BaseTrack *clnTrk = (BaseTrack*)srcTrk->GetClone(0,NULL);
	if(clnTrk)
	{
		dst->InsertTrack(clnTrk,(BaseTrack*)prvTrk);
		if(doc) CDAddUndo(doc,CD_UNDO_NEW,clnTrk);
		prvTrk = clnTrk;
	}
	
	return TRUE;
}

Bool CDCopySequenceParameters(BaseSequence *srcSeq, BaseSequence *dstSeq)
{
	if(!srcSeq || !dstSeq) return FALSE;

	GeData d;
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_NAME),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_NAME),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_T1),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_T1),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_T2),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_T2),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_T3),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_T3),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_LOOPS),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_LOOPS),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_SOFT),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_SOFT),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_LEFTI),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_LEFTI),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_RIGHTI),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_RIGHTI),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_TIME),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_TIME),d);
	
	CDGetParameter(srcSeq,DescLevel(ID_BASESEQ_TIME_EXLOOP),d);
	CDSetParameter(dstSeq,DescLevel(ID_BASESEQ_TIME_EXLOOP),d);
	
	return TRUE;
}

Bool CDCopyTrackAttributes(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, DescID dscID)
{
	if(!src || !dst) return FALSE;
	
	BaseTrack *srcTrk = src->FindTrack(dscID); if(!srcTrk) return FALSE;
	BaseTrack *dstTrk = dst->FindTrack(dscID); if(!dstTrk) return FALSE;
	
	BaseSequence *srcSeq = srcTrk->GetFirstSequence(); if(!srcSeq) return FALSE;
	BaseSequence *dstSeq = dstTrk->GetFirstSequence(); if(!dstSeq) return FALSE;
	
	if(!CDCopySequenceParameters(srcSeq,dstSeq)) return FALSE;
	
	srcSeq = srcSeq->GetNext();
	dstSeq = dstSeq->GetNext();
	while(srcSeq && dstSeq)
	{
		if(!CDCopySequenceParameters(srcSeq,dstSeq)) return FALSE;

		srcSeq = srcSeq->GetNext();
		dstSeq = dstSeq->GetNext();
	}
	
	return TRUE;
}

Bool CDHasLoopAfter(BaseObject *op, DescID dscID)
{
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				GeData d;
				CDGetParameter(seq,DescLevel(ID_BASESEQ_LOOPS),d);
				if(d.GetReal() > 0) return TRUE;
			}
		}
	}
	
	return FALSE;
}

// R9 key functions
Bool CDSetKey(BaseDocument *doc, BaseList2D *op, const BaseTime &time, DescID dscID, Real value, LONG intrp)
{
	if(!op || !doc) return FALSE;
	
	// check if track exists
	BaseTrack *track = op->FindTrack(dscID);
	if(!track)
	{
		track = AllocValueTrack(op,dscID); if(!track) return FALSE;
		op->InsertTrack(track,NULL);
		CDAddUndo(doc,CD_UNDO_NEW,track);
	}
	
	// check for sequence
	BaseSequence *seq = NULL;
	for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
	{
		if(time >= seq->GetT1() && time <= seq->GetT2()) break;
	}
	if(!seq)
	{
		seq = track->AutoAddSequence(doc,time); 
		CDAddUndo(doc,CD_UNDO_NEW,seq);	
		if(!seq) return FALSE;
	}
	
	BaseKey *key = BaseKey::Alloc(seq->GetType()); if(!key) return FALSE;
	AnimValue *av = GetKeyValue(key); if(!av) return FALSE;
	
	av->value = value;
	key->SetTime(time);
	seq->InsertKey(key);
	CDAddUndo(doc,CD_UNDO_NEW,key);
	
	return TRUE;
}

LONG CDGetNearestKey(BaseList2D *op, DescID dscID, LONG curFrm, LONG fps)
{
	LONG ind = 0;
	
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				if(seq->GetKeyCount() > 0)
				{
					BaseKey *key = seq->GetFirstKey();
					if(key)
					{
						LONG keyFrm = key->GetTime().GetFrame(fps);
						while(key && keyFrm <= curFrm)
						{
							if(kInd > ind) ind = kInd;
							key = key->GetNext();
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
	LONG frm = MAXLONG, ind = 0;
	
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				if(seq->GetKeyCount() > 0)
				{
					BaseKey *key = seq->GetFirstKey();
					if(key)
					{
						LONG keyFrm = key->GetTime().GetFrame(fps);
						while(key && keyFrm <= curFrm)
						{
							if(kInd > ind) ind = kInd;
							key = key->GetNext();
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
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				cnt += seq->GetKeyCount();
			}
		}
	}
	
	return cnt;
}

LONG CDGetKeyFrame(BaseList2D *op, DescID dscID, LONG ind, LONG fps)
{
	LONG frm = 0;
	
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				if(seq->GetKeyCount() > 0)
				{
					BaseKey *key = seq->GetFirstKey();
					if(key)
					{
						while(key && kInd <= ind)
						{
							if(kInd == ind) frm = key->GetTime().GetFrame(fps);
							key = key->GetNext();
							if(key) kInd++;
						}
					}
				}
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
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				if(seq->GetKeyCount() > 0)
				{
					BaseKey *key = seq->GetFirstKey();
					if(key)
					{
						while(key && kInd <= ind)
						{
							if(kInd == ind)
							{
								AnimValue *av = GetKeyValue(key);
								if(av) val = av->value;
							}
							key = key->GetNext();
							if(key) kInd++;
						}
					}
				}
			}
		}
	}
	
	return val;
}

void CDSetTrackZeroAngle(BaseList2D *op, DescID dscID)
{
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				if(seq->GetKeyCount() > 0)
				{
					BaseKey *key = seq->GetFirstKey();
					while(key)
					{
						AnimValue *av = GetKeyValue(key);
						if(av)
						{
							av->inter_left = 0;
							av->inter_right = 0;
							
							av->time_left = -0.25;
							av->time_right = 0.25;
							
							av->value_left = 0.0;
							av->value_right = 0.0;
						}
						kInd++;
						key = key->GetNext();
					}
				}
			}
		}
	}
}

LONG CDGetKeyInterpolation(BaseList2D *op, DescID dscID, LONG ind)
{
	LONG intrp = CD_SPLINE_INTERPOLATION;
	
	if(op)
	{
		BaseTrack *track = op->FindTrack(dscID);
		if(track)
		{
			LONG kInd = 0;
			BaseSequence *seq = NULL;
			for(seq = track->GetFirstSequence(); seq; seq=seq->GetNext())
			{
				if(seq->GetKeyCount() > 0)
				{
					BaseKey *key = seq->GetFirstKey();
					if(key)
					{
						while(key && kInd <= ind)
						{
							if(kInd == ind)
							{
								AnimValue *av = GetKeyValue(key);
								if(av)
								{
									LONG rtIntrp = av->inter_right;
									
									BaseKey *nxtKey = key->GetNext();
									if(nxtKey)
									{
										AnimValue *nav = GetKeyValue(nxtKey);
										if(nav)
										{
											LONG ltIntrp = nav->inter_left;
											
											if(ltIntrp == 2 && rtIntrp == 2)
												intrp = CD_LINEAR_INTERPOLATION;
											else if(ltIntrp == 3 && rtIntrp == 3)
												intrp = CD_STEP_INTERPOLATION;
										}
									}
								}
							}
							key = key->GetNext();
							if(key) kInd++;
						}
					}
				}
			}
		}
	}
	
	return intrp;
}

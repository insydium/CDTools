#ifndef _R10Animation_H_
#define _R10Animation_H_

#include "c4d.h"

#include "CDCompatibility.h"

enum
{
	CD_SPLINE_INTERPOLATION	= 1,
	CD_LINEAR_INTERPOLATION = 2,
	CD_STEP_INTERPOLATION	= 3
};



// R10 track functions
Bool CDScaleTrack(BaseDocument *doc, BaseList2D *op, DescID dscID, Real sca);
Bool CDHasAnimationTrack(BaseList2D *op, DescID dscID);
Bool CDDeleteAnimationTrack(BaseDocument *doc, BaseList2D *op, DescID dscID);
CTrack* CDGetLastTrack(BaseList2D *op);
Bool CDDeleteAllAnimationTracks(BaseDocument *doc, BaseList2D *op);
Bool CDCopyAnimationTrack(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, DescID dscID, BaseList2D *prvTrk);
Bool CDCopyTrackAttributes(BaseDocument *doc, BaseList2D *src, BaseList2D *dst, DescID dscID);
Bool CDHasLoopBefore(BaseObject *op, DescID dscID);
Bool CDHasLoopAfter(BaseObject *op, DescID dscID);
Bool CDIsLoopBeforeOscillate(BaseObject *op, DescID dscID);
Bool CDIsLoopAfterOscillate(BaseObject *op, DescID dscID);

// R10 key functions
Bool CDSetKey(BaseDocument *doc, BaseList2D *op, const BaseTime &time, DescID dscID, Real value, LONG intrp = CD_SPLINE_INTERPOLATION);
LONG CDGetNearestKey(BaseList2D *op, DescID dscID, LONG curFrm, LONG fps);
LONG CDGetNextKeyFrame(BaseList2D *op, DescID dscID, LONG curFrm, LONG fps);
LONG CDGetTrackKeyCount(BaseList2D *op, DescID dscID);
LONG CDGetKeyFrame(BaseList2D *op, DescID dscID, LONG ind, LONG fps);
Real CDGetKeyValue(BaseList2D *op, DescID dscID, LONG ind);
Real CDGetCurveValue(BaseList2D *op, DescID dscID, BaseTime time, LONG fps);
void CDSetTrackZeroAngle(BaseList2D *op, DescID dscID);
LONG CDGetKeyInterpolation(BaseList2D *op, DescID dscID, LONG ind);

#endif
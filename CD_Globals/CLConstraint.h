//  CLConstraint.h

#ifndef _CLCONSTRAINT_H_
#define _CLCONSTRAINT_H_

#include "CDArray.h"
#include "CLParticle.h"

enum
{
	CD_EDGE_CONSTRAINT = 0,
	CD_BEND_CONSTRAINT,
	CD_SHEAR_CONSTRAINT
};

class CLConstraint
{
public:
	LONG	aInd, bInd, type;
	Real	rest_distance;
	//Vector	ref;
	
	CLConstraint()
	{
		aInd = -1;
		bInd = -1;
		type = 0;
		rest_distance = 0.0;
	}
	~CLConstraint() {}
	
	CLConstraint(CLParticle *prt, LONG a, LONG b, LONG t)
	{
		aInd = a;
		bInd = b;
		type = t;
		//ref = prt[a].pos;
		Vector v = prt[a].pos - prt[b].pos;
		rest_distance = Len(v);
	}
	
	CLConstraint(const CLConstraint &c)
	{	
		aInd = c.aInd;
		bInd = c.bInd;
		type = c.type;
		rest_distance = c.rest_distance;
	}
	
	void SatisfyConstraint(CLParticle *prt)
	{
		Real wtA = prt[aInd].wt, wtB = prt[bInd].wt;
		if(wtA > 0.0 || wtB > 0.0)
		{
			Vector delta = prt[bInd].pos - prt[aInd].pos;
			Real deltaLength = Len(delta);
			if(deltaLength < 0.001) deltaLength = 0.001;
				
			Vector correctionVector = delta * (1 - rest_distance/deltaLength);
			if(wtA > 0.0 && wtB > 0.0)
			{
				if(wtA < 0.99 && wtB < 0.99)
				{	
					//prt[aInd].OffsetPosition(correctionVector * (1.0-wtB) * wtA);
					//prt[bInd].OffsetPosition(-correctionVector * (1.0-wtA) * wtB);
					prt[aInd].OffsetPosition(correctionVector * 0.5 * wtA);
					prt[bInd].OffsetPosition(-correctionVector * 0.5 * wtB);
				}
				else
				{
					prt[aInd].OffsetPosition(correctionVector * 0.5);
					prt[bInd].OffsetPosition(-correctionVector * 0.5);
				}
			}
			else
			{
				if(wtA == 0.0 && wtB > 0.0)
				{
					//prt[bInd].OffsetPosition(-correctionVector * wtB);// * (1.0-wtA)
					prt[bInd].OffsetPosition(-correctionVector);
				}
				else if(wtB == 0.0 && wtA > 0.0)
				{
					//prt[aInd].OffsetPosition(correctionVector * wtA);// * (1.0-wtB)
					prt[aInd].OffsetPosition(correctionVector);
				}
			}
		}
	}
	
	LONG GetType(void) { return type; }
	//void UpdateRef(Vector v) { ref = v;}
};


class CLConstraintArray : public CDArray<CLConstraint>
{
public:
	CLConstraintArray() {}
	~CLConstraintArray() {}
	
	LONG GetIndex(LONG a, LONG b)
	{
		if(a < 0 || b < 0) return -1;
		
		for(LONG i=0; i<data.size(); i++)
		{
			if(a == data[i].aInd && b == data[i].bInd) return i;
		}
		
		return -1;
	}
};


#endif
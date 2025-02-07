#ifndef _CDEdgeCutArray_H_
#define _CDEdgeCutArray_H_

// c++ includes

// c4d includes
#include "c4d.h"

// CD includes
#include "CDArray.h"

struct CutEdge
{
	LONG	a;
	LONG	b;
	Real	c;
	LONG	vInd;
};

class CDEdgeCutArray : public CDArray<CutEdge>
{
public:
	CDEdgeCutArray() {}
	~CDEdgeCutArray() {}
	
	LONG GetCutPtIndex(CutEdge e)
	{
		LONG i;
		for(i=0; i<data.size(); i++)
		{
			if(e.a == data[i].a && e.b == data[i].b) return data[i].vInd;
		}
		
		return 0;
	}
};

#endif

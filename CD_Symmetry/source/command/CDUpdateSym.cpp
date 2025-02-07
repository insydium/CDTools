//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"
#include "toolbridge.h"

//#include "CDSymTag.h"
#include "oCDDynSym.h"
#include "CDSymmetry.h"

class CDUpdateSymmetry : public CommandData
{
	private:
		Bool ComponentCountEqual(BaseObject *op, BaseObject *cln);
		
	public:
		virtual Bool Execute(BaseDocument *doc);
};

Bool CDUpdateSymmetry::ComponentCountEqual(BaseObject *op, BaseObject *cln)
{
	LONG opCnt, clnCnt;

	opCnt = ToPoint(op)->GetPointCount();
	clnCnt = ToPoint(cln)->GetPointCount();
	if(opCnt != clnCnt) return false;
	
	opCnt = ToPoly(op)->GetPolygonCount();
	clnCnt = ToPoly(cln)->GetPolygonCount();
	if(opCnt != clnCnt) return false;

	return true;	
}

Bool CDUpdateSymmetry::Execute(BaseDocument *doc)
{
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG  i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				if(IsValidPointObject(op) && !op->GetDown())
				{
					BaseObject *pr = op->GetUp();
					if(pr)
					{
						if(pr->GetType() == ID_CDDYNSYMMETRY)
						{
							BaseContainer *oData = pr->GetDataInstance();
							if(oData)
							{
								if(pr->GetCache())
								{
									CacheToMesh(doc,pr,op);
								}
							}
						}
					}
				}
			}
		}
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

Bool RegisterCDUpdateSymmetry(void)
{
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDUPDTSYM); if (!name.Content()) return true;
	return CDRegisterCommandPlugin(ID_CDSYMMETRYUPDATE,name,PLUGINFLAG_HIDEPLUGINMENU,"","CD Update Symmetry",CDDataAllocator(CDUpdateSymmetry));
}

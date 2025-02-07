//	Cactus Dan's Select Parent 1.0 plugin
//	Copyright 2013 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"

class SelectParentCommand : public CommandData
{
public:
	virtual Bool Execute(BaseDocument *doc);
};

Bool SelectParentCommand::Execute(BaseDocument *doc)
{
	BaseObject *op = doc->GetActiveObject();
	if(op)
	{
		BaseObject *pr = op->GetUp();
		if(pr)
		{
			doc->SetActiveObject(pr);
			CallCommand(100004769);
			EventAdd(EVENT_FORCEREDRAW);
		}
	}
	return true;
}

#define ID_SELECTPARENT	1030689

Bool RegisterSelectParentCommand(void)
{
	String name = GeLoadString(IDS_SELECTPRNT); if (!name.Content()) return true;
	return CDRegisterCommandPlugin(ID_SELECTPARENT,name,0,"SelectParent.tif","Select Parent",CDDataAllocator(SelectParentCommand));
}

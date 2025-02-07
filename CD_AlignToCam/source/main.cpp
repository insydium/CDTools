//	Cactus Dan's Align to Camera 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

// Starts the plugin registration

#include "c4d.h"
#include "CDAlignCam.h"
#include "CDGeneral.h"

// forward declarations
Bool RegisterCDAlignToCamera(void);
Bool RegisterCDAlignToCamMessagePlugin(void);

static AtomArray *atcTagList = NULL;

Bool PluginStart(void)
{
	String pName = "CD Align to Camera";
	if(!CDCheckC4DVersion(pName)) return false;
	
	// Allocate the lists
	atcTagList = AtomArray::Alloc();
	
	// message plugins
	if (!RegisterCDAlignToCamMessagePlugin()) return false;
	
	// tag plugins
	if (!RegisterCDAlignToCamera()) return false;

	GePrint(pName+" v1.003");
	return true;
}

void PluginEnd(void)
{
	AtomArray::Free(atcTagList);
}

Bool PluginMessage(LONG id, void *data)
{
	CDListData *atcd;
	
	//use the following lines to set a plugin priority
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return false; // don't start plugin without resource
			return true;

		case C4DMSG_PRIORITY: 
			return true;

		case ID_CDALIGNTOCAMERA:
			atcd = (CDListData *)data;
			atcd->list = atcTagList;
			return true;
	}

	return false;
}

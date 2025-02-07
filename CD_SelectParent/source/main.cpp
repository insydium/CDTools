//	Cactus Dan's Select Parent 1.0 plugin
//	Copyright 2013 by Cactus Dan Libisch

// Starts the plugin registration

#include "c4d.h"

#include "CDGeneral.h"

// forward declarations
Bool RegisterSelectParentCommand(void);

Bool PluginStart(void)
{
	String pName = "Select Parent";
	if(!CDCheckC4DVersion(pName)) return false;

	// tag plugins
	if (!RegisterSelectParentCommand()) return false;

	GePrint("Select Parent v1.0");
	return true;
}

void PluginEnd(void)
{
}

Bool PluginMessage(LONG id, void *data)
{
	//use the following lines to set a plugin priority
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return false; // don't start plugin without resource
			return true;

		case C4DMSG_PRIORITY: 
			return true;

	}

	return false;
}

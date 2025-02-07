//	Cactus Dan's HPB View 1.0 plugin
//	Copyright 2010 by Cactus Dan Libisch

// Starts the plugin registration

#include "c4d.h"
#include "CDGeneral.h"

// forward declarations
Bool RegisterCDHPBView(void);

Bool PluginStart(void)
{
	String pName = "CD HPB View";
	if(!CDCheckC4DVersion(pName)) return false;
	
	// tag plugins
	if (!RegisterCDHPBView()) return false;

	GePrint(pName+" v1.002");
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

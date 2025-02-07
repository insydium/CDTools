//	Cactus Dan's Position Tracks Delete 1.0 plugin
//	Copyright 2011 by Cactus Dan Libisch

// Starts the plugin registration

#include "c4d.h"

#include "CDGeneral.h"

// forward declarations
Bool RegisterCDPTracksXCommand(void);
Bool RegisterCDSTracksXCommand(void);

Bool PluginStart(void)
{
	String pName = "Position Tracks Delete";
	if(!CDCheckC4DVersion(pName)) return false;

	// command plugins
	if (!RegisterCDPTracksXCommand()) return false;
	if (!RegisterCDSTracksXCommand()) return false;

	GePrint("CD P/S Tracks Delete v1.0");
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

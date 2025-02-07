#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDDebug.h"
#include "CDCloth.h"

class CDClothMessagePlugin : public MessageData
{
	public:
		virtual Bool CoreMessage(LONG id, const BaseContainer &bc);

};

Bool CDClothMessagePlugin::CoreMessage(LONG id, const BaseContainer &bc)
{
	
	BaseDocument *doc = GetActiveDocument(); if(!doc) return true;
	
	//ListReceivedCoreMessages("CDTestMessagePlugin", id, bc);
	
	switch(id)
	{
		case EVMSG_CHANGE:
			//PluginMessage(ID_CD_MESSAGEDATA_TEST,NULL);
			break;
	}
	
	return true;
}

Bool RegisterCDClothMessagePlugin(void)
{
	return RegisterMessagePlugin(ID_CDCLOTHMESSAGE,"Cloth Message",0,CDDataAllocator(CDClothMessagePlugin));
}

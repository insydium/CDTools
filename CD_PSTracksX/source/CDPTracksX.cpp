//	Cactus Dan's Position Tracks Delete 1.0 plugin
//	Copyright 2011 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

#include "CDCompatibility.h"

class CDPTracksXCommand : public CommandData
{
public:
	Bool RemoveChildPositionTracks(BaseDocument *doc, BaseObject *op);
	
	virtual Bool Execute(BaseDocument *doc);
};

Bool CDPTracksXCommand::RemoveChildPositionTracks(BaseDocument *doc, BaseObject *op)
{
	while(op)
	{
		StatusSetSpin();
		
		DescID dscID;
		
		// position track X 
		dscID = CDGetPSRTrackDescriptionID(CD_POS_X);
		CDDeleteAnimationTrack(doc,op,dscID);
		
		// position track Y
		dscID = CDGetPSRTrackDescriptionID(CD_POS_Y);
		CDDeleteAnimationTrack(doc,op,dscID);
		
		// position track Z
		dscID = CDGetPSRTrackDescriptionID(CD_POS_Z);
		CDDeleteAnimationTrack(doc,op,dscID);
		
		RemoveChildPositionTracks(doc,op->GetDown());
		
		op = op->GetNext();
	}
	
	return true;
}

Bool CDPTracksXCommand::Execute(BaseDocument *doc)
{
	BaseObject *op = doc->GetActiveObject();
	if(op)
	{
		RemoveChildPositionTracks(doc,op->GetDown());
		
		StatusClear();
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	return true;
}

#define ID_CDPOSITIONTRACKSDELETE	1027080

Bool RegisterCDPTracksXCommand(void)
{
	// decide by name if the plugin shall be registered - just for user convenience  PLUGINFLAG_HIDE|
	String name = GeLoadString(IDS_CDPTRACKX); if (!name.Content()) return true;

	return CDRegisterCommandPlugin(ID_CDPOSITIONTRACKSDELETE,name,0,"CDPTracksX.tif","CD Position Tracks Delete",CDDataAllocator(CDPTracksXCommand));
}

//	Cactus Dan's Scale Tracks Delete 1.0 plugin
//	Copyright 2011 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

#include "CDCompatibility.h"

class CDSTracksXCommand : public CommandData
{
public:
	Bool RemoveChildScaleTracks(BaseDocument *doc, BaseObject *op);
	
	virtual Bool Execute(BaseDocument *doc);
};

Bool CDSTracksXCommand::RemoveChildScaleTracks(BaseDocument *doc, BaseObject *op)
{
	while(op)
	{
		StatusSetSpin();
		
		DescID dscID;
		
		// position track X 
		dscID = CDGetPSRTrackDescriptionID(CD_SCA_X);
		CDDeleteAnimationTrack(doc,op,dscID);
		
		// position track Y
		dscID = CDGetPSRTrackDescriptionID(CD_SCA_Y);
		CDDeleteAnimationTrack(doc,op,dscID);
		
		// position track Z
		dscID = CDGetPSRTrackDescriptionID(CD_SCA_Z);
		CDDeleteAnimationTrack(doc,op,dscID);
		
		RemoveChildScaleTracks(doc,op->GetDown());
		
		op = op->GetNext();
	}
	
	return true;
}

Bool CDSTracksXCommand::Execute(BaseDocument *doc)
{
	BaseObject *op = doc->GetActiveObject();
	if(op)
	{
		RemoveChildScaleTracks(doc,op->GetDown());
		
		StatusClear();
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	return true;
}

#define ID_CDSCALETRACKSDELETE	1031327

Bool RegisterCDSTracksXCommand(void)
{
	// decide by name if the plugin shall be registered - just for user convenience  PLUGINFLAG_HIDE|
	String name = GeLoadString(IDS_CDSTRACKX); if (!name.Content()) return true;

	return CDRegisterCommandPlugin(ID_CDSCALETRACKSDELETE,name,0,"CDSTracksX.tif","CD Scale Tracks Delete",CDDataAllocator(CDSTracksXCommand));
}

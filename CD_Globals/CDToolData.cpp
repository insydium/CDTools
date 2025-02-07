/*
 *  CDToolData.cpp
 *  CDTests
 *
 *  Created by Dan Libisch on 6/9/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#include "CDToolData.h"

CDToolData::CDToolData()
{
}

CDToolData::~CDToolData()
{
}

LONG CDToolData::CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags)
{
	return flags;
}

Bool CDToolData::CDInitDisplayControl(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, const AtomArray* active)
{
	return true;
}

Bool CDToolData::CDGetDEnabling(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	return true;
}

Bool CDToolData::CDGetDDescription(BaseDocument *doc, BaseContainer &data, Description *description, LONG &flags)
{
	/*
	 CDGetDDescription must return with the following code
	 if creating descriptions programatically:
	 
	 flags |= CD_DESCFLAGS_DESC_LOADED; // "CD" description flag equivalents set
	 return CDSuperGetDDescriptionReturn(doc, data, description, flags);
	 */
	
	return true;
}

Bool CDToolData::CDGetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, GeData &t_data, LONG &flags)
{
	return true;
}

Bool CDToolData::CDSetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG &flags)
{
	return true;
}


//Parent class function call returns
Bool CDToolData::CDSuperGetDDescriptionReturn(BaseDocument *doc, BaseContainer &data, Description *description, LONG &flags)
{
#if API_VERSION < 12000
	return ToolData::GetDDescription(doc, data, description, flags);
#else
	return ToolData::GetDDescription(doc, data, description, (DESCFLAGS_DESC&)flags);
#endif
}

Bool CDToolData::CDSuperGetDEnablingReturn(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
#if API_VERSION < 12000
	GeData tdata = t_data;
	return ToolData::GetDEnabling(doc, data, id, tdata, flags, itemdesc);
#else
	return ToolData::GetDEnabling(doc, data, id, t_data, (DESCFLAGS_ENABLE)flags, itemdesc);
#endif
}

Bool CDToolData::CDSuperGetDParameterReturn(BaseDocument *doc, BaseContainer &data, const DescID &id, GeData &t_data, LONG &flags)
{
#if API_VERSION < 12000
	return ToolData::GetDParameter(doc, data, id, t_data, flags);
#else
	return ToolData::GetDParameter(doc, data, id, t_data, (DESCFLAGS_GET&)flags);
#endif
}

Bool CDToolData::CDSuperSetDParameterReturn(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG &flags)
{
#if API_VERSION < 12000
	return ToolData::SetDParameter(doc, data, id, t_data, flags);
#else
	return ToolData::SetDParameter(doc, data, id, t_data, (DESCFLAGS_SET&)flags);
#endif
}



/*
 *  CDToolData.h
 *  CDTests
 *
 *  Created by Dan Libisch on 6/9/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */
#ifndef _CDTOOLDATA_H_
#define _CDTOOLDATA_H_


#include "c4d.h"

#include "CDCompatibility.h" // header for compatibility constants and functions

class CDToolData : public ToolData
{
public:
	CDToolData();
	virtual ~CDToolData();

	virtual LONG CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags);
	virtual Bool CDInitDisplayControl(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, const AtomArray* active);
	
	virtual Bool CDGetDDescription(BaseDocument *doc, BaseContainer &data, Description *description, LONG &flags);
	virtual Bool CDGetDEnabling(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, GeData &t_data, LONG &flags);
	virtual Bool CDSetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG &flags);
	
#if API_VERSION < 12000
	virtual LONG Draw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt,LONG flags)
		{ return CDDraw(doc, data, bd, bh, bt, flags); }
	virtual Bool InitDisplayControl(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, AtomArray* active)
		{ return CDInitDisplayControl(doc, data, bd, active); }
	
	virtual Bool GetDDescription(BaseDocument *doc, BaseContainer &data, Description *description, LONG &flags)
		{ return CDGetDDescription(doc, data, description, flags); }
	virtual Bool GetDEnabling(BaseDocument *doc, BaseContainer &data, const DescID &id, GeData &t_data, LONG flags, const BaseContainer *itemdesc)
		{ return CDGetDEnabling(doc, data, id, t_data, flags, itemdesc); }
	virtual Bool GetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, GeData &t_data, LONG &flags)
		{ return CDGetDParameter(doc, data, id, t_data, flags); }
	virtual Bool SetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG &flags)
		{ return CDSetDParameter(doc, data, id, t_data, flags); }
#else
	virtual TOOLDRAW Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags)
		{ return (TOOLDRAW)CDDraw(doc, data, bd, bh, bt, (LONG)flags); }
	virtual Bool InitDisplayControl(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, const AtomArray* active)
		{ return CDInitDisplayControl(doc, data, bd, active); }
	
	virtual Bool GetDDescription(BaseDocument *doc, BaseContainer &data, Description *description, DESCFLAGS_DESC &flags)
		{ return CDGetDDescription(doc, data, description, (LONG&)flags); }
	virtual Bool GetDEnabling(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
		{ return CDGetDEnabling(doc, data, id, t_data, (LONG)flags, itemdesc); }
	virtual Bool GetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, GeData &t_data, DESCFLAGS_GET &flags)
		{ return CDGetDParameter(doc, data, id, t_data, (LONG&)flags); }
	virtual Bool SetDParameter(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, DESCFLAGS_SET &flags)
		{ return CDSetDParameter(doc, data, id, t_data, (LONG&)flags); }
#endif
	
	//Parent class function call returns
	Bool CDSuperGetDDescriptionReturn(BaseDocument *doc, BaseContainer &data, Description *description, LONG &flags);
	Bool CDSuperGetDEnablingReturn(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	Bool CDSuperGetDParameterReturn(BaseDocument *doc, BaseContainer &data, const DescID &id, GeData &t_data, LONG &flags);
	Bool CDSuperSetDParameterReturn(BaseDocument *doc, BaseContainer &data, const DescID &id, const GeData &t_data, LONG &flags);
	
};


#endif
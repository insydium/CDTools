//	Cactus Dan's HPB View 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tHpbView.h"
#include "CDGeneral.h"
//#include "CDCompatibility.h"
#include "CDTagData.h"

#define ID_CDHPBVIEW		1025477


class CDHPBView : public CDTagData
{
private:
	Bool DrawGuide(BaseDraw *bd, Matrix m);
	
public:
	virtual Bool Init(GeListNode *node);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);

	static NodeData *Alloc(void) { return CDDataAllocator(CDHPBView); }
};

Bool CDHPBView::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
		
	tData->SetBool(GUIDE_SHOW,true);
	tData->SetBool(GUIDE_SIZE,100.0);
	
	return true;
}

Bool CDHPBView::DrawGuide(BaseDraw *bd, Matrix m)
{
	Vector a = Vector(0,0,1);
	Vector b = Vector(-0.1,0,0.7);
	Vector c = Vector(-0.04,0,0.7);
	Vector d = Vector(-0.04,0,0);
	Vector e = Vector(0.04,0,0);
	Vector f = Vector(0.04,0,0.7);
	Vector g = Vector(0.1,0,0.7);


	CDDrawCircle(bd,m);
	
	CDDrawLine(bd,m*a,m*b);
	CDDrawLine(bd,m*b,m*c);
	CDDrawLine(bd,m*c,m*d);
	CDDrawLine(bd,m*d,m*e);
	CDDrawLine(bd,m*e,m*f);
	CDDrawLine(bd,m*f,m*g);
	CDDrawLine(bd,m*g,m*a);
	
	return true;
}

Bool CDHPBView::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	if(!tData->GetBool(GUIDE_SHOW)) return true;
	
	Real rad = tData->GetReal(GUIDE_SIZE);
	Vector opRot = CDGetRot(op);
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Matrix opM = op->GetMg(), prM = Matrix();
	if(op->GetUp()) prM = op->GetUp()->GetMg();
	
	Matrix zM = opM;
	zM.v1 *= rad;
	zM.v2 *= rad;
	zM.v3 *= rad;
	
	bd->SetPen(Vector(0,0,1));
	DrawGuide(bd,zM);
	
	Matrix yM = prM * MatrixRotY(opRot.x) * MatrixRotX(pi05);
	yM.off = opM.off;
	yM.v1 *= rad;
	yM.v2 *= rad;
	yM.v3 *= rad;
	
	bd->SetPen(Vector(0,1,0));
	DrawGuide(bd,yM);
	
	Matrix xM = prM * MatrixRotY(-pi05) * MatrixRotY(opRot.x) * MatrixRotZ(opRot.y) * MatrixRotZ(pi05);
	xM.off = opM.off;
	xM.v1 *= rad;
	xM.v2 *= rad;
	xM.v3 *= rad;
	
	bd->SetPen(Vector(1,0,0));
	DrawGuide(bd,xM);
	
	return true;
}

Bool RegisterCDHPBView(void)
{
	LONG info = TAG_VISIBLE;
	
	// decide by name if the plugin shall be registered - just for user convenience  TAG_EXPRESSION|
	String name=GeLoadString(IDS_CDHPBV); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDHPBVIEW,name,info,CDHPBView::Alloc,"tHpbView","tHpbView.tif",0);
}

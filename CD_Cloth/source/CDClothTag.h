//
//  CDClothTag.h
//  Created by Dan Libisch on 8/4/13.
//  Copyright 2013 Libisch Graphic Design. All rights reserved.
//

#ifndef _CDClothTag_H_
#define _CDClothTag_H_

// c4d includes
#include "c4d.h"
#include "lib_modeling.h"

// CDCL includes
#include "CDCloth.h"
#include "CDTagData.h"
#include "CLConstraint.h"
#include "CDArray.h"

#include "CDJSStructures.h"
#include "CDMStructures.h"

enum
{
	//CL_DAMPING				= 1000,
	//CL_TIME_STEP				= 1001,
	//CL_ITERATIONS				= 1002,
	//CL_STIFFNESS				= 1003,
	
	//CL_USE_GRAVITY			= 1010,
	//CL_GRAVITY				= 1011,
	
	//CL_WIND_OBJECT			= 1020,
	
	//CL_COLLISION_OBJECT		= 1030,
	
	CL_CONSTR_COUNT				= 10000,
	CL_POINT_COUNT				= 10001,
	CL_POINTS_ADDED				= 10002
};


class CDClothTagPlugin : public CDTagData
{
private:
	CDArray<LONG>			wtChange;
	CDAABB					bounds;

	CDArray<CLParticle>		particles;
	CLConstraintArray		constraints;
	
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
public:
	Bool InitParticles(BaseObject *op, BaseContainer *tData);
	
	void RemoveUnusedConstraints(void);
	void AddConstraint(LONG a, LONG b, LONG type);
	void AddEdgeBendConstraint(CPolygon *vadr, LONG plyInd, LONG start, LONG a, LONG b);
	Bool CreateConstraints(BaseObject *op, BaseContainer *tData);
	
	void TimeStep(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	void SatisfyConstraints(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	void AddSkinForce(BaseContainer *tData, BaseObject *op, Vector *ref, CDSkinVertex *skWt);
	void AddForce(BaseContainer *tData, Vector f);
	void WindForces(BaseContainer *tData, Real t);
	
	Bool BoundsIntersection(CDAABB *b, Matrix bM, Matrix opM);
	Bool PlanerCollision(BaseContainer *tData, BaseTag *cldTag);
	Bool SphericalCollision(BaseContainer *tData, BaseTag *tag, BaseObject *op);
	Bool CDJointCollision(BaseContainer *tData, BaseTag *tag, BaseObject *op);
	Vector GetPolygonCenter(const CPolygon &v);
	Bool PolygonalCollision(BaseContainer *tData, BaseTag *tag, BaseObject *op);
	void Collision(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	
	void RemapClothWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map);
	Bool TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg);
	
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	
	// accessor functions
	Bool GetWeight(BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool SetWeight(BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDClothTagPlugin); }
};


#endif

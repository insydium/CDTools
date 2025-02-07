//
//  CDRopeTag.h
//  Created by Dan Libisch on 8/4/13.
//  Copyright 2013 Libisch Graphic Design. All rights reserved.
//

#ifndef _CDRopeTag_H_
#define _CDRopeTag_H_

// c4d includes
#include "c4d.h"
#include "lib_modeling.h"

// CDCL includes
#include "CDTagData.h"
#include "CLConstraint.h"

enum
{
	//RP_DAMPING				= 1000,
	//RP_TIME_STEP			= 1001,
	//RP_ITERATIONS			= 1002,
	//RP_STIFFNESS			= 1003,
	
	//RP_USE_GRAVITY			= 1010,
	//RP_GRAVITY				= 1011,
	//RP_KEEP_SHAPE			= 1012,
	
	//RP_WIND_OBJECT			= 1020,
	
	//RP_COLLISION_OBJECT	= 1030,
	
	RP_POINT_COUNT			= 10000,
	RP_CONSTR_COUNT			= 10001,
	
	RP_POINTS_ADDED			= 10002
};

class CDRopeTagPlugin : public CDTagData
{
private:
	CDArray<Vector>			ref;
	CDArray<CLParticle>		particles;
	CLConstraintArray		constraints;
	
public:
	Bool InitParticles(BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Bool ConstraintExists(LONG a, LONG b);
	void AddConstraint(LONG a, LONG b, LONG type);
	Bool CreateConstraints(BaseObject *op, BaseContainer *tData);
	
	void TimeStep(BaseContainer *tData);
	void SatisfyConstraints(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	void AddShapeForce(BaseContainer *tData, BaseObject *op);
	void AddForce(BaseContainer *tData, Vector f);
	void WindForces(BaseContainer *tData, Real t);
	
	Bool PlanerCollision(BaseContainer *tData, BaseContainer *ctData, BaseObject *op);
	Bool SphericalCollision(BaseContainer *tData, BaseContainer *ctData, BaseObject *op);
	void Collision(BaseDocument *doc, BaseContainer *tData);
	
	Bool RestoreReference(BaseObject *op);
	//void RemapReference(Vector *padr, TranslationMaps *map);
	void RemapRopeWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map);
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
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDRopeTagPlugin); }
};


#endif

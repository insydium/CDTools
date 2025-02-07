#ifndef _CDRungeKutta_H_
#define _CDRungeKutta_H_

// c4d includes
#include "c4d.h"

// CD includes
#include "CDQuaternion.h"

#define MAXFORCE		1000000.0

struct State
{
	// primary
	Vector			position;
	Vector			momentum;
	Vector			scale;
	Vector			scaleMomentum;
	CDQuaternion	orientation;
	Vector			angularMomentum;
	
	// secondary
	Vector			velocity;
	Vector			scaleVelocity;
	CDQuaternion	spin;
	Vector			angularVelocity;
	
	// constant
	Real			size;
	Real			pMass;
	Real			invPMass;
	
	Real			sMass;
	Real			invSMass;
	
	Real			rMass;
	Real			invRMass;
	
	Real			inertia;
	Real			invInertia;
};

struct Derivative
{
	Vector			velocity;
	Vector			force;
	CDQuaternion	spin;
	Vector			torque;
	Vector			scaleVelocity;
	Vector			scaleForce;
};

class CDRungeKutta
{
	private:	
		Real kP, bP, kS, bS, kR, bR;
		
	public:
		CDRungeKutta();
		~CDRungeKutta();
		
		void Recalculate(State &s);
		State Interpolate(State a, State b, Real mix);
		
		Derivative Evaluate(State &s, Real t);
		Derivative Evaluate(State s, Real t, Real dt, Derivative &d);
		void Integrate(State &s, Real t, Real dt);
		
		void Forces(State &s, Real t, Vector &force, Vector &torque, Vector &scaleForce);
		
		void SetSpringConstants(Real kpCnst, Real bpCnst, Real ksCnst, Real bsCnst, Real krCnst, Real brCnst, Real f);

};

#endif
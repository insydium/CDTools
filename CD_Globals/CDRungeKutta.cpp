#include "CDRungeKutta.h"
#include "CDDebug.h"

CDRungeKutta::CDRungeKutta()
{
	kP = bP = kS = bS = kR = bR = 1;
}

CDRungeKutta::~CDRungeKutta() { }

State CDRungeKutta::Interpolate(State a, State b, Real mix)
{
	State s;
	// position
	s.position = CDBlend(a.position, b.position, mix);
	s.momentum = CDBlend(a.momentum, b.momentum, mix);
	// scale
	s.scale = CDBlend(a.scale, b.scale, mix);
	s.scaleMomentum = CDBlend(a.scaleMomentum, b.scaleMomentum, mix);
	//rotation
	s.orientation = CDQSlerp(a.orientation, b.orientation, mix);
	s.angularMomentum = CDBlend(a.angularMomentum, b.angularMomentum, mix);
	
	Recalculate(s);
	
	return s;
}

void CDRungeKutta::Recalculate(State &s)
{
	// position
	s.velocity = s.momentum * s.invPMass;
	// scale
	s.scaleVelocity = s.scaleMomentum * s.invSMass;
	// rotation
	s.angularVelocity = s.angularMomentum * s.invInertia;
	s.orientation = !(s.orientation);
	s.spin = 0.5 * CDQuaternion(0.0, s.angularVelocity.x, s.angularVelocity.y, s.angularVelocity.z) * s.orientation;
}

Derivative CDRungeKutta::Evaluate(State &s, Real t)
{
	Derivative output;
	// position
	output.velocity = s.velocity;
	// scale
	output.scaleVelocity = s.scaleVelocity;
	//rotation
	output.spin = s.spin;
	
	Forces(s, t, output.force, output.torque, output.scaleForce);
	return output;
}

Derivative CDRungeKutta::Evaluate(State s, Real t, Real dt, Derivative &d)
{
	// position
	s.position += d.velocity*dt;
	s.momentum += d.force*dt;
	// scale
	s.scale += d.scaleVelocity*dt;
	s.scaleMomentum += d.scaleForce*dt;
	//rotation
	s.orientation += d.spin * dt;
	s.angularMomentum += d.torque * dt;
	Recalculate(s);
	
	Derivative output;
	// position
	output.velocity = s.velocity;
	// scale
	output.scaleVelocity = s.scaleVelocity;
	// rotation
	output.spin = s.spin;
	
	Forces(s, t, output.force, output.torque, output.scaleForce);
	return output;
}

void CDRungeKutta::Integrate(State &s, Real t, Real dt)
{
	Derivative a = Evaluate(s, t);
	Derivative b = Evaluate(s, t, dt*0.5f, a);
	Derivative c = Evaluate(s, t, dt*0.5f, b);
	Derivative d = Evaluate(s, t, dt, c);
	
	// position
	s.position += (1.0 / 6.0) * dt * (a.velocity + 2.0 * (b.velocity + c.velocity) + d.velocity);
	s.momentum += (1.0 / 6.0) * dt * (a.force + 2.0 * (b.force + c.force) + d.force);
	// scale
	s.scale += (1.0 / 6.0) * dt * (a.scaleVelocity + 2.0 * (b.scaleVelocity + c.scaleVelocity) + d.scaleVelocity);
	s.scaleMomentum += (1.0 / 6.0) * dt * (a.scaleForce + 2.0 * (b.scaleForce + c.scaleForce) + d.scaleForce);
	//rotation
	s.orientation += (1.0 / 6.0) * dt * (a.spin + (2.0 * (b.spin + c.spin)) + d.spin);
	s.angularMomentum += (1.0 / 6.0) * dt * (a.torque + 2.0 * (b.torque + c.torque) + d.torque);
	
	Recalculate(s);
}


void CDRungeKutta::Forces(State &s, Real t, Vector &force, Vector &torque, Vector &scaleForce)
{
	//Clear scaleForces
	force = Vector();
	scaleForce = Vector();
	torque = Vector();
	
	// position
	force += (-kP * Len(s.position) * (VNorm(s.position)) - bP * s.velocity);
	if(Len(force) > MAXFORCE)
	{
		Vector dir = VNorm(force);
		force = dir * MAXFORCE;
	}
	// scale
	scaleForce += (-kS * Len(s.scale) * (VNorm(s.scale)) - bS * s.scaleVelocity);
	if(Len(scaleForce) > MAXFORCE)
	{
		Vector dir = VNorm(scaleForce);
		scaleForce = dir * MAXFORCE;
	}
	// rotation
	CDQuaternion diffQuat; 
	diffQuat = CDQConjugate(s.orientation) * CDQuaternion();
	Real angle = diffQuat.GetAngle();
	Vector axis = diffQuat.v;
	
	torque += (kR * angle * VNorm(axis)) - (bR * s.angularVelocity);
}

void CDRungeKutta::SetSpringConstants(Real kpCnst, Real bpCnst, Real ksCnst, Real bsCnst, Real krCnst, Real brCnst, Real f)
{
	// position
	kP = kpCnst * f;
	bP = bpCnst * kpCnst * 0.08 * f;
	// scale
	kS = ksCnst * f;
	bS = bsCnst * ksCnst * 0.08 * f;
	// rotation
	kR = krCnst * f;
	bR = brCnst * krCnst * 0.08 * f;
}
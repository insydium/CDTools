//  CLParticle.h

#ifndef _CLPARTICLE_H_
#define _CLPARTICLE_H_

class CLParticle
{
public:
	Real	mass;
	Real	wt;
	
	Vector	pos;
	Vector	old_pos;
	Vector	acceleration;
	Vector	start_pos;
	
	CLParticle()
	{
		mass = 1.0;
		wt = 0.0;
		//ind = -1;
		pos = Vector(0,0,0);
		old_pos = Vector(0,0,0);
		acceleration = Vector(0,0,0);
		start_pos = Vector(0,0,0);
	}
	
	void SetParticle(Vector p, Vector op, Vector a, Vector sp, Real ms, Real w)//, Bool mv
	{
		pos = p;
		old_pos = op;
		acceleration = a;
		start_pos = sp;
		mass = 1.0;
		wt = w;
	}
	
	void AddForce(Vector f)
	{
		if(wt > 0.0) acceleration += (f/mass) * wt;
	}
	
	void TimeStep(Real d, Real t)
	{
		if(wt > 0.0)
		{
			Vector temp = pos;
			pos = pos + (pos-old_pos) * (1.0-d) + acceleration * t;
			old_pos = temp;
			acceleration = Vector(0,0,0);	
		}
	}
	
	void ResetAcceleration(void) { acceleration = Vector(0,0,0); }
	void SetStartPosition(void) { start_pos = pos;}
	void SetPosition(const Vector v) { pos = v; }
	void OffsetPosition( const Vector v) { pos += v; }
};

#endif
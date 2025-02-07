//	Cactus Dan's Quaternion Class
//	Copyright 2008 by Cactus Dan Libisch

// CDQuaternion.cpp
#include "CDQuaternion.h"

// Create identity quaternion
CDQuaternion::CDQuaternion(void)
{
	w = 1.0;
	v.x = 0.0;
	v.y = 0.0;
	v.z = 0.0;
}

// Create quaternion from separate components
CDQuaternion::CDQuaternion(Real s, Real x, Real y, Real z)
{
	w = s;
	v.x = x;
	v.y = y;
	v.z = z;
}

//Create Quaternion from axis and angle from 3DMath Primer
void CDQuaternion::SetAngleAxis(Real angle, const Vector &axis)
{	
	Vector a = VNorm(axis);
	Real t2 = angle * 0.5;
	
	w = Cos(t2);
	v.x = (a.x) * Sin(t2);
	v.y = (a.y) * Sin(t2);
	v.z = (a.z) * Sin(t2);
}

//Get the rotation angle  from 3DMath Primer
Real CDQuaternion::GetAngle(void)
{
	return ACos(w) * 2.0;
}

//Get the rotations axis from 3DMath Primer
Vector CDQuaternion::GetAxis(void)
{
	Real a = 1.0 - w*w;

	if(a <= 0.0)
	{
		// Protect against numerical imprecision
		return Vector(1.0, 0.0, 0.0);
	}

	// Compute 1 / sin(theta/2)
	Real t = 1.0 / Sqrt(a);

	return Vector(v.x*t, v.y*t, v.z*t);
}

// HPB to Quaternion from 3DMath Primer
void CDQuaternion::SetHPB(const Vector &rot)
{
	CDQuaternion qh,qp,qb;
		
	qh.SetAngleAxis(rot.x,Vector(0,1,0));
	qp.SetAngleAxis(rot.y,Vector(1,0,0));
	qb.SetAngleAxis(rot.z,Vector(0,0,1));

	*this = CDQConjugate(qb * (qp * qh)) * CDQuaternion();
}

// Quaternion to HPB from 3DMath Primer
Vector CDQuaternion::GetHPB(void) const
{
	Vector hpb;
	
	// Extract sin(pitch)
	Real sp = -2.0 * (v.y*v.z + w*v.x);

	// Check for Gimbel lock, giving slight tolerance for numerical imprecision

	if(Abs(sp) > 0.9999)
	{
		// Looking straight up or down
		hpb.y = pi05 * sp;

		// Compute heading, slam bank to zero

		hpb.x = (Real)atan2((double)-v.x*v.z - w*v.y, (double)0.5 - v.y*v.y - v.z*v.z);
		hpb.z = 0.0;

	}
	else
	{

		// Compute angles.  We don't have to use the "safe" asin
		// function because we already checked for range errors when
		// checking for Gimbel lock

		hpb.y	= ASin(sp);
		hpb.x	= (Real)atan2((double)v.x*v.z - w*v.y, (double)0.5 - v.x*v.x - v.y*v.y);
		hpb.z	= (Real)atan2((double)v.x*v.y - w*v.z, (double)0.5 - v.x*v.x - v.z*v.z);
	}
	
	return hpb;
}

// Matrix to Quaternion from 3DMath Primer
void CDQuaternion::SetMatrix(const Matrix &aM)
{
	Matrix m = GetNormalizedMatrix(aM);

	Real wSquared = m.v1.x + m.v2.y + m.v3.z;
	Real xSquared = m.v1.x - m.v2.y - m.v3.z;
	Real ySquared = m.v2.y - m.v1.x - m.v3.z;
	Real zSquared = m.v3.z - m.v1.x - m.v2.y;

	LONG biggestIndex = 0;
	Real biggestSquare = wSquared;
	if(xSquared > biggestSquare)
	{
		biggestSquare = xSquared;
		biggestIndex = 1;
	}
	if(ySquared > biggestSquare)
	{
		biggestSquare = ySquared;
		biggestIndex = 2;
	}
	if(zSquared > biggestSquare)
	{
		biggestSquare = zSquared;
		biggestIndex = 3;
	}
	
	Real biggestValue = Sqrt(biggestSquare + 1.0) * 0.5;
	Real mult = 0.25 / biggestValue;
	
	switch(biggestIndex)
	{
		case 0:
			w = biggestValue;
			v.x = (m.v2.z - m.v3.y) * mult;
			v.y = (m.v3.x - m.v1.z) * mult;
			v.z = (m.v1.y - m.v2.x) * mult;
			break;
		case 1:
			v.x = biggestValue;
			w = (m.v2.z - m.v3.y) * mult;
			v.y = (m.v1.y + m.v2.x) * mult;
			v.z = (m.v3.x + m.v1.z) * mult;
			break;
		case 2:
			v.y = biggestValue;
			w = (m.v3.x - m.v1.z) * mult;
			v.x = (m.v1.y + m.v2.x) * mult;
			v.z = (m.v2.z + m.v3.y) * mult;
			break;
		case 3:
			v.z = biggestValue;
			w = (m.v1.y - m.v2.x) * mult;
			v.x = (m.v3.x + m.v1.z) * mult;
			v.y = (m.v2.z + m.v3.y) * mult;
			break;
	}
	
	if(w < 0.0)
	{
		w = -w;
		v.x = -v.x;
		v.y = -v.y;
		v.z = -v.z;
	}
}

// Quaternion to Matrix from 3DMath Primer
Matrix CDQuaternion::GetMatrix(void) const
{
	Matrix m;
	
	Real	ww = 2.0 * w;
	Real	xx = 2.0 * v.x;
	Real	yy = 2.0 * v.y;
	Real	zz = 2.0 * v.z;

	m.off = Vector(0,0,0);

	m.v1.x = 1.0 - yy*v.y - zz*v.z;
	m.v1.y = xx*v.y + ww*v.z;
	m.v1.z = xx*v.z - ww*v.y;

	m.v2.x = xx*v.y - ww*v.z;
	m.v2.y = 1.0 - xx*v.x - zz*v.z;
	m.v2.z = yy*v.z + ww*v.x;

	m.v3.x = xx*v.z + ww*v.y;
	m.v3.y = yy*v.z - ww*v.x;
	m.v3.z = 1.0 - xx*v.x - yy*v.y;
	
	return GetNormalizedMatrix(m);
}//


// Quaternion math
CDQuaternion CDQPower(const CDQuaternion &q, Real s)
{
	CDQuaternion r=q;

	if(Abs(q.w) < 0.9999)
	{
		Real a, aNew, mult;
		
		a = ACos(q.w);
		aNew = a*s;
		r.w = Cos(aNew);
		
		mult = Sin(aNew) / Sin(a);
		r.v.x = q.v.x * mult;
		r.v.y = q.v.y * mult;
		r.v.z = q.v.z * mult;
	}

	return r;
}

CDQuaternion CDQLog(const CDQuaternion &q)
{
	CDQuaternion r;

	Real a = ACos(q.w);
	Real sina = Sin(a);
	r.w = 0.0;
	if(sina > 0.0)
	{
		r.v.x = a*q.v.x/sina;
		r.v.y = a*q.v.y/sina;
		r.v.z = a*q.v.z/sina;
	}
	else
	{
		r.v.x = 0.0;
		r.v.y = 0.0;
		r.v.z = 0.0;
	}
	
	return r;
}

CDQuaternion CDQConjugate(const CDQuaternion &q)
{
	CDQuaternion r;

	r.w = q.w;
	r.v = -q.v;

	return r;
}

Real CDQDot(const CDQuaternion &q1, const CDQuaternion &q2)
{
	Real dot = q1.w*q2.w + q1.v.x*q2.v.x + q1.v.y*q2.v.y + q1.v.z*q2.v.z;
	
	return dot;
}


// Quaternion interpolation
CDQuaternion CDQSlerp(const CDQuaternion &q1, const CDQuaternion &q2, const Real alpha)
{
	Real       sum,teta,beta1,beta2;
	CDQuaternion q,qd=q2;

	sum = (q1.w * q2.w) + VDot(q1.v, q2.v);

	if(sum<0.0)
	{
		sum=-sum;
		qd.v=-qd.v;
		qd.w=-qd.w;
	}

	if(1.0-sum>CD_EPSILON)
	{
		teta = ACos(sum);
		Real sn = 1.0/Sin(teta);
		beta1 = Sin((1.0-alpha)*teta)*sn;
		beta2 = Sin(alpha*teta)*sn;
	}
	else
	{
		beta1 = 1.0-alpha;
		beta2 = alpha;
	}
	q.w   = beta1*q1.w   + beta2*qd.w;
	q.v.x = beta1*q1.v.x + beta2*qd.v.x;
	q.v.y = beta1*q1.v.y + beta2*qd.v.y;
	q.v.z = beta1*q1.v.z + beta2*qd.v.z;

	return q;
}

CDQuaternion CDQSlerpSpin(const CDQuaternion &q1, const CDQuaternion &q2, const Real alpha, LONG spin)
{
	CDQuaternion q;

    Real beta, alpha2;			// complementary interp parameter
    Real theta;			// angle between A and B
    Real sin_t, cos_t;		// sine, cosine of theta
    Real phi;				// theta plus spins
    Bool bflip;			// use negation of B?

    // cosine theta = dot product of A and B
    cos_t = CDQDot(q1,q2);

    // ifB is on opposite hemisphere from A, use -B instead
    if(cos_t < 0.0)
	{
		cos_t = -cos_t;
		bflip = true;
    }
	else  bflip = false;

    /* ifB is (within precision limits) the same as A,
     * just linear interpolate between A and B.
     * Can't do spins, since we don't know what direction to spin.
     */
    if(1.0 - cos_t < CD_EPSILON)
	{
		beta = 1.0 - alpha;
    }
	else 
	{				
		/* normal case */
		theta = acos(cos_t);
		phi   = theta + spin * pi;
		sin_t = sin(theta);
		beta  = sin(theta - alpha*phi) / sin_t;
		alpha2 = sin(alpha*phi) / sin_t;
    }

    if(bflip) alpha2 = -alpha2;

    /* interpolate */
    q.v.x = beta*q1.v.x + alpha2*q2.v.x;
    q.v.y = beta*q1.v.y + alpha2*q2.v.y;
    q.v.z = beta*q1.v.z + alpha2*q2.v.z;
    q.w = beta*q1.w + alpha2*q2.w;

	return !q;
}

CDQuaternion CDQSlerpBezier(const CDQuaternion &q1, const CDQuaternion &q2, const CDQuaternion &q3, const CDQuaternion &q4, const CDQuaternion &q5, const Real alpha)
{
	CDQuaternion a,b,c,d,e,f,g,h,i;
	
	a = CDQSlerp(q1, q2, alpha);
	b = CDQSlerp(q2, q3, alpha);
	c = CDQSlerp(q3, q4, alpha);
	d = CDQSlerp(q4, q5, alpha);
	
	e = CDQSlerp(a, b, alpha);
	f = CDQSlerp(b, c, alpha);
	g = CDQSlerp(c, d, alpha);

	h = CDQSlerp(e, f, alpha);
	i = CDQSlerp(f, g, alpha);
	
	return CDQSlerp(h, i, alpha);
}


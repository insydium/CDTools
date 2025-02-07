#ifndef _CDQuaternion_H_
#define _CDQuaternion_H_

#include "math.h"
#include "ge_math.h"
#include "ge_matrix.h"
#include "ge_vector.h"

#include "CDCompatibility.h"
#include "CDGeneral.h"

#define CD_EPSILON 0.00001

class CDQuaternion
{
	public:
		Real   w; // angle
		Vector v; // axis

		CDQuaternion(void);
		CDQuaternion(Real s, Real x, Real y, Real z);
		 
		Matrix GetMatrix(void) const;			 	
		void SetMatrix(const Matrix &aM);
		Vector GetHPB(void) const;
		void SetHPB(const Vector &rot);
		
		void SetAngleAxis(Real angle, const Vector &axis);
		Real GetAngle(void);
		Vector GetAxis(void);
		
		friend inline CDQuaternion operator-(const CDQuaternion &q);
		friend inline CDQuaternion operator!(const CDQuaternion &q);
		friend inline CDQuaternion operator+(const CDQuaternion &q1, const CDQuaternion &q2);
		friend inline CDQuaternion operator-(const CDQuaternion &q1, const CDQuaternion &q2);
		friend inline CDQuaternion operator*(const CDQuaternion &q1, const CDQuaternion &q2);

		friend inline CDQuaternion& operator+=(CDQuaternion &q1, const CDQuaternion &q2);
		friend inline CDQuaternion& operator-=(CDQuaternion &q1, const CDQuaternion &q2);
		friend inline CDQuaternion& operator*=(CDQuaternion &q1, const CDQuaternion &q2);

		friend inline Bool operator ==(CDQuaternion &q1, const CDQuaternion &q2);
		friend inline Bool operator !=(CDQuaternion &q1, const CDQuaternion &q2);

		friend inline CDQuaternion operator*(const CDQuaternion &q, Real s);
		friend inline CDQuaternion operator*(Real s, const CDQuaternion &q);
		friend inline CDQuaternion operator/(const CDQuaternion &q, Real s);
		friend inline CDQuaternion& operator*=(CDQuaternion &q, Real s);
		friend inline CDQuaternion& operator/=(CDQuaternion &q, Real s);
};

inline Bool operator ==(CDQuaternion &q1, const CDQuaternion &q2)
{
	return (Abs(q1.w-q2.w) < 0.001 && Abs(q1.v.x-q2.v.x) < 0.001 && Abs(q1.v.y-q2.v.y) < 0.001 && Abs(q1.v.z-q2.v.z) < 0.001);
}

inline Bool operator !=(CDQuaternion &q1, const CDQuaternion &q2)
{
	return !(q1 == q2);
}

inline CDQuaternion operator-(const CDQuaternion &q)
{
	return CDQuaternion(-q.w, -q.v.x, -q.v.y, -q.v.z);
}

inline CDQuaternion operator!(const CDQuaternion &q)
{
	Real l = Sqrt(q.w*q.w + q.v.x*q.v.x + q.v.y*q.v.y + q.v.z*q.v.z);
	if (l > 0.0)
	{
		return CDQuaternion(q.w/l, q.v.x/l, q.v.y/l, q.v.z/l);
	}
	else
	{
		return CDQuaternion();
	}
	
}

inline CDQuaternion operator+(const CDQuaternion &q1, const CDQuaternion &q2)
{
	return CDQuaternion(q1.w+q2.w, q1.v.x+q2.v.x, q1.v.y+q2.v.y, q1.v.z+q2.v.z);
}

inline CDQuaternion operator-(const CDQuaternion &q1, const CDQuaternion &q2)
{
	return CDQuaternion(q1.w-q2.w, q1.v.x-q2.v.x, q1.v.y-q2.v.y, q1.v.z-q2.v.z);
}

inline CDQuaternion operator*(const CDQuaternion &q1, const CDQuaternion &q2)
{
	return CDQuaternion( q1.w*q2.w - q1.v.x*q2.v.x - q1.v.y*q2.v.y - q1.v.z*q2.v.z, 
					   q1.v.x*q2.w + q1.w*q2.v.x + q1.v.y*q2.v.z - q1.v.z*q2.v.y,
					   q1.v.y*q2.w + q1.w*q2.v.y + q1.v.z*q2.v.x - q1.v.x*q2.v.z,
					   q1.v.z*q2.w + q1.w*q2.v.z + q1.v.x*q2.v.y - q1.v.y*q2.v.x);
}

inline CDQuaternion& operator+=(CDQuaternion &q1, const CDQuaternion &q2)
{
	q1.w += q2.w;
	q1.v.x += q2.v.x;
	q1.v.y += q2.v.y;
	q1.v.z += q2.v.z;
	return q1;
}

inline CDQuaternion& operator-=(CDQuaternion &q1, const CDQuaternion &q2)
{
	q1.w -= q2.w;
	q1.v.x -= q2.v.x;
	q1.v.y -= q2.v.y;
	q1.v.z -= q2.v.z;
	return q1;
}

inline CDQuaternion& operator*=(CDQuaternion &q1, const CDQuaternion &q2)
{
	q1.w *= q2.w;
	q1.v.x *= q2.v.x;
	q1.v.y *= q2.v.y;
	q1.v.z *= q2.v.z;
	return q1;
}

inline CDQuaternion operator*(const CDQuaternion &q, Real s)
{
	return CDQuaternion(q.w*s, q.v.x*s, q.v.y*s, q.v.z*s);
}

inline CDQuaternion operator*(Real s, const CDQuaternion &q)
{
	return CDQuaternion(q.w*s, q.v.x*s, q.v.y*s, q.v.z*s);
}
inline CDQuaternion operator/(const CDQuaternion &q, Real s)
{
	if(s != 0.0) return CDQuaternion(q.w/s, q.v.x/s, q.v.y/s, q.v.z/s);
	else return q;
}

inline CDQuaternion& operator*=(CDQuaternion &q, Real s)
{
	q.w *= s;
	q.v.x *= s;
	q.v.y *= s;
	q.v.z *= s;
	return q;
}

inline CDQuaternion& operator/=(CDQuaternion &q, Real s)
{
	q.w /= s;
	q.v.x /= s;
	q.v.y /= s;
	q.v.z /= s;
	return q;
}

// Quaternion math
CDQuaternion CDQPower(const CDQuaternion &q, Real s);
CDQuaternion CDQLog(const CDQuaternion &q);
CDQuaternion CDQConjugate(const CDQuaternion &q);
Real CDQDot(const CDQuaternion &q1, const CDQuaternion &q2);

// Quaternion interpolation
CDQuaternion CDQSlerp(const CDQuaternion &q1, const CDQuaternion &q2, const Real alpha);
CDQuaternion CDQSlerpBezier(const CDQuaternion &q1, const CDQuaternion &q2, const CDQuaternion &q3, const CDQuaternion &q4, const CDQuaternion &q5, const Real alpha);
CDQuaternion CDQSlerpSpin(const CDQuaternion &q1, const CDQuaternion &q2, const Real alpha, LONG spin);



#endif

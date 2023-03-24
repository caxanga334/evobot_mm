//
// EvoBot - Neoptolemus' Natural Selection bot, based on Botman's HPB bot template
//
// bot_math.h
// 
// Contains all useful math functions for bot stuff
//

#pragma once

#ifndef BOT_MATH_H
#define BOT_MATH_H

#include <extdll.h>

static const Vector ZERO_VECTOR = Vector(0.0f, 0.0f, 0.0f);
static const Vector UP_VECTOR = Vector(0.0f, 0.0f, 1.0f); // Normalized "up" direction
static const Vector RIGHT_VECTOR = Vector(1.0f, 0.0f, 0.0f); // Normalized "right" direction
static const Vector FWD_VECTOR = Vector(0.0f, 1.0f, 0.0f); // Normalized "forward" direction

static const float MATH_PI = 3.141592654f;
static const float DEGREES_RADIANS_CONV = (MATH_PI / 180.0f);

static const float GOLDSRC_GRAVITY = 800.0f; // Default speed of gravity in GoldSrc units per second squared

// Defines a frustum plane
typedef struct _FRUSTUM_PLANE_T
{
	Vector normal;
	Vector point;
	float d;
} frustum_plane_t;


// GENERAL MATH

// Is the input string a valid number?
bool isNumber(const char* line);
// Square the input
float sqrf(float input);
// Return the sign (-1 if number is negative, 1 if positive, 0 if 0)
float signf(float input);
// Clamp value between min and max
float clampf(float input, float inMin, float inMax);
// Clamp the angle to a valid GoldSrc angle (-180 to 180)
void ClampAngle(float& angle);
// Spherical linear interpolation of float from start to end at interp speed
float fInterpTo(float start, float end, float DeltaTime, float InterpSpeed);
// Linear interpolation of float from start to end at interp speed
float fInterpConstantTo(float start, float end, float DeltaTime, float InterpSpeed);
// Random float between min value and max value
float frandrange(float MinValue, float MaxValue);
// Random integer between min and max values
int irandrange(int MinValue, int MaxValue);
// Convert degrees to radians
float fDegreesToRadians(const float Degrees);
// Return random boolean
bool randbool();

// VECTOR MATH

// 2D (ignore Z axis) distance between v1 and v2
float vDist2D(const Vector& v1, const Vector& v2);
// 3D distance between v1 and v2
float vDist3D(const Vector& v1, const Vector& v2);
// Squared (no sqrt) 2D distance (ignore Z axis) between v1 and v2
float vDist2DSq(const Vector& v1, const Vector& v2);
// Squared (no sqrt) 3D distance between v1 and v2
float vDist3DSq(const Vector& v1, const Vector& v2);

// 3D length of vector
float vSize3D(const Vector& V);
// 2D length (no Z axis) of vector
float vSize2D(const Vector& V);
// Squared 3D length of vector
float vSize3DSq(const Vector& V);
// Squared 2D length (no Z axis) of vector
float vSize2DSq(const Vector& V);

// Are two vectors equal, using default epsilon of 0.1f
bool vEquals(const Vector& v1, const Vector& v2);
// Are two vectors equal, using custom epsilon
bool vEquals(const Vector& v1, const Vector& v2, const float epsilon);

// Returns the dot product of two unit vectors
float UTIL_GetDotProduct(const Vector& v1, const Vector& v2);
// Returns the dot product of two unit vectors excluding Z axis
float UTIL_GetDotProduct2D(const Vector& v1, const Vector& v2);

// Normalize the unit vector (modifies input)
void UTIL_NormalizeVector(Vector* vec);
// Normalize the unit vector without the Z axis (modifies input)
void UTIL_NormalizeVector2D(Vector* vec);
// Returns a normalized copy of the input unit vector
Vector UTIL_GetVectorNormal(const Vector& vec);
// Returns a normalized 2D copy of the input vector without Z axis
Vector UTIL_GetVectorNormal2D(const Vector& vec);
//Returns the cross product of v1 and v2
Vector UTIL_GetCrossProduct(const Vector& v1, const Vector& v2);
// Returns the surface normal of a poly defined at points v1, v2 and v3 (clockwise)
Vector UTIL_GetSurfaceNormal(const Vector& v1, const Vector& v2, const Vector& v3);

// WIP: Trying to get a working random unit vector in cone. Not currently used
Vector UTIL_GetRandomUnitVectorInCone(const Vector& ConeDirection, const float HalfAngleRadians);
Vector random_unit_vector_within_cone(const Vector Direction, double HalfAngleRadians);

// Takes in a bot's current view angle and a target direction to look in, and returns the appropriate view angles. Eases into the target
Vector ViewInterpTo(const Vector& CurrentViewAngles, const Vector& TargetDirection, const float DeltaTime, const float InterpSpeed);


// LINE MATH

// For given line defined by lineFrom -> lineTo, how far away from that line is CheckPoint?
float vDistanceFromLine3D(const Vector& lineFrom, const Vector& lineTo, const Vector& CheckPoint);
// For given line defined by lineFrom -> lineTo, how far away from that line is CheckPoint? Ignores Z axis
float vDistanceFromLine2D(const Vector& lineFrom, const Vector& lineTo, const Vector& CheckPoint);
// For given line defined by lineFrom -> lineTo, get squared distance from that line of CheckPoint. Ignores Z axis
float vDistanceFromLine2DSq(const Vector& lineFrom, const Vector& lineTo, const Vector& CheckPoint);

// Returns 0 if point sits right on the line defined by lineFrom -> lineTo, -1 if it sits to the left, 1 if it sits to the right. Ignores Z axis
int vPointOnLine(const Vector& lineFrom, const Vector& lineTo, const Vector& point);

// Finds the closest point along a line to the input point
Vector vClosestPointOnLine(const Vector& lineFrom, const Vector& lineTo, const Vector& point);
// Finds the closest point along a line to the input point, ignores Z axis
Vector vClosestPointOnLine2D(const Vector& lineFrom, const Vector& lineTo, const Vector& point);
// Finds the closest point along a line to the input point, assumes infinite line
Vector vClosestPointOnInfiniteLine3D(const Vector& PointOnLine, const Vector& NormalisedLineDir, const Vector& TestPoint);
// Finds the closest point along a line to the input point, assumes infinite line. Ignores Z axis
Vector vClosestPointOnInfiniteLine2D(const Vector& PointOnLine, const Vector& NormalisedLineDir, const Vector& TestPoint);

// CONVERSIONS

// Converts input metres to GoldSrc units (approx 1 metres = 52.5 units)
float UTIL_MetresToGoldSrcUnits(const float Metres);
// Converts input GoldSrc units to metres (approx 52.5 units = 1 metre)
float UTIL_GoldSrcUnitsToMetres(const float GoldSrcUnits);
// Converts angles to unit vector
void UTIL_AnglesToVector(const Vector& angles, Vector* fwd, Vector* right, Vector* up);
// Returns unit vector pointing in direction of input angles
Vector UTIL_GetForwardVector(const Vector& angles);
// Returns 2D unit vector pointing in direction of input view angles, ignoring Z axis
Vector UTIL_GetForwardVector2D(const Vector& angles);

// GEOMETRY STUFF

// Returns random point on a circle, assuming circle normal is (0, 0, 1)
Vector UTIL_RandomPointOnCircle(const Vector origin, const float radius);

// Returns the required pitch needed to hit the target point from launch point, taking projectile speed and gravity into account
Vector GetPitchForProjectile(Vector LaunchPoint, Vector TargetPoint, const float ProjectileSpeed, const float Gravity);

// Confirms if the given point is on the inside of a frustum plane or not
bool UTIL_PointInsidePlane(const frustum_plane_t* plane, const Vector& point);

/* Tests to see if the defined cylinder is intersecting with the supplied frustum plane.

   Since players are always upright, it is reasonable to assume that it is impossible for both the
   top and bottom of the cylinder to be outside the plane if it is intersecting, therefore
   we only need to test the top and bottom cylinder at the closest point to the plane.*/
bool UTIL_CylinderInsidePlane(const frustum_plane_t* plane, const Vector& centre, float height, float radius);
// Set the normal and position for the plane based on the 3 points defining it
void UTIL_SetFrustumPlane(frustum_plane_t* plane, Vector v1, Vector v2, Vector v3);
// Finds the closest point to the polygon, defined by segments (edges)
float UTIL_GetDistanceToPolygon2DSq(const Vector TestPoint, const Vector* Segments, const int NumSegments);

#endif
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <list>
#include <math.h>

using namespace std;

struct Vector2 {
    Vector2(): x(0), y(0) {}
    Vector2(double x, double y): x(x), y(y) {}
    double x, y;

};

Vector2 operator -(Vector2 v1, Vector2 v2);
Vector2 operator +(Vector2 v1, Vector2 v2);
Vector2 operator *(double d, Vector2 v);

double dot(Vector2 v1, Vector2 v2);
double length(Vector2 v);
Vector2 normalized(Vector2 v);
double getDistance(Vector2 vLine1, Vector2 vLine2, Vector2 vPos);


struct Vector3 {
    Vector3(): x(0), y(0), z(0) {}
    Vector3(double x, double y, double z): x(x), y(y), z(z) {}
    Vector3(Vector2 v, double z): x(v.x), y(v.y), z(z) {}
    double x, y, z;
};

typedef list<Vector2> PointList;

bool operator<(const Vector3 a, const Vector3 b);

struct Triangle2 {
    Triangle2( Vector2 v1, Vector2 v2, Vector2 v3): v1(v1), v2(v2), v3(v3) {}
    Vector2 v1, v2, v3;
};

struct Triangle3 {
    Triangle3( Vector3 v1, Vector3 v2, Vector3 v3): v1(v1), v2(v2), v3(v3) {}
    Triangle3( Triangle2 t, double height): v1(t.v1, height), v2(t.v2, height), v3(t.v3, height) {}
    Vector3 v1, v2, v3;
};

#endif // GEOMETRY_H

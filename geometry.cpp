
#include "geometry.h"

bool operator<(const Vector3 a, const Vector3 b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return a.z < b.z;
}

Vector2 operator-(Vector2 v1, Vector2 v2) { return Vector2(v1.x - v2.x, v1.y - v2.y); }
Vector2 operator+(Vector2 v1, Vector2 v2) { return Vector2(v1.x + v2.x, v1.y + v2.y); }
Vector2 operator*(double d, Vector2 v) { return Vector2(d*v.x , d*v.y); }

double dot(Vector2 v1, Vector2 v2) { return v1.x * v2.x + v1.y * v2.y; }
double length(Vector2 v) { return sqrt(dot(v,v));}
Vector2 normalized(Vector2 v) { double len = length(v); return Vector2( v.x/len, v.y/=len);}

double getDistance(Vector2 vLine1, Vector2 vLine2, Vector2 vPos)
{
    Vector2 vLinePos = vLine1;
    Vector2 vLineDir = normalized(vLine2 - vLine1);

    Vector2 v = vLinePos - vPos;
    double dist = length( v - dot(v, vLineDir) * vLineDir);
    return dist;
}

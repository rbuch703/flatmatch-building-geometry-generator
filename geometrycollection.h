#ifndef GEOMETRYCOLLECTION_H
#define GEOMETRYCOLLECTION_H

#include <list>
#include <string>

using namespace std;

struct Vertex3 {
    Vertex3(double x, double y, double z): x(x), y(y), z(z) {}
    double x, y, z;
};

struct Triangle {
    Triangle( Vertex3 v1, Vertex3 v2, Vertex3 v3): v1(v1), v2(v2), v3(v3) {}
    Vertex3 v1, v2, v3;
};

typedef list<Vertex3> LineStrip;

class GeometryCollection
{
public:
    GeometryCollection();

    void mergeInLineStrips( const list<LineStrip> &strips);
    void addLineStrip( const LineStrip &strip) { lineStrips.push_back(strip); }
    void addTriangle( const Triangle &triangle) { faces.push_back(triangle);}

private:
    list<LineStrip> lineStrips;
    list<Triangle> faces;

public:
    string toJson() const;
};

#endif // GEOMETRYCOLLECTION_H

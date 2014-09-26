#ifndef GEOMETRY_H
#define GEOMETRY_H

struct Vertex2 {
    Vertex2(double x, double y): x(x), y(y) {}
    double x, y;
};

struct Vertex3 {
    Vertex3(double x, double y, double z): x(x), y(y), z(z) {}
    Vertex3(Vertex2 v, double z): x(v.x), y(v.y), z(z) {}
    double x, y, z;
};

bool operator<(const Vertex3 a, const Vertex3 b);


struct Triangle3 {
    Triangle3( Vertex3 v1, Vertex3 v2, Vertex3 v3): v1(v1), v2(v2), v3(v3) {}
    Vertex3 v1, v2, v3;
};

struct Triangle2 {
    Triangle2( Vertex2 v1, Vertex2 v2, Vertex2 v3): v1(v1), v2(v2), v3(v3) {}
    Vertex2 v1, v2, v3;
};

#endif // GEOMETRY_H

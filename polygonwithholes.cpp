#include "polygonwithholes.h"
#include <assert.h>
#include <boost/foreach.hpp>


#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <iostream>

using namespace std;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point;
typedef CGAL::Polygon_2<K> Polygon_2;
//typedef CGAL::Vector_2 Vector2;

PolygonWithHoles::PolygonWithHoles( ) {}

bool isClockwise(const PointList &poly)
{
    list<Point> points;
    BOOST_FOREACH(const OsmPoint p, poly)
    {
        points.push_back(Point(p.lat, p.lng ));
        //cout << p.lat << "," << p.lng << endl;
    }
    //cout << endl;

    assert (points.front() == points.back());
    assert(poly.size() > 0);
    points.pop_back(); //remove duplicate closing point

    Polygon_2 pgn(points.begin(), points.end());
    assert( pgn.is_simple() );

    //cout << "Orientation: " << pgn.orientation() << endl;
    return pgn.orientation() == CGAL::CLOCKWISE;
}

struct Vector2 {
    Vector2(): x(0), y(0) {}
    Vector2(double x, double y): x(x), y(y) {}
    double x, y;

};

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

double getDistance(PointList::iterator vLine1, PointList::iterator vLine2, PointList::iterator vPos)
{
    return getDistance( Vector2(vLine1->lat, vLine1->lng),
                        Vector2(vLine2->lat, vLine2->lng),
                        Vector2(vPos  ->lat, vPos  ->lng));
}


void simplifyPolygon( PointList &poly)
{
    PointList::iterator pNext = poly.begin();
    PointList::iterator pPrev = pNext++;
    PointList::iterator pHere = pNext++;

    static const double MAX_DIST = 1E-6;//FIXME: guessed value
    while (pNext != poly.end())
    {
        double dist = getDistance(pPrev, pNext, pHere);
        if (dist < MAX_DIST)
        {
            pHere = poly.erase(pHere);
            pNext++;
        } else
        {
            pNext++;
            pHere++;
            pPrev++;
        }
    }

    //edge case: test for removal of first/last vertex
    pHere = poly.begin();
    pPrev = poly.end();
    --(--pPrev);    //end-1 is last (=first) vertex, end-2 is its predecessor
    pNext = poly.begin();
    pNext++;
    if ( getDistance(pPrev, pNext, pHere) < MAX_DIST)
    {
        /*cout << "Prev:" << pPrev->lat << ", " << pPrev->lng << endl;
        cout << "Here:" << pHere->lat << ", " << pHere->lng << endl;
        cout << "Next:" << pNext->lat << ", " << pNext->lng << endl;*/
        // remove first=last vertex, duplicate new first one to close polygon
        poly.pop_front();
        poly.pop_back();
        poly.push_back(poly.front());
    }

}

PolygonWithHoles::PolygonWithHoles( const PointList &outer, const list<PointList> &holes):
    outer(outer), holes(holes)
{
    /*
    BOOST_FOREACH(const OsmPoint p, outer)
        cout << p.lat << "," << p.lng << endl;
    cout << endl;*/

    simplifyPolygon(this->outer);

    /** CGALs definition for Polygons with holes includes that the outer polygon
     *  has to be oriented CCW, and all holes CW. Since we will use CGAL quite a lot,
     *  make sure the polygon conforms to these requirements on creation.
     */

    if (isClockwise(this->outer))
        this->outer.reverse();

    BOOST_FOREACH(PointList &poly, this->holes)
    {
        if (!isClockwise(poly))
            poly.reverse();
    }

    /*FIXME: add sanity checks:
     * - (done) every PointList has to form a CLOSED and SIMPLE polygon
     * - no two holes overlap each other (otherwise form their union)
     * - every hole is completely inside the outer polygon (= hole minus polygon does not intersect with polygon)
     * - outer polygon is oriented CCW, holes are oriented CW
     */
}


PolygonWithHoles PolygonWithHoles::fromOsmRelation(const OsmRelation &rel)
{
    const PointList *outer = NULL;
    for (list<OsmRelationMember>::const_iterator member = rel.members.begin(); member != rel.members.end(); member++)
    {
        if (member->role == "outer")
        {
            outer = &(member->way.points);
            break;
        }
    }
    assert(outer);

    list<PointList> holes;
    for (list<OsmRelationMember>::const_iterator member = rel.members.begin(); member != rel.members.end(); member++)
    {
        if (&member->way.points == outer)
            continue;

        assert((member->role == "inner" && member->role == "") || "unknown member role");
        holes.push_back( member->way.points);

    }

    return PolygonWithHoles(*outer, holes);
}


/*
string PolygonWithHoles::edgesToJson() const
{
    //FIXME: add code
    return "{}";
}*/

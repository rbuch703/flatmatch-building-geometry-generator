#include "polygonwithholes.h"
#include "geometrycollection.h"
#include <assert.h>

PolygonWithHoles::PolygonWithHoles( const PointList &outer, const list<PointList> &holes, const Tags &tags ):
    outer(outer), holes(holes), tags(tags)
{
    /*FIXME: add sanity checks:
     * - every PointList has to form a CLOSED and SIMPLE polygon
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

    return PolygonWithHoles(*outer, holes, rel.tags);
}


void addEdges(const PointList &poly, list<LineStrip> &edgesOut, double minHeight, double height) {
    LineStrip lower;
    LineStrip upper;

    for (PointList::const_iterator it = poly.begin(); it != poly.end(); it++)
    {
        lower.push_back( Vertex3(it->lat, it->lng, minHeight) );
        upper.push_back( Vertex3(it->lat, it->lng, height   ) );
    }

    edgesOut.push_back(lower);
    edgesOut.push_back(upper);
}

list<LineStrip> PolygonWithHoles::getEdges() const {
    list<LineStrip> edges;
    //FIXME: add actual height
    addEdges( this->outer, edges, 0, 20);

    //FIXME: add actual height
    for (list<PointList>::const_iterator hole = this->holes.begin(); hole != this->holes.end(); hole++)
        addEdges( *hole, edges, 0, 20);

    return edges;
}

/*
string PolygonWithHoles::edgesToJson() const
{
    //FIXME: add code
    return "{}";
}*/

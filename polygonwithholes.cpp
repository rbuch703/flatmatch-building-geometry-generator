#include "polygonwithholes.h"

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

string PolygonWithHoles::edgesToJson() const
{
    //FIXME: add code
    return "{}";
}

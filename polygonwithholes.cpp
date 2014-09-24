#include "polygonwithholes.h"

#include <assert.h>
PolygonWithHoles PolygonWithHoles::fromOsmRelation(const OsmRelation &rel)
{
    const PointList *outer;
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

#ifndef POLYGONWITHHOLES_H
#define POLYGONWITHHOLES_H

#include "osmtypes.h"
#include <list>

class PolygonWithHoles
{
public:
    PolygonWithHoles( );
    PolygonWithHoles( const PointList &outer, const list<PointList> &holes, const Tags &tags ):
        outer(outer), holes(holes), tags(tags) {}

    static PolygonWithHoles fromOsmRelation(const OsmRelation &rel);

private:
    PointList outer;
    list<PointList> holes;
    Tags tags;
};

#endif // POLYGONWITHHOLES_H

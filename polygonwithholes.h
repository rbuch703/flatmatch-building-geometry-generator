#ifndef POLYGONWITHHOLES_H
#define POLYGONWITHHOLES_H

#include "osmtypes.h"
#include "geometry.h"
#include <list>
#include <string>

/** This class represents a 2D polygon with holes. It has no other attributes such as
 *  height, OSM tags, etc.
 */

class PolygonWithHoles
{
public:
    PolygonWithHoles( );
    PolygonWithHoles( const OsmPointList &outer, const list<OsmPointList> &holes, const OsmPoint center);

    list<Triangle2>         triangulate() const;
    list<PointList>         getSkeletonFaces() const;

    static PolygonWithHoles fromOsmRelation(const OsmRelation &rel, OsmPoint center);
    const PointList&        getOuterPolygon() const {return outer;}
    const list<PointList>&  getHoles() const {return holes;}
//    string edgesToJson() const;
private:
    PointList outer;
    list<PointList> holes;
};

#endif // POLYGONWITHHOLES_H

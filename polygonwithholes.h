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
    PolygonWithHoles( const OsmPointList &outer, const list<OsmPointList> &holes, const OsmPoint center, const char* name);

    list<Triangle2>         triangulate() const;
    list<list<Vector3> >    getSkeletonFaces() const;
    list<Triangle3>         triangulateRoof() const;

    const PointList&        getOuterPolygon() const {return outer;}
    const list<PointList>&  getHoles() const {return holes;}

public:
    static PolygonWithHoles fromOsmRelation(OsmRelation rel, OsmPoint center, const char* name);

    //    string edgesToJson() const;
private:
    PointList outer;
    list<PointList> holes;
};

#endif // POLYGONWITHHOLES_H

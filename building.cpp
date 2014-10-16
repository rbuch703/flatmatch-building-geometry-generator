#include "building.h"

#include <sstream>
#include <boost/foreach.hpp>
//#include <list>
#include <map>
#include <vector>
#include <assert.h>
#include <iomanip>  //for std::setprecision

#include <iostream>

using namespace std;
Building::Building()
{
}

Building::Building(PolygonWithHoles layout, BuildingAttributes attributes, string name):
    attributes(attributes), layout(layout), name(name)
{
}

string Building::getName() const { return this->name; }


void addOutlineEdges(const PointList &poly, list<LineStrip> &edgesOut, double minHeight, double height, bool addHorizontalEdges, bool addVerticalEdges) {
    LineStrip lower;
    LineStrip upper;

    list<LineStrip> verticalEdges;

    BOOST_FOREACH(const Vector2 &pt, poly)
    {

        //cout << "##," << it->lat << "," << it->lng << endl;
        lower.push_back( Vector3( pt, minHeight) );
        upper.push_back( Vector3( pt, height   ) );
        if (addVerticalEdges)
        {
            LineStrip verticalEdge;
            verticalEdge.push_back( Vector3(pt, minHeight));
            verticalEdge.push_back( Vector3(pt, height));
            edgesOut.push_back(verticalEdge);
        }
    }
    if (edgesOut.size() > 0)
        edgesOut.pop_back();    //remove last verticalEdge, which is identical to the first one;


    if (addHorizontalEdges)
    {
        edgesOut.push_back(lower);
        edgesOut.push_back(upper);
    }
}

void addOutlineFaces(const PointList &poly, list<Triangle3> &facesOut, double minHeight, double height)
{

    PointList::const_iterator p2 = poly.begin();
    PointList::const_iterator p1 = p2++;

    for (; p2 != poly.end(); p1++,p2++)
    {
        /** B - D
         *  | / |
         *  A - C
         */
        Vector3 A(*p1, minHeight);
        Vector3 B(*p1, height);
        Vector3 C(*p2, minHeight);
        Vector3 D(*p2, height);

        facesOut.push_back( Triangle3(A, B, D));
        facesOut.push_back( Triangle3(A, D, C));
    }
}


list<LineStrip> Building::getEdges() const {
    list<LineStrip> edges;

    //float minHeight = this->attributes.getMinHeight();
    float wallHeight= this->attributes.getHeightWithoutRoof();
    float roofHeight= this->attributes.getRoofHeight();

    //if (this->attributes.isFreeStandingRoof())
    //    minHeight = wallHeight;


    //addOutlineEdges( this->layout.getOuterPolygon(), edges, minHeight, wallHeight, false, !this->attributes.isFreeStandingRoof() );

    //BOOST_FOREACH( const PointList &edge, this->layout.getHoles())
    //    addOutlineEdges( edge, edges, minHeight, wallHeight, true, !this->attributes.isFreeStandingRoof());

    //cout << "Wall height is " << wallHeight << endl;
    //cout << "Roof Height is " << roofHeight << endl;
    list<LineStrip > faces;
    if (roofHeight)
    {
        faces = layout.getSkeletonFaces();
    }

    //no edge faces for flat roofs: their edges have already been added by addOutlineEdges();
    BOOST_FOREACH( LineStrip &face, faces)
    {
        BOOST_FOREACH( Vector3 &point, face)
        {
                point.z = point.z * roofHeight + wallHeight;
                //cout << point.x << ", " << point.y << endl;
        }

        //face.pop_back();
        edges.push_back(face);

        //return edges;
    }

    return edges;
}

list<LineStrip> Building::getOutlines() const {
    list<LineStrip> outlines;

    float height = this->attributes.getHeightWithoutRoof();

    LineStrip outline;
    PointList lst = this->layout.getOuterPolygon();
    BOOST_FOREACH( Vector2 v, lst)
        outline.push_back(Vector3(v, height));

    outlines.push_back(outline);

    BOOST_FOREACH( const PointList &hole, this->layout.getHoles())
    {
        LineStrip outline;
        BOOST_FOREACH( Vector2 v, hole)
            outline.push_back(Vector3(v, height));

        outlines.push_back(outline);
    }

    return outlines;
}



list<Triangle3> Building::getFaces() const {
    list<Triangle3> faces;

    //float minHeight = this->attributes.getMinHeight();
    float wallHeight= this->attributes.getHeightWithoutRoof();
    float roofHeight= this->attributes.getRoofHeight();

    /*
    if (!this->attributes.isFreeStandingRoof())
    {
        addOutlineFaces( this->layout.getOuterPolygon(), faces, minHeight, wallHeight);

        BOOST_FOREACH( const PointList &hole, this->layout.getHoles())
            addOutlineFaces( hole, faces, minHeight, wallHeight);
    }*/


    /// add flat roof triangulation
    list<Triangle3> tris;

    if (roofHeight != 0.0)
    {
        tris = layout.triangulateRoof();
    }
    else
    {

        //cerr << "triangulating roof for " << this->getName() << endl;
        list<Triangle2> tris2d = layout.triangulate();
        BOOST_FOREACH(Triangle2 tri, tris2d)
        {
            //swap 2nd and 3rd vertex to invert surface normal
            tris.push_back(Triangle3( Vector3(tri.v1, 0),
                                      Vector3(tri.v3, 0),
                                      Vector3(tri.v2, 0))); //flat roof -> zero height
        }
    }

    float minHeight = attributes.getMinHeight();
    if (attributes.isFreeStandingRoof())
        minHeight = attributes.getHeightWithoutRoof();

    //cerr << "minHeight is " << minHeight << endl;
    //cerr << "roofHeight is " << roofHeight << endl;
    if (minHeight > 0)  //triangulate lower surface
    {
        //cerr << "triangulating lower side for " << this->getName() << endl;
        list<Triangle2> tris2d = layout.triangulate();
        BOOST_FOREACH(Triangle2 tri, tris2d)
            faces.push_back(Triangle3( tri, minHeight));
    }

    /** So far, the roof height is just normalized to the interval [0..1];
     *  So we still have to scale it according to its actual height, and move
     *  it atop the walls
    */
    BOOST_FOREACH( Triangle3 tri, tris)
    {
        //swap two vertices to change vertex orientation
        /*Triangle3 t( Vector3(tri.v1, totalHeight),
                     Vector3(tri.v3, totalHeight),
                     Vector3(tri.v2, totalHeight));*/

        tri.v1.z = tri.v1.z * roofHeight + wallHeight;
        tri.v2.z = tri.v2.z * roofHeight + wallHeight;
        tri.v3.z = tri.v3.z * roofHeight + wallHeight;
        faces.push_back(tri);
    }

    return faces;
}

/**
 * @brief getVertexId returns the position of 'v' in 'vertices', adding 'v' to 'vertices' if it is not already contained.
 * @param v the vertex whose id is to be determined
 * @param vertices the list of all already registered vertices
 * @param vertexIds a dictionary mapping all registered vertices to their respective id
 * @return the vertex id of v
 */
int getVertexId( Vector3 v, list<Vector3> &vertices, map<Vector3, int> &vertexIds)
{
    if (vertexIds.count(v) == 0)
    {
        vertexIds.insert(make_pair(v, vertexIds.size()));
        vertices.push_back(v);
    }
    return vertexIds[v];
}

bool Building::hasNonZeroHeight() const
{
    return this->attributes.getTotalHeight() > 0;
}

string Building::toJSON(const OsmPoint &center) const {
    stringstream ss(ios_base::out);

    list<Vector3> vertices;
    map<Vector3, int> vertexIds;
    /*if (this->attributes.getRoofColor()!= "" || this->attributes.getWallColor() != "")
    {

    }*/

    ss << "{" << endl;
    ss << "\t\"id\":\"" << this->name << "\"," << endl;
    float minHeight = this->attributes.isFreeStandingRoof() ?
                        this->attributes.getHeightWithoutRoof() :
                        this->attributes.getMinHeight();

    ss << "\t\"minHeightInMeters\":" << minHeight << "," << endl;
    ss << "\t\"heightWithoutRoofInMeters\":" << this->attributes.getHeightWithoutRoof() << ", " << endl;
    ss << "\t\"numLevels\":" << this->attributes.getNumLevels() << "," << endl;
    Vector3 wc = this->attributes.getWallColor();
    ss << std::setprecision(3);
    ss << "\t\"wallColor\":[" << wc.x << "," << wc.y << "," << wc.z << "]," << endl;
    Vector3 rc = this->attributes.getRoofColor();
    ss << "\t\"roofColor\":[" << rc.x << "," << rc.y << "," << rc.z << "]," << endl;

    /// ====== store drawable edges =====
    ss << "\t\"edges\":[";

    bool isFirstEdge = true;
    BOOST_FOREACH(const LineStrip &edge, this->getEdges())
    {
        ss << (!isFirstEdge? ",":"") << "[";
        isFirstEdge = false;
        bool isFirstVertex = true;
        BOOST_FOREACH(Vector3 v, edge)
        {
            if (!isFirstVertex)
                ss << ",";
            isFirstVertex = false;
            ss << getVertexId(v, vertices, vertexIds);
        }

        ss << "]";
    }
    ss << "]," << endl;

    /// ====== store building outlines (outer polygon and all holes) =====
    ss << "\t\"outlines\": [";

    bool isFirstOutline = true;
    BOOST_FOREACH(const LineStrip &edge, this->getOutlines())
    {
        ss << (!isFirstOutline ? ",":"") << "[";
        isFirstOutline = false;
        bool isFirstVertex = true;
        BOOST_FOREACH(Vector3 v, edge)
        {
            if (!isFirstVertex)
                ss << ",";
            isFirstVertex = false;
            ss << getVertexId(v, vertices, vertexIds);
        }

        ss << "]";
    }


    ss << "]," << endl;
    /// ====== store faces =====
    ss << "\t\"faces\":[";

    bool isFirstFace = true;
    BOOST_FOREACH(Triangle3 tri, this->getFaces())
    {
        if (!isFirstFace)
            ss << ",";
        isFirstFace = false;

        ss << getVertexId(tri.v1, vertices, vertexIds) << ","
           << getVertexId(tri.v3, vertices, vertexIds) << ","
           << getVertexId(tri.v2, vertices, vertexIds);
    }


    ss << "]," << endl;

    ss << "\t\"vertices\": [";

    assert(vertexIds.size() == vertices.size());

    bool isFirstVertex = true;
    BOOST_FOREACH(Vector3 v, vertices)
    {
        if (!isFirstVertex)
            ss << ",";
        isFirstVertex = false;

        static const double EARTH_CIRCUMFERENCE = 2 * 3.141592 * (6378.1 * 1000); // [m]
        double latToYFactor = 1/360.0 * EARTH_CIRCUMFERENCE;
        double lngToXFactor = 1/360.0 * EARTH_CIRCUMFERENCE * cos( center.lat / 180 * 3.141592);

        double lng = v.x / lngToXFactor + center.lng;
        double lat = v.y / latToYFactor + center.lat;
        // precision of 8 decimal places has been tested experimentally to be sufficient
        ss << std::setprecision(8);
        ss << "[" << lat << "," << lng << "," << v.z << "]";
    }

    ss << "]" << endl;


    ss << "}";

    return ss.str();
}

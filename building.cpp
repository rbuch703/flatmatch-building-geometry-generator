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


void addOutlineEdges(const PointList &poly, list<LineStrip> &edgesOut, double minHeight, double height) {
    LineStrip lower;
    LineStrip upper;

    list<LineStrip> horizontalEdges;

    BOOST_FOREACH(const Vector2 &pt, poly)
    {

        //cout << "##," << it->lat << "," << it->lng << endl;
        lower.push_back( Vector3( pt, minHeight) );
        upper.push_back( Vector3( pt, height   ) );
        LineStrip horizontalEdge;
        horizontalEdge.push_back( Vector3(pt, minHeight));
        horizontalEdge.push_back( Vector3(pt, height));
        edgesOut.push_back(horizontalEdge);
    }
    edgesOut.pop_back();    //remove last horizontalEdge, which is identical to the first one;

    edgesOut.push_back(lower);
    edgesOut.push_back(upper);
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


//FIXME: add back-conversion
list<LineStrip> Building::getEdges() const {
    list<LineStrip> edges;

    float minHeight = this->attributes.getMinHeight();
    float totalHeight=this->attributes.getTotalHeight();

    addOutlineEdges( this->layout.getOuterPolygon(), edges, minHeight, totalHeight);

    BOOST_FOREACH( const PointList &edge, this->layout.getHoles())
        addOutlineEdges( edge, edges, minHeight, totalHeight);

    list<PointList> faces = layout.getSkeletonFaces();
    BOOST_FOREACH( const PointList &face, faces)
    {
        LineStrip strip;
        BOOST_FOREACH( Vector2 point, face)
        {
            strip.push_back( Vector3(point, totalHeight));
        }

        edges.push_back(strip);
    }

    return edges;
}

list<Triangle3> Building::getFaces() const {
    list<Triangle3> faces;

    float minHeight = this->attributes.getMinHeight();
    float totalHeight=this->attributes.getTotalHeight();

    addOutlineFaces( this->layout.getOuterPolygon(), faces, minHeight, totalHeight);

    BOOST_FOREACH( const PointList &hole, this->layout.getHoles())
        addOutlineFaces( hole, faces, minHeight, totalHeight);

    /// add flat roof triangulation
    list<Triangle2> tris = layout.triangulate();
    BOOST_FOREACH( const Triangle2 &tri, tris)
    {
        //swap two vertices to change vertex orientation
        Triangle3 t( Vector3(tri.v1, totalHeight),
                     Vector3(tri.v3, totalHeight),
                     Vector3(tri.v2, totalHeight));

        faces.push_back(t);
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


string Building::toJSON(const OsmPoint &center) const {
    stringstream ss(ios_base::out);

    list<Vector3> vertices;
    map<Vector3, int> vertexIds;

    ss << "{" << endl;
    ss << "\t\"id\": \"" << this->name << "\"," << endl;

    ss << "\t\"edges\": [";
    //list<LineStrip> edges = ;

    bool isFirstEdge = true;
    BOOST_FOREACH(const LineStrip &edge, this->getEdges())
    {
        ss << (!isFirstEdge? ",":"") << endl << "\t\t[";
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

    assert(vertexIds.size() == vertices.size());
    ss << endl << "\t]," << endl;

    ss << "\t\"faces\": [";

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

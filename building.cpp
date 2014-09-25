#include "building.h"

#include <sstream>
#include <boost/foreach.hpp>
//#include <list>
#include <map>
#include <vector>
#include <assert.h>
#include <iomanip>  //for std::setprecision

Building::Building()
{
}

Building::Building(PolygonWithHoles layout, BuildingAttributes attributes, string name):
    attributes(attributes), layout(layout), name(name)
{
}

void addEdges(const PointList &poly, list<LineStrip> &edgesOut, double minHeight, double height) {
    LineStrip lower;
    LineStrip upper;

    list<LineStrip> horizontalEdges;

    for (PointList::const_iterator it = poly.begin(); it != poly.end(); it++)
    {
        //cout << "##," << it->lat << "," << it->lng << endl;
        lower.push_back( Vertex3(it->lat, it->lng, minHeight) );
        upper.push_back( Vertex3(it->lat, it->lng, height   ) );
        LineStrip horizontalEdge;
        horizontalEdge.push_back( Vertex3(it->lat, it->lng, minHeight));
        horizontalEdge.push_back( Vertex3(it->lat, it->lng, height));
        edgesOut.push_back(horizontalEdge);
    }
    edgesOut.pop_back();    //remove last horizontalEdge, which is identical to the first one;
    //exit(0);
    edgesOut.push_back(lower);
    edgesOut.push_back(upper);
}

void addFaces(const PointList &poly, list<Triangle> &facesOut, double minHeight, double height) {

    PointList::const_iterator p2 = poly.begin();
    PointList::const_iterator p1 = p2++;

    for (; p2 != poly.end(); p1++,p2++)
    {
        /** B - D
         *  | / |
         *  A - C
         */
        Vertex3 A(p1->lat, p1->lng, minHeight);
        Vertex3 B(p1->lat, p1->lng, height);
        Vertex3 C(p2->lat, p2->lng, minHeight);
        Vertex3 D(p2->lat, p2->lng, height);

        facesOut.push_back( Triangle(A, D, B));
        facesOut.push_back( Triangle(A, C, D));
    }
}



list<LineStrip> Building::getEdges() const {
    list<LineStrip> edges;

    float minHeight = this->attributes.getMinHeight();
    float totalHeight=this->attributes.getTotalHeight();

    addEdges( this->layout.getOuterPolygon(), edges, minHeight, totalHeight );

    BOOST_FOREACH( const PointList &edge, this->layout.getHoles())
        addEdges( edge, edges, minHeight, totalHeight);

    return edges;
}

list<Triangle> Building::getFaces() const {
    list<Triangle> faces;

    float minHeight = this->attributes.getMinHeight();
    float totalHeight=this->attributes.getTotalHeight();

    addFaces( this->layout.getOuterPolygon(), faces, minHeight, totalHeight );

    BOOST_FOREACH( const PointList &hole, this->layout.getHoles())
        addFaces( hole, faces, minHeight, totalHeight);

    return faces;
}

/**
 * @brief getVertexId returns the position of 'v' in 'vertices', adding 'v' to 'vertices' if it is not already contained.
 * @param v the vertex whose id is to be determined
 * @param vertices the list of all already registered vertices
 * @param vertexIds a dictionary mapping all registered vertices to their respective id
 * @return the vertex id of v
 */
int getVertexId( Vertex3 v, list<Vertex3> &vertices, map<Vertex3, int> &vertexIds)
{
    if (vertexIds.count(v) == 0)
    {
        vertexIds.insert(make_pair(v, vertexIds.size()));
        vertices.push_back(v);
    }
    return vertexIds[v];
}


string Building::toJSON() const {
    stringstream ss(ios_base::out);

    list<Vertex3> vertices;
    map<Vertex3, int> vertexIds;

    ss << "{" << endl;
    ss << "\t\"id\":" << this->name << "," << endl;

    ss << "\t\"edges\": [";
    //list<LineStrip> edges = ;

    bool isFirstEdge = true;
    BOOST_FOREACH(const LineStrip &edge, this->getEdges())
    {
        ss << (!isFirstEdge? ",":"") << endl << "\t\t[";
        isFirstEdge = false;
        bool isFirstVertex = true;
        BOOST_FOREACH(Vertex3 v, edge)
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
    BOOST_FOREACH(Triangle tri, this->getFaces())
    {
        if (!isFirstFace)
            ss << ",";
        isFirstFace = false;

        ss << getVertexId(tri.v1, vertices, vertexIds) << ","
           << getVertexId(tri.v2, vertices, vertexIds) << ","
           << getVertexId(tri.v3, vertices, vertexIds);
    }


    ss << "]," << endl;

    ss << "\t\"vertices\": [";

    bool isFirstVertex = true;
    BOOST_FOREACH(Vertex3 v, vertices)
    {
        if (!isFirstVertex)
            ss << ",";
        isFirstVertex = false;

        // precision of 8 decimal places has been tested experimentally to be sufficient
        ss << std::setprecision(8);
        ss << "[" << v.x << "," << v.y << "," << v.z << "]";
    }

    ss << "]" << endl;


    ss << "}";

    return ss.str();
}

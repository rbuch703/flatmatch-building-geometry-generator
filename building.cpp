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
    //exit(0);
    edgesOut.push_back(lower);
    edgesOut.push_back(upper);
}

list<LineStrip> Building::getEdges() const {
    list<LineStrip> edges;
    //FIXME: add actual height
    addEdges( this->layout.getOuterPolygon(), edges, 0, 20);

    //FIXME: add actual height
    BOOST_FOREACH( const PointList &edge, this->layout.getHoles())
        addEdges( edge, edges, 0, 20);

    return edges;
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
            if (vertexIds.count(v) == 0)
            {
                vertexIds.insert(make_pair(v, vertexIds.size()));
                vertices.push_back(v);
            }
            ss << vertexIds[v];
        }

        ss << "]";
    }

    assert(vertexIds.size() == vertices.size());
    ss << endl << "\t]," << endl;

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


    ss << "}" << endl;

    return ss.str();
}

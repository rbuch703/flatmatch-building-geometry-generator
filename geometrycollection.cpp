#include "geometrycollection.h"
#include <sstream>
#include <boost/foreach.hpp>

using namespace std;

GeometryCollection::GeometryCollection()
{
}

void GeometryCollection::mergeInLineStrips( const list<LineStrip> &strips)
{
    for (list<LineStrip>::const_iterator strip = strips.begin(); strip != strips.end(); strip++)
        this->lineStrips.push_back(*strip);
}

string GeometryCollection::toJson() const {
    stringstream ss(ios_base::out);

    ss << "{" << endl;
    ss << "  \"edges\": [";

    //for (list<LineStrip>::const_iterator strip = lineStrips.begin(); strip != lineStrips.end(); strip++)
    bool isFirstEdge = true;
    BOOST_FOREACH( const LineStrip &strip, this->lineStrips)
    {
        if (!isFirstEdge)
            ss << "," << endl;
        else
            isFirstEdge = false;

        ss << "[";

        bool isFirstVertex = true;
        BOOST_FOREACH( const Vertex3 &v, strip)
        {
            if (!isFirstVertex)
                ss << ",";
            else
                isFirstVertex = false;

            ss << v.x << "," << v.y << "," << v.z << endl;
        }

        ss << "]";
    }

    ss << "]" << endl;


    ss << "}" << endl;

    return ss.str();

}


/** structure of JSON output
 * root = {
 *   (vertices: [v1, v2, ...])
 *   edges: [
 *      - [ strip 1 data],
 *      - [ strip 2 data],
 *      - ...
 *   ],
 *   faces = [ tri1, tri2, ...],
 * }
 * */

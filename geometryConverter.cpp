#include <iostream>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "geometryConverter.h"

#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <map>
#include <string>
#include <set>

extern QNetworkReply *reply;

typedef pair<string, string> OsmKeyValuePair;

struct OsmPoint {
    OsmPoint(): lat(nan("")), lng(nan("")) {}
    OsmPoint(double lat, double lng): lat(lat), lng(lng) {}
    double lat, lng;

};

bool operator==(const OsmPoint &p1, const OsmPoint &p2) { return p1.lat == p2.lat && p1.lng == p2.lng;}
bool operator!=(const OsmPoint &p1, const OsmPoint &p2) { return p1.lat != p2.lat || p1.lng != p2.lng;}

struct OsmWay
{
    OsmWay( uint64_t id = 0): id(id) {}

    string getName() const {
        if (name.length()) return name;
        return QString::number(id).toStdString();
    }
    /*OsmWay( uint64_t way_id, list<uint64_t> way_refs = list<uint64_t>,
            list<OSMKeyValuePair> way_tags = list<OSMKeyValuePair>() ):
        id(way_id), refs(way_refs), tags(way_tags)
    {

    }

    bool hasKey(string key) const;
    const string &getValue(string key) const;
    const string &operator[](string key) const {return getValue(key);}*/

    uint64_t id;
    string name;
    list<OsmPoint> points;
    map<string, string> tags;
};

struct OsmRelationMember {
    OsmWay way;
    string role;
};

struct OsmRelation
{
    OsmRelation( uint64_t id = 0): id(id) {}

    uint64_t id;
    map<string, string> tags;
    list< OsmRelationMember  >members;
};

QString getDepthString(int depth)
{
    QString s = "";
    while (depth--)
        s+="  ";
    return s;
}

void parseJsonObject(QJsonObject object, int depth);
void parseJsonValue(QJsonValue val, int depth);

#define INDENT getDepthString(depth).toStdString()

void parseJsonArray(QJsonArray array, int depth = 0)
{
    for (QJsonArray::const_iterator it = array.begin(); it != array.end(); it++)
    {
        cout << INDENT;
        parseJsonValue(*it, depth+1);
    }

}

void parseJsonObject(QJsonObject object, int depth = 0)
{
    QStringList keys = object.keys();
    for (QStringList::const_iterator it = keys.begin(); it != keys.end(); it++)
    {
        cout << INDENT << (*it).toStdString() << " = ";
        QJsonValue val = object.value(*it);
        parseJsonValue(val, depth+1);
    }
}

void parseJsonValue(QJsonValue val, int depth = 0)
{
    if (val.isArray())
    {
        cout << "[" << endl;
        parseJsonArray(val.toArray(), depth+1);
        cout << INDENT << "]" << endl;
    }

    if (val.isBool())
        cout << (val.toBool() ? "true":"false") << endl;

    if (val.isDouble())
        cout << val.toDouble() << endl;

    if (val.isNull())
        cout << "<NULL>" << endl;

    if (val.isObject())
    {
        cout << "  {" << endl;
        parseJsonObject(val.toObject(), depth+1);
        cout << INDENT << "}" << endl;
    }

    if (val.isString())
    {
        cout << "'" << val.toString().toStdString() << "'" << endl;
    }

}

// this method does not parse node tags, as those are irrelevant to our use case
map<uint64_t, OsmPoint > getPoints(QJsonArray elements)
{
    map<uint64_t, OsmPoint > points;
    for (QJsonArray::const_iterator el = elements.begin(); el != elements.end(); el++)
    {
        QJsonObject obj = (*el).toObject();
        if (obj["type"].toString() != "node")
            continue;

        uint64_t id = obj["id"].toDouble();
        double lat = obj["lat"].toDouble();
        double lng = obj["lon"].toDouble();
        OsmPoint point(lat, lng);

        assert(!points.count(id));
        //if (!nodes.count(id))
            points.insert( make_pair(id, point) );
    }
    return points;
}

map<string, string> getTags(QJsonObject tags)
{
    map<string, string> res;
    for (QJsonObject::const_iterator it = tags.begin(); it != tags.end(); it++)
    {
        QString key = it.key();
        string value = it.value().toString().toStdString();
        //cout << "\t" << key.toStdString() << "=" << value <<  endl;
        assert( res.count(key.toStdString()) == 0);
        res.insert( make_pair(key.toStdString(), value));
    }
    return res;
}

map<uint64_t, OsmRelation> getRelations(QJsonArray elements, map<uint64_t, OsmWay> &ways)
{
    map<uint64_t, OsmRelation > relations;
    // set of way_ids of all ways that are part of relations, and thus will not be treated as dedicated geometry
    // We flag all ways belonging to relations using this set, and remove all those ways from the map of ways at
    // the end of this method. A direct removal is not possible, since a way may be part of several relations (
    // so if it was remove right after a single relation was foudn that contains it, it would be missing when
    // parsing other relations that may contain it as well.
    set<uint64_t> waysInRelations;

    for (QJsonArray::const_iterator el = elements.begin(); el != elements.end(); el++)
    {
        QJsonObject obj = (*el).toObject();
        if (obj["type"].toString() != "relation")
            continue;

        OsmRelation rel(obj["id"].toDouble());
        rel.tags = getTags(obj["tags"].toObject());

        QJsonArray jNodes = obj["members"].toArray();
        for (QJsonArray::const_iterator it = jNodes.begin(); it != jNodes.end(); it++)
        {
            QJsonObject member = (*it).toObject();
            QString type = member["type"].toString();
            OsmRelationMember m;
            uint64_t way_id = member["ref"].toDouble();
            m.role  = member["role"].toString().toStdString();

            if ( type == "node") //don't need individual nodes of relations, skip them silently
                continue;
            /* Cascaded relations may hold relevant information. But their semantics are not standardized,
             * and their processing would relatively complex. So ignore them for now.
             */
            if (type == "relation")
            {
                cout << "[INFO] skipping sub-relation " << way_id << " of relation " << rel.id << endl;
                continue;
            }
            assert(type == "way");

            if (ways.count(way_id) == 0)
            {
                cout << "[WARN] member way " << way_id << " of relation " << rel.id
                     << " is not part of JSON response, ignoring..." << endl;
                continue;
            }
            waysInRelations.insert(way_id);
            m.way = ways[way_id];
            if (m.way.points.front() != m.way.points.back())
            {
                cout << "[WARN] non-closed way " << way_id << " is part of relation " << rel.id << ". This is unsupported" <<endl;
            }
            rel.members.push_back(m);
        }

        //cout << "relation " << rel.id << " has " << rel.members.size() << " way members" << endl;

        assert(relations.count(rel.id) == 0);
        relations.insert(make_pair(rel.id, rel) );
    }
    cout << "[INFO] parsed " << relations.size() << " relations" << endl;

    cout << "[DBG] removing " << waysInRelations.size() << " ways that are part of relations" << endl;
    for (set<uint64_t>::const_iterator it = waysInRelations.begin(); it != waysInRelations.end(); it++)
        ways.erase(*it);
    return relations;
}

map<uint64_t, OsmWay> getWays(QJsonArray elements, const map<uint64_t, OsmPoint> &points)
{
    map<uint64_t, OsmWay> ways;
    for (QJsonArray::const_iterator el = elements.begin(); el != elements.end(); el++)
    {
        QJsonObject obj = (*el).toObject();
        if (obj["type"].toString() != "way")
            continue;
        //parseJsonObject( obj, 1);

        OsmWay way(obj["id"].toDouble());

        QJsonArray jNodes = obj["nodes"].toArray();
        for (QJsonArray::const_iterator it = jNodes.begin(); it != jNodes.end(); it++)
        {
            assert ( (*it).isDouble() == true);
            uint64_t nodeId = (*it).toDouble();
            assert( points.count(nodeId) > 0);
            way.points.push_back(points.at(nodeId));
        }

        way.tags = getTags(obj["tags"].toObject());

        if (ways.count(way.id) == 0)
            ways.insert( make_pair(way.id, way));
        else
        /* 'way' is present twice in the JSON response. This may happen when a way is at the same time
         * a building *and* a member of a relation that is itself a building. However, it seems that
         * in the latter case the way is stripped of its tags. So we need to make sure that we keep the
         * right version of a duplicate way
         */
        {
            assert( ways[way.id].tags.size() == 0 || way.tags.size() == 0);
            if (ways[way.id].tags.size() == 0)
                ways[way.id] = way;
        }
        //cout << "way " << way.id << " has " << way.points.size() << " nodes" << endl;

    }

    cout << "[INFO] parsed " << ways.size() << " ways" << endl;
    return ways;
}

/* Multipolygon-Relations in OSM may consist of several line segments (ways) of different roles:
 * These line segments may represent whole or partial outlines or holes in the multipolygon.
 * This method merges those ways that represent partial holes or outlines, so that all
 * (merged) ways are closed polygons
*/
#if 0
void mergeRelationWays(map<uint64_t, OsmRelation> &relations)
{
    //rel.outlines = [];

    var currentOutline = null;
    for (var j in rel.members)
    {
        var way = rel.members[j];
        delete rel.members[j];
        /* if we currently have an open outline segment, then this next ways must be connectable
         * to that outline. If not then we have to fallback to close that open outline segment with a
         * straight line (which is usually not the intended result), store it, and continue with the next one
        */
        if (currentOutline)
        {
            var res = Buildings.joinWays(currentOutline, way);
            if (res) //join succeeded
                currentOutline = res;
            else //join failed --> force-close old outline
            {
                console.log("Force-closed way %s in relation %s", currentOutline.ref.id, rel.id)

                //if it is already closed, it should not be currentOutline in the first place
                if (currentOutline.ref.nodes[0] == currentOutline.ref.nodes[currentOutline.ref.nodes.length-1])
                    console.log("BUG: current outline should not be closed (at relation %s)", rel.id);
                else
                    currentOutline.ref.nodes.push(currentOutline.ref.nodes[0]);

                rel.outlines.push(currentOutline);
                currentOutline = way;
            }
        } else
        {
            currentOutline = way;
        }

        //if currentOutline is closed, it is complete and can be stored away
        if (currentOutline)
        {
            //console.log(currentOutline);
            if (currentOutline.ref.nodes[0] == currentOutline.ref.nodes[currentOutline.ref.nodes.length-1])
            {
                rel.outlines.push(currentOutline);
                currentOutline = null;
            }
        }
    }

    /*if there is an open outline left, we have to force-close it (since there is no other segment left
     *to close it with) using a straight line */
    if (currentOutline)
    {
        if (currentOutline.ref.nodes[0] == currentOutline.ref.nodes[currentOutline.ref.nodes.length-1])
            console.log("BUG: current outline should not be closed (at relation %s)", rel.id);
        else
            currentOutline.ref.nodes.push(currentOutline.ref.nodes[0]);

        rel.outlines.push(currentOutline);
        currentOutline = null;
    }

}
#endif

void GeometryConverter::onDownloadFinished()
{
    cout << "Download Complete" << endl;


    if (reply->error() > 0) {
        cout << "Error" << endl;
        cout << reply->errorString().toStdString();
    }
    else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        assert( ! doc.isNull());
        //cout << ( doc.isNull() ? "invalid":"valid") << endl;
        //QJsonObject obj = doc.object();
        QJsonArray elements = doc.object()["elements"].toArray();
        map<uint64_t, OsmPoint > nodes = getPoints(elements);
        cout << "parsed " << nodes.size() << " nodes" << endl;

        map<uint64_t, OsmWay> ways = getWays(elements, nodes);
        map<uint64_t, OsmRelation> relations = getRelations(elements, ways);

        //mergeRelationWays(relations);

        cout << "Emitting 'done' signal" << endl;
        emit done();
    }

}


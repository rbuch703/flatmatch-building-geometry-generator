#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <iostream>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <map>
#include <string>
#include <set>

#include "geometryConverter.h"
#include "osmtypes.h"
#include "polygonwithholes.h"
#include "buildingattributes.h"
#include "building.h"


QNetworkReply *reply;


QString getDepthString(int depth)
{
    QString s = "";
    while (depth--)
        s+="  ";
    return s;
}

/* this method does not parse node tags, as those are irrelevant to our use case.
 * It only parses lat/lon coordinates, hence the name 'points' instead of 'nodes'
 */
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

map<string, string> getTags(QJsonObject tagsObject)
{
    map<string, string> res;
    for (QJsonObject::const_iterator it = tagsObject.begin(); it != tagsObject.end(); it++)
    {
        QString key = it.key();
        string value = it.value().toString().toStdString();
        //cout << "\t" << key.toStdString() << "=" << value <<  endl;
        assert( res.count(key.toStdString()) == 0);
        res.insert( make_pair(key.toStdString(), value));
    }
    return res;
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
        map<string, uint64_t> roles;

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

            if (roles.count(m.role) == 0)
                roles.insert(make_pair(m.role, 0));
            roles[m.role] += 1;

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

        //assert(roles["outer"] == 1);    //exactly one 'outer' way
        //cout << "relation " << rel.id << " has " << rel.members.size() << " way members" << endl;
        if (roles["outer"] != 1)
            cout << "[WARN] relation " <<rel.id << " has more than one 'outer' member. This is currently unsupported" << endl;

        //for (map<string, uint64_t>::const_iterator role =roles.begin(); role != roles.end(); role++)
        //    cout << "\t" << (*role).first << "(" << (*role).second << ")" << endl;

        assert(relations.count(rel.id) == 0);
        relations.insert(make_pair(rel.id, rel) );
    }
    cout << "[INFO] parsed " << relations.size() << " relations" << endl;

    cout << "[DBG] removing " << waysInRelations.size() << " ways that are part of relations" << endl;
    for (set<uint64_t>::const_iterator it = waysInRelations.begin(); it != waysInRelations.end(); it++)
        ways.erase(*it);
    return relations;
}


/* In some (technically invalid) cases, an OSM multipolygon has few tags or even no tags at all,
 * and the tags describing it instead are part of its only 'outer' member. This method copies
 * relevant tags from those 'outer' members to the relation itself. This is usually caused when a
 * user of the OSM web editor promotes a way to a relation, but is not aware that he has to move
 * the tags manually.
 *
 * */
void promoteTags(OsmRelation &relation)
{
    for (list<OsmRelationMember>::const_iterator it = relation.members.begin(); it != relation.members.end(); it++)
    {
        if (it->role != "outer")
            continue;

        const OsmWay &way = it->way;
        for (map<string, string>::const_iterator tag = way.tags.begin(); tag != way.tags.end(); tag++)
        {
            if (relation.tags.count(tag->first) == 0)
            {
                assert(false && "untested code");
                relation.tags.insert(*tag);
                cout << "[DBG] adding " << (tag->first) << "=" << tag->second << " to relation" << relation.id << endl;
            }
        }

    }
}

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

        //list<PolygonWithHoles> polygons;
        list<Building> buildings;
        for (map<uint64_t, OsmRelation>::iterator rel = relations.begin(); rel != relations.end(); rel++)
        {
            promoteTags(rel->second);
            buildings.push_back( Building(
                PolygonWithHoles::fromOsmRelation(rel->second),
                BuildingAttributes( rel->second.tags ),
                string("\"r")+QString::number(rel->second.id).toStdString()+"\"" ));
        }
        /** Fixme: all geometry processing (simplification, roof construction, ...) requires an Euclidean
         *        Coordinate system. But OSM data is given as lat/lng pairs, which are not Euclidean (
         *        the mapping from lng to meters changes with lat). Thus, we need to convert all lat/lng
         *        data to local Euclidean coordinates, and convert them back before the JSON export.
         *
         */

        for (map<uint64_t, OsmWay>::const_iterator way = ways.begin(); way != ways.end(); way++)
            buildings.push_back( Building(
                PolygonWithHoles(way->second.points, list<PointList>()),
                BuildingAttributes ( way->second.tags),
                string("\"w")+QString::number(way->second.id).toStdString()+"\"" ));

            //polygons.push_back( PolygonWithHoles(way->second.points, list<PointList>()));

        cout << "[DBG] unified geometry to " << buildings.size() << " buildings." << endl;

        cerr << "[" << endl;
        bool isFirstBuilding = true;
        for (list<Building>::const_iterator it = buildings.begin(); it != buildings.end(); it++)
        {
            if (!isFirstBuilding)
                cerr << ",";

            isFirstBuilding = false;
            cerr << it->toJSON() << endl;
        }
        cerr << "]" << endl;
        /*
        GeometryCollection geometry;

        for (list<PolygonWithHoles>::const_iterator it = polygons.begin(); it != polygons.end(); it++)
        {
            list<LineStrip> lineStrips = it->getEdges();
            geometry.mergeInLineStrips( lineStrips );
        }

        cerr << geometry.toJson() << endl;*/



        /** "Tracer Bullet"
         * - JSON-Geometry für Wände ausgeben
          */
        //mergeRelationWays(relations);

        cout << "Emitting 'done' signal" << endl << endl;
        emit done();
    }

}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);   //initialize infrastructure for Qt event loop.
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    //QObject::connect(manager, SIGNAL(finished(QNetworkReply *)),
    //                             SLOT(slotRequestFinished(QNetworkReply *)));

    QNetworkRequest request;
    QString buildingsAtFlatViewDefaultLocation = "http://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3B(way%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Bway%5B%22building%3Apart%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Brelation%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208))%3Bout%20body%3B%3E%3Bout%20skel%20qt%3B";
    QString buildingRelationsInMagdeburg = "http://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3Brelation%5B%22building%22%5D%2852%2E059034798886984%2C11%2E523628234863281%2C52%2E19519199255819%2C11%2E765155792236326%29%3Bout%20body%3B%3E%3Bout%20skel%20qt%3B";
    QString someBuildingsInMagdeburg = "http://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3B(way%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Bway%5B%22building%3Apart%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Brelation%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208))%3Bout%20body%3B%3E%3Bout%20skel%20qt%3B";
    //request.setUrl(QUrl());
    request.setUrl(someBuildingsInMagdeburg);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    reply = manager->get(request);
    GeometryConverter * dummy = new GeometryConverter();

    QObject::connect(reply, SIGNAL(finished()), dummy, SLOT(onDownloadFinished()));
    //                           SLOT(slotProgress(qint64,qint64)));

    QObject::connect(dummy, SIGNAL(done()), &app, SLOT(quit()) );
    //                           SLOT(slotProgress(qint64,qint64)));

    return app.exec();

}


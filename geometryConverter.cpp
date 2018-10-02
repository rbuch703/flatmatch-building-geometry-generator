#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QRegExp>

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

bool isCgiMode = false;
int tileX, tileY;


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

    cerr << ", " << ways.size() << " ways";
    cerr.flush();
    return ways;
}


map<uint64_t, OsmRelation> getRelations(QJsonArray elements, map<uint64_t, OsmWay> &ways,
                                        set<uint64_t> &wayBlacklist, set<uint64_t> &relationBlacklist)
{
    map<uint64_t, OsmRelation > relations;
    // set of way_ids of all ways that are part of relations, and thus will not be treated as dedicated geometry
    // We flag all ways belonging to relations using this set, and remove all those ways from the map of ways at
    // the end of this method. A direct removal is not possible, since a way may be part of several relations (
    // so if it was remove right after a single relation was found that contains it, it would be missing when
    // parsing other relations that may contain it as well.
    set<OsmRelationMember> multipolygonWays;

    for (QJsonArray::const_iterator el = elements.begin(); el != elements.end(); el++)
    {

        QJsonObject obj = (*el).toObject();
        if (obj["type"].toString() != "relation")
            continue;

        OsmRelation rel(obj["id"].toDouble());
        rel.tags = getTags(obj["tags"].toObject());

        bool isBuildingRelation = rel.tags.count("type") && (rel.tags["type"] == "building");
        bool isMultipolygonRelation = rel.tags.count("type") && (rel.tags["type"] == "multipolygon");

        QJsonArray jNodes = obj["members"].toArray();
        for (QJsonArray::const_iterator it = jNodes.begin(); it != jNodes.end(); it++)
        {
            QJsonObject member = (*it).toObject();
            QString type = member["type"].toString();
            OsmRelationMember m;
            uint64_t ref = member["ref"].toDouble();
            m.role  = member["role"].toString().toStdString();

            if (isMultipolygonRelation)
            {
                // We don't need individual nodes of relations, so skip them silently.
                /* Cascaded relations could hold relevant information. But their semantics are not standardized,
                 * and their processing would be relatively complex. So we ignore them for now.
                 */
                if ( type != "way")
                {
                    if (type == "relation")
                        cerr << "[INFO] skipping sub-relation " << ref << " of relation " << rel.id << endl;

                    continue;
                }

                if (ways.count(ref) == 0)
                {
                    cerr << "[WARN] member way " << ref << " of relation " << rel.id
                         << " is not part of JSON response, ignoring..." << endl;
                    continue;
                }
                m.way = ways[ref];
                multipolygonWays.insert(m);
                rel.members.push_back(m);
            }

            if (isBuildingRelation && m.role == "outline")
            {
                if (type == "way")
                    wayBlacklist.insert(ref);
                if (type == "relation")
                    relationBlacklist.insert(ref);
            }
        }

        //for (map<string, uint64_t>::const_iterator role =roles.begin(); role != roles.end(); role++)
        //    cout << "\t" << (*role).first << "(" << (*role).second << ")" << endl;

        if (isMultipolygonRelation)
        {
            assert(relations.count(rel.id) == 0);
            relations.insert(make_pair(rel.id, rel) );
        }
    }
    cerr << ", " << relations.size() << " relations" << endl;

    cerr << "[DBG] removing " << multipolygonWays.size() << " ways that are outlines of multipolygons" << endl;
    for (set<OsmRelationMember>::const_iterator it = multipolygonWays.begin(); it != multipolygonWays.end(); it++)
    {
        /** remove all those ways from the list of ways representing buildings that cannot be buildings:
          * - multipolygon outer ways cannot be buildings themselves, because they represent the same
          *   building outline as the multipolygon itself (multipolygon inner ways on the other hand may
          *   be buildings, e.g. the 'inner' hole of one building may be another building
          * - ways that are not tagged as "building" or "building:part" are not buildings, but instead
          *   are part of the server response just because they are multipolygon parts.
          */
        if ((it->role == "outer") || (!it->way.tags.count("building") && !it->way.tags.count("building:part")))
        {
//            cerr << "removing way " << it->way.id << " from list of independent ways" << endl;
            ways.erase(it->way.id);
        }
    }
    return relations;
}

/*
set<uint64_t> getOutlineRelations(map<uint64_t, OsmRelation> &relations)
{
    set<uint64_t> res;

    for (map<uint64_t, OsmRelation>::const_iterator it = relations.begin(); it != relations.end(); it++)
    {
        const OsmRelation &rel = *it;
        if ( (!rel.tags.count("type")) || (rel.tags["type"] != "building" ))
            continue;

        for (list<OsmRelationMember>::const_iterator mbr = rel.members.begin(); mbr != rel.members.end(); mbr ++)
        {
            if (mbr->role != "outline")
                continue;
            mbr->

        }
    }

}*/


void GeometryConverter::onDownloadFinished()
{
    cerr << "... download complete." << endl;


    if (reply->error() > 0) {
        cerr << "Error" << endl;
        cerr << reply->errorString().toStdString();
    }
    else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        assert( ! doc.isNull());
        //cout << ( doc.isNull() ? "invalid":"valid") << endl;
        //QJsonObject obj = doc.object();
        QJsonArray elements = doc.object()["elements"].toArray();
        map<uint64_t, OsmPoint > nodes = getPoints(elements);
        cerr << "[INFO] parsed " << nodes.size() << " nodes";
        cerr.flush();

        //query center point. Needed to convert lat/lng coordinates to local euclidean coordinates
        OsmPoint center(tiley2lat(tileY+0.5, 14), tilex2lng(tileX+0.5, 14));

        map<uint64_t, OsmWay> ways = getWays(elements, nodes);
        set<uint64_t> wayBlacklist, relationBlacklist;
        map<uint64_t, OsmRelation> relations = getRelations(elements, ways,
                                                            wayBlacklist, relationBlacklist);

        //cerr << "Blacklisted " << wayBlacklist.size() << " ways and "
        //     << relationBlacklist.size() << " relations" << endl;

        list<Building> buildings;
        for (map<uint64_t, OsmRelation>::iterator rel = relations.begin(); rel != relations.end(); rel++)
        {
            if ( relationBlacklist.count(rel->first) )
                continue;

            rel->second.promoteTags();
            string name = string("r")+QString::number(rel->second.id).toStdString();
            buildings.push_back( Building(
                PolygonWithHoles::fromOsmRelation(rel->second, center, name.c_str()),
                BuildingAttributes( rel->second.tags ), name));
        }

        for (map<uint64_t, OsmWay>::const_iterator way = ways.begin(); way != ways.end(); way++)
        {
            if ( wayBlacklist.count(way->first) )
                continue;

            //cerr << "[DBG] parsing way " << way->second.id << endl;
            string name = string("w")+QString::number(way->second.id).toStdString();
            buildings.push_back( Building(
                PolygonWithHoles(way->second.points, list<OsmPointList>(), center, name.c_str()),
                BuildingAttributes ( way->second.tags), name));
        }
        cerr << "[DBG] unified geometry to " << buildings.size() << " buildings." << endl;

        cout << "[" << endl;
        bool isFirstBuilding = true;
        for (list<Building>::const_iterator it = buildings.begin(); it != buildings.end(); it++)
        {
//            if (it->getName() != "r2353571")
//                continue;

            if (!it->hasNonZeroHeight())
                continue;

            if (!isFirstBuilding)
                cout << ",";

            isFirstBuilding = false;
            cout << endl << it->toJSON(center);

        }
        cout << "]" << endl;

        cerr << "Emitting 'done' signal" << endl << endl;
        emit done();
    }

}



QString getAABBString(int tileX, int tileY, int zoom)
{
    double latMin = tiley2lat(tileY+1, zoom);
    double latMax = tiley2lat(tileY,   zoom);

    double lngMin = tilex2lng(tileX,   zoom);
    double lngMax = tilex2lng(tileX+1, zoom);

    return QString("(") +
                   QString::number(latMin, 'g', 8) + "," +  QString::number(lngMin, 'g', 8) + "," +
                   QString::number(latMax, 'g', 8) + "," +  QString::number(lngMax, 'g', 8) + ")";
}

QString getEnvironmentString(const char* name)
{
    char* s = secure_getenv(name);
    return QString(s ? s : "");
}


int main(int argc, char *argv[])
{
    isCgiMode =  (getEnvironmentString("REQUEST_METHOD") != "");

    string tileXStr;
    string tileYStr;

    if (isCgiMode)
    {
        cout << "Content-Type: application/json; charset=utf-8\r\n";
        //cout << "Etag: DEADBEEF\r\n";
        cout << "\r\n";
        QString path = getEnvironmentString("PATH_INFO");
        QRegExp reTile("/(\\d+)/(\\d+)/?(.json)?");
        if (!reTile.exactMatch(path))
        {
            cout << "[\"Error: invalid request '" << path.toStdString() << "'\"]" << endl;
            exit(0);
        }
        tileXStr = reTile.cap(1).toStdString();
        tileYStr = reTile.cap(2).toStdString();
    }
    else
    {
        //tileXStr = argc < 2 ? "8722" : argv[1];
        //tileYStr = argc < 3 ? "5401" : argv[2];
        tileXStr = argc < 2 ? "8721" : argv[1];
        tileYStr = argc < 3 ? "5399" : argv[2];
    }

    tileX = atoi(tileXStr.c_str());
    tileY = atoi(tileYStr.c_str());

    //QDir::current().mkpath( QString("cache/") + tileXStr.c_str() );

    QCoreApplication app(argc, argv);   //initialize infrastructure for Qt event loop.
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QNetworkRequest request;

    cerr << "processing tile 14/" << tileX << "/" << tileY;
    cerr.flush();
    QString sAABB =getAABBString(tileX, tileY, 14);
    QString buildingsAtFlatViewDefaultLocation = QString("")+
            "http://overpass-api.de/api/interpreter?data=[out:json][timeout:25];(way[\"building\"]"+
            ///"http://render.rbuch703.de/api/interpreter?data=[out:json][timeout:25];(way[\"building\"]"+
            sAABB+
            ";way[\"building:part\"]"+sAABB+
            ";relation[\"building\"][\"type\"=\"multipolygon\"]"+sAABB+
            ";relation[\"building:part\"][\"type\"=\"multipolygon\"]"+sAABB+
            ";relation[\"type\"=\"building\"]"+sAABB+
            ";);out body;>;out skel qt;";

    //cerr <<"Query: " << buildingsAtFlatViewDefaultLocation.toStdString()  << endl;
    /*
    QString buildingRelationsInMagdeburg = "http://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3Brelation%5B%22building%22%5D%2852%2E059034798886984%2C11%2E523628234863281%2C52%2E19519199255819%2C11%2E765155792236326%29%3Bout%20body%3B%3E%3Bout%20skel%20qt%3B";
    QString someBuildingsInMagdeburg = "http://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3B(way%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Bway%5B%22building%3Apart%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Brelation%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208))%3Bout%20body%3B%3E%3Bout%20skel%20qt%3B";
    QString locationWithManySmallBuildings = "http://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3B(way%5B%22building%22%5D(52.12303781069966%2C11.616555837901302%2C52.14100422065452%2C11.645824523655705)%3Bway%5B%22building%3Apart%22%5D(52.12303781069966%2C11.616555837901302%2C52.14100422065452%2C11.645824523655705)%3Brelation%5B%22building%22%5D(52.12303781069966%2C11.616555837901302%2C52.14100422065452%2C11.645824523655705))%3Bout%20body%3B%3E%3Bout%20skel%20qt%3B";
    */
    //request.setUrl(QUrl());
    request.setUrl(buildingsAtFlatViewDefaultLocation );
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    reply = manager->get(request);
    GeometryConverter * dummy = new GeometryConverter();

    QObject::connect(reply, SIGNAL(finished()), dummy, SLOT(onDownloadFinished()));
    //                           SLOT(slotProgress(qint64,qint64)));

    QObject::connect(dummy, SIGNAL(done()), &app, SLOT(quit()) );
    //                           SLOT(slotProgress(qint64,qint64)));

    return app.exec();

}


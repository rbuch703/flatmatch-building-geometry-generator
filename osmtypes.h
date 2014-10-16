#ifndef OSMTYPES_H
#define OSMTYPES_H

#include <math.h>
#include <stdint.h>

#include <qstring.h>

#include <map>
#include <string>
#include <list>

using namespace std;

//typedef pair<string, string> OsmKeyValuePair;
typedef map<string, string> Tags;

struct OsmPoint {
    OsmPoint(): lat(nan("")), lng(nan("")) {}
    OsmPoint(double lat, double lng): lat(lat), lng(lng) {}
    double lat, lng;

};

typedef list<OsmPoint> OsmPointList;

bool operator==(const OsmPoint &p1, const OsmPoint &p2);
bool operator!=(const OsmPoint &p1, const OsmPoint &p2);

class OsmWay
{
public:
    OsmWay( uint64_t id = 0): id(id) {}
    string getName() const;
    static bool mergeable(const OsmWay &w1, const OsmWay &w2);
    static OsmWay merge(const OsmWay &w1, const OsmWay &w2);
public:
    uint64_t id;
    string name;
    list<OsmPoint> points;
    map<string, string> tags;
};

struct OsmRelationMember {
    OsmRelationMember() {};
    OsmRelationMember(OsmWay way, string roleName): way(way), role(roleName) {};
    OsmWay way;
    string role;
};

struct OsmRelation
{
    OsmRelation( uint64_t id = 0): id(id) {}
    void promoteTags();
    void mergeWays();

    uint64_t id;
    map<string, string> tags;
    list< OsmRelationMember  >members;
};

#endif // OSMTYPES_H

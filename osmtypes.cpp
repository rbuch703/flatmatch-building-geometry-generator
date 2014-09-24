
#include "osmtypes.h"

bool operator==(const OsmPoint &p1, const OsmPoint &p2) { return p1.lat == p2.lat && p1.lng == p2.lng;}
bool operator!=(const OsmPoint &p1, const OsmPoint &p2) { return p1.lat != p2.lat || p1.lng != p2.lng;}


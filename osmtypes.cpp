
#include "osmtypes.h"
#include <boost/foreach.hpp>
#include <assert.h>

bool operator==(const OsmPoint &p1, const OsmPoint &p2) { return p1.lat == p2.lat && p1.lng == p2.lng;}
bool operator!=(const OsmPoint &p1, const OsmPoint &p2) { return p1.lat != p2.lat || p1.lng != p2.lng;}


string OsmWay::getName() const {
    return name.length() ? name : QString::number(id).toStdString();
}

bool OsmWay::mergeable(const OsmWay &w1, const OsmWay &w2)
{
    if ((w1.points.front() == w1.points.back()) || (w2.points.front() == w2.points.back()))
        return false;

    return ((w1.points.front() == w2.points.front()) ||
            (w1.points.front() == w2.points.back() ) ||
            (w1.points.back()  == w2.points.front()) ||
            (w1.points.back()  == w2.points.back() ));
}

list<OsmPoint> merge(list<OsmPoint> l1, list<OsmPoint> l2)
{
    //step 1: bring l1 and l2 onti canonical form: l1.back() = l2.front()
    if ( (l1.front() == l2.front() ))
        l1.reverse();
    else if (l1.front() == l2.back())
        swap(l1, l2);
    else if (l1.back() == l2.front())
        {}  //nothing to do
    else if (l1.back() == l2.back())
        l2.reverse();
    else
        assert(false && "Merge algorithm error");

    assert( (l1.back() == l2.front())&& "Merge algorithm error");

    l1.pop_back(); //since l1.back() == l2.front(), just appending the lists would lead to a duplicate point
    l1.insert( l1.end(), l2.begin(), l2.end());
    return l1;
}

OsmWay OsmWay::merge(const OsmWay &w1, const OsmWay &w2)
{
    /** note: this method does not merge tags. Merging tags is ill-defined (what if both ways have
     *        different values for the same key), and this method is supposed to be used to merge
     *        geometry ways that are part of a relation, where the tags of the ways themselves are
     *        irrelevant.
          */

    assert( mergeable(w1, w2));
    list<OsmPoint> points = ::merge(w1.points, w2.points);
    OsmWay res(-1);
    res.name = string("(")  + w1.getName() + "-" + w2.getName() + ")";
    res.points = points;
    return res;
}

bool operator< (const OsmRelationMember &m1, const OsmRelationMember &m2)
{
    return (m1.way.id  < m2.way.id) ||
           (m1.way.id == m2.way.id && m1.role < m2.role);
}



/* In some (technically invalid) cases, an OSM multipolygon has few tags or even no tags at all,
 * and the tags describing it instead are part of its only 'outer' member. This method copies
 * relevant tags from those 'outer' members to the relation itself. This is usually caused when a
 * user of the OSM web editor promotes a way to a relation, but is not aware that he has to move
 * the tags manually.
 * */
void OsmRelation::promoteTags()
{
    for (list<OsmRelationMember>::const_iterator it = members.begin(); it != members.end(); it++)
    {
        if (it->role != "outer")
            continue;

        const OsmWay &way = it->way;
        for (map<string, string>::const_iterator tag = way.tags.begin(); tag != way.tags.end(); tag++)
        {
            if (tags.count(tag->first) == 0)
            {
                //assert(false && "untested code");
                tags.insert(*tag);
                //cerr << "[DBG] adding " << (tag->first) << "=" << tag->second << " to relation " << relation.id << endl;
            }
        }

    }
}

/** mergeWays() needs this typedef, because the comma in 'pair<string, list<OsmWay> >' prevents the
 *  original type to be used in the BOOST_FOREACH macro*/
typedef pair<string, list<OsmWay> > Role;

void OsmRelation::mergeWays()
{
    list<OsmRelationMember> res;
    map< string, list<OsmWay> > ways;
    BOOST_FOREACH( OsmRelationMember mbr, members)
    {
        if (mbr.way.points.front() == mbr.way.points.back())
        {
            res.push_back(mbr);
            continue;
        }

        ways[mbr.role].push_back( mbr.way);
    }

    BOOST_FOREACH(Role role, ways)
    {
        string roleName = role.first;
        list<OsmWay> ways = role.second;
        for ( list<OsmWay>::iterator it1 = ways.begin(); it1 != ways.end(); it1++)
        {
            list<OsmWay>::iterator it2 = it1;
            it2++;
            while (it2 != ways.end())
            {
                if (OsmWay::mergeable(*it1, *it2))
                {
                    *it1 = OsmWay::merge(*it1, *it2);
                    it2 = ways.erase(it2);
                } else
                    it2++;
            }
        }
        BOOST_FOREACH( OsmWay way, ways)
            res.push_back(OsmRelationMember(way, roleName));
    }

    members = res;
}

#include "polygonwithholes.h"

#include <assert.h>

#include <boost/foreach.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Polygon_2.h>
#include<CGAL/Polygon_with_holes_2.h>
#include<CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>

#include <iostream>

struct FaceInfo2
{
    FaceInfo2(){}
    int nesting_level;
    bool in_domain(){
        return nesting_level%2 == 1;
    }
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point;
typedef CGAL::Polygon_2<K> Polygon_2;
typedef CGAL::Polygon_with_holes_2<K> Polygon_with_holes ;
//typedef CGAL::Vector_2<K> Vector2;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2,K> Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<K,Fbb> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb,Fb> TDS;
typedef CGAL::Exact_predicates_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag> CDT;
//typedef CDT::Point Point;
typedef CGAL::Polygon_2<K> Polygon_2;

typedef CGAL::Straight_skeleton_2<K> Ss ;
typedef boost::shared_ptr<Ss> SsPtr ;

using namespace std;


PolygonWithHoles::PolygonWithHoles( ) {}

bool isClockwise(const PointList &poly)
{
    list<Point> points;
    BOOST_FOREACH (const Vector2 p, poly)
    {
        points.push_back(Point(p.x, p.y));
        //cout << p.lat << "," << p.lng << endl;
    }
    //cout << endl;

    assert (points.front() == points.back());
    assert(poly.size() > 0);
    points.pop_back(); //remove duplicate closing point

    Polygon_2 pgn(points.begin(), points.end());
    assert( pgn.is_simple() );

    //cout << "Orientation: " << pgn.orientation() << endl;
    return pgn.orientation() == CGAL::CLOCKWISE;
}

void simplifyPolygon( PointList &poly)
{
    PointList::iterator pNext = poly.begin();
    PointList::iterator pPrev = pNext++;
    PointList::iterator pHere = pNext++;

    static const double MAX_DIST = 0.3;//[m] FIXME: guessed value
    while (pNext != poly.end())
    {
        double dist = getDistance(*pPrev, *pNext, *pHere);
        if (dist < MAX_DIST)
        {
            pHere = poly.erase(pHere);
            pNext++;
        } else
        {
            pNext++;
            pHere++;
            pPrev++;
        }
    }

    //edge case: test for removal of first/last vertex
    pHere = poly.begin();
    pPrev = poly.end();
    --(--pPrev);    //end-1 is last (=first) vertex, end-2 is its predecessor
    pNext = poly.begin();
    pNext++;
    if ( getDistance(*pPrev, *pNext, *pHere) < MAX_DIST)
    {
        /*cout << "Prev:" << pPrev->lat << ", " << pPrev->lng << endl;
        cout << "Here:" << pHere->lat << ", " << pHere->lng << endl;
        cout << "Next:" << pNext->lat << ", " << pNext->lng << endl;*/
        // remove first=last vertex, duplicate new first one to close polygon
        poly.pop_front();
        poly.pop_back();
        poly.push_back(poly.front());
    }

}

PolygonWithHoles::PolygonWithHoles(const OsmPointList &outer, const list<OsmPointList> &holes, const OsmPoint center)
{

    static const double EARTH_CIRCUMFERENCE = 2 * 3.141592 * (6378.1 * 1000); // [m]
    double latToYFactor = 1/360.0 * EARTH_CIRCUMFERENCE;
    double lngToXFactor = 1/360.0 * EARTH_CIRCUMFERENCE * cos( center.lat / 180 * 3.141592);

    BOOST_FOREACH (const OsmPoint pt, outer)
    {
        double dLat = pt.lat - center.lat;
        double dLng = pt.lng - center.lng;
        this->outer.push_back( Vector2( dLng * lngToXFactor, dLat * latToYFactor ));
    }

    BOOST_FOREACH (const OsmPointList & hole, holes)
    {
        PointList convertedHole;
        BOOST_FOREACH (const OsmPoint pt, hole)
        {
            double dLat = pt.lat - center.lat;
            double dLng = pt.lng - center.lng;
            convertedHole.push_back( Vector2( dLng * lngToXFactor, dLat * latToYFactor ));
        }
        this->holes.push_back(convertedHole);
    }
    //FIXME: should not be done here, but rather in 'Building' before constructing the PolygonWIthHoles


    simplifyPolygon(this->outer);

    /** CGALs definition for Polygons with holes includes that the outer polygon
     *  has to be oriented CCW, and all holes CW. Since we will use CGAL quite a lot,
     *  make sure the polygon conforms to these requirements on creation.
     */

    if (isClockwise(this->outer))
        this->outer.reverse();

    BOOST_FOREACH (PointList &poly, this->holes)
    {
        if (!isClockwise(poly))
            poly.reverse();
    }

    /*FIXME: add sanity checks:
     * - (done) every PointList has to form a CLOSED and SIMPLE polygon
     * - no two holes overlap each other (otherwise form their union)
     * - every hole is completely inside the outer polygon (= hole minus polygon does not intersect with polygon)
     * - outer polygon is oriented CCW, holes are oriented CW
     */
}


PolygonWithHoles PolygonWithHoles::fromOsmRelation(const OsmRelation &rel, OsmPoint center)
{
    const OsmPointList *outer = NULL;
    for (list<OsmRelationMember>::const_iterator member = rel.members.begin(); member != rel.members.end(); member++)
    {
        if (member->role == "outer")
        {
            outer = &(member->way.points);
            break;
        }
    }
    assert(outer);

    list<OsmPointList> holes;
    for (list<OsmRelationMember>::const_iterator member = rel.members.begin(); member != rel.members.end(); member++)
    {
        if (&member->way.points == outer)
            continue;

        assert((member->role == "inner" && member->role == "") || "unknown member role");
        holes.push_back( member->way.points);

    }

    return PolygonWithHoles(*outer, holes, center);
}

void mark_domains(CDT& ct, CDT::Face_handle start, int index,
             std::list<CDT::Edge>& border )
{
    if(start->info().nesting_level != -1){
        return;
    }
    std::list<CDT::Face_handle> queue;
    queue.push_back(start);
    while(! queue.empty()){
        CDT::Face_handle fh = queue.front();
        queue.pop_front();
        if(fh->info().nesting_level == -1){
            fh->info().nesting_level = index;
            for(int i = 0; i < 3; i++){
                CDT::Edge e(fh,i);
                CDT::Face_handle n = fh->neighbor(i);
                if(n->info().nesting_level == -1){
                    if(ct.is_constrained(e)) border.push_back(e);
                    else queue.push_back(n);
                }
            }
        }
    }
}
//explore set of facets connected with non constrained edges,
//and attribute to each such set a nesting level.
//We start from facets incident to the infinite vertex, with a nesting
//level of 0. Then we recursively consider the non-explored facets incident
//to constrained edges bounding the former set and increase the nesting level by 1.
//Facets in the domain are those with an odd nesting level.
void
mark_domains(CDT& cdt)
{
    for(CDT::All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it){
        it->info().nesting_level = -1;
    }
    std::list<CDT::Edge> border;
    mark_domains(cdt, cdt.infinite_face(), 0, border);
    while(! border.empty()){
        CDT::Edge e = border.front();
        border.pop_front();
        CDT::Face_handle n = e.first->neighbor(e.second);
        if(n->info().nesting_level == -1){
            mark_domains(cdt, n, e.first->info().nesting_level+1, border);
        }
    }
}
void insert_polygon(CDT& cdt,const Polygon_2& polygon){
    if ( polygon.is_empty() ) return;
    CDT::Vertex_handle v_prev=cdt.insert(*CGAL::cpp11::prev(polygon.vertices_end()));
    for (Polygon_2::Vertex_iterator vit=polygon.vertices_begin();
         vit!=polygon.vertices_end();++vit)
    {
        CDT::Vertex_handle vh=cdt.insert(*vit);
        cdt.insert_constraint(vh,v_prev);
        v_prev=vh;
    }
}


Polygon_2 asCgalPolygon(const PointList &poly)
{
    Polygon_2 res;
    PointList tmp = poly;
    tmp.pop_back(); //remove duplicate start/end vertex

    BOOST_FOREACH(const Vector2 &p, tmp)
        res.push_back( Point(p.x, p.y));

    return res;
}

list<Triangle2> PolygonWithHoles::triangulate() const
{
    list<Triangle2> res;
    CDT cdt;

    insert_polygon(cdt, asCgalPolygon(this->outer));

    BOOST_FOREACH(const PointList &hole, this->getHoles())
        insert_polygon(cdt, asCgalPolygon(hole));

    //Mark facets that are inside the domain bounded by the polygon
    mark_domains(cdt);
    int count=0;
    for (CDT::Finite_faces_iterator fit=cdt.finite_faces_begin();
         fit!=cdt.finite_faces_end();++fit)
    {
        if (! fit->info().in_domain() )
            continue;

        ++count;
        CDT::Triangle t = cdt.triangle(fit);

        res.push_back( Triangle2 ( Vector2( t[0][0], t[0][1]),
                                   Vector2( t[1][0], t[1][1]),
                                   Vector2( t[2][0], t[2][1])));

    }
    //std::cout << "There are " << count << " facets in the domain." << std::endl;
    //return 0;
    return res;
}

list<PointList> PolygonWithHoles::getSkeletonFaces() const
{
    list<PointList> res;

    Polygon_with_holes poly( asCgalPolygon(this->outer) );

    BOOST_FOREACH(const PointList &hole, this->getHoles())
        poly.add_hole( asCgalPolygon(hole) );

    SsPtr iss = CGAL::create_interior_straight_skeleton_2(poly);

    for ( Ss::Halfedge_const_iterator i = iss->halfedges_begin(); i != iss->halfedges_end(); ++i )
    {
        if (i->is_bisector())
            continue;

        if (i->is_border())
            continue;

        PointList face;
        face.push_back( Vector2(i->vertex()->point()[0], i->vertex()->point()[1]));
        //std::cout << "\t" << i->vertex()->point() << std::endl;


        for ( Ss::Halfedge_const_handle ii = i->next(); ii != i; ii = ii->next())
        {
            //std::cout << "\t" << ii->vertex()->point() << std::endl;
            face.push_back( Vector2(ii->vertex()->point()[0], ii->vertex()->point()[1]));
        }

        face.push_back(face.front()); //duplicate first vertex to close polygon

        res.push_back(face);
        //print_point(i->opposite()->vertex()->point()) ;
        //std::cout << "->" ;
        //print_point(i->vertex()->point());
        //std::cout << " " << ( i->is_bisector() ? "bisector" : "contour" );
        //std::cout << " " << ( i->is_border() ? "border": "") << std::endl;
    }

    return res;
}

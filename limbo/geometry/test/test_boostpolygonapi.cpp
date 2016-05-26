/*************************************************************************
    > File Name: test_boostpolygonapi.cpp
    > Author: Yibo Lin
    > Mail: yibolin@utexas.edu
    > Created Time: Fri 31 Oct 2014 02:53:46 PM CDT
 ************************************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <boost/polygon/polygon.hpp>
#include <limbo/geometry/Polygon2Rectangle.h>
#include <limbo/geometry/api/BoostPolygonApi.h>
#include <limbo/geometry/test/GeoBoostPolygonApi.h>
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::list;
using std::set;

namespace gtl = boost::polygon;
namespace lg = limbo::geometry; 

void test1(string const& filename)
{
    std::vector<gtl::rectangle_data<int> > vRect; 
	lg::Polygon2Rectangle<vector<gtl::point_data<int> >, vector<gtl::rectangle_data<int> > > p2r (vRect, lg::HOR_VER_SLICING);
	assert(p2r.read(filename));
	assert(p2r());
	p2r.print("p2r1.gp");
}

void test2(string const& filename)
{
    std::vector<gtl::rectangle_data<int> > vRect; 
	lg::Polygon2Rectangle<list<gtl::point_data<int> >, vector<gtl::rectangle_data<int> > > p2r (vRect, lg::HOR_VER_SLICING);
	assert(p2r.read(filename));
	assert(p2r());
	p2r.print("p2r2.gp");
}

void test3(string const& filename)
{
    std::vector<gtl::rectangle_data<int> > vRect; 
	lg::Polygon2Rectangle<set<gtl::point_data<int> , lg::point_compare_type>, vector<gtl::rectangle_data<int> > > p2r (vRect, lg::HOR_VER_SLICING);
	assert(p2r.read(filename));
	assert(p2r());
	p2r.print("p2r3.gp");
}

void test4()
{
    vector<gtl::point_data<int> > vPoint (8);
    vPoint[0] = gtl::construct<gtl::point_data<int> >(0, 0);
    vPoint[1] = gtl::construct<gtl::point_data<int> >(0, 10);
    vPoint[2] = gtl::construct<gtl::point_data<int> >(10, 10);
    vPoint[3] = gtl::construct<gtl::point_data<int> >(10, 20);
    vPoint[4] = gtl::construct<gtl::point_data<int> >(20, 20);
    vPoint[5] = gtl::construct<gtl::point_data<int> >(20, 10);
    vPoint[6] = gtl::construct<gtl::point_data<int> >(30, 10);
    vPoint[7] = gtl::construct<gtl::point_data<int> >(30, 0);
    //vPoint.push_back(gtl::construct<gtl::point_data<int> >(0, 0));
    vector<gtl::rectangle_data<int> > vRectangle;
    assert(lg::polygon2RectangleBoost(vPoint, vRectangle));
    for (std::size_t i = 0; i != vRectangle.size(); ++i)
    {
        gtl::rectangle_data<int> const& rect = vRectangle[i];
        cout << "(" << gtl::xl(rect) << ", " << gtl::yl(rect) << ", " << gtl::xh(rect) << ", " << gtl::yh(rect) << ")\n";
    }
    cout << "test 4 passed\n";
}

int main(int argc, char** argv)
{
    test4();
	if (argc > 1)
	{
		test1(argv[1]);
		test2(argv[1]);
		test3(argv[1]);
	}
	else cout << "at least 1 argument is required" << endl;

	return 0;
}

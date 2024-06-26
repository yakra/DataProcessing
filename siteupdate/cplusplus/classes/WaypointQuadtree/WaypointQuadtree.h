class ErrorList;
class Waypoint;
#include <forward_list>
#include <list>
#include <iostream>
#include <mutex>
#include <vector>

using VInfoVec = std::vector<std::pair<Waypoint*,size_t>>;
class WaypointQuadtree
{	// This class defines a recursive quadtree structure to store
	// Waypoint objects for efficient geometric searching.

	public:
	double min_lat, min_lng, max_lat, max_lng, mid_lat, mid_lng;
	WaypointQuadtree *nw_child, *ne_child, *sw_child, *se_child;
	std::list<Waypoint*> points;
	unsigned int unique_locations;
	std::recursive_mutex mtx;

	bool refined();
	WaypointQuadtree(double, double, double, double);
	void refine();
	void insert(Waypoint*, bool);
	void near_miss_waypoints(Waypoint*, double);
	void nmplogs();
	std::string str();
	unsigned int size();
	std::list<Waypoint*> point_list();
	void graph_points(VInfoVec&, VInfoVec&, size_t&);
	bool is_valid(ErrorList &);
	unsigned int total_nodes();
	void get_tmg_lines(std::list<std::string> &, std::list<std::string> &, std::string);
	void write_qt_tmg(std::string);
	void final_report(std::vector<unsigned int>&);
	void sort();
      #ifdef threading_enabled
	void terminal_nodes(std::forward_list<WaypointQuadtree*>*, size_t&);
	static void sortnodes(std::forward_list<WaypointQuadtree*>*);
      #endif
};

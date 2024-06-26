class HighwaySystem;
class Route;
class TravelerList;
class Waypoint;
#include "../../templates/TMBitset.cpp"
#include <list>
#include <mutex>
#include <vector>

class HighwaySegment
{   /* This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes */

	public:
	Waypoint *waypoint1;
	Waypoint *waypoint2;
	Route *route;
	double length;
	std::list<HighwaySegment*> *concurrent;
	TMBitset<TravelerList*, uint32_t> clinched_by;

	HighwaySegment(Waypoint*, Route*);
	~HighwaySegment();

	std::string str();
	void add_concurrency(std::ofstream&, Waypoint*);
	//std::string concurrent_travelers_sanity_check();

	// graph generation functions
	std::string segment_name();
	const char* clinchedby_code(char*, unsigned int);
	void write_label(std::ofstream&, std::vector<HighwaySystem*> *);
	HighwaySegment* canonical_edge_segment();
};

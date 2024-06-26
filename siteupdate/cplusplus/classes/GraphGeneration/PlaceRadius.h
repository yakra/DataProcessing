class GraphListEntry;
class HGEdge;
class HGVertex;
class HighwayGraph;
class WaypointQuadtree;
#include "../../templates/TMBitset.cpp"
#include <iostream>
#include <vector>

class PlaceRadius
{	/* This class encapsulates a place name, file base name, latitude,
	longitude, and radius (in miles) to define the area to which our
	place-based graphs are restricted.
	*/

	public:
	std::string descr;	// long description of area, E.G. "New York City"
	std::string title;	// filename title, short name for area, E.G. "nyc"
	double lat, lng;	// center latitude, longitude
	double r;		// radius in miles

	PlaceRadius(const char *, const char *, double &, double &, double &);

	bool contains_vertex(double, double);
	void matching_ve(TMBitset<HGVertex*,uint64_t>&, TMBitset<HGEdge*,uint64_t>&, WaypointQuadtree*);
	void   ve_search(TMBitset<HGVertex*,uint64_t>&, TMBitset<HGEdge*,uint64_t>&, WaypointQuadtree*, double, double);
};

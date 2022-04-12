class HighwaySystem;
class PlaceRadius;
class Region;
#include <list>
#include <string>
#include <vector>

class GraphListEntry
{	/* This class encapsulates information about generated graphs for
	inclusion in the DB table.  Field names here
	(or function names if both are on the same line)
	match column names in the "graphs" DB table. */
	public:
	// Info for use in threaded subgraph generation. Not used in DB.
	std::list<Region*> *regions;
	std::list<HighwaySystem*> *systems;
	PlaceRadius *placeradius;

	// Info for the "graphs" DB table
	std::string root;	std::string filename();
	std::string descr;
	unsigned int vertices;
	unsigned int edges;
	unsigned int travelers;
	char form;		std::string format();
	char cat;		std::string category();

	static std::vector<GraphListEntry> entries;
	static size_t num; // iterator for entries
	std::string tag();

	GraphListEntry(char, unsigned int, unsigned int, unsigned int); // < master graph ctor | v----- subgraph ctor -----v 
	GraphListEntry(std::string, std::string, char, char, std::list<Region*>*, std::list<HighwaySystem*>*, PlaceRadius*);
	static void add_group(std::string&&,  std::string&&,  char, std::list<Region*>*, std::list<HighwaySystem*>*, PlaceRadius*);
};

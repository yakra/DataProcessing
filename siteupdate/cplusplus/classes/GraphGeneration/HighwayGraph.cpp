class HighwayGraph
{   /* This class implements the capability to create graph
    data structures representing the highway data.

    On construction, build a set of unique vertex names
    and determine edges, at most one per concurrent segment.
    Create two sets of edges - one for the full graph
    and one for the graph with hidden waypoints compressed into
    multi-point edges.
    */

	public:
	// first, find unique waypoints and create vertex labels
	std::unordered_set<std::string> vertex_names;
	// to track the waypoint name compressions, add log entries
	// to this list
	std::list<std::string> waypoint_naming_log;
	std::unordered_map<Waypoint*, HighwayGraphVertexInfo*> vertices;

	HighwayGraph
	(	WaypointQuadtree &all_waypoints,
		std::list<HighwaySystem*> &highway_systems,
		DatacheckEntryList *datacheckerrors,
		unsigned int numthreads,
		ElapsedTime &et
	)
	{	// loop for each Waypoint, create a unique name and vertex,
		// unless it's a point not in or colocated with any active
		// or preview system, or is colocated and not at the front
		// of its colocation list
		unsigned int counter = 0;
		std::cout << et.et() << "Creating unique names and vertices" << std::flush;
		for (Waypoint *w : all_waypoints.point_list())
		{	if (counter % 10000 == 0) std::cout << '.' << std::flush;
			counter++;
			// skip if this point is occupied by only waypoints in devel systems
			if (!w->is_or_colocated_with_active_or_preview()) continue;
			// skip if colocated and not at front of list
			if (w->colocated && w != w->colocated->front()) continue;

			// come up with a unique name that brings in its meaning

			// start with the canonical name
			std::string point_name = w->canonical_waypoint_name(waypoint_naming_log);
			bool good_to_go = 1;

			// if that's taken, append the region code
			if (vertex_names.find(point_name) != vertex_names.end())
			{	point_name += "|" + w->route->region->code;
				waypoint_naming_log.push_back("Appended region: " + point_name);
				good_to_go = 0;
			}

			// if that's taken, see if the simple name is available
			if (!good_to_go && vertex_names.find(point_name) != vertex_names.end())
			{	std::string simple_name = w->simple_waypoint_name();
				if (vertex_names.find(simple_name) == vertex_names.end())
				{	waypoint_naming_log.push_back("Revert to simple: " + simple_name + " from (taken) " + point_name);
					point_name = simple_name;
					good_to_go = 1;
				}
				else	good_to_go = 0;
			}

			// if we have not yet succeeded, add !'s until we do
			if (!good_to_go) while (vertex_names.find(point_name) != vertex_names.end())
			{	point_name += "!";
				waypoint_naming_log.push_back("Appended !: " + point_name);
			}

			// we're good; now construct a vertex
			if (!w->colocated)
				vertices[w] = new HighwayGraphVertexInfo(w, &*(vertex_names.insert(point_name).first), datacheckerrors, numthreads);
			else	vertices[w] = new HighwayGraphVertexInfo(w->colocated->front(), &*(vertex_names.insert(point_name).first), datacheckerrors, numthreads);
			// vertices are deleted by HighwayGraph::clear
		}
		std::cout << '!' << std::endl;
		//#include "../../debug/unique_names.cpp"

		// add edges, which end up in two separate vertex adjacency lists,
		counter = 0;
		std::cout << et.et() << "Creating edges" << std::flush;
		for (HighwaySystem *h : highway_systems)
		{	if (h->devel()) continue;
			if (counter % 6 == 0) std::cout << '.' << std::flush;
			counter++;
			for (Route &r : h->route_list)
			  for (HighwaySegment *s : r.segment_list)
			    if (!s->concurrent || s == s->concurrent->front())
			    {	// first one copy for the full simple graph
				bool duplicate = 0;
				HighwayGraphEdgeInfo *e = new HighwayGraphEdgeInfo(s, this, duplicate);
				// and again for a graph where hidden waypoints
				// are merged into the edge structures
				if (!duplicate) new HighwayGraphCollapsedEdgeInfo(e);
				// edges & collapsed edges are deleted by ~HighwayGraphVertexInfo, called by HighwayGraph::clear
			    }
		}
		std::cout << "!\n" << et.et() << "Full graph has " << vertices.size() << " vertices, " << edge_count() << " edges." << std::endl;

		// compress edges adjacent to hidden vertices
		counter = 0;
		std::cout << et.et() << "Compressing collapsed edges" << std::flush;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		{	if (counter % 10000 == 0) std::cout << '.' << std::flush;
			counter++;
			if (wv.second->is_hidden)
			{	if (wv.second->incident_collapsed_edges.size() < 2)
				{	// these cases are flagged as HIDDEN_TERMINUS
					wv.second->is_hidden = 0;
					continue;
				}
				if (wv.second->incident_collapsed_edges.size() > 2)
				{	std::list<Waypoint*>::iterator it = wv.second->first_waypoint->colocated->begin();
					Waypoint *dcw = *it; // "datacheck waypoint"
					for (it++; it != wv.second->first_waypoint->colocated->end(); it++)
					    if (dcw->root_at_label() > (*it)->root_at_label())
						dcw->root_at_label() = (*it)->root_at_label();
					datacheckerrors->add(dcw->route, dcw->label, "", "", "HIDDEN_JUNCTION", std::to_string(wv.second->incident_collapsed_edges.size()));
					wv.second->is_hidden = 0;
					continue;
				}
				// construct from vertex_info this time
				new HighwayGraphCollapsedEdgeInfo(wv.second);
				// collapsed edges are deleted by ~HighwayGraphVertexInfo, called by HighwayGraph::clear
			}
		}

		// print summary info
		std::cout << "!\n" << et.et() << "Edge compressed graph has " << num_visible_vertices()
			  << " vertices, " << collapsed_edge_count() << " edges." << std::endl;
	} // end ctor

	unsigned int num_visible_vertices()
	{	unsigned int count = 0;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		  if (!wv.second->is_hidden) count ++;
		return count;
	}

	unsigned int edge_count()
	{	unsigned int edges = 0;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		  edges += wv.second->incident_edges.size();
		return edges/2;
	}

	unsigned int collapsed_edge_count()
	{	unsigned int edges = 0;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		  if (!wv.second->is_hidden)
		    edges += wv.second->incident_collapsed_edges.size();
		return edges/2;
	}

	void clear()
	{	for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices) delete wv.second;
		vertex_names.clear();
		waypoint_naming_log.clear();
		vertices.clear();
	}

	std::unordered_set<HighwayGraphVertexInfo*> matching_vertices(GraphListEntry &g, unsigned int &vis)
	{	// return a set of vertices from the graph, optionally
		// restricted by region or system or placeradius area
		vis = 0;
		std::unordered_set<HighwayGraphVertexInfo*> vertex_set;
		std::unordered_set<HighwayGraphVertexInfo*> rg_vertex_set;
		std::unordered_set<HighwayGraphVertexInfo*> sys_vertex_set;
		// rg_vertex_set is the union of all sets in regions
		if (g.regions) for (Region *r : *g.regions)
			rg_vertex_set.insert(r->vertices.begin(), r->vertices.end());
		// sys_vertex_set is the union of all sets in systems
		if (g.systems) for (HighwaySystem *h : *g.systems)
			sys_vertex_set.insert(h->vertices.begin(), h->vertices.end());

		// determine which vertices are within our region(s) and/or system(s)
		if (g.regions)
		     {	vertex_set = rg_vertex_set;
			if (g.systems)
				// both regions & systems populated: vertex_set is
				// intersection of rg_vertex_set & sys_vertex_set
				for (HighwayGraphVertexInfo *v : sys_vertex_set)
				  vertex_set.erase(v);
		     }
		else	if (g.systems)
				// only systems populated
				vertex_set = sys_vertex_set;
			else	// neither are populated; include all vertices...
			  for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
				// ...unless a PlaceRadius is specified
				if (!g.placeradius || g.placeradius->contains_vertex_info(wv.second))
				  vertex_set.insert(wv.second);

		// if placeradius is provided along with region or
		// system parameters, erase vertices outside placeradius
		if (g.placeradius && (g.systems || g.regions))
		{	std::unordered_set<HighwayGraphVertexInfo*>::iterator v = vertex_set.begin();
			while(v != vertex_set.end())
			  if (!g.placeradius->contains_vertex_info(*v))
				v = vertex_set.erase(v);
			  else	v++;
		}
		// find number of visible vertices
		for (HighwayGraphVertexInfo *v : vertex_set)
		  if (!v->is_hidden) vis++;
		return vertex_set;
	}//*/

	std::unordered_set<HighwayGraphEdgeInfo*> matching_edges(std::unordered_set<HighwayGraphVertexInfo*> &mv, GraphListEntry &g)
	{	// return a set of edges from the graph, optionally
		// restricted by region or system or placeradius area
		std::unordered_set<HighwayGraphEdgeInfo*> edge_set;
		std::unordered_set<HighwayGraphEdgeInfo*> rg_edge_set;
		std::unordered_set<HighwayGraphEdgeInfo*> sys_edge_set;
		// rg_edge_set is the union of all sets in regions
		if (g.regions) for (Region *r : *g.regions)
			rg_edge_set.insert(r->edges.begin(), r->edges.end());
		// sys_edge_set is the union of all sets in systems
		if (g.systems) for (HighwaySystem *h : *g.systems)
			sys_edge_set.insert(h->edges.begin(), h->edges.end());

		// determine which edges are within our region(s) and/or system(s)
		if (g.regions)
		     {	edge_set = rg_edge_set;
			if (g.systems)
				// both regions & systems populated: edge_set is
				// intersection of rg_edge_set & sys_edge_set
				for (HighwayGraphEdgeInfo *v : sys_edge_set)
				  edge_set.erase(v);
		     }
		else	if (g.systems)
				// only systems populated
				edge_set = sys_edge_set;
			else	// neither are populated; include all edges///
			  for (HighwayGraphVertexInfo *v : mv)
			    for (HighwayGraphEdgeInfo *e : v->incident_edges)
				// ...unless a PlaceRadius is specified
				if (!g.placeradius || g.placeradius->contains_edge(e))
				  edge_set.insert(e);

		// if placeradius is provided along with non-empty region
		// or system parameters, erase edges outside placeradius
		if (g.placeradius && (g.systems || g.regions))
		{	std::unordered_set<HighwayGraphEdgeInfo*>::iterator e = edge_set.begin();
			while(e != edge_set.end())
			  if (!g.placeradius->contains_edge(*e))
				e = edge_set.erase(e);
			  else	e++;
		}
		return edge_set;
	}

	std::unordered_set<HighwayGraphCollapsedEdgeInfo*> matching_collapsed_edges(std::unordered_set<HighwayGraphVertexInfo*> &mv, GraphListEntry &g)
	{	// return a set of edges from the graph edges for the collapsed
		// edge format, optionally restricted by region or system or
		// placeradius area
		std::unordered_set<HighwayGraphCollapsedEdgeInfo*> edge_set;
		for (HighwayGraphVertexInfo *v : mv)
		{	if (v->is_hidden) continue;
			for (HighwayGraphCollapsedEdgeInfo *e : v->incident_collapsed_edges)
			  if (!g.placeradius || g.placeradius->contains_edge(e))
			  {	bool rg_in_rg = 0;
				if (g.regions) for (Region *r : *g.regions)
				  if (r == e->region)
				  {	rg_in_rg = 1;
					break;
				  }
				if (!g.regions || rg_in_rg)
				{	bool system_match = !g.systems;
					if (!system_match)
					  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
					  {	bool sys_in_sys = 0;
						if (g.systems) for (HighwaySystem *s : *g.systems)
						  if (s == rs.second)
						  {	sys_in_sys = 1;
							break;
						  }
						if (sys_in_sys) system_match = 1;
					  }
					if (system_match) edge_set.insert(e);
				}
			  }
		}
		return edge_set;
	}

	// write the entire set of highway data in .tmg format.
	// The first line is a header specifying
	// the format and version number, the second line specifying the
	// number of waypoints, w, and the number of connections, c, then w
	// lines describing waypoints (label, latitude, longitude), then c
	// lines describing connections (endpoint 1 number, endpoint 2
	// number, route label)
	//
	// passes number of vertices and number of edges written by reference
	//
	void write_master_tmg_simple(GraphListEntry *msptr, std::string filename)
	{	std::ofstream tmgfile(filename.data());
		tmgfile << "TMG 1.0 simple\n";
		tmgfile << vertices.size() << ' ' << edge_count() << '\n';
		// number waypoint entries as we go to support original .gra
		// format output
		int vertex_num = 0;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		{	char fstr[42];
			sprintf(fstr, "%.15g %.15g", wv.second->lat, wv.second->lng);
			tmgfile << *(wv.second->unique_name) << ' ' << fstr << '\n';
			wv.second->vertex_num[0] = vertex_num;
			vertex_num++;
		}
		// sanity check
		if (vertices.size() != vertex_num)
			std::cout << "ERROR: computed " << vertices.size() << " waypoints but wrote " << vertex_num << std::endl;

		// now edges, only print if not already printed
		int edge = 0;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		  for (HighwayGraphEdgeInfo *e : wv.second->incident_edges)
		    if (!e->written)
		    {	e->written = 1;
			tmgfile << e->vertex1->vertex_num[0] << ' ' << e->vertex2->vertex_num[0] << ' ' << e->label(0) << '\n';
			edge++;
		    }
		// sanity checks
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		  for (HighwayGraphEdgeInfo *e : wv.second->incident_edges)
		    if (!e->written)
		      std::cout << "ERROR: never wrote edge " << e->vertex1->vertex_num << ' ' << e->vertex2->vertex_num[0] << ' ' << e->label(0) << std::endl;
		if (edge_count() != edge)
		  std::cout << "ERROR: computed " << edge_count() << " edges but wrote " << edge << std::endl;

		tmgfile.close();
		msptr->vertices = vertices.size();
		msptr->edges = edge_count();
	}

	// write the entire set of data in the tmg collapsed edge format
	void write_master_tmg_collapsed(GraphListEntry *mcptr, std::string filename, unsigned int threadnum)
	{	std::ofstream tmgfile(filename.data());
		unsigned int num_collapsed_edges = collapsed_edge_count();
		tmgfile << "TMG 1.0 collapsed\n";
		tmgfile << num_visible_vertices() << " " << num_collapsed_edges << '\n';

		// write visible vertices
		int vis_vertex_num = 0;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		  if (!wv.second->is_hidden)
		  {	char fstr[42];
			sprintf(fstr, "%.15g %.15g", wv.second->lat, wv.second->lng);
			tmgfile << *(wv.second->unique_name) << ' ' << fstr << '\n';
			wv.second->vis_vertex_num[threadnum] = vis_vertex_num;
			vis_vertex_num++;
		  }
		// write collapsed edges
		int edge = 0;
		for (std::pair<const Waypoint*, HighwayGraphVertexInfo*> wv : vertices)
		  if (!wv.second->is_hidden)
		    for (HighwayGraphCollapsedEdgeInfo *e : wv.second->incident_collapsed_edges)
		      if (!e->written)
		      {	e->written = 1;
			tmgfile << e->collapsed_tmg_line(0, threadnum) << '\n';
			edge++;
		      }
		// sanity check on edges written
		if (num_collapsed_edges != edge)
			std::cout << "ERROR: computed " << num_collapsed_edges << " collapsed edges, but wrote " << edge << '\n';

		tmgfile.close();
		mcptr->vertices = num_visible_vertices();
		mcptr->edges = num_collapsed_edges;
	}

	// write a subset of the data,
	// in both simple and collapsed formats,
	// restricted by regions in the list if given,
	// by system in the list if given,
	// or to within a given area if placeradius is given
	void write_subgraphs_tmg(std::vector<GraphListEntry> &graph_vector, std::string path, size_t graphnum, unsigned int threadnum)
	{	unsigned int visible_v;
		std::string simplefilename = path+graph_vector[graphnum].filename();
		std::string collapfilename = path+graph_vector[graphnum+1].filename();
		std::ofstream simplefile(simplefilename.data());
		std::ofstream collapfile(collapfilename.data());
		std::unordered_set<HighwayGraphVertexInfo*> mv = matching_vertices(graph_vector[graphnum], visible_v);
		std::unordered_set<HighwayGraphEdgeInfo*> mse = matching_edges(mv, graph_vector[graphnum]);
		std::unordered_set<HighwayGraphCollapsedEdgeInfo*> mce = matching_collapsed_edges(mv, graph_vector[graphnum]);
		std::cout << graph_vector[graphnum].tag()
			  << '(' << mv.size() << ',' << mse.size() << ") "
			  << '(' << visible_v << ',' << mce.size() << ") " << std::flush;
		simplefile << "TMG 1.0 simple\n";
		collapfile << "TMG 1.0 collapsed\n";
		simplefile << mv.size() << ' ' << mse.size() << '\n';
		collapfile << visible_v << ' ' << mce.size() << '\n';

		// write vertices
		unsigned int sv = 0;
		unsigned int cv = 0;
		for (HighwayGraphVertexInfo *v : mv)
		{	char fstr[43];
			sprintf(fstr, " %.15g %.15g", v->lat, v->lng);
			// all vertices, for simple graph
			simplefile << *(v->unique_name) << fstr << '\n';
			v->vertex_num[threadnum] = sv;
			sv++;
			// visible vertices, for collapsed graph
			if (!v->is_hidden)
			{	collapfile << *(v->unique_name) << fstr << '\n';
				v->vis_vertex_num[threadnum] = cv;
				cv++;
			}
		}
		// write edges
		for (HighwayGraphEdgeInfo *e : mse)
			simplefile << e->vertex1->vertex_num[threadnum] << ' '
				   << e->vertex2->vertex_num[threadnum] << ' '
				   << e->label(graph_vector[graphnum].systems) << '\n';
		for (HighwayGraphCollapsedEdgeInfo *e : mce)
			collapfile << e->collapsed_tmg_line(graph_vector[graphnum].systems, threadnum) << '\n';
		simplefile.close();
		collapfile.close();

		graph_vector[graphnum].vertices = mv.size();
		graph_vector[graphnum+1].vertices = visible_v;
		graph_vector[graphnum].edges = mse.size();
		graph_vector[graphnum+1].edges = mce.size();
	}
};
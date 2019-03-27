std::mutex WaypointQuadtree::mtx;

bool WaypointQuadtree::WaypointQuadtree::refined()
{	return nw_child;
}

WaypointQuadtree::WaypointQuadtree(double MinLat, double MinLng, double MaxLat, double MaxLng)
{	// initialize an empty quadtree node on a given space"""
	min_lat = MinLat;
	min_lng = MinLng;
	max_lat = MaxLat;
	max_lng = MaxLng;
	mid_lat = (min_lat + max_lat) / 2;
	mid_lng = (min_lng + max_lng) / 2;
	nw_child = 0;
	ne_child = 0;
	sw_child = 0;
	se_child = 0;
	unique_locations = 0;
}

void WaypointQuadtree::refine()
{	// refine a quadtree into 4 sub-quadrants"""
	//std::cout << "QTDEBUG: " << str() << " being refined" << std::endl;
	nw_child = new WaypointQuadtree(mid_lat, min_lng, max_lat, mid_lng);
	ne_child = new WaypointQuadtree(mid_lat, mid_lng, max_lat, max_lng);
	sw_child = new WaypointQuadtree(min_lat, min_lng, mid_lat, mid_lng);
	se_child = new WaypointQuadtree(min_lat, mid_lng, mid_lat, max_lng);
	for (Waypoint *p : points) insert(p);
	points.clear();
}

void WaypointQuadtree::insert(Waypoint *w)
{	// insert Waypoint *w into this quadtree node
	//std::cout << "QTDEBUG: " << str() << " insert " << w->str() << std::endl;
	if (!refined())
	{	if (!waypoint_at_same_point(w))	//FIXME Try (!w->colocated) instead. Efficiency increase?
		{	//std::cout << "QTDEBUG: " << str() << " at " << unique_locations << " unique locations" << std::endl;
			unique_locations++;
		}
		points.push_front(w);
		if (unique_locations > 50)  // 50 unique points max per quadtree node
			refine();
	}
	else	if (w->lat < mid_lat)
			if (w->lng < mid_lng)
				sw_child->insert(w);
			else	se_child->insert(w);
		else	if (w->lng < mid_lng)
				nw_child->insert(w);
			else	ne_child->insert(w);
}

Waypoint *WaypointQuadtree::waypoint_at_same_point(Waypoint *w)
{	// find an existing waypoint at the same coordinates as w
	if (refined())
		if (w->lat < mid_lat)
			if (w->lng < mid_lng)
				return sw_child->waypoint_at_same_point(w);
			else	return se_child->waypoint_at_same_point(w);
		else	if (w->lng < mid_lng)
				return nw_child->waypoint_at_same_point(w);
			else	return ne_child->waypoint_at_same_point(w);
	for (Waypoint *p : points)
		if (p->same_coords(w)) return p;
	return 0;
}

std::forward_list<Waypoint*> WaypointQuadtree::near_miss_waypoints(Waypoint *w, double tolerance)
{	// compute and return a list of existing waypoints which are
	// within the near-miss tolerance (in degrees lat, lng) of w
	std::forward_list<Waypoint*> near_miss_points;

	//std::cout << "DEBUG: computing nmps for " << w->str() << " within " << std::to_string(tolerance) << " in " << str() << std::endl;
	// first check if this is a terminal quadrant, and if it is,
	// we search for NMPs within this quadrant
	if (!refined())
	{	//std::cout << "DEBUG: terminal quadrant comparing with " << std::distance(points.begin(), points.end()) << " points." << std::endl;
		for (Waypoint *p : points)
		  if (p != w && !p->same_coords(w) && p->nearby(w, tolerance))
		  {	//std::cout << "DEBUG: found nmp " << p->str() << std::endl;
			near_miss_points.push_front(p);
		  }
	}
	// if we're not a terminal quadrant, we need to determine which
	// of our child quadrants we need to search and recurse into
	// each
	else {	//std::cout << "DEBUG: recursive case, mid_lat=" << std::to_string(mid_lat) << " mid_lng=" << std::to_string(mid_lng) << std::endl;
		bool look_north = (w->lat + tolerance) >= mid_lat;
		bool look_south = (w->lat - tolerance) <= mid_lat;
		bool look_east = (w->lng + tolerance) >= mid_lng;
		bool look_west = (w->lng - tolerance) <= mid_lng;
		//std::cout << "DEBUG: recursive case, " << look_north << " " << look_south << " " << look_east << " " << look_west << std::endl;
		// now look in the appropriate child quadrants
		std::forward_list<Waypoint*> add_points;
		if (look_north && look_west)
		{	add_points = nw_child->near_miss_waypoints(w, tolerance);
			near_miss_points.splice_after(near_miss_points.before_begin(), add_points);
		}
		if (look_north && look_east)
		{	add_points = ne_child->near_miss_waypoints(w, tolerance);
			near_miss_points.splice_after(near_miss_points.before_begin(), add_points);
		}
		if (look_south && look_west)
		{	add_points = sw_child->near_miss_waypoints(w, tolerance);
			near_miss_points.splice_after(near_miss_points.before_begin(), add_points);
		}
		if (look_south && look_east)
		{	add_points = se_child->near_miss_waypoints(w, tolerance);
			near_miss_points.splice_after(near_miss_points.before_begin(), add_points);
		}
	     }
	return near_miss_points;
}

std::string WaypointQuadtree::str() //FIXME use sprintf
{	std::string s = "WaypointQuadtree at (" + \
		std::to_string(min_lat) + "," + std::to_string(min_lng) + ") to (" + \
		std::to_string(max_lat) + "," + std::to_string(max_lng) + ")";
	if (refined())
		return s + " REFINED";
	else	return s + " contains " + std::to_string(std::distance(points.begin(), points.end())) + " waypoints";
}

unsigned int WaypointQuadtree::size()
{	// return the number of Waypoints in the tree
	if (refined())
		return nw_child->size() + ne_child->size() + sw_child->size() + se_child->size();
	else	return std::distance(points.begin(), points.end()); //FIXME std::list faster here? Where & how often is WaypointQuadtree::size() used?
}

std::forward_list<Waypoint*> WaypointQuadtree::point_list()
{	// return a list of all points in the quadtree
	if (refined())
	{	std::forward_list<Waypoint*> all_points, add_points;
		add_points = sw_child->point_list();	all_points.splice_after(all_points.before_begin(), add_points);
		add_points = se_child->point_list();	all_points.splice_after(all_points.before_begin(), add_points);
		add_points = nw_child->point_list();	all_points.splice_after(all_points.before_begin(), add_points);
		add_points = ne_child->point_list();	all_points.splice_after(all_points.before_begin(), add_points);
		return all_points;
	}
	else	return points;
}

bool WaypointQuadtree::is_valid(ErrorList &el)
{	// make sure the quadtree is valid
	if (refined())
	{	// refined nodes should not contain points
		if (!points.empty()) el.add_error(str() + " contains " + std::to_string(std::distance(points.begin(), points.end())) + "waypoints");
		return nw_child->is_valid(el) && ne_child->is_valid(el) && sw_child->is_valid(el) && se_child->is_valid(el);
		// EDB: Removed tests for whether a node has children.
		// This made more sense in the original Python version of the code.
		// There, the criterion for whether a node was refined was whether points was None, or an empty list.
		// Static typing in C++ doesn't allow this, thus the refined() test becomes whether a node has children, making this sanity check moot.
	}
	else	// not refined, but should have no more than 50 unique points
	  if (unique_locations > 50)
	  {	el.add_error("WaypointQuadtree.is_valid terminal quadrant has too many unique points (" + std::to_string(unique_locations) + ")");
		return 0;
	  }
	std::cout << "WaypointQuadtree is valid." << std::endl;
	return 1;
}

unsigned int WaypointQuadtree::max_colocated()
{	// return the maximum number of waypoints colocated at any one location
	unsigned int max_col = 1;
	for (Waypoint *p : point_list())
	  if (max_col < p->num_colocated()) max_col = p->num_colocated();
	//std::cout << "Largest colocate count = " << max_col << std::endl;
	return max_col;
}

unsigned int WaypointQuadtree::total_nodes()
{	if (!refined())
		// not refined, no children, return 1 for self
		return 1;
	else	return 1 + nw_child->total_nodes() + ne_child->total_nodes() + sw_child->total_nodes() + se_child->total_nodes();
}

void WaypointQuadtree::sort()
{	if (refined())
	{	ne_child->sort();
		nw_child->sort();
		se_child->sort();
		sw_child->sort();
	}
	else	points.sort(sort_root_at_label);
}

void WaypointQuadtree::get_tmg_lines(std::list<std::string> &vertices, std::list<std::string> &edges, std::string n_name)
{	if (refined())
	{	double cmn_lat = min_lat;
		double cmx_lat = max_lat;
		if (cmn_lat < -80)	cmn_lat = -80;
		if (cmx_lat > 80)	cmx_lat = 80;
		edges.push_back(std::to_string(vertices.size())+" "+std::to_string(vertices.size()+1)+" "+n_name+"_NS");
		edges.push_back(std::to_string(vertices.size()+2)+" "+std::to_string(vertices.size()+3)+" "+n_name+"_EW");
		vertices.push_back(n_name+"@+S "+std::to_string(cmn_lat)+" "+std::to_string(mid_lng));
		vertices.push_back(n_name+"@+N "+std::to_string(cmx_lat)+" "+std::to_string(mid_lng));
		vertices.push_back(n_name+"@+W "+std::to_string(mid_lat)+" "+std::to_string(min_lng));
		vertices.push_back(n_name+"@+E "+std::to_string(mid_lat)+" "+std::to_string(max_lng));
		nw_child->get_tmg_lines(vertices, edges, n_name+"A");
		ne_child->get_tmg_lines(vertices, edges, n_name+"B");
		sw_child->get_tmg_lines(vertices, edges, n_name+"C");
		se_child->get_tmg_lines(vertices, edges, n_name+"D");
	}
}

void WaypointQuadtree::write_qt_tmg()
{	std::list<std::string> vertices, edges;
	get_tmg_lines(vertices, edges, "M");
	std::ofstream tmgfile("WaypointQuadtree.tmg");
	tmgfile << "TMG 1.0 simple\n";
	tmgfile << vertices.size() << ' ' << edges.size() << '\n';
	for (std::string v : vertices)	tmgfile << v << '\n';
	for (std::string e : edges)	tmgfile << e << '\n';
	tmgfile.close();
}
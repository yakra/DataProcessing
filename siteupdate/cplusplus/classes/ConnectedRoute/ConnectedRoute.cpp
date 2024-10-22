#include "ConnectedRoute.h"
#include "../Datacheck/Datacheck.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/tmstring.h"

ConnectedRoute::ConnectedRoute(std::string &line, HighwaySystem *sys, ErrorList &el)
{	mileage = 0;
	disconnected = 0;

	// parse chopped routes csv line
	system = sys;
	size_t NumFields = 5;
	std::string sys_str, roots_str;
	std::string* fields[5] = {&sys_str, &route, &banner, &groupname, &roots_str};
	split(line, fields, NumFields, ';');
	if (NumFields != 5)
	{	el.add_error("Could not parse " + sys->systemname
			   + "_con.csv line: [" + line + "], expected 5 fields, found " + std::to_string(NumFields));
		return;
	}
	// system
	if (system->systemname != sys_str)
		el.add_error("System mismatch parsing " + system->systemname
			   + "_con.csv line [" + line + "], expected " + system->systemname);
	// route
	if (route.size() > DBFieldLength::route)
		el.add_error("route > " + std::to_string(DBFieldLength::route)
			   + " bytes in " + system->systemname + "_con.csv line: " + line);
	// banner
	if (banner.size() > DBFieldLength::banner)
		el.add_error("banner > " + std::to_string(DBFieldLength::banner)
			   + " bytes in " + system->systemname + "_con.csv line: " + line);
	// groupname
	if (groupname.size() > DBFieldLength::city)
		el.add_error("groupname > " + std::to_string(DBFieldLength::city)
			   + " bytes in " + system->systemname + "_con.csv line: " + line);
	// roots
	lower(roots_str.data());
	int rootOrder = 0;
	size_t l = 0;
	for (size_t r = 0; r != -1; l = r+1)
	{	r = roots_str.find(',', l);
		try {	Route *root = Route::root_hash.at(roots_str.substr(l, r-l));
			roots.push_back(root);
			if (root->con_route)
			  el.add_error("Duplicate root in " + sys->systemname + "_con.csv: " + root->root +
				       " already in " + root->con_route->system->systemname + "_con.csv");
			if (system != root->system)
			  el.add_error("System mismatch: chopped route " + root->root + " from " + root->system->systemname +
				       ".csv in connected route in " + system->systemname + "_con.csv");
			root->con_route = this;
			// save order of route in connected route
			root->rootOrder = rootOrder;
			rootOrder++;
		    }
		catch (std::out_of_range& oor)
		    {	el.add_error("Could not find Route matching ConnectedRoute root " + roots_str.substr(l, r-l) +
					" in system " + system->systemname + '.');
		    }
	}
	if (roots.size() < 1) el.add_error("No valid roots in " + system->systemname + "_con.csv line: " + line);
}

std::string ConnectedRoute::connected_rtes_line()
{	/* return a connected routes system csv line, for debug purposes */
	std::string line = system->systemname + ';' + route + ';' + banner + ';' + groupname + ';';
	if (!roots.empty())
	{	line += roots[0]->root;
		for (size_t i = 1; i < roots.size(); i++)
			line += ',' + roots[i]->root;
	}
	return line;
}

std::string ConnectedRoute::readable_name()
{	/* return a string for a human-readable connected route name */
	std::string ans = route + banner;
	if (!groupname.empty()) ans += " (" +  groupname + ")";
	return ans;
}

size_t ConnectedRoute::index()
{	return this - system->con_routes.data;
}

/*std::string ConnectedRoute::list_lines(int pos, int len, std::string newline, size_t indent)
{	// return .list file lines marking (len) consecutive
	// segments, starting at waypoint (pos) segments into route
	//std::cout << "\nDEBUG: list_lines for " << readable_name() << " (" << roots.size() << " connected root(s))" << std::endl;
	std::string lines;
	for (Route *r : roots)
	{	//std::cout << "DEBUG: [" << pos << " + " << len << " = " << pos+len << "] " << r->str() << std::endl;
		std::string line = std::string(indent, ' ') + r->list_line(pos, pos+len);
		if (line.size() > indent) lines += line + newline;
		pos -= r->segments.size;
	}
	// strip final newline
	while (lines.back() == '\n' || lines.back() == '\r') lines.pop_back();
	return lines;
}//*/

void ConnectedRoute::verify_connectivity()
{	for (size_t i = 1; i < roots.size(); i++)
	{	// check for mismatched route endpoints within connected routes
		auto& q = roots[i-1];
		auto& r = roots[i];
		auto flag = [&]()
		{	Datacheck::add(q, q->con_end()->label, "", "", "DISCONNECTED_ROUTE",  r->points[0].root_at_label());
			Datacheck::add(r,  r->points[0].label, "", "", "DISCONNECTED_ROUTE", q->con_end()->root_at_label());
			disconnected = 1;
			q->set_disconnected();
			r->set_disconnected();
		};
		if ( q->points.size > 1 && r->points.size > 1 && !r->points.begin()->same_coords(q->con_end()) )
			if	( q->con_end()->same_coords(&r->points.back()) )	// R can be reversed
			  if	( q->con_beg()->same_coords(r->points.begin())		// Can Q be reversed instead?
			  &&	( q == roots[0] || q->is_disconnected() )		// Is Q not locked into one direction?
			  &&	( i+1 < roots.size() )					// Is there another chopped route after R?
			  &&	(    r->points.back().same_coords(roots[i+1]->points.begin())		// And does its beginning
				  || r->points.back().same_coords(&roots[i+1]->points.back()) ))	// or end link to R as-is?
				q->set_reversed();
			  else	r->set_reversed();
			else if ( q->con_beg()->same_coords(&r->points.back()) )	// Q & R can both be reversed together
			  if	( q == roots[0] || q->is_disconnected() )		// as long as Q's direction is unknown
			  {	q->set_reversed();
				r->set_reversed();
			  }
			  else	flag();
			else if ( q->con_beg()->same_coords(r->points.begin())		// Only Q can be reversed
			     && ( q == roots[0] || q->is_disconnected() ))		// as long as its direction is unknown
				q->set_reversed();
			else	flag();
	}
}

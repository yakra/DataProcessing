#include "ConnectedRoute.h"
#include "../Datacheck/Datacheck.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"

void ConnectedRoute::verify_connectivity()
{   for (size_t i = 1; i < roots.size(); i++)
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

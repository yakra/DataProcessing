#define FMT_HEADER_ONLY
#include "TravelerList.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../../functions/tmstring.h"
#include <fmt/format.h>
#include <fstream>

void TravelerList::userlog(const double total_active_only_miles, const double total_active_preview_miles)
{	char fstr[112];
	std::cout << "." << std::flush;
	std::ofstream log(Args::logfilepath+"/users/"+traveler_name+".log", std::ios::app);
	log << "Clinched Highway Statistics\n";
	log << "Overall in active systems: " << format_clinched_mi(fstr, active_only_miles(), total_active_only_miles) << '\n';
	log << "Overall in active+preview systems: " << format_clinched_mi(fstr, active_preview_miles(), total_active_preview_miles) << '\n';

	log << "Overall by region: (each line reports active only then active+preview)\n";
	std::list<Region*> travregions;
	for (std::pair<Region* const, double> &rm : active_preview_mileage_by_region)
		travregions.push_back(rm.first);
	travregions.sort();
	for (Region *region : travregions)
	{	double t_active_miles = 0;
		if (active_only_mileage_by_region.count(region))
			t_active_miles = active_only_mileage_by_region.at(region);
		log << region->code << ": " << format_clinched_mi(fstr, t_active_miles, region->active_only_mileage) << ", "
		    << format_clinched_mi(fstr, active_preview_mileage_by_region.at(region), region->active_preview_mileage) << '\n';
	}
	unsigned int active_systems_traveled = 0;
	unsigned int active_systems_clinched = 0;
	unsigned int preview_systems_traveled = 0;
	unsigned int preview_systems_clinched = 0;
	size_t index = this - allusers.data;

	// stats by system
	for (HighwaySystem *h = HighwaySystem::syslist.data, *end = HighwaySystem::syslist.end(); h != end; h++)
	  if (h->active_or_preview())
	  {	if (system_region_mileages.count(h))
		{	double t_system_overall = system_miles(h);
			if (h->active())
				active_systems_traveled++;
			else	preview_systems_traveled++;

			// stats by region covered by system, always in csmbr for
			// the DB, but add to logs only if it's been traveled at
			// all and it covers multiple regions
			auto& sysmbr = h->mileage_by_region;
			log << "System " << h->systemname << " (" << h->level_name() << ") overall: "
			    << format_clinched_mi(fstr, t_system_overall, h->total_mileage()) << '\n';
			if (sysmbr.size() > 1)
			{	log << "System " << h->systemname << " by region:\n";
				std::list<Region*> sysregions;
				for (std::pair<Region* const, double> &rm : sysmbr)
					sysregions.push_back(rm.first);
				sysregions.sort();
				for (Region *region : sysregions)
				{	double system_region_mileage = 0;
					auto it = system_region_mileages.at(h).find(region);
					if (it != system_region_mileages.at(h).end())
						system_region_mileage = it->second;
					log << "  " << region->code << ": " << format_clinched_mi(fstr, system_region_mileage, sysmbr.at(region)) << '\n';
				}
			}

			// stats by highway for the system, by connected route and
			// by each segment crossing region boundaries if applicable
			unsigned int num_con_rtes_traveled = 0;
			unsigned int num_con_rtes_clinched = 0;
			log << "System " << h->systemname << " by route (traveled routes only):\n";
			for (ConnectedRoute& cr : h->con_routes)
			{	double con_clinched_miles = 0;
				std::vector<std::pair<Route*, double>> chop_mi;
				auto& roots = cr.roots;
				for (Route *r : roots)
				{	// find traveled mileage on this by this user
					double miles = r->clinched_by_traveler_index(index);
					if (miles)
					{	cr_values.emplace_back(r, miles);
						con_clinched_miles += miles;
						chop_mi.emplace_back(r, miles);
					}
				}
				if (con_clinched_miles)
				{	num_con_rtes_traveled += 1;
					num_con_rtes_clinched += (con_clinched_miles == cr.mileage);
					ccr_values.emplace_back(&cr, con_clinched_miles);
					log << cr.readable_name() << ": " << format_clinched_mi(fstr, con_clinched_miles, cr.mileage) << '\n';
					if (roots.size() == 1)
						log << " (" << roots[0]->readable_name() << " only)\n";
					else {	for (auto& rm : chop_mi)
						    log << "  " << rm.first->readable_name() << ": "
							<< format_clinched_mi(fstr, rm.second,rm.first->mileage) << "\n";
						log << '\n';
					     }
				}
			}
			if (num_con_rtes_clinched == h->con_routes.size)
			  if (h->active())
				active_systems_clinched++;
			  else	preview_systems_clinched++;
			*fmt::format_to(fstr, " connected routes traveled: {} of {} ({:.1f}%), clinched: {} of {} ({:.1f}%).",
				num_con_rtes_traveled, (int)h->con_routes.size, 100*(double)num_con_rtes_traveled/h->con_routes.size,
				num_con_rtes_clinched, (int)h->con_routes.size, 100*(double)num_con_rtes_clinched/h->con_routes.size) = 0;
			log << "System " << h->systemname << fstr << '\n';
		}
	  }

	// grand summary, active only
	*fmt::format_to(fstr,"\nTraveled {} of {} ({:.1f}%), Clinched {} of {} ({:.1f}%) active systems",
		active_systems_traveled, HighwaySystem::num_active, 100*(double)active_systems_traveled/HighwaySystem::num_active,
		active_systems_clinched, HighwaySystem::num_active, 100*(double)active_systems_clinched/HighwaySystem::num_active) = 0;
	log << fstr << '\n';
	// grand summary, active+preview
	*fmt::format_to(fstr,"Traveled {} of {} ({:.1f}%), Clinched {} of {} ({:.1f}%) preview systems",
		preview_systems_traveled, HighwaySystem::num_preview, 100*(double)preview_systems_traveled/HighwaySystem::num_preview,
		preview_systems_clinched, HighwaySystem::num_preview, 100*(double)preview_systems_clinched/HighwaySystem::num_preview) = 0;
	log << fstr << '\n';

	// updated routes, sorted by date
	log << "\nMost recent updates for listed routes:\n";
	std::list<Route*> route_list(updated_routes.begin(), updated_routes.end());
	updated_routes.clear();
	route_list.sort(sort_route_updates_oldest);
	for (Route* r : route_list)
	    log	<< r->last_update[0] << " | " << r->last_update[1] << " | " << r->last_update[2] << " | "
		<< r->last_update[3] << " | " << r->last_update[4] << '\n';

	log.close();
}

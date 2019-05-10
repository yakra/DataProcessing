void userlog
(	ClinchedDBValues *clin_db_val,
	const double total_active_only_miles,
	const double total_active_preview_miles,
	std::list<HighwaySystem*> *highway_systems,
	std::string path
)
{	char fstr[112];
	std::cout << "." << std::flush;
	std::string filename = path+traveler_name+".log";
	std::ofstream log(filename.data(), std::ios::app);
	log << "Clinched Highway Statistics\n";
	log << "Overall in active systems: " << format_clinched_mi(active_only_miles(), total_active_only_miles) << '\n';
	log << "Overall in active+preview systems: " << format_clinched_mi(active_preview_miles(), total_active_preview_miles) << '\n';

	log << "Overall by region: (each line reports active only then active+preview)\n";
	std::list<Region*> travregions;
	for (std::pair<Region* const, double> &rm : active_preview_mileage_by_region)
		travregions.push_back(rm.first);
	travregions.sort(sort_regions_by_code);
	for (Region *region : travregions)
	{	double t_active_miles = 0;
		double total_active_miles = 0;
		if (active_only_mileage_by_region.find(region) != active_only_mileage_by_region.end())
		{	t_active_miles = active_only_mileage_by_region.at(region);
			total_active_miles = region->active_only_mileage;
		}
		log << region->code << ": " << format_clinched_mi(t_active_miles, total_active_miles) << ", "
		    << format_clinched_mi(active_preview_mileage_by_region.at(region), region->active_preview_mileage) << '\n';
	}

	unsigned int active_systems = 0;
	unsigned int preview_systems = 0;

	// present stats by system here, also generate entries for
	// DB table clinchedSystemMileageByRegion as we compute and
	// have the data handy
	for (HighwaySystem *h : *highway_systems)
	  if (h->active_or_preview())
	  {	if (h->active()) active_systems++;
		else	preview_systems++;
		double t_system_overall = 0;
		if (system_region_mileages.find(h) != system_region_mileages.end())
			t_system_overall = system_region_miles(h);
		log << "System " << h->systemname << " (" << h->level_name() << ") overall: " << format_clinched_mi(t_system_overall, h->total_mileage()) << '\n';
		if (t_system_overall)
		  if (h->active())
			active_systems_traveled++;
		  else	preview_systems_traveled++;
		if (t_system_overall == h->total_mileage())
		  if (h->active())
			active_systems_clinched++;
		  else	preview_systems_clinched++;

		// stats by region covered by system, always in csmbr for
		// the DB, but add to logs only if it's been traveled at
		// all and it covers multiple regions
		if (t_system_overall)
		{	if (h->mileage_by_region.size() > 1)
				log << "System " << h->systemname << " by region:\n";
			std::list<Region*> sysregions;
			for (std::pair<Region* const, double> &rm : h->mileage_by_region)
				sysregions.push_back(rm.first);
			sysregions.sort(sort_regions_by_code);
			for (Region *region : sysregions)
			{	double system_region_mileage = 0;
				if (system_region_mileages.find(h) != system_region_mileages.end()
				 && system_region_mileages.at(h).find(region) != system_region_mileages.at(h).end())
				{	system_region_mileage = system_region_mileages.at(h).at(region);
					sprintf(fstr, "%.15g", system_region_mileage);
					if (!strchr(fstr, '.')) strcat(fstr, ".0");
					clin_db_val->add_csmbr("('" + h->systemname + "','" + region->code + "','" +
							       traveler_name + "','" + fstr + "')");
				}
				if (h->mileage_by_region.size() > 1)
					log << "  " << region->code << ": " << format_clinched_mi(system_region_mileage, h->mileage_by_region.at(region)) << '\n';
			}
		}

		// stats by highway for the system, by connected route and
		// by each segment crossing region boundaries if applicable
		if (t_system_overall)
		{	std::unordered_map<ConnectedRoute*, double> system_con_umap;
			unsigned int num_con_rtes_clinched = 0;
			log << "System " << h->systemname << " by route (traveled routes only):\n";
			for (ConnectedRoute &cr : h->con_route_list)
			{	double con_total_miles = 0;
				double con_clinched_miles = 0;
				std::string to_write = "";
				for (Route *r : cr.roots)
				{	// find traveled mileage on this by this user
					double miles = r->clinched_by_traveler(this);
					if (miles)
					{	char clinched = '0';
						if (miles >= r->mileage) clinched = '1';
						sprintf(fstr, "%.15g", miles);
						if (!strchr(fstr, '.')) strcat(fstr, ".0");
						clin_db_val->add_cr("('" + r->root + "','" + traveler_name + "','" + std::string(fstr) + "','" + clinched + "')");
						routes_traveled[r] = miles;
						con_clinched_miles += miles;
						to_write += "  " + r->readable_name() + ": " + format_clinched_mi(miles,r->mileage) + "\n";
					}
					con_total_miles += r->mileage;
				}
				if (con_clinched_miles)
				{	system_con_umap[&cr] = con_clinched_miles;
					char clinched = '0';
					/*yDEBUG
					if (traveler_name == "oscar" && cr.system->systemname == "usaus" && cr.route == "US85")
						printf("\nOscar on US85:\ncon_clinched_miles = %.17f\n   con_total_miles = %.17f\n",
							con_clinched_miles, con_total_miles);//*/
					if (con_clinched_miles == con_total_miles)
					{	num_con_rtes_clinched++;
						clinched = '1';
					}
					sprintf(fstr, "%.15g", con_clinched_miles);
					if (!strchr(fstr, '.')) strcat(fstr, ".0");
					clin_db_val->add_ccr("('" + cr.roots[0]->root + "','" + traveler_name +
							     "','" + std::string(fstr) + "','" + clinched + "')");
					log << cr.readable_name() << ": " << format_clinched_mi(con_clinched_miles, con_total_miles) << '\n';
					if (cr.roots.size() == 1)
						log << " (" << cr.roots[0]->readable_name() << " only)\n";
					else	log << to_write << '\n';
				}
			}
			con_routes_clinched[h] = num_con_rtes_clinched;
			sprintf(fstr, " connected routes traveled: %i of %i (%.1f%%), clinched: %i of %i (%.1f%%).",
				(int)system_con_umap.size(), (int)h->con_route_list.size(), 100*(double)system_con_umap.size()/h->con_route_list.size(),
				num_con_rtes_clinched,    (int)h->con_route_list.size(),    100*(double)num_con_rtes_clinched/h->con_route_list.size());
			log << "System " << h->systemname << fstr << '\n';
			con_routes_traveled[h] = system_con_umap;
			//#include "debug/oscars_usaus_ConRtes.cpp"
		}
	  }

	// grand summary, active only
	sprintf(fstr,"Traveled %i of %i (%.1f%%), Clinched %i of %i (%.1f%%) active systems",
			active_systems_traveled, active_systems, 100*(double)active_systems_traveled/active_systems,
			active_systems_clinched, active_systems, 100*(double)active_systems_clinched/active_systems);
	log << fstr << '\n';
	// grand summary, active+preview
	sprintf(fstr,"Traveled %i of %i (%.1f%%), Clinched %i of %i (%.1f%%) preview systems",
			preview_systems_traveled, preview_systems, 100*(double)preview_systems_traveled/preview_systems,
			preview_systems_clinched, preview_systems, 100*(double)preview_systems_clinched/preview_systems);
	log << fstr << '\n';

	log.close();
}

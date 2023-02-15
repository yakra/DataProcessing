// Tab Width = 8

// Travel Mapping Project, Jim Teresco and Eric Bryant, 2015-2023
/* Code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015-2023, Jim Teresco and Eric Bryant
Original Python version by Jim Teresco, with contributions from Eric Bryant and the TravelMapping team
C++ translation by Eric Bryant

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
*/

#include <cstring>
#include <dirent.h>
#include "classes/Args/Args.h"
#include "classes/DBFieldLength/DBFieldLength.h"
#include "classes/ConnectedRoute/ConnectedRoute.h"
#include "classes/Datacheck/Datacheck.h"
#include "classes/ElapsedTime/ElapsedTime.h"
#include "classes/ErrorList/ErrorList.h"
#include "classes/GraphGeneration/GraphListEntry.h"
#include "classes/GraphGeneration/HGEdge.h"
#include "classes/GraphGeneration/HGVertex.h"
#include "classes/GraphGeneration/HighwayGraph.h"
#include "classes/GraphGeneration/PlaceRadius.h"
#include "classes/HighwaySegment/HighwaySegment.h"
#include "classes/HighwaySystem/HighwaySystem.h"
#include "classes/Region/Region.h"
#include "classes/Route/Route.h"
#include "classes/TravelerList/TravelerList.h"
#include "classes/Waypoint/Waypoint.h"
#include "classes/WaypointQuadtree/WaypointQuadtree.h"
#include "functions/crawl_hwy_data.h"
#include "functions/split.h"
#include "functions/sql_file.h"
#include "templates/contains.cpp"
#ifdef threading_enabled
#include <thread>
#include "threads/threads.h"
#endif
void allbyregionactiveonly(std::mutex*, double);
void allbyregionactivepreview(std::mutex*, double);
using namespace std;

int main(int argc, char *argv[])
{	ifstream file;
	string line;
	mutex list_mtx, term_mtx;

	// argument parsing
	if (Args::init(argc, argv)) return 1;
      #ifndef threading_enabled
	Args::numthreads = 1;
      #endif

	// start a timer for including elapsed time reports in messages
	ElapsedTime et(Args::timeprecision);
	time_t timestamp = time(0);
	cout << "Start: " << ctime(&timestamp);

	// create ErrorList
	ErrorList el;

	// Get list of travelers in the system
	TravelerList::ids = move(Args::userlist);
	if (TravelerList::ids.empty())
	{	DIR *dir;
		dirent *ent;
		if ((dir = opendir (Args::userlistfilepath.data())) != NULL)
		{	while ((ent = readdir (dir)) != NULL)
			{	string trav = ent->d_name;
				if (trav.size() > 5 && trav.substr(trav.size()-5, string::npos) == ".list")
					TravelerList::ids.push_back(trav);
			}
			closedir(dir);
		}
		else	el.add_error("Error opening user list file path \""+Args::userlistfilepath+"\". (Not found?)");
	}
	else for (string &id : TravelerList::ids) id += ".list";

	// read region, country, continent descriptions
	cout << et.et() << "Reading region, country, and continent descriptions." << endl;

	// continents
	vector<pair<string, string>> continents;
	file.open(Args::highwaydatapath+"/continents.csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/continents.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.size() && line.back() == 0x0D) line.pop_back();	// trim DOS newlines
			if (line.empty()) continue;
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse continents.csv line: [" + line
					   + "], expected 2 fields, found 1");
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse continents.csv line: [" + line
					   + "], expected 2 fields, found 3");
				continue;
			}
			// verify field lengths
			if (code.size() > DBFieldLength::continentCode)
				el.add_error("Continent code > " + std::to_string(DBFieldLength::continentCode)
					   + " bytes in continents.csv line " + line);
			if (name.size() > DBFieldLength::continentName)
				el.add_error("Continent name > " + std::to_string(DBFieldLength::continentName)
					   + " bytes in continents.csv line " + line);
			continents.emplace_back(pair<string, string>(code, name));
		}
	     }
	file.close();
	// create a dummy continent to catch unrecognized continent codes in .csv files
	continents.emplace_back(pair<string, string>("error", "unrecognized continent code"));

	// countries
	vector<pair<string, string>> countries;
	file.open(Args::highwaydatapath+"/countries.csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/countries.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.size() && line.back() == 0x0D) line.pop_back();	// trim DOS newlines
			if (line.empty()) continue;
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse countries.csv line: [" + line
					   + "], expected 2 fields, found 1");
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse countries.csv line: [" + line
					   + "], expected 2 fields, found 3");
				continue;
			}
			// verify field lengths
			if (code.size() > DBFieldLength::countryCode)
				el.add_error("Country code > " + std::to_string(DBFieldLength::countryCode)
					   + " bytes in countries.csv line " + line);
			if (name.size() > DBFieldLength::countryName)
				el.add_error("Country name > " + std::to_string(DBFieldLength::countryName)
					   + " bytes in countries.csv line " + line);
			countries.emplace_back(pair<string, string>(code, name));
		}
	     }
	file.close();
	// create a dummy country to catch unrecognized country codes in .csv files
	countries.emplace_back(pair<string, string>("error", "unrecognized country code"));

	//regions
	file.open(Args::highwaydatapath+"/regions.csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/regions.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.size() && line.back() == 0x0D) line.pop_back();	// trim DOS newlines
			if (line.empty()) continue;
			Region* r = new Region(line, countries, continents, el);
				    // deleted on termination of program
			if (r->is_valid)
			{	Region::allregions.push_back(r);
				Region::code_hash[r->code] = r;
			} else	delete r;
		}
	     }
	file.close();
	// create a dummy region to catch unrecognized region codes in .csv files
	Region::allregions.push_back(new Region("error;unrecognized region code;error;error;unrecognized region code", countries, continents, el));
	Region::code_hash[Region::allregions.back()->code] = Region::allregions.back();

	// Create a list of HighwaySystem objects, one per system in systems.csv file
	cout << et.et() << "Reading systems list in " << Args::highwaydatapath << "/" << Args::systemsfile << "." << endl;
	file.open(Args::highwaydatapath+"/"+Args::systemsfile);
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/"+Args::systemsfile);
	else {	getline(file, line); // ignore header line
		list<string> ignoring;
		while(getline(file, line))
		{	if (line.size() && line.back() == 0x0D) line.pop_back();	// trim DOS newlines
			if (line.empty()) continue;
			if (line[0] == '#')
			{	ignoring.push_back("Ignored comment in " + Args::systemsfile + ": " + line);
				continue;
			}
			HighwaySystem *hs = new HighwaySystem(line, el, countries);
					    // deleted on termination of program
			if (!hs->is_valid) delete hs;
			else {	HighwaySystem::syslist.push_back(hs);
			     }
		}
		cout << endl;
		// at the end, print the lines ignored
		for (string& l : ignoring) cout << l << endl;
		ignoring.clear();
	     }
	file.close();

	// For tracking whether any .wpt files are in the directory tree
	// that do not have a .csv file entry that causes them to be
	// read into the data
	cout << et.et() << "Finding all .wpt files. " << flush;
	unordered_set<string> splitsystems;
	crawl_hwy_data(Args::highwaydatapath+"/hwy_data", splitsystems, 0);
	cout << Route::all_wpt_files.size() << " files found." << endl;

	// For finding colocated Waypoints and concurrent segments, we have
	// quadtree of all Waypoints in existence to find them efficiently
	WaypointQuadtree all_waypoints(-90,-180,90,180);

	// Next, read all of the .wpt files for each HighwaySystem
	cout << et.et() << "Reading waypoints for all routes." << endl;
      #ifdef threading_enabled
	std::vector<std::thread> thr(Args::numthreads);
	HighwaySystem::it = HighwaySystem::syslist.begin();
	#define THREADLOOP for (unsigned int t = 0; t < thr.size(); t++)
	THREADLOOP thr[t] = thread(ReadWptThread, t, &list_mtx, &el, &all_waypoints);
	THREADLOOP thr[t].join();
      #else
	for (HighwaySystem* h : HighwaySystem::syslist)
	{	std::cout << h->systemname << std::flush;
		bool usa_flag = h->country->first == "USA";
		for (Route* r : h->route_list)
			r->read_wpt(&all_waypoints, &el, usa_flag);
		std::cout << "!" << std::endl;
	}
      #endif

	//cout << et.et() << "Writing WaypointQuadtree.tmg." << endl;
	//all_waypoints.write_qt_tmg(Args::logfilepath+"/WaypointQuadtree.tmg");
	cout << et.et() << "Sorting waypoints in Quadtree." << endl;
	all_waypoints.sort();

	cout << et.et() << "Finding unprocessed wpt files." << endl;
	if (Route::all_wpt_files.size())
	{	ofstream unprocessedfile(Args::logfilepath+"/unprocessedwpts.log");
		cout << Route::all_wpt_files.size() << " .wpt files in " << Args::highwaydatapath << "/hwy_data not processed, see unprocessedwpts.log." << endl;
		list<string> all_wpts_list(Route::all_wpt_files.begin(), Route::all_wpt_files.end());
		all_wpts_list.sort();
		for (const string &f : all_wpts_list) unprocessedfile << strstr(f.data(), "hwy_data") << '\n';
		unprocessedfile.close();
		Route::all_wpt_files.clear();
	}
	else	cout << "All .wpt files in " << Args::highwaydatapath << "/hwy_data processed." << endl;

      #ifdef threading_enabled
	// create NMP lists
	cout << et.et() << "Searching for near-miss points." << endl;
	HighwaySystem::it = HighwaySystem::syslist.begin();
	THREADLOOP thr[t] = thread(NmpSearchThread, t, &list_mtx, &all_waypoints);
	THREADLOOP thr[t].join();
      #endif

	// Near-miss point log
	cout << et.et() << "Near-miss point log and tm-master.nmp file." << endl;

	// read in fp file
	unordered_set<string> nmpfps;
	file.open(Args::highwaydatapath+"/nmpfps.log");
	while (getline(file, line))
	{	while (line.size() && (line.back() == ' ' || line.back() == '\r'))
			line.pop_back(); // trim DOS newlines & whitespace
		if (line.size())
		    if (line.size() >= 51)
			if (!strcmp(line.data()+line.size()-20, " [LOOKS INTENTIONAL]"))
				nmpfps.emplace(line, 0, line.size()-20);
			else if (line.size() >= 55 && !strcmp(line.data()+line.size()-24, " [SOME LOOK INTENTIONAL]"))
				nmpfps.emplace(line, 0, line.size()-24);
			else	nmpfps.emplace(std::move(line));
		    else	nmpfps.emplace(std::move(line));
	}
	file.close();

	list<string> nmploglines;
	ofstream nmplog(Args::logfilepath+"/nearmisspoints.log");
	ofstream nmpnmp(Args::logfilepath+"/tm-master.nmp");
	for (Waypoint *w : all_waypoints.point_list()) w->nmplogs(nmpfps, nmpnmp, nmploglines);
	nmpnmp.close();

	// sort and write actual lines to nearmisspoints.log
	nmploglines.sort();
	for (string &n : nmploglines) nmplog << n << '\n';
	nmploglines.clear();
	nmplog.close();

	// report any unmatched nmpfps.log entries
	ofstream nmpfpsunmatchedfile(Args::logfilepath+"/nmpfpsunmatched.log");
	list<string> nmpfplist(nmpfps.begin(), nmpfps.end());
	nmpfplist.sort();
	for (string &line : nmpfplist)
		nmpfpsunmatchedfile << line << '\n';
	nmpfpsunmatchedfile.close();
	nmpfplist.clear();
	nmpfps.clear();

	// if requested, rewrite data with near-miss points merged in
	if (Args::nmpmergepath != "" && !Args::errorcheck)
	{	cout << et.et() << "Writing near-miss point merged wpt files." << endl;
	      #ifdef threading_enabled
		HighwaySystem::it = HighwaySystem::syslist.begin();
		THREADLOOP thr[t] = thread(NmpMergedThread, t, &list_mtx);
		THREADLOOP thr[t].join();
	      #else
		for (HighwaySystem *h : HighwaySystem::syslist)
		{	std::cout << h->systemname << std::flush;
			for (Route *r : h->route_list)
			  r->write_nmp_merged();
			std::cout << '.' << std::flush;
		}
	      #endif
		cout << endl;
	}

	#include "tasks/concurrency_detection.cpp"

	cout << et.et() << "Creating label hashes and checking route integrity." << endl;
      #ifdef threading_enabled
	HighwaySystem::it = HighwaySystem::syslist.begin();
	THREADLOOP thr[t] = thread(RteIntThread, t, &list_mtx, &el);
	THREADLOOP thr[t].join();
      #else
	for (HighwaySystem *h : HighwaySystem::syslist)
	  h->route_integrity(el);
      #endif

	#include "tasks/read_updates.cpp"

	// Create a list of TravelerList objects, one per person
	cout << et.et() << "Processing traveler list files:" << endl;
      #ifdef threading_enabled
	TravelerList::id_it = TravelerList::ids.begin();
	THREADLOOP thr[t] = thread(ReadListThread, t, &list_mtx, &el);
	THREADLOOP thr[t].join();
      #else
	for (string &t : TravelerList::ids)
	{	cout << t << ' ' << std::flush;
		TravelerList::allusers.push_back(new TravelerList(t, &el));
	}
      #endif
	TravelerList::ids.clear();
	cout << endl << et.et() << "Processed " << TravelerList::allusers.size() << " traveler list files. Sorting and numbering." << endl;
	TravelerList::allusers.sort(sort_travelers_by_name);
	// assign traveler numbers for master traveled graph
	unsigned int travnum = 0;
	for (TravelerList *t : TravelerList::allusers)
	{	t->traveler_num[0] = travnum;
		travnum++;
	}

	cout << et.et() << "Clearing route & label hash tables." << endl;
	Route::root_hash.clear();
	Route::pri_list_hash.clear();
	Route::alt_list_hash.clear();
	for (HighwaySystem* h : HighwaySystem::syslist)
	  for (Route* r : h->route_list)
	  {	r->pri_label_hash.clear();
		r->alt_label_hash.clear();
		r->duplicate_labels.clear();
	  }

	cout << et.et() << "Writing route and label logs." << endl;
	unsigned int total_unused_alt_labels = 0;
	unsigned int total_unusedaltroutenames = 0;
	list<string> unused_alt_labels;
	ofstream piufile(Args::logfilepath+"/pointsinuse.log");
	ofstream lniufile(Args::logfilepath+"/listnamesinuse.log");
	ofstream uarnfile(Args::logfilepath+"/unusedaltroutenames.log");
	ofstream flipfile(Args::logfilepath+"/flippedroutes.log");
	timestamp = time(0);
	piufile << "Log file created at: " << ctime(&timestamp);
	lniufile << "Log file created at: " << ctime(&timestamp);
	uarnfile << "Log file created at: " << ctime(&timestamp);
	for (HighwaySystem *h : HighwaySystem::syslist)
	{	for (Route *r : h->route_list)
		{	// labelsinuse.log line
			if (r->labels_in_use.size())
			{	piufile << r->root << '(' << r->point_list.size() << "):";
				list<string> liu_list(r->labels_in_use.begin(), r->labels_in_use.end());
				liu_list.sort();
				for (string& label : liu_list) piufile << ' ' << label;
				piufile << '\n';
				r->labels_in_use.clear();
			}
			// unusedaltlabels.log lines, to be sorted by root later
			if (r->unused_alt_labels.size())
			{	total_unused_alt_labels += r->unused_alt_labels.size();
				string ual_entry = r->root + '(' + to_string(r->unused_alt_labels.size()) + "):";
				list<string> ual_list(r->unused_alt_labels.begin(), r->unused_alt_labels.end());
				ual_list.sort();
				for (string& label : ual_list) ual_entry += ' ' + label;
				unused_alt_labels.push_back(ual_entry);
				r->unused_alt_labels.clear();
			}
			// flippedroutes.log line
			if (r->is_reversed()) flipfile << r->root << '\n';
		}
		// listnamesinuse.log line
		if (h->listnamesinuse.size())
		{	lniufile << h->systemname << '(' << h->route_list.size() << "):";
			list<string> lniu_list(h->listnamesinuse.begin(), h->listnamesinuse.end());
			lniu_list.sort();
			for (string& list_name : lniu_list) lniufile << " \"" << list_name << '"';
			lniufile << '\n';
			h->listnamesinuse.clear();
		}
		// unusedaltroutenames.log line
		if (h->unusedaltroutenames.size())
		{	total_unusedaltroutenames += h->unusedaltroutenames.size();
			uarnfile << h->systemname << '(' << h->unusedaltroutenames.size() << "):";
			list<string> uarn_list(h->unusedaltroutenames.begin(), h->unusedaltroutenames.end());
			uarn_list.sort();
			for (string& list_name : uarn_list) uarnfile << " \"" << list_name << '"';
			uarnfile << '\n';
			h->unusedaltroutenames.clear();
		}
	}
	piufile.close();
	lniufile.close();
	flipfile.close();
	uarnfile << "Total: " << total_unusedaltroutenames << '\n';
	uarnfile.close();
	// sort lines and write unusedaltlabels.log
	unused_alt_labels.sort();
	ofstream ualfile(Args::logfilepath+"/unusedaltlabels.log");
	timestamp = time(0);
	ualfile << "Log file created at: " << ctime(&timestamp);
	for (string &ual_entry : unused_alt_labels) ualfile << ual_entry << '\n';
	unused_alt_labels.clear();
	ualfile << "Total: " << total_unused_alt_labels << '\n';
	ualfile.close();


	// now augment any traveler clinched segments for concurrencies
	cout << et.et() << "Augmenting travelers for detected concurrent segments." << flush;
      #ifdef threading_enabled
	auto augment_lists = new vector<string>[Args::numthreads];
				      // deleted once written to concurrencies.log
	TravelerList::tl_it = TravelerList::allusers.begin();
	THREADLOOP thr[t] = thread(ConcAugThread, t, &list_mtx, augment_lists+t);
	THREADLOOP thr[t].join();
	cout << "!\n" << et.et() << "Writing to concurrencies.log." << endl;
	THREADLOOP for (std::string& entry : augment_lists[t]) concurrencyfile << entry << '\n';
	delete[] augment_lists;
      #else
	for (TravelerList *t : TravelerList::allusers)
	{	cout << '.' << flush;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs != s && hs->route->system->active_or_preview() && hs->add_clinched_by(t))
		       	concurrencyfile << "Concurrency augment for traveler " << t->traveler_name << ": [" << hs->str() << "] based on [" << s->str() << "]\n";
	}
	cout << '!' << endl;
      #endif
	concurrencyfile.close();

	/*ofstream sanetravfile(Args::logfilepath+"/concurrent_travelers_sanity_check.log");
	for (HighwaySystem *h : HighwaySystem::syslist)
	    for (Route *r : h->route_list)
		for (HighwaySegment *s : r->segment_list)
		    sanetravfile << s->concurrent_travelers_sanity_check();
	sanetravfile.close(); //*/

	// compute lots of regional stats:
	// overall, active+preview, active only,
	// and per-system which falls into just one of these categories
      #ifdef threading_enabled
	cout << et.et() << "Performing per-route data checks and computing stats." << flush;
	HighwaySystem::it = HighwaySystem::syslist.begin();
	THREADLOOP thr[t] = thread(CompStatsRThread, t, &list_mtx);
	THREADLOOP thr[t].join();
	cout << '!' << endl;
	cout << et.et() << "Computing stats per traveler." << flush;
	TravelerList::tl_it = TravelerList::allusers.begin();
	THREADLOOP thr[t] = thread(CompStatsTThread, t, &list_mtx);
	THREADLOOP thr[t].join();
	cout << '!' << endl;
      #else
	cout << et.et() << "Performing per-route data checks and computing stats." << flush;
	for (HighwaySystem *h : HighwaySystem::syslist)
	{	cout << "." << flush;
		for (Route *r : h->route_list)
		{ 	r->compute_stats_r();
		  	for (HighwaySegment *s : r->segment_list)
			  for (TravelerList *t : s->clinched_by)
			    s->compute_stats_t(t);
		}
	}
	cout << '!' << endl;
      #endif

	cout << et.et() << "Writing highway data stats log file (highwaydatastats.log)." << endl;
	char fstr[112];
	ofstream hdstatsfile(Args::logfilepath+"/highwaydatastats.log");
	timestamp = time(0);
	hdstatsfile << "Travel Mapping highway mileage as of " << ctime(&timestamp);

	double active_only_miles = 0;
	double active_preview_miles = 0;
	double overall_miles = 0;
	for (Region* r : Region::allregions)
	{	active_only_miles += r->active_only_mileage;
		active_preview_miles += r->active_preview_mileage;
		overall_miles += r->overall_mileage;
	}
	sprintf(fstr, "Active routes (active): %.2f mi\n", active_only_miles);
	hdstatsfile << fstr;
	sprintf(fstr, "Clinchable routes (active, preview): %.2f mi\n", active_preview_miles);
	hdstatsfile << fstr;
	sprintf(fstr, "All routes (active, preview, devel): %.2f mi\n", overall_miles);
	hdstatsfile << fstr;
	hdstatsfile << "Breakdown by region:\n";
	// let's sort alphabetically by region
	// a nice enhancement later here might break down by continent, then country,
	// then region
	list<string> region_entries;
	for (Region* region : Region::allregions)
	  if (region->overall_mileage)
	  {	sprintf(fstr, ": %.2f (active), %.2f (active, preview) %.2f (active, preview, devel)\n",
			region->active_only_mileage, region->active_preview_mileage, region->overall_mileage);
		region_entries.push_back(region->code + fstr);
	  }
	region_entries.sort();
	for (string& e : region_entries) hdstatsfile << e;

	for (HighwaySystem *h : HighwaySystem::syslist)
	{	sprintf(fstr, ") total: %.2f mi\n", h->total_mileage());
		hdstatsfile << "System " << h->systemname << " (" << h->level_name() << fstr;
		if (h->mileage_by_region.size() > 1)
		{	hdstatsfile << "System " << h->systemname << " by region:\n";
			list<Region*> regions_in_system;
			for (pair<Region* const, double>& rm : h->mileage_by_region)
				regions_in_system.push_back(rm.first);
			regions_in_system.sort(sort_regions_by_code);
			for (Region *r : regions_in_system)
			{	sprintf(fstr, ": %.2f mi\n", h->mileage_by_region.at(r));
				hdstatsfile << r->code << fstr;
			}
		}
		hdstatsfile << "System " << h->systemname << " by route:\n";
		for (ConnectedRoute* cr : h->con_route_list)
		{	string to_write = "";
			for (Route *r : cr->roots)
			{	sprintf(fstr, ": %.2f mi\n", r->mileage);
				to_write += "  " + r->readable_name() + fstr;
				cr->mileage += r->mileage;
			}
			sprintf(fstr, ": %.2f mi", cr->mileage);
			hdstatsfile << cr->readable_name() << fstr;
			if (cr->roots.size() == 1)
				hdstatsfile << " (" << cr->roots[0]->readable_name() << " only)\n";
			else	hdstatsfile << '\n' << to_write;
		}
	}
	hdstatsfile.close();

	// now add user clinched stats to their log entries
	cout << et.et() << "Creating per-traveler stats logs and augmenting data structure." << flush;
      #ifdef threading_enabled
	TravelerList::tl_it = TravelerList::allusers.begin();
	THREADLOOP thr[t] = thread(UserLogThread, t, &list_mtx, active_only_miles, active_preview_miles);
	THREADLOOP thr[t].join();
      #else
	for (TravelerList *t : TravelerList::allusers)
		t->userlog(active_only_miles, active_preview_miles);
      #endif
	cout << "!" << endl;

	// write stats csv files
	cout << et.et() << "Writing stats csv files." << endl;
      #ifdef threading_enabled
	HighwaySystem::it = HighwaySystem::syslist.begin();
	switch(Args::mtcsvfiles ? Args::numthreads : 1)
	{   case 1:
      #endif
		cout << et.et() << "Writing allbyregionactiveonly.csv." << endl;
		allbyregionactiveonly(0, active_only_miles);
		cout << et.et() << "Writing allbyregionactivepreview.csv." << endl;
		allbyregionactivepreview(0, active_preview_miles);
		cout << et.et() << "Writing per-system stats csv files." << endl;
		for (HighwaySystem* h : HighwaySystem::syslist) h->stats_csv();
      #ifdef threading_enabled
		break;
	   case 2:
		thr[0] = thread(allbyregionactiveonly,    &list_mtx, active_only_miles);
		thr[1] = thread(allbyregionactivepreview, &list_mtx, active_preview_miles);
		thr[0].join();
		thr[1].join();
		break;
	   default:
		thr[0] = thread(allbyregionactiveonly,    nullptr, active_only_miles);
		thr[1] = thread(allbyregionactivepreview, nullptr, active_preview_miles);
		thr[2] = thread(StatsCsvThread, 2, &list_mtx);
		thr[0].join();
		thr[1].join();
		thr[2].join();
	}
      #endif

	cout << et.et() << "Reading datacheckfps.csv." << endl;
	Datacheck::read_fps(Args::highwaydatapath, el);

      #ifdef threading_enabled
	thread sqlthread;
	if   (!Args::errorcheck)
	{	std::cout << et.et() << "Start writing database file " << Args::databasename << ".sql.\n" << std::flush;
		sqlthread=thread(sqlfile1, &et, &continents, &countries, &updates, &systemupdates, &term_mtx);
	}
      #endif

	#include "tasks/graph_generation.cpp"

	cout << et.et() << "Marking datacheck false positives." << flush;
	Datacheck::mark_fps(Args::logfilepath, et);
	cout << et.et() << "Writing log of unmatched datacheck FP entries." << endl;
	Datacheck::unmatchedfps_log(Args::logfilepath);
	cout << et.et() << "Writing datacheck.log" << endl;
	Datacheck::datacheck_log(Args::logfilepath);

	if   (Args::errorcheck)
		cout << et.et() << "SKIPPING database file." << endl;
	else {
	      #ifdef threading_enabled
		sqlthread.join();
		std::cout << et.et() << "Resume writing database file " << Args::databasename << ".sql.\n" << std::flush;
	      #else
		std::cout << et.et() << "Writing database file " << Args::databasename << ".sql.\n" << std::flush;
		sqlfile1(&et, &continents, &countries, &updates, &systemupdates, &term_mtx);
	      #endif
		sqlfile2(&et, &graph_types);
	     }

	// See if we have any errors that should be fatal to the site update process
	if (el.error_list.size())
	{	cout << et.et() << "ABORTING due to " << el.error_list.size() << " errors:" << endl;
		for (unsigned int i = 0; i < el.error_list.size(); i++)
			cout << i+1 << ": " << el.error_list[i] << endl;
		return 1;
	}

	// print some statistics
	cout << et.et() << "Processed " << HighwaySystem::syslist.size() << " highway systems." << endl;
	unsigned int routes = 0;
	unsigned int points = 0;
	unsigned int segments = 0;
	for (HighwaySystem *h : HighwaySystem::syslist)
	{	routes += h->route_list.size();
		for (Route *r : h->route_list)
		{	points += r->point_list.size();
			segments += r->segment_list.size();
		}
	}
	cout << "Processed " << routes << " routes with a total of " << points << " points and " << segments << " segments." << endl;
	if (points != all_waypoints.size())
	  cout << "MISMATCH: all_waypoints contains " << all_waypoints.size() << " waypoints!" << endl;
	cout << et.et() << "WaypointQuadtree contains " << all_waypoints.total_nodes() << " total nodes." << endl;

	vector<unsigned int> colocate_counts(2,0);
	if (!Args::errorcheck)
	{	// compute colocation of waypoints stats
		cout << et.et() << "Computing waypoint colocation stats, reporting all with 9 or more colocations:" << endl;
		all_waypoints.final_report(colocate_counts);
		cout << "Waypoint colocation counts:" << endl;
		unsigned int unique_locations = 0;
		for (unsigned int c = 1; c < colocate_counts.size(); c++)
		{	unique_locations += colocate_counts[c];
			printf("%6i are each occupied by %2i waypoints.\n", colocate_counts[c], c);
		}
		cout << "Unique locations: " << unique_locations << endl;
	} else	all_waypoints.final_report(colocate_counts);

	/* EDB
	cout << endl;
	unsigned int a_count = 0;
	unsigned int p_count = 0;
	unsigned int d_count = 0;
	unsigned other_count = 0;
	unsigned int total_rtes = 0;
	for (HighwaySystem *h : HighwaySystem::syslist)
	  for (Route* r : h->route_list)
	  {	total_rtes++;
		if (h->devel()) d_count++;
		else {	if (h->active()) a_count++;
			else if (h->preview()) p_count++;
			     else other_count++;
		     }
	  }
	cout << a_count+p_count << " clinchable routes:" << endl;
	cout << a_count << " in active systems, and" << endl;
	cout << p_count << " in preview systems." << endl;
	cout << d_count << " routes in devel systems." << endl;
	if (other_count) cout << other_count << " routes not designated active/preview/devel!" << endl;
	cout << total_rtes << " total routes." << endl;//*/

	if (Args::errorcheck)
	    cout << "!!! DATA CHECK SUCCESSFUL !!!" << endl;

	timestamp = time(0);
	cout << "Finish: " << ctime(&timestamp);
	cout << "Total run time: " << et.et() << endl;

}

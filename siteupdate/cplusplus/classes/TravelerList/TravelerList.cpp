#include "TravelerList.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/tmstring.h"
#include "../../templates/contains.cpp"
#include <dirent.h>

TravelerList::TravelerList(std::string& travname, ErrorList* el)
{	// initialize object variables
	traveler_num = new unsigned int[Args::numthreads];
		       // deleted by ~TravelerList
	traveler_num[0] = this - allusers.data; // init for master traveled graph
	traveler_name.assign(travname, 0, travname.size()-5); // strip ".list" from end of travname
	if (traveler_name.size() > DBFieldLength::traveler)
	  el->add_error("Traveler name " + traveler_name + " > " + std::to_string(DBFieldLength::traveler) + "bytes");

	// variables used in construction
	unsigned int list_entries = 0;
	std::ofstream splist;
	std::string update;
	if (Args::splitregionpath != "") splist.open(Args::splitregionpath+"/list_files/"+travname);

	// init user log
	std::ofstream log(Args::logfilepath+"/users/"+traveler_name+".log");
	time_t StartTime = time(0);
	log << "Log file created at: ";
	mtx.lock();
	log << ctime(&StartTime);
	mtx.unlock();
	// write last update date & time if known
	std::ifstream file(Args::userlistfilepath+"/../time_files/"+traveler_name+".time");
	if (file.is_open())
	{	getline(file, update);
		if (update.size())
		{	log << travname << " last updated: " << update << '\n';
			update.assign(update, 0, 10);
		}
		file.close();
	}

	// read .list file into memory
	  // we can't getline here because it only allows one delimiter, and we need two; '\r' and '\n'.
	  // at least one .list file contains newlines using only '\r' (0x0D):
	  // https://github.com/TravelMapping/UserData/blob/6309036c44102eb3325d49515b32c5eef3b3cb1e/list_files/whopperman.list
	file.open(Args::userlistfilepath+"/"+travname);
	if (!file.is_open())
	{	std::cout << "\nERROR: " << travname << " not found.";
		file_not_found = 1;
	}
	if (file_not_found)
		// We're going to abort, so no point in continuing to fully build out TravelerList objects.
		// Future constructors will proceed only this far, to get a complete list of invalid names.
		return;
	else	std::cout << traveler_name << ' ' << std::flush;
	file.seekg(0, std::ios::end);
	unsigned long listdatasize = file.tellg();
	file.seekg(0, std::ios::beg);
	char *listdata = new char[listdatasize+1];
			 // deleted after processing lines
	file.read(listdata, listdatasize);
	listdata[listdatasize] = 0; // add null terminator
	file.close();

	// get canonical newline for writing splitregion .list files
	std::string newline;
	char* c = listdata;
	while (*c != '\r' && *c != '\n' && *c) c++;
	if (*c == '\r')
		if (c[1] == '\n')	newline = "\r\n";
		else			newline = "\r";
	else	if (c[0] == '\n')	newline = "\n";
	// Use CRLF as failsafe if .list file contains no newlines.
		else			newline = "\r\n";

	// skip UTF-8 byte order mark if present
	if (strncmp(listdata, "\xEF\xBB\xBF", 3))
		c = listdata;
	else {	c = listdata+3;
		splist << "\xEF\xBB\xBF";
	     }

	// separate listdata into series of lines & newlines
	while (*c == '\r' || *c == '\n') splist << *c++; // skip leading blank lines
	std::vector<char*> lines;
	std::vector<std::string> endlines;
	for (size_t spn = 0; *c; c += spn)
	{	endlines.emplace_back();
		for (spn = strcspn(c, "\n\r"); c[spn] == '\n' || c[spn] == '\r'; spn++)
		{	endlines.back().push_back(c[spn]);
			c[spn] = 0;
		}
		lines.push_back(c);
	}
	lines.push_back(listdata+listdatasize+1);	// add a dummy "past-the-end" element to make lines[l+1]-2 work

	// process lines
	for (unsigned int l = 0; l < lines.size()-1; l++)
	{	// strip whitespace from beginning
		char *c = lines[l];
		while (*c == ' ' || *c == '\t') c++;
		char* beg = c;
		// ignore whitespace or "comment" lines
		if (*c == 0 || *c == '#')
		{	splist << lines[l] << endlines[l];
			continue;
		}
		// split line into fields
		std::vector<std::string> fields;
		for (size_t spn; *c; c += spn)
		{	if (*c == '#') break;
			spn = strcspn(c, " \t");
			fields.emplace_back(c,spn);
			while (c[spn] == ' ' || c[spn] == '\t') spn++;
		}
		// strip whitespace from end
		while ( strchr(" \t", fields.back().back()) ) fields.back().pop_back();

		// lambda for whitespace-trimmed .list line used in userlog error reports & warnings
		// calculate once, then it's available for re-use
		char* trim_line = 0;
		auto get_trim_line = [&]()
		{	if (!trim_line)
			{	char* end = lines[l+1]-2;	// -2 skips over 0 inserted while separating listdata into lines
				while (*end == 0) end--;	// skip back more for CRLF cases & lines followed by blank lines
				while (*end == ' ' || *end == '\t') end--;		  // strip whitespace @ end
				size_t size = end-beg+1;		// +1 because end points to final good char
				trim_line = (char*)malloc(size+1);	// +1 because need room for null terminator
				strncpy(trim_line, beg, size);
				trim_line[size] = 0;
			}
			return trim_line;
		};
		#define UPDATE_NOTE(R) if (R->last_update) \
		{	updated_routes.insert(R); \
			log << "  Route updated " << R->last_update[0] << ": " << R->readable_name() << '\n'; \
		}
		if (fields.size() == 4)
		     {
			#include "mark_chopped_route_segments.cpp"
		     }
		else if (fields.size() == 6)
		     {
			#include "mark_connected_route_segments.cpp"
		     }
		else {	log << "Incorrect format line (4 or 6 fields expected, found "
			    << fields.size() << "): " << get_trim_line() << '\n';
			splist << lines[l] << endlines[l];
		     }
		#undef UPDATE_NOTE
		free(trim_line);
	}
	delete[] listdata;
	log << "Processed " << list_entries << " good lines marking " << clinched_segments.size() << " segments traveled.\n";
	log.close();
	splist.close();
}

TravelerList::~TravelerList() {delete[] traveler_num;}

void TravelerList::get_ids(ErrorList& el)
{	ids = std::move(Args::userlist);
	if (ids.empty())
	{	DIR *dir;
		dirent *ent;
		if ((dir = opendir (Args::userlistfilepath.data())) != NULL)
		{	while ((ent = readdir (dir)) != NULL)
			{	std::string trav(ent->d_name);
				if (trav.size() > 5 && !strcmp(trav.data()+trav.size()-5, ".list"))
					ids.push_back(trav);
			}
			closedir(dir);
		}
		else	el.add_error("Error opening user list file path \""+Args::userlistfilepath+"\". (Not found?)");
	}
	else for (std::string& id : ids) id += ".list";
	ids.sort();
	tl_it = allusers.alloc(ids.size());
}

/* Return active mileage across all regions */
double TravelerList::active_only_miles()
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : active_only_mileage_by_region) mi += rm.second;
	return mi;
}

/* Return active+preview mileage across all regions */
double TravelerList::active_preview_miles()
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : active_preview_mileage_by_region) mi += rm.second;
	return mi;
}

/* Return mileage across all regions for a specified system */
double TravelerList::system_miles(HighwaySystem *h)
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : system_region_mileages.at(h)) mi += rm.second;
	return mi;
}

std::mutex TravelerList::mtx;
std::list<std::string> TravelerList::ids;
std::list<std::string>::iterator TravelerList::id_it;
TMArray<TravelerList> TravelerList::allusers;
TravelerList* TravelerList::tl_it;
bool TravelerList::file_not_found = 0;

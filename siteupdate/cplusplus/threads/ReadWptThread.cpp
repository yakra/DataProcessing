void ReadWptThread
(	unsigned int id, std::list<HighwaySystem*> *hs_list, std::mutex *hs_mtx,
	std::string path, ErrorList *el, std::unordered_set<std::string> *all_wpt_files,
	WaypointQuadtree *all_waypoints, std::mutex *strtok_mtx, DatacheckEntryList *datacheckerrors
)
{
      #ifdef DebugReadWpt
	printf("Starting ReadWptThread %02i\n", id);
	fflush(stdout);
      #endif
	while (hs_list->size())
	{	hs_mtx->lock();
	      #ifdef DebugReadWpt
		printf("ReadWptThread %02i obtained lock\n", id);
		fflush(stdout);
	      #endif
		if (!hs_list->size())
		{
		      #ifdef DebugReadWpt
			printf("ReadWptThread %02i: size set to 0 by other thread; returning\n", id);
			fflush(stdout);
		      #endif
			hs_mtx->unlock();
			return;
		}
	      #ifdef DebugReadWpt
		printf("ReadWptThread %02i with hs_list->size()=%li\n", id, hs_list->size());
		fflush(stdout);
	      #endif
		HighwaySystem *h(hs_list->front());
	      #ifdef DebugReadWpt
		printf("ReadWptThread %02i assigned %s\n", id, h->systemname.data());
		fflush(stdout);
	      #endif
		hs_list->pop_front();
	      #ifdef DebugReadWpt
		printf("ReadWptThread %02i hs_list->pop_front() successful.\n", id);
		fflush(stdout);
	      #endif
		hs_mtx->unlock();
	      #ifdef DebugReadWpt
		printf("ReadWptThread %02i begin HighwaySystem processing\n", id);
		fflush(stdout);
	      #else
		std::cout << h->systemname << std::flush;
	      #endif
		for (Route &r : h->route_list)
			r.read_wpt(id, all_waypoints, el, path, strtok_mtx, datacheckerrors, all_wpt_files);
	      #ifdef DebugReadWpt
		printf("ReadWptThread %02i end HighwaySystem processing\n", id);
		fflush(stdout);
	      #else
		std::cout << "!\n" << std::flush;
	      #endif
	}
      #ifdef DebugReadWpt
	printf("Ending ReadWptThread %02i\n", id);
	fflush(stdout);
      #endif
}

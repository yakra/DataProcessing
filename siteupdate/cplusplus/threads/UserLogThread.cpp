void UserLogThread(unsigned int id, std::list<TravelerList*> *tl_list, std::mutex *mtx, ClinchedDBValues *clin_db_val, const double total_active_only_miles, const double total_active_preview_miles, std::list<HighwaySystem*> *highway_systems, std::string path)
{
      #ifdef DebugUserLog
	printf("Starting UserLogThread %02i\n", id);
	fflush(stdout);
      #endif
	while (tl_list->size())
	{	mtx->lock();
	      #ifdef DebugUserLog
		printf("UserLogThread %02i obtained lock\n", id);
		fflush(stdout);
	      #endif
		if (!tl_list->size())
		{
		      #ifdef DebugUserLog
			printf("UserLogThread %02i: size set to 0 by other thread; returning\n", id);
			fflush(stdout);
		      #endif
			mtx->unlock();
			return;
		}
	      #ifdef DebugUserLog
		printf("UserLogThread %02i with tl_list->size()=%li\n", id, tl_list->size());
		fflush(stdout);
	      #endif
		TravelerList *t(tl_list->front());
	      #ifdef DebugUserLog
		printf("UserLogThread %02i assigned %s\n", id, t->traveler_name.data());
		fflush(stdout);
	      #endif
		tl_list->pop_front();
	      #ifdef DebugUserLog
		printf("UserLogThread %02i tl_list->pop_front() successful.\n", id);
		fflush(stdout);
	      #endif
		mtx->unlock();
	      #ifdef DebugUserLog
		printf("UserLogThread %02i begin TravelerList processing\n", id);
		fflush(stdout);
	      #endif
		t->userlog(id, clin_db_val, total_active_only_miles, total_active_preview_miles, highway_systems, path);
	      #ifdef DebugUserLog
		printf("UserLogThread %02i end TravelerList processing\n", id);
		fflush(stdout);
	      #endif
	}
      #ifdef DebugUserLog
	printf("Ending UserLogThread %02i\n", id);
	fflush(stdout);
      #endif
}

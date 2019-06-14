void ReadListThread(unsigned int id, std::list<std::string> *traveler_ids, std::list<TravelerList*> *traveler_lists, std::mutex *tl_mtx, std::mutex *strtok_mtx, Arguments *args, std::unordered_map<std::string, Route*> *route_hash)
{
      #ifdef DebugReadList
	printf("Starting ReadListThread %02i\n", id);
	fflush(stdout);
      #endif
	while (traveler_ids->size())
	{	tl_mtx->lock();
	      #ifdef DebugReadList
		printf("ReadListThread %02i obtained lock\n", id);
		fflush(stdout);
	      #endif
		if (!traveler_ids->size())
		{
		      #ifdef DebugReadList
			printf("ReadListThread %02i: size set to 0 by other thread; returning\n", id);
			fflush(stdout);
		      #endif
			tl_mtx->unlock();
			return;
		}
	      #ifdef DebugReadList
		printf("ReadListThread %02i with traveler_ids->size()=%li\n", id, traveler_ids->size());
		fflush(stdout);
	      #endif
		std::string tl(traveler_ids->front());
	      #ifdef DebugReadList
		printf("ReadListThread %02i assigned %s\n", id, tl.data());
		fflush(stdout);
	      #endif
		traveler_ids->pop_front();
	      #ifdef DebugReadList
		printf("ReadListThread %02i traveler_ids->pop_front() successful.\n", id);
		fflush(stdout);
	      #else
		std::cout << ' ' << tl << std::flush;
	      #endif
		tl_mtx->unlock();
	      #ifdef DebugReadList
		printf("ReadListThread %02i begin list processing\n", id);
		fflush(stdout);
	      #endif
		TravelerList *t = new TravelerList(tl, route_hash, args, strtok_mtx);
				  // deleted on termination of program
	      #ifdef DebugReadList
		printf("ReadListThread %02i end list processing. Waiting for alltrav_mtx.\n", id);
		fflush(stdout);
	      #endif
		TravelerList::alltrav_mtx.lock();
	      #ifdef DebugReadList
		printf("ReadListThread %02i alltrav_mtx locked; ", id);
		fflush(stdout);
	      #endif
		traveler_lists->push_back(t);
	      #ifdef DebugReadList
		printf("%s added to list of lists.\n", t->traveler_name.data());
		fflush(stdout);
	      #endif
		TravelerList::alltrav_mtx.unlock();
	      #ifdef DebugReadList
		printf("ReadListThread %02i alltrav_mtx unlocked.\n", id);
		fflush(stdout);
	      #endif
	}
      #ifdef DebugReadList
	printf("Ending ReadListThread %02i\n", id);
	fflush(stdout);
      #endif
}

void ConcAugThread(unsigned int id, std::list<TravelerList*> *travlists, std::mutex *tl_mtx, std::mutex *log_mtx, std::ofstream *concurrencyfile)
{
      #ifdef DebugConcAug
	printf("Starting ConcAugThread %02i\n", id);
	fflush(stdout);
      #endif
	while (travlists->size())
	{	tl_mtx->lock();
	      #ifdef DebugConcAug
		printf("ConcAugThread %02i obtained lock\n", id);
		fflush(stdout);
	      #endif
		if (!travlists->size())
		{
		      #ifdef DebugConcAug
			printf("ConcAugThread %02i: size set to 0 by other thread; returning\n", id);
			fflush(stdout);
		      #endif
			tl_mtx->unlock();
			return;
		}
	      #ifdef DebugConcAug
		printf("ConcAugThread %02i with travlists.size()=%li\n", id, travlists->size());
		fflush(stdout);
	      #endif
		TravelerList *t(travlists->front());
	      #ifdef DebugConcAug
		printf("ConcAugThread %02i assigned %s\n", id, t->traveler_name.data());
		fflush(stdout);
	      #endif
		travlists->pop_front();
	      #ifdef DebugConcAug
		printf("ConcAugThread %02i travlists->pop_front() successful.\n", id);
		fflush(stdout);
	      #endif
		tl_mtx->unlock();
	      #ifdef DebugConcAug
		printf("ConcAugThread %02i begin traveler processing\n", id);
		fflush(stdout);
	      #else
		std::cout << '.' << std::flush;
	      #endif
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs->route->system->active_or_preview() && hs->add_clinched_by(t))
		      {	log_mtx->lock();
			*concurrencyfile << "Concurrency augment for traveler " << t->traveler_name << ": [" << hs->str() << "] based on [" << s->str() << "]\n";
			log_mtx->unlock();
		      }
	      #ifdef DebugConcAug
		printf("ConcAugThread %02i end traveler processing\n", id);
		fflush(stdout);
	      #endif
	}
      #ifdef DebugConcAug
	printf("Ending ConcAugThread %02i\n", id);
	fflush(stdout);
      #endif
}

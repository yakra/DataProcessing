void ComputeStatsThread(unsigned int id, std::list<HighwaySystem*> *hs_list, std::mutex *mtx)
{
      #ifdef DebugComputeStats
	printf("Starting ComputeStatsThread %02i\n", id);
	fflush(stdout);
      #endif
	while (hs_list->size())
	{	mtx->lock();
	      #ifdef DebugComputeStats
		printf("ComputeStatsThread %02i obtained lock\n", id);
		fflush(stdout);
	      #endif
		if (!hs_list->size())
		{
		      #ifdef DebugComputeStats
			printf("ComputeStatsThread %02i: size set to 0 by other thread; returning\n", id);
			fflush(stdout);
		      #endif
			mtx->unlock();
			return;
		}
	      #ifdef DebugComputeStats
		printf("ComputeStatsThread %02i with hs_list->size()%li\n", id, hs_list->size());
		fflush(stdout);
	      #endif
		HighwaySystem *h(hs_list->front());
	      #ifdef DebugComputeStats
		printf("ComputeStatsThread %02i assigned %s\n", id, h->systemname.data());
		fflush(stdout);
	      #endif
		hs_list->pop_front();
	      #ifdef DebugComputeStats
		printf("ComputeStatsThread %02i hs_list->pop_front() successful. Releasing lock.\n", id);
		fflush(stdout);
	      #endif
		mtx->unlock();
	      #ifdef DebugComputeStats
		printf("ComputeStatsThread %02i begin segment processing\n", id);
		fflush(stdout);
	      #else
		std::cout << '.' << std::flush;
	      #endif
		for (Route &r : h->route_list)
		  for (HighwaySegment *s : r.segment_list)
		    s->compute_stats(id);
	      #ifdef DebugComputeStats
		printf("ComputeStatsThread %02i end segment processing\n", id);
		fflush(stdout);
	      #endif
	}
      #ifdef DebugComputeStats
	printf("Ending ComputeStatsThread %02i\n", id);
	fflush(stdout);
      #endif
}

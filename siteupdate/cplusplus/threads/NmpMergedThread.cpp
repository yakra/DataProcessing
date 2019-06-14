void NmpMergedThread(unsigned int id, std::list<HighwaySystem*> *hs_list, std::mutex *mtx, std::string *nmpmergepath)
{
      #ifdef DebugNmpMerged
	printf("Starting NMPMergedThread %02i\n", id);
	fflush(stdout);
      #endif
	while (hs_list->size())
	{	mtx->lock();
	      #ifdef DebugNmpMerged
		printf("NmpMergedThread %02i obtained lock\n", id);
		fflush(stdout);
	      #endif
		if (!hs_list->size())
		{
		      #ifdef DebugReadWpt
			printf("NmpMergedThread %02i: size set to 0 by other thread; returning\n", id);
			fflush(stdout);
		      #endif
			mtx->unlock();
			return;
		}
	      #ifdef DebugNmpMerged
		printf("NmpMergedThread %02i with hs_list->size()=%li\n", id, hs_list->size());
		fflush(stdout);
	      #endif
		HighwaySystem *h(hs_list->front());
	      #ifdef DebugNmpMerged
		printf("NmpMergedThread %02i assigned %s\n", id, h->systemname.data());
		fflush(stdout);
	      #endif
		hs_list->pop_front();
	      #ifdef DebugNmpMerged
		printf("NmpMergedThread %02i hs_list->pop_front() successful. Releasing lock.\n", id);
		fflush(stdout);
	      #endif
		mtx->unlock();
	      #ifdef DebugNmpMerged
		printf("NmpMergedThread %02i begin route processing\n", id);
		fflush(stdout);
	      #endif
		for (Route &r : h->route_list)
			r.write_nmp_merged(*nmpmergepath + "/" + r.region->code);
	      #ifdef DebugNmpMerged
		printf("NmpMergedThread %02i end route processing\n", id);
		fflush(stdout);
	      #endif
	}
      #ifdef DebugNmpMerged
	printf("Ending NMPMergedThread %02i\n", id);
	fflush(stdout);
      #endif
}

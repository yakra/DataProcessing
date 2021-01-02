void StatsCsvThread
(	unsigned int id, std::list<HighwaySystem*>* hs_list, std::list<HighwaySystem*>::iterator* it,
	std::mutex* hs_mtx, std::string* path, std::list<TravelerList*>* traveler_lists
)
{	//printf("Starting StatsCsvThread %02i\n", id); fflush(stdout);
	while (*it != hs_list->end())
	{	hs_mtx->lock();
		if (*it == hs_list->end())
		{	hs_mtx->unlock();
			return;
		}
		HighwaySystem *h(**it);
		//printf("StatsCsvThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		(*it)++;
		//printf("StatsCsvThread %02i (*it)++\n", id); fflush(stdout);
		hs_mtx->unlock();
		h->stats_csv(*path, *traveler_lists);
	}
}

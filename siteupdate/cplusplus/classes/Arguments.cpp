class Arguments
{	public:
	/* w */ std::string highwaydatapath;
	/* s */ std::string systemsfile;
	/* u */ std::string userlistfilepath;
	/* d */ std::string databasename;
	/* l */ std::string logfilepath;
	/* c */ std::string csvstatfilepath;
	/* g */ std::string graphfilepath;
	/* k */ bool skipgraphs;
	/* n */ std::string nmpmergepath;
	/* p */ std::string splitregion, splitregionpath;
	/* U */ std::list<std::string> userlist;
	/* t */ int numthreads;
		int ReadWptThreads;
		int NmpSearchThreads;
		int NmpMergedThreads;
		int ReadListThreads;
		int ConcAugThreads;
		int CompStatsThreads;
		int UserLogThreads;
		int GraphThreads;
	/* e */ bool errorcheck;
	/* h */ bool help;

	Arguments(int argc, char *argv[])
	{	// defaults
		/* w */ highwaydatapath = "../../../HighwayData";
		/* s */ systemsfile = "systems.csv";
		/* u */ userlistfilepath = "../../../UserData/list_files";
		/* d */ databasename = "TravelMapping";
		/* l */ logfilepath = ".";
		/* c */ csvstatfilepath = ".";
		/* g */ graphfilepath = ".";
		/* k */ skipgraphs = 0;
		/* n */ nmpmergepath = "";
		/* p */ splitregionpath = "";
		/* U */ // nothing to do here
		/* t */ numthreads = 4;
			ReadWptThreads = 0;
			NmpSearchThreads = 0;
			NmpMergedThreads = 0;
			ReadListThreads = 0;
			ConcAugThreads = 0;
			CompStatsThreads = 0;
			UserLogThreads = 0;
			GraphThreads = 0;
		/* e */ errorcheck = 0;
		/* h */ help = 0;

		// parsing
		for (unsigned int n = 1; n < argc; n++)
		{	     if ( n+1 < argc && !strcmp(argv[n], "-w") || !strcmp(argv[n], "--highwaydatapath") ) {
				highwaydatapath = argv[n+1]; n++; }
			else if ( n+1 < argc && !strcmp(argv[n], "-s") || !strcmp(argv[n], "--systemsfile") ) {
				systemsfile = argv[n+1]; n++; }
			else if ( n+1 < argc && !strcmp(argv[n], "-u") || !strcmp(argv[n], "--userlistfilepath") ) {
				userlistfilepath = argv[n+1]; n++; }
			else if ( n+1 < argc && !strcmp(argv[n], "-d") || !strcmp(argv[n], "--databasename") ) {
				databasename = argv[n+1]; n++; }
			else if ( n+1 < argc && !strcmp(argv[n], "-l") || !strcmp(argv[n], "--logfilepath") ) {
				logfilepath = argv[n+1]; n++; }
			else if ( n+1 < argc && !strcmp(argv[n], "-c") || !strcmp(argv[n], "--csvstatfilepath") ) {
				csvstatfilepath = argv[n+1]; n++; }
			else if ( n+1 < argc && !strcmp(argv[n], "-g") || !strcmp(argv[n], "--graphfilepath") ) {
				graphfilepath = argv[n+1]; n++; }
			else if (               !strcmp(argv[n], "-k") || !strcmp(argv[n], "--skipgraphs") )
				skipgraphs = 1;
			else if ( n+1 < argc && !strcmp(argv[n], "-n") || !strcmp(argv[n], "--nmpmergepath") ) {
				nmpmergepath = argv[n+1]; n++; }
			else if ( n+2 < argc && !strcmp(argv[n], "-p") || !strcmp(argv[n], "--splitregion") ) {
				splitregionpath = argv[n+1]; splitregion = argv[n+2]; n +=2; }
			else if ( n+1 < argc && !strcmp(argv[n], "-t") || !strcmp(argv[n], "--numthreads") ) {
				numthreads = strtol(argv[n+1], 0, 10); n++; if (numthreads<1) numthreads=1; }
			else if (               !strcmp(argv[n], "-e") || !strcmp(argv[n], "--errorcheck") )
				errorcheck = 1;
			else if (               !strcmp(argv[n], "-h") || !strcmp(argv[n], "--help") ) {
				help = 1; show_help(); }
			else if ( n+1 < argc && !strcmp(argv[n], "-U") || !strcmp(argv[n], "--userlist") )
				while (n+1 < argc && argv[n+1][0] != '-')
				{	userlist.push_back(argv[n+1]);
					n++;
				}
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--ReadWptThreads") ) {
				ReadWptThreads   = strtol(argv[n+1], 0, 10); n++; if (ReadWptThreads<1) ReadWptThreads=1; }
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--NmpSearchThreads") ) {
				NmpSearchThreads = strtol(argv[n+1], 0, 10); n++; if (NmpSearchThreads<1) NmpSearchThreads=1; }
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--NmpMergedThreads") ) {
				NmpMergedThreads = strtol(argv[n+1], 0, 10); n++; if (NmpMergedThreads<1) NmpMergedThreads=1; }
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--ReadListThreads") ) {
				ReadListThreads  = strtol(argv[n+1], 0, 10); n++; if (ReadListThreads<1) ReadListThreads=1; }
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--ConcAugThreads") ) {
				ConcAugThreads   = strtol(argv[n+1], 0, 10); n++; if (ConcAugThreads<1) ConcAugThreads=1; }
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--CompStatsThreads") ) {
				CompStatsThreads = strtol(argv[n+1], 0, 10); n++; if (CompStatsThreads<1) CompStatsThreads=1; }
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--UserLogThreads") ) {
				UserLogThreads   = strtol(argv[n+1], 0, 10); n++; if (UserLogThreads<1) UserLogThreads=1; }
			else if ( n+1 < argc  &&  !strcmp(argv[n], "--GraphThreads") ) {
				GraphThreads  = strtol(argv[n+1], 0, 10); n++; if (GraphThreads<1) GraphThreads=1; }
		} // end parsing
		if (!ReadWptThreads)	ReadWptThreads   = numthreads;
		if (!NmpSearchThreads)	NmpSearchThreads = numthreads;
		if (!NmpMergedThreads)	NmpMergedThreads = numthreads;
		if (!ReadListThreads)	ReadListThreads  = numthreads;
		if (!ConcAugThreads)	ConcAugThreads   = numthreads;
		if (!CompStatsThreads)	CompStatsThreads = numthreads;
		if (!UserLogThreads)	UserLogThreads   = numthreads;
		if (!GraphThreads)	GraphThreads     = numthreads;
	} // end ctor

	void show_help()
	{	std::cout <<	"usage: siteupdate.py [-h] [-w HIGHWAYDATAPATH] [-s SYSTEMSFILE]\n";
		std::cout <<	"                     [-u USERLISTFILEPATH] [-d DATABASENAME] [-l LOGFILEPATH]\n";
		std::cout <<	"                     [-c CSVSTATFILEPATH] [-g GRAPHFILEPATH] [-k]\n";
		std::cout <<	"                     [-n NMPMERGEPATH] [-p SPLITREGIONPATH SPLITREGION]\n";
		std::cout <<	"                     [-U USERLIST [USERLIST ...]] [-t NUMTHREADS] [-e]\n";
		std::cout <<	"\n";
		std::cout <<	"Create SQL, stats, graphs, and log files from highway and user data for the\n";
		std::cout <<	"Travel Mapping project.\n";
		std::cout <<	"\n";
		std::cout <<	"optional arguments:\n";
		std::cout <<	"  -h, --help            show this help message and exit\n";
		std::cout <<	"  -w HIGHWAYDATAPATH, --highwaydatapath HIGHWAYDATAPATH\n";
		std::cout <<	"                        path to the root of the highway data directory\n";
		std::cout <<	"                        structure\n";
		std::cout <<	"  -s SYSTEMSFILE, --systemsfile SYSTEMSFILE\n";
		std::cout <<	"                        file of highway systems to include\n";
		std::cout <<	"  -u USERLISTFILEPATH, --userlistfilepath USERLISTFILEPATH\n";
		std::cout <<	"                        path to the user list file data\n";
		std::cout <<	"  -d DATABASENAME, --databasename DATABASENAME\n";
		std::cout <<	"                        Database name for .sql file name\n";
		std::cout <<	"  -l LOGFILEPATH, --logfilepath LOGFILEPATH\n";
		std::cout <<	"                        Path to write log files, which should have a \"users\"\n";
		std::cout <<	"                        subdirectory\n";
		std::cout <<	"  -c CSVSTATFILEPATH, --csvstatfilepath CSVSTATFILEPATH\n";
		std::cout <<	"                        Path to write csv statistics files\n";
		std::cout <<	"  -g GRAPHFILEPATH, --graphfilepath GRAPHFILEPATH\n";
		std::cout <<	"                        Path to write graph format data files\n";
		std::cout <<	"  -k, --skipgraphs      Turn off generation of graph files\n";
		std::cout <<	"  -n NMPMERGEPATH, --nmpmergepath NMPMERGEPATH\n";
		std::cout <<	"                        Path to write data with NMPs merged (generated only if\n";
		std::cout <<	"                        specified)\n";
		std::cout <<	"  -p SPLITREGIONPATH SPLITREGION, --splitregion SPLITREGIONPATH SPLITREGION\n";
		std::cout <<	"                        Path to logs & .lists for a specific...\n";
		std::cout <<	"                        Region being split into subregions.\n";
		std::cout <<	"                        For Development.\n";
		std::cout <<	"  -U USERLIST [USERLIST ...], --userlist USERLIST [USERLIST ...]\n";
		std::cout <<	"                        For Development: list of users to use in dataset\n";
		std::cout <<	"  -t NUMTHREADS, --numthreads NUMTHREADS\n";
		std::cout <<	"                        Number of threads to use for concurrent tasks\n";
		std::cout <<	"  -e, --errorcheck      Run only the subset of the process needed to verify\n";
		std::cout <<	"                        highway data changes\n";
		std::cout <<	"  --ReadWptThreads READWPTTHREADS\n";
		std::cout <<	"                        Number of threads for reading WPT files\n";
		std::cout <<	"  --NmpSearchThreads NMPSEARCHTHREADS\n";
		std::cout <<	"                        Number of threads for detecting near-miss points\n";
		std::cout <<	"  --NmpMergedThreads NMPMERGEDTHREADS\n";
		std::cout <<	"                        Number of threads for writing near-miss point merged wpt files\n";
		std::cout <<	"  --ReadListThreads READLISTTHREADS\n";
		std::cout <<	"                        Number of threads for processing traveler list files\n";
		std::cout <<	"  --ConcAugThreads CONCAUGTHREADS\n";
		std::cout <<	"                        Number of threads for augmenting travelers for detected concurrent segments\n";
		std::cout <<	"  --CompStatsThreads COMPSTATSTHREADS\n";
		std::cout <<	"                        Number of threads for computing stats\n";
		std::cout <<	"  --UserLogThreads USERLOGTHREADS\n";
		std::cout <<	"                        Number of threads for creating per-traveler stats logs and augmenting data structure\n";
		std::cout <<	"  --GraphThreads GRAPHTHREADS\n";
		std::cout <<	"                        Number of threads for writing graphs\n";
	}
};

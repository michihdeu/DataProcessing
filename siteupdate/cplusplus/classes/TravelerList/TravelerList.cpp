TravelerList::TravelerList(std::string travname, ErrorList *el, Arguments *args)
{	active_systems_traveled = 0;
	active_systems_clinched = 0;
	preview_systems_traveled = 0;
	preview_systems_clinched = 0;
	unsigned int list_entries = 0;
	traveler_num = new unsigned int[args->numthreads];
		       // deleted on termination of program
	traveler_name = travname.substr(0, travname.size()-5); // strip ".list" from end of travname
	if (traveler_name.size() > DBFieldLength::traveler)
	  el->add_error("Traveler name " + traveler_name + " > " + std::to_string(DBFieldLength::traveler) + "bytes");
	std::ofstream log(args->logfilepath+"/users/"+traveler_name+".log");
	std::ofstream splist;
	if (args->splitregionpath != "") splist.open(args->splitregionpath+"/list_files/"+travname);
	time_t StartTime = time(0);
	log << "Log file created at: " << ctime(&StartTime);
	std::vector<char*> lines;
	std::vector<std::string> endlines;
	std::ifstream file(args->userlistfilepath+"/"+travname);
	// we can't getline here because it only allows one delimiter, and we need two; '\r' and '\n'.
	// at least one .list file contains newlines using only '\r' (0x0D):
	// https://github.com/TravelMapping/UserData/blob/6309036c44102eb3325d49515b32c5eef3b3cb1e/list_files/whopperman.list
	file.seekg(0, std::ios::end);
	unsigned long listdatasize = file.tellg();
	file.seekg(0, std::ios::beg);
	char *listdata = new char[listdatasize+1];
	file.read(listdata, listdatasize);
	listdata[listdatasize] = 0; // add null terminator
	file.close();

	// get canonical newline for writing splitregion .list files
	std::string newline;
	unsigned long c = 0;
	while (listdata[c] != '\r' && listdata[c] != '\n' && c < listdatasize) c++;
	if (listdata[c] == '\r')
		if (listdata[c+1] == '\n')	newline = "\r\n";
		else				newline = "\r";
	else	if (listdata[c] == '\n')	newline = "\n";
	// Use CRLF as failsafe if .list file contains no newlines.
		else				newline = "\r\n";

	// separate listdata into series of lines & newlines
	size_t spn = 0;
	for (char *c = listdata; *c; c += spn)
	{	endlines.push_back("");
		for (spn = strcspn(c, "\n\r"); c[spn] == '\n' || c[spn] == '\r'; spn++)
		{	endlines.back().push_back(c[spn]);
			c[spn] = 0;
		}
		lines.push_back(c);
	}
	lines.push_back(listdata+listdatasize+1); // add a dummy "past-the-end" element to make lines[l+1]-2 work
	// strip UTF-8 byte order mark if present
	if (!strncmp(lines[0], "\xEF\xBB\xBF", 3))
	{	lines[0] += 3;
		splist << "\xEF\xBB\xBF";
	}

	for (unsigned int l = 0; l < lines.size()-1; l++)
	{	std::string orig_line(lines[l]);
		// strip whitespace
		while (lines[l][0] == ' ' || lines[l][0] == '\t') lines[l]++;
		char * endchar = lines[l+1]-2; // -2 skips over the 0 inserted while separating listdata into lines
		while (*endchar == 0) endchar--;  // skip back more for CRLF cases, and lines followed by blank lines
		while (*endchar == ' ' || *endchar == '\t')
		{	*endchar = 0;
			endchar--;
		}
		std::string trim_line(lines[l]);
		// ignore empty or "comment" lines
		if (lines[l][0] == 0 || lines[l][0] == '#')
		{	splist << orig_line << endlines[l];
			continue;
		}
		// process fields in line
		std::vector<char*> fields;
		size_t spn = 0;
		for (char* c = lines[l]; *c; c += spn)
		{	for (spn = strcspn(c, " \t"); c[spn] == ' ' || c[spn] == '\t'; spn++) c[spn] = 0;
			if (*c == '#') break;
			else fields.push_back(c);
		}
		if (fields.size() == 4)
		     {
			#include "mark_chopped_route_segments.cpp"
		     }
		else {	log << "Incorrect format line: " << trim_line << '\n';
			splist << orig_line << endlines[l];
		     }
	}
	delete[] listdata;
	log << "Processed " << list_entries << " good lines marking " << clinched_segments.size() << " segments traveled.\n";
	log.close();
	splist.close();
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
double TravelerList::system_region_miles(HighwaySystem *h)
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : system_region_mileages.at(h)) mi += rm.second;
	return mi;
}

#include "userlog.cpp"

std::mutex TravelerList::alltrav_mtx;

bool sort_travelers_by_name(const TravelerList *t1, const TravelerList *t2)
{	return t1->traveler_name < t2->traveler_name;
}

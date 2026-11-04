#include "TDCMapping.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

static std::string trim(const std::string &s){
	size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
	return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

bool TDCMapping::Load(const std::string &path){
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mTDCMapping: cannot open " << path << "\033[0m\n";
		return false;
	}
	fName.assign(V1190::kChannels, "");
	TDCData probe;
	std::string line;
	int n = 0;
	while (std::getline(fin, line)){
		std::string t = trim(line);
		if (t.empty() || t[0] < '0' || t[0] > '9'){
			continue;			// comments and the header
		}
		size_t comma = t.find(',');
		int ch = std::atoi(t.substr(0, comma).c_str());
		std::string name = (comma == std::string::npos) ? "" : trim(t.substr(comma + 1));
		if (name.empty() || name.compare(0, 4, "csi_") == 0 || ch < 0 || ch >= V1190::kChannels){
			continue;
		}
		if (!probe.Set(name, 0)){
			std::cerr << "\033[1;33mTDCMapping: '" << name << "' (channel " << ch
			          << ") is not a TDCData member - add it to include/TDCData.h\033[0m\n";
			continue;
		}
		fName[ch] = name;
		n++;
	}
	std::cout << "TDCMapping: " << n << " named channels from " << path << "\n";
	return n > 0;
}

void TDCMapping::Fill(const V1190 &tdc, TDCData &d) const{
	d.Clear();
	for (size_t i = 0; i < tdc.hits.size(); i++){
		const V1190Hit &h = tdc.hits[i];
		if (h.trailing || h.channel < 0 || h.channel >= (int)fName.size() || fName[h.channel].empty()){
			continue;
		}
		if (d.fChannels.count(fName[h.channel]) == 0){
			d.Set(fName[h.channel], h.time);	// first leading edge only
		}
	}
}

#include "PixieMapping.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

int PixieMapping::Index(int module, int channel){
	if (module < 0 || module >= kModules || channel < 0 || channel >= kChannels){
		return -1;
	}
	return module * kChannels + channel;
}

bool PixieMapping::Load(const Config &cfg){
	firstSlot = cfg.GetInt("geometry", "FIRST_SLOT", 2);
	std::string path = cfg.GetString("geometry", "CHANNEL_MAP");
	if (path.empty()){
		std::cerr << "\033[1;31mPixieMapping: no [geometry] CHANNEL_MAP in " << cfg.File() << "\033[0m\n";
		return false;
	}
	return Load(path);
}

bool PixieMapping::Load(const std::string &path){
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mPixieMapping: cannot open " << path << "\033[0m\n";
		return false;
	}
	std::fill(fTable.begin(), fTable.end(), Entry());
	fN = 0;
	int count[4] = {0, 0, 0, 0};
	std::string line;
	while (std::getline(fin, line)){
		std::istringstream ss(line);
		int module, channel, bar;
		std::string wall, side;
		if (!(ss >> module >> channel >> wall >> bar >> side)){
			continue;			// header, comments, blank lines
		}
		int idx = Index(module, channel);
		if (idx < 0){
			std::cerr << "\033[1;33mPixieMapping: module/channel out of range: '" << line << "'\033[0m\n";
			continue;
		}
		Entry e;
		char w = (char)std::toupper((unsigned char)wall[0]);
		char s = (char)std::toupper((unsigned char)side[0]);
		e.detector = (w == 'B') ? PixieData::kNWB
		           : (w == 'A') ? PixieData::kNWA
		           : (w == 'V') ? PixieData::kVETO : PixieData::kUnmapped;
		e.side = (s == 'L' || s == 'T') ? 0
		       : (s == 'R' || s == 'B') ? 1 : -1;
		e.bar  = bar;
		if (e.detector == PixieData::kUnmapped || e.side < 0){
			std::cerr << "\033[1;33mPixieMapping: unknown wall/side in '" << line << "'\033[0m\n";
			continue;
		}
		if (fTable[idx].detector != PixieData::kUnmapped){
			std::cerr << "\033[1;33mPixieMapping: module " << module << " channel " << channel
			          << " listed twice (last one wins)\033[0m\n";
			count[fTable[idx].detector]--;
			fN--;
		}
		fTable[idx] = e;
		count[e.detector]++;
		fN++;
	}
	std::cout << "PixieMapping: " << fN << " channels (NWB " << count[PixieData::kNWB]
	          << ", NWA " << count[PixieData::kNWA] << ", VETO " << count[PixieData::kVETO]
	          << ") from " << path << ", module 0 = slot " << firstSlot << "\n";
	return fN > 0;
}

void PixieMapping::Split(PixieData &d, PixieData &lana, PixieData &veto){
	lana.Clear();
	veto.Clear();
	for (int i = 0; i < d.fMult; i++){
		int module = d.fSlot[i] - firstSlot;
		int idx = Index(module, d.fChannel[i]);
		const Entry e = (idx < 0) ? Entry() : fTable[idx];
		d.fModule[i]   = module;
		d.fDetector[i] = e.detector;
		d.fBar[i]      = e.bar;
		d.fSide[i]     = e.side;
		if (e.detector == PixieData::kNWB || e.detector == PixieData::kNWA){
			lana.AppendHit(d, i);
		}
		else if (e.detector == PixieData::kVETO){
			veto.AppendHit(d, i);
		}
		else {
			fUnmappedHits++;
			fUnmappedBySlotCh[d.fSlot[i] * 100 + d.fChannel[i]]++;
		}
	}
}

void PixieMapping::PrintSummary() const{
	if (fUnmappedHits == 0){
		return;
	}
	std::cout << "\033[1;33mPixieMapping: " << fUnmappedHits
	          << " hits on channels not in the map (not written):\033[0m\n";
	for (std::map<int, long>::const_iterator it = fUnmappedBySlotCh.begin(); it != fUnmappedBySlotCh.end(); ++it){
		std::cout << "  slot " << it->first / 100 << " (module " << it->first / 100 - firstSlot
		          << ") channel " << it->first % 100 << ": " << it->second << "\n";
	}
}

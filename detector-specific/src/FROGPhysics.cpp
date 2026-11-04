#include "FROGPhysics.h"

#include <cstdlib>
#include <iostream>

bool FROGPhysics::Load(const Config &cfg){
	fZeroSuppress = cfg.GetInt   ("physics", "ZERO_SUPPRESS", 1) != 0;
	fHGThreshold  = cfg.GetDouble("physics", "HG_THRESHOLD",  0);
	fToTCut       = cfg.GetDouble("physics", "TOT_CUT",       10);
	fLGMin        = cfg.GetDouble("physics", "LG_MIN",        0);
	fHGMin        = cfg.GetDouble("physics", "HG_MIN",        0);
	fDropFailed   = cfg.GetInt   ("physics", "DROP_FAILED",   0) != 0;

	std::vector<std::vector<std::string>> per = cfg.GetAll("physics", "HG_THRESHOLD_CHANNEL");
	for (size_t i = 0; i < per.size(); i++){
		if (per[i].size() < 3){
			std::cerr << "FROGPhysics: HG_THRESHOLD_CHANNEL needs board channel threshold\n";
			continue;
		}
		fHGThresholdChannel[std::make_pair(std::atoi(per[i][0].c_str()), std::atoi(per[i][1].c_str()))] =
			std::atof(per[i][2].c_str());
	}
	std::cout << "FROGPhysics: zero suppression " << (fZeroSuppress ? "ON" : "OFF")
	          << ", HG threshold " << fHGThreshold << " (+" << fHGThresholdChannel.size() << " per channel)"
	          << ", ToT cut " << fToTCut << ", LG/HG min " << fLGMin << "/" << fHGMin
	          << ", failing hits " << (fDropFailed ? "dropped" : "flagged") << "\n";
	return true;
}

double FROGPhysics::HGThreshold(int board, int channel) const{
	std::map<std::pair<int, int>, double>::const_iterator it = fHGThresholdChannel.find(std::make_pair(board, channel));
	return (it != fHGThresholdChannel.end()) ? it->second : fHGThreshold;
}

void FROGPhysics::Apply(FROGData &d) const{
	// 1. zero suppression: keep channels with timing, or HG above threshold
	if (fZeroSuppress){
		std::vector<bool> keep(d.fMult);
		for (int i = 0; i < d.fMult; i++){
			bool timing = (d.fToA[i] > -10000) || (d.fToT[i] > -10000);
			double thr  = HGThreshold(d.fBoard[i], d.fChannel[i]);
			keep[i] = timing || (thr > 0 && d.fHG[i] >= thr);
		}
		d.Keep(keep);
	}

	// 2. cuts -> fPass
	d.fNPass = 0;
	for (int i = 0; i < d.fMult; i++){
		bool pass = true;
		if (fToTCut >= 0 && d.fToT[i] < fToTCut) pass = false;
		if (fLGMin  >  0 && d.fLG[i]  < fLGMin)  pass = false;
		if (fHGMin  >  0 && d.fHG[i]  < fHGMin)  pass = false;
		d.fPass[i] = pass ? 1 : 0;
		d.fNPass  += d.fPass[i];
	}

	// 3. optionally drop the hits that failed
	if (fDropFailed){
		std::vector<bool> keep(d.fMult);
		for (int i = 0; i < d.fMult; i++){
			keep[i] = d.fPass[i] != 0;
		}
		d.Keep(keep);
	}
}

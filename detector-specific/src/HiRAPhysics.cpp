#include "HiRAPhysics.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

static const double kNone = -10000;

// "TELE10STREF13" -> telescope 10, rest "STREF13"
static bool splitTele(const std::string &tok, int &tele, std::string &rest){
	if (tok.compare(0, 4, "TELE") != 0){
		return false;
	}
	size_t i = 4;
	while (i < tok.size() && isdigit((unsigned char)tok[i])) i++;
	if (i == 4){
		return false;
	}
	tele = std::atoi(tok.substr(4, i - 4).c_str());
	rest = tok.substr(i);
	return tele >= 0 && tele < HiRAPhysics::kTelescopes;
}

bool HiRAPhysics::Load(const Config &cfg){
	fEFRawThresh  = cfg.GetDouble("physics", "SI_EF_E_RAW_THRESHOLD", 0);
	fEBRawThresh  = cfg.GetDouble("physics", "SI_EB_E_RAW_THRESHOLD", 0);
	fEFThresh     = cfg.GetDouble("physics", "SI_EF_E_THRESHOLD", 0);
	fEBThresh     = cfg.GetDouble("physics", "SI_EB_E_THRESHOLD", 0);
	fCsIRawThresh = cfg.GetDouble("physics", "CSI_E_RAW_THRESHOLD", 0);
	fMatchSigma   = cfg.GetDouble("physics", "STRIP_ENERGY_MATCHING_SIGMA", 0.020);
	fMatchNSigma  = cfg.GetDouble("physics", "STRIP_ENERGY_MATCHING_NUMBER_OF_SIGMA", 5);
	fMaxStripMult = cfg.GetInt   ("physics", "MAX_STRIP_MULTIPLICITY", 8);
	fMaxTracks    = cfg.GetInt   ("physics", "MAX_TRACKS_PER_TELESCOPE", 4);

	for (int t = 0; t < kTelescopes; t++){
		for (int f = 0; f < 2; f++) for (int s = 0; s < kStrips; s++) fStripOn[t][f][s] = true;
		for (int c = 0; c < kCrystals; c++) fCsIOn[t][c] = true;
	}
	std::vector<std::vector<std::string>> all = cfg.GetAll("physics", "DISABLE_ALL");
	for (size_t i = 0; i < all.size(); i++){
		int t; std::string rest;
		if (all[i].empty() || !splitTele(all[i][0], t, rest)){
			std::cerr << "HiRAPhysics: bad DISABLE_ALL entry\n";
			continue;
		}
		for (int f = 0; f < 2; f++) for (int s = 0; s < kStrips; s++) fStripOn[t][f][s] = false;
		for (int c = 0; c < kCrystals; c++) fCsIOn[t][c] = false;
	}
	std::vector<std::vector<std::string>> ch = cfg.GetAll("physics", "DISABLE_CHANNEL");
	for (size_t i = 0; i < ch.size(); i++){
		int t; std::string rest;
		if (ch[i].empty() || !splitTele(ch[i][0], t, rest)){
			std::cerr << "HiRAPhysics: bad DISABLE_CHANNEL entry\n";
			continue;
		}
		if (rest.compare(0, 5, "STREF") == 0 || rest.compare(0, 5, "STREB") == 0){
			int s = std::atoi(rest.substr(5).c_str());
			if (s >= 0 && s < kStrips) fStripOn[t][rest[4] == 'F' ? 0 : 1][s] = false;
		}
		else if (rest.compare(0, 3, "CSI") == 0){
			int c = std::atoi(rest.substr(3).c_str());
			if (c >= 0 && c < kCrystals) fCsIOn[t][c] = false;
		}
		else {
			std::cerr << "HiRAPhysics: unknown channel type in DISABLE_CHANNEL " << ch[i][0] << "\n";
		}
	}
	std::cout << "HiRAPhysics: matching " << fMatchNSigma << " x " << fMatchSigma << " MeV, max "
	          << fMaxTracks << " particles/telescope, " << all.size() << " telescopes + "
	          << ch.size() << " channels disabled\n";
	return true;
}

void HiRAPhysics::Apply(HiRAData &d){
	// ---- 1. CsI: keep crystals above threshold and enabled
	{
		size_t j = 0;
		for (int i = 0; i < d.fCsImulti; i++){
			if (d.fCsIRawADC[i] < fCsIRawThresh || !CsIEnabled(d.fCsInumtel[i], d.fCsInumcsi[i])){
				continue;
			}
			d.fCsInumtel[j] = d.fCsInumtel[i];  d.fCsInumcsi[j] = d.fCsInumcsi[i];
			d.fCsIRawADC[j] = d.fCsIRawADC[i];  d.fCsITime[j]   = d.fCsITime[i];
			j++;
		}
		d.fCsInumtel.resize(j);  d.fCsInumcsi.resize(j);  d.fCsIRawADC.resize(j);  d.fCsITime.resize(j);
		d.fCsImulti = (Int_t)j;
	}

	// ---- 2. usable strips per telescope and face (indices into the EF/EB blocks)
	std::vector<int> ef[kTelescopes], eb[kTelescopes];
	for (int i = 0; i < d.fEFmulti; i++){
		int t = d.fEFnumtel[i];
		if (StripEnabled(t, 0, d.fEFnumstrip[i]) && d.fEFEnergyHi[i] >= fEFRawThresh &&
		    d.fEFEnergyCal[i] > kNone && d.fEFEnergyCal[i] >= fEFThresh){
			ef[t].push_back(i);
		}
	}
	for (int i = 0; i < d.fEBmulti; i++){
		int t = d.fEBnumtel[i];
		if (StripEnabled(t, 1, d.fEBnumstrip[i]) && d.fEBEnergyHi[i] >= fEBRawThresh &&
		    d.fEBEnergyCal[i] > kNone && d.fEBEnergyCal[i] >= fEBThresh){
			eb[t].push_back(i);
		}
	}

	// ---- 3. front x back pairing, best |dE| first, each strip once
	const double maxHalfDiff = fMatchNSigma * fMatchSigma;
	for (int t = 0; t < kTelescopes; t++){
		if (ef[t].empty() || eb[t].empty()){
			continue;
		}
		if ((int)ef[t].size() > fMaxStripMult || (int)eb[t].size() > fMaxStripMult){
			fCount["telescopes over MAX_STRIP_MULTIPLICITY"]++;
			continue;
		}
		struct Cand { double dE; int f, b; };
		std::vector<Cand> cand;
		for (size_t i = 0; i < ef[t].size(); i++){
			for (size_t j = 0; j < eb[t].size(); j++){
				double dE = std::fabs(d.fEFEnergyCal[ef[t][i]] - d.fEBEnergyCal[eb[t][j]]);
				if (dE / 2.0 < maxHalfDiff){
					cand.push_back({dE, ef[t][i], eb[t][j]});
				}
			}
		}
		std::sort(cand.begin(), cand.end(), [](const Cand &a, const Cand &b){ return a.dE < b.dE; });

		std::vector<bool> usedF(d.fEFmulti, false), usedB(d.fEBmulti, false);
		int nTracks = 0;
		for (size_t k = 0; k < cand.size() && nTracks < fMaxTracks; k++){
			int f = cand[k].f, b = cand[k].b;
			if (usedF[f] || usedB[b]){
				continue;
			}
			usedF[f] = usedB[b] = true;
			nTracks++;

			// ---- 4. particle: strips, the CsI behind the pixel, pixel angles
			const HiRAPixel *px = fGeo.GetPixel(t, d.fEFnumstrip[f], d.fEBnumstrip[b]);
			int csi = px ? px->csi % 4 : -1;
			double csiADC = kNone, csiTime = kNone;
			for (int c = 0; c < d.fCsImulti; c++){
				if (d.fCsInumtel[c] == t && d.fCsInumcsi[c] == csi){
					csiADC  = d.fCsIRawADC[c];
					csiTime = d.fCsITime[c];
					break;
				}
			}
			d.fnumtel.push_back(t);
			d.fnumstripf.push_back(d.fEFnumstrip[f]);
			d.fnumstripb.push_back(d.fEBnumstrip[b]);
			d.fnumcsi.push_back(csi);
			d.fEnergySifHi.push_back(d.fEFEnergyHi[f]);
			d.fEnergySifLo.push_back(d.fEFEnergyLo[f]);
			d.fTimeSif.push_back(d.fEFTime[f]);
			d.fEnergySifCal.push_back(d.fEFEnergyCal[f]);
			d.fEnergySibHi.push_back(d.fEBEnergyHi[b]);
			d.fEnergySibLo.push_back(d.fEBEnergyLo[b]);
			d.fTimeSib.push_back(d.fEBTime[b]);
			d.fEnergySibCal.push_back(d.fEBEnergyCal[b]);
			d.fCsIADC.push_back(csiADC);
			d.fTimecsi.push_back(csiTime);
			d.fTheta.push_back(px ? px->theta : kNone);
			d.fPhi.push_back(px ? px->phi : kNone);
			d.fdTheta.push_back(px ? px->dtheta : kNone);
			d.fdPhi.push_back(px ? px->dphi : kNone);
		}
		fCount["strips left unpaired"] += ef[t].size() + eb[t].size() - 2 * nTracks;
	}
	d.fmulti = (Int_t)d.fnumtel.size();
	fCount["particles"] += d.fmulti;
}

void HiRAPhysics::PrintSummary() const{
	std::cout << "HiRAPhysics:";
	for (std::map<std::string, long>::const_iterator it = fCount.begin(); it != fCount.end(); ++it){
		std::cout << "  " << it->first << " " << it->second << ";";
	}
	std::cout << "\n";
}

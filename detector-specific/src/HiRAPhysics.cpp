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

	fNeighbourDist= cfg.GetDouble("physics", "NEIGHBOUR_DISTANCE", 1.41422);
	fRequireCsI   = cfg.GetInt("physics", "IGNORE_NOT_MATCHING_CSI", 1) != 0;

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
				std::vector<HiRAStripPair> cand;
		for (size_t i = 0; i < ef[t].size(); i++){
			for (size_t j = 0; j < eb[t].size(); j++){
				double dE = std::fabs(d.fEFEnergyCal[ef[t][i]] - d.fEBEnergyCal[eb[t][j]]);
				if (dE / 2.0 < maxHalfDiff){
					HiRAStripPair p;
					p.dE = dE;
					p.f  = ef[t][i];
					p.b  = eb[t][j];
					cand.push_back(p);
				}
			}
		}
		std::sort(cand.begin(), cand.end(), HiRAPhysics::SmallerDE);

		// ---- 4. tracks: best |dE| pairs first, each strip used once
		std::vector<HiRATrack> trk;
		std::vector<bool> usedF(d.fEFmulti, false), usedB(d.fEBmulti, false);
		for (size_t k = 0; k < cand.size() && (int)trk.size() < fMaxTracks; k++){
			int f = cand[k].f, b = cand[k].b;
			if (usedF[f] || usedB[b]){
				continue;
			}
			usedF[f] = usedB[b] = true;
			HiRATrack x;
			x.f   = f;
			x.b   = b;
			x.px  = fGeo.GetPixel(t, d.fEFnumstrip[f], d.fEBnumstrip[b]);
			x.csi = x.px ? x.px->csi % 4 : -1;
			x.eF  = d.fEFEnergyCal[f];
			x.eB  = d.fEBEnergyCal[b];
			trk.push_back(x);
		}
		fCount["strips left unpaired"] += ef[t].size() + eb[t].size() - 2 * trk.size();

		// ---- 5. neighbour merge (THiraPhysics): two tracks within NEIGHBOUR_DISTANCE strips
		//         (sqrt(2) = 3x3 block) on the same CsI become one; energies summed, each track merges once
		if (fNeighbourDist > 0){
			std::vector<bool> merged(trk.size(), false);
			for (size_t j = 0; j < trk.size(); j++){
				for (size_t k = 0; k < trk.size(); k++){
					if (j == k || merged[j] || merged[k] || trk[j].csi != trk[k].csi){
						continue;
					}
					double dx = d.fEFnumstrip[trk[j].f] - d.fEFnumstrip[trk[k].f];
					double dy = d.fEBnumstrip[trk[j].b] - d.fEBnumstrip[trk[k].b];
					if (dx * dx + dy * dy <= fNeighbourDist * fNeighbourDist + 1e-6){
						trk[j].eF += trk[k].eF;
						trk[j].eB += trk[k].eB;
						trk[k].keep = false;
						merged[j] = merged[k] = true;
						fCount["tracks merged"]++;
					}
				}
			}
		}

		// ---- 6. crystal multiplicity (THiraPhysics): 2+ tracks on one CsI -> reject all of them
		int nOnCsI[kCrystals] = {0, 0, 0, 0};
		for (size_t j = 0; j < trk.size(); j++){
			if (trk[j].keep && trk[j].csi >= 0){
				nOnCsI[trk[j].csi]++;
			}
		}
		for (size_t j = 0; j < trk.size(); j++){
			if (trk[j].keep && trk[j].csi >= 0 && nOnCsI[trk[j].csi] > 1){
				trk[j].keep = false;
				fCount["tracks sharing a CsI (rejected)"]++;
			}
		}

		// ---- 7. particles: the track's CsI must have fired (IGNORE_NOT_MATCHING_CSI)
		for (size_t j = 0; j < trk.size(); j++){
			if (!trk[j].keep){
				continue;
			}
			const HiRATrack &x = trk[j];
			double csiADC = kNone, csiTime = kNone;
			for (int c = 0; c < d.fCsImulti; c++){
				if (d.fCsInumtel[c] == t && d.fCsInumcsi[c] == x.csi){
					csiADC  = d.fCsIRawADC[c];
					csiTime = d.fCsITime[c];
					break;
				}
			}
			if (fRequireCsI && csiADC <= kNone){
				fCount["tracks without a fired CsI (rejected)"]++;
				continue;
			}
			d.fnumtel.push_back(t);
			d.fnumstripf.push_back(d.fEFnumstrip[x.f]);
			d.fnumstripb.push_back(d.fEBnumstrip[x.b]);
			d.fnumcsi.push_back(x.csi);
			d.fEnergySifHi.push_back(d.fEFEnergyHi[x.f]);	// raw: kept strip only
			d.fEnergySifLo.push_back(d.fEFEnergyLo[x.f]);
			d.fTimeSif.push_back(d.fEFTime[x.f]);
			d.fEnergySifCal.push_back(x.eF);		// MeV: summed if merged
			d.fEnergySibHi.push_back(d.fEBEnergyHi[x.b]);
			d.fEnergySibLo.push_back(d.fEBEnergyLo[x.b]);
			d.fTimeSib.push_back(d.fEBTime[x.b]);
			d.fEnergySibCal.push_back(x.eB);
			d.fCsIADC.push_back(csiADC);
			d.fTimecsi.push_back(csiTime);
			d.fTheta.push_back(x.px ? x.px->theta : kNone);
			d.fPhi.push_back(x.px ? x.px->phi : kNone);
			d.fdTheta.push_back(x.px ? x.px->dtheta : kNone);
			d.fdPhi.push_back(x.px ? x.px->dphi : kNone);
		}
	}
	d.fmulti = (Int_t)d.fnumtel.size();
	fCount["particles"] += d.fmulti;
}

bool HiRAPhysics::SmallerDE(const HiRAStripPair& a, const HiRAStripPair& b){
	return a.dE < b.dE;
}

void HiRAPhysics::PrintSummary() const{
	std::cout << "HiRAPhysics:";
	for (std::map<std::string, long>::const_iterator it = fCount.begin(); it != fCount.end(); ++it){
		std::cout << "  " << it->first << " " << it->second << ";";
	}
	std::cout << "\n";
}

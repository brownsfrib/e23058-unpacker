#include "HiRAPhysics.h"

#include <algorithm>
#include <cmath>
#include <iostream>

HiRAPhysics::HiRAPhysics(const HiRAGeometry &geo, const HiRAConfig &cfg, const HiRACalibration &cal)
	: fGeo(geo), fCfg(cfg), fCal(cal) {}

void HiRAPhysics::Clear(){
	strips.clear();
	csi.clear();
	pixels.clear();
	tracks.clear();
}

void HiRAPhysics::Build(const HiRA &hira){
	Clear();
	if (!hira.fired){
		return;
	}
	PreTreat(hira);
	PairFrontBack();
	CombineNeighbours();
	MatchCsI();
	AssignAngles();
}

// ---- step 2: raw strips/CsI -> mapped, enabled, above threshold, calibrated
void HiRAPhysics::PreTreat(const HiRA &hira){
	for (size_t i = 0; i < hira.strips.size(); i++){
		const HINPStrip &s = hira.strips[i];
		const HiRAChannel *g = fGeo.Get(s.motherboard, s.chip, s.strip);
		if (!g){                                              fCount["strip not in geometry"]++; continue; }
		if (!fCfg.StripEnabled(g->telescope, g->face, s.strip)){ fCount["strip disabled"]++;       continue; }
		double rawThr = (g->face == 0) ? fCfg.efRawThresh : fCfg.ebRawThresh;
		if (s.highGain < rawThr){                             fCount["strip below raw threshold"]++; continue; }

		HiRAStripHit h;
		h.telescope = g->telescope;
		h.face      = g->face;
		h.strip     = s.strip;
		h.rawHG     = s.highGain;
		h.rawLG     = s.lowGain;
		h.time      = s.time;
		if (!fCal.Energy(h.telescope, h.face, h.strip, h.rawHG, h.rawLG, fCfg.hgSaturation, h.energy, h.usedLG)){
			fCount["strip not calibrated"]++; continue;
		}
		double thr = (g->face == 0) ? fCfg.efThresh : fCfg.ebThresh;
		if (h.energy < thr){                                  fCount["strip below threshold"]++; continue; }
		strips.push_back(h);
	}

	// CsI: every crystal whose ADC channel is above threshold
	for (int id = 0; id < HiRAGeometry::kNCsI; id++){
		const HiRACsI *c = fGeo.GetCsI(id);
		if (!c || c->adcModule < 0){
			continue;
		}
		const V785 &adc = (c->adcModule == 0) ? hira.adc1 : hira.adc2;
		if (!adc.has[c->adcChannel] || adc.value[c->adcChannel] < fCfg.csiRawThresh){
			continue;
		}
		if (!fCfg.CsIEnabled(c->telescope, c->quadrant)){
			continue;
		}
		HiRACsIHit h;
		h.id        = id;
		h.telescope = c->telescope;
		h.crystal   = c->quadrant;
		h.raw       = adc.value[c->adcChannel];
		csi.push_back(h);
	}
	fCount["strips kept"] += strips.size();
	fCount["CsI hits"]    += csi.size();
}

// ---- step 3: front x back pairs in the same telescope with matching energy.
// Best match (smallest |dE|) first; each strip used once.
void HiRAPhysics::PairFrontBack(){
	const double maxHalfDiff = fCfg.matchNSigma * fCfg.matchSigma;

	for (int t = 0; t < HiRAConfig::kTelescopes; t++){
		std::vector<int> ef, eb;			// indices into strips
		for (size_t i = 0; i < strips.size(); i++){
			if (strips[i].telescope != t) continue;
			(strips[i].face == 0 ? ef : eb).push_back(i);
		}
		if (ef.empty() || eb.empty()){
			continue;
		}
		if ((int)ef.size() > fCfg.maxStripMult || (int)eb.size() > fCfg.maxStripMult){
			fCount["telescope over max strip multiplicity"]++;
			continue;
		}

		struct Cand { double dE; int f, b; };
		std::vector<Cand> cand;
		for (size_t i = 0; i < ef.size(); i++){
			for (size_t j = 0; j < eb.size(); j++){
				double dE = std::fabs(strips[ef[i]].energy - strips[eb[j]].energy);
				if (dE / 2.0 < maxHalfDiff){
					cand.push_back({dE, ef[i], eb[j]});
				}
			}
		}
		std::sort(cand.begin(), cand.end(), [](const Cand &a, const Cand &b){ return a.dE < b.dE; });

		std::vector<bool> used(strips.size(), false);
		size_t before = pixels.size();
		for (size_t k = 0; k < cand.size(); k++){
			if (used[cand[k].f] || used[cand[k].b]) continue;
			used[cand[k].f] = used[cand[k].b] = true;
			const HiRAStripHit &f = strips[cand[k].f], &b = strips[cand[k].b];
			HiRAPixelHit p;
			p.telescope = t;
			p.ef = f.strip;          p.eb = b.strip;
			p.efEnergy = f.energy;   p.ebEnergy = b.energy;
			p.efTime = f.time;       p.ebTime = b.time;
			p.crystal = fCfg.CrystalFor(p.ef, p.eb);
			pixels.push_back(p);
		}
		fCount["strips left unpaired"] += ef.size() + eb.size() - 2 * (pixels.size() - before);
	}
	fCount["pixels"] += pixels.size();
}

void HiRAPhysics::CombineNeighbours(){
	// TODO step 4 - merge pixels within neighbourDistance in the same crystal
}

void HiRAPhysics::MatchCsI(){
	// TODO step 5 - require a CsI hit in the pixel's crystal. For now every pixel
	// becomes a track; csiId is the CsI behind the pixel, csiRaw is filled if it fired.
	for (size_t i = 0; i < pixels.size(); i++){
		const HiRAPixelHit &p = pixels[i];
		HiRATrack t;
		t.telescope = p.telescope;
		t.ef = p.ef;              t.eb = p.eb;
		t.efEnergy = p.efEnergy;  t.ebEnergy = p.ebEnergy;
		t.siEnergy = p.efEnergy;  t.siTime = p.efTime;
		t.csiId = (p.crystal >= 0) ? 4 * p.telescope + p.crystal : -1;
		for (size_t k = 0; k < csi.size(); k++){
			if (csi[k].id == t.csiId){
				t.csiRaw = csi[k].raw;
				break;
			}
		}
		tracks.push_back(t);
	}
}

// ---- step 6: angles from the pixel table
void HiRAPhysics::AssignAngles(){
	for (size_t i = 0; i < tracks.size(); i++){
		HiRATrack &t = tracks[i];
		const HiRAPixel *px = fGeo.GetPixel(t.telescope, t.ef, t.eb);
		if (!px){
			fCount["track without pixel angles"]++;
			continue;
		}
		t.theta = px->theta;  t.dtheta = px->dtheta;
		t.phi   = px->phi;    t.dphi   = px->dphi;
	}
	fCount["tracks"] += tracks.size();
}

void HiRAPhysics::PrintSummary() const{
	std::cout << "HiRAPhysics:\n";
	for (std::map<std::string, long>::const_iterator it = fCount.begin(); it != fCount.end(); ++it){
		std::cout << "  " << it->first << ": " << it->second << "\n";
	}
}

#include "HiRAPhysics.h"

#include <iostream>

// SKELETON: bodies are filled in step by step (see the step numbers in the TODOs)

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

void HiRAPhysics::PreTreat(const HiRA &hira){
	// TODO step 2
	// for each HINPStrip s in hira.strips:
	//   g = fGeo.Get(s.motherboard, s.chip, s.strip) -> telescope, face   (skip if null)
	//   skip if !fCfg.StripEnabled(...) or raw HG below SI_EF/EB_E_RAW_THRESHOLD
	//   fCal.Energy(...) -> MeV; skip if below SI_EF/EB_E_THRESHOLD
	//   push HiRAStripHit
	// for each CsI channel (hira.adc1 / adc2 via the CsI map), raw > CSI_E_RAW_THRESHOLD:
	//   push HiRACsIHit (+ TDC time via the CsI map's tdcChannel)
	(void)hira;
}

void HiRAPhysics::PairFrontBack(){
	// TODO step 3 - per telescope: all EF x EB pairs with |E_EF - E_EB|/2 < nSigma*sigma,
	// then resolve: best |dE| first, each strip used once; enforce maxStripMult
}

void HiRAPhysics::CombineNeighbours(){
	// TODO step 4 - port of the stripDist / epsilon block: merge pixels within
	// neighbourDistance that map to the same crystal; sum energies; keep the lower index
}

void HiRAPhysics::MatchCsI(){
	// TODO step 5 - crystalMult per telescope; pixel + CsI in the same crystal and
	// crystalMult < 2 -> HiRATrack; IGNORE_NOT_MATCHING_CSI drops pixels with no CsI
}

void HiRAPhysics::AssignAngles(){
	// TODO step 6 - theta/phi/dtheta/dphi from the pixel table (HiRAGeometry::GetPixel)
}

void HiRAPhysics::PrintSummary() const{
	std::cout << "HiRAPhysics:";
	for (std::map<std::string, long>::const_iterator it = fCount.begin(); it != fCount.end(); ++it){
		std::cout << "  " << it->first << " " << it->second;
	}
	std::cout << "\n";
}

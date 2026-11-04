#include "HiRATree.h"

static const double kDefault = -10000;

HiRATree::HiRATree(HiRA &hira, const HiRAPhysics *phys, const char *name) : fHiRA(hira), fPhys(phys){
	fTree = new TTree(name, "HiRA (silicon strips, CsI, TDC)");	// owned by the open TFile

	fTree->Branch("eventNumber",    &fEventNumber, "eventNumber/L");
	fTree->Branch("timestamp",      &fTimestamp,   "timestamp/l");

	fTree->Branch("hiraNStrips",    &fNStrips,     "hiraNStrips/I");
	fTree->Branch("hiraMB",         fMB,           "hiraMB[hiraNStrips]/I");
	fTree->Branch("hiraChip",       fChip,         "hiraChip[hiraNStrips]/I");
	fTree->Branch("hiraBoard",      fBoard,        "hiraBoard[hiraNStrips]/I");
	fTree->Branch("hiraStrip",      fStrip,        "hiraStrip[hiraNStrips]/I");
	fTree->Branch("hiraDetCode",    &fDetCode);					// std::vector<std::string>
	fTree->Branch("hiraSiHG",       fSiHG,         "hiraSiHG[hiraNStrips]/D");
	fTree->Branch("hiraSiLG",       fSiLG,         "hiraSiLG[hiraNStrips]/D");
	fTree->Branch("hiraSiTime",     fSiTime,       "hiraSiTime[hiraNStrips]/D");
	fTree->Branch("hiraTheta",      fTheta,        "hiraTheta[hiraNStrips]/D");	// DEFAULT if not in geometry
	fTree->Branch("hiraDTheta",     fDTheta,       "hiraDTheta[hiraNStrips]/D");
	fTree->Branch("hiraPhi",        fPhi,          "hiraPhi[hiraNStrips]/D");
	fTree->Branch("hiraDPhi",       fDPhi,         "hiraDPhi[hiraNStrips]/D");

	fTree->Branch("hiraCsI1",       fCsI1,         "hiraCsI1[32]/D");		// ADC1, DEFAULT if absent
	fTree->Branch("hiraCsI2",       fCsI2,         "hiraCsI2[32]/D");		// ADC2

	fTree->Branch("hiraTDCMult",    &fTDCMult,     "hiraTDCMult/I");
	fTree->Branch("hiraTDCChannel", fTDCChannel,   "hiraTDCChannel[hiraTDCMult]/I");
	fTree->Branch("hiraTDCTime",    fTDCTime,      "hiraTDCTime[hiraTDCMult]/D");	// raw

	// reconstructed tracks: one entry per paired front x back pixel
	fTree->Branch("hiraNTrk",       &fNTrk,        "hiraNTrk/I");
	fTree->Branch("hiraTrkTele",    fTrkTele,      "hiraTrkTele[hiraNTrk]/I");
	fTree->Branch("hiraTrkEF",      fTrkEF,        "hiraTrkEF[hiraNTrk]/I");
	fTree->Branch("hiraTrkEB",      fTrkEB,        "hiraTrkEB[hiraNTrk]/I");
	fTree->Branch("hiraTrkEFE",     fTrkEFE,       "hiraTrkEFE[hiraNTrk]/D");	// MeV
	fTree->Branch("hiraTrkEBE",     fTrkEBE,       "hiraTrkEBE[hiraNTrk]/D");
	fTree->Branch("hiraTrkCsI",     fTrkCsI,       "hiraTrkCsI[hiraNTrk]/I");	// CsI behind the pixel
	fTree->Branch("hiraTrkCsIRaw",  fTrkCsIRaw,    "hiraTrkCsIRaw[hiraNTrk]/D");	// 0 if it did not fire
	fTree->Branch("hiraTrkTheta",   fTrkTheta,     "hiraTrkTheta[hiraNTrk]/D");	// pixel angles
	fTree->Branch("hiraTrkDTheta",  fTrkDTheta,    "hiraTrkDTheta[hiraNTrk]/D");
	fTree->Branch("hiraTrkPhi",     fTrkPhi,       "hiraTrkPhi[hiraNTrk]/D");
	fTree->Branch("hiraTrkDPhi",    fTrkDPhi,      "hiraTrkDPhi[hiraNTrk]/D");
}

bool HiRATree::Build(){
	// silicon strips
	fNStrips = 0;
	fDetCode.clear();
	for (size_t i=0; i < fHiRA.strips.size() && fNStrips < kMaxStrips; i++){
		const HINPStrip   &s = fHiRA.strips[i];
		const HiRAChannel *g = fHiRA.StripGeometry(s);
		fMB[fNStrips]     = s.motherboard;
		fChip[fNStrips]   = s.chip;
		fBoard[fNStrips]  = s.board;
		fStrip[fNStrips]  = s.strip;
		fSiHG[fNStrips]   = s.highGain;
		fSiLG[fNStrips]   = s.lowGain;
		fSiTime[fNStrips] = s.time;
		fTheta[fNStrips]  = g ? g->theta  : kDefault;
		fDTheta[fNStrips] = g ? g->dtheta : kDefault;
		fPhi[fNStrips]    = g ? g->phi    : kDefault;
		fDPhi[fNStrips]   = g ? g->dphi   : kDefault;
		fDetCode.push_back(g ? g->detCode : std::string());
		fNStrips++;
	}

	// CsI
	for (int c = 0; c < V785::kChannels; c++){
		fCsI1[c] = fHiRA.adc1.has[c] ? fHiRA.adc1.value[c] : kDefault;
		fCsI2[c] = fHiRA.adc2.has[c] ? fHiRA.adc2.value[c] : kDefault;
	}

	// TDC (leading edges, raw)
	fTDCMult = 0;
	for (size_t i = 0; i < fHiRA.tdc.hits.size() && fTDCMult < kMaxTDC; i++){
		const V1190Hit &h = fHiRA.tdc.hits[i];
		if (h.trailing){
			continue;
		}
		fTDCChannel[fTDCMult] = h.channel;
		fTDCTime[fTDCMult]    = h.time;
		fTDCMult++;
	}

	// reconstructed tracks
	fNTrk = 0;
	if (fPhys){
		for (size_t i = 0; i < fPhys->tracks.size() && fNTrk < kMaxStrips; i++){
			const HiRATrack &t = fPhys->tracks[i];
			fTrkTele[fNTrk]   = t.telescope;
			fTrkEF[fNTrk]     = t.ef;
			fTrkEB[fNTrk]     = t.eb;
			fTrkEFE[fNTrk]    = t.efEnergy;
			fTrkEBE[fNTrk]    = t.ebEnergy;
			fTrkCsI[fNTrk]    = t.csiId;
			fTrkCsIRaw[fNTrk] = t.csiRaw;
			fTrkTheta[fNTrk]  = t.theta;
			fTrkDTheta[fNTrk] = t.dtheta;
			fTrkPhi[fNTrk]    = t.phi;
			fTrkDPhi[fNTrk]   = t.dphi;
			fNTrk++;
		}
	}

	return fHiRA.fired;
}

void HiRATree::Fill(Long64_t eventNumber, ULong64_t timestamp){
	fEventNumber = eventNumber;
	fTimestamp   = timestamp;
	fTree->Fill();
}

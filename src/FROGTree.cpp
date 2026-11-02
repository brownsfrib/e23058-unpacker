#include "FROGTree.h"

static const double kDefault = -10000;

FROGTree::FROGTree(FROG &frog, const char *name) : fFrog(frog){
	fTree = new TTree(name, "FROG (A5202 SiPM array)");	// owned by the open TFile

	fTree->Branch("eventNumber", &fEventNumber, "eventNumber/L");
	fTree->Branch("timestamp",   &fTimestamp,   "timestamp/l");
	fTree->Branch("frogNBoards", &fNBoards,     "frogNBoards/I");
	fTree->Branch("frogMult",    &fMult,        "frogMult/I");
	fTree->Branch("frogBoard",   fBoard,        "frogBoard[frogMult]/I");
	fTree->Branch("frogChannel", fChannel,      "frogChannel[frogMult]/I");
	fTree->Branch("frogLG",      fLG,           "frogLG[frogMult]/D");
	fTree->Branch("frogHG",      fHG,           "frogHG[frogMult]/D");
	fTree->Branch("frogToA",     fToA,          "frogToA[frogMult]/D");	// DEFAULT if absent
	fTree->Branch("frogToT",     fToT,          "frogToT[frogMult]/D");
	fTree->Branch("frogTheta",   fTheta,        "frogTheta[frogMult]/D");	// DEFAULT if not in geometry
	fTree->Branch("frogDTheta",  fDTheta,       "frogDTheta[frogMult]/D");
	fTree->Branch("frogPhi",     fPhi,          "frogPhi[frogMult]/D");
	fTree->Branch("frogDPhi",    fDPhi,         "frogDPhi[frogMult]/D");
}

bool FROGTree::Build(){
	fNBoards = (Int_t)fFrog.boards.size();
	fMult    = 0;
	for (size_t i = 0; i < fFrog.hits.size() && fMult < kMaxHits; i++){
		const FrogHit &h = fFrog.hits[i];
		if (totCut >= 0 && (!h.has_tot || h.tot < totCut)){
			continue;
		}
		fBoard[fMult]   = h.board;
		fChannel[fMult] = h.channel;
		fLG[fMult]      = h.has_lg     ? h.lg     : kDefault;
		fHG[fMult]      = h.has_hg     ? h.hg     : kDefault;
		fToA[fMult]     = h.has_toa    ? h.toa    : kDefault;
		fToT[fMult]     = h.has_tot    ? h.tot    : kDefault;
		fTheta[fMult]   = h.has_angles ? h.theta  : kDefault;
		fDTheta[fMult]  = h.has_angles ? h.dtheta : kDefault;
		fPhi[fMult]     = h.has_angles ? h.phi    : kDefault;
		fDPhi[fMult]    = h.has_angles ? h.dphi   : kDefault;
		fMult++;
	}
	return fMult > 0;
}

void FROGTree::Fill(Long64_t eventNumber, ULong64_t timestamp){
	fEventNumber = eventNumber;
	fTimestamp   = timestamp;
	fTree->Fill();
}

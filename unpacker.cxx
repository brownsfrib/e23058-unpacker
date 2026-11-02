// Multi-detector unpacker: reads a glommed NSCLDAQ .evt file and writes a TTree.
// Each source ID is routed to its detector class; only FROG is decoded so far.
//
//   export STAGEAREA=/path/to/stagearea/   RUN=0100   basePath=/path/to/project
//   . compile.sh unpacker.cxx && ./bin/unpacker

// C++ includes
#include <iostream>
#include <string>
#include <stdexcept>

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"

// Unpacker includes
#include "Unpacker.h"
#include "FROG.h"
#include "Pixie.h"
#include "HiRA.h"

#define DEFAULT -10000
#define kMaxFrogHits 512	// 6 boards x 64 channels = 384
#define kMaxHiraTDC 256		// using a V1190A, 128 channels. this is fine.
#define kMaxHiraStrips 512	// matches maxlen used in HINP class

// ---- source ID -> detector map: the one place to edit -----------------------
//   FROG  : sid 3  (FROG sub-event builder; inner sids 0-5 are the A5202 boards)
//   Pixie : sid 6
//   HiRA  : sid 7
// -----------------------------------------------------------------------------

std::string requireEnv(const char *name){
	const char *v = gSystem->Getenv(name);
	if (v == nullptr){
		throw std::runtime_error(std::string("\033[1;31mEnvironment variable ") + name + " not set.\033[0m\n");
	}
	return std::string(v);
}

int main(int argc, char *argv[]){

	std::string basePath  = requireEnv("basePath");
	std::string stageArea = requireEnv("REGLOM");
	std::string runString = requireEnv("RUN");

	std::string runPath = stageArea + "run-" + runString + "-reglom.evt";
	if (gSystem->AccessPathName(runPath.c_str())){
		throw std::runtime_error("\033[1;31mFile " + runPath + " does not exist.\033[0m\n");
	}

	// ---- detectors ----
	Unpacker unpacker;
	// ---- Setting FROG unpacker ----
	FROG  frog;
	frog.SetHGThreshold(1000);          	// keep if HG >= 1000, any channel
	frog.SetHGThreshold(2, 46, 900);    	// per channel
	if (!frog.geometry.Load(basePath + "/FROG/combined-unpacker/input/FROG_channel_theta_phi.dat")){
		std::string msg = "\033[1;31mERROR: Could not load FROG_channel_theta_phi.dat input file\033[0m\n";
		throw std::runtime_error(msg);
	}
	frog.zeroSuppress = false;
	// ---- Setting FROG unpacker ----

	// -------------------------------

	// ---- Setting HiRA unpacker ----
	HiRA  hira;
	if (!hira.geometry.Load(basePath + "/FROG/combined-unapcker/input/Hira_channel_theta_phi.dat")){
		std::string msg = "\033[1;31mERROR: Could not load Hira_channel_theta_phi.dat input file\033[0m\n";
		throw std::runtime_error(msg);
	}
	// ---- Setting HiRA unpacker ----

	Pixie pixie; 				// not built yet

	unpacker.Register(3, &frog);
	unpacker.Register(7, &hira);		
	//unpacker.Register(6, &pixie);		// uncomment once Pixie::Unpack is written

	if (!unpacker.Open(runPath)){
		return 1;
	}

	// ---- output tree ----
	const std::string outputPath = basePath + "/FROG/combined-unpacker/output/run_" + runString + ".root";
	TFile *outputFile = TFile::Open(outputPath.c_str(), "RECREATE");
	TTree tree("FROG", "FROG glommed events");

	// ---- FROG information ----
	ULong64_t timestamp;
	Int_t     frogMult;
	Int_t 	  frogNBoards;
	Int_t     frogBoard[kMaxFrogHits];
	Int_t     frogChannel[kMaxFrogHits];
	Double_t  frogLG[kMaxFrogHits];
	Double_t  frogHG[kMaxFrogHits];
	Double_t  frogToA[kMaxFrogHits];
	Double_t  frogToT[kMaxFrogHits];

	// Angular information
	Double_t frogTheta[kMaxFrogHits];
	Double_t frogDTheta[kMaxFrogHits];
	Double_t frogPhi[kMaxFrogHits];
	Double_t frogDPhi[kMaxFrogHits];


	tree.Branch("timestamp",   	 &timestamp,  		"timestamp/l");
	tree.Branch("frogMult",    	 &frogMult,   		"frogMult/I");
	tree.Branch("frogNBoards", 	 &frogNBoards,		"frogNBoards/I");
	tree.Branch("frogBoard",   	 frogBoard,   		"frogBoard[frogMult]/I");
	tree.Branch("frogChannel", 	 frogChannel, 		"frogChannel[frogMult]/I");
	tree.Branch("frogLG",      	 frogLG,      		"frogLG[frogMult]/D");
	tree.Branch("frogHG",      	 frogHG,      		"frogHG[frogMult]/D");
	tree.Branch("frogToA",     	 frogToA,     		"frogToA[frogMult]/D");	// DEFAULT if absent
	tree.Branch("frogToT",     	 frogToT,     		"frogToT[frogMult]/D");	// DEFAULT if absent
	tree.Branch("frogTheta",	 frogTheta,	 	"frogTheta[frogMult]/D");
	tree.Branch("frogDTheta",	 frogDTheta,	 	"frogDTheta[frogMult]/D");
	tree.Branch("frogPhi",	 	 frogPhi,	 	"frogPhi[frogMult]/D");
	tree.Branch("frogDPhi",	 	 frogDPhi,	 	"frogDPhi[frogMult]/D");
	// ---- FROG information ----


	// ---- HiRA information ----
	Int_t     hiraNStrips;
	Int_t     hiraMB[kMaxHiraStrips];
	Int_t     hiraBoard[kMaxHiraStrips];
	Int_t     hiraStrip[kMaxHiraStrips];
	Double_t  hiraHG[kMaxHiraStrips];
	Double_t  hiraLG[kMaxHiraStrips];
	Double_t  hiraTime[kMaxHiraStrips];
	Double_t  hiraADC1[V785::kChannels];
	Double_t  hiraADC2[V785::kChannels];
	Int_t     hiraTDCMult;
	Int_t     hiraTDCChannel[kMaxHiraTDC];
	Double_t  hiraTDCTime[kMaxHiraTDC];

	// HiRA angular information goes here

	tree.Branch("hiraNStrips",    &hiraNStrips,   "hiraNStrips/I");
	tree.Branch("hiraMB",         hiraMB,         "hiraMB[hiraNStrips]/I");
	tree.Branch("hiraBoard",      hiraBoard,      "hiraBoard[hiraNStrips]/I");
	tree.Branch("hiraStrip",      hiraStrip,      "hiraStrip[hiraNStrips]/I");
	tree.Branch("hiraHG",         hiraHG,         "hiraHG[hiraNStrips]/D");
	tree.Branch("hiraLG",         hiraLG,         "hiraLG[hiraNStrips]/D");
	tree.Branch("hiraTime",       hiraTime,       "hiraTime[hiraNStrips]/D");
	tree.Branch("hiraADC1",       hiraADC1,       "hiraADC1[32]/D");	// DEFAULT if absent
	tree.Branch("hiraADC2",       hiraADC2,       "hiraADC2[32]/D");
	tree.Branch("hiraTDCMult",    &hiraTDCMult,   "hiraTDCMult/I");
	tree.Branch("hiraTDCChannel", hiraTDCChannel, "hiraTDCChannel[hiraTDCMult]/I");
	tree.Branch("hiraTDCTime",    hiraTDCTime,    "hiraTDCTime[hiraTDCMult]/D");
	// ---- HiRA information ----


	// ---- Pixie information ----
	// ----  not written yet  ----
	// ---- Pixie information ----


	// ---- event loop ----
	long written = 0;
	while (unpacker.NextEvent()){
		if ((unpacker.physicsEvents % 30000) == 0){
			std::cout << '\r' << unpacker.physicsEvents << std::flush;
		}

		if (frog.sawHeader){
			continue;	// A5202 run header: no hits (see summary)
		}
		// drop events with no detector we decode yet

		timestamp = unpacker.eventTimestamp;

		// ---- FROG writing ---- 
		frogNBoards = (Int_t)frog.boards.size();    // new
		frogMult = 0;
		for (size_t i = 0; i < frog.hits.size() && frogMult < kMaxFrogHits; i++){
			const FrogHit &h = frog.hits[i];
			if ((!h.has_tot) || (h.tot <= 5)){
				continue;
			}
			frogBoard[frogMult]   = h.board;
			frogChannel[frogMult] = h.channel;
			frogLG[frogMult]      = h.has_lg  ? h.lg  : DEFAULT;
			frogHG[frogMult]      = h.has_hg  ? h.hg  : DEFAULT;
			frogToA[frogMult]     = h.has_toa ? h.toa : DEFAULT;
			frogToT[frogMult]     = h.has_tot ? h.tot : DEFAULT;

			// angular information
			frogTheta[frogMult] 	= h.has_angles ? h.theta : DEFAULT;
			frogDTheta[frogMult] 	= h.has_angles ? h.dtheta : DEFAULT;
			frogPhi[frogMult] 	= h.has_angles ? h.phi : DEFAULT;
			frogDPhi[frogMult] 	= h.has_angles ? h.dphi : DEFAULT;

			frogMult++;
		}
		// ---- FROG writing ---- 

		// ---- HiRA writing ---- 
		hiraNStrips = 0;
		hiraTDCMult = 0;
		for (int ch=0; ch < V785::kChannels; ch++){
			hiraADC1[ch] = hira.adc1.has[ch] ? hira.adc1.value[ch] : DEFAULT;
			hiraADC2[ch] = hira.adc2.has[ch] ? hira.adc2.value[ch] : DEFAULT;
		}

		// ---- HiRA writing ---- 
		if ((frogMult == 0) && (!hira.fired)){
			continue;
		}

		tree.Fill();
		written++;
	}

	unpacker.PrintSummary();

	outputFile->cd();
	tree.Write();
	outputFile->Close();
	std::cout << "\033[1;35mWrote " << written << " events to " << outputPath << "\033[0m\n";

	return 0;
}

// Multi-detector unpacker: reads a glommed NSCLDAQ .evt file and writes one
// TTree per detector into a single ROOT file. Every tree gets one entry per
// kept event, so they line up entry-for-entry:
//
//   TTree *hira = (TTree*)f->Get("HiRA");
//   hira->AddFriend("FROG");
//   hira->Draw("hiraTheta:FROG.frogTheta");
//
//   export basePath=...  REGLOM=/path/to/reglom/  RUN=0100
//   make && ./bin/unpacker

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
#include "FROGTree.h"

#include "HiRA.h"
#include "HiRATree.h"
#include "HiRAConfig.h"
#include "HiRACalibration.h"
#include "HiRAGeometry.h"
#include "HiRAPhysics.h"

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
	const std::string projectPath = basePath + "/combined-unpacker";

	std::string runPath = stageArea + "run-" + runString + "-reglom.evt";
	if (gSystem->AccessPathName(runPath.c_str())){
		throw std::runtime_error("\033[1;31mFile " + runPath + " does not exist.\033[0m\n");
	}

	// ---- detectors ----
	Unpacker unpacker;
	FROG  frog;
	Pixie pixie;				// not built yet
	HiRA  hira;

	frog.SetHGThreshold(1000);		// keep if HG >= 1000, any channel
	frog.SetHGThreshold(2, 46, 900);	// per channel
	frog.zeroSuppress = false;

	if (!frog.geometry.Load(projectPath + "/input/FROG_channel_theta_phi.dat")){
		return 1;
	}

	HiRAConfig hiraCfg;
	hiraCfg.Load(projectPath + "/input/ConfigHira.dat");
	HiRACalibration hiraCal;
	hiraCal.Load(projectPath + "/input/hira-calibration/HiRA_strip_calibration.txt");
	if (!hira.geometry.Load(projectPath + "/input/HiRA_channel_theta_phi.dat")){
		return 1;
	}
	if (!hira.geometry.LoadCsI(projectPath + "/input/HiRA_csi.csv")){
		return 1;
	}
	if (!hira.geometry.LoadPixels(projectPath + "/input/HiRA_pixels.csv")){
		return 1;
	}
	HiRAPhysics hiraPhysics(hira.geometry, hiraCfg, hiraCal);




	// hira.tdcMap.Load(projectPath + "/input/HiRA_tdc_map.csv");	// format TBD

	unpacker.Register(3, &frog);
	// unpacker.Register(6, &pixie);	// uncomment once Pixie::Unpack is written
	unpacker.Register(7, &hira);

	if (!unpacker.Open(runPath)){
		return 1;
	}

	// ---- output: one file, one tree per detector ----
	const std::string outputPath = projectPath + "/output/run_" + runString + ".root";
	TFile *outputFile = TFile::Open(outputPath.c_str(), "RECREATE");

	FROGTree frogTree(frog);		// tree "FROG"
	HiRATree hiraTree(hira, &hiraPhysics);	// tree "HiRA"
	frogTree.totCut = 10;			// FROG channels need ToT >= 10 (< 0: no cut)

	// ---- event loop ----
	Long64_t written = 0;
	while (unpacker.NextEvent()){
		if ((unpacker.physicsEvents % 30000) == 0){
			std::cout << '\r' << unpacker.physicsEvents << std::flush;
		}
		if (frog.sawHeader){
			continue;			// A5202 run header: no hits (see summary)
		}

		hiraPhysics.Build(hira);
		bool frogHas = frogTree.Build();
		bool hiraHas = hiraTree.Build();
		if (!frogHas && !hiraHas){
			continue;			// nothing from any detector
		}

		// fill EVERY tree for every kept event, so entries stay aligned
		frogTree.Fill(written, unpacker.eventTimestamp);
		hiraTree.Fill(written, unpacker.eventTimestamp);
		written++;
	}

	hiraPhysics.PrintSummary();
	unpacker.PrintSummary();

	outputFile->Write();
	outputFile->Close();
	std::cout << "\033[1;35mWrote " << written << " events (trees FROG, HiRA) to "
	          << outputPath << "\033[0m\n";

	return 0;
}

// E23058 unpacker: glommed NSCLDAQ .evt -> output/run_<RUN>.root, tree "E23058"
// with branches FROG., HIRA., TDC., LANA., VETO. (one entry per kept event).
//
//   export REGLOM=/path/to/reglom/  RUN=0100
//   ./bin/unpacker            (run from the project directory)
//
// Per detector:  reader -> Data -> Geometry -> [Calibration] -> Physics -> TreeWriter
// Settings live in input/FROG/FROGConfig.dat, input/HIRA/HiRAConfig.dat and
// input/PIXIE/PixieConfig.dat. LANA./VETO. are raw + channel map only (no calibration).

// C++ includes
#include <iostream>
#include <string>
#include <vector>

// ROOT includes
#include "TSystem.h"

// Unpacker includes
#include "Config.h"
#include "TreeWriter.h"
#include "Unpacker.h"

#include "FROG.h"
#include "FROGData.h"
#include "FROGGeometry.h"
#include "FROGPhysics.h"

#include "HiRA.h"
#include "HiRAData.h"
#include "HiRAGeometry.h"
#include "HiRACalibration.h"
#include "HiRAPhysics.h"

#include "TDCData.h"
#include "TDCMapping.h"

#include "Pixie.h"
#include "PixieData.h"
#include "PixieMapping.h"

// ---- source IDs: FROG 3 (inner sids 0-5 = A5202 boards), Pixie 6, HiRA 7

static std::string requireEnv(const char *name){
	const char *v = gSystem->Getenv(name);
	if (v == nullptr){
		std::cerr << "\033[1;31mEnvironment variable " << name << " not set.\033[0m\n";
		exit(1);
	}
	return v;
}

int main(){
	const std::string run     = requireEnv("RUN");
	const std::string runPath = requireEnv("REGLOM") + "/run-" + run + "-reglom.evt";

	// ---- configuration
	Config frogCfg, hiraCfg, pixieCfg;
	if (!frogCfg.Load("input/FROG/FROGConfig.dat") || !hiraCfg.Load("input/HIRA/HiRAConfig.dat") ||
	    !pixieCfg.Load("input/PIXIE/PixieConfig.dat")){
		std::cerr << "\033[1;31mRun ./bin/unpacker from the project directory.\033[0m\n";
		return 1;
	}

	// ---- FROG
	FROGData     frogData;
	FROG         frog(&frogData);
	FROGGeometry frogGeo;
	FROGPhysics  frogPhys;
	if (!frogGeo.Load(frogCfg) || !frogPhys.Load(frogCfg)){
		return 1;
	}

	// ---- HiRA (+ the auxiliary TDC channels in the same stack)
	HiRAData        hiraData;
	HiRA            hira;
	HiRAGeometry    hiraGeo;
	HiRACalibration hiraCal;
	HiRAPhysics     hiraPhys(hiraGeo);
	TDCData         tdcData;
	TDCMapping      tdcMap;
	if (!hiraGeo.Load(hiraCfg) || !hiraCal.Load(hiraCfg) || !hiraPhys.Load(hiraCfg) ||
	    !tdcMap.Load(hiraCfg.GetString("geometry", "TDC_MAPPING"))){
		return 1;
	}

	// ---- Pixie / DDAS: every decoded hit -> pixieAll (not written), split by
	//      the channel map into lanaData (walls A, B) and vetoData
	PixieData    pixieAll, lanaData, vetoData;
	Pixie        pixie(&pixieAll, pixieCfg.GetInt("readout", "WRITE_TRACES", 0) != 0);
	PixieMapping pixieMap;
	const bool   keepPixieOnly = pixieCfg.GetInt("readout", "KEEP_PIXIE_ONLY", 1) != 0;
	std::vector<std::vector<std::string>> pixieSIDs = pixieCfg.GetAll("readout", "SOURCE_ID");
	if (!pixieMap.Load(pixieCfg)){
		return 1;
	}

	frogCfg.ReportUnused();
	hiraCfg.ReportUnused();
	pixieCfg.ReportUnused();

	// ---- input / output
	Unpacker unpacker;
	unpacker.Register(3, &frog);
	unpacker.Register(7, &hira);
	if (pixieSIDs.empty()){
		unpacker.Register(6, &pixie);
	}
	for (size_t i = 0; i < pixieSIDs.size(); i++){
		for (size_t j = 0; j < pixieSIDs[i].size(); j++){
			unpacker.Register((uint32_t)std::stoul(pixieSIDs[i][j]), &pixie);
		}
	}
	if (!unpacker.Open(runPath)){
		return 1;
	}
	gSystem->mkdir("output", true);
	TreeWriter writer("output/run_" + run + ".root");
	if (!writer.IsOpen()){
		return 1;
	}
	writer.Add("FROG.", &frogData);
	writer.Add("HIRA.", &hiraData);
	writer.Add("TDC.",  &tdcData);
	writer.Add("LANA.", &lanaData);
	writer.Add("VETO.", &vetoData);

	// ---- event loop
	while (unpacker.NextEvent()){
		if ((unpacker.physicsEvents % 30000) == 0){
			std::cout << '\r' << unpacker.physicsEvents << std::flush;
		}
		if (frog.sawHeader){
			continue;			// A5202 run header: no hits
		}

		frogGeo.Apply(frogData);
		frogPhys.Apply(frogData);

		hiraGeo.Map(hira, hiraData);
		hiraCal.Apply(hiraData);
		hiraPhys.Apply(hiraData);
		tdcMap.Fill(hira.tdc, tdcData);

		pixieMap.Split(pixieAll, lanaData, vetoData);

		const bool pixieHits = (lanaData.fMult > 0 || vetoData.fMult > 0);
		if (frogData.fNPass == 0 && !hira.fired && !(keepPixieOnly && pixieHits)){
			continue;			// nothing worth writing
		}
		writer.Fill(unpacker.eventTimestamp);
	}
	std::cout << "\n";

	hiraGeo.PrintSummary();
	hiraPhys.PrintSummary();
	unpacker.PrintSummary();		// includes FROG / HiRA / Pixie reader summaries
	pixieMap.PrintSummary();

	Long64_t n = writer.Entries();
	writer.Close();
	std::cout << "\033[1;35mWrote " << n << " events (tree E23058: FROG. HIRA. TDC. LANA. VETO.) to output/run_"
	          << run << ".root\033[0m\n";
	return 0;
}

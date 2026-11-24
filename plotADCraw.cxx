// E23058 unpacker: glommed NSCLDAQ .evt -> output/run_<RUN>.root, tree "E23058"
// with branches FROG., HIRA., TDC., NWB., NWA., VETO., PIXIETRIG. (one entry per kept event).
//
//   export REGLOM=/path/to/reglom/  RUN=0100
//   ./bin/unpacker            (run from the project directory)
//
// Per detector:  reader -> Data -> Geometry -> [Calibration] -> Physics -> TreeWriter
// Settings live in input/FROG/FROGConfig.dat, input/HIRA/HiRAConfig.dat and
// input/PIXIE/PixieConfig.dat. NWB./NWA./VETO. are raw + channel map only (no calibration).

// C++ includes
#include <iostream>
#include <string>
#include <vector>

// ROOT includes
#include "TSystem.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"

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

int main(int argc, char* argv[]){
	const std::string run     = requireEnv("RUN");
	const std::string runPath = requireEnv("REGLOM") + "/run-" + run + "-reglom.evt";

	TApplication app("rawADC", &argc, argv);
	gStyle->SetPalette(kViridis);

	Unpacker unpacker;
	HiRA hira;
	unpacker.Register(7, &hira);

	if (!unpacker.Open(runPath)){
		return 1;
	}

	const int nModules = 2; // number of ADC modules used
	TH2D* valueVsChannel[nModules];
	TH1D* spectra[nModules][V785::kChannels];
	for (int i=0; i < nModules; i++){
		valueVsChannel[i] = new TH2D(Form("adc%d", i), Form("ADC%d raw; Channel; Value", i),
				V785::kChannels, 0, V785::kChannels, 1024, 0, 4096);
		valueVsChannel[i]->GetXaxis()->CenterTitle(true);
		valueVsChannel[i]->GetYaxis()->CenterTitle(true);
		valueVsChannel[i]->SetOption("colz");

		for (int ch=0; ch < V785::kChannels; ch++){
			spectra[i][ch] = new TH1D(Form("adc%d_ch%d", i, ch), Form("ADC%d channel %d; Value; Counts", i, ch),
					1024, 0, 4096);
			spectra[i][ch]->GetXaxis()->CenterTitle(true);
			spectra[i][ch]->GetYaxis()->CenterTitle(true);
		}
	}
	while (unpacker.NextEvent()){
		if ((unpacker.physicsEvents % 30000) == 0){
			std::cout << '\r' << unpacker.physicsEvents << std::flush;
		}
		if (!hira.fired){
			continue;
		}
		const V785* adc[nModules] = {&hira.adc1, &hira.adc2};
		for (int i=0; i < nModules; i++){
			for (int ch=0; ch < V785::kChannels; ch++){
				if (!adc[i]->has[ch]){
					continue;
				}
				valueVsChannel[i]->Fill(ch, adc[i]->value[ch]);
				spectra[i][ch]->Fill(adc[i]->value[ch]);
			}
		}
	}
	std::cout << '\n';
	unpacker.PrintSummary();

	TCanvas can("can", "", 1000, 800);
	can.Divide(2,1);
	for (int i=0; i < nModules; i++){
		can.cd(i+1);
		valueVsChannel[i]->Draw();
	}
	can.Update();

	app.Run();


	return 0;
}

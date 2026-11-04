#ifndef TDCDATA_H
#define TDCDATA_H

// C/C++ includes
#include <map>
#include <string>

// ROOT includes
#include "Rtypes.h"

/**
 * !\brief Auxiliary V1190 TDC channels: written to the E23058 tree as branch "TDC."
 *
 * Every channel in input/TDC/TDCMapping.csv except the CsI ones (those go
 * to HIRA.fCsITime). Raw first leading-edge hit; -10000 = no hit.
 * Member names are the "Variable" column of TDCMapping.csv.
 * fChannels holds the same values, only for channels that fired, so a
 * macro can loop over them:  for (auto &c : tdc->fChannels) { c.first, c.second }
 */
class TDCData{
	public:
		TDCData();
		virtual ~TDCData();

		void Clear();

		// set a channel by its TDCMapping name; false if the name is not a member
		bool Set(const std::string &name, Double_t value);

		Double_t fMasterTrigger;
		Double_t fDiamondStrip0;
		Double_t fDiamondStrip1;
		Double_t fDiamondStrip2;
		Double_t fDiamondStrip3;
		Double_t fDiamondStrip4;
		Double_t fBeamRF;
		Double_t fDiamondAnode;
		Double_t fUpstreamScintCFD;
		Double_t fFROGOrOr;
		Double_t fFROGMaj;
		Double_t fFROGTrigger;
		Double_t fTimeReset;

		std::map<std::string, Double_t> fChannels;	// name -> value, channels that fired

	private:
		Double_t *Member(const std::string &name);

		ClassDef(TDCData, 1);
};

#endif // TDCDATA_H

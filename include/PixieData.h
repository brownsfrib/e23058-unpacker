#ifndef PIXIEDATA_H
#define PIXIEDATA_H

// C/C++ includes
#include <vector>

// ROOT includes
#include "Rtypes.h"

/**
 * !\brief Pixie-16 (DDAS) hit data. Two instances are written to the E23058
 *         tree: branch "LANA." (neutron walls A + B) and branch "VETO." (WMU VETO).
 *
 * One entry per DDAS hit in every per-hit vector (length fMult), in the
 * order the hits arrived. Raw values only, plus the channel mapping
 * (input/PIXIE/Pixie_ChannelMapping.txt). No calibration.
 * Filled by: Pixie (reader -> one unmapped PixieData with every hit) ->
 *            PixieMapping::Split (each mapped hit to LANA or VETO, mapped fields set).
 */
class PixieData{
	public:
		PixieData();
		virtual ~PixieData();

		void Clear();
		void AppendHit(const PixieData &src, int i);	// copy hit i of src (raw + mapped fields)

		// NWB = LANA neutron wall B (facing the target), NWA = LANA wall A (behind B)
		enum DetectorID { kUnmapped = 0, kNWB = 1, kNWA = 2, kVETO = 3 };

		// ---- raw, per hit
		Int_t                   fMult;		// number of hits
		std::vector<Int_t>      fCrate;
		std::vector<Int_t>      fSlot;
		std::vector<Int_t>      fChannel;	// 0-15
		std::vector<Int_t>      fEnergy;	// on-board trapezoidal filter (ADC units)
		std::vector<Double_t>   fTime;		// ns, coarse timestamp + CFD correction
		std::vector<ULong64_t>  fCoarseTime;	// ns, timestamp only
		std::vector<Int_t>      fCFD;		// raw CFD fraction
		std::vector<Int_t>      fCFDTrigSource;	// 500 MSPS: bits 31:29 of header word 2
		std::vector<Int_t>      fCFDFail;	// 1 = CFD failed (500 MSPS: trig source 7)
		std::vector<Int_t>      fFinishCode;	// 1 = pileup flagged by the module
		std::vector<Int_t>      fOverflow;	// 1 = ADC out of range
		std::vector<Int_t>      fTraceLength;	// samples (trace itself only if WRITE_TRACES 1)
		std::vector<ULong64_t>  fExtTimestamp;	// 0 if not enabled
		std::vector<std::vector<UInt_t>>   fEnergySums;	// 4 per hit, empty if not enabled
		std::vector<std::vector<UInt_t>>   fQDCSums;	// 8 per hit, empty if not enabled
		std::vector<std::vector<UShort_t>> fTrace;	// empty unless WRITE_TRACES 1

		// ---- mapped, per hit (PixieMapping)
		std::vector<Int_t>      fModule;	// slot - FIRST_SLOT (the map's "Module")
		std::vector<Int_t>      fDetector;	// DetectorID: 1 NWB, 2 NWA, 3 VETO, 0 not in map
		std::vector<Int_t>      fBar;		// bar, -1 if not in map
		std::vector<Int_t>      fSide;		// 0 = L (LANA) / T (VETO), 1 = R / B, -1 if not in map

		// ---- counts (LANA.: fNNWB + fNNWA = fMult;  VETO.: both 0)
		Int_t                   fNNWB;		// hits with fDetector == kNWB
		Int_t                   fNNWA;		// hits with fDetector == kNWA

		ClassDef(PixieData, 1);
};

#endif // PIXIEDATA_H

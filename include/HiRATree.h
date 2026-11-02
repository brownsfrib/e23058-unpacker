#ifndef HIRATREE_H
#define HIRATREE_H

// C/C++ includes
#include <string>
#include <vector>

// ROOT includes
#include "TTree.h"

// Unpacker includes
#include "HiRA.h"

/**
 * !\brief Writes the HiRA TTree (one entry per kept event).
 *
 * Construct AFTER TFile::Open so the tree belongs to that file.
 *   silicon strips : motherboard, chip, board, strip, detector code,
 *                    HG/LG energy, HINP time, theta/phi (+ uncertainties)
 *   CsI            : ADC1 and ADC2, 32 channels each
 *   TDC            : V1190 leading-edge hits, raw times
 */
class HiRATree{
	public:
		static const int kMaxStrips = 512;
		static const int kMaxTDC    = 256;

		HiRATree(HiRA &hira, const char *name = "HiRA");

		bool Build();		// copy this event into the buffers; true if anything to write
		void Fill(Long64_t eventNumber, ULong64_t timestamp);
		TTree *Tree() { return fTree; }

	private:
		HiRA  &fHiRA;
		TTree *fTree;

		Long64_t  fEventNumber;
		ULong64_t fTimestamp;

		// silicon strips
		Int_t     fNStrips;
		Int_t     fMB[kMaxStrips];
		Int_t     fChip[kMaxStrips];
		Int_t     fBoard[kMaxStrips];
		Int_t     fStrip[kMaxStrips];
		Double_t  fSiHG[kMaxStrips];
		Double_t  fSiLG[kMaxStrips];
		Double_t  fSiTime[kMaxStrips];
		Double_t  fTheta[kMaxStrips];
		Double_t  fDTheta[kMaxStrips];
		Double_t  fPhi[kMaxStrips];
		Double_t  fDPhi[kMaxStrips];
		std::vector<std::string> fDetCode;	// "" if not in geometry

		// CsI
		Double_t  fCsI1[V785::kChannels];
		Double_t  fCsI2[V785::kChannels];

		// TDC
		Int_t     fTDCMult;
		Int_t     fTDCChannel[kMaxTDC];
		Double_t  fTDCTime[kMaxTDC];
};

#endif // HIRATREE_H

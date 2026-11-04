#ifndef FROGDATA_H
#define FROGDATA_H

// C/C++ includes
#include <vector>

// ROOT includes
#include "Rtypes.h"

/**
 * !\brief FROG event data: written to the E23058 tree as branch "FROG."
 *
 * One entry per SiPM hit in every per-hit vector (length fMult).
 * Raw values are A5202 units; -10000 = not measured.
 * Filled by: FROG (reader) -> FROGGeometry (angles) -> FROGPhysics (cuts).
 */
class FROGData{
	public:
		FROGData();
		virtual ~FROGData();

		void Clear();
		void Keep(const std::vector<bool> &keep);	// drop hits where keep[i] == false

		// ---- per SiPM hit
		Int_t                 fMult;		// number of hits stored
		std::vector<Int_t>    fBoard;		// A5202 board (= source ID)
		std::vector<Int_t>    fChannel;		// 0-63
		std::vector<Double_t> fLG;		// low gain ADC
		std::vector<Double_t> fHG;		// high gain ADC
		std::vector<Double_t> fToA;		// time of arrival (LSB)
		std::vector<Double_t> fToT;		// time over threshold (LSB)
		std::vector<Double_t> fTheta;		// deg
		std::vector<Double_t> fDTheta;		// half-width, deg
		std::vector<Double_t> fPhi;		// deg, 0 = beam left, 90 = up
		std::vector<Double_t> fDPhi;		// half-width, deg
		std::vector<Int_t>    fPass;		// 1 = passes FROGPhysics cuts
		Int_t                 fNPass;		// number of hits with fPass == 1

		// ---- per board read out this event
		Int_t                 fNBoards;
		std::vector<Int_t>    fBoardID;
		std::vector<ULong64_t> fBoardTrigID;	// A5202 trigger ID
		std::vector<Double_t> fBoardTime;	// A5202 time stamp (us)

		ClassDef(FROGData, 1);
};

#endif // FROGDATA_H

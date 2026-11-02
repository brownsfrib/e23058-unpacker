#ifndef FROG_H
#define FROG_H

// C/C++ includes
#include <iostream>
#include <cstdint>
#include <map>
#include <vector>

// Project specific includes
#include "Detector.h"
#include "A5202.h"
#include "FROGGeometry.h"

// One channel of one A5202 board in one event
struct FrogHit {
	uint8_t  board    = 0;
	uint8_t  channel  = 0;
	bool     has_lg   = false;
	bool     has_hg   = false;
	bool     has_toa  = false;
	bool     has_tot  = false;
	uint16_t lg       = 0;
	uint16_t hg       = 0;
	double   toa      = 0.0;	// LSB or ns, see FrogBoard::time_in_ns
	double   tot      = 0.0;

	// angular data
	bool has_angles	  = false;
	double theta 	  = 0.0;
	double dtheta 	  = 0.0;
	double phi	  = 0.0;
	double dphi	  = 0.0;
};

// Per-board info for one event
struct FrogBoard {
	uint8_t  board         = 0;	// board byte from the A5202 payload
	uint32_t sourceID      = 0;	// sub-event-builder source ID
	uint8_t  mode          = 0;	// DataQualifier (DTQ_*)
	bool     time_in_ns    = false;
	uint64_t triggerID     = 0;
	double   tstamp_us     = 0.0;	// board timestamp
	uint64_t fragTimestamp = 0;	// event-builder timestamp of this board's fragment
};

/**
 * !\brief FROG: the A5202 (Janus / FERS-5200) array.
 *
 * FROG's own event builder glues the boards together before the global
 * one does, so a FROG fragment's payload is itself a built body whose
 * inner fragments are single boards (inner source ID = board). Each inner
 * payload is handed to the A5202 class unchanged. A FROG fragment that is
 * NOT a built body is decoded as a single A5202 payload, so files without
 * the FROG sub-builder also work.
 *
 * The A5202 run header (first item of a run, no metadata word) is stored
 * in 'headers', keyed by inner source ID; that event has no hits.
 */
class FROG : public Detector{
	public:
		FROG();
		~FROG();

		void Clear() override;
		bool Unpack(const Ring::Fragment &frag) override;
		void PrintSummary() const override;

		FROGGeometry geometry; 						// for angle stuff and shit
		bool zeroSuppress = true;
		void SetHGThreshold(uint16_t thresh); 				// common threshold to all boards
		void SetHGThreshold(int board, int channel, uint16_t thresh); 	// useful for removing noisey channels when we get to that part

		// this event
		std::vector<FrogHit>   hits;
		std::vector<FrogBoard> boards;
		bool                   sawHeader = false;

		// run level: A5202 file header per board source ID
		std::map<uint32_t, RunHeader> headers;

	private:
		bool UnpackBoard(const Ring::Fragment &frag);
		bool Keep(const FrogHit& hit) const;

		A5202 fA5202;
		std::vector<Ring::Fragment> fInner;

		long fBoardFrags = 0;
		long fBadFrags   = 0;

		static const int kBoards   = 256;	// board id is a uint8
		static const int kChannels = 64;
		uint16_t fHGThr[kBoards][kChannels];	// 0 = no HG threshold

		std::map<int, long> fReadPerBoard;	// channels read out
		std::map<int, long> fHitsPerBoard;	// channels kept
};

#endif // FROG_H

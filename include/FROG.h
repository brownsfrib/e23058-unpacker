#ifndef FROG_H
#define FROG_H

// C/C++ includes
#include <cstdint>
#include <map>
#include <vector>

// Unpacker includes
#include "Detector.h"
#include "A5202.h"
#include "FROGData.h"

/**
 * !\brief FROG reader: A5202 (Janus / FERS-5200) ring items -> FROGData.
 *
 * FROG's own event builder glues the boards together, so a FROG fragment is
 * itself a built body with one inner fragment per board. Each board payload
 * is decoded by the A5202 class and every channel it reports is appended to
 * FROGData (raw values only: no angles, no cuts - see FROGGeometry and
 * FROGPhysics). The A5202 run header (first item of a run) has no hits and
 * sets sawHeader.
 */
class FROG : public Detector{
	public:
		explicit FROG(FROGData *data);
		~FROG();

		void Clear() override;
		bool Unpack(const Ring::Fragment &frag) override;
		void PrintSummary() const override;

		bool sawHeader = false;				// this event was the A5202 run header
		std::map<uint32_t, RunHeader> headers;		// A5202 run header per board

	private:
		bool UnpackBoard(const Ring::Fragment &frag);

		FROGData *fData;
		A5202 fA5202;
		std::vector<Ring::Fragment> fInner;

		long fBoardFrags = 0;
		long fBadFrags   = 0;
		std::map<int, long> fChannelsPerBoard;
};

#endif // FROG_H

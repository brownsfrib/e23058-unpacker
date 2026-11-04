#ifndef FROGPHYSICS_H
#define FROGPHYSICS_H

// C/C++ includes
#include <map>
#include <utility>

// Unpacker includes
#include "Config.h"
#include "FROGData.h"

/**
 * !\brief FROG cuts. FROGConfig.dat [physics]:
 *
 *   ZERO_SUPPRESS   1          drop channels that did not fire: no ToA/ToT and
 *                              HG below HG_THRESHOLD (0 = keep every channel)
 *   HG_THRESHOLD    1000       zero-suppression HG threshold, all channels (0 = off)
 *   HG_THRESHOLD_CHANNEL  b c thr   per-channel override (repeatable)
 *   TOT_CUT         10         fPass requires ToT >= TOT_CUT   (< 0: no ToT cut)
 *   LG_MIN / HG_MIN 0          fPass requires LG/HG >= value   (0: no cut)
 *   DROP_FAILED     0          0 = flag with fPass only, 1 = also drop failing hits
 */
class FROGPhysics{
	public:
		bool Load(const Config &cfg);
		void Apply(FROGData &data) const;

	private:
		double HGThreshold(int board, int channel) const;

		bool   fZeroSuppress = true;
		double fHGThreshold  = 0;
		std::map<std::pair<int, int>, double> fHGThresholdChannel;
		double fToTCut       = 10;
		double fLGMin        = 0;
		double fHGMin        = 0;
		bool   fDropFailed   = false;
};

#endif // FROGPHYSICS_H

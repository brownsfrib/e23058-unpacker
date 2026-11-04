#ifndef FROGGEOMETRY_H
#define FROGGEOMETRY_H

// C/C++ includes
#include <string>
#include <vector>

// Unpacker includes
#include "Config.h"
#include "FROGData.h"

struct FrogAngles{
	bool   valid  = false;		// channel not in the file
	double theta  = 0.0;
	double dtheta = 0.0;		// half-width
	double phi    = 0.0;
	double dphi   = 0.0;		// half-width
};

/**
 * !\brief FROG geometry: (board, channel) -> theta/phi, from the .dat made by
 *         toBinary_FROG (40-byte records: int32 board, channel; double theta,
 *         dtheta, phi, dphi).
 *
 * FROGConfig.dat [geometry]:  CHANNEL_TABLE  <path to .dat>
 */
class FROGGeometry{
	public:
		bool Load(const Config &cfg);
		bool Load(const std::string &path);

		void Apply(FROGData &data) const;	// fill fTheta/fDTheta/fPhi/fDPhi (-10000 if unmapped)

		const FrogAngles &Get(int board, int channel) const;
		int Size() const { return fN; }

	private:
		static const int kBoards   = 256;
		static const int kChannels = 64;
		std::vector<FrogAngles> fTable = std::vector<FrogAngles>(kBoards * kChannels);
		FrogAngles fNone;
		int fN = 0;
};

#endif // FROGGEOMETRY_H

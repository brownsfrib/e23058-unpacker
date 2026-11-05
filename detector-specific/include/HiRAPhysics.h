#ifndef HIRAPHYSICS_H
#define HIRAPHYSICS_H

// C/C++ includes
#include <map>
#include <string>

// Unpacker includes
#include "Config.h"
#include "HiRAData.h"
#include "HiRAGeometry.h"

/**
 * !\brief HiRA reconstruction: strips + CsI -> particles (fmulti, fnumtel, ...).
 *
 * HiRAConfig.dat [physics]:
 *   SI_EF_E_RAW_THRESHOLD / SI_EB_E_RAW_THRESHOLD   raw HG a strip needs to be used
 *   SI_EF_E_THRESHOLD     / SI_EB_E_THRESHOLD       calibrated MeV a strip needs to be used
 *   CSI_E_RAW_THRESHOLD                              CsI kept in HIRA.fCsI* above this ADC
 *   STRIP_ENERGY_MATCHING_SIGMA, ..._NUMBER_OF_SIGMA front/back pair if |E_EF - E_EB|/2 < n*sigma
 *   MAX_STRIP_MULTIPLICITY                           skip telescopes with more strips per face
 *   MAX_TRACKS_PER_TELESCOPE                         best |dE| pairs kept per telescope
 *   DISABLE_CHANNEL TELE<n>STREF<s> | TELE<n>STREB<s> | TELE<n>CSI<c>,  DISABLE_ALL TELE<n>
 *
 * Steps: CsI threshold -> front/back pairing -> particles with the pixel's
 * CsI and angles. Not yet: neighbour merging, requiring a CsI hit.
 */


// A front x back strip pair that passes the energy match (indices into the HiRAData EF/EB blocks).
struct HiRAStripPair{
	double dE = 0;		// |E_EF - E_EB|, MeV
	int    f  = -1;		// index into the fEF* vectors
	int    b  = -1;		// index into the fEB* vectors
};

// A silicon track in one telescope while HiRAPhysics::Apply() builds particles.
struct HiRATrack{
	int              f    = -1;		// index into the fEF* vectors
	int              b    = -1;		// index into the fEB* vectors
	int              csi  = -1;		// CsI behind the pixel, 0-3 (-1 if none)
	double           eF   = 0;		// front energy, MeV (summed if merged)
	double           eB   = 0;		// back energy, MeV (summed if merged)
	const HiRAPixel *px   = nullptr;	// pixel angles
	bool             keep = true;		// false = merged away or rejected
};


class HiRAPhysics{
	public:
		static const int kTelescopes 	= 12;
		static const int kStrips 	= 32; 
		static const int kCrystals 	= 4;

		explicit HiRAPhysics(const HiRAGeometry &geo) : fGeo(geo) {}
		bool Load(const Config &cfg);
		void Apply(HiRAData &data);
		void PrintSummary() const;
		static bool SmallerDE(const HiRAStripPair &a, const HiRAStripPair &b);


	private:
		bool StripEnabled(int t, int face, int s) const {
			return t >= 0 && t < kTelescopes && s >= 0 && s < kStrips && fStripOn[t][face][s];
		}
		bool CsIEnabled(int t, int c) const {
			return t >= 0 && t < kTelescopes && c >= 0 && c < kCrystals && fCsIOn[t][c];
		}

		const HiRAGeometry &fGeo;

		double 	fEFRawThresh 		= 0;
		double	fEBRawThresh 		= 0;
		double	fEFThresh 		= 0;
		double	fEBThresh 		= 0;
		double	fCsIRawThresh 		= 0;
		double 	fMatchSigma 		= 0.020;
		double	fMatchNSigma 		= 5;
		int     fMaxStripMult 		= 8;
		int	fMaxTracks 		= 4;
		double  fNeighbourDist 		= 1.41522; // sqrt(2) woooo we need a quake 3 algo in here
		bool    fRequireCsI		= true;
		bool    fStripOn[kTelescopes][2][kStrips];
		bool    fCsIOn[kTelescopes][kCrystals];

		std::map<std::string, long> fCount;
};

#endif // HIRAPHYSICS_H

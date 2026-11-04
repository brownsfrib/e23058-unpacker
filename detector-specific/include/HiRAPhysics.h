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
class HiRAPhysics{
	public:
		static const int kTelescopes = 12, kStrips = 32, kCrystals = 4;

		explicit HiRAPhysics(const HiRAGeometry &geo) : fGeo(geo) {}
		bool Load(const Config &cfg);
		void Apply(HiRAData &data);
		void PrintSummary() const;

	private:
		bool StripEnabled(int t, int face, int s) const { return t >= 0 && t < kTelescopes && s >= 0 && s < kStrips && fStripOn[t][face][s]; }
		bool CsIEnabled(int t, int c) const { return t >= 0 && t < kTelescopes && c >= 0 && c < kCrystals && fCsIOn[t][c]; }

		const HiRAGeometry &fGeo;

		double fEFRawThresh = 0, fEBRawThresh = 0, fEFThresh = 0, fEBThresh = 0, fCsIRawThresh = 0;
		double fMatchSigma = 0.020, fMatchNSigma = 5;
		int    fMaxStripMult = 8, fMaxTracks = 4;
		bool   fStripOn[kTelescopes][2][kStrips];
		bool   fCsIOn[kTelescopes][kCrystals];

		std::map<std::string, long> fCount;
};

#endif // HIRAPHYSICS_H

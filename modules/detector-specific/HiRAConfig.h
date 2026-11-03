#ifndef HIRACONFIG_H
#define HIRACONFIG_H

// C/C++ includes
#include <string>
#include <vector>

/**
 * !\brief Analysis settings from ConfigHira.dat (NPTool token format).  SKELETON
 *
 * Telescopes are P0-P11. Faces: 0 = EF (front), 1 = EB (back).
 */
class HiRAConfig{
	public:
		static const int kTelescopes = 12;
		static const int kStrips     = 32;
		static const int kCrystals   = 4;

		HiRAConfig();
		bool Load(const std::string &path);	// TODO step 1
		void Print() const;			// TODO step 1

		// channel status (DISABLE_CHANNEL / DISABLE_ALL)
		bool StripEnabled(int telescope, int face, int strip) const;
		bool CsIEnabled(int telescope, int crystal) const;

		// Si -> CsI: crystal 0-3 whose box contains (ef, eb), -1 if none
		int  CrystalFor(int ef, int eb) const;

		// pairing / recombination
		double matchSigma        = 0.020;	// STRIP_ENERGY_MATCHING_SIGMA (MeV)
		double matchNSigma       = 5;		// STRIP_ENERGY_MATCHING_NUMBER_OF_SIGMA
		int    maxStripMult      = 8;		// MAX_STRIP_MULTIPLICITY (per face, per telescope)
		double neighbourDistance = 1.41422;	// NEIGHBOUR_DISTANCE (strip units)

		// CsI matching
		double csiSize = 16;			// CSI_SIZE
		double csiCentreEF[kCrystals] = {7.5, 7.5, 23.5, 23.5};	// CSI_CENTRE c ef eb
		double csiCentreEB[kCrystals] = {23.5, 7.5, 7.5, 23.5};
		bool   ignoreNotMatchingCsI = true;	// IGNORE_NOT_MATCHING_CSI

		// thresholds
		double hgSaturation  = 7900;		// HG_SATURATION (raw)
		double efRawThresh   = 0, ebRawThresh = 0;	// SI_EF/EB_E_RAW_THRESHOLD
		double efThresh      = 0, ebThresh    = 0;	// SI_EF/EB_E_THRESHOLD (MeV)
		double csiRawThresh  = 0;		// CSI_E_RAW_THRESHOLD

	private:
		bool fStrip[kTelescopes][2][kStrips];
		bool fCsI[kTelescopes][kCrystals];
};

#endif // HIRACONFIG_H

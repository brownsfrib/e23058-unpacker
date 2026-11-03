#ifndef HIRACALIBRATION_H
#define HIRACALIBRATION_H

// C/C++ includes
#include <string>

/**
 * !\brief Strip energy calibration from HiRA_strip_calibration.txt.  SKELETON
 *
 *   telescope face(EF/EB) strip hg_slope hg_offset lg_slope lg_offset
 *   E [MeV] = slope * raw + offset; HG unless raw HG >= hgSaturation, then LG.
 * (CsI calibration is non-linear and Z/A dependent - a separate class later.)
 */
class HiRACalibration{
	public:
		static const int kTelescopes = 12;
		static const int kStrips     = 32;

		HiRACalibration();
		bool Load(const std::string &path);	// TODO step 1

		// calibrated energy (MeV); usedLG reports which gain was used.
		// Returns false if the strip has no calibration.
		bool Energy(int telescope, int face, int strip, double rawHG, double rawLG,
		            double hgSaturation, double &energy, bool &usedLG) const;	// TODO step 1

	private:
		struct Coeff { bool valid = false; double hgSlope = 0, hgOffset = 0, lgSlope = 0, lgOffset = 0; };
		Coeff fCoeff[kTelescopes][2][kStrips];
};

#endif // HIRACALIBRATION_H

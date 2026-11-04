#ifndef HIRACALIBRATION_H
#define HIRACALIBRATION_H

// C/C++ includes
#include <string>

// Unpacker includes
#include "Config.h"
#include "HiRAData.h"

/**
 * !\brief HiRA silicon strip energy calibration (CsI is not calibrated).
 *
 * HiRAConfig.dat [calibration]:
 *   STRIP_CALIBRATION  <file>   lines: telescope face(EF/EB) strip hg_slope hg_offset lg_slope lg_offset
 *   HG_SATURATION      7900     raw HG at/above this -> use the LG calibration
 * E [MeV] = slope * raw + offset.  Fills fEFEnergyCal / fEBEnergyCal (-10000 if no calibration).
 */
class HiRACalibration{
	public:
		static const int kTelescopes = 12, kStrips = 32;

		bool Load(const Config &cfg);
		void Apply(HiRAData &data) const;

		// false if the strip has no calibration
		bool Energy(int telescope, int face, int strip, double rawHG, double rawLG, double &energy) const;

	private:
		struct Coeff { bool valid = false; double hgSlope = 0, hgOffset = 0, lgSlope = 0, lgOffset = 0; };
		Coeff  fCoeff[kTelescopes][2][kStrips];
		double fHGSaturation = 7900;
};

#endif // HIRACALIBRATION_H

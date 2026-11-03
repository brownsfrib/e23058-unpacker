#ifndef HIRAPHYSICS_H
#define HIRAPHYSICS_H

// C/C++ includes
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Unpacker includes
#include "HiRA.h"
#include "HiRAGeometry.h"
#include "HiRAConfig.h"
#include "HiRACalibration.h"

// ---- one silicon strip after pre-treatment (mapped, enabled, above threshold, calibrated)
struct HiRAStripHit {
	int    telescope 	= -1;
	int    face      	= -1;		// 0 = EF, 1 = EB
	int    strip     	= -1;		// 0-31 on that face
	double rawHG 		= 0;
	double rawLG 		= 0;
	bool   usedLG    	= false;
	double energy    	= 0;		// MeV
	double time      	= 0;		// HINP time (raw)
};

// ---- one CsI crystal that fired
struct HiRACsIHit {
	int    id        	= -1;		// global 4*telescope + crystal
	int    telescope 	= -1;
	int    crystal   	= -1;		// 0-3
	double raw       	= 0;		// V785 value
	double time      	= -1;		// V1190 time (raw), -1 if no TDC hit
};

// ---- a front x back pair (a pixel), possibly merged with neighbours
struct HiRAPixelHit {
	int	telescope 	= -1;
	int	ef 		= -1;
	int	eb 		= -1;
	double 	efEnergy 	= 0;
	double 	ebEnergy 	= 0;
	double 	efTime 		= 0;
	double 	ebTime 		= 0;
	int    	crystal   	= -1;		// from HiRAConfig::CrystalFor(ef, eb)
	int    	nMerged   	= 1;		// pixels combined into this one
};

// ---- final output: one particle
struct HiRATrack {
	int	telescope 	= -1;
	int	ef 		= -1;
	int	eb 		= -1;
	double 	efEnergy 	= 0; 
	double	ebEnergy 	= 0;
	double	siEnergy 	= 0;	// siEnergy = EF energy (TAKE_E_EF)
	double 	siTime   	= 0;
	int    	csiId    	= -1;		// global CsI id, -1 if none matched
	double 	csiRaw   	= 0;
	double 	csiTime  	= -1;
	double 	theta 		= 0;
	double 	phi 		= 0;
	double	dtheta 		= 0;
	double	dphi 		= 0;	// from the pixel table
};

/**
 * !\brief HiRA physics reconstruction: raw HiRA event -> particle tracks.  SKELETON
 *
 * Port of THiraPhysics::BuildSimplePhysicalEvent() for real data:
 *   1. PreTreat        strips -> (telescope, face, strip), channel status, raw thresholds,
 *                      calibration, calibrated thresholds; CsI -> crystal, raw threshold
 *   2. PairFrontBack   EF x EB within a telescope by energy (|E_EF - E_EB| / 2 < n sigma)
 *                      (new: the simulation got the pairing for free from Geant4 ordering)
 *   3. CombineNeighbours  merge pixels within neighbourDistance in the same crystal
 *   4. MatchCsI        pixel -> crystal; a crystal with > 1 pixel is ambiguous -> dropped
 *   5. AssignAngles    theta/phi from the pixel table
 */
class HiRAPhysics{
	public:
		HiRAPhysics(const HiRAGeometry &geo, const HiRAConfig &cfg, const HiRACalibration &cal);

		void Clear();
		void Build(const HiRA &hira);		// runs steps 1-5
		void PrintSummary() const;

		// results of the last Build()
		std::vector<HiRAStripHit> strips;	// after step 1
		std::vector<HiRACsIHit>   csi;		// after step 1
		std::vector<HiRAPixelHit> pixels;	// after steps 2-3
		std::vector<HiRATrack>    tracks;	// final

	private:
		void PreTreat(const HiRA &hira);	// TODO step 2
		void PairFrontBack();			// TODO step 3
		void CombineNeighbours();		// TODO step 4
		void MatchCsI();			// TODO step 5
		void AssignAngles();			// TODO step 6

		const HiRAGeometry    &fGeo;
		const HiRAConfig      &fCfg;
		const HiRACalibration &fCal;

		// counters for PrintSummary(): how many hits were lost at which step
		std::map<std::string, long> fCount;
};

#endif // HIRAPHYSICS_H

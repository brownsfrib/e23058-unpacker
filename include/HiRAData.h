#ifndef HIRADATA_H
#define HIRADATA_H

// C/C++ includes
#include <vector>

// ROOT includes
#include "Rtypes.h"

/**
 * !\brief HiRA event data: written to the E23058 tree as branch "HIRA."
 *
 * Names follow the E15190 HiRA classes. Every block is flat: each entry
 * carries its own telescope number (P0-P11). -10000 = not measured.
 * Filled by: HiRAGeometry (electronics -> telescope/strip, CsI) ->
 *            HiRACalibration (strip energies) -> HiRAPhysics (particles).
 */
class HiRAData{
	public:
		HiRAData();
		virtual ~HiRAData();

		void Clear();

		// ---- front strips (EF) that fired; one entry per strip
		Int_t                 fEFmulti;
		std::vector<Int_t>    fEFnumtel;	// telescope
		std::vector<Int_t>    fEFnumstrip;	// 0-31
		std::vector<Int_t>    fEFmb;		// motherboard (electronics address)
		std::vector<Int_t>    fEFchip;		// HINP chip  (electronics address)
		std::vector<Double_t> fEFEnergyHi;	// raw high gain
		std::vector<Double_t> fEFEnergyLo;	// raw low gain
		std::vector<Double_t> fEFTime;		// raw HINP time
		std::vector<Double_t> fEFEnergyCal;	// MeV, -10000 if not calibrated

		// ---- back strips (EB), same layout
		Int_t                 fEBmulti;
		std::vector<Int_t>    fEBnumtel;
		std::vector<Int_t>    fEBnumstrip;
		std::vector<Int_t>    fEBmb;
		std::vector<Int_t>    fEBchip;
		std::vector<Double_t> fEBEnergyHi;
		std::vector<Double_t> fEBEnergyLo;
		std::vector<Double_t> fEBTime;
		std::vector<Double_t> fEBEnergyCal;

		// ---- CsI above threshold; one entry per crystal
		Int_t                 fCsImulti;
		std::vector<Int_t>    fCsInumtel;	// telescope
		std::vector<Int_t>    fCsInumcsi;	// 0-3 within the telescope
		std::vector<Double_t> fCsIRawADC;	// V785 value (not calibrated)
		std::vector<Double_t> fCsITime;		// raw V1190 time, -10000 if no hit

		// ---- reconstructed particles; one entry per particle
		Int_t                 fmulti;
		std::vector<Int_t>    fnumtel;
		std::vector<Int_t>    fnumstripf;	// front strip
		std::vector<Int_t>    fnumstripb;	// back strip
		std::vector<Int_t>    fnumcsi;		// CsI behind the pixel, 0-3 (-1 if none)
		std::vector<Double_t> fEnergySifHi;
		std::vector<Double_t> fEnergySifLo;
		std::vector<Double_t> fTimeSif;
		std::vector<Double_t> fEnergySifCal;	// MeV
		std::vector<Double_t> fEnergySibHi;
		std::vector<Double_t> fEnergySibLo;
		std::vector<Double_t> fTimeSib;
		std::vector<Double_t> fEnergySibCal;	// MeV
		std::vector<Double_t> fCsIADC;		// raw V785 of fnumcsi, -10000 if it did not fire
		std::vector<Double_t> fTimecsi;		// raw V1190 time of fnumcsi, -10000 if none
		std::vector<Double_t> fTheta;		// deg, from the pixel
		std::vector<Double_t> fPhi;		// deg, 0 = beam left, 90 = up
		std::vector<Double_t> fdTheta;		// half-width, deg
		std::vector<Double_t> fdPhi;		// half-width, deg

		ClassDef(HiRAData, 1);
};

#endif // HIRADATA_H

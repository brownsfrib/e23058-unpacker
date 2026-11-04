#include "HiRAData.h"

HiRAData::HiRAData(){
	Clear();
}

HiRAData::~HiRAData(){}

void HiRAData::Clear(){
	fEFmulti = 0;
	fEFnumtel.clear();  fEFnumstrip.clear();  fEFmb.clear();  fEFchip.clear();
	fEFEnergyHi.clear(); fEFEnergyLo.clear(); fEFTime.clear(); fEFEnergyCal.clear();

	fEBmulti = 0;
	fEBnumtel.clear();  fEBnumstrip.clear();  fEBmb.clear();  fEBchip.clear();
	fEBEnergyHi.clear(); fEBEnergyLo.clear(); fEBTime.clear(); fEBEnergyCal.clear();

	fCsImulti = 0;
	fCsInumtel.clear();  fCsInumcsi.clear();  fCsIRawADC.clear();  fCsITime.clear();

	fmulti = 0;
	fnumtel.clear();  fnumstripf.clear();  fnumstripb.clear();  fnumcsi.clear();
	fEnergySifHi.clear();  fEnergySifLo.clear();  fTimeSif.clear();  fEnergySifCal.clear();
	fEnergySibHi.clear();  fEnergySibLo.clear();  fTimeSib.clear();  fEnergySibCal.clear();
	fCsIADC.clear();  fTimecsi.clear();
	fTheta.clear();  fPhi.clear();  fdTheta.clear();  fdPhi.clear();
}

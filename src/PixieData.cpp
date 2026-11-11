#include "PixieData.h"

PixieData::PixieData(){
	Clear();
}

PixieData::~PixieData(){}

void PixieData::Clear(){
	fMult = 0;
	fCrate.clear();       fSlot.clear();          fChannel.clear();
	fEnergy.clear();      fTime.clear();          fCoarseTime.clear();
	fCFD.clear();         fCFDTrigSource.clear(); fCFDFail.clear();
	fFinishCode.clear();  fOverflow.clear();      fTraceLength.clear();
	fExtTimestamp.clear();
	fEnergySums.clear();  fQDCSums.clear();       fTrace.clear();

	fModule.clear();      fBar.clear();           fSide.clear();
}

void PixieData::AppendHit(const PixieData &s, int i){
	fCrate.push_back(s.fCrate[i]);             fSlot.push_back(s.fSlot[i]);
	fChannel.push_back(s.fChannel[i]);         fEnergy.push_back(s.fEnergy[i]);
	fTime.push_back(s.fTime[i]);               fCoarseTime.push_back(s.fCoarseTime[i]);
	fCFD.push_back(s.fCFD[i]);                 fCFDTrigSource.push_back(s.fCFDTrigSource[i]);
	fCFDFail.push_back(s.fCFDFail[i]);         fFinishCode.push_back(s.fFinishCode[i]);
	fOverflow.push_back(s.fOverflow[i]);       fTraceLength.push_back(s.fTraceLength[i]);
	fExtTimestamp.push_back(s.fExtTimestamp[i]);
	fEnergySums.push_back(s.fEnergySums[i]);   fQDCSums.push_back(s.fQDCSums[i]);
	fTrace.push_back(s.fTrace[i]);
	fModule.push_back(s.fModule[i]);           fBar.push_back(s.fBar[i]);
	fSide.push_back(s.fSide[i]);
	fMult = (Int_t)fCrate.size();
}

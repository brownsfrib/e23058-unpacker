#include "TDCData.h"

static const Double_t kNone = -10000;

TDCData::TDCData(){
	Clear();
}

TDCData::~TDCData(){}

void TDCData::Clear(){
	fMasterTrigger = fDiamondStrip0 = fDiamondStrip1 = fDiamondStrip2 = fDiamondStrip3 =
	fDiamondStrip4 = fBeamRF = fDiamondAnode = fUpstreamScintCFD =
	fFROGOrOr = fFROGMaj = fFROGTrigger = fTimeReset = kNone;
	fChannels.clear();
}

Double_t *TDCData::Member(const std::string &name){
	if (name == "fMasterTrigger")    return &fMasterTrigger;
	if (name == "fDiamondStrip0")    return &fDiamondStrip0;
	if (name == "fDiamondStrip1")    return &fDiamondStrip1;
	if (name == "fDiamondStrip2")    return &fDiamondStrip2;
	if (name == "fDiamondStrip3")    return &fDiamondStrip3;
	if (name == "fDiamondStrip4")    return &fDiamondStrip4;
	if (name == "fBeamRF")           return &fBeamRF;
	if (name == "fDiamondAnode")     return &fDiamondAnode;
	if (name == "fUpstreamScintCFD") return &fUpstreamScintCFD;
	if (name == "fFROGOrOr")         return &fFROGOrOr;
	if (name == "fFROGMaj")          return &fFROGMaj;
	if (name == "fFROGTrigger")      return &fFROGTrigger;
	if (name == "fTimeReset")        return &fTimeReset;
	return nullptr;
}

bool TDCData::Set(const std::string &name, Double_t value){
	Double_t *m = Member(name);
	if (!m){
		return false;
	}
	*m = value;
	fChannels[name] = value;
	return true;
}

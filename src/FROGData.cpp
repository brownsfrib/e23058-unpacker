#include "FROGData.h"

FROGData::FROGData(){
	Clear();
}

FROGData::~FROGData(){}

void FROGData::Clear(){
	fMult = 0;
	fBoard.clear();  fChannel.clear();
	fLG.clear();     fHG.clear();     fToA.clear();  fToT.clear();
	fTheta.clear();  fDTheta.clear(); fPhi.clear();  fDPhi.clear();
	fPass.clear();
	fNPass = 0;

	fNBoards = 0;
	fBoardID.clear();  fBoardTrigID.clear();  fBoardTime.clear();
}

template<class T> static void keepOnly(std::vector<T> &v, const std::vector<bool> &keep){
	size_t j = 0;
	for (size_t i = 0; i < v.size(); i++){
		if (keep[i]){
			v[j++] = v[i];
		}
	}
	v.resize(j);
}

void FROGData::Keep(const std::vector<bool> &keep){
	keepOnly(fBoard, keep);  keepOnly(fChannel, keep);
	keepOnly(fLG, keep);     keepOnly(fHG, keep);     keepOnly(fToA, keep);  keepOnly(fToT, keep);
	keepOnly(fTheta, keep);  keepOnly(fDTheta, keep); keepOnly(fPhi, keep);  keepOnly(fDPhi, keep);
	keepOnly(fPass, keep);
	fMult = (Int_t)fBoard.size();
	fNPass = 0;
	for (size_t i = 0; i < fPass.size(); i++){
		fNPass += fPass[i];
	}
}

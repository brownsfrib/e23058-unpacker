#include "FROGGeometry.h"

#include <cstdint>
#include <fstream>
#include <iostream>

static const double kNone = -10000;

bool FROGGeometry::Load(const Config &cfg){
	std::string path = cfg.GetString("geometry", "CHANNEL_TABLE");
	if (path.empty()){
		std::cerr << "\033[1;31mFROGGeometry: no [geometry] CHANNEL_TABLE in " << cfg.File() << "\033[0m\n";
		return false;
	}
	return Load(path);
}

bool FROGGeometry::Load(const std::string &path){
	std::ifstream fin(path.c_str(), std::ios::binary);
	if (!fin){
		std::cerr << "\033[1;31mFROGGeometry: cannot open " << path << "\033[0m\n";
		return false;
	}
	int32_t b, c;
	double th, dth, ph, dph;
	fN = 0;
	while (fin.read((char*)&b, sizeof b) && fin.read((char*)&c, sizeof c) &&
	       fin.read((char*)&th, sizeof th) && fin.read((char*)&dth, sizeof dth) &&
	       fin.read((char*)&ph, sizeof ph) && fin.read((char*)&dph, sizeof dph)){
		if (b < 0 || b >= kBoards || c < 0 || c >= kChannels){
			std::cerr << "FROGGeometry: skipping board " << b << " channel " << c << "\n";
			continue;
		}
		FrogAngles &a = fTable[b * kChannels + c];
		a.valid = true;
		a.theta = th;  a.dtheta = dth;  a.phi = ph;  a.dphi = dph;
		fN++;
	}
	std::cout << "FROGGeometry: loaded " << fN << " channels from " << path << "\n";
	return fN > 0;
}

const FrogAngles &FROGGeometry::Get(int board, int channel) const{
	if (board < 0 || board >= kBoards || channel < 0 || channel >= kChannels){
		return fNone;
	}
	return fTable[board * kChannels + channel];
}

void FROGGeometry::Apply(FROGData &d) const{
	for (int i = 0; i < d.fMult; i++){
		const FrogAngles &a = Get(d.fBoard[i], d.fChannel[i]);
		d.fTheta[i]  = a.valid ? a.theta  : kNone;
		d.fDTheta[i] = a.valid ? a.dtheta : kNone;
		d.fPhi[i]    = a.valid ? a.phi    : kNone;
		d.fDPhi[i]   = a.valid ? a.dphi   : kNone;
	}
}

#include "HiRACalibration.h"

#include <fstream>
#include <iostream>
#include <sstream>

bool HiRACalibration::Load(const Config &cfg){
	fHGSaturation = cfg.GetDouble("calibration", "HG_SATURATION", 7900);
	std::string path = cfg.GetString("calibration", "STRIP_CALIBRATION");
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mHiRACalibration: cannot open strip calibration '" << path << "'\033[0m\n";
		return false;
	}
	std::string line;
	int n = 0, lineNo = 0;
	while (std::getline(fin, line)){
		lineNo++;
		size_t first = line.find_first_not_of(" \t\r");
		if (first == std::string::npos || line[first] == '#'){
			continue;
		}
		std::istringstream ss(line);
		int t, s;
		std::string face;
		Coeff c;
		if (!(ss >> t >> face >> s >> c.hgSlope >> c.hgOffset >> c.lgSlope >> c.lgOffset)){
			std::cerr << "HiRACalibration: line " << lineNo << " could not be parsed - skipped\n";
			continue;
		}
		int f = (face == "EF") ? 0 : (face == "EB") ? 1 : -1;
		if (t < 0 || t >= kTelescopes || f < 0 || s < 0 || s >= kStrips){
			std::cerr << "HiRACalibration: line " << lineNo << " out of range - skipped\n";
			continue;
		}
		c.valid = true;
		fCoeff[t][f][s] = c;
		n++;
	}
	std::cout << "HiRACalibration: loaded " << n << " strips from " << path << "\n";
	return n > 0;
}

bool HiRACalibration::Energy(int t, int f, int s, double rawHG, double rawLG, double &energy) const{
	if (t < 0 || t >= kTelescopes || f < 0 || f > 1 || s < 0 || s >= kStrips || !fCoeff[t][f][s].valid){
		return false;
	}
	const Coeff &c = fCoeff[t][f][s];
	energy = (rawHG >= fHGSaturation) ? c.lgSlope * rawLG + c.lgOffset : c.hgSlope * rawHG + c.hgOffset;
	return true;
}

void HiRACalibration::Apply(HiRAData &d) const{
	double e;
	for (int i = 0; i < d.fEFmulti; i++){
		if (Energy(d.fEFnumtel[i], 0, d.fEFnumstrip[i], d.fEFEnergyHi[i], d.fEFEnergyLo[i], e)){
			d.fEFEnergyCal[i] = e;
		}
	}
	for (int i = 0; i < d.fEBmulti; i++){
		if (Energy(d.fEBnumtel[i], 1, d.fEBnumstrip[i], d.fEBEnergyHi[i], d.fEBEnergyLo[i], e)){
			d.fEBEnergyCal[i] = e;
		}
	}
}

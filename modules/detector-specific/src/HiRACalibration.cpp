#include "HiRACalibration.h"

#include <fstream>
#include <iostream>
#include <sstream>

HiRACalibration::HiRACalibration(){}

bool HiRACalibration::Load(const std::string &path){
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mHiRACalibration: cannot open " << path << "\033[0m\n";
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
			std::cerr << "HiRACalibration line " << lineNo << ": could not parse - skipped\n";
			continue;
		}
		int f = (face == "EF") ? 0 : (face == "EB") ? 1 : -1;
		if (t < 0 || t >= kTelescopes || f < 0 || s < 0 || s >= kStrips){
			std::cerr << "HiRACalibration line " << lineNo << ": out of range - skipped\n";
			continue;
		}
		c.valid = true;
		fCoeff[t][f][s] = c;
		n++;
	}
	std::cout << "HiRACalibration: loaded " << n << " strips from " << path << "\n";
	return n > 0;
}

bool HiRACalibration::Energy(int telescope, int face, int strip, double rawHG, double rawLG,
                             double hgSaturation, double &energy, bool &usedLG) const{
	energy = 0;
	usedLG = false;
	if (telescope < 0 || telescope >= kTelescopes || face < 0 || face > 1 || strip < 0 || strip >= kStrips){
		return false;
	}
	const Coeff &c = fCoeff[telescope][face][strip];
	if (!c.valid){
		return false;
	}
	usedLG = (rawHG >= hgSaturation);
	energy = usedLG ? c.lgSlope * rawLG + c.lgOffset : c.hgSlope * rawHG + c.hgOffset;
	return true;
}

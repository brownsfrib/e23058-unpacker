#include "HiRAConfig.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

HiRAConfig::HiRAConfig(){
	for (int t = 0; t < kTelescopes; t++){
		for (int f = 0; f < 2; f++){
			for (int s = 0; s < kStrips; s++){
				fStrip[t][f][s] = true;
			}
		}
		for (int c = 0; c < kCrystals; c++){
			fCsI[t][c] = true;
		}
	}
}

// "TELE10STREF13" -> telescope 10, rest "STREF13". Returns false if malformed.
static bool splitTele(const std::string &token, int &tele, std::string &rest){
	if (token.compare(0, 4, "TELE") != 0){
		return false;
	}
	size_t i = 4;
	while (i < token.size() && isdigit((unsigned char)token[i])){
		i++;
	}
	if (i == 4){
		return false;
	}
	tele = std::atoi(token.substr(4, i - 4).c_str());
	rest = token.substr(i);
	return tele >= 0 && tele < HiRAConfig::kTelescopes;
}

bool HiRAConfig::Load(const std::string &path){
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mHiRAConfig: cannot open " << path << " - using defaults\033[0m\n";
		return false;
	}

	bool reading = false;
	std::string line;
	int lineNo = 0;
	while (std::getline(fin, line)){
		lineNo++;
		size_t pct = line.find('%');		// '%' starts a comment anywhere on the line
		if (pct != std::string::npos){
			line = line.substr(0, pct);
		}
		std::istringstream ss(line);
		std::string key;
		if (!(ss >> key)){
			continue;
		}
		if (key == "ConfigHira"){
			reading = true;
			continue;
		}
		if (!reading){
			continue;
		}

		double v;
		if      (key == "STRIP_ENERGY_MATCHING_SIGMA"           && ss >> v) matchSigma        = v;
		else if (key == "STRIP_ENERGY_MATCHING_NUMBER_OF_SIGMA" && ss >> v) matchNSigma       = v;
		else if (key == "MAX_STRIP_MULTIPLICITY"                && ss >> v) maxStripMult      = (int)v;
		else if (key == "NEIGHBOUR_DISTANCE"                    && ss >> v) neighbourDistance = v;
		else if (key == "CSI_SIZE"                              && ss >> v) csiSize           = v;
		else if (key == "HG_SATURATION"                         && ss >> v) hgSaturation      = v;
		else if (key == "SI_EF_E_RAW_THRESHOLD"                 && ss >> v) efRawThresh       = v;
		else if ((key == "SI_EB_E_RAW_THRESHOLD" || key == "SI_EF_EB_RAW_THRESHOLD") && ss >> v) ebRawThresh = v;
		else if (key == "SI_EF_E_THRESHOLD"                     && ss >> v) efThresh          = v;
		else if (key == "SI_EB_E_THRESHOLD"                     && ss >> v) ebThresh          = v;
		else if (key == "CSI_E_RAW_THRESHOLD"                   && ss >> v) csiRawThresh      = v;
		else if (key == "IGNORE_NOT_MATCHING_CSI")                          ignoreNotMatchingCsI = true;
		else if (key == "CSI_CENTRE"){
			int c; double ef, eb;
			if (ss >> c >> ef >> eb && c >= 0 && c < kCrystals){
				csiCentreEF[c] = ef;
				csiCentreEB[c] = eb;
			}
			else {
				std::cerr << "HiRAConfig line " << lineNo << ": bad CSI_CENTRE\n";
			}
		}
		else if (key == "DISABLE_ALL" || key == "DISABLE_CHANNEL"){
			std::string tok, rest;
			int t;
			if (!(ss >> tok) || !splitTele(tok, t, rest)){
				std::cerr << "HiRAConfig line " << lineNo << ": bad " << key << " '" << tok << "'\n";
				continue;
			}
			if (key == "DISABLE_ALL"){
				for (int f = 0; f < 2; f++) for (int s = 0; s < kStrips; s++) fStrip[t][f][s] = false;
				for (int c = 0; c < kCrystals; c++) fCsI[t][c] = false;
			}
			else if (rest.compare(0, 5, "STREF") == 0 || rest.compare(0, 5, "STREB") == 0){
				int face = (rest[4] == 'F') ? 0 : 1;
				int s = std::atoi(rest.substr(5).c_str());
				if (s >= 0 && s < kStrips) fStrip[t][face][s] = false;
			}
			else if (rest.compare(0, 3, "CSI") == 0){
				int c = std::atoi(rest.substr(3).c_str());
				if (c >= 0 && c < kCrystals) fCsI[t][c] = false;
			}
			else {
				std::cerr << "HiRAConfig line " << lineNo << ": unknown channel type '" << rest << "'\n";
			}
		}
		else {
			std::cerr << "HiRAConfig line " << lineNo << ": unknown or incomplete key '" << key << "' - ignored\n";
		}
	}
	return true;
}

bool HiRAConfig::StripEnabled(int telescope, int face, int strip) const{
	if (telescope < 0 || telescope >= kTelescopes || face < 0 || face > 1 || strip < 0 || strip >= kStrips){
		return false;
	}
	return fStrip[telescope][face][strip];
}

bool HiRAConfig::CsIEnabled(int telescope, int crystal) const{
	if (telescope < 0 || telescope >= kTelescopes || crystal < 0 || crystal >= kCrystals){
		return false;
	}
	return fCsI[telescope][crystal];
}

int HiRAConfig::CrystalFor(int ef, int eb) const{
	for (int c = 0; c < kCrystals; c++){
		if (std::fabs(csiCentreEF[c] - ef) < csiSize / 2.0 &&
		    std::fabs(csiCentreEB[c] - eb) < csiSize / 2.0){
			return c;
		}
	}
	return -1;
}

void HiRAConfig::Print() const{
	std::cout << "HiRAConfig: match sigma " << matchSigma << " x " << matchNSigma
	          << ", max strip mult " << maxStripMult << ", neighbour distance " << neighbourDistance
	          << ", CsI size " << csiSize << ", HG saturation " << hgSaturation << "\n"
	          << "  thresholds raw EF/EB/CsI " << efRawThresh << "/" << ebRawThresh << "/" << csiRawThresh
	          << ", calibrated EF/EB " << efThresh << "/" << ebThresh << " MeV\n";
	int off = 0;
	for (int t = 0; t < kTelescopes; t++){
		for (int f = 0; f < 2; f++) for (int s = 0; s < kStrips; s++) off += !fStrip[t][f][s];
		for (int c = 0; c < kCrystals; c++) off += !fCsI[t][c];
	}
	std::cout << "  disabled channels: " << off << "\n";
}
